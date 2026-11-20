// Source: bun/src/exe_format/elf.cp -> cp/lib/exe_format/elf.cp (Zero-copy ELF64 header & section parser)
import std from "std";
import { ExeArch, archFromElfMachine, readU16Le, readU32Le, readU64Le } from "./types.cp";

pub const ELF64_EHDR_SIZE: usize = 64;
pub const ELF64_SHDR_SIZE: usize = 64;

pub struct Elf64Header {
    e_type: u16,
    e_machine: u16,
    e_version: u32,
    e_entry: u64,
    e_phoff: u64,
    e_shoff: u64,
    e_flags: u32,
    e_ehsize: u16,
    e_phentsize: u16,
    e_phnum: u16,
    e_shentsize: u16,
    e_shnum: u16,
    e_shstrndx: u16,
    arch: ExeArch,
}

pub struct Elf64Section {
    name: []const u8,
    sh_type: u32,
    flags: u64,
    addr: u64,
    offset: u64,
    size: u64,
    data: []const u8,
}

pub fn parseElf64Header(bytes: []const u8): Elf64Header! {
    if (bytes.len < ELF64_EHDR_SIZE) {
        return err.TruncatedHeader;
    }
    if (bytes[0] != 0x7F || bytes[1] != 'E' || bytes[2] != 'L' || bytes[3] != 'F') {
        return err.InvalidMagic;
    }
    if (bytes[4] != 2) {
        return err.UnsupportedClass;
    }
    if (bytes[5] != 1) {
        return err.UnsupportedEndianness;
    }

    const e_type = readU16Le(bytes, 16)?;
    const e_machine = readU16Le(bytes, 18)?;
    const e_version = readU32Le(bytes, 20)?;
    const e_entry = readU64Le(bytes, 24)?;
    const e_phoff = readU64Le(bytes, 32)?;
    const e_shoff = readU64Le(bytes, 40)?;
    const e_flags = readU32Le(bytes, 48)?;
    const e_ehsize = readU16Le(bytes, 52)?;
    const e_phentsize = readU16Le(bytes, 54)?;
    const e_phnum = readU16Le(bytes, 56)?;
    const e_shentsize = readU16Le(bytes, 58)?;
    const e_shnum = readU16Le(bytes, 60)?;
    const e_shstrndx = readU16Le(bytes, 62)?;

    if (e_shnum > 0) {
        if (e_shentsize < ELF64_SHDR_SIZE) {
            return err.InvalidSectionTable;
        }
        if (e_shoff > (bytes.len as u64)) {
            return err.OutOfBounds;
        }
        const total_sh_bytes = (e_shentsize as u64) * (e_shnum as u64);
        if (total_sh_bytes > (bytes.len as u64) - e_shoff) {
            return err.OutOfBounds;
        }
    }

    return Elf64Header{
        e_type: e_type,
        e_machine: e_machine,
        e_version: e_version,
        e_entry: e_entry,
        e_phoff: e_phoff,
        e_shoff: e_shoff,
        e_flags: e_flags,
        e_ehsize: e_ehsize,
        e_phentsize: e_phentsize,
        e_phnum: e_phnum,
        e_shentsize: e_shentsize,
        e_shnum: e_shnum,
        e_shstrndx: e_shstrndx,
        arch: archFromElfMachine(e_machine),
    };
}

fn readSectionString(strtab: []const u8, name_off: u32): []const u8 {
    const off: usize = name_off as usize;
    if (off >= strtab.len) {
        return "";
    }
    let end: usize = off;
    while (end < strtab.len && strtab[end] != 0) : (end += 1) {}
    return strtab[off..end];
}

fn resolveShstrtab(bytes: []const u8, hdr: Elf64Header): []const u8! {
    if (hdr.e_shnum == 0 || hdr.e_shstrndx == 0xFFFF || hdr.e_shstrndx >= hdr.e_shnum) {
        return "";
    }
    const shstr_off = (hdr.e_shoff as usize) + (hdr.e_shstrndx as usize) * (hdr.e_shentsize as usize);
    const str_offset = readU64Le(bytes, shstr_off + 24)?;
    const str_size = readU64Le(bytes, shstr_off + 32)?;
    if (str_offset > (bytes.len as u64) || str_size > (bytes.len as u64) - str_offset) {
        return err.OutOfBounds;
    }
    const start: usize = str_offset as usize;
    const len: usize = str_size as usize;
    return bytes[start .. start + len];
}

fn parseSectionAt(bytes: []const u8, hdr: Elf64Header, strtab: []const u8, idx: usize): Elf64Section! {
    const off = (hdr.e_shoff as usize) + idx * (hdr.e_shentsize as usize);
    const sh_name = readU32Le(bytes, off)?;
    const sh_type = readU32Le(bytes, off + 4)?;
    const sh_flags = readU64Le(bytes, off + 8)?;
    const sh_addr = readU64Le(bytes, off + 16)?;
    const sh_offset = readU64Le(bytes, off + 24)?;
    const sh_size = readU64Le(bytes, off + 32)?;

    let sec_data: []const u8 = "";
    // SHT_NOBITS == 8 has no file backing
    if (sh_type != 8 && sh_size > 0) {
        if (sh_offset > (bytes.len as u64) || sh_size > (bytes.len as u64) - sh_offset) {
            return err.OutOfBounds;
        }
        const start: usize = sh_offset as usize;
        const len: usize = sh_size as usize;
        sec_data = bytes[start .. start + len];
    }

    return Elf64Section{
        name: readSectionString(strtab, sh_name),
        sh_type: sh_type,
        flags: sh_flags,
        addr: sh_addr,
        offset: sh_offset,
        size: sh_size,
        data: sec_data,
    };
}

pub fn listElf64Sections(alloc: std.mem.Allocator, bytes: []const u8): []Elf64Section! {
    const hdr = parseElf64Header(bytes)?;
    const strtab = resolveShstrtab(bytes, hdr)?;

    let list = std.ArrayList(Elf64Section).empty;
    errdefer list.deinit(alloc);

    let i: usize = 0;
    while (i < (hdr.e_shnum as usize)) : (i += 1) {
        const sec = parseSectionAt(bytes, hdr, strtab, i)?;
        list.append(alloc, sec)?;
    }

    return list.toOwnedSlice(alloc);
}

pub fn findElf64Section(bytes: []const u8, name: []const u8): Elf64Section?! {
    const hdr = parseElf64Header(bytes)?;
    const strtab = resolveShstrtab(bytes, hdr)?;

    let i: usize = 0;
    while (i < (hdr.e_shnum as usize)) : (i += 1) {
        const sec = parseSectionAt(bytes, hdr, strtab, i)?;
        if (std.mem.eql(u8, sec.name, name)) {
            return sec;
        }
    }
    return null;
}
