/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/ScopeGuard.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Devices/GPU/AMD/Atom/Bios.h>
#include <Kernel/Devices/GPU/AMD/Atom/Interpreter.h>
#include <Kernel/Devices/GPU/AMD/NativeGraphicsAdapter.h>
#include <Kernel/Library/StdLib.h>

namespace Kernel::Graphics::AMD::Atom {

ErrorOr<NonnullOwnPtr<Bios>> Bios::try_create(AMDNativeGraphicsAdapter& adapter)
{
    if (auto bios = try_create_from_expansion_rom(adapter); bios.is_error()) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "Failed to read VBIOS from PCI expansion ROM: {}", bios.error());
    } else {
        dmesgln_pci(adapter, "Loaded VBIOS from PCI expansion ROM");
        return bios.release_value();
    }

    return Error::from_errno(ENXIO);
}

Bios::Bios(NonnullOwnPtr<KBuffer>&& bios)
    : m_bios(move(bios))
{
}

ErrorOr<NonnullOwnPtr<Bios>> Bios::try_create_from_kbuffer(NonnullOwnPtr<KBuffer>&& bios_buffer)
{
    auto bios = TRY(adopt_nonnull_own_or_enomem(new (nothrow) Bios(move(bios_buffer))));
    if (!bios->is_valid()) {
        return Error::from_errno(EIO);
    }
    TRY(bios->index_iio());
    return bios;
}

ErrorOr<NonnullOwnPtr<Bios>> Bios::try_create_from_expansion_rom(AMDNativeGraphicsAdapter& adapter)
{
    const size_t size = PCI::get_expansion_rom_space_size(adapter.device_identifier());
    if (size == 0)
        return Error::from_errno(ENXIO);

    SpinlockLocker locker(adapter.device_identifier().operation_lock());

    // TODO: There might be some conflicts here with the DeviceExpansionROM sysfs component.
    // Its probably fine for now because this just maps and unmaps it really quickly at a
    // moment that that driver is not mapping it.
    auto expansion_rom_ptr = PCI::read32_locked(adapter.device_identifier(), PCI::RegisterOffset::EXPANSION_ROM_POINTER);
    if (expansion_rom_ptr == 0)
        return Error::from_errno(ENXIO);

    ScopeGuard unmap_rom_on_return([&] {
        PCI::write32_locked(adapter.device_identifier(), PCI::RegisterOffset::EXPANSION_ROM_POINTER, expansion_rom_ptr);
    });
    // OR with 1 to map the expansion rom pointer into memory
    PCI::write32_locked(adapter.device_identifier(), PCI::RegisterOffset::EXPANSION_ROM_POINTER, expansion_rom_ptr | 1);

    auto bios_mapping = TRY(Memory::map_typed<u8>(PhysicalAddress(expansion_rom_ptr), size, Memory::Region::Access::Read));
    auto bios_buffer = TRY(KBuffer::try_create_with_bytes("AMD GPU VBIOS"sv, ReadonlyBytes(bios_mapping.ptr(), size)));
    return try_create_from_kbuffer(move(bios_buffer));
}

bool Bios::is_valid() const
{
    auto error_or_rom = this->try_read_from_bios<ROM>(0);
    if (error_or_rom.is_error()) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "VBIOS size is too small");
        return false;
    }

    auto const rom = error_or_rom.release_value();
    if (rom->magic != 0xAA55) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "VBIOS signature incorrect 0x{:x}", rom->magic);
        return false;
    } else if (!rom->rom_table_offset) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "Cannot locate VBIOS ROM table header");
        return false;
    }

    auto const rom_table = this->try_read_from_bios<ROMTable>(rom->rom_table_offset);
    if (rom_table.is_error())
        return false;

    auto const atom_magic = ReadonlyBytes(rom_table.value()->magic);
    if (atom_magic != "ATOM"sv && atom_magic != "MOTA"sv) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "Invalid VBIOS magic");
        return false;
    }

    return true;
}

