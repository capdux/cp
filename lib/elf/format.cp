// Source: /data/dev/bun/src/exe_format/
// lib/elf/format.cp
// Executable Binary Format Detection & ELF64 / Mach-O / PE Header Inspector
// Absorbed from Bun src/exe_format/elf.cp, macho.cp & pe.cp

const std = @import("std");

pub enum ExeFormatKind: u8 {
    Unknown = 0,
    Elf = 1,
    MachO64 = 2,
    MachOFat = 3,
    Pe = 4,
    Wasm = 5,
    MachO32 = 6,
    Shebang = 7,
    Elf32 = 8,
}

pub enum ExeFormat: u8 {
    Unknown = 0,
    Elf32 = 1,
    Elf64 = 2,
    MachO32 = 3,
    MachO64 = 4,
    MachOFat = 5,
    Pe = 6,
    Wasm = 7,
    Shebang = 8,

    pub static fn detect(bytes: []const u8): ExeFormat {
        if (bytes.len >= 2 && bytes[0] == '#' && bytes[1] == '!') {
            return ExeFormat.Shebang;
        }
        if (bytes.len >= 4) {
            // ELF: \x7F 'E' 'L' 'F'
            if (bytes[0] == 0x7F && bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F') {
                if (bytes.len >= 5 && bytes[4] == 1) {
                    return ExeFormat.Elf32;
                }
                return ExeFormat.Elf64;
            }
            // Mach-O 64-bit little-endian (0xCFFAEDFE) or big-endian (0xFEEDFACF)
            if ((bytes[0] == 0xCF && bytes[1] == 0xFA && bytes[2] == 0xED && bytes[3] == 0xFE) ||
                (bytes[0] == 0xFE && bytes[1] == 0xED && bytes[2] == 0xFA && bytes[3] == 0xCF))
            {
                return ExeFormat.MachO64;
            }
            // Mach-O 32-bit little-endian (0xCEFAEDFE) or big-endian (0xFEEDFACE)
            if ((bytes[0] == 0xCE && bytes[1] == 0xFA && bytes[2] == 0xED && bytes[3] == 0xFE) ||
                (bytes[0] == 0xFE && bytes[1] == 0xED && bytes[2] == 0xFA && bytes[3] == 0xCE))
            {
                return ExeFormat.MachO32;
            }
            // Mach-O Universal Fat binary (0xCAFEBABE or 0xBEBAFECA)
            if ((bytes[0] == 0xCA && bytes[1] == 0xFE && bytes[2] == 0xBA && bytes[3] == 0xBE) ||
                (bytes[0] == 0xBE && bytes[1] == 0xBA && bytes[2] == 0xFE && bytes[3] == 0xCA))
            {
                return ExeFormat.MachOFat;
            }
            // WebAssembly: \x00 'a' 's' 'm'
            if (bytes[0] == 0x00 && bytes[1] == 'a' && bytes[2] == 's' && bytes[3] == 'm') {
                return ExeFormat.Wasm;
            }
        }
        if (bytes.len >= 2 && bytes[0] == 'M' && bytes[1] == 'Z') {
            if (bytes.len >= 64) {
                const pe_off = ExeInspector.readU32Le(bytes, 60) as usize;
                if (pe_off + 4 <= bytes.len) {
                    if (bytes[pe_off] == 'P' && bytes[pe_off + 1] == 'E' && bytes[pe_off + 2] == 0 && bytes[pe_off + 3] == 0) {
                        return ExeFormat.Pe;
                    }
                    return ExeFormat.Unknown;
                }
            }
            return ExeFormat.Pe;
        }
        return ExeFormat.Unknown;
    }

    pub fn formatName(): []const u8 {
        return switch (_) {
            .Unknown => "unknown",
            .Elf32 => "elf32",
            .Elf64 => "elf64",
            .MachO32 => "macho32",
            .MachO64 => "macho64",
            .MachOFat => "macho_fat",
            .Pe => "pe",
            .Wasm => "wasm",
            .Shebang => "shebang",
        };
    }
}

pub enum ElfClass: u8 {
    None = 0,
    Class32 = 1,
    Class64 = 2,
}

