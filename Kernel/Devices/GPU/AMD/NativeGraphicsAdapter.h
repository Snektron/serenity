/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Devices/GPU/GenericGraphicsAdapter.h>

namespace Kernel {

class AMDNativeGraphicsAdapter final
    : public GenericGraphicsAdapter
    , public PCI::Device {

public:
    static ErrorOr<bool> probe(PCI::DeviceIdentifier const&);
    static ErrorOr<NonnullLockRefPtr<GenericGraphicsAdapter>> create(PCI::DeviceIdentifier const&);

    virtual ~AMDNativeGraphicsAdapter() = default;
    virtual StringView device_name() const override { return "AMDNativeGraphicsAdapter"sv; }
private:
    explicit AMDNativeGraphicsAdapter(PCI::DeviceIdentifier const&);

    ErrorOr<void> initialize_adapter();
};
}
