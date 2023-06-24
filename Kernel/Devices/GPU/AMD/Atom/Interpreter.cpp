/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <AK/FixedArray.h>
#include <AK/ScopeGuard.h>
#include <AK/StdLibExtras.h>
#include <Kernel/Devices/GPU/AMD/Atom/Bios.h>
#include <Kernel/Devices/GPU/AMD/Atom/Interpreter.h>
#include <Kernel/Devices/GPU/AMD/Device.h>
#include <Kernel/Devices/GPU/AMD/NativeGraphicsAdapter.h>
#include <Kernel/Tasks/Process.h>

namespace Kernel::Graphics::AMD::Atom {

static constexpr auto instruction_table = Array {
    InstructionDescriptor(OpCode::Invalid),
    InstructionDescriptor(OpCode::Move, Location::Register),
    InstructionDescriptor(OpCode::Move, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Move, Location::WorkSpace),
    InstructionDescriptor(OpCode::Move, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Move, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Move, Location::MemController),
    InstructionDescriptor(OpCode::And, Location::Register),
    InstructionDescriptor(OpCode::And, Location::ParameterSpace),
    InstructionDescriptor(OpCode::And, Location::WorkSpace),
    InstructionDescriptor(OpCode::And, Location::FrameBuffer),
    InstructionDescriptor(OpCode::And, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::And, Location::MemController),
    InstructionDescriptor(OpCode::Or, Location::Register),
    InstructionDescriptor(OpCode::Or, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Or, Location::WorkSpace),
    InstructionDescriptor(OpCode::Or, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Or, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Or, Location::MemController),
    InstructionDescriptor(OpCode::ShiftLeft, Location::Register),
    InstructionDescriptor(OpCode::ShiftLeft, Location::ParameterSpace),
    InstructionDescriptor(OpCode::ShiftLeft, Location::WorkSpace),
    InstructionDescriptor(OpCode::ShiftLeft, Location::FrameBuffer),
    InstructionDescriptor(OpCode::ShiftLeft, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::ShiftLeft, Location::MemController),
    InstructionDescriptor(OpCode::ShiftRight, Location::Register),
    InstructionDescriptor(OpCode::ShiftRight, Location::ParameterSpace),
    InstructionDescriptor(OpCode::ShiftRight, Location::WorkSpace),
    InstructionDescriptor(OpCode::ShiftRight, Location::FrameBuffer),
    InstructionDescriptor(OpCode::ShiftRight, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::ShiftRight, Location::MemController),
    InstructionDescriptor(OpCode::Mul, Location::Register),
    InstructionDescriptor(OpCode::Mul, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Mul, Location::WorkSpace),
    InstructionDescriptor(OpCode::Mul, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Mul, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Mul, Location::MemController),
    InstructionDescriptor(OpCode::Div, Location::Register),
    InstructionDescriptor(OpCode::Div, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Div, Location::WorkSpace),
    InstructionDescriptor(OpCode::Div, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Div, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Div, Location::MemController),
    InstructionDescriptor(OpCode::Add, Location::Register),
    InstructionDescriptor(OpCode::Add, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Add, Location::WorkSpace),
    InstructionDescriptor(OpCode::Add, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Add, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Add, Location::MemController),
    InstructionDescriptor(OpCode::Sub, Location::Register),
    InstructionDescriptor(OpCode::Sub, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Sub, Location::WorkSpace),
    InstructionDescriptor(OpCode::Sub, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Sub, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Sub, Location::MemController),
    InstructionDescriptor(OpCode::SetPort, Port::ATI),
    InstructionDescriptor(OpCode::SetPort, Port::PCI),
    InstructionDescriptor(OpCode::SetPort, Port::SysIO),
    InstructionDescriptor(OpCode::SetRegBlock),
    InstructionDescriptor(OpCode::SetFBBase),
    InstructionDescriptor(OpCode::Compare, Location::Register),
    InstructionDescriptor(OpCode::Compare, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Compare, Location::WorkSpace),
    InstructionDescriptor(OpCode::Compare, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Compare, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Compare, Location::MemController),
    InstructionDescriptor(OpCode::Switch),
    InstructionDescriptor(OpCode::Jump, Condition::Always),
    InstructionDescriptor(OpCode::Jump, Condition::Equal),
    InstructionDescriptor(OpCode::Jump, Condition::Below),
    InstructionDescriptor(OpCode::Jump, Condition::Above),
    InstructionDescriptor(OpCode::Jump, Condition::BelowOrEqual),
    InstructionDescriptor(OpCode::Jump, Condition::AboveOrEqual),
    InstructionDescriptor(OpCode::Jump, Condition::NotEqual),
    InstructionDescriptor(OpCode::Test, Location::Register),
    InstructionDescriptor(OpCode::Test, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Test, Location::WorkSpace),
    InstructionDescriptor(OpCode::Test, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Test, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Test, Location::MemController),
    InstructionDescriptor(OpCode::Delay, Unit::MilliSecond),
    InstructionDescriptor(OpCode::Delay, Unit::MicroSecond),
    InstructionDescriptor(OpCode::CallTable),
    InstructionDescriptor(OpCode::Repeat),
    InstructionDescriptor(OpCode::Clear, Location::Register),
    InstructionDescriptor(OpCode::Clear, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Clear, Location::WorkSpace),
    InstructionDescriptor(OpCode::Clear, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Clear, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Clear, Location::MemController),
    InstructionDescriptor(OpCode::Nop),
    InstructionDescriptor(OpCode::Eot),
    InstructionDescriptor(OpCode::Mask, Location::Register),
    InstructionDescriptor(OpCode::Mask, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Mask, Location::WorkSpace),
    InstructionDescriptor(OpCode::Mask, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Mask, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Mask, Location::MemController),
    InstructionDescriptor(OpCode::PostCard),
    InstructionDescriptor(OpCode::Beep),
    InstructionDescriptor(OpCode::SaveReg),
    InstructionDescriptor(OpCode::RestoreReg),
    InstructionDescriptor(OpCode::SetDataBlock),
    InstructionDescriptor(OpCode::Xor, Location::Register),
    InstructionDescriptor(OpCode::Xor, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Xor, Location::WorkSpace),
    InstructionDescriptor(OpCode::Xor, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Xor, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Xor, Location::MemController),
    InstructionDescriptor(OpCode::Shl, Location::Register),
    InstructionDescriptor(OpCode::Shl, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Shl, Location::WorkSpace),
    InstructionDescriptor(OpCode::Shl, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Shl, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Shl, Location::MemController),
    InstructionDescriptor(OpCode::Shr, Location::Register),
    InstructionDescriptor(OpCode::Shr, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Shr, Location::WorkSpace),
    InstructionDescriptor(OpCode::Shr, Location::FrameBuffer),
    InstructionDescriptor(OpCode::Shr, Location::PhaseLockedLoop),
    InstructionDescriptor(OpCode::Shr, Location::MemController),
    InstructionDescriptor(OpCode::Debug),
    InstructionDescriptor(OpCode::ProcessDS),
    InstructionDescriptor(OpCode::Mul32, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Mul32, Location::WorkSpace),
    InstructionDescriptor(OpCode::Div32, Location::ParameterSpace),
    InstructionDescriptor(OpCode::Div32, Location::WorkSpace),
};
static_assert(instruction_table.size() == 127);

static constexpr auto opcode_name_table = Array {
    "invalid",
    "move",
    "and",
    "or",
    "shiftleft",
    "shiftright",
    "mul",
    "div",
    "add",
    "sub",
    "setport",
    "setregblock",
    "setfbbase",
    "compare",
    "switch",
    "jump",
    "test",
    "delay",
    "calltable",
    "repeat",
    "clear",
    "nop",
    "eot",
    "mask",
    "postcard",
    "beep",
    "savereg",
    "restorereg",
    "setdatablock",
    "xor",
    "shl",
    "shr",
    "debug",
    "processds",
    "mul32",
    "div32",
};

static constexpr auto cond_name_table = Array {
    "always",
    "equal",
    "below",
    "above",
    "beloworequal",
    "aboveorequal",
    "notequal",
};

static constexpr auto io_name_table = Array {
    "mm",
    "pll",
    "mc",
    "pcie",
    "pcie port",
};

static AddressMode src_to_dst_align[8][4] = {
    { AddressMode::DWord, AddressMode::DWord, AddressMode::DWord, AddressMode::DWord },
    { AddressMode::Word0, AddressMode::Word8, AddressMode::Word16, AddressMode::DWord },
    { AddressMode::Word0, AddressMode::Word8, AddressMode::Word16, AddressMode::DWord },
    { AddressMode::Word0, AddressMode::Word8, AddressMode::Word16, AddressMode::DWord },
    { AddressMode::Byte0, AddressMode::Byte8, AddressMode::Byte16, AddressMode::Byte24 },
    { AddressMode::Byte0, AddressMode::Byte8, AddressMode::Byte16, AddressMode::Byte24 },
    { AddressMode::Byte0, AddressMode::Byte8, AddressMode::Byte16, AddressMode::Byte24 },
    { AddressMode::Byte0, AddressMode::Byte8, AddressMode::Byte16, AddressMode::Byte24 },
};

static int def_dst[8] = { 0, 0, 1, 2, 0, 1, 2, 3 };

u32 Operand::value() const
{
    u32 value = this->full_value;
    switch (this->address_mode) {
    case AddressMode::DWord:
        return value;
    case AddressMode::Word0:
        return value & 0x0000FFFF;
    case AddressMode::Word8:
        return (value & 0x00FFFF00) >> 8;
    case AddressMode::Word16:
        return (value & 0xFFFF0000) >> 16;
    case AddressMode::Byte0:
        return value & 0x000000FF;
    case AddressMode::Byte8:
        return (value & 0x0000FF00) >> 8;
    case AddressMode::Byte16:
        return (value & 0x00FF0000) >> 16;
    case AddressMode::Byte24:
        return (value & 0xFF000000) >> 24;
    }
}

Interpreter::Interpreter(Context& ctx, AMDNativeGraphicsAdapter& adapter, CommandDescriptor cmd_desc, Span<u32> ps, Span<u32> ws, u16 debug_depth)
    : m_adapter(adapter)
    , m_ctx(ctx)
    , m_cmd_desc(cmd_desc)
    , m_parameter_space(ps)
    , m_workspace(ws)
    , m_debug_depth(debug_depth)
{
}

ErrorOr<void> Interpreter::execute(AMDNativeGraphicsAdapter& adapter, Command cmd, Span<u32> parameters)
{
    Context ctx;
    return execute_recursive(ctx, adapter, cmd, parameters, 0);
}

ErrorOr<void> Interpreter::execute_recursive(Context& ctx, AMDNativeGraphicsAdapter& adapter, Command cmd, Span<u32> parameters, u16 debug_depth)
{
    auto const desc = TRY(adapter.bios().command(cmd));
    auto work_space = TRY(FixedArray<u32>::create(desc.work_space_size / sizeof(u32)));
    auto interp = Interpreter(ctx, adapter, desc, parameters, work_space.span(), debug_depth);

    interp.traceff("--- Executing command {:04x} @ {:04x} (len={:04x}, ps={:02x}, ws={:02x})", to_underlying(cmd), desc.base, desc.size, desc.parameter_space_size, desc.work_space_size);
    interp.flush_trace();

    VERIFY(parameters.size() * sizeof(u32) >= desc.parameter_space_size);

    while (true) {
        bool const cont = TRY(interp.next());
        interp.flush_trace();
        if (!cont)
            break;
    }

    return {};
}

ErrorOr<u32> Interpreter::execute_iio(u32 program, u32 index, u32 data)
{
    u16 iio_pc = m_adapter.bios().iio_program(program);
    if (iio_pc == 0) {
        dmesgln_pci(m_adapter, "Atom: invalid IIO program {}", program);
        return Error::from_errno(EIO);
    }

    auto const iio8 = [&, this]() -> u8 {
        return m_adapter.bios().read8(iio_pc++);
    };

    auto const iio16 = [&]() -> u16 {
        return iio8() | (iio8() << 8);
    };

    u32 temp = 0xCDCDCDCD;
    while (true) {
        u8 const op = iio8();
        switch (static_cast<IndirectIO>(op)) {
        case IndirectIO::Nop:
            break;
        case IndirectIO::Read: {
            u16 index = iio16();
            temp = m_adapter.device().read_register(index);
            break;
        }
        case IndirectIO::Write: {
            u16 index = iio16();
            m_adapter.device().write_register(index, temp);
            break;
        }
        case IndirectIO::Clear:
            temp &= ~((0xFFFFFFFF >> (32 - iio8())) << iio8());
            break;
        case IndirectIO::Set:
            temp |= ((0xFFFFFFFF >> (32 - iio8())) << iio8());
            break;
        case IndirectIO::MoveIndex: {
            u8 a = iio8();
            u8 b = iio8();
            u8 c = iio8();
            temp &= ~((0xFFFFFFFF >> (32 - a)) << c);
            temp |= ((index >> b) & (0xFFFFFFFF >> (32 - a))) << c;
            break;
        }
        case IndirectIO::MoveData: {
            u8 a = iio8();
            u8 b = iio8();
            u8 c = iio8();
            temp &= ~((0xFFFFFFFF >> (32 - a)) << c);
            temp |= ((data >> b) & (0xFFFFFFFF >> (32 - a))) << c;
            break;
        }
        case IndirectIO::MoveAttr: {
            u8 a = iio8();
            u8 b = iio8();
            u8 c = iio8();
            temp &= ~((0xFFFFFFFF >> (32 - a)) << c);
            temp |= ((m_ctx.io_attr >> b) & (0xFFFFFFFF >> (32 - a))) << c;
            break;
        }
        case IndirectIO::End:
            return temp;
        case IndirectIO::Start:
        default:
            dmesgln_pci(m_adapter, "Atom: invalid IIO opcode {:02x}", op);
            return Error::from_errno(EIO);
        }
    }
}

ErrorOr<bool> Interpreter::next()
{
    auto const start_pc = m_pc;
    auto const inst = read8();
    auto const desc = instruction_table[inst < instruction_table.size() ? inst : 0];

    traceff("{:04x}+{:04x}: {: <12}", m_cmd_desc.base, start_pc, opcode_name_table[to_underlying(desc.opcode)]);

    switch (desc.opcode) {
    case OpCode::Invalid:
        dmesgln_pci(m_adapter, "Atom: Invalid instruction {:02x} at {:04x}+{:04x}", inst, m_cmd_desc.base, m_pc);
        return Error::from_errno(EIO);
    case OpCode::Move: {
        u8 attr = read8();
        // Special case: If we are moving dwords, do not read the destination register. This is
        // what Linux does. It seems that reading some registers causes the value of the next read to
        // change.
        bool const is_dword_move = static_cast<AddressMode>((attr >> 3) & 0x7) == AddressMode::DWord;
        auto const dst = is_dword_move ? read_dst_skip(desc.operand.dst_loc, attr) : read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        write_dst(dst, src.value());
        break;
    }
    case OpCode::And: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        write_dst(dst, dst.value() & src.value());
        break;
    }
    case OpCode::Or: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        write_dst(dst, dst.value() | src.value());
        break;
    }
    case OpCode::ShiftLeft: {
        u8 attr = read8();
        attr &= 0x38;
        attr |= def_dst[attr >> 3] << 6;
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const shift = read_immediate(AddressMode::Byte0);
        traceff(" shift:{:02x}", shift);
        write_dst(dst, dst.value() << shift);
        break;
    }
    case OpCode::ShiftRight: {
        u8 attr = read8();
        attr &= 0x38;
        attr |= def_dst[attr >> 3] << 6;
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const shift = read_immediate(AddressMode::Byte0);
        traceff(" shift:{:02x}", shift);
        write_dst(dst, dst.value() >> shift);
        break;
    }
    case OpCode::Mul: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        m_ctx.divmul[0] = dst.value() * src.value();
        traceff(" => {:08x}", m_ctx.divmul[0]);
        break;
    }
    case OpCode::Div: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        if (src.value() != 0) {
            m_ctx.divmul[0] = dst.value() / src.value();
            m_ctx.divmul[1] = dst.value() % src.value();
        } else {
            m_ctx.divmul[0] = 0;
            m_ctx.divmul[1] = 0;
        }
        traceff(" => {:08x} {:08x}", m_ctx.divmul[0], m_ctx.divmul[1]);
        break;
    }
    case OpCode::Add: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        write_dst(dst, dst.value() + src.value());
        break;
    }
    case OpCode::Sub: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        write_dst(dst, dst.value() - src.value());
        break;
    }
    case OpCode::SetPort: {
        switch (desc.operand.port) {
        case Port::ATI: {
            u16 port = read16();
            if (port == 0) {
                m_ctx.io_mode = IOMode::MemoryMapped;
                trace(" mm"sv);
            } else {
                m_ctx.io_mode = IOMode::IIO;
                m_ctx.iio_program = port;
                if (port < io_name_table.size()) {
                    traceff(" iio:{}", io_name_table[port]);
                } else {
                    traceff(" iio:{:02x}", port);
                }
            }
            break;
        }
        case Port::PCI:
            (void)read8();
            m_ctx.io_mode = IOMode::PCI;
            trace(" pci"sv);
            break;
        case Port::SysIO:
            (void)read8();
            m_ctx.io_mode = IOMode::SysIO;
            trace(" sysio"sv);
            break;
        }
        break;
    }
    case OpCode::SetRegBlock: {
        m_ctx.reg_block = read16();
        traceff(" block:{:04x}", m_ctx.reg_block);
        break;
    }
    case OpCode::SetFBBase: {
        u8 attr = read8();
        m_ctx.fb_base = read_src(attr).value();
        break;
    }
    case OpCode::Compare: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        m_ctx.comp_above = dst.value() > src.value();
        m_ctx.comp_equal = dst.value() == src.value();
        traceff(" => {} {}", m_ctx.comp_above ? "above" : "below", m_ctx.comp_equal ? "equal" : "notequal");
        break;
    }
    case OpCode::Switch: {
        u8 attr = read8();
        auto const src = read_src(attr);
        flush_trace();
        bool stop = false;
        while (!stop) {
            const u8 case_type = read8();
            switch (case_type) {
            case CaseMagic: {
                auto const cond = read_immediate(src.address_mode);
                u16 const target = read16();
                traceff("  case:{:08x} target:{:04x}", cond, target);
                if (cond == src.value()) {
                    traceff(" => taken");
                    flush_trace();
                    m_pc = target;
                    stop = true;
                }
                flush_trace();
                break;
            }
            case CaseEnd: {
                // 2 case ends marks end of switch
                if (read8() != CaseEnd) {
                    dmesgln_pci(m_adapter, "Atom: Invalid case end");
                    return Error::from_errno(EIO);
                }
                stop = true;
                break;
            }
            default:
                dmesgln_pci(m_adapter, "Atom: Invalid case");
                return Error::from_errno(EIO);
            }
        }
        break;
    }
    case OpCode::Jump: {
        u16 target = read16();
        bool take = false;
        traceff(" {} {:04x}", cond_name_table[to_underlying(desc.operand.cond)], target);
        switch (desc.operand.cond) {
        case Condition::Above:
            take = m_ctx.comp_above;
            break;
        case Condition::AboveOrEqual:
            take = m_ctx.comp_above || m_ctx.comp_equal;
            break;
        case Condition::Always:
            take = true;
            break;
        case Condition::Below:
            take = !(m_ctx.comp_above || m_ctx.comp_equal);
            break;
        case Condition::BelowOrEqual:
            take = !m_ctx.comp_above;
            break;
        case Condition::Equal:
            take = m_ctx.comp_equal;
            break;
        case Condition::NotEqual:
            take = !m_ctx.comp_equal;
            break;
        }

        traceff(" => {}", take ? "taken" : "not taken");
        // TODO: Deadlock detection?
        // NOTE: The jump target is here INCLUDING the command descriptor, but
        // m_pc does not include that.
        if (take)
            m_pc = target;
        break;
    }
    case OpCode::Test: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        m_ctx.comp_equal = (dst.value() & src.value()) == 0;
        traceff(" => {}", m_ctx.comp_equal ? "equal" : "notequal");
        break;
    }
    case OpCode::Delay: {
        u8 count = read8();
        Thread::BlockResult block_result(Thread::BlockResult::WokeNormally);
        switch (desc.operand.unit) {
        case Unit::MicroSecond:
            traceff(" {}us", count);
            block_result = Thread::current()->sleep(Duration::from_microseconds(count));
            break;
        case Unit::MilliSecond:
            traceff(" {}ms", count);
            block_result = Thread::current()->sleep(Duration::from_milliseconds(count));
            break;
        }

        if (block_result.was_interrupted()) {
            dmesgln_pci(m_adapter, "Atom warning: interrupted during sleep");
        }

        break;
    }
    case OpCode::CallTable: {
        auto const index = read8();
        auto const cmd = static_cast<Command>(index); // TODO: Is this cast valid? Because its an enum class?
        auto const ps = m_parameter_space.slice(m_cmd_desc.parameter_space_size / sizeof(u32));
        traceff(" {:02x}", index);
        flush_trace();
        TRY(execute_recursive(m_ctx, m_adapter, cmd, ps, m_debug_depth + 1));
        break;
    }
    case OpCode::Clear: {
        u8 attr = read8();
        attr &= 0x38;
        attr |= def_dst[attr >> 3] << 6;
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        write_dst(dst, 0);
        break;
    }
    case OpCode::Nop:
        break;
    case OpCode::Eot:
        return false;
        break;
    case OpCode::Mask: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const mask = read_immediate(static_cast<AddressMode>((attr >> 3 & 0x7)));
        traceff(" mask:{:08x}", mask);
        auto const src = read_src(attr);
        write_dst(dst, (dst.value() & mask) | src.value());
        break;
    }
    case OpCode::PostCard: {
        traceff("=> {:02x}", read8());
        break;
    }
    case OpCode::Beep: {
        dmesgln_pci(m_adapter, "beep!");
        break;
    }
    case OpCode::SetDataBlock: {
        u8 index = read8();
        traceff(" block:{:02x}", index);
        switch (index) {
        case 0:
            m_ctx.data_block = 0;
            break;
        case 255:
            m_ctx.data_block = m_cmd_desc.base;
            break;
        default:
            m_ctx.data_block = m_adapter.bios().datatable(index);
            break;
        }
        traceff(" base:{:02x}", m_ctx.data_block);
        break;
    }
    case OpCode::Xor: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        write_dst(dst, dst.value() ^ src.value());
        break;
    }
    case OpCode::Shl: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        auto result = dst;
        if (src.value() < 32)
            result.full_value = dst.full_value << src.value();
        else
            result.full_value = 0;
        write_dst(dst, result.value());
        break;
    }
    case OpCode::Shr: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        auto result = dst;
        if (src.value() < 32)
            result.full_value = dst.full_value >> src.value();
        else
            result.full_value = 0;
        write_dst(dst, result.value());
        break;
    }
    case OpCode::Debug:
        traceff(" => {:02x}", read8());
        break;
    case OpCode::ProcessDS:
        traceff(" => {:04x}", read16());
        break;
    case OpCode::Mul32: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        u64 result = static_cast<u64>(dst.value()) * static_cast<u64>(src.value());
        m_ctx.divmul[0] = result & 0xFFFFFFFF;
        m_ctx.divmul[1] = result >> 32;
        traceff(" => {:08x} {:08x}", m_ctx.divmul[1], m_ctx.divmul[0]);
        break;
    }
    case OpCode::Div32: {
        u8 attr = read8();
        auto const dst = read_dst(desc.operand.dst_loc, attr);
        auto const src = read_src(attr);
        if (src.value() != 0) {
            u64 num = dst.value() | (static_cast<u64>(m_ctx.divmul[1]) << 32);
            u64 result = num / src.value();
            m_ctx.divmul[0] = result & 0xFFFFFFFF;
            m_ctx.divmul[1] = result >> 32;
        } else {
            m_ctx.divmul[0] = 0;
            m_ctx.divmul[1] = 0;
        }
        traceff(" => {:08x} {:08x}", m_ctx.divmul[1], m_ctx.divmul[0]);
        break;
    }
    // Linux does not implement the following opcodes, so they must not be used...
    case OpCode::Repeat:
    case OpCode::SaveReg:
    case OpCode::RestoreReg:
        dmesgln_pci(m_adapter, "Atom: Unimplemented opcode: {}", opcode_name_table[to_underlying(desc.opcode)]);
        return Error::from_errno(ENOTIMPL);
    }

    return true;
}

