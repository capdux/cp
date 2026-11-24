// lib/coff/index.cp — Windows PE/COFF binary header & section table parser (cap:coff)
// Reuses Zig 0.17 std.coff constants and enums; zero heap allocation, strict bounds checks.

const std = @import("std");

pub err CoffError {
    Truncated,
    InvalidDosMagic,
    InvalidPeOffset,
    InvalidPeSignature,
    InvalidOptionalMagic,
    SectionOutOfBounds,
}

pub const DOS_MAGIC: u16 = 0x5A4D;
pub const PE_SIGNATURE: []const u8 = std.coff.pe_signature;
pub const PE_POINTER_OFFSET: usize = std.coff.pe_pointer_offset;
pub const COFF_HEADER_SIZE: usize = 20;
pub const SECTION_HEADER_SIZE: usize = 40;

pub const OPT_MAGIC_PE32: u16 = std.coff.IMAGE_NT_OPTIONAL_HDR32_MAGIC;
pub const OPT_MAGIC_PE32_PLUS: u16 = std.coff.IMAGE_NT_OPTIONAL_HDR64_MAGIC;

pub type Machine = std.coff.IMAGE.FILE.MACHINE;
pub type Subsystem = std.coff.Subsystem;

pub const MACHINE_UNKNOWN: u16 = @intFromEnum(std.coff.IMAGE.FILE.MACHINE.UNKNOWN);
pub const MACHINE_I386: u16 = @intFromEnum(std.coff.IMAGE.FILE.MACHINE.I386);
pub const MACHINE_AMD64: u16 = @intFromEnum(std.coff.IMAGE.FILE.MACHINE.AMD64);
pub const MACHINE_ARM: u16 = @intFromEnum(std.coff.IMAGE.FILE.MACHINE.ARM);
pub const MACHINE_ARM64: u16 = @intFromEnum(std.coff.IMAGE.FILE.MACHINE.ARM64);
pub const MACHINE_RISCV64: u16 = @intFromEnum(std.coff.IMAGE.FILE.MACHINE.RISCV64);

pub const SUBSYSTEM_UNKNOWN: u16 = @intFromEnum(std.coff.Subsystem.UNKNOWN);
pub const SUBSYSTEM_NATIVE: u16 = @intFromEnum(std.coff.Subsystem.NATIVE);
pub const SUBSYSTEM_WINDOWS_GUI: u16 = @intFromEnum(std.coff.Subsystem.WINDOWS_GUI);
pub const SUBSYSTEM_WINDOWS_CUI: u16 = @intFromEnum(std.coff.Subsystem.WINDOWS_CUI);
pub const SUBSYSTEM_EFI_APPLICATION: u16 = @intFromEnum(std.coff.Subsystem.EFI_APPLICATION);

pub const FLAG_EXECUTABLE_IMAGE: u16 = 0x0002;
pub const FLAG_LARGE_ADDRESS_AWARE: u16 = 0x0020;
pub const FLAG_DLL: u16 = 0x2000;

fn coffReadU16Le(bytes: []const u8, off: usize): u16 {
    const b0: u16 = bytes[off];
    const b1: u16 = bytes[off + 1];
    return b0 | (b1 << 8);
}

