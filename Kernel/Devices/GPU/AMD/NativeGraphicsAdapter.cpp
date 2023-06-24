/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Library/Panic.h>

#include <Kernel/Boot/CommandLine.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/IDs.h>
#include <Kernel/Devices/GPU/AMD/Atom/Bios.h>
#include <Kernel/Devices/GPU/AMD/Atom/Interpreter.h>
#include <Kernel/Devices/GPU/AMD/NativeGraphicsAdapter.h>
#include <Kernel/Devices/GPU/AMD/VI/Device.h>
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

    m_device = TRY(adopt_nonnull_own_or_enomem(new (nothrow) VI::VIDevice(*this)));
    TRY(m_device->map_mmio());

    m_bios_debug = kernel_command_line().enable_atombios_debug();

    m_bios = TRY(Atom::Bios::try_create(*this));
    m_bios->dump_version(*this);
    TRY(m_bios->asic_init(*this));
    dmesgln_pci(*this, "GPU POSTed");

    return Error::from_errno(ENODEV);
}

void AMDNativeGraphicsAdapter::write_register(u32 reg, u32 data)
{
    m_device->write_register(reg, data);
}

u32 AMDNativeGraphicsAdapter::read_register(u32 reg)
{
    return m_device->read_register(reg);
}

}
