// Reference-hardened against gimli-rs/object (MIT/Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc Microsoft COFF / PE-COFF Object Parser + 16-Lane SIMD Scanner) | Created: 2026-11-22

import { simdObjTrimNul16, simdObjBytesEqual } from "./macho.cp";

pub const COFF_HEADER_SIZE: usize = 20;
pub const COFF_SECTION_SIZE: usize = 40;
pub const COFF_SYMBOL_SIZE: usize = 18;
pub const COFF_RELOC_SIZE: usize = 10;

pub const PE_DOS_MAGIC: u16 = 0x5A4D;
pub const PE32_OPT_MAGIC: u16 = 0x010B;
pub const PE32PLUS_OPT_MAGIC: u16 = 0x020B;
pub const PE32_MIN_OPT_HEADER_SIZE: u16 = 96;
pub const PE32PLUS_MIN_OPT_HEADER_SIZE: u16 = 112;

/// Safety bounds preventing resource exhaustion & multiplication overflow
pub const COFF_MAX_SECTIONS: u16 = 4096;
pub const COFF_MAX_SYMBOLS: u32 = 1_000_000;

// Machine Types
pub const MACHINE_UNKNOWN: u16 = 0x0000;
pub const MACHINE_I386: u16 = 0x014C;
pub const MACHINE_ARM: u16 = 0x01C0;
pub const MACHINE_AMD64: u16 = 0x8664;
pub const MACHINE_ARM64: u16 = 0xAA64;
pub const MACHINE_RISCV64: u16 = 0x5064;

// Section Characteristics
pub const SCN_CNT_CODE: u32 = 0x00000020;
pub const SCN_CNT_INITIALIZED_DATA: u32 = 0x00000040;
pub const SCN_CNT_UNINITIALIZED_DATA: u32 = 0x00000080;
pub const SCN_MEM_EXECUTE: u32 = 0x20000000;
pub const SCN_MEM_READ: u32 = 0x40000000;
pub const SCN_MEM_WRITE: u32 = 0x80000000;

// Symbol Storage Classes
pub const SYM_CLASS_EXTERNAL: u8 = 2;
pub const SYM_CLASS_STATIC: u8 = 3;
pub const SYM_CLASS_FILE: u8 = 103;
pub const SYM_CLASS_SECTION: u8 = 104;

// Special Section Numbers
pub const SYM_UNDEFINED: i16 = 0;
pub const SYM_ABSOLUTE: i16 = -1;
pub const SYM_DEBUG: i16 = -2;

pub err CoffError {
    TruncatedHeader,
    InvalidMachine,
    InvalidPeSignature,
    InvalidOptionalHeader,
    TooManySections,
    SectionTableOutOfBounds,
    SectionDataOutOfBounds,
    TooManySymbols,
    SymbolTableOutOfBounds,
    InvalidStringTable,
}

fn readU16Le(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return b0 | (b1 << 8);
}

fn readI16Le(buf: []const u8, off: usize): i16 {
    return @bitCast(readU16Le(buf, off));
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn copy8(src: []const u8, off: usize): [8]u8 {
    let out: [8]u8 = [0; 8];
    let i: usize = 0;
    while (i < 8) {
        out[i] = src[off + i];
        i += 1;
    }
    return out;
}

fn parseDecimalU32(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < '0' || ch > '9') {
            return null;
        }
        acc = acc * 10 + ((ch - '0') as u64);
        if (acc > 0xFFFFFFFF) {
            return null;
        }
        i += 1;
    }
    return acc as u32;
}

/// Returns true if `machine` matches one of the recognized COFF machine IDs.
pub fn isKnownCoffMachine(machine: u16): bool {
    return machine == MACHINE_UNKNOWN ||
        machine == MACHINE_I386 ||
        machine == MACHINE_ARM ||
        machine == MACHINE_AMD64 ||
        machine == MACHINE_ARM64 ||
        machine == MACHINE_RISCV64;
}