u8 Interpreter::read8()
{
    return m_adapter.bios().read8(m_cmd_desc.base + m_pc++);
}

u16 Interpreter::read16()
{
    return read8() | (read8() << 8);
}

u32 Interpreter::read32()
{
    return read16() | (read16() << 16);
}

Operand Interpreter::read_dst(Location loc, u8 attr)
{
    auto const dst_mod = (attr >> 6) & 0x3;
    auto const address_mode = (attr >> 3) & 0x7;
    auto const dst_attr = to_underlying(loc)
        | (to_underlying(src_to_dst_align[address_mode][dst_mod]) << 3);
    return read_src(dst_attr);
}

Operand Interpreter::read_dst_skip(Location dst_loc, u8 attr)
{
    attr = to_underlying(dst_loc) | (to_underlying(src_to_dst_align[(attr >> 3) & 0x7][(attr >> 6) & 0x3]) << 3);

    u16 pc = m_pc;
    Location loc = static_cast<Location>(attr & 0x7);
    AddressMode address_mode = static_cast<AddressMode>((attr >> 3) & 0x7);
    u32 value = 0xCDCDCDCD;

    switch (loc) {
    case Location::Register: {
        u32 index = read16() + m_ctx.reg_block;
        traceff(" reg[{:04x}]", index);
        break;
    }
    case Location::ParameterSpace: {
        u8 index = read8();
        traceff(" ps[{:02x}]", index);
        break;
    }
    case Location::WorkSpace: {
        u8 index = read8();
        switch (static_cast<WorkSpace>(index)) {
        case WorkSpace::Quotient:
            trace(" ws[quotient]"sv);
            break;
        case WorkSpace::Remainder:
            trace(" ws[remainder]"sv);
            break;
        case WorkSpace::DataPtr:
            trace(" ws[dataptr]"sv);
            break;
        case WorkSpace::Shift:
            trace(" ws[dataptr]"sv);
            break;
        case WorkSpace::OrMask:
            trace(" ws[ormask]"sv);
            break;
        case WorkSpace::AndMask:
            trace(" ws[andmask]"sv);
            break;
        case WorkSpace::FBWindow:
            trace(" ws[fbwindow]"sv);
            break;
        case WorkSpace::Attributes:
            trace(" ws[attributes]"sv);
            break;
        case WorkSpace::RegPtr:
            trace(" ws[regptr]"sv);
            break;
        default:
            traceff(" ws[{:02x}]", index);
            break;
        }
        break;
    }
    case Location::ID: {
        u16 index = read16();
        traceff(" id[{:04x}]", index);
        break;
    }
    case Location::FrameBuffer: {
        u8 index = read8();
        traceff(" fb[{:02x}]", index);
        break;
    }
    case Location::Immediate: {
        u32 value = read_immediate(address_mode);
        traceff(" imm:[{:08x}]", value);
        VERIFY_NOT_REACHED();
        break;
    }
    case Location::PhaseLockedLoop: {
        u8 index = read8();
        traceff(" pll[{:02x}]", index);
        // TODO: Error?
        VERIFY_NOT_REACHED();
        break;
    }
    case Location::MemController: {
        u8 index = read8();
        traceff(" mc[{:02x}]", index);
        // TODO: Error?
        VERIFY_NOT_REACHED();
        break;
    }
    }
    trace("[        ]"sv);

    return Operand { value, loc, address_mode, pc };
}

