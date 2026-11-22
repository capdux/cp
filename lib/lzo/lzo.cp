// Origin: Pure-CP Stdlib (Zero-Alloc lzop Container & LZO1X Decompressor) | Created: 2026-11-22

pub const LZOP_MAGIC: [9]u8 = [0x89, 'L', 'Z', 'O', 0x00, 0x0D, 0x0A, 0x1A, 0x0A];

// Compression Methods
pub const METHOD_LZO1X_1: u8 = 1;
pub const METHOD_LZO1X_1_15: u8 = 2;
pub const METHOD_LZO1X_999: u8 = 3;

// Header Flags
pub const F_ADLER32_D: u32 = 0x00000001;
pub const F_ADLER32_C: u32 = 0x00000002;
pub const F_STDIN: u32 = 0x00000004;
pub const F_STDOUT: u32 = 0x00000008;
pub const F_EXTRA_FIELD: u32 = 0x00000040;
pub const F_CRC32_D: u32 = 0x00000100;
pub const F_CRC32_C: u32 = 0x00000200;
pub const F_H_FILTER: u32 = 0x00000800;
pub const F_H_CRC32: u32 = 0x00001000;

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

/// Computes RFC 1950 / LZO Adler-32 checksum.
pub fn lzoAdler32(data: []const u8): u32 {
    let s1: u32 = 1;
    let s2: u32 = 0;
    let i: usize = 0;
    while (i < data.len) {
        s1 = (s1 + (data[i] as u32)) % 65521;
        s2 = (s2 + s1) % 65521;
        i += 1;
    }
    return (s2 << 16) | s1;
}

