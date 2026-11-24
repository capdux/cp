// Source: Absorbed from /data/dev/bun/src/exe_format/elf.cp | Synced: 2026-11-23T21:43:11Z
// lib/elf/elf64.cp
// Full ELF64 Program Header, Section Header, Symbol Table & Dynamic Section Parser (cap:elf)
// Absorbed from Bun src/exe_format/elf.cp with pure CP semantics

const std = @import("std");
import { Elf64HeaderInfo, ExeInspector } from "./format.cp";

// Program Header Types (p_type)
pub const PT_NULL: u32 = 0;
pub const PT_LOAD: u32 = 1;
pub const PT_DYNAMIC: u32 = 2;
pub const PT_INTERP: u32 = 3;
pub const PT_NOTE: u32 = 4;
pub const PT_SHLIB: u32 = 5;
pub const PT_PHDR: u32 = 6;
pub const PT_TLS: u32 = 7;
pub const PT_GNU_EH_FRAME: u32 = 0x6474e550;
pub const PT_GNU_STACK: u32 = 0x6474e551;
pub const PT_GNU_RELRO: u32 = 0x6474e552;

// Program Header Flags (p_flags)
pub const PF_X: u32 = 1;
pub const PF_W: u32 = 2;
pub const PF_R: u32 = 4;

// Section Header Types (sh_type)
pub const SHT_NULL: u32 = 0;
pub const SHT_PROGBITS: u32 = 1;
pub const SHT_SYMTAB: u32 = 2;
pub const SHT_STRTAB: u32 = 3;
pub const SHT_RELA: u32 = 4;
pub const SHT_HASH: u32 = 5;
pub const SHT_DYNAMIC: u32 = 6;
pub const SHT_NOTE: u32 = 7;
pub const SHT_NOBITS: u32 = 8;
pub const SHT_REL: u32 = 9;
pub const SHT_DYNSYM: u32 = 11;
pub const SHT_INIT_ARRAY: u32 = 14;
pub const SHT_FINI_ARRAY: u32 = 15;
pub const SHT_GNU_HASH: u32 = 0x6ffffff6;

// Section Header Flags (sh_flags)
pub const SHF_WRITE: u64 = 1;
pub const SHF_ALLOC: u64 = 2;
pub const SHF_EXECINSTR: u64 = 4;

// Symbol Binding (st_info >> 4)
pub const STB_LOCAL: u8 = 0;
pub const STB_GLOBAL: u8 = 1;
pub const STB_WEAK: u8 = 2;

// Symbol Type (st_info & 0xf)
pub const STT_NOTYPE: u8 = 0;
pub const STT_OBJECT: u8 = 1;
pub const STT_FUNC: u8 = 2;
pub const STT_SECTION: u8 = 3;
pub const STT_FILE: u8 = 4;
pub const STT_TLS: u8 = 6;

// Symbol Visibility (st_other & 0x3)
pub const STV_DEFAULT: u8 = 0;
pub const STV_INTERNAL: u8 = 1;
pub const STV_HIDDEN: u8 = 2;
pub const STV_PROTECTED: u8 = 3;

// Dynamic Tags (d_tag)
pub const DT_NULL: i64 = 0;
pub const DT_NEEDED: i64 = 1;
pub const DT_PLTRELSZ: i64 = 2;
pub const DT_PLTGOT: i64 = 3;
pub const DT_HASH: i64 = 4;
pub const DT_STRTAB: i64 = 5;
pub const DT_SYMTAB: i64 = 6;
pub const DT_RELA: i64 = 7;
pub const DT_STRSZ: i64 = 10;
pub const DT_SYMENT: i64 = 11;
pub const DT_INIT: i64 = 12;
pub const DT_FINI: i64 = 13;
pub const DT_SONAME: i64 = 14;
pub const DT_RPATH: i64 = 15;
pub const DT_SYMBOLIC: i64 = 16;
pub const DT_REL: i64 = 17;
pub const DT_JMPREL: i64 = 23;
pub const DT_RUNPATH: i64 = 29;
pub const DT_FLAGS: i64 = 30;
pub const DT_FLAGS_1: i64 = 0x6ffffffb;

