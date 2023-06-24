/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Library/Panic.h>

#include <Kernel/Boot/CommandLine.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/IDs.h>
#include <Kernel/Devices/GPU/AMD/Arch/VI/Registers.h>
#include <Kernel/Devices/GPU/AMD/Atom/Bios.h>
#include <Kernel/Devices/GPU/AMD/Atom/Interpreter.h>
#include <Kernel/Devices/GPU/AMD/NativeGraphicsAdapter.h>
#include <Kernel/Memory/PhysicalAddress.h>

namespace Kernel::Graphics::AMD {

static constexpr u16 supported_models[] {
    0x67df, // RX 580X
};

static bool is_supported_model(u16 device_id)
{
    for (auto const& id : supported_models) {
        if (id == device_id)
            return true;
    }
    return false;
}

ErrorOr<bool> AMDNativeGraphicsAdapter::probe(PCI::DeviceIdentifier const& pci_device_identifier)
{
    PCI::HardwareID id = pci_device_identifier.hardware_id();
    return id.vendor_id == PCI::VendorID::AMD && is_supported_model(id.device_id);
}

ErrorOr<NonnullLockRefPtr<GenericGraphicsAdapter>> AMDNativeGraphicsAdapter::create(
    PCI::DeviceIdentifier const& pci_device_identifier)
{
    auto adapter = TRY(adopt_nonnull_lock_ref_or_enomem(new (nothrow) AMDNativeGraphicsAdapter(pci_device_identifier)));
    TRY(adapter->initialize());
    return adapter;
}

AMDNativeGraphicsAdapter::AMDNativeGraphicsAdapter(PCI::DeviceIdentifier const& pci_device_identifier)
    : GenericGraphicsAdapter()
    , PCI::Device(pci_device_identifier)
{
}

ErrorOr<void> AMDNativeGraphicsAdapter::initialize()
{
    dbgln_if(AMD_GRAPHICS_DEBUG, "AMD Native Graphics Adapter @ {}", device_identifier().address());

    PCI::enable_memory_space(device_identifier());
    PCI::enable_io_space(device_identifier());
    PCI::enable_bus_mastering(device_identifier());

    // Note: BAR5 is only from >= bonaire (GFX7)
    auto const mmio_addr = PhysicalAddress(PCI::get_BAR5(device_identifier()) & PCI::bar_address_mask);
    auto const mmio_size = PCI::get_BAR_space_size(device_identifier(), PCI::HeaderType0BaseRegister::BAR5);

    dmesgln_pci(*this, "MMIO @ {}, space size is 0x{:x} bytes", mmio_addr, mmio_size);

    m_mmio_registers = TRY(Memory::map_typed<u32 volatile>(mmio_addr, mmio_size, Memory::Region::Access::ReadWrite));

    m_bios_debug = kernel_command_line().enable_atombios_debug();

    m_bios = TRY(Atom::Bios::try_create(*this));
    m_bios->dump_version(*this);
    TRY(m_bios->asic_init(*this));

    return Error::from_errno(ENODEV);
}

void AMDNativeGraphicsAdapter::write_register(u32 reg, u32 data)
{
    auto const mmio = m_mmio_registers.ptr();
    if (reg * sizeof(u32) < m_mmio_registers.length) {
        mmio[reg] = data;
    } else {
        // Outside the mapped range, write via PCIe
        // TODO: Abstract this to architecture-specific write function
        // Note: PCIEIndex and PCIEData are supposed to be < m_mmio_registers.length
        SpinlockLocker locker(m_mmio_register_lock);
        mmio[to_underlying(VI::Registers::PCIEIndex)] = reg * sizeof(u32);
        (void)mmio[to_underlying(VI::Registers::PCIEIndex)];
        mmio[to_underlying(VI::Registers::PCIEData)] = data;
        (void)mmio[to_underlying(VI::Registers::PCIEData)];
    }
}

u32 AMDNativeGraphicsAdapter::read_register(u32 reg)
{
    auto const mmio = m_mmio_registers.ptr();
    u32 data;
    if (reg * sizeof(u32) < m_mmio_registers.length) {
        data = mmio[reg];
    } else {
        // Outside the mapped range, write via PCIe
        // TODO: Abstract this to architecture-specific read function
        // Note: PCIEIndex and PCIEData are supposed to be < m_mmio_registers.length
        SpinlockLocker locker(m_mmio_register_lock);
        mmio[to_underlying(VI::Registers::PCIEIndex)] = reg * sizeof(u32);
        (void)mmio[to_underlying(VI::Registers::PCIEIndex)];
        data = mmio[to_underlying(VI::Registers::PCIEData)];
    }
    return data;
}

}
