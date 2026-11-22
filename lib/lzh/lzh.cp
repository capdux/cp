// Origin: Pure-CP Stdlib (Zero-Alloc LZH/LHA Archive Level 0/1/2 & CRC-16/ARC Parser) | Created: 2026-11-22

// LZH Compression Methods (5 bytes)
pub const LZH_METHOD_LH0: []const u8 = "-lh0-";
pub const LZH_METHOD_LH4: []const u8 = "-lh4-";
pub const LZH_METHOD_LH5: []const u8 = "-lh5-";
pub const LZH_METHOD_LH6: []const u8 = "-lh6-";
pub const LZH_METHOD_LH7: []const u8 = "-lh7-";
pub const LZH_METHOD_LHD: []const u8 = "-lhd-";

// Header Levels
pub const LEVEL_0: u8 = 0;
pub const LEVEL_1: u8 = 1;
pub const LEVEL_2: u8 = 2;

// Extended Header Types
pub const EXT_COMMON_CRC: u8 = 0x00;
pub const EXT_FILENAME: u8 = 0x01;
pub const EXT_DIRECTORY: u8 = 0x02;
pub const EXT_UNIX_PERM: u8 = 0x50;
pub const EXT_UNIX_GID_UID: u8 = 0x51;
pub const EXT_UNIX_MTIME: u8 = 0x54;

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Computes the Level 0/1 LZH header 8-bit sum modulo 256.
pub fn lzhHeaderSum(bytes: []const u8): u8 {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        sum += bytes[i] as u32;
        i += 1;
    }
    return (sum & 0xFF) as u8;
}

fn lzhCrc16Update(init_crc: u16, data: []const u8): u16 {
    let crc = init_crc;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ (data[i] as u16);
        let bit: usize = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc;
}

/// Computes CRC-16/ARC (reflected polynomial `0xA001`, init `0x0000`, xorout `0x0000`).
/// Standard check vector: `"123456789"` -> `0xBB3D`.
pub fn lzhCrc16(data: []const u8): u16 {
    return lzhCrc16Update(0, data);
}

