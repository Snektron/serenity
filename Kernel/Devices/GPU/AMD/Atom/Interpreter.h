/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/Span.h>
#include <AK/StringBuilder.h>
#include <AK/Types.h>
#include <Kernel/Boot/CommandLine.h>
#include <Kernel/Devices/GPU/AMD/Atom/Definitions.h>

namespace Kernel {

class AMDNativeGraphicsAdapter;

}

namespace Kernel::Graphics::AMD::Atom {

class Bios;

struct InstructionDescriptor {
    OpCode opcode;
    union {
        Location dst_loc;
        Condition cond;
        Port port;
        Unit unit;
    } operand;

    constexpr explicit InstructionDescriptor(OpCode opcode)
        : opcode(opcode)
    {
    }

    constexpr explicit InstructionDescriptor(OpCode opcode, Location dst_loc)
        : opcode(opcode)
    {
        operand.dst_loc = dst_loc;
    }

    constexpr explicit InstructionDescriptor(OpCode opcode, Condition cond)
        : opcode(opcode)
    {
        operand.cond = cond;
    }

    constexpr explicit InstructionDescriptor(OpCode opcode, Port port)
        : opcode(opcode)
    {
        operand.port = port;
    }

    constexpr explicit InstructionDescriptor(OpCode opcode, Unit unit)
        : opcode(opcode)
    {
        operand.unit = unit;
    }
};

struct Operand {
    // Original value of the location (32 bits)
    u32 full_value;
    Location loc;
    AddressMode address_mode;
    u16 pc;

    u32 value() const;
};

class Interpreter {
public:
    static ErrorOr<void> execute(AMDNativeGraphicsAdapter& gpu, Command cmd, Span<u32> parameters);

private:
    // Global context that is shared between calls to different tables
    struct Context {
        u32 divmul[2] { 0, 0 };
        u32 fb_base { 0 };
        u16 data_block { 0 };
        u16 reg_block { 0 };
        IOMode io_mode { IOMode::MemoryMapped };
        u8 iio_program { 0 }; // Only valid if io_mode is IIO
        u8 shift { 0 };
        bool comp_equal { false };
        bool comp_above { false };
        u16 io_attr { 0 };
    };

    static ErrorOr<void> execute_recursive(Context& ctx, AMDNativeGraphicsAdapter& gpu, Command cmd, Span<u32> parameters, u16 debug_depth);

    explicit Interpreter(Context& ctx, AMDNativeGraphicsAdapter& gpu, CommandDescriptor cmd_desc, Span<u32> ps, Span<u32> ws, u16 debug_depth);

    ErrorOr<u32> execute_iio(u32 program, u32 index, u32 data);

    ErrorOr<bool> next();

    u8 read8();
    u16 read16();
    u32 read32();

    Operand read_dst(Location loc, u8 attr);
    Operand read_dst_skip(Location loc, u8 attr);
    Operand read_src(u8 attr);
    u32 read_immediate(AddressMode mode);
    void write_dst(Operand const& op, u32 value);

    template<typename... Parameters>
    void traceff(CheckedFormatString<Parameters...>&& fmt, Parameters const&... parameters)
    {
        if (trace_enabled()) {
            m_trace.appendff(forward<CheckedFormatString<Parameters...>>(fmt), parameters...);
        }
    }

    void trace(StringView msg);
    void flush_trace();
    bool trace_enabled() const;

    AMDNativeGraphicsAdapter& m_gpu;
    Context& m_ctx;
    CommandDescriptor m_cmd_desc;

    Span<u32> m_parameter_space;
    Span<u32> m_workspace;
    u16 m_pc { sizeof(CommandTableEntry) };
    u16 reg_block { 0 };
    u16 m_debug_depth;
    StringBuilder m_trace;
};

}
