// lib/macho/index.cp — macOS/iOS Mach-O 32/64 & Universal Fat binary header parser (cap:macho)
// Reuses Zig 0.17 std.macho constants; zero heap allocation, strict bounds checks.

const std = @import("std");

pub err MachoError {
    Truncated,
    InvalidMagic,
    ArchOutOfBounds,
}

pub const MH_MAGIC: u32 = std.macho.MH_MAGIC;
pub const MH_CIGAM: u32 = std.macho.MH_CIGAM;
pub const MH_MAGIC_64: u32 = std.macho.MH_MAGIC_64;
pub const MH_CIGAM_64: u32 = std.macho.MH_CIGAM_64;
pub const FAT_MAGIC: u32 = std.macho.FAT_MAGIC;
pub const FAT_CIGAM: u32 = std.macho.FAT_CIGAM;
pub const FAT_MAGIC_64: u32 = std.macho.FAT_MAGIC_64;
pub const FAT_CIGAM_64: u32 = std.macho.FAT_CIGAM_64;

pub const CPU_TYPE_X86: u32 = 0x00000007;
pub const CPU_TYPE_X86_64: u32 = @as(u32, @intCast(std.macho.CPU_TYPE_X86_64));
pub const CPU_TYPE_ARM: u32 = 0x0000000C;
pub const CPU_TYPE_ARM64: u32 = @as(u32, @intCast(std.macho.CPU_TYPE_ARM64));

pub const MH_OBJECT: u32 = std.macho.MH_OBJECT;
pub const MH_EXECUTE: u32 = std.macho.MH_EXECUTE;
pub const MH_DYLIB: u32 = std.macho.MH_DYLIB;
pub const MH_BUNDLE: u32 = 0x8;

pub const MACH_HEADER_32_SIZE: usize = 28;
pub const MACH_HEADER_64_SIZE: usize = 32;
pub const FAT_HEADER_SIZE: usize = 8;
pub const FAT_ARCH_32_SIZE: usize = 20;
pub const FAT_ARCH_64_SIZE: usize = 32;

pub enum MachoKind {
    Mach32,
    Mach64,
    Fat32,
    Fat64,
}