pub struct Elf64Phdr {
    pub p_type: u32 = 0,
    pub p_flags: u32 = 0,
    pub p_offset: u64 = 0,
    pub p_vaddr: u64 = 0,
    pub p_paddr: u64 = 0,
    pub p_filesz: u64 = 0,
    pub p_memsz: u64 = 0,
    pub p_align: u64 = 0,

    pub fn isReadable(): bool {
        return (_.p_flags & PF_R) != 0;
    }

    pub fn isWritable(): bool {
        return (_.p_flags & PF_W) != 0;
    }

    pub fn isExecutable(): bool {
        return (_.p_flags & PF_X) != 0;
    }

    pub fn typeName(): []const u8 {
        return switch (_.p_type) {
            0 => "PT_NULL",
            1 => "PT_LOAD",
            2 => "PT_DYNAMIC",
            3 => "PT_INTERP",
            4 => "PT_NOTE",
            5 => "PT_SHLIB",
            6 => "PT_PHDR",
            7 => "PT_TLS",
            0x6474e550 => "PT_GNU_EH_FRAME",
            0x6474e551 => "PT_GNU_STACK",
            0x6474e552 => "PT_GNU_RELRO",
            else => "PT_UNKNOWN",
        };
    }
}

pub struct Elf64Shdr {
    pub sh_name: u32 = 0,
    pub sh_type: u32 = 0,
    pub sh_flags: u64 = 0,
    pub sh_addr: u64 = 0,
    pub sh_offset: u64 = 0,
    pub sh_size: u64 = 0,
    pub sh_link: u32 = 0,
    pub sh_info: u32 = 0,
    pub sh_addralign: u64 = 0,
    pub sh_entsize: u64 = 0,

    pub fn isWritable(): bool {
        return (_.sh_flags & SHF_WRITE) != 0;
    }

    pub fn isAlloc(): bool {
        return (_.sh_flags & SHF_ALLOC) != 0;
    }

    pub fn isExecutable(): bool {
        return (_.sh_flags & SHF_EXECINSTR) != 0;
    }

    pub fn typeName(): []const u8 {
        return switch (_.sh_type) {
            0 => "SHT_NULL",
            1 => "SHT_PROGBITS",
            2 => "SHT_SYMTAB",
            3 => "SHT_STRTAB",
            4 => "SHT_RELA",
            5 => "SHT_HASH",
            6 => "SHT_DYNAMIC",
            7 => "SHT_NOTE",
            8 => "SHT_NOBITS",
            9 => "SHT_REL",
            11 => "SHT_DYNSYM",
            14 => "SHT_INIT_ARRAY",
            15 => "SHT_FINI_ARRAY",
            0x6ffffff6 => "SHT_GNU_HASH",
            else => "SHT_OTHER",
        };
    }
}

pub struct Elf64Sym {
    pub st_name: u32 = 0,
    pub st_info: u8 = 0,
    pub st_other: u8 = 0,
    pub st_shndx: u16 = 0,
    pub st_value: u64 = 0,
    pub st_size: u64 = 0,

    pub fn bind(): u8 {
        return _.st_info >> 4;
    }

    pub fn symType(): u8 {
        return _.st_info & 0x0F;
    }

    pub fn visibility(): u8 {
        return _.st_other & 0x03;
    }

    pub fn bindName(): []const u8 {
        return switch (_.bind()) {
            0 => "LOCAL",
            1 => "GLOBAL",
            2 => "WEAK",
            else => "OTHER",
        };
    }