ErrorOr<void> Bios::index_iio()
{
    // Pre-fill IIO table so that we dont have to do a linear search every time
    // TODO: Maybe move this to some part in Interpreter...
    auto const* const rom = this->must_read_from_bios<ROM>(0);
    auto const rom_tab = this->must_read_from_bios<ROMTable>(rom->rom_table_offset);
    auto const dat_tab = this->must_read_from_bios<DataTableV11>(rom_tab->data_table_offset);
    auto const base = dat_tab->indirect_io_access + sizeof(TableHeader);
    auto const iio_ptr = this->must_read_from_bios<u8>(base);

    m_iio.fill(0);
    u32 i = 0;
    while (true) {
        auto const opcode = static_cast<IndirectIO>(iio_ptr[i]);
        if (opcode != IndirectIO::Start)
            break;

        m_iio[iio_ptr[i + 1]] = base + i + 2;
        i += 2;
        while (true) {
            switch (static_cast<IndirectIO>(iio_ptr[i])) {
            case IndirectIO::Nop:
                i += 1;
                continue;
            case IndirectIO::Start:
                i += 2;
                continue;
            case IndirectIO::Read:
            case IndirectIO::Write:
            case IndirectIO::Clear:
            case IndirectIO::Set:
                i += 3;
                continue;
            case IndirectIO::MoveIndex:
            case IndirectIO::MoveAttr:
            case IndirectIO::MoveData:
                i += 4;
                continue;
            case IndirectIO::End:
                i += 3;
                break;
            }
            break;
        }
    }
    return {};
}

ErrorOr<NonnullOwnPtr<KString>> Bios::name() const
{
    auto const rom = this->must_read_from_bios<ROM>(0);
    if (rom->number_of_strings == 0) {
        return KString::try_create("(unknown)"sv);
    }

    u8* name_ptr = this->m_bios->data() + rom->vbios_name_offset;

    // Skip atombios strings
    for (u16 i = 0; i < rom->number_of_strings; ++i) {
        while (*name_ptr != 0)
            ++name_ptr;
        ++name_ptr;
    }
    name_ptr += 2; // Skip \r\n

    size_t len = 0;
    while (name_ptr[len] && len < 64)
        ++len;

    // Trim up whitespace
    while (len > 0 && name_ptr[len - 1] <= ' ')
        --len;

    return KString::try_create(StringView(name_ptr, len));
}

u16 Bios::datatable(u16 index) const
{
    auto const* const rom = this->must_read_from_bios<ROM>(0);
    auto const rom_tab = this->must_read_from_bios<ROMTable>(rom->rom_table_offset);
    auto const dat_tab = this->must_read_from_bios<DataTable>(rom_tab->data_table_offset);
    return dat_tab->data[index];
}

ErrorOr<CommandDescriptor> Bios::command(Command cmd) const
{
    auto const* const rom = this->must_read_from_bios<ROM>(0);
    auto const rom_tab = this->must_read_from_bios<ROMTable>(rom->rom_table_offset);
    auto const cmd_tab = this->must_read_from_bios<CommandTable>(rom_tab->cmd_table_offset);
    auto const cmd_ptr = cmd_tab->commands[to_underlying(cmd)];

    if (cmd_ptr == 0) {
        // Unsupported command
        return Error::from_errno(ENXIO);
    }

    auto const* entry = this->must_read_from_bios<CommandTableEntry>(cmd_ptr);
    return CommandDescriptor { cmd_ptr, entry->size, entry->ws, entry->ps };
}

u8 Bios::read8(u16 offset) const
{
    return *this->must_read_from_bios<u8>(offset);
}

u16 Bios::read16(u16 offset) const
{
    return read8(offset) | (read8(offset + 1) << 8);
}

u32 Bios::read32(u16 offset) const
{
    return read16(offset) | (read16(offset + 2) << 16);
}

ErrorOr<void> Bios::invoke(AMDNativeGraphicsAdapter& gpu, Command cmd, Span<u32> parameters) const
{
    VERIFY(&gpu.bios() == this);
    return Graphics::AMD::Atom::Interpreter::execute(gpu, cmd, parameters);
}

ErrorOr<void> Bios::asic_init(AMDNativeGraphicsAdapter& gpu) const
{
    auto const rom = this->must_read_from_bios<ROM>(0);
    auto const rom_tab = this->must_read_from_bios<ROMTable>(rom->rom_table_offset);
    auto const dat_tab = this->must_read_from_bios<DataTableV11>(rom_tab->data_table_offset);
    auto const firmware_info = this->must_read_from_bios<FirmwareInfoV22>(dat_tab->firmware_info);

    if (firmware_info->header.format_revision != 2 || firmware_info->header.content_revision != 2) {
        return Error::from_errno(ENOTIMPL);
    }

    AsicInitV11Parameters parameters;
    parameters.sclk_freq = firmware_info->default_sclk_freq;
    parameters.mclk_freq = firmware_info->default_mclk_freq;

    dmesgln_pci(gpu, "initializing AMD GPU with sclk={}KHz, mclk={}KHz", parameters.sclk_freq * 10, parameters.mclk_freq * 10);

    return invoke(gpu, Command::AsicInit, to_parameter_space(parameters));
}

}
