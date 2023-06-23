/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Array.h>
#include <AK/Error.h>
#include <AK/OwnPtr.h>
#include <AK/Span.h>
#include <AK/Types.h>
#include <Kernel/Devices/GPU/AMD/Atom/Definitions.h>
#include <Kernel/Devices/GPU/AMD/Atom/Interpreter.h>
#include <Kernel/Library/KBuffer.h>
#include <Kernel/Library/KString.h>

namespace Kernel {

class AMDNativeGraphicsAdapter;

};

namespace Kernel::Graphics::AMD::Atom {

// Atom definitions from atom.h and atomfirmware.h
class Bios final {
public:
    static ErrorOr<NonnullOwnPtr<Bios>> try_create(AMDNativeGraphicsAdapter& adapter);

    void dump_version(AMDNativeGraphicsAdapter& adapter) const;

    u16 datatable(u16 index) const;

    ErrorOr<CommandDescriptor> command(Command cmd) const;

    u8 read8(u16 offset) const;
    u16 read16(u16 offset) const;
    u32 read32(u16 offset) const;

    u16 iio_program(u16 index) const { return m_iio[index]; }

    ErrorOr<void> invoke(AMDNativeGraphicsAdapter& gpu, Command cmd, Span<u32> parameters) const;

    ErrorOr<void> asic_init(AMDNativeGraphicsAdapter& gpu) const;

private:
    explicit Bios(NonnullOwnPtr<KBuffer>&& bios);

    static ErrorOr<NonnullOwnPtr<Bios>> try_create_from_kbuffer(NonnullOwnPtr<KBuffer>&& bios_buffer);
    static ErrorOr<NonnullOwnPtr<Bios>> try_create_from_expansion_rom(AMDNativeGraphicsAdapter& adapter);

    template<typename T>
    ErrorOr<T const*> try_read_from_bios(u16 offset) const
    {
        auto const bios = this->m_bios->bytes();
        if (offset + sizeof(T) > bios.size())
            return Error::from_errno(EIO);
        return must_read_from_bios<T>(offset);
    }

    template<typename T>
    T const* must_read_from_bios(u16 offset) const
    {
        static_assert(alignof(T) == 1);
        auto const bios = this->m_bios->bytes();
        return reinterpret_cast<T const*>(bios.slice(offset, sizeof(T)).data());
    }

    template<typename T>
    static Span<u32> to_parameter_space(T& t)
    {
        u32* ptr = reinterpret_cast<u32*>(&t);
        return Span<u32>(ptr, sizeof(T) / sizeof(u32));
    }

    bool is_valid() const;
    ErrorOr<void> index_iio();

    Spinlock<LockRank::None> m_execution_lock;
    NonnullOwnPtr<KBuffer> m_bios;
    Array<u16, MaxIIOPrograms> m_iio;
};

}
