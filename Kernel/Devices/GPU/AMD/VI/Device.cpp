/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Devices/GPU/AMD/NativeGraphicsAdapter.h>
#include <Kernel/Devices/GPU/AMD/VI/Device.h>
#include <Kernel/Devices/GPU/AMD/VI/Registers.h>

namespace Kernel::Graphics::AMD::VI {

VIDevice::VIDevice(AMDNativeGraphicsAdapter& adapter)
    : AMDDevice(adapter)
{
}

void VIDevice::write_pcie_register(u16 reg, u32 data)
{
    // Outside the mapped range, write via PCIe
    // Note: PCIEIndex and PCIEData are supposed to be < m_mmio_registers.length
    auto const mmio = m_mmio_registers.ptr();
    SpinlockLocker locker(m_mmio_register_lock);
    mmio[to_underlying(Registers::PCIEIndex)] = reg * sizeof(u32);
    (void)mmio[to_underlying(Registers::PCIEIndex)];
    mmio[to_underlying(Registers::PCIEData)] = data;
    (void)mmio[to_underlying(Registers::PCIEData)];
}

u32 VIDevice::read_pcie_register(u16 reg)
{
    auto const mmio = m_mmio_registers.ptr();
    // Outside the mapped range, write via PCIe
    // Note: PCIEIndex and PCIEData are supposed to be < m_mmio_registers.length
    SpinlockLocker locker(m_mmio_register_lock);
    mmio[to_underlying(Registers::PCIEIndex)] = reg * sizeof(u32);
    (void)mmio[to_underlying(Registers::PCIEIndex)];
    return mmio[to_underlying(Registers::PCIEData)];
}

}