    pub fn typeName(): []const u8 {
        return switch (_.symType()) {
            0 => "NOTYPE",
            1 => "OBJECT",
            2 => "FUNC",
            3 => "SECTION",
            4 => "FILE",
            6 => "TLS",
            else => "OTHER",
        };
    }

    pub fn visibilityName(): []const u8 {
        return switch (_.visibility()) {
            0 => "DEFAULT",
            1 => "INTERNAL",
            2 => "HIDDEN",
            3 => "PROTECTED",
            else => "DEFAULT",
        };
    }
}

pub struct Elf64Dyn {
    pub d_tag: i64 = 0,
    pub d_val: u64 = 0,

    pub fn tagName(): []const u8 {
        return switch (_.d_tag) {
            0 => "DT_NULL",
            1 => "DT_NEEDED",
            2 => "DT_PLTRELSZ",
            3 => "DT_PLTGOT",
            4 => "DT_HASH",
            5 => "DT_STRTAB",
            6 => "DT_SYMTAB",
            7 => "DT_RELA",
            10 => "DT_STRSZ",
            11 => "DT_SYMENT",
            12 => "DT_INIT",
            13 => "DT_FINI",
            14 => "DT_SONAME",
            15 => "DT_RPATH",
            16 => "DT_SYMBOLIC",
            17 => "DT_REL",
            23 => "DT_JMPREL",
            29 => "DT_RUNPATH",
            30 => "DT_FLAGS",
            0x6ffffffb => "DT_FLAGS_1",
            else => "DT_OTHER",
        };
    }
}

struct ByteSpan {
    off: usize = 0,
    end: usize = 0,
}

pub struct Elf64File {
    pub bytes: []const u8 = "",
    pub ehdr: Elf64HeaderInfo = Elf64HeaderInfo{},

    pub static fn parse(bytes: []const u8): Elf64File? {
        const hdr = ExeInspector.parseElf64Header(bytes) ?? return null;
        return Elf64File{
            bytes: bytes,
            ehdr: hdr,
        };
    }

    pub fn header(): Elf64HeaderInfo {
        return _.ehdr;
    }

    fn cStringEnd(start: usize, limit: usize): usize {
        let pos = start;
        const max_end = if (limit > _.bytes.len) _.bytes.len else limit;
        while (pos < max_end && _.bytes[pos] != 0) {
            pos += 1;
        }
        return pos;
    }

    // --- Program Headers ---
    pub fn phdrCount(): usize {
        return _.ehdr.phnum as usize;
    }

    pub fn getPhdr(index: usize): Elf64Phdr? {
        if (index >= (_.ehdr.phnum as usize)) {
            return null;
        }
        const entsz: usize = if (_.ehdr.phentsize >= 56) (_.ehdr.phentsize as usize) else 56;
        const off = (_.ehdr.phoff as usize) + index * entsz;
        if (off + 56 > _.bytes.len) {
            return null;
        }
        const le = _.ehdr.is_little_endian;
        return Elf64Phdr{
            p_type: ExeInspector.readU32(_.bytes, off, le),
            p_flags: ExeInspector.readU32(_.bytes, off + 4, le),
            p_offset: ExeInspector.readU64(_.bytes, off + 8, le),
            p_vaddr: ExeInspector.readU64(_.bytes, off + 16, le),
            p_paddr: ExeInspector.readU64(_.bytes, off + 24, le),
            p_filesz: ExeInspector.readU64(_.bytes, off + 32, le),
            p_memsz: ExeInspector.readU64(_.bytes, off + 40, le),
            p_align: ExeInspector.readU64(_.bytes, off + 48, le),
        };
    }

    pub fn findPhdr(p_type: u32): Elf64Phdr? {
        let i: usize = 0;
        const n = _.phdrCount();
        while (i < n) {
            if (ph := _.getPhdr(i)) {
                if (ph.p_type == p_type) {
                    return ph;
                }
            }
            i += 1;
        }
        return null;
    }

