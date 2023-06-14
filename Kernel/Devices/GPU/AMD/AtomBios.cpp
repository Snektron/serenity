/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Find.h>
#include <AK/ScopeGuard.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Devices/GPU/AMD/AtomBios.h>
#include <Kernel/Devices/GPU/AMD/NativeGraphicsAdapter.h>
#include <Kernel/Library/StdLib.h>

namespace Kernel::Graphics::AMD {

ErrorOr<NonnullOwnPtr<AtomBios>> AtomBios::try_create(AMDNativeGraphicsAdapter& adapter)
{
    if (auto bios = try_create_from_expansion_rom(adapter); bios.is_error()) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "Failed to read VBIOS from PCI expansion ROM: {}", bios.error());
    } else {
        dmesgln_pci(adapter, "Loaded VBIOS from PCI expansion ROM");
        return bios.release_value();
    }

    return Error::from_errno(ENXIO);
}

AtomBios::AtomBios(NonnullOwnPtr<KBuffer>&& bios)
    : m_bios(move(bios))
{
}

ErrorOr<NonnullOwnPtr<AtomBios>> AtomBios::try_create_from_kbuffer(NonnullOwnPtr<KBuffer>&& bios_buffer)
{
    auto bios = TRY(adopt_nonnull_own_or_enomem(new (nothrow) AtomBios(move(bios_buffer))));
    if (!bios->is_valid()) {
        return Error::from_errno(EIO);
    }
    return bios;
}

ErrorOr<NonnullOwnPtr<AtomBios>> AtomBios::try_create_from_expansion_rom(AMDNativeGraphicsAdapter& adapter)
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

bool AtomBios::is_valid() const
{
    auto error_or_rom = this->read_from_bios<AtomBios::ROM>(0);
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

    auto const rom_table = this->read_from_bios<AtomBios::ROMTable>(rom->rom_table_offset);
    if (rom_table.is_error())
        return false;

    auto const atom_magic = ReadonlyBytes(rom_table.value()->magic);
    if (atom_magic != "ATOM"sv && atom_magic != "MOTA"sv) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "Invalid VBIOS magic");
        return false;
    }

    return true;
}

ErrorOr<NonnullOwnPtr<KString>> AtomBios::name() const
{
    auto const rom = MUST(this->read_from_bios<AtomBios::ROM>(0));
    if (rom->number_of_strings == 0) {
        return KString::try_create("(unknown)"sv);
    }

    u8* name_ptr = this->m_bios->data() + rom->vbios_name_offset;

    // Skip atombios strings
    // TODO: Improve?
    for (u16 i = 0; i < rom->number_of_strings; ++i) {
        while (*name_ptr != 0)
            ++name_ptr;
        ++name_ptr;
    }
    name_ptr += 2; // Skip \r\n

    size_t len = 0;
    while (name_ptr[len])
        ++len;

    // Trim up whitespace
    while (len > 0 && name_ptr[len - 1] <= ' ')
        --len;

    return KString::try_create(StringView(name_ptr, len));
}

}