/// Returns `true` if `m` is a valid 5-byte LZH method identifier (`m[0] == '-' && m[4] == '-'`).
pub fn isLzhMethod(m: []const u8): bool {
    if (m.len != 5 || m[0] != 0x2D || m[4] != 0x2D) {
        return false;
    }
    let i: usize = 1;
    while (i < 4) {
        const c = m[i];
        const is_alnum = (c >= 0x30 && c <= 0x39) || (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A);
        if (!is_alnum) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Returns `true` if `data` starts with a plausible LZH header (valid 5-byte method at `2..7` and level `0..=2` at byte 20).
pub fn isLzh(data: []const u8): bool {
    if (data.len < 22) {
        return false;
    }
    return isLzhMethod(data[2..7]) && data[20] <= LEVEL_2;
}

/// Single LZH extended header entry (`ext_type` and zero-copy `data` payload).
pub struct LzhExtHeader {
    ext_type: u8,
    data: []const u8,
}

/// Zero-copy iterator over an LZH extended header chain (starting at the initial 2-byte `next_size`).
pub struct LzhExtIter {
    raw: []const u8,
    pos: usize,
    next_size: usize,

    pub fn init(ext_headers_bytes: []const u8): LzhExtIter {
        if (ext_headers_bytes.len < 2) {
            return LzhExtIter{
                .raw = ext_headers_bytes,
                .pos = 0,
                .next_size = 0,
            };
        }
        return LzhExtIter{
            .raw = ext_headers_bytes,
            .pos = 2,
            .next_size = readU16Le(ext_headers_bytes, 0) as usize,
        };
    }

    fn next(*_): LzhExtHeader? {
        if (_.next_size == 0) {
            return null;
        }
        const sz = _.next_size;
        if (sz < 3 || sz > _.raw.len - _.pos) {
            _.next_size = 0;
            return null;
        }
        const ext_type = _.raw[_.pos];
        const payload = _.raw[(_.pos + 1)..(_.pos + sz - 2)];
        const following = readU16Le(_.raw, _.pos + sz - 2) as usize;
        _.pos += sz;
        _.next_size = following;
        return LzhExtHeader{
            .ext_type = ext_type,
            .data = payload,
        };
    }
}

/// Parsed LZH archive entry (Level 0, Level 1, or Level 2) with zero-copy slices.
pub struct LzhEntry {
    level: u8,
    method: [5]u8,
    compressed_size: u32,
    original_size: u32,
    mtime_raw: u32,
    crc16: u16,
    os_id: u8,
    name: []const u8,
    dir_name: []const u8,
    ext_headers_bytes: []const u8,
    data: []const u8,
    consumed: usize,

    /// Returns `true` if the compression method is `"-lh0-"` (uncompressed store).
    fn isStored(): bool {
        return bytesEq(_.method[0..], LZH_METHOD_LH0);
    }

    /// Returns `true` if the compression method is `"-lhd-"` (directory entry).
    fn isDir(): bool {
        return bytesEq(_.method[0..], LZH_METHOD_LHD);
    }

    /// Returns a zero-copy iterator over this entry's extended headers (empty for Level 0).
    fn extHeaders(): LzhExtIter {
        return LzhExtIter.init(_.ext_headers_bytes);
    }

    /// Verifies `lzhCrc16(data) == crc16` for stored (`-lh0-`) entries.
    fn verifyDataCrc(): bool {
        if (!_.isStored()) {
            return false;
        }
        return lzhCrc16(_.data) == _.crc16;
    }
}

fn scanExtChainEnd(data: []const u8, first_size_off: usize, limit: usize): usize? {
    if (first_size_off + 2 > limit) {
        return null;
    }
    let cur_size = readU16Le(data, first_size_off) as usize;
    let pos = first_size_off + 2;
    let guard: usize = 0;
    while (cur_size != 0) {
        if (cur_size < 3 || cur_size > limit - pos || guard >= 1024) {
            return null;
        }
        const next_size = readU16Le(data, pos + cur_size - 2) as usize;
        pos += cur_size;
        cur_size = next_size;
        guard += 1;
    }
    return pos;
}

/// Parses a single LZH entry (Level 0, Level 1, or Level 2) with full header checksum/CRC validation.
pub fn parseLzhEntry(data: []const u8): LzhEntry? {
    if (data.len < 22 || !isLzhMethod(data[2..7])) {
        return null;
    }
    const level = data[20];
    if (level > LEVEL_2) {
        return null;
    }

    let method: [5]u8 = [0; 5];
    let mi: usize = 0;
    while (mi < 5) {
        method[mi] = data[2 + mi];
        mi += 1;
    }

    const raw_comp_size = readU32Le(data, 7);
    const original_size = readU32Le(data, 11);
    const mtime_raw = readU32Le(data, 15);

    if (level == LEVEL_0) {
        const hdr_size = data[0] as usize;
        const hdr_sum = data[1];
        if (hdr_size < 22 || 2 + hdr_size > data.len) {
            return null;
        }
        if (lzhHeaderSum(data[2..(2 + hdr_size)]) != hdr_sum) {
            return null;
        }
        const name_len = data[21] as usize;
        const hdr_end = 2 + hdr_size;
        if (22 + name_len + 2 > hdr_end) {
            return null;
        }
        const name = data[22..(22 + name_len)];
        const crc16 = readU16Le(data, 22 + name_len);
        let os_id: u8 = 0;
        if (22 + name_len + 2 < hdr_end) {
            os_id = data[22 + name_len + 2];
        }
        const comp_sz = raw_comp_size as usize;
        if (comp_sz > data.len - hdr_end) {
            return null;
        }
        return LzhEntry{
            .level = level,
            .method = method,
            .compressed_size = raw_comp_size,
            .original_size = original_size,
            .mtime_raw = mtime_raw,
            .crc16 = crc16,
            .os_id = os_id,
            .name = name,
            .dir_name = data[0..0],
            .ext_headers_bytes = data[0..0],
            .data = data[hdr_end..(hdr_end + comp_sz)],
            .consumed = hdr_end + comp_sz,
        };
    }

    if (level == LEVEL_1) {
        const hdr_size = data[0] as usize;
        const hdr_sum = data[1];
        if (hdr_size < 25 || 2 + hdr_size > data.len) {
            return null;
        }
        if (lzhHeaderSum(data[2..(2 + hdr_size)]) != hdr_sum) {
            return null;
        }
        const name_len = data[21] as usize;
        const base_end = 2 + hdr_size;
        if (22 + name_len + 5 > base_end) {
            return null;
        }
        let name = data[22..(22 + name_len)];
        const crc16 = readU16Le(data, 22 + name_len);
        const os_id = data[24 + name_len];
        const first_ext_off = base_end - 2;
        const ext_end = scanExtChainEnd(data, first_ext_off, data.len) ?? return null;
        const ext_total_after_base = ext_end - base_end;
        if ((raw_comp_size as usize) < ext_total_after_base) {
            return null;
        }
        const actual_comp_sz = (raw_comp_size as usize) - ext_total_after_base;
        if (actual_comp_sz > data.len - ext_end) {
            return null;
        }

        const ext_bytes = data[first_ext_off..ext_end];
        let dir_name = data[0..0];
        let eit = LzhExtIter.init(ext_bytes);
        while (true) {
            if (eh := eit.next()) {
                if (eh.ext_type == EXT_FILENAME && eh.data.len > 0) {
                    name = eh.data;
                } else if (eh.ext_type == EXT_DIRECTORY) {
                    dir_name = eh.data;
                }
            } else {
                break;
            }
        }

        return LzhEntry{
            .level = level,
            .method = method,
            .compressed_size = actual_comp_sz as u32,
            .original_size = original_size,
            .mtime_raw = mtime_raw,
            .crc16 = crc16,
            .os_id = os_id,
            .name = name,
            .dir_name = dir_name,
            .ext_headers_bytes = ext_bytes,
            .data = data[ext_end..(ext_end + actual_comp_sz)],
            .consumed = ext_end + actual_comp_sz,
        };
    }

    // Level 2
    const total_hdr_size = readU16Le(data, 0) as usize;
    if (total_hdr_size < 26 || total_hdr_size > data.len) {
        return null;
    }
    const crc16 = readU16Le(data, 21);
    const os_id = data[23];
    const ext_end = scanExtChainEnd(data, 24, total_hdr_size) ?? return null;
    const ext_bytes = data[24..ext_end];

    // Scan extended headers to extract metadata & verify EXT_COMMON_CRC if present
    let name = data[0..0];
    let dir_name = data[0..0];
    let cur_size = readU16Le(data, 24) as usize;
    let cur_pos: usize = 26;
    while (cur_size != 0) {
        const sz = cur_size;
        const ext_type = data[cur_pos];
        const payload = data[(cur_pos + 1)..(cur_pos + sz - 2)];
        if (ext_type == EXT_FILENAME) {
            name = payload;
        } else if (ext_type == EXT_DIRECTORY) {
            dir_name = payload;
        } else if (ext_type == EXT_COMMON_CRC && payload.len >= 2) {
            const expected_hcrc = readU16Le(payload, 0);
            const crc_field_off = cur_pos + 1;
            const zeros: [2]u8 = [0, 0];
            let calc = lzhCrc16Update(0, data[0..crc_field_off]);
            calc = lzhCrc16Update(calc, zeros[0..]);
            calc = lzhCrc16Update(calc, data[(crc_field_off + 2)..total_hdr_size]);
            if (calc != expected_hcrc) {
                return null;
            }
        }
        cur_size = readU16Le(data, cur_pos + sz - 2) as usize;
        cur_pos += sz;
    }

    const comp_sz = raw_comp_size as usize;
    if (comp_sz > data.len - total_hdr_size) {
        return null;
    }

    return LzhEntry{
        .level = level,
        .method = method,
        .compressed_size = raw_comp_size,
        .original_size = original_size,
        .mtime_raw = mtime_raw,
        .crc16 = crc16,
        .os_id = os_id,
        .name = name,
        .dir_name = dir_name,
        .ext_headers_bytes = ext_bytes,
        .data = data[total_hdr_size..(total_hdr_size + comp_sz)],
        .consumed = total_hdr_size + comp_sz,
    };
}

/// Zero-alloc sequential iterator over all entries in an LZH archive until the trailing `0x00` sentinel or EOF.
pub struct LzhEntryIter {
    data: []const u8,
    pos: usize,
    done: bool,
    has_err: bool,

    pub fn init(data: []const u8): LzhEntryIter {
        return LzhEntryIter{
            .data = data,
            .pos = 0,
            .done = false,
            .has_err = false,
        };
    }

    fn hasError(): bool {
        return _.has_err;
    }

    fn next(*_): LzhEntry? {
        if (_.done || _.has_err || _.pos >= _.data.len) {
            return null;
        }
        // Standard LZH end-of-archive sentinel: single 0x00 byte (or 0x0000 for Level 2)
        if (_.data[_.pos] == 0) {
            _.done = true;
            return null;
        }
        if (entry := parseLzhEntry(_.data[_.pos.._.data.len])) {
            _.pos += entry.consumed;
            return entry;
        }
        _.has_err = true;
        _.done = true;
        return null;
    }
}

/// Creates a zero-alloc sequential iterator over entries in an LZH archive.
pub fn iterLzhEntries(data: []const u8): LzhEntryIter {
    return LzhEntryIter.init(data);
}