    pub fn interpreter(): ([]const u8)? {
        const ph = _.findPhdr(PT_INTERP) ?? return null;
        const off = ph.p_offset as usize;
        const sz = ph.p_filesz as usize;
        if (off + sz > _.bytes.len || sz == 0) {
            return null;
        }
        const end = _.cStringEnd(off, off + sz);
        return _.bytes[off..end];
    }

    // --- Section Headers ---
    pub fn shdrCount(): usize {
        if (_.ehdr.shnum == 0 && _.ehdr.shoff > 0) {
            if (sh0 := _.readShdrRaw(0)) {
                return sh0.sh_size as usize;
            }
        }
        return _.ehdr.shnum as usize;
    }

    fn readShdrRaw(index: usize): Elf64Shdr? {
        const entsz: usize = if (_.ehdr.shentsize >= 64) (_.ehdr.shentsize as usize) else 64;
        const off = (_.ehdr.shoff as usize) + index * entsz;
        if (_.ehdr.shoff == 0 || off + 64 > _.bytes.len) {
            return null;
        }
        const le = _.ehdr.is_little_endian;
        return Elf64Shdr{
            sh_name: ExeInspector.readU32(_.bytes, off, le),
            sh_type: ExeInspector.readU32(_.bytes, off + 4, le),
            sh_flags: ExeInspector.readU64(_.bytes, off + 8, le),
            sh_addr: ExeInspector.readU64(_.bytes, off + 16, le),
            sh_offset: ExeInspector.readU64(_.bytes, off + 24, le),
            sh_size: ExeInspector.readU64(_.bytes, off + 32, le),
            sh_link: ExeInspector.readU32(_.bytes, off + 40, le),
            sh_info: ExeInspector.readU32(_.bytes, off + 44, le),
            sh_addralign: ExeInspector.readU64(_.bytes, off + 48, le),
            sh_entsize: ExeInspector.readU64(_.bytes, off + 56, le),
        };
    }

    pub fn getShdr(index: usize): Elf64Shdr? {
        if (index >= _.shdrCount()) {
            return null;
        }
        return _.readShdrRaw(index);
    }

    fn shdrSpan(shdr: Elf64Shdr): ByteSpan? {
        if (shdr.sh_type == SHT_NOBITS) {
            return ByteSpan{ off: 0, end: 0 };
        }
        const off = shdr.sh_offset as usize;
        const sz = shdr.sh_size as usize;
        if (off > _.bytes.len || sz > _.bytes.len - off) {
            return null;
        }
        return ByteSpan{ off: off, end: off + sz };
    }

    pub fn sectionData(shdr: Elf64Shdr): ([]const u8)? {
        if (shdr.sh_type == SHT_NOBITS) {
            return "";
        }
        const span = _.shdrSpan(shdr) ?? return null;
        return _.bytes[span.off..span.end];
    }

    fn shstrtabSpan(): ByteSpan? {
        const str_idx = _.ehdr.shstrndx as usize;
        if (str_idx == 0xFFFF || str_idx >= _.shdrCount()) {
            return null;
        }
        const sec = _.getShdr(str_idx) ?? return null;
        return _.shdrSpan(sec);
    }

    pub fn sectionName(shdr: Elf64Shdr): []const u8 {
        const span = _.shstrtabSpan() ?? return "";
        const name_off = span.off + (shdr.sh_name as usize);
        if (name_off >= span.end) {
            return "";
        }
        const name_end = _.cStringEnd(name_off, span.end);
        return _.bytes[name_off..name_end];
    }

    pub fn findSection(name: []const u8): Elf64Shdr? {
        const span = _.shstrtabSpan() ?? return null;
        const n = _.shdrCount();
        let i: usize = 0;
        while (i < n) {
            if (sec := _.getShdr(i)) {
                const name_off = span.off + (sec.sh_name as usize);
                if (name_off < span.end) {
                    const name_end = _.cStringEnd(name_off, span.end);
                    if (std.mem.eql(u8, _.bytes[name_off..name_end], name)) {
                        return sec;
                    }
                }
            }
            i += 1;
        }
        return null;
    }