/// Locates the 20-byte COFF `IMAGE_FILE_HEADER` offset inside either a PE image (MZ + `PE\0\0`)
/// or a standalone COFF `.obj` buffer (offset `0`). Returns `null` if MZ header is malformed.
pub fn locateCoffHeaderOffset(data: []const u8): usize? {
    if (data.len >= 2 && readU16Le(data, 0) == PE_DOS_MAGIC) {
        if (data.len < 64) return null;
        const pe_off_u64: u64 = readU32Le(data, 60) as u64;
        const total_u64: u64 = data.len as u64;
        if (pe_off_u64 > total_u64 || 4 > total_u64 - pe_off_u64) return null;
        const pe_off = pe_off_u64 as usize;
        if (!simdObjBytesEqual(data[pe_off .. pe_off + 4], "PE\x00\x00")) {
            return null;
        }
        const coff_off = pe_off + 4;
        if (coff_off > data.len || COFF_HEADER_SIZE > data.len - coff_off) return null;
        return coff_off;
    }
    if (data.len < COFF_HEADER_SIZE) return null;
    return 0;
}

/// Parsed 20-byte COFF file header (`IMAGE_FILE_HEADER`).
pub struct CoffHeader {
    machine: u16,
    num_sections: u16,
    timestamp: u32,
    ptr_to_symbol_table: u32,
    num_symbols: u32,
    size_of_opt_header: u16,
    characteristics: u16,
}

/// Parsed 40-byte COFF section header (`IMAGE_SECTION_HEADER`).
pub struct CoffSection {
    raw_name: [8]u8,
    virtual_size: u32,
    virtual_addr: u32,
    size_of_raw_data: u32,
    ptr_to_raw_data: u32,
    ptr_to_relocs: u32,
    num_relocs: u16,
    characteristics: u32,
    raw_off: usize,

    fn isCode(): bool {
        return (_.characteristics & SCN_CNT_CODE) != 0;
    }

    fn isReadable(): bool {
        return (_.characteristics & SCN_MEM_READ) != 0;
    }

    fn isWritable(): bool {
        return (_.characteristics & SCN_MEM_WRITE) != 0;
    }

    fn isExecutable(): bool {
        return (_.characteristics & SCN_MEM_EXECUTE) != 0;
    }

    /// Returns the NUL-trimmed short name slice from `raw_name[0..8]` using SIMD NUL scan.
    fn shortName(): []const u8 {
        return simdObjTrimNul16(_.raw_name[0..8]);
    }
}

/// Parsed 18-byte COFF symbol table entry (`IMAGE_SYMBOL`).
pub struct CoffSymbol {
    raw_name: [8]u8,
    str_table_offset: u32?,
    value: u32,
    section_num: i16,
    sym_type: u16,
    storage_class: u8,
    num_aux_symbols: u8,
    raw_off: usize,

    fn isExternal(): bool {
        return _.storage_class == SYM_CLASS_EXTERNAL;
    }

    fn isStatic(): bool {
        return _.storage_class == SYM_CLASS_STATIC;
    }

    fn isUndefined(): bool {
        return _.section_num == SYM_UNDEFINED;
    }

    fn isFunction(): bool {
        return _.sym_type == 0x20;
    }
}

/// Parsed 10-byte COFF relocation entry (`IMAGE_RELOCATION`).
pub struct CoffReloc {
    virtual_addr: u32,
    symbol_idx: u32,
    reloc_type: u16,
}

/// Zero-copy view of a parsed COFF / PE-COFF `.obj` or PE32/PE32+ file.
pub struct CoffObject {
    raw: []const u8,
    header: CoffHeader,
    sec_table_off: usize,
    str_table_off: usize,
    str_table_len: usize,

    /// Returns the section header at 0-based index `idx`.
    fn sectionAt(idx: usize): CoffSection? {
        if (idx >= (_.header.num_sections as usize)) {
            return null;
        }
        const off = _.sec_table_off + idx * COFF_SECTION_SIZE;
        if (off > _.raw.len || COFF_SECTION_SIZE > _.raw.len - off) {
            return null;
        }
        return CoffSection {
            raw_name: copy8(_.raw, off),
            virtual_size: readU32Le(_.raw, off + 8),
            virtual_addr: readU32Le(_.raw, off + 12),
            size_of_raw_data: readU32Le(_.raw, off + 16),
            ptr_to_raw_data: readU32Le(_.raw, off + 20),
            ptr_to_relocs: readU32Le(_.raw, off + 24),
            num_relocs: readU16Le(_.raw, off + 32),
            characteristics: readU32Le(_.raw, off + 36),
            raw_off: off,
        };
    }

    /// Finds the first section whose resolved name matches `target` using 16-lane SIMD comparison.
    pub fn findSectionByName(target: []const u8): CoffSection? {
        let i: usize = 0;
        const count = _.header.num_sections as usize;
        while (i < count) : (i += 1) {
            if (sec := _.sectionAt(i)) {
                if (name := _.sectionName(sec)) {
                    if (simdObjBytesEqual(name, target)) {
                        return sec;
                    }
                }
            }
        }
        return null;
    }