pub enum ElfEndian: u8 {
    None = 0,
    Little = 1,
    Big = 2,
}

pub enum ElfType: u16 {
    None = 0,
    Rel = 1,
    Exec = 2,
    Dyn = 3,
    Core = 4,
}

pub enum ElfMachine: u16 {
    Unknown = 0,
    X86 = 3,
    Ppc64 = 21,
    Arm = 40,
    X86_64 = 62,
    AArch64 = 183,
    RiscV = 243,
}

pub struct Elf64HeaderInfo {
    is_64bit: bool = false;
    is_little_endian: bool = false;
    elf_type: u16 = 0;
    machine: u16 = 0;
    entry_point: u64 = 0;
    phoff: u64 = 0;
    shoff: u64 = 0;
    phnum: u16 = 0;
    shnum: u16 = 0;
    shstrndx: u16 = 0;
    ei_class: u8 = 0;
    ei_data: u8 = 0;
    ei_osabi: u8 = 0;
    flags: u32 = 0;
    ehsize: u16 = 64;
    phentsize: u16 = 56;
    shentsize: u16 = 64;

    pub fn typeKind(): ElfType {
        return switch (_.elf_type) {
            1 => ElfType.Rel,
            2 => ElfType.Exec,
            3 => ElfType.Dyn,
            4 => ElfType.Core,
            else => ElfType.None,
        };
    }

    pub fn machineKind(): ElfMachine {
        return switch (_.machine) {
            3 => ElfMachine.X86,
            21 => ElfMachine.Ppc64,
            40 => ElfMachine.Arm,
            62 => ElfMachine.X86_64,
            183 => ElfMachine.AArch64,
            243 => ElfMachine.RiscV,
            else => ElfMachine.Unknown,
        };
    }
}

pub struct ExeInspector {
    /// Detects executable binary format from magic bytes.
    pub static fn detectFormat(bytes: []const u8): ExeFormatKind {
        if (bytes.len >= 2 && bytes[0] == '#' && bytes[1] == '!') {
            return ExeFormatKind.Shebang;
        }
        if (bytes.len >= 4) {
            // ELF: \x7F 'E' 'L' 'F'
            if (bytes[0] == 0x7F && bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F') {
                return ExeFormatKind.Elf;
            }
            // Mach-O 64-bit little-endian (0xCFFAEDFE) or big-endian (0xFEEDFACF)
            if ((bytes[0] == 0xCF && bytes[1] == 0xFA && bytes[2] == 0xED && bytes[3] == 0xFE) ||
                (bytes[0] == 0xFE && bytes[1] == 0xED && bytes[2] == 0xFA && bytes[3] == 0xCF))
            {
                return ExeFormatKind.MachO64;
            }
            // Mach-O 32-bit
            if ((bytes[0] == 0xCE && bytes[1] == 0xFA && bytes[2] == 0xED && bytes[3] == 0xFE) ||
                (bytes[0] == 0xFE && bytes[1] == 0xED && bytes[2] == 0xFA && bytes[3] == 0xCE))
            {
                return ExeFormatKind.MachO32;
            }
            // Mach-O Universal Fat binary (0xCAFEBABE)
            if (bytes[0] == 0xCA && bytes[1] == 0xFE && bytes[2] == 0xBA && bytes[3] == 0xBE) {
                return ExeFormatKind.MachOFat;
            }
            // WebAssembly: \x00 'a' 's' 'm'
            if (bytes[0] == 0x00 && bytes[1] == 'a' && bytes[2] == 's' && bytes[3] == 'm') {
                return ExeFormatKind.Wasm;
            }
        }
        if (bytes.len >= 2 && bytes[0] == 'M' && bytes[1] == 'Z') {
            return ExeFormatKind.Pe;
        }
        return ExeFormatKind.Unknown;
    }

    pub static fn readU16Le(buf: []const u8, offset: usize): u16 {
        return (buf[offset] as u16) | ((buf[offset + 1] as u16) << 8);
    }

    pub static fn readU32Le(buf: []const u8, offset: usize): u32 {
        return (buf[offset] as u32) |
               ((buf[offset + 1] as u32) << 8) |
               ((buf[offset + 2] as u32) << 16) |
               ((buf[offset + 3] as u32) << 24);
    }

