/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/Types.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Memory/TypedMapping.h>

namespace Kernel::Graphics::AMD {

class AMDNativeGraphicsAdapter;

class AMDDevice {
    AK_MAKE_NONMOVABLE(AMDDevice);

public:
    explicit AMDDevice(AMDNativeGraphicsAdapter& adapter);
    virtual ~AMDDevice() = default;

    ErrorOr<void> map_mmio();

    virtual void write_pcie_register(u16 reg, u32 data) = 0;
    virtual u32 read_pcie_register(u16 reg) = 0;

    void write_register(u16 reg, u32 data);
    u32 read_register(u16 reg);

protected:
    virtual PCI::HeaderType0BaseRegister mmio_bar() const = 0;

    AMDNativeGraphicsAdapter& m_adapter;
    Memory::TypedMapping<u32 volatile> m_mmio_registers;
};

}
