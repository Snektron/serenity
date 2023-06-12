/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/Types.h>
#include <Kernel/Library/KBuffer.h>
#include <Kernel/Library/StdLib.h>

namespace Kernel {

class AMDNativeGraphicsAdapter;

};

namespace Kernel::Graphics::AMD {

// Atom definitions from atom.h and atomfirmware.h
class AtomBios final {
public:
    struct [[gnu::packed]] ROM {
        u16 magic;
        u8 reserved[46];
        u8 ati_magic[10];
        u8 reserved2[14];
        u16 rom_table_offset;
    };
    static_assert(offsetof(ROM, ati_magic) == 0x30);
    static_assert(offsetof(ROM, rom_table_offset) == 0x48);
    static_assert(sizeof(ROM) == 0x4A);

    struct [[gnu::packed]] TableHeader {
        u16 structure_size;
        u8 format_revision;
        u8 content_revision;
    };

    struct [[gnu::packed]] ROMTable {
        TableHeader header;
        u8 magic[4];
        u16 bios_segment_address;
        u16 protected_mode_offset;
        u16 config_filename_offset;
        u16 crc_block_offset;
        u16 vbios_bootup_message_offset;
        u16 int10_offset;
        u16 pci_bus_dev_init_code;
        u16 io_base_address;
        u16 subsystem_vendor_id;
        u16 subsystem_id;
        u16 pci_info_offset;
        u16 cmd_table_offset;
        u16 data_table_offset;
        u16 reserved;
    };

    enum class Command {
        AsicInit = 0x00,
    };

    struct [[gnu::packed]] CommandTable {
        TableHeader header;
        u16 commands[];
    };

    static ErrorOr<AtomBios> try_create(AMDNativeGraphicsAdapter& adapter);

private:
    explicit AtomBios(NonnullOwnPtr<KBuffer>&& bios);

    static ErrorOr<AtomBios> try_create_from_kbuffer(NonnullOwnPtr<KBuffer>&& bios_buffer);
    static ErrorOr<AtomBios> try_create_from_expansion_rom(AMDNativeGraphicsAdapter& adapter);

    bool is_valid() const;

    NonnullOwnPtr<KBuffer> m_bios;
};

}