fn coffReadU32Le(bytes: []const u8, off: usize): u32 {
    const b0: u32 = bytes[off];
    const b1: u32 = bytes[off + 1];
    const b2: u32 = bytes[off + 2];
    const b3: u32 = bytes[off + 3];
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn coffReadU64Le(bytes: []const u8, off: usize): u64 {
    const lo: u64 = coffReadU32Le(bytes, off);
    const hi: u64 = coffReadU32Le(bytes, off + 4);
    return lo | (hi << 32);
}

pub struct CoffHeader {
    pub machine: u16 = 0;
    pub number_of_sections: u16 = 0;
    pub time_date_stamp: u32 = 0;
    pub pointer_to_symbol_table: u32 = 0;
    pub number_of_symbols: u32 = 0;
    pub size_of_optional_header: u16 = 0;
    pub characteristics: u16 = 0;

    pub fn isExecutable(): bool {
        return (_.characteristics & FLAG_EXECUTABLE_IMAGE) != 0;
    }

    pub fn isDll(): bool {
        return (_.characteristics & FLAG_DLL) != 0;
    }

    pub fn isLargeAddressAware(): bool {
        return (_.characteristics & FLAG_LARGE_ADDRESS_AWARE) != 0;
    }
}

pub struct SectionHeader {
    pub name_bytes: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    pub virtual_size: u32 = 0;
    pub virtual_address: u32 = 0;
    pub size_of_raw_data: u32 = 0;
    pub pointer_to_raw_data: u32 = 0;
    pub pointer_to_relocations: u32 = 0;
    pub pointer_to_linenumbers: u32 = 0;
    pub number_of_relocations: u16 = 0;
    pub number_of_linenumbers: u16 = 0;
    pub characteristics: u32 = 0;

    pub fn nameLen(): usize {
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            if (_.name_bytes[i] == 0) {
                return i;
            }
        }
        return 8;
    }

    pub fn nameEquals(expected: []const u8): bool {
        const len = _.nameLen();
        if (len != expected.len) {
            return false;
        }
        let i: usize = 0;
        while (i < len) : (i += 1) {
            if (_.name_bytes[i] != expected[i]) {
                return false;
            }
        }
        return true;
    }

    pub fn isCode(): bool {
        return (_.characteristics & 0x00000020) != 0;
    }

    pub fn isExecutable(): bool {
        return (_.characteristics & 0x20000000) != 0;
    }

    pub fn isReadable(): bool {
        return (_.characteristics & 0x40000000) != 0;
    }

    pub fn isWritable(): bool {
        return (_.characteristics & 0x80000000) != 0;
    }
}

pub struct PeInfo {
    pub pe_offset: u32 = 0;
    pub coff: CoffHeader = CoffHeader{};
    pub has_optional_header: bool = false;
    pub is_pe32_plus: bool = false;
    pub optional_magic: u16 = 0;
    pub entry_point_rva: u32 = 0;
    pub image_base: u64 = 0;
    pub section_alignment: u32 = 0;
    pub file_alignment: u32 = 0;
    pub size_of_image: u32 = 0;
    pub size_of_headers: u32 = 0;
    pub subsystem: u16 = 0;
    pub dll_characteristics: u16 = 0;
    pub section_table_offset: usize = 0;

    pub fn sectionCount(): usize {
        return _.coff.number_of_sections as usize;
    }

    pub fn parseSection(bytes: []const u8, index: usize): SectionHeader! {
        if (index >= (_.coff.number_of_sections as usize)) {
            return error.SectionOutOfBounds;
        }
        const off = _.section_table_offset + index * SECTION_HEADER_SIZE;
        return parseSectionHeader(bytes, off);
    }
}

pub fn parseCoffHeader(bytes: []const u8): CoffHeader! {
    if (bytes.len < COFF_HEADER_SIZE) {
        return error.Truncated;
    }
    return CoffHeader{
        machine: coffReadU16Le(bytes, 0),
        number_of_sections: coffReadU16Le(bytes, 2),
        time_date_stamp: coffReadU32Le(bytes, 4),
        pointer_to_symbol_table: coffReadU32Le(bytes, 8),
        number_of_symbols: coffReadU32Le(bytes, 12),
        size_of_optional_header: coffReadU16Le(bytes, 16),
        characteristics: coffReadU16Le(bytes, 18),
    };
}

pub fn parseSectionHeader(bytes: []const u8, offset: usize): SectionHeader! {
    if (offset > bytes.len || bytes.len - offset < SECTION_HEADER_SIZE) {
        return error.SectionOutOfBounds;
    }
    let name: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        name[i] = bytes[offset + i];
    }
    return SectionHeader{
        name_bytes: name,
        virtual_size: coffReadU32Le(bytes, offset + 8),
        virtual_address: coffReadU32Le(bytes, offset + 12),
        size_of_raw_data: coffReadU32Le(bytes, offset + 16),
        pointer_to_raw_data: coffReadU32Le(bytes, offset + 20),
        pointer_to_relocations: coffReadU32Le(bytes, offset + 24),
        pointer_to_linenumbers: coffReadU32Le(bytes, offset + 28),
        number_of_relocations: coffReadU16Le(bytes, offset + 32),
        number_of_linenumbers: coffReadU16Le(bytes, offset + 34),
        characteristics: coffReadU32Le(bytes, offset + 36),
    };
}

