/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Devices/GPU/AMD/Device.h>
#include <Kernel/Devices/GPU/AMD/NativeGraphicsAdapter.h>

namespace Kernel::Graphics::AMD {

AMDDevice::AMDDevice(AMDNativeGraphicsAdapter& adapter)
    : m_adapter(adapter)
{
}

ErrorOr<void> AMDDevice::map_mmio()
{
    auto const bar = mmio_bar();
    auto const addr = PhysicalAddress(PCI::get_BAR(m_adapter.device_identifier(), bar) & PCI::bar_address_mask);
    auto const size = PCI::get_BAR_space_size(m_adapter.device_identifier(), bar);
    dmesgln_pci(m_adapter, "MMIO @ {}, space size is 0x{:x} bytes", addr, size);
    m_mmio_registers = TRY(Memory::map_typed<u32 volatile>(addr, size, Memory::Region::Access::ReadWrite));
    return {};
}

void AMDDevice::write_register(u16 reg, u32 data)
{
    if (reg * sizeof(u32) < m_mmio_registers.length) {
        m_mmio_registers.ptr()[reg] = data;
    } else {
        write_pcie_register(reg, data);
    }
}

u32 AMDDevice::read_register(u16 reg)
{
    if (reg * sizeof(u32) < m_mmio_registers.length) {
        return m_mmio_registers.ptr()[reg];
    }
    return read_pcie_register(reg);
}

}
