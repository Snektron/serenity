/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/ScopeGuard.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Devices/GPU/AMD/AtomBios.h>
#include <Kernel/Devices/GPU/AMD/NativeGraphicsAdapter.h>
#include <Kernel/Library/StdLib.h>

namespace Kernel::Graphics::AMD {

ErrorOr<AtomBios> AtomBios::try_create(AMDNativeGraphicsAdapter& adapter)
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

ErrorOr<AtomBios> AtomBios::try_create_from_kbuffer(NonnullOwnPtr<KBuffer>&& bios_buffer)
{
    auto bios = AtomBios(move(bios_buffer));
    if (!bios.is_valid()) {
        return Error::from_errno(EIO);
    }
    return bios;
}

ErrorOr<AtomBios> AtomBios::try_create_from_expansion_rom(AMDNativeGraphicsAdapter& adapter)
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
    auto bios_buffer = TRY(KBuffer::try_create_with_bytes("AMD GPU VBIOS"sv, AK::ReadonlyBytes(bios_mapping.ptr(), size)));
    return try_create_from_kbuffer(move(bios_buffer));
}

bool AtomBios::is_valid() const
{
    auto const bios = this->m_bios->bytes();

    if (bios.size() < sizeof(AtomBios::ROM)) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "VBIOS size is too small");
        return false;
    }

    auto const* rom = reinterpret_cast<AtomBios::ROM const*>(bios.data());
    if (rom->magic != 0xAA55) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "VBIOS signature incorrect 0x{:x}", rom->magic);
        return false;
    }

    if (!rom->rom_table_offset) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "Cannot locate VBIOS ROM table header");
        return false;
    }

    auto const* rom_table = reinterpret_cast<AtomBios::ROMTable const*>(bios.slice(rom->rom_table_offset, sizeof(AtomBios::ROMTable)).data());
    auto const atom_magic = AK::ReadonlyBytes(rom_table->magic);
    if (atom_magic != "ATOM"sv && atom_magic != "MOTA"sv) {
        dbgln_if(AMD_GRAPHICS_DEBUG, "Invalid VBIOS magic");
        return false;
    }

    return true;
}

}