fn machoReadU32Le(bytes: []const u8, off: usize): u32 {
    const b0: u32 = bytes[off];
    const b1: u32 = bytes[off + 1];
    const b2: u32 = bytes[off + 2];
    const b3: u32 = bytes[off + 3];
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn machoReadU32Be(bytes: []const u8, off: usize): u32 {
    const b0: u32 = bytes[off];
    const b1: u32 = bytes[off + 1];
    const b2: u32 = bytes[off + 2];
    const b3: u32 = bytes[off + 3];
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn machoReadU64Le(bytes: []const u8, off: usize): u64 {
    const lo: u64 = machoReadU32Le(bytes, off);
    const hi: u64 = machoReadU32Le(bytes, off + 4);
    return lo | (hi << 32);
}

fn machoReadU64Be(bytes: []const u8, off: usize): u64 {
    const hi: u64 = machoReadU32Be(bytes, off);
    const lo: u64 = machoReadU32Be(bytes, off + 4);
    return (hi << 32) | lo;
}

fn machoReadU32(bytes: []const u8, off: usize, big_endian: bool): u32 {
    if (big_endian) {
        return machoReadU32Be(bytes, off);
    }
    return machoReadU32Le(bytes, off);
}

fn machoReadU64(bytes: []const u8, off: usize, big_endian: bool): u64 {
    if (big_endian) {
        return machoReadU64Be(bytes, off);
    }
    return machoReadU64Le(bytes, off);
}

pub struct FatArchInfo {
    pub cpu_type: u32 = 0;
    pub cpu_subtype: u32 = 0;
    pub offset: u64 = 0;
    pub size: u64 = 0;
    pub alignment: u32 = 0;
}

pub struct MachoInfo {
    pub kind: MachoKind = MachoKind.Mach64;
    pub magic: u32 = 0;
    pub is_big_endian: bool = false;
    pub is_64bit: bool = false;
    pub is_fat: bool = false;
    pub cpu_type: u32 = 0;
    pub cpu_subtype: u32 = 0;
    pub file_type: u32 = 0;
    pub ncmds: u32 = 0;
    pub sizeofcmds: u32 = 0;
    pub flags: u32 = 0;
    pub reserved: u32 = 0;
    pub nfat_arch: u32 = 0;

    pub fn parseFatArch(bytes: []const u8, index: usize): FatArchInfo! {
        if (!_.is_fat || index >= (_.nfat_arch as usize)) {
            return error.ArchOutOfBounds;
        }
        const be = _.is_big_endian;
        if (_.is_64bit) {
            const off = FAT_HEADER_SIZE + index * FAT_ARCH_64_SIZE;
            if (off > bytes.len || bytes.len - off < FAT_ARCH_64_SIZE) {
                return error.Truncated;
            }
            return FatArchInfo{
                cpu_type: machoReadU32(bytes, off, be),
                cpu_subtype: machoReadU32(bytes, off + 4, be),
                offset: machoReadU64(bytes, off + 8, be),
                size: machoReadU64(bytes, off + 16, be),
                alignment: machoReadU32(bytes, off + 24, be),
            };
        } else {
            const off = FAT_HEADER_SIZE + index * FAT_ARCH_32_SIZE;
            if (off > bytes.len || bytes.len - off < FAT_ARCH_32_SIZE) {
                return error.Truncated;
            }
            return FatArchInfo{
                cpu_type: machoReadU32(bytes, off, be),
                cpu_subtype: machoReadU32(bytes, off + 4, be),
                offset: machoReadU32(bytes, off + 8, be) as u64,
                size: machoReadU32(bytes, off + 12, be) as u64,
                alignment: machoReadU32(bytes, off + 16, be),
            };
        }
    }
}

pub fn parseMachoHeader(bytes: []const u8): MachoInfo! {
    if (bytes.len < 4) {
        return error.Truncated;
    }
    const magic_le = machoReadU32Le(bytes, 0);
    const magic_be = machoReadU32Be(bytes, 0);

    // Thin Mach-O 64-bit
    if (magic_le == MH_MAGIC_64 || magic_be == MH_MAGIC_64) {
        if (bytes.len < MACH_HEADER_64_SIZE) {
            return error.Truncated;
        }
        const be = magic_be == MH_MAGIC_64;
        return MachoInfo{
            kind: MachoKind.Mach64,
            magic: MH_MAGIC_64,
            is_big_endian: be,
            is_64bit: true,
            is_fat: false,
            cpu_type: machoReadU32(bytes, 4, be),
            cpu_subtype: machoReadU32(bytes, 8, be),
            file_type: machoReadU32(bytes, 12, be),
            ncmds: machoReadU32(bytes, 16, be),
            sizeofcmds: machoReadU32(bytes, 20, be),
            flags: machoReadU32(bytes, 24, be),
            reserved: machoReadU32(bytes, 28, be),
        };
    }

    // Thin Mach-O 32-bit
    if (magic_le == MH_MAGIC || magic_be == MH_MAGIC) {
        if (bytes.len < MACH_HEADER_32_SIZE) {
            return error.Truncated;
        }
        const be = magic_be == MH_MAGIC;
        return MachoInfo{
            kind: MachoKind.Mach32,
            magic: MH_MAGIC,
            is_big_endian: be,
            is_64bit: false,
            is_fat: false,
            cpu_type: machoReadU32(bytes, 4, be),
            cpu_subtype: machoReadU32(bytes, 8, be),
            file_type: machoReadU32(bytes, 12, be),
            ncmds: machoReadU32(bytes, 16, be),
            sizeofcmds: machoReadU32(bytes, 20, be),
            flags: machoReadU32(bytes, 24, be),
        };
    }

    // Universal Fat 32-bit (standard big-endian 0xCAFEBABE or swapped)
    if (magic_be == FAT_MAGIC || magic_le == FAT_MAGIC) {
        if (bytes.len < FAT_HEADER_SIZE) {
            return error.Truncated;
        }
        const be = magic_be == FAT_MAGIC;
        const nfat = machoReadU32(bytes, 4, be);
        return MachoInfo{
            kind: MachoKind.Fat32,
            magic: FAT_MAGIC,
            is_big_endian: be,
            is_64bit: false,
            is_fat: true,
            nfat_arch: nfat,
        };
    }

    // Universal Fat 64-bit (0xCAFEBABF)
    if (magic_be == FAT_MAGIC_64 || magic_le == FAT_MAGIC_64) {
        if (bytes.len < FAT_HEADER_SIZE) {
            return error.Truncated;
        }
        const be = magic_be == FAT_MAGIC_64;
        const nfat = machoReadU32(bytes, 4, be);
        return MachoInfo{
            kind: MachoKind.Fat64,
            magic: FAT_MAGIC_64,
            is_big_endian: be,
            is_64bit: true,
            is_fat: true,
            nfat_arch: nfat,
        };
    }

    return error.InvalidMagic;
}

pub fn isMacho(bytes: []const u8): bool {
    const res = parseMachoHeader(bytes) catch return false;
    return res.magic != 0;
}

pub fn cpuTypeName(cpu_type: u32): []const u8 {
    if (cpu_type == CPU_TYPE_ARM64) { return "ARM64"; }
    if (cpu_type == CPU_TYPE_X86_64) { return "X86_64"; }
    if (cpu_type == CPU_TYPE_ARM) { return "ARM"; }
    if (cpu_type == CPU_TYPE_X86) { return "X86"; }
    return "UNKNOWN";
}

pub fn fileTypeName(file_type: u32): []const u8 {
    if (file_type == MH_OBJECT) { return "OBJECT"; }
    if (file_type == MH_EXECUTE) { return "EXECUTE"; }
    if (file_type == MH_DYLIB) { return "DYLIB"; }
    if (file_type == MH_BUNDLE) { return "BUNDLE"; }
    return "UNKNOWN";
}
