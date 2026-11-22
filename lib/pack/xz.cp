// Origin: Pure-CP Stdlib (Zero-Alloc XZ v1.1.0 Container Format Parser) | Created: 2026-11-22

pub const XZ_HEADER_MAGIC: [6]u8 = [0xFD, '7', 'z', 'X', 'Z', 0x00];
pub const XZ_FOOTER_MAGIC: [2]u8 = ['Y', 'Z'];
pub const XZ_STREAM_HEADER_SIZE: usize = 12;
pub const XZ_STREAM_FOOTER_SIZE: usize = 12;

// Check Types
pub const CHECK_NONE: u8 = 0x00;
pub const CHECK_CRC32: u8 = 0x01;
pub const CHECK_CRC64: u8 = 0x04;
pub const CHECK_SHA256: u8 = 0x0A;

// Filter IDs
pub const FILTER_DELTA: u64 = 0x03;
pub const FILTER_X86: u64 = 0x04;
pub const FILTER_POWERPC: u64 = 0x05;
pub const FILTER_IA64: u64 = 0x06;
pub const FILTER_ARM: u64 = 0x07;
pub const FILTER_ARMTHUMB: u64 = 0x08;
pub const FILTER_SPARC: u64 = 0x09;
pub const FILTER_ARM64: u64 = 0x0A;
pub const FILTER_RISCV: u64 = 0x0B;
pub const FILTER_LZMA2: u64 = 0x21;

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

/// Computes IEEE 802.3 CRC32 (reflected polynomial 0xEDB88320, init 0xFFFFFFFF, final XOR).
pub fn xzCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc ^= data[i] as u32;
        let bit: u8 = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return ~crc;
}

/// Result of decoding a 7-bit little-endian XZ Variable-Length Integer (1..9 bytes, 63-bit max).
pub struct XzVliResult {
    value: u64,
    consumed: usize,
}

/// Decodes an XZ 7-bit VLI (at most 9 bytes / 63 bits). Returns `null` on truncation or > 9 bytes.
pub fn decodeXzVli(data: []const u8): XzVliResult? {
    if (data.len == 0) {
        return null;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < 9) {
        if (i >= data.len) {
            return null;
        }
        const b = data[i];
        val |= ((b & 0x7F) as u64) << ((i * 7) as u6);
        if ((b & 0x80) == 0) {
            return XzVliResult {
                value: val,
                consumed: i + 1,
            };
        }
        i += 1;
    }
    return null;
}

/// Encodes a 63-bit unsigned integer into XZ 7-bit VLI format.
/// Returns number of bytes written (1..9), or `null` if `val > 0x7FFFFFFFFFFFFFFF` or `out` is too small.
pub fn encodeXzVli(val: u64, out: *[]u8): usize? {
    if (val > 0x7FFFFFFFFFFFFFFF) {
        return null;
    }
    let rem = val;
    let i: usize = 0;
    while (true) {
        if (i >= out.*.len) {
            return null;
        }
        if (rem >= 0x80) {
            out.*[i] = ((rem & 0x7F) as u8) | 0x80;
            rem >>= 7;
            i += 1;
        } else {
            out.*[i] = (rem & 0x7F) as u8;
            return i + 1;
        }
    }
}

/// Parsed 12-byte XZ Stream Header.
pub struct XzStreamHeader {
    check_type: u8,
    flags_raw: [2]u8,
    crc32: u32,

    /// Returns the check digest size in bytes (0, 4, 8, or 32).
    fn checkSize(): usize {
        if (_.check_type == CHECK_NONE) {
            return 0;
        }
        if (_.check_type == CHECK_CRC32) {
            return 4;
        }
        if (_.check_type == CHECK_CRC64) {
            return 8;
        }
        if (_.check_type == CHECK_SHA256) {
            return 32;
        }
        return 0;
    }

    /// Returns the human-readable check algorithm name.
    fn checkName(): []const u8 {
        if (_.check_type == CHECK_NONE) {
            return "none";
        }
        if (_.check_type == CHECK_CRC32) {
            return "crc32";
        }
        if (_.check_type == CHECK_CRC64) {
            return "crc64";
        }
        if (_.check_type == CHECK_SHA256) {
            return "sha256";
        }
        return "unknown";
    }
}

/// Parsed 12-byte XZ Stream Footer.
pub struct XzStreamFooter {
    crc32: u32,
    backward_size: u64,
    check_type: u8,
    flags_raw: [2]u8,
}

/// Decoded XZ Block Filter entry with zero-copy property slice.
pub struct XzFilter {
    filter_id: u64,
    props: []const u8,