Operand Interpreter::read_src(u8 attr)
{
    u16 pc = m_pc;
    Location loc = static_cast<Location>(attr & 0x7);
    AddressMode address_mode = static_cast<AddressMode>((attr >> 3) & 0x7);
    u32 value = 0xCDCDCDCD;
    switch (loc) {
    case Location::Register: {
        u32 index = read16() + m_ctx.reg_block;
        traceff(" reg[{:04x}]", index);
        switch (m_ctx.io_mode) {
        case IOMode::MemoryMapped:
            value = m_adapter.device().read_register(index);
            break;
        case IOMode::PCI:
            dmesgln_pci(m_adapter, "Atom: Reading from PCI registers not implemented");
            VERIFY_NOT_REACHED();
            break;
        case IOMode::SysIO:
            dmesgln_pci(m_adapter, "Atom: Reading from SysIO registers not implemented");
            VERIFY_NOT_REACHED();
            break;
        default:
            value = MUST(execute_iio(m_ctx.iio_program & 0x7F, index, 0));
            break;
        }
        break;
    }
    case Location::ParameterSpace: {
        u8 index = read8();
        traceff(" ps[{:02x}]", index);
        // TODO: Linux notes that this access may be unaligned. Why?
        value = m_parameter_space[index];
        break;
    }
    case Location::WorkSpace: {
        u8 index = read8();
        switch (static_cast<WorkSpace>(index)) {
        case WorkSpace::Quotient:
            trace(" ws[quotient]"sv);
            value = m_ctx.divmul[0];
            break;
        case WorkSpace::Remainder:
            trace(" ws[remainder]"sv);
            value = m_ctx.divmul[1];
            break;
        case WorkSpace::DataPtr:
            trace(" ws[dataptr]"sv);
            value = m_ctx.data_block;
            break;
        case WorkSpace::Shift:
            trace(" ws[dataptr]"sv);
            value = m_ctx.shift;
            break;
        case WorkSpace::OrMask:
            trace(" ws[ormask]"sv);
            value = 1 << m_ctx.shift;
            break;
        case WorkSpace::AndMask:
            trace(" ws[andmask]"sv);
            value = ~(1 << m_ctx.shift);
            break;
        case WorkSpace::FBWindow:
            trace(" ws[fbwindow]"sv);
            value = m_ctx.fb_base;
            break;
        case WorkSpace::Attributes:
            trace(" ws[attributes]"sv);
            value = m_ctx.io_attr;
            break;
        case WorkSpace::RegPtr:
            trace(" ws[regptr]"sv);
            value = m_ctx.reg_block;
            break;
        default:
            traceff(" ws[{:02x}]", index);
            value = m_workspace[index];
            break;
        }
        break;
    }
    case Location::ID: {
        u16 index = read16();
        traceff(" id[{:04x}]", index);
        value = m_adapter.bios().read32(index + m_ctx.data_block);
        break;
    }
    case Location::FrameBuffer: {
        u8 index = read8();
        traceff(" fb[{:02x}]", index);
        // TODO: Implement
        dbgln("todo: framebuffer read");
        break;
    }
    case Location::Immediate: {
        value = read_immediate(address_mode);
        trace(" imm:"sv);
        break;
    }
    case Location::PhaseLockedLoop: {
        u8 index = read8();
        traceff(" pll[{:02x}]", index);
        dmesgln_pci(m_adapter, "Atom: reading from unimplemented pll");
        // TODO: Error?
        VERIFY_NOT_REACHED();
        break;
    }
    case Location::MemController: {
        u8 index = read8();
        traceff(" mc[{:02x}]", index);
        dmesgln_pci(m_adapter, "Atom: reading from unimplemented mc");
        // TODO: Error?
        VERIFY_NOT_REACHED();
        break;
    }
    }

    auto const op = Operand { value, loc, address_mode, pc };
    switch (address_mode) {
    case AddressMode::DWord:
        traceff("[{:08x}]", op.value());
        break;
    case AddressMode::Word0:
        traceff("[    {:04x}]", op.value());
        break;
    case AddressMode::Word8:
        traceff("[  {:04x}  ]", op.value());
        break;
    case AddressMode::Word16:
        traceff("[{:04x}    ]", op.value());
        break;
    case AddressMode::Byte0:
        traceff("[      {:02x}]", op.value());
        break;
    case AddressMode::Byte8:
        traceff("[    {:02x}  ]", op.value());
        break;
    case AddressMode::Byte16:
        traceff("[  {:02x}    ]", op.value());
        break;
    case AddressMode::Byte24:
        traceff("[{:02x}      ]", op.value());
        break;
    }

    return op;
}