/// Computes IEEE 802.3 CRC32 (reflected polynomial 0xEDB88320, init 0xFFFFFFFF, final XOR).
pub fn lzoCrc32(data: []const u8): u32 {
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

/// Parsed `lzop` container header.
pub struct LzopHeader {
    version: u16,
    lib_version: u16,
    version_needed: u16,
    method: u8,
    level: u8,
    flags: u32,
    mode: u32,
    mtime_low: u32,
    mtime_high: u32,
    filename: []const u8,
    header_checksum: u32,
    blocks_offset: usize,

    fn usesHeaderCrc32(): bool {
        return (_.flags & F_H_CRC32) != 0;
    }

    fn hasUncompressedAdler32(): bool {
        return (_.flags & F_ADLER32_D) != 0;
    }

    fn hasUncompressedCrc32(): bool {
        return (_.flags & F_CRC32_D) != 0;
    }

    fn hasCompressedAdler32(): bool {
        return (_.flags & F_ADLER32_C) != 0;
    }

    fn hasCompressedCrc32(): bool {
        return (_.flags & F_CRC32_C) != 0;
    }
}

/// Decoded `lzop` data block with zero-copy payload slice.
pub struct LzopBlock {
    uncompressed_size: u32,
    compressed_size: u32,
    d_adler32: u32,
    d_crc32: u32,
    c_adler32: u32,
    c_crc32: u32,
    payload: []const u8,

    /// Returns true when the block is stored uncompressed (`compressed_size == uncompressed_size`).
    fn isUncompressed(): bool {
        return _.compressed_size == _.uncompressed_size;
    }
}

/// Zero-copy iterator over `lzop` data blocks until the `0x00000000` EOF sentinel.
pub struct LzopBlockIter {
    data: []const u8,
    pos: usize,
    flags: u32,
    done: bool,
    err: bool,

    fn hasError(): bool {
        return _.err;
    }

    fn next(*_): LzopBlock? {
        if (_.done) {
            return null;
        }
        if (_.data.len - _.pos < 4) {
            _.done = true;
            _.err = true;
            return null;
        }
        const uncomp_size = readU32Be(_.data, _.pos);
        _.pos += 4;
        if (uncomp_size == 0) {
            _.done = true;
            return null;
        }

        if (_.data.len - _.pos < 4) {
            _.done = true;
            _.err = true;
            return null;
        }
        const comp_size = readU32Be(_.data, _.pos);
        _.pos += 4;
        if (comp_size == 0 || comp_size > uncomp_size) {
            _.done = true;
            _.err = true;
            return null;
        }

        let d_adler32: u32 = 0;
        let d_crc32: u32 = 0;
        let c_adler32: u32 = 0;
        let c_crc32: u32 = 0;

        if ((_.flags & F_ADLER32_D) != 0) {
            if (_.data.len - _.pos < 4) {
                _.done = true;
                _.err = true;
                return null;
            }
            d_adler32 = readU32Be(_.data, _.pos);
            _.pos += 4;
        }
        if ((_.flags & F_CRC32_D) != 0) {
            if (_.data.len - _.pos < 4) {
                _.done = true;
                _.err = true;
                return null;
            }
            d_crc32 = readU32Be(_.data, _.pos);
            _.pos += 4;
        }

        const is_compressed = comp_size < uncomp_size;
        if (is_compressed) {
            if ((_.flags & F_ADLER32_C) != 0) {
                if (_.data.len - _.pos < 4) {
                    _.done = true;
                    _.err = true;
                    return null;
                }
                c_adler32 = readU32Be(_.data, _.pos);
                _.pos += 4;
            }
            if ((_.flags & F_CRC32_C) != 0) {
                if (_.data.len - _.pos < 4) {
                    _.done = true;
                    _.err = true;
                    return null;
                }
                c_crc32 = readU32Be(_.data, _.pos);
                _.pos += 4;
            }
        }

        const plen = comp_size as usize;
        if (_.data.len - _.pos < plen) {
            _.done = true;
            _.err = true;
            return null;
        }
        const payload = _.data[_.pos .. _.pos + plen];
        _.pos += plen;

        if (is_compressed) {
            if ((_.flags & F_ADLER32_C) != 0 && lzoAdler32(payload) != c_adler32) {
                _.done = true;
                _.err = true;
                return null;
            }
            if ((_.flags & F_CRC32_C) != 0 && lzoCrc32(payload) != c_crc32) {
                _.done = true;
                _.err = true;
                return null;
            }
        }

        return LzopBlock {
            uncompressed_size: uncomp_size,
            compressed_size: comp_size,
            d_adler32: d_adler32,
            d_crc32: d_crc32,
            c_adler32: c_adler32,
            c_crc32: c_crc32,
            payload: payload,
        };
    }
}

/// Fast 9-byte `lzop` magic check.
pub fn isLzop(data: []const u8): bool {
    if (data.len < 9) {
        return false;
    }
    let i: usize = 0;
    while (i < 9) {
        if (data[i] != LZOP_MAGIC[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parses and validates an `lzop` container header, verifying the header checksum
/// over `[9..cksum_pos]` using CRC32 when `F_H_CRC32` is set or Adler32 otherwise.
pub fn parseLzopHeader(data: []const u8): LzopHeader? {
    if (!isLzop(data)) {
        return null;
    }
    let pos: usize = 9;
    if (data.len - pos < 4) {
        return null;
    }
    const version = readU16Be(data, pos);
    pos += 2;
    const lib_version = readU16Be(data, pos);
    pos += 2;

    let version_needed: u16 = 0;
    let method: u8 = 0;
    let level: u8 = 0;

    if (version >= 0x0940) {
        if (data.len - pos < 4) {
            return null;
        }
        version_needed = readU16Be(data, pos);
        pos += 2;
        method = data[pos];
        pos += 1;
        level = data[pos];
        pos += 1;
    } else {
        if (data.len - pos < 1) {
            return null;
        }
        method = data[pos];
        pos += 1;
    }

    if (data.len - pos < 4) {
        return null;
    }
    const flags = readU32Be(data, pos);
    pos += 4;

    if ((flags & F_H_FILTER) != 0) {
        if (data.len - pos < 4) {
            return null;
        }
        pos += 4;
    }

    if (data.len - pos < 8) {
        return null;
    }
    const mode = readU32Be(data, pos);
    pos += 4;
    const mtime_low = readU32Be(data, pos);
    pos += 4;

    let mtime_high: u32 = 0;
    if (version >= 0x0940) {
        if (data.len - pos < 4) {
            return null;
        }
        mtime_high = readU32Be(data, pos);
        pos += 4;
    }

    if (data.len - pos < 1) {
        return null;
    }
    const name_len = data[pos] as usize;
    pos += 1;
    if (data.len - pos < name_len + 4) {
        return null;
    }
    const filename = data[pos .. pos + name_len];
    pos += name_len;

    const cksum_pos = pos;
    const header_checksum = readU32Be(data, cksum_pos);
    pos += 4;

    if ((flags & F_H_CRC32) != 0) {
        if (lzoCrc32(data[9..cksum_pos]) != header_checksum) {
            return null;
        }
    } else {
        if (lzoAdler32(data[9..cksum_pos]) != header_checksum) {
            return null;
        }
    }

    if ((flags & F_EXTRA_FIELD) != 0) {
        if (data.len - pos < 4) {
            return null;
        }
        const extra_len = readU32Be(data, pos) as usize;
        pos += 4;
        if (data.len - pos < extra_len + 4) {
            return null;
        }
        pos += extra_len + 4;
    }

    return LzopHeader {
        version: version,
        lib_version: lib_version,
        version_needed: version_needed,
        method: method,
        level: level,
        flags: flags,
        mode: mode,
        mtime_low: mtime_low,
        mtime_high: mtime_high,
        filename: filename,
        header_checksum: header_checksum,
        blocks_offset: pos,
    };
}

/// Returns a zero-copy block iterator starting at `header.blocks_offset`.
pub fn iterLzopBlocks(header: LzopHeader, data: []const u8): LzopBlockIter {
    let start = header.blocks_offset;
    if (start > data.len) {
        start = data.len;
    }
    return LzopBlockIter {
        data: data,
        pos: start,
        flags: header.flags,
        done: false,
        err: false,
    };
}

fn lzoReadExtLen(src: []const u8, ip: *usize): usize? {
    let acc: usize = 0;
    while (true) {
        if (ip.* >= src.len) {
            return null;
        }
        const b = src[ip.*];
        ip.* += 1;
        if (b == 0) {
            if (acc > 0x10000000) {
                return null;
            }
            acc += 255;
        } else {
            return acc + (b as usize);
        }
    }
}

fn lzoCopyLiterals(src: []const u8, ip: *usize, dst: *[]u8, op: *usize, count: usize): bool {
    if (src.len - ip.* < count || dst.*.len - op.* < count) {
        return false;
    }
    let i: usize = 0;
    while (i < count) {
        dst.*[op.* + i] = src[ip.* + i];
        i += 1;
    }
    ip.* += count;
    op.* += count;
    return true;
}

fn lzoCopyMatch(dst: *[]u8, op: *usize, dist: usize, length: usize): bool {
    if (dist == 0 || dist > op.* || dst.*.len - op.* < length) {
        return false;
    }
    let src_i = op.* - dist;
    let dst_i = op.*;
    let rem = length;
    while (rem > 0) {
        dst.*[dst_i] = dst.*[src_i];
        src_i += 1;
        dst_i += 1;
        rem -= 1;
    }
    op.* = dst_i;
    return true;
}

/// Pure CP zero-allocation LZO1X safe decompressor.
/// Supports literal runs, M1/M2/M3/M4 sliding-window back-references (including overlapping RLE copies),
/// EOS sentinel (`0x11 0x00 0x00`), and strict bounds checking on input, output, and match distance.
pub fn lzo1xDecompress(src: []const u8, dst: *[]u8): usize? {
    if (src.len == 0) {
        return null;
    }

    let ip: usize = 0;
    let op: usize = 0;
    let in_match = false;
    let t: u8 = 0;

    // Initial byte handling (> 17 means initial literal run of t - 17 bytes)
    if (src[0] > 17) {
        const init_lit = (src[0] - 17) as usize;
        ip = 1;
        if (!lzoCopyLiterals(src, *ip, dst, *op, init_lit)) {
            return null;
        }
        if (ip >= src.len) {
            return null;
        }
        t = src[ip];
        ip += 1;
        if (init_lit < 4) {
            in_match = true;
        } else if (t < 16) {
            // M1 match following >= 4 literal bytes: len = 3, dist = 1 + 0x0800 + (t >> 2) + (next << 2)
            if (ip >= src.len) {
                return null;
            }
            const next_b = src[ip];
            ip += 1;
            const dist: usize = ((t >> 2) as usize) + ((next_b as usize) << 2) + 0x0801;
            if (!lzoCopyMatch(dst, *op, dist, 3)) {
                return null;
            }
            const trail = (t & 3) as usize;
            if (trail > 0) {
                if (!lzoCopyLiterals(src, *ip, dst, *op, trail)) {
                    return null;
                }
                if (ip >= src.len) {
                    return null;
                }
                t = src[ip];
                ip += 1;
                in_match = true;
            }
        } else {
            in_match = true;
        }
    }

    while (true) {
        if (!in_match) {
            if (ip >= src.len) {
                return null;
            }
            t = src[ip];
            ip += 1;
            if (t < 16) {
                let lit_len: usize = 0;
                if (t == 0) {
                    if (ext := lzoReadExtLen(src, *ip)) {
                        lit_len = ext + 18;
                    } else {
                        return null;
                    }
                } else {
                    lit_len = (t as usize) + 3;
                }
                if (!lzoCopyLiterals(src, *ip, dst, *op, lit_len)) {
                    return null;
                }
                if (ip >= src.len) {
                    return null;
                }
                t = src[ip];
                ip += 1;
                if (t < 16) {
                    // M1 match following literal run: len = 3, dist = 1 + 0x0800 + (t >> 2) + (next << 2)
                    if (ip >= src.len) {
                        return null;
                    }
                    const next_b = src[ip];
                    ip += 1;
                    const dist: usize = ((t >> 2) as usize) + ((next_b as usize) << 2) + 0x0801;
                    if (!lzoCopyMatch(dst, *op, dist, 3)) {
                        return null;
                    }
                    const trail = (t & 3) as usize;
                    if (trail > 0) {
                        if (!lzoCopyLiterals(src, *ip, dst, *op, trail)) {
                            return null;
                        }
                        if (ip >= src.len) {
                            return null;
                        }
                        t = src[ip];
                        ip += 1;
                        in_match = true;
                    }
                    continue;
                }
            }
            in_match = true;
        }

        // Match dispatch (M2 / M3 / M4 / EOS / short M1)
        let dist: usize = 0;
        let mlen: usize = 0;
        let trail: usize = 0;

        if (t >= 64) {
            // M2 match
            if (ip >= src.len) {
                return null;
            }
            const next_b = src[ip];
            ip += 1;
            dist = (((t >> 2) & 7) as usize) + ((next_b as usize) << 3) + 1;
            mlen = ((t >> 5) as usize) + 1;
            trail = (t & 3) as usize;
        } else if (t >= 32) {
            // M3 match
            let lraw = (t & 31) as usize;
            if (lraw == 0) {
                if (ext := lzoReadExtLen(src, *ip)) {
                    lraw = ext + 31;
                } else {
                    return null;
                }
            }
            mlen = lraw + 2;
            if (src.len - ip < 2) {
                return null;
            }
            const lo = src[ip];
            const hi = src[ip + 1];
            ip += 2;
            dist = ((lo >> 2) as usize) + ((hi as usize) << 6) + 1;
            trail = (lo & 3) as usize;
        } else if (t >= 16) {
            // M4 match or EOS sentinel (0x11 0x00 0x00)
            const dist_hi = ((t & 8) as usize) << 11;
            let lraw = (t & 7) as usize;
            if (lraw == 0) {
                if (ext := lzoReadExtLen(src, *ip)) {
                    lraw = ext + 7;
                } else {
                    return null;
                }
            }
            mlen = lraw + 2;
            if (src.len - ip < 2) {
                return null;
            }
            const lo = src[ip];
            const hi = src[ip + 1];
            ip += 2;
            const d = dist_hi + ((lo >> 2) as usize) + ((hi as usize) << 6);
            if (d == 0) {
                // EOS sentinel requires lraw == 1 (mlen == 3)
                if (mlen != 3) {
                    return null;
                }
                return op;
            }
            dist = d + 0x4000;
            trail = (lo & 3) as usize;
        } else {
            // Short M1 match (after trailing literals): len = 2
            if (ip >= src.len) {
                return null;
            }
            const next_b = src[ip];
            ip += 1;
            dist = ((t >> 2) as usize) + ((next_b as usize) << 2) + 1;
            mlen = 2;
            trail = (t & 3) as usize;
        }

        if (!lzoCopyMatch(dst, *op, dist, mlen)) {
            return null;
        }

        if (trail == 0) {
            in_match = false;
        } else {
            if (!lzoCopyLiterals(src, *ip, dst, *op, trail)) {
                return null;
            }
            if (ip >= src.len) {
                return null;
            }
            t = src[ip];
            ip += 1;
            in_match = true;
        }
    }
}
