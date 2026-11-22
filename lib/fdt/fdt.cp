// Origin: Pure-CP Stdlib (Zero-Alloc Devicetree Spec v0.4 Flattened Device Tree / .dtb Parser) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// FDT Protocol Constants (Big-Endian)
// ---------------------------------------------------------------------------

pub const FDT_MAGIC: u32 = 0xD00DFEED;
pub const FDT_HEADER_SIZE: usize = 40;
pub const FDT_VERSION: u32 = 17;

// Structure Block Token Tags
pub const FDT_BEGIN_NODE: u32 = 0x00000001;
pub const FDT_END_NODE: u32 = 0x00000002;
pub const FDT_PROP: u32 = 0x00000003;
pub const FDT_NOP: u32 = 0x00000004;
pub const FDT_END: u32 = 0x00000009;

// ---------------------------------------------------------------------------
// Internal Big-Endian & C-String Helpers
// ---------------------------------------------------------------------------

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readU64Be(buf: []const u8, off: usize): u64 {
    const hi = readU32Be(buf, off) as u64;
    const lo = readU32Be(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn align4(n: usize): usize {
    const rem = n % 4;
    if (rem == 0) {
        return n;
    }
    return n + (4 - rem);
}

fn readCStringAt(buf: []const u8, off: usize): ([]const u8)? {
    if (off >= buf.len) {
        return null;
    }
    let i: usize = off;
    while (i < buf.len) {
        if (buf[i] == 0) {
            return buf[off..i];
        }
        i += 1;
    }
    return null;
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// 40-byte Flattened Device Tree Header (`fdt_header`).
pub struct FdtHeader {
    totalsize: u32,
    off_dt_struct: u32,
    off_dt_strings: u32,
    off_mem_rsvmap: u32,
    version: u32,
    last_comp_version: u32,
    boot_cpuid_phys: u32,
    size_dt_strings: u32,
    size_dt_struct: u32,
}

/// A single 16-byte entry from the FDT Memory Reservation Block.
pub struct FdtMemRsv {
    address: u64,
    size: u64,
}

/// Zero-copy iterator over Memory Reservation Block entries until the `(0, 0)` sentinel.
pub struct FdtMemRsvIter {
    bytes: []const u8,
    pos: usize,
    done: bool,

    fn next(*_): FdtMemRsv? {
        if (_.done || _.pos + 16 > _.bytes.len) {
            _.done = true;
            return null;
        }
        const addr = readU64Be(_.bytes, _.pos);
        const sz = readU64Be(_.bytes, _.pos + 8);
        _.pos += 16;
        if (addr == 0 && sz == 0) {
            _.done = true;
            return null;
        }
        return FdtMemRsv {
            address: addr,
            size: sz,
        };
    }
}

/// Classification of yielded Structure Block tokens (`FDT_NOP` is skipped automatically).
pub enum FdtTokenKind: u8 {
    begin_node = 1,
    end_node = 2,
    prop = 3,
    end = 9,
}

/// Zero-copy token yielded from the FDT Structure Block.
pub struct FdtToken {
    kind: FdtTokenKind,
    depth: u32,
    name: []const u8,
    value: []const u8,

    /// Decodes a 4-byte big-endian property value as `u32`.
    fn asU32(): u32? {
        if (_.value.len != 4) {
            return null;
        }
        return readU32Be(_.value, 0);
    }

    /// Decodes an 8-byte big-endian property value as `u64`.
    fn asU64(): u64? {
        if (_.value.len != 8) {
            return null;
        }
        return readU64Be(_.value, 0);
    }

    /// Decodes a NUL-terminated string property value (stripping the trailing NUL byte).
    fn asString(): ([]const u8)? {
        if (_.value.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.value.len) {
            if (_.value[i] == 0) {
                return _.value[0..i];
            }
            i += 1;
        }
        return null;
    }
}

/// Single-pass zero-copy iterator over the FDT Structure Block.
/// Automatically skips `FDT_NOP`, maintains 4-byte alignment, and tracks nesting `depth`.
pub struct FdtTokenIter {
    struct_bytes: []const u8,
    strings_bytes: []const u8,
    pos: usize,
    depth: u32,
    done: bool,
    err: bool,

    fn hasError(): bool {
        return _.err;
    }

    fn next(*_): FdtToken? {
        while (!_.done && !_.err) {
            if (_.pos + 4 > _.struct_bytes.len) {
                _.err = true;
                _.done = true;
                return null;
            }
            const tag = readU32Be(_.struct_bytes, _.pos);
            _.pos += 4;

            if (tag == FDT_NOP) {
                continue;
            }

            if (tag == FDT_BEGIN_NODE) {
                const name_start = _.pos;
                let nul_pos: usize = name_start;
                let found_nul: bool = false;
                while (nul_pos < _.struct_bytes.len) {
                    if (_.struct_bytes[nul_pos] == 0) {
                        found_nul = true;
                        break;
                    }
                    nul_pos += 1;
                }
                if (!found_nul) {
                    _.err = true;
                    _.done = true;
                    return null;
                }
                const next_pos = align4(nul_pos + 1);
                if (next_pos > _.struct_bytes.len) {
                    _.err = true;
                    _.done = true;
                    return null;
                }
                const cur_depth = _.depth;
                _.depth += 1;
                _.pos = next_pos;
                return FdtToken {
                    kind: FdtTokenKind.begin_node,
                    depth: cur_depth,
                    name: _.struct_bytes[name_start..nul_pos],
                    value: _.struct_bytes[0..0],
                };
            }

            if (tag == FDT_END_NODE) {
                if (_.depth == 0) {
                    _.err = true;
                    _.done = true;
                    return null;
                }
                _.depth -= 1;
                return FdtToken {
                    kind: FdtTokenKind.end_node,
                    depth: _.depth,
                    name: _.struct_bytes[0..0],
                    value: _.struct_bytes[0..0],
                };
            }

            if (tag == FDT_PROP) {
                if (_.pos + 8 > _.struct_bytes.len) {
                    _.err = true;
                    _.done = true;
                    return null;
                }
                const prop_len = readU32Be(_.struct_bytes, _.pos) as usize;
                const nameoff = readU32Be(_.struct_bytes, _.pos + 4);
                _.pos += 8;

                const rem = _.struct_bytes.len - _.pos;
                if (prop_len > rem) {
                    _.err = true;
                    _.done = true;
                    return null;
                }
                const aligned_len = align4(prop_len);
                if (aligned_len > rem) {
                    _.err = true;
                    _.done = true;
                    return null;
                }

                if (prop_name := readCStringAt(_.strings_bytes, nameoff as usize)) {
                    const val_start = _.pos;
                    _.pos += aligned_len;
                    return FdtToken {
                        kind: FdtTokenKind.prop,
                        depth: _.depth,
                        name: prop_name,
                        value: _.struct_bytes[val_start .. val_start + prop_len],
                    };
                } else {
                    _.err = true;
                    _.done = true;
                    return null;
                }
            }

            if (tag == FDT_END) {
                _.done = true;
                if (_.depth != 0) {
                    _.err = true;
                    return null;
                }
                return FdtToken {
                    kind: FdtTokenKind.end,
                    depth: 0,
                    name: _.struct_bytes[0..0],
                    value: _.struct_bytes[0..0],
                };
            }

            // Unknown / invalid structure token
            _.err = true;
            _.done = true;
            return null;
        }
        return null;
    }
}

/// Validated Flattened Device Tree (`.dtb`) blob view.
pub struct FdtBlob {
    raw: []const u8,
    header: FdtHeader,

    /// Returns a zero-copy iterator over the Memory Reservation Block.
    fn memReservations(): FdtMemRsvIter {
        const start = _.header.off_mem_rsvmap as usize;
        const end = _.header.totalsize as usize;
        return FdtMemRsvIter {
            bytes: _.raw[start..end],
            pos: 0,
            done: false,
        };
    }

    /// Returns a zero-copy single-pass iterator over the Structure Block.
    fn tokens(): FdtTokenIter {
        const s_off = _.header.off_dt_struct as usize;
        const s_len = _.header.size_dt_struct as usize;
        const str_off = _.header.off_dt_strings as usize;
        const str_len = _.header.size_dt_strings as usize;
        return FdtTokenIter {
            struct_bytes: _.raw[s_off .. s_off + s_len],
            strings_bytes: _.raw[str_off .. str_off + str_len],
            pos: 0,
            depth: 0,
            done: false,
            err: false,
        };
    }

    /// Resolves a NUL-terminated string from the Strings Block at byte offset `nameoff`.
    fn stringAt(nameoff: u32): ([]const u8)? {
        const str_off = _.header.off_dt_strings as usize;
        const str_len = _.header.size_dt_strings as usize;
        return readCStringAt(_.raw[str_off .. str_off + str_len], nameoff as usize);
    }
}

// ---------------------------------------------------------------------------
// Parsing Functions
// ---------------------------------------------------------------------------

/// Returns `true` if `data` has at least 4 bytes and starts with `FDT_MAGIC` (`0xD00DFEED`).
pub fn isFdt(data: []const u8): bool {
    if (data.len < 4) {
        return false;
    }
    return readU32Be(data, 0) == FDT_MAGIC;
}

/// Parses and validates a Flattened Device Tree (`.dtb`) binary blob.
pub fn parseFdt(data: []const u8): FdtBlob? {
    if (data.len < FDT_HEADER_SIZE) {
        return null;
    }
    if (readU32Be(data, 0) != FDT_MAGIC) {
        return null;
    }

    const totalsize = readU32Be(data, 4);
    if ((totalsize as usize) < FDT_HEADER_SIZE || (totalsize as usize) > data.len) {
        return null;
    }

    const off_dt_struct = readU32Be(data, 8);
    const off_dt_strings = readU32Be(data, 12);
    const off_mem_rsvmap = readU32Be(data, 16);
    const version = readU32Be(data, 20);
    const last_comp_version = readU32Be(data, 24);
    const boot_cpuid_phys = readU32Be(data, 28);
    const size_dt_strings = readU32Be(data, 32);
    const size_dt_struct = readU32Be(data, 36);

    if (version < FDT_VERSION || last_comp_version > FDT_VERSION) {
        return null;
    }

    // All section offsets (and structure block size) must be 4-byte aligned and in-bounds
    const total_u64 = totalsize as u64;
    if ((off_mem_rsvmap % 4) != 0 || (off_mem_rsvmap as usize) < FDT_HEADER_SIZE || (off_mem_rsvmap as u64) > total_u64) {
        return null;
    }
    if ((off_dt_struct % 4) != 0 || (size_dt_struct % 4) != 0) {
        return null;
    }
    if ((off_dt_struct as usize) < FDT_HEADER_SIZE || (off_dt_struct as u64) + (size_dt_struct as u64) > total_u64) {
        return null;
    }
    if ((off_dt_strings % 4) != 0 || (off_dt_strings as usize) < FDT_HEADER_SIZE) {
        return null;
    }
    if ((off_dt_strings as u64) + (size_dt_strings as u64) > total_u64) {
        return null;
    }

    return FdtBlob {
        raw: data[0 .. totalsize as usize],
        header: FdtHeader {
            totalsize: totalsize,
            off_dt_struct: off_dt_struct,
            off_dt_strings: off_dt_strings,
            off_mem_rsvmap: off_mem_rsvmap,
            version: version,
            last_comp_version: last_comp_version,
            boot_cpuid_phys: boot_cpuid_phys,
            size_dt_strings: size_dt_strings,
            size_dt_struct: size_dt_struct,
        },
    };
}