u32 Interpreter::read_immediate(AddressMode mode)
{
    switch (mode) {
    case AddressMode::DWord:
        return read32();
    case AddressMode::Word0:
    case AddressMode::Word8:
    case AddressMode::Word16:
        return read16();
    case AddressMode::Byte0:
    case AddressMode::Byte8:
    case AddressMode::Byte16:
    case AddressMode::Byte24:
        return read8();
    }
}

void Interpreter::write_dst(Operand const& op, u32 value)
{
    u16 saved_pc = m_pc;
    ScopeGuard restore_pc = [this, &saved_pc] {
        m_pc = saved_pc;
    };

    m_pc = op.pc;

    switch (op.address_mode) {
    case AddressMode::DWord:
        traceff(" => [{:08x}]", value);
        break;
    case AddressMode::Word0:
        traceff(" => [    {:04x}]", value);
        value = (op.full_value & 0xFFFF0000) | value;
        break;
    case AddressMode::Word8:
        traceff(" => [  {:04x}  ]", value);
        value = (op.full_value & 0xFF0000FF) | (value << 8);
        break;
    case AddressMode::Word16:
        traceff(" => [{:04x}    ]", value);
        value = (op.full_value & 0x0000FFFF) | (value << 16);
        break;
    case AddressMode::Byte0:
        traceff(" => [      {:02x}]", value);
        value = (op.full_value & 0xFFFFFF00) | value;
        break;
    case AddressMode::Byte8:
        traceff(" => [    {:02x}  ]", value);
        value = (op.full_value & 0xFFFF00FF) | (value << 8);
        break;
    case AddressMode::Byte16:
        traceff(" => [  {:02x}    ]", value);
        value = (op.full_value & 0xFF00FFFF) | (value << 16);
        break;
    case AddressMode::Byte24:
        traceff(" => [{:02x}      ]", value);
        value = (op.full_value & 0x00FFFFFF) | (value << 24);
        break;
    }

    flush_trace();

    switch (op.loc) {
    case Location::Register: {
        u32 index = read16() + m_ctx.reg_block;
        switch (m_ctx.io_mode) {
        case IOMode::MemoryMapped:
            if (index == 0) {
                m_adapter.device().write_register(index, value << 2);
            } else {
                m_adapter.device().write_register(index, value);
            }
            break;
        case IOMode::PCI:
            dmesgln_pci(m_adapter, "Atom: PCI registers are not implemented");
            VERIFY_NOT_REACHED();
            break;
        case IOMode::SysIO:
            dmesgln_pci(m_adapter, "Atom SysIO registers are not implemented");
            VERIFY_NOT_REACHED();
            break;
        case IOMode::IIO:
            MUST(execute_iio(m_ctx.iio_program | 0x80, index, value));
            break;
        }
        break;
    }
    case Location::ParameterSpace: {
        u8 index = read8();
        m_parameter_space[index] = value;
        break;
    }
    case Location::WorkSpace: {
        u8 index = read8();
        switch (static_cast<WorkSpace>(index)) {
        case WorkSpace::Quotient:
            m_ctx.divmul[0] = value;
            break;
        case WorkSpace::Remainder:
            m_ctx.divmul[1] = value;
            break;
        case WorkSpace::DataPtr:
            m_ctx.data_block = value;
            break;
        case WorkSpace::Shift:
            m_ctx.shift = value;
            break;
        case WorkSpace::OrMask:
        case WorkSpace::AndMask:
            break;
        case WorkSpace::FBWindow:
            m_ctx.fb_base = value;
            break;
        case WorkSpace::Attributes:
            m_ctx.io_attr = value;
            break;
        case WorkSpace::RegPtr:
            m_ctx.reg_block = value;
            break;
        default:
            m_workspace[index] = value;
            break;
        }
        break;
    }
    case Location::FrameBuffer: {
        u8 index = read8();
        // TODO: Implement
        dbgln("todo: framebuffer write");
        (void)index;
        break;
    }
    case Location::PhaseLockedLoop: {
        u8 index = read8();
        dmesgln_pci(m_adapter, "Atom: writing to unimplemented pll");
        (void)index;
        break;
    }
    case Location::MemController: {
        u8 index = read8();
        (void)index;
        dmesgln_pci(m_adapter, "Atom: writing to unimplemented mc");
        break;
    }
    case Location::ID:
    case Location::Immediate:
        // TODO: Make error?
        VERIFY_NOT_REACHED();
    }
}

void Interpreter::trace(StringView msg)
{
    if (trace_enabled()) {
        m_trace.appendff(msg);
    }
}

void Interpreter::flush_trace()
{
    if (m_trace.length() != 0 && trace_enabled()) {
        dbgln("Atom: [{}] {}", m_debug_depth, m_trace.string_view());
        m_trace.clear();
    }
}

bool Interpreter::trace_enabled() const
{
    return m_adapter.bios().atom_debug_enabled();
}

}