    /// Decodes the LZMA2 dictionary size when `filter_id == FILTER_LZMA2 && props.len == 1`.
    fn lzma2DictSize(): u64? {
        if (_.filter_id != FILTER_LZMA2 || _.props.len != 1) {
            return null;
        }
        const p = _.props[0];
        if ((p & 0xC0) != 0) {
            return null;
        }
        const bits = p & 0x3F;
        if (bits > 40) {
            return null;
        }
        if (bits == 40) {
            return 0xFFFFFFFF;
        }
        const base: u64 = (2 as u64) | ((bits & 1) as u64);
        const shift: u6 = ((bits / 2) + 11) as u6;
        return base << shift;
    }
}

/// Parsed XZ Block Header (8..1024 bytes, 1..4 filters).
pub struct XzBlockHeader {
    header_size: usize,
    filter_count: u8,
    has_compressed_size: bool,
    has_uncompressed_size: bool,
    compressed_size: u64,
    uncompressed_size: u64,
    filters: [4]XzFilter,
}

/// Decoded XZ Index record `(unpadded_size, uncompressed_size)`.
pub struct XzIndexRecord {
    unpadded_size: u64,
    uncompressed_size: u64,
}

/// Zero-copy iterator over XZ Index records.
pub struct XzIndexRecordIter {
    data: []const u8,
    pos: usize,
    remaining: u64,

    fn next(*_): XzIndexRecord? {
        if (_.remaining == 0 || _.pos >= _.data.len) {
            return null;
        }
        if (r1 := decodeXzVli(_.data[_.pos..])) {
            _.pos += r1.consumed;
            if (r2 := decodeXzVli(_.data[_.pos..])) {
                _.pos += r2.consumed;
                _.remaining -= 1;
                return XzIndexRecord {
                    unpadded_size: r1.value,
                    uncompressed_size: r2.value,
                };
            }
        }
        _.remaining = 0;
        _.pos = _.data.len;
        return null;
    }
}

/// Parsed XZ Index section with zero-copy record slice.
pub struct XzIndex {
    record_count: u64,
    records_bytes: []const u8,
    total_size: usize,

    fn records(): XzIndexRecordIter {
        return XzIndexRecordIter {
            data: _.records_bytes,
            pos: 0,
            remaining: _.record_count,
        };
    }
}