    /// Returns a zero-copy slice of the raw section data.
    fn sectionData(sec: CoffSection): ([]const u8)? {
        if (sec.size_of_raw_data == 0) {
            return _.raw[0..0];
        }
        const start = sec.ptr_to_raw_data as usize;
        const size = sec.size_of_raw_data as usize;
        if (start > _.raw.len || size > _.raw.len - start) {
            return null;
        }
        return _.raw[start .. start + size];
    }

    /// Looks up a NUL-terminated string at byte offset `off` in the COFF string table.
    fn stringAt(off: u32): ([]const u8)? {
        if (_.str_table_len < 4) {
            return null;
        }
        const rel = off as usize;
        if (rel < 4 || rel >= _.str_table_len) {
            return null;
        }
        const start = _.str_table_off + rel;
        const limit = _.str_table_off + _.str_table_len;
        const trimmed = simdObjTrimNul16(_.raw[start..limit]);
        if (trimmed.len >= limit - start) {
            return null;
        }
        return trimmed;
    }

    /// Resolves the section name, supporting `/<decimal>` long names in the COFF string table.
    fn sectionName(sec: CoffSection): ([]const u8)? {
        const trimmed = simdObjTrimNul16(sec.raw_name[0..8]);
        if (trimmed.len >= 2 && trimmed[0] == '/') {
            if (off := parseDecimalU32(trimmed[1..])) {
                return _.stringAt(off);
            }
        }
        if (sec.raw_off + trimmed.len > _.raw.len) {
            return null;
        }
        return _.raw[sec.raw_off .. sec.raw_off + trimmed.len];
    }

    /// Returns the symbol table entry at 0-based index `idx`.
    fn symbolAt(idx: usize): CoffSymbol? {
        if (_.header.ptr_to_symbol_table == 0 || idx >= (_.header.num_symbols as usize)) {
            return null;
        }
        const sym_base = _.header.ptr_to_symbol_table as u64;
        const rel = (idx as u64) * (COFF_SYMBOL_SIZE as u64);
        const total = _.raw.len as u64;
        if (sym_base > total || rel > total - sym_base || (COFF_SYMBOL_SIZE as u64) > total - sym_base - rel) {
            return null;
        }
        const off = (sym_base + rel) as usize;
        const zeroes = readU32Le(_.raw, off);
        let str_off: u32? = null;
        if (zeroes == 0) {
            str_off = readU32Le(_.raw, off + 4);
        }
        return CoffSymbol {
            raw_name: copy8(_.raw, off),
            str_table_offset: str_off,
            value: readU32Le(_.raw, off + 8),
            section_num: readI16Le(_.raw, off + 12),
            sym_type: readU16Le(_.raw, off + 14),
            storage_class: _.raw[off + 16],
            num_aux_symbols: _.raw[off + 17],
            raw_off: off,
        };
    }

    /// Resolves a symbol's name (inline 8-byte short name or COFF string table lookup).
    fn symbolName(sym: CoffSymbol): ([]const u8)? {
        if (off := sym.str_table_offset) {
            return _.stringAt(off);
        }
        const trimmed = simdObjTrimNul16(sym.raw_name[0..8]);
        if (sym.raw_off + trimmed.len > _.raw.len) {
            return null;
        }
        return _.raw[sym.raw_off .. sym.raw_off + trimmed.len];
    }

    /// Returns relocation entry `idx` for section `sec`.
    fn relocAt(sec: CoffSection, idx: usize): CoffReloc? {
        if (idx >= (sec.num_relocs as usize)) {
            return null;
        }
        const start = sec.ptr_to_relocs as u64;
        const rel_off = (idx as u64) * (COFF_RELOC_SIZE as u64);
        const total = _.raw.len as u64;
        if (start > total || rel_off > total - start || (COFF_RELOC_SIZE as u64) > total - start - rel_off) {
            return null;
        }
        const off = (start + rel_off) as usize;
        return CoffReloc {
            virtual_addr: readU32Le(_.raw, off),
            symbol_idx: readU32Le(_.raw, off + 4),
            reloc_type: readU16Le(_.raw, off + 8),
        };
    }
}