    pub static fn readU64Le(buf: []const u8, offset: usize): u64 {
        const lo = ExeInspector.readU32Le(buf, offset) as u64;
        const hi = ExeInspector.readU32Le(buf, offset + 4) as u64;
        return lo | (hi << 32);
    }

    pub static fn readU16Be(buf: []const u8, offset: usize): u16 {
        return ((buf[offset] as u16) << 8) | (buf[offset + 1] as u16);
    }

    pub static fn readU32Be(buf: []const u8, offset: usize): u32 {
        return ((buf[offset] as u32) << 24) |
               ((buf[offset + 1] as u32) << 16) |
               ((buf[offset + 2] as u32) << 8) |
               (buf[offset + 3] as u32);
    }

    pub static fn readU64Be(buf: []const u8, offset: usize): u64 {
        const hi = ExeInspector.readU32Be(buf, offset) as u64;
        const lo = ExeInspector.readU32Be(buf, offset + 4) as u64;
        return (hi << 32) | lo;
    }

    pub static fn readU16(buf: []const u8, offset: usize, little_endian: bool): u16 {
        return if (little_endian) ExeInspector.readU16Le(buf, offset) else ExeInspector.readU16Be(buf, offset);
    }

    pub static fn readU32(buf: []const u8, offset: usize, little_endian: bool): u32 {
        return if (little_endian) ExeInspector.readU32Le(buf, offset) else ExeInspector.readU32Be(buf, offset);
    }

    pub static fn readU64(buf: []const u8, offset: usize, little_endian: bool): u64 {
        return if (little_endian) ExeInspector.readU64Le(buf, offset) else ExeInspector.readU64Be(buf, offset);
    }

    pub static fn readI64(buf: []const u8, offset: usize, little_endian: bool): i64 {
        const u = ExeInspector.readU64(buf, offset, little_endian);
        return @bitCast(u);
    }

    /// Parses a 64-byte ELF64 header (`Elf64_Ehdr`) from `bytes` (little or big endian).
    pub static fn parseElf64Header(bytes: []const u8): Elf64HeaderInfo? {
        if (bytes.len < 64) {
            return null;
        }
        if (ExeInspector.detectFormat(bytes) != ExeFormatKind.Elf) {
            return null;
        }
        const ei_class = bytes[4];
        const ei_data = bytes[5];
        if (ei_class != 2 || (ei_data != 1 && ei_data != 2)) {
            return null;
        }
        const le = (ei_data == 1);

        return Elf64HeaderInfo {
            is_64bit: true,
            is_little_endian: le,
            elf_type: ExeInspector.readU16(bytes, 16, le),
            machine: ExeInspector.readU16(bytes, 18, le),
            entry_point: ExeInspector.readU64(bytes, 24, le),
            phoff: ExeInspector.readU64(bytes, 32, le),
            shoff: ExeInspector.readU64(bytes, 40, le),
            phnum: ExeInspector.readU16(bytes, 56, le),
            shnum: ExeInspector.readU16(bytes, 60, le),
            shstrndx: ExeInspector.readU16(bytes, 62, le),
            ei_class: ei_class,
            ei_data: ei_data,
            ei_osabi: bytes[7],
            flags: ExeInspector.readU32(bytes, 48, le),
            ehsize: ExeInspector.readU16(bytes, 52, le),
            phentsize: ExeInspector.readU16(bytes, 54, le),
            shentsize: ExeInspector.readU16(bytes, 58, le),
        };
    }

    /// Validates a PE/COFF executable DOS stub (`MZ`) and returns the `PE\0\0` signature offset (`e_lfanew`).
    pub static fn parsePeSignatureOffset(bytes: []const u8): u32? {
        if (bytes.len < 64 || bytes[0] != 'M' || bytes[1] != 'Z') {
            return null;
        }
        const pe_off = ExeInspector.readU32Le(bytes, 60);
        const off = pe_off as usize;
        if (off + 4 <= bytes.len) {
            if (bytes[off] == 'P' && bytes[off + 1] == 'E' && bytes[off + 2] == 0 && bytes[off + 3] == 0) {
                return pe_off;
            }
        }
        return null;
    }
}
