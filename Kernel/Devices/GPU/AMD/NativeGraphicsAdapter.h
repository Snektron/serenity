/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/OwnPtr.h>
#include <AK/Types.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Devices/GPU/GenericGraphicsAdapter.h>
#include <Kernel/Locking/Spinlock.h>
#include <Kernel/Memory/TypedMapping.h>

namespace Kernel {

namespace Graphics::AMD::Atom {

class Bios;

};

class AMDNativeGraphicsAdapter final
    : public GenericGraphicsAdapter
    , public PCI::Device {

public:
    static ErrorOr<bool> probe(PCI::DeviceIdentifier const&);
    static ErrorOr<NonnullLockRefPtr<GenericGraphicsAdapter>> create(PCI::DeviceIdentifier const&);

    virtual ~AMDNativeGraphicsAdapter() = default;
    virtual StringView device_name() const override { return "AMDNativeGraphicsAdapter"sv; }

    Graphics::AMD::Atom::Bios& bios() const { return *m_bios; }

    // Write a to an AMD GPU register.
    // - reg: Register dword index.
    // - data: Value to write.
    void write_register(u32 reg, u32 data);
    // Read from an AMD GPU register.
    // - reg: Register dword index.
    u32 read_register(u32 reg);

private:
    explicit AMDNativeGraphicsAdapter(PCI::DeviceIdentifier const&);

    ErrorOr<void> initialize();

    Spinlock<LockRank::None> m_mmio_register_lock;
    Memory::TypedMapping<u32 volatile> m_mmio_registers;

    OwnPtr<Graphics::AMD::Atom::Bios> m_bios;
};
}
