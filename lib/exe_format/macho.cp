// Source: bun/src/exe_format/macho.cp -> cp/lib/exe_format/macho.cp (Zero-copy Mach-O 64 header parser)
import std from "std";
import { ExeArch, archFromMachOCpu, readU32Le } from "./types.cp";

pub const MH_MAGIC_64: u32 = 0xFEEDFACF;
pub const MACH_HEADER_64_SIZE: usize = 32;

pub struct MachO64HeaderInfo {
    magic: u32,
    cputype: u32,
    cpusubtype: u32,
    filetype: u32,
    ncmds: u32,
    sizeofcmds: u32,
    flags: u32,
    arch: ExeArch,
}

pub fn parseMachO64Header(bytes: []const u8): MachO64HeaderInfo! {
    if (bytes.len < MACH_HEADER_64_SIZE) {
        return err.TruncatedHeader;
    }

    const magic = readU32Le(bytes, 0)?;
    if (magic != MH_MAGIC_64) {
        return err.InvalidMagic;
    }

    const cputype = readU32Le(bytes, 4)?;
    const cpusubtype = readU32Le(bytes, 8)?;
    const filetype = readU32Le(bytes, 12)?;
    const ncmds = readU32Le(bytes, 16)?;
    const sizeofcmds = readU32Le(bytes, 20)?;
    const flags = readU32Le(bytes, 24)?;

    if ((sizeofcmds as usize) > bytes.len - MACH_HEADER_64_SIZE) {
        return err.OutOfBounds;
    }
    // Each load command is at least 8 bytes (cmd: u32, cmdsize: u32)
    if ((ncmds as u64) * 8 > (sizeofcmds as u64)) {
        return err.InvalidLoadCommands;
    }

    return MachO64HeaderInfo{
        magic: magic,
        cputype: cputype,
        cpusubtype: cpusubtype,
        filetype: filetype,
        ncmds: ncmds,
        sizeofcmds: sizeofcmds,
        flags: flags,
        arch: archFromMachOCpu(cputype),
    };
}