/// Fast magic and length check for an XZ stream header (`0xFD "7zXZ" 0x00`).
pub fn isXz(data: []const u8): bool {
    if (data.len < XZ_STREAM_HEADER_SIZE) {
        return false;
    }
    let i: usize = 0;
    while (i < 6) {
        if (data[i] != XZ_HEADER_MAGIC[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parses and validates a 12-byte XZ Stream Header (magic, flags, and CRC32).
pub fn parseXzHeader(data: []const u8): XzStreamHeader? {
    if (!isXz(data)) {
        return null;
    }
    const f0 = data[6];
    const f1 = data[7];
    if (f0 != 0 || (f1 & 0xF0) != 0) {
        return null;
    }
    const expected_crc = readU32Le(data, 8);
    const actual_crc = xzCrc32(data[6..8]);
    if (actual_crc != expected_crc) {
        return null;
    }
    let flags: [2]u8 = undefined;
    flags[0] = f0;
    flags[1] = f1;
    return XzStreamHeader {
        check_type: f1 & 0x0F,
        flags_raw: flags,
        crc32: expected_crc,
    };
}

/// Parses and validates a 12-byte XZ Stream Footer (`"YZ"` magic, flags, backward_size, and CRC32).
pub fn parseXzFooter(data: []const u8): XzStreamFooter? {
    if (data.len < XZ_STREAM_FOOTER_SIZE) {
        return null;
    }
    if (data[10] != XZ_FOOTER_MAGIC[0] || data[11] != XZ_FOOTER_MAGIC[1]) {
        return null;
    }
    const f0 = data[8];
    const f1 = data[9];
    if (f0 != 0 || (f1 & 0xF0) != 0) {
        return null;
    }
    const expected_crc = readU32Le(data, 0);
    const actual_crc = xzCrc32(data[4..10]);
    if (actual_crc != expected_crc) {
        return null;
    }
    const raw_backward = readU32Le(data, 4) as u64;
    let flags: [2]u8 = undefined;
    flags[0] = f0;
    flags[1] = f1;
    return XzStreamFooter {
        crc32: expected_crc,
        backward_size: (raw_backward + 1) * 4,
        check_type: f1 & 0x0F,
        flags_raw: flags,
    };
}

/// Parses and validates an XZ Block Header (size byte, flags, optional sizes, 1..4 filters, zero padding, and CRC32).
pub fn parseXzBlockHeader(data: []const u8): XzBlockHeader? {
    if (data.len < 8) {
        return null;
    }
    const encoded_size = data[0];
    if (encoded_size == 0) {
        return null;
    }
    const header_size = ((encoded_size as usize) + 1) * 4;
    if (data.len < header_size) {
        return null;
    }

    const body_end = header_size - 4;
    const expected_crc = readU32Le(data, body_end);
    if (xzCrc32(data[0..body_end]) != expected_crc) {
        return null;
    }

    const block_flags = data[1];
    if ((block_flags & 0x3C) != 0) {
        return null;
    }
    const filter_count: u8 = (block_flags & 0x03) + 1;
    const has_compressed_size = (block_flags & 0x40) != 0;
    const has_uncompressed_size = (block_flags & 0x80) != 0;

    let pos: usize = 2;
    let compressed_size: u64 = 0;
    let uncompressed_size: u64 = 0;

    if (has_compressed_size) {
        if (r := decodeXzVli(data[pos..body_end])) {
            compressed_size = r.value;
            pos += r.consumed;
        } else {
            return null;
        }
    }
    if (has_uncompressed_size) {
        if (r := decodeXzVli(data[pos..body_end])) {
            uncompressed_size = r.value;
            pos += r.consumed;
        } else {
            return null;
        }
    }

    let filters: [4]XzFilter = undefined;
    let init_i: usize = 0;
    while (init_i < 4) {
        filters[init_i] = XzFilter {
            filter_id: 0,
            props: data[0..0],
        };
        init_i += 1;
    }

    let f: usize = 0;
    while (f < (filter_count as usize)) {
        let fid: u64 = 0;
        if (r_id := decodeXzVli(data[pos..body_end])) {
            fid = r_id.value;
            pos += r_id.consumed;
        } else {
            return null;
        }
        let plen: usize = 0;
        if (r_pl := decodeXzVli(data[pos..body_end])) {
            if (r_pl.value > (body_end - pos) as u64) {
                return null;
            }
            plen = r_pl.value as usize;
            pos += r_pl.consumed;
        } else {
            return null;
        }
        if (plen > body_end - pos) {
            return null;
        }
        filters[f] = XzFilter {
            filter_id: fid,
            props: data[pos .. pos + plen],
        };
        pos += plen;
        f += 1;
    }

    // Header padding bytes must all be 0x00
    while (pos < body_end) {
        if (data[pos] != 0) {
            return null;
        }
        pos += 1;
    }

    return XzBlockHeader {
        header_size: header_size,
        filter_count: filter_count,
        has_compressed_size: has_compressed_size,
        has_uncompressed_size: has_uncompressed_size,
        compressed_size: compressed_size,
        uncompressed_size: uncompressed_size,
        filters: filters,
    };
}

/// Parses and validates an XZ Index section (`0x00` indicator, VLI record count, records, 4B zero padding, and CRC32).
pub fn parseXzIndex(data: []const u8): XzIndex? {
    if (data.len < 8 || data[0] != 0x00) {
        return null;
    }

    let pos: usize = 1;
    let record_count: u64 = 0;
    if (r_cnt := decodeXzVli(data[pos..])) {
        record_count = r_cnt.value;
        pos += r_cnt.consumed;
    } else {
        return null;
    }

    const records_start = pos;
    let i: u64 = 0;
    while (i < record_count) {
        if (r1 := decodeXzVli(data[pos..])) {
            if (r1.value == 0) {
                return null;
            }
            pos += r1.consumed;
        } else {
            return null;
        }
        if (r2 := decodeXzVli(data[pos..])) {
            pos += r2.consumed;
        } else {
            return null;
        }
        i += 1;
    }
    const records_end = pos;

    const pad = (4 - (pos % 4)) % 4;
    if (data.len - pos < pad + 4) {
        return null;
    }
    let p: usize = 0;
    while (p < pad) {
        if (data[pos + p] != 0) {
            return null;
        }
        p += 1;
    }

    const crc_off = pos + pad;
    const expected_crc = readU32Le(data, crc_off);
    if (xzCrc32(data[0..crc_off]) != expected_crc) {
        return null;
    }

    return XzIndex {
        record_count: record_count,
        records_bytes: data[records_start..records_end],
        total_size: crc_off + 4,
    };
}
