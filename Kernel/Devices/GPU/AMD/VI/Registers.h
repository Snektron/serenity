/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Types.h>

namespace Kernel::Graphics::AMD::VI {

// These are mostly from amdgpu bif_5_0_d.h
enum class Registers : u16 {
    PCIEIndex = 0xe,
    PCIEData = 0xf,
};

}
