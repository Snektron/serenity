/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Endian.h>
#include <AK/Types.h>
#include <Kernel/Library/StdLib.h>

namespace Kernel::Graphics::AMD::Atom {

struct [[gnu::packed]] ROM {
    LittleEndian<u16> magic;
    u8 reserved[45];
    u8 number_of_strings;
    u8 ati_magic[10];
    u8 reserved2[14];
    LittleEndian<u16> rom_table_offset;
    u8 reserved3[36];
    LittleEndian<u16> vbios_name_offset;
};
static_assert(offsetof(ROM, number_of_strings) == 0x2F);
static_assert(offsetof(ROM, ati_magic) == 0x30);
static_assert(offsetof(ROM, rom_table_offset) == 0x48);
static_assert(offsetof(ROM, vbios_name_offset) == 0x6E);
static_assert(sizeof(ROM) == 0x70);

struct [[gnu::packed]] TableHeader {
    u16 structure_size;
    u8 format_revision;
    u8 content_revision;
};

struct [[gnu::packed]] ROMTable {
    TableHeader header;
    u8 magic[4];
    LittleEndian<u16> bios_segment_address;
    LittleEndian<u16> protected_mode_offset;
    LittleEndian<u16> config_filename_offset;
    LittleEndian<u16> crc_block_offset;
    LittleEndian<u16> vbios_bootup_message_offset;
    LittleEndian<u16> int10_offset;
    LittleEndian<u16> pci_bus_dev_init_code;
    LittleEndian<u16> io_base_address;
    LittleEndian<u16> subsystem_vendor_id;
    LittleEndian<u16> subsystem_id;
    LittleEndian<u16> pci_info_offset;
    LittleEndian<u16> cmd_table_offset;
    LittleEndian<u16> data_table_offset;
    LittleEndian<u16> reserved;
};

enum class Command {
    AsicInit = 0x00,
};

struct [[gnu::packed]] CommandTable {
    TableHeader header;
    LittleEndian<u16> commands[];
};

struct [[gnu::packed]] DataTable {
    TableHeader header;
    u16 data[];
};

struct [[gnu::packed]] DataTableV11 {
    TableHeader header;
    LittleEndian<u16> utility_pipeline;
    LittleEndian<u16> multimedia_capability_info;
    LittleEndian<u16> multimedia_config_info;
    LittleEndian<u16> standard_vesa_timing;
    LittleEndian<u16> firmware_info;
    LittleEndian<u16> palette_data;
    LittleEndian<u16> lcd_info;
    LittleEndian<u16> dig_transmitter_info;
    LittleEndian<u16> smu_info;
    LittleEndian<u16> datatable9;
    LittleEndian<u16> gpio_i2c_info;
    LittleEndian<u16> vram_usage_by_firmware;
    LittleEndian<u16> gpio_pin_lut;
    LittleEndian<u16> vesa_to_internal_mode_lut;
    LittleEndian<u16> gfx_info;
    LittleEndian<u16> powerplay_info;
    LittleEndian<u16> datatable16;
    LittleEndian<u16> save_restore_info;
    LittleEndian<u16> ppll_ss_info;
    LittleEndian<u16> datatable19;
    LittleEndian<u16> datatable20;
    LittleEndian<u16> mclk_ss_info;
    LittleEndian<u16> object_header;
    LittleEndian<u16> indirect_io_access;
    LittleEndian<u16> asic_vddc_info;
    LittleEndian<u16> asic_mvddc_info;
    LittleEndian<u16> tv_videomode;
    LittleEndian<u16> vram_info;
    LittleEndian<u16> memory_training_info;
    LittleEndian<u16> integrated_system_info;
    LittleEndian<u16> asic_profiling_info;
    LittleEndian<u16> voltage_object_info;
    LittleEndian<u16> power_source_info;
    LittleEndian<u16> service_info;
};

struct [[gnu::packed]] FirmwareInfoV22 {
    TableHeader header;
    LittleEndian<u32> firmware_revision;
    LittleEndian<u32> default_sclk_freq; // in 10KHz
    LittleEndian<u32> default_mclk_freq; // in 10KHz
    // NOTE: Many more fields are in this structure, but we dont care about them for now.
};

struct [[gnu::packed]] alignas(u32) AsicInitV11Parameters {
    u32 sclk_freq;
    u32 mclk_freq;
    u32 reserved[14] { 0 };
};

struct [[gnu::packed]] CommandTableEntry {
    LittleEndian<u16> size;
    LittleEndian<u16> reserved;
    u8 ws;
    u8 ps : 7;
    u8 reserved2 : 1;
};
static_assert(offsetof(CommandTableEntry, ws) == 4);
static_assert(sizeof(CommandTableEntry) == 6);

enum class OpCode : u8 {
    Invalid,
    Move,
    And,
    Or,
    ShiftLeft,
    ShiftRight,
    Mul,
    Div,
    Add,
    Sub,
    SetPort,
    SetRegBlock,
    SetFBBase,
    Compare,
    Switch,
    Jump,
    Test,
    Delay,
    CallTable,
    Repeat,
    Clear,
    Nop,
    Eot,
    Mask,
    PostCard,
    Beep,
    SaveReg,
    RestoreReg,
    SetDataBlock,
    Xor,
    Shl,
    Shr,
    Debug,
    ProcessDS,
    Mul32,
    Div32,
};

enum class Location {
    Register,
    ParameterSpace,
    WorkSpace,
    FrameBuffer,
    ID,
    Immediate,
    PhaseLockedLoop,
    MemController,
};

enum class Condition {
    Always,
    Equal,
    Below,
    Above,
    BelowOrEqual,
    AboveOrEqual,
    NotEqual,
};

enum class Port {
    ATI,
    PCI,
    SysIO,
};

enum class Unit {
    MilliSecond,
    MicroSecond,
};

enum class AddressMode {
    DWord,
    Word0,
    Word8,
    Word16,
    Byte0,
    Byte8,
    Byte16,
    Byte24,
};

enum class IOMode {
    MemoryMapped,
    PCI,
    SysIO,
    IIO,
};

enum class WorkSpace {
    Quotient = 0x40,
    Remainder = 0x41,
    DataPtr = 0x42,
    Shift = 0x43,
    OrMask = 0x44,
    AndMask = 0x45,
    FBWindow = 0x46,
    Attributes = 0x47,
    RegPtr = 0x48,
};

enum class IndirectIO {
    Nop = 0,
    Start = 1,
    Read = 2,
    Write = 3,
    Clear = 4,
    Set = 5,
    MoveIndex = 6,
    MoveAttr = 7,
    MoveData = 8,
    End = 9,
};

constexpr u8 CaseMagic = 0x63;
constexpr u8 CaseEnd = 0x5A;
constexpr u32 MaxIIOPrograms = 256;

// This is not an atom structure really, but it lets us group the interesting
// bits from a command table entry together.
struct CommandDescriptor {
    u16 base;
    u16 size;
    u8 work_space_size;
    u8 parameter_space_size;
};

}
