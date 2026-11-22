// Origin: Pure-CP Stdlib (Zero-Alloc Microsoft COFF / PE-COFF Object Parser) | Created: 2026-11-22

pub const COFF_HEADER_SIZE: usize = 20;
pub const COFF_SECTION_SIZE: usize = 40;
pub const COFF_SYMBOL_SIZE: usize = 18;
pub const COFF_RELOC_SIZE: usize = 10;

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
    let out: [8]u8 = undefined;
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

    /// Returns the NUL-trimmed short name slice from `raw_name[0..8]`.
    fn shortName(): []const u8 {
        let end: usize = 0;
        while (end < 8 && _.raw_name[end] != 0) {
            end += 1;
        }
        return _.raw_name[0..end];
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

/// Zero-copy view of a parsed COFF / PE-COFF `.obj` file.
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
        if (off + COFF_SECTION_SIZE > _.raw.len) {
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
        let end = start;
        while (end < limit && _.raw[end] != 0) {
            end += 1;
        }
        if (end >= limit) {
            return null;
        }
        return _.raw[start..end];
    }

    /// Resolves the section name, supporting `/<decimal>` long names in the COFF string table.
    fn sectionName(sec: CoffSection): ([]const u8)? {
        let len: usize = 0;
        while (len < 8 && sec.raw_name[len] != 0) {
            len += 1;
        }
        if (len >= 2 && sec.raw_name[0] == '/') {
            if (off := parseDecimalU32(sec.raw_name[1..len])) {
                return _.stringAt(off);
            }
        }
        if (sec.raw_off + len > _.raw.len) {
            return null;
        }
        return _.raw[sec.raw_off .. sec.raw_off + len];
    }

    /// Returns the symbol table entry at 0-based index `idx`.
    fn symbolAt(idx: usize): CoffSymbol? {
        if (_.header.ptr_to_symbol_table == 0 || idx >= (_.header.num_symbols as usize)) {
            return null;
        }
        const off = (_.header.ptr_to_symbol_table as usize) + idx * COFF_SYMBOL_SIZE;
        if (off + COFF_SYMBOL_SIZE > _.raw.len) {
            return null;
        }
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
        let len: usize = 0;
        while (len < 8 && sym.raw_name[len] != 0) {
            len += 1;
        }
        if (sym.raw_off + len > _.raw.len) {
            return null;
        }
        return _.raw[sym.raw_off .. sym.raw_off + len];
    }

    /// Returns relocation entry `idx` for section `sec`.
    fn relocAt(sec: CoffSection, idx: usize): CoffReloc? {
        if (idx >= (sec.num_relocs as usize)) {
            return null;
        }
        const start = sec.ptr_to_relocs as usize;
        const rel_off = idx * COFF_RELOC_SIZE;
        if (start > _.raw.len || rel_off > _.raw.len - start || _.raw.len - start - rel_off < COFF_RELOC_SIZE) {
            return null;
        }
        const off = start + rel_off;
        return CoffReloc {
            virtual_addr: readU32Le(_.raw, off),
            symbol_idx: readU32Le(_.raw, off + 4),
            reloc_type: readU16Le(_.raw, off + 8),
        };
    }
}

/// Parses and validates a COFF / PE-COFF `.obj` buffer.
/// Validates the 20B header, section table bounds, symbol table bounds, and 4B string table size.
pub fn parseCoff(data: []const u8): CoffObject? {
    if (data.len < COFF_HEADER_SIZE) {
        return null;
    }

    const machine = readU16Le(data, 0);
    if (!isKnownCoffMachine(machine)) {
        return null;
    }

    const num_sections = readU16Le(data, 2);
    const timestamp = readU32Le(data, 4);
    const ptr_to_symbol_table = readU32Le(data, 8);
    const num_symbols = readU32Le(data, 12);
    const size_of_opt_header = readU16Le(data, 16);
    const characteristics = readU16Le(data, 18);

    const sec_table_off = COFF_HEADER_SIZE + (size_of_opt_header as usize);
    const sec_table_bytes = (num_sections as usize) * COFF_SECTION_SIZE;
    if (sec_table_off > data.len || sec_table_bytes > data.len - sec_table_off) {
        return null;
    }

    let str_table_off: usize = 0;
    let str_table_len: usize = 0;
    if (ptr_to_symbol_table == 0) {
        if (num_symbols != 0) {
            return null;
        }
    } else {
        const sym_off = ptr_to_symbol_table as usize;
        const sym_bytes = (num_symbols as usize) * COFF_SYMBOL_SIZE;
        if (sym_off > data.len || sym_bytes > data.len - sym_off) {
            return null;
        }
        const st_off = sym_off + sym_bytes;
        if (st_off < data.len) {
            if (data.len - st_off < 4) {
                return null;
            }
            const st_size = readU32Le(data, st_off) as usize;
            if (st_size < 4 || st_size > data.len - st_off) {
                return null;
            }
            str_table_off = st_off;
            str_table_len = st_size;
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