pub fn parsePeHeader(bytes: []const u8): PeInfo! {
    if (bytes.len < PE_POINTER_OFFSET + 4) {
        return error.Truncated;
    }
    if (coffReadU16Le(bytes, 0) != DOS_MAGIC) {
        return error.InvalidDosMagic;
    }
    const pe_off_u32 = coffReadU32Le(bytes, PE_POINTER_OFFSET);
    const pe_off: usize = pe_off_u32 as usize;
    if (pe_off < 64 || pe_off > bytes.len || bytes.len - pe_off < 4 + COFF_HEADER_SIZE) {
        return error.InvalidPeOffset;
    }
    if (bytes[pe_off] != 0x50 || bytes[pe_off + 1] != 0x45 || bytes[pe_off + 2] != 0 || bytes[pe_off + 3] != 0) {
        return error.InvalidPeSignature;
    }

    const coff_off = pe_off + 4;
    const coff_hdr = parseCoffHeader(bytes[coff_off..])?;
    const opt_off = coff_off + COFF_HEADER_SIZE;
    const opt_size: usize = coff_hdr.size_of_optional_header as usize;

    if (opt_size > bytes.len - opt_off) {
        return error.Truncated;
    }

    let info = PeInfo{
        pe_offset: pe_off_u32,
        coff: coff_hdr,
        section_table_offset: opt_off + opt_size,
    };

    if (opt_size >= 2) {
        const magic = coffReadU16Le(bytes, opt_off);
        info.has_optional_header = true;
        info.optional_magic = magic;

        if (magic == OPT_MAGIC_PE32) {
            if (opt_size < 96) {
                return error.Truncated;
            }
            info.is_pe32_plus = false;
            info.entry_point_rva = coffReadU32Le(bytes, opt_off + 16);
            info.image_base = coffReadU32Le(bytes, opt_off + 28) as u64;
            info.section_alignment = coffReadU32Le(bytes, opt_off + 32);
            info.file_alignment = coffReadU32Le(bytes, opt_off + 36);
            info.size_of_image = coffReadU32Le(bytes, opt_off + 56);
            info.size_of_headers = coffReadU32Le(bytes, opt_off + 60);
            info.subsystem = coffReadU16Le(bytes, opt_off + 68);
            info.dll_characteristics = coffReadU16Le(bytes, opt_off + 70);
        } else if (magic == OPT_MAGIC_PE32_PLUS) {
            if (opt_size < 112) {
                return error.Truncated;
            }
            info.is_pe32_plus = true;
            info.entry_point_rva = coffReadU32Le(bytes, opt_off + 16);
            info.image_base = coffReadU64Le(bytes, opt_off + 24);
            info.section_alignment = coffReadU32Le(bytes, opt_off + 32);
            info.file_alignment = coffReadU32Le(bytes, opt_off + 36);
            info.size_of_image = coffReadU32Le(bytes, opt_off + 56);
            info.size_of_headers = coffReadU32Le(bytes, opt_off + 60);
            info.subsystem = coffReadU16Le(bytes, opt_off + 68);
            info.dll_characteristics = coffReadU16Le(bytes, opt_off + 70);
        } else {
            return error.InvalidOptionalMagic;
        }
    }

    return info;
}

pub fn isPe(bytes: []const u8): bool {
    const res = parsePeHeader(bytes) catch return false;
    return res.pe_offset >= 64;
}

pub fn machineName(code: u16): []const u8 {
    if (code == MACHINE_AMD64) { return "AMD64"; }
    if (code == MACHINE_ARM64) { return "ARM64"; }
    if (code == MACHINE_I386) { return "I386"; }
    if (code == MACHINE_ARM) { return "ARM"; }
    if (code == MACHINE_RISCV64) { return "RISCV64"; }
    return "UNKNOWN";
}

pub fn subsystemName(code: u16): []const u8 {
    if (code == SUBSYSTEM_NATIVE) { return "NATIVE"; }
    if (code == SUBSYSTEM_WINDOWS_GUI) { return "WINDOWS_GUI"; }
    if (code == SUBSYSTEM_WINDOWS_CUI) { return "WINDOWS_CUI"; }
    if (code == SUBSYSTEM_EFI_APPLICATION) { return "EFI_APPLICATION"; }
    return "UNKNOWN";
}
