/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Devices/GPU/AMD/Device.h>
#include <Kernel/Locking/Spinlock.h>

namespace Kernel::Graphics::AMD::VI {

class VIDevice final
    : public AMDDevice {

public:
    explicit VIDevice(AMDNativeGraphicsAdapter& adapter);

    void write_pcie_register(u16 reg, u32 data) override;
    u32 read_pcie_register(u16 reg) override;

protected:
    PCI::HeaderType0BaseRegister mmio_bar() const override { return PCI::HeaderType0BaseRegister::BAR5; }

private:
    Spinlock<LockRank::None> m_mmio_register_lock;
};

}