    pub fn findSectionData(name: []const u8): ([]const u8)? {
        const sec = _.findSection(name) ?? return null;
        return _.sectionData(sec);
    }

    pub fn findSectionByType(sh_type: u32): Elf64Shdr? {
        const n = _.shdrCount();
        let i: usize = 0;
        while (i < n) {
            if (sec := _.getShdr(i)) {
                if (sec.sh_type == sh_type) {
                    return sec;
                }
            }
            i += 1;
        }
        return null;
    }

    // --- Symbol Table (.symtab / .dynsym) ---
    fn symbolSection(is_dynamic: bool): Elf64Shdr? {
        if (is_dynamic) {
            if (sec := _.findSection(".dynsym")) {
                return sec;
            }
            return _.findSectionByType(SHT_DYNSYM);
        }
        if (sec := _.findSection(".symtab")) {
            return sec;
        }
        return _.findSectionByType(SHT_SYMTAB);
    }

    fn symbolStrtabSpan(sym_sh: Elf64Shdr, is_dynamic: bool): ByteSpan? {
        if (sym_sh.sh_link > 0) {
            if (linked := _.getShdr(sym_sh.sh_link as usize)) {
                return _.shdrSpan(linked);
            }
        }
        const sec = (if (is_dynamic) _.findSection(".dynstr") else _.findSection(".strtab")) ?? return null;
        return _.shdrSpan(sec);
    }

    pub fn symbolCount(is_dynamic: bool): usize {
        const sec = _.symbolSection(is_dynamic) ?? return 0;
        const entsize: usize = if (sec.sh_entsize >= 24) (sec.sh_entsize as usize) else 24;
        return (sec.sh_size as usize) / entsize;
    }

    pub fn getSymbol(index: usize, is_dynamic: bool): Elf64Sym? {
        const sec = _.symbolSection(is_dynamic) ?? return null;
        const span = _.shdrSpan(sec) ?? return null;
        const entsize: usize = if (sec.sh_entsize >= 24) (sec.sh_entsize as usize) else 24;
        const off = span.off + index * entsize;
        if (off + 24 > span.end) {
            return null;
        }
        const le = _.ehdr.is_little_endian;
        return Elf64Sym{
            st_name: ExeInspector.readU32(_.bytes, off, le),
            st_info: _.bytes[off + 4],
            st_other: _.bytes[off + 5],
            st_shndx: ExeInspector.readU16(_.bytes, off + 6, le),
            st_value: ExeInspector.readU64(_.bytes, off + 8, le),
            st_size: ExeInspector.readU64(_.bytes, off + 16, le),
        };
    }

    pub fn symbolName(sym: Elf64Sym, is_dynamic: bool): []const u8 {
        const sec = _.symbolSection(is_dynamic) ?? return "";
        const span = _.symbolStrtabSpan(sec, is_dynamic) ?? return "";
        const name_off = span.off + (sym.st_name as usize);
        if (name_off >= span.end) {
            return "";
        }
        const name_end = _.cStringEnd(name_off, span.end);
        return _.bytes[name_off..name_end];
    }

    pub fn findSymbol(name: []const u8, is_dynamic: bool): Elf64Sym? {
        const sec = _.symbolSection(is_dynamic) ?? return null;
        const span = _.symbolStrtabSpan(sec, is_dynamic) ?? return null;
        const count = _.symbolCount(is_dynamic);
        let i: usize = 0;
        while (i < count) {
            if (sym := _.getSymbol(i, is_dynamic)) {
                const name_off = span.off + (sym.st_name as usize);
                if (name_off < span.end) {
                    const name_end = _.cStringEnd(name_off, span.end);
                    if (std.mem.eql(u8, _.bytes[name_off..name_end], name)) {
                        return sym;
                    }
                }
            }
            i += 1;
        }
        return null;
    }

