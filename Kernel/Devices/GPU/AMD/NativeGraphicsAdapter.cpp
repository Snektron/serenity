/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/IDs.h>
#include <Kernel/Devices/GPU/AMD/NativeGraphicsAdapter.h>
#include <Kernel/Memory/PhysicalAddress.h>

namespace Kernel {

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
    TRY(adapter->initialize_adapter());
    return adapter;
}

AMDNativeGraphicsAdapter::AMDNativeGraphicsAdapter(PCI::DeviceIdentifier const& pci_device_identifier)
    : GenericGraphicsAdapter()
    , PCI::Device(const_cast<PCI::DeviceIdentifier&>(pci_device_identifier))
{
}

ErrorOr<void> AMDNativeGraphicsAdapter::initialize_adapter()
{
    dbgln_if(AMD_GRAPHICS_DEBUG, "AMD Native Graphics Adapter @ {}", device_identifier().address());
    return Error::from_errno(ENODEV);
}

}