fn parseCoffAtOffset(data: []const u8, hdr_off: usize): CoffObject? {
    if (hdr_off > data.len || COFF_HEADER_SIZE > data.len - hdr_off) {
        return null;
    }

    const machine = readU16Le(data, hdr_off);
    if (!isKnownCoffMachine(machine)) {
        return null;
    }

    const num_sections = readU16Le(data, hdr_off + 2);
    if (num_sections > COFF_MAX_SECTIONS) {
        return null;
    }
    const timestamp = readU32Le(data, hdr_off + 4);
    const ptr_to_symbol_table = readU32Le(data, hdr_off + 8);
    const num_symbols = readU32Le(data, hdr_off + 12);
    const size_of_opt_header = readU16Le(data, hdr_off + 16);
    const characteristics = readU16Le(data, hdr_off + 18);

    const opt_off = hdr_off + COFF_HEADER_SIZE;
    const total_u64 = data.len as u64;
    if ((opt_off as u64) + (size_of_opt_header as u64) > total_u64) {
        return null;
    }
    if (size_of_opt_header > 0) {
        if (size_of_opt_header < 2) return null;
        const opt_magic = readU16Le(data, opt_off);
        if (opt_magic == PE32_OPT_MAGIC) {
            if (size_of_opt_header < PE32_MIN_OPT_HEADER_SIZE) return null;
        } else if (opt_magic == PE32PLUS_OPT_MAGIC) {
            if (size_of_opt_header < PE32PLUS_MIN_OPT_HEADER_SIZE) return null;
        } else {
            return null;
        }
    }

    const sec_table_off = opt_off + (size_of_opt_header as usize);
    const sec_table_bytes: u64 = (num_sections as u64) * (COFF_SECTION_SIZE as u64);
    if ((sec_table_off as u64) > total_u64 || sec_table_bytes > total_u64 - (sec_table_off as u64)) {
        return null;
    }

    let str_table_off: usize = 0;
    let str_table_len: usize = 0;
    if (ptr_to_symbol_table == 0) {
        if (num_symbols != 0) {
            return null;
        }
    } else {
        if (num_symbols > COFF_MAX_SYMBOLS) {
            return null;
        }
        const sym_off: u64 = ptr_to_symbol_table as u64;
        const sym_bytes: u64 = (num_symbols as u64) * (COFF_SYMBOL_SIZE as u64);
        if (sym_off > total_u64 || sym_bytes > total_u64 - sym_off) {
            return null;
        }
        const st_off_u64 = sym_off + sym_bytes;
        if (st_off_u64 < total_u64) {
            if (total_u64 - st_off_u64 < 4) {
                return null;
            }
            const st_off = st_off_u64 as usize;
            const st_size: u64 = readU32Le(data, st_off) as u64;
            if (st_size < 4 || st_size > total_u64 - st_off_u64) {
                return null;
            }
            str_table_off = st_off;
            str_table_len = st_size as usize;
        }
    }

    return CoffObject {
        raw: data,
        header: CoffHeader {
            machine: machine,
            num_sections: num_sections,
            timestamp: timestamp,
            ptr_to_symbol_table: ptr_to_symbol_table,
            num_symbols: num_symbols,
            size_of_opt_header: size_of_opt_header,
            characteristics: characteristics,
        },
        sec_table_off: sec_table_off,
        str_table_off: str_table_off,
        str_table_len: str_table_len,
    };
}

/// Parses and validates a standalone COFF `.obj` buffer (offset 0).
pub fn parseCoff(data: []const u8): CoffObject? {
    return parseCoffAtOffset(data, 0);
}

/// Parses and validates either a PE32/PE32+ image (MZ + `PE\0\0` + COFF) or a standalone COFF `.obj`.
pub fn parsePeOrCoff(data: []const u8): CoffObject? {
    const hdr_off = locateCoffHeaderOffset(data) ?? return null;
    return parseCoffAtOffset(data, hdr_off);
}

