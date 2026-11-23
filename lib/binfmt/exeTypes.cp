// Source: cp/lib/exe_format/types.cp (Native CP executable format detection & shared types)
import std from "std";

pub enum ExeKind {
    Elf64,
    Elf32,
    MachO64,
    MachOFat,
    Pe32Plus,
    Pe32,
    Wasm,
    Shebang,
    Unknown,
}

pub enum ExeArch {
    Unknown,
    X86_64,
    X86,
    Aarch64,
    Arm,
    Riscv64,
    Wasm32,
    Universal,
}

pub fn readU16Le(buf: []const u8, off: usize): u16! {
    if (off + 2 > buf.len) {
        return err.OutOfBounds;
    }
    const b0: u16 = buf[off];
    const b1: u16 = buf[off + 1];
    return b0 | (b1 << 8);
}

pub fn readU32Le(buf: []const u8, off: usize): u32! {
    if (off + 4 > buf.len) {
        return err.OutOfBounds;
    }
    const b0: u32 = buf[off];
    const b1: u32 = buf[off + 1];
    const b2: u32 = buf[off + 2];
    const b3: u32 = buf[off + 3];
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

pub fn readU64Le(buf: []const u8, off: usize): u64! {
    if (off + 8 > buf.len) {
        return err.OutOfBounds;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        const b: u64 = buf[off + i];
        val |= b << ((i * 8) as u6);
    }
    return val;
}

pub fn writeU64Le(buf: []u8, off: usize, val: u64): void! {
    if (off + 8 > buf.len) {
        return err.OutOfBounds;
    }
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        buf[off + i] = ((val >> ((i * 8) as u6)) & 0xFF) as u8;
    }
}

pub fn archFromElfMachine(e_machine: u16): ExeArch {
    return switch (e_machine) {
        3 => ExeArch.X86,
        40 => ExeArch.Arm,
        62 => ExeArch.X86_64,
        183 => ExeArch.Aarch64,
        243 => ExeArch.Riscv64,
        else => ExeArch.Unknown,
    };
}

pub fn archFromPeMachine(machine: u16): ExeArch {
    return switch (machine) {
        0x014C => ExeArch.X86,
        0x01C0, 0x01C4 => ExeArch.Arm,
        0x8664 => ExeArch.X86_64,
        0xAA64 => ExeArch.Aarch64,
        0x5064 => ExeArch.Riscv64,
        else => ExeArch.Unknown,
    };
}

pub fn archFromMachOCpu(cputype: u32): ExeArch {
    return switch (cputype) {
        7 => ExeArch.X86,
        0x01000007 => ExeArch.X86_64,
        12 => ExeArch.Arm,
        0x0100000C => ExeArch.Aarch64,
        else => ExeArch.Unknown,
    };
}

pub fn detectExeKind(bytes: []const u8): ExeKind {
    if (bytes.len >= 2 && bytes[0] == '#' && bytes[1] == '!') {
        return ExeKind.Shebang;
    }
    if (bytes.len < 4) {
        return ExeKind.Unknown;
    }

    // ELF: \x7FELF
    if (bytes[0] == 0x7F && bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F') {
        if (bytes.len < 5) {
            return ExeKind.Unknown;
        }
        if (bytes[4] == 2) {
            return ExeKind.Elf64;
        }
        if (bytes[4] == 1) {
            return ExeKind.Elf32;
        }
        return ExeKind.Unknown;
    }

    // WebAssembly: \0asm
    if (bytes[0] == 0x00 && bytes[1] == 'a' && bytes[2] == 's' && bytes[3] == 'm') {
        return ExeKind.Wasm;
    }

    const magic32 = readU32Le(bytes, 0) catch 0;
    // Mach-O 64-bit LE (0xFEEDFACF) or BE (0xCFFAEDFE)
    if (magic32 == 0xFEEDFACF || magic32 == 0xCFFAEDFE) {
        return ExeKind.MachO64;
    }
    // Mach-O Universal Fat binary (0xCAFEBABE BE -> 0xBEBAFECA LE)
    if (magic32 == 0xBEBAFECA || magic32 == 0xCAFEBABE) {
        return ExeKind.MachOFat;
    }

    // PE-COFF: MZ header + e_lfanew -> PE\0\0
    if (bytes[0] == 'M' && bytes[1] == 'Z') {
        if (bytes.len < 64) {
            return ExeKind.Unknown;
        }
        const pe_off_u32 = readU32Le(bytes, 60) catch return ExeKind.Unknown;
        const pe_off: usize = pe_off_u32 as usize;
        if (pe_off > bytes.len || bytes.len - pe_off < 26) {
            return ExeKind.Unknown;
        }
        const pe_sig = readU32Le(bytes, pe_off) catch return ExeKind.Unknown;
        if (pe_sig != 0x00004550) {
            return ExeKind.Unknown;
        }
        const opt_magic = readU16Le(bytes, pe_off + 24) catch return ExeKind.Unknown;
        if (opt_magic == 0x020B) {
            return ExeKind.Pe32Plus;
        }
        if (opt_magic == 0x010B) {
            return ExeKind.Pe32;
        }
        return ExeKind.Unknown;
    }

    return ExeKind.Unknown;
}