    // --- Dynamic Section (.dynamic & PT_DYNAMIC) ---
    fn dynamicSpan(): ByteSpan? {
        if (sec := _.findSection(".dynamic")) {
            return _.shdrSpan(sec);
        }
        if (sec2 := _.findSectionByType(SHT_DYNAMIC)) {
            return _.shdrSpan(sec2);
        }
        if (ph := _.findPhdr(PT_DYNAMIC)) {
            const off = ph.p_offset as usize;
            const sz = ph.p_filesz as usize;
            if (off + sz <= _.bytes.len) {
                return ByteSpan{ off: off, end: off + sz };
            }
        }
        return null;
    }

    fn dynstrSpan(): ByteSpan? {
        if (sec := _.findSection(".dynamic")) {
            if (sec.sh_link > 0) {
                if (linked := _.getShdr(sec.sh_link as usize)) {
                    return _.shdrSpan(linked);
                }
            }
        } else if (sec2 := _.findSectionByType(SHT_DYNAMIC)) {
            if (sec2.sh_link > 0) {
                if (linked := _.getShdr(sec2.sh_link as usize)) {
                    return _.shdrSpan(linked);
                }
            }
        }
        const str_sh = _.findSection(".dynstr") ?? return null;
        return _.shdrSpan(str_sh);
    }

    pub fn dynamicCount(): usize {
        const span = _.dynamicSpan() ?? return 0;
        const total = (span.end - span.off) / 16;
        let i: usize = 0;
        const le = _.ehdr.is_little_endian;
        while (i < total) {
            const tag = ExeInspector.readI64(_.bytes, span.off + i * 16, le);
            if (tag == DT_NULL) {
                return i;
            }
            i += 1;
        }
        return total;
    }

    pub fn getDynamic(index: usize): Elf64Dyn? {
        const span = _.dynamicSpan() ?? return null;
        const off = span.off + index * 16;
        if (off + 16 > span.end) {
            return null;
        }
        const le = _.ehdr.is_little_endian;
        return Elf64Dyn{
            d_tag: ExeInspector.readI64(_.bytes, off, le),
            d_val: ExeInspector.readU64(_.bytes, off + 8, le),
        };
    }

    pub fn dynamicString(offset: u64): ([]const u8)? {
        const span = _.dynstrSpan() ?? return null;
        const off = span.off + (offset as usize);
        if (off >= span.end) {
            return null;
        }
        const end = _.cStringEnd(off, span.end);
        return _.bytes[off..end];
    }

    pub fn getNeededLibs(out: [][]const u8): usize {
        let count: usize = 0;
        const total = _.dynamicCount();
        let i: usize = 0;
        while (i < total && count < out.len) {
            if (d := _.getDynamic(i)) {
                if (d.d_tag == DT_NEEDED) {
                    if (lib := _.dynamicString(d.d_val)) {
                        out[count] = lib;
                        count += 1;
                    }
                }
            }
            i += 1;
        }
        return count;
    }

    fn findDynamicStringTag(tag: i64): ([]const u8)? {
        const total = _.dynamicCount();
        let i: usize = 0;
        while (i < total) {
            if (d := _.getDynamic(i)) {
                if (d.d_tag == tag) {
                    return _.dynamicString(d.d_val);
                }
            }
            i += 1;
        }
        return null;
    }

    pub fn soname(): ([]const u8)? {
        return _.findDynamicStringTag(DT_SONAME);
    }

    pub fn rpath(): ([]const u8)? {
        return _.findDynamicStringTag(DT_RPATH);
    }

    pub fn runpath(): ([]const u8)? {
        return _.findDynamicStringTag(DT_RUNPATH);
    }
}

pub type Elf64Parser = Elf64File;