/// Zero-allocation strict validator for COFF `.obj` and PE32/PE32+ images aligned with `gimli-rs/object`.
/// Verifies MZ/PE signatures, machine ID, optional header magic/size, section table bounds,
/// every section's `PointerToRawData + SizeOfRawData` & relocations, and symbol/string table bounds.
pub fn validateCoffStrict(data: []const u8): CoffError!void {
    if (data.len < COFF_HEADER_SIZE) return err.TruncatedHeader;

    let hdr_off: usize = 0;
    if (readU16Le(data, 0) == PE_DOS_MAGIC) {
        if (data.len < 64) return err.TruncatedHeader;
        const pe_off_u64: u64 = readU32Le(data, 60) as u64;
        const total_u64: u64 = data.len as u64;
        if (pe_off_u64 > total_u64 || 4 > total_u64 - pe_off_u64) return err.InvalidPeSignature;
        const pe_off = pe_off_u64 as usize;
        if (!simdObjBytesEqual(data[pe_off .. pe_off + 4], "PE\x00\x00")) {
            return err.InvalidPeSignature;
        }
        hdr_off = pe_off + 4;
        if (hdr_off > data.len || COFF_HEADER_SIZE > data.len - hdr_off) return err.TruncatedHeader;
    }

    const machine = readU16Le(data, hdr_off);
    if (!isKnownCoffMachine(machine)) return err.InvalidMachine;

    const num_sections = readU16Le(data, hdr_off + 2);
    if (num_sections > COFF_MAX_SECTIONS) return err.TooManySections;

    const ptr_to_symbol_table = readU32Le(data, hdr_off + 8);
    const num_symbols = readU32Le(data, hdr_off + 12);
    const size_of_opt_header = readU16Le(data, hdr_off + 16);

    const total_u64: u64 = data.len as u64;
    const opt_off: usize = hdr_off + COFF_HEADER_SIZE;
    if ((opt_off as u64) + (size_of_opt_header as u64) > total_u64) {
        return err.InvalidOptionalHeader;
    }
    if (size_of_opt_header > 0) {
        if (size_of_opt_header < 2) return err.InvalidOptionalHeader;
        const opt_magic = readU16Le(data, opt_off);
        if (opt_magic == PE32_OPT_MAGIC) {
            if (size_of_opt_header < PE32_MIN_OPT_HEADER_SIZE) return err.InvalidOptionalHeader;
        } else if (opt_magic == PE32PLUS_OPT_MAGIC) {
            if (size_of_opt_header < PE32PLUS_MIN_OPT_HEADER_SIZE) return err.InvalidOptionalHeader;
        } else {
            return err.InvalidOptionalHeader;
        }
    }

    const sec_table_off: usize = opt_off + (size_of_opt_header as usize);
    const sec_table_bytes: u64 = (num_sections as u64) * (COFF_SECTION_SIZE as u64);
    if ((sec_table_off as u64) > total_u64 || sec_table_bytes > total_u64 - (sec_table_off as u64)) {
        return err.SectionTableOutOfBounds;
    }

    // Validate every section's raw data and relocation table bounds
    let si: usize = 0;
    while (si < (num_sections as usize)) : (si += 1) {
        const s_off = sec_table_off + si * COFF_SECTION_SIZE;
        const size_of_raw_data: u64 = readU32Le(data, s_off + 16) as u64;
        const ptr_to_raw_data: u64 = readU32Le(data, s_off + 20) as u64;
        const ptr_to_relocs: u64 = readU32Le(data, s_off + 24) as u64;
        const num_relocs: u64 = readU16Le(data, s_off + 32) as u64;

        if (size_of_raw_data > 0) {
            if (ptr_to_raw_data > total_u64 || size_of_raw_data > total_u64 - ptr_to_raw_data) {
                return err.SectionDataOutOfBounds;
            }
        }
        if (num_relocs > 0) {
            const rel_bytes: u64 = num_relocs * (COFF_RELOC_SIZE as u64);
            if (ptr_to_relocs > total_u64 || rel_bytes > total_u64 - ptr_to_relocs) {
                return err.SectionDataOutOfBounds;
            }
        }
    }

    // Validate symbol table & string table bounds
    if (ptr_to_symbol_table == 0) {
        if (num_symbols != 0) return err.SymbolTableOutOfBounds;
    } else {
        if (num_symbols > COFF_MAX_SYMBOLS) return err.TooManySymbols;
        const sym_off: u64 = ptr_to_symbol_table as u64;
        const sym_bytes: u64 = (num_symbols as u64) * (COFF_SYMBOL_SIZE as u64);
        if (sym_off > total_u64 || sym_bytes > total_u64 - sym_off) {
            return err.SymbolTableOutOfBounds;
        }
        const st_off_u64 = sym_off + sym_bytes;
        if (st_off_u64 < total_u64) {
            if (total_u64 - st_off_u64 < 4) return err.InvalidStringTable;
            const st_size: u64 = readU32Le(data, st_off_u64 as usize) as u64;
            if (st_size < 4 || st_size > total_u64 - st_off_u64) {
                return err.InvalidStringTable;
            }
        }
    }
}
