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
}

pub enum ElfMachine: u16 {
    Unknown = 0,
    X86 = 3,
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
}

pub struct ExeInspector {
    /// Detects executable binary format from the first 4 magic bytes.
    pub static fn detectFormat(bytes: []const u8): ExeFormatKind {
        if (bytes.len >= 4) {
            // ELF: \x7F 'E' 'L' 'F'
            if (bytes[0] == 0x7F && bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F') {
                return ExeFormatKind.Elf;
            }
            // Mach-O 64-bit little-endian (0xCFFAEDFE) or big-endian (0xFEFACF)
            if ((bytes[0] == 0xCF && bytes[1] == 0xFA && bytes[2] == 0xED && bytes[3] == 0xFE) ||
                (bytes[0] == 0xFE && bytes[1] == 0xED && bytes[2] == 0xFA && bytes[3] == 0xCF))
            {
                return ExeFormatKind.MachO64;
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

    /// Parses a 64-byte ELF64 header (`Elf64_Ehdr`) from `bytes`.
    pub static fn parseElf64Header(bytes: []const u8): Elf64HeaderInfo? {
        if (bytes.len < 64) {
            return null;
        }
        if (ExeInspector.detectFormat(bytes) != ExeFormatKind.Elf) {
            return null;
        }
        const ei_class = bytes[4];
        const ei_data = bytes[5];
        if (ei_class != 2 || ei_data != 1) {
            return null;
        }

        return Elf64HeaderInfo {
            is_64bit: true,
            is_little_endian: true,
            elf_type: ExeInspector.readU16Le(bytes, 16),
            machine: ExeInspector.readU16Le(bytes, 18),
            entry_point: ExeInspector.readU64Le(bytes, 24),
            phoff: ExeInspector.readU64Le(bytes, 32),
            shoff: ExeInspector.readU64Le(bytes, 40),
            phnum: ExeInspector.readU16Le(bytes, 56),
            shnum: ExeInspector.readU16Le(bytes, 60),
            shstrndx: ExeInspector.readU16Le(bytes, 62),
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
