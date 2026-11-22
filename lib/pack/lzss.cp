// Origin: Pure-CP Stdlib (Zero-Alloc LZSS Sliding-Window Compression & Decompression Codec) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// LZSS Protocol Constants (12-bit / 4KB Window, 3..18 Match Length)
// ---------------------------------------------------------------------------

pub const LZSS_WINDOW_SIZE: usize = 4096;
pub const LZSS_MIN_MATCH: usize = 3;
pub const LZSS_MAX_MATCH: usize = 18;
pub const LZSS_FRAME_HEADER_SIZE: usize = 4;

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn bitMask(bit: usize): u8 {
    if (bit == 0) { return 0x01; }
    if (bit == 1) { return 0x02; }
    if (bit == 2) { return 0x04; }
    if (bit == 3) { return 0x08; }
    if (bit == 4) { return 0x10; }
    if (bit == 5) { return 0x20; }
    if (bit == 6) { return 0x40; }
    return 0x80;
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

// ---------------------------------------------------------------------------
// Core Zero-Allocation LZSS Codec
// ---------------------------------------------------------------------------

/// Returns the worst-case compressed byte size for an input of `src_len` bytes
/// (`src_len + ceil(src_len / 8)` when all bytes are emitted as literals).
pub fn lzssMaxCompressedSize(src_len: usize): usize {
    if (src_len == 0) {
        return 0;
    }
    return src_len + (src_len + 7) / 8;
}

/// Compresses `src` into `dst` using classic 12-bit/4KB LZSS with LSB-first flag bytes.
/// Bit=1 denotes a 1-byte literal; Bit=0 denotes a 2-byte match token
/// (`dist_minus_1` in low 12 bits, `len_minus_3` in high 4 bits).
/// Supports overlapping RLE matches (`dist < len`). Returns compressed byte count or `null` on overflow.
pub fn lzssCompress(src: []const u8, dst: []u8): usize? {
    if (src.len == 0) {
        return 0;
    }
    let in_pos: usize = 0;
    let out_pos: usize = 0;

    while (in_pos < src.len) {
        if (out_pos >= dst.len) {
            return null;
        }
        const flag_pos = out_pos;
        out_pos += 1;
        let flags: u8 = 0;

        let bit: usize = 0;
        while (bit < 8 && in_pos < src.len) {
            const win_start: usize = if (in_pos > LZSS_WINDOW_SIZE) in_pos - LZSS_WINDOW_SIZE else 0;
            const rem = src.len - in_pos;
            const max_len: usize = if (rem < LZSS_MAX_MATCH) rem else LZSS_MAX_MATCH;

            let best_len: usize = 0;
            let best_dist: usize = 0;

            if (max_len >= LZSS_MIN_MATCH) {
                let cand: usize = in_pos;
                while (cand > win_start) {
                    cand -= 1;
                    if (src[cand] != src[in_pos]) {
                        continue;
                    }
                    let m: usize = 1;
                    while (m < max_len && src[cand + m] == src[in_pos + m]) {
                        m += 1;
                    }
                    if (m > best_len) {
                        best_len = m;
                        best_dist = in_pos - cand;
                        if (best_len == max_len) {
                            break;
                        }
                    }
                }
            }

            if (best_len >= LZSS_MIN_MATCH) {
                if (out_pos + 2 > dst.len) {
                    return null;
                }
                const dist_m1: usize = best_dist - 1;
                const len_m3: usize = best_len - LZSS_MIN_MATCH;
                dst[out_pos] = (dist_m1 & 0xFF) as u8;
                dst[out_pos + 1] = (((dist_m1 >> 8) & 0x0F) | ((len_m3 & 0x0F) << 4)) as u8;
                out_pos += 2;
                in_pos += best_len;
            } else {
                if (out_pos >= dst.len) {
                    return null;
                }
                dst[out_pos] = src[in_pos];
                out_pos += 1;
                in_pos += 1;
                flags = flags | bitMask(bit);
            }

            bit += 1;
        }

        dst[flag_pos] = flags;
    }

    return out_pos;
}

/// Decompresses an LZSS stream `src` into `dst`.
/// Supports overlapping RLE byte-by-byte copies and strictly rejects `dist > out_pos`,
/// truncated match tokens, dangling flag bytes, and destination buffer overflows.
pub fn lzssDecompress(src: []const u8, dst: []u8): usize? {
    if (src.len == 0) {
        return 0;
    }
    let in_pos: usize = 0;
    let out_pos: usize = 0;

    while (in_pos < src.len) {
        const flags = src[in_pos];
        in_pos += 1;
        if (in_pos >= src.len) {
            // Dangling flag byte with no following token
            return null;
        }

        let bit: usize = 0;
        while (bit < 8 && in_pos < src.len) {
            if ((flags & bitMask(bit)) != 0) {
                if (out_pos >= dst.len) {
                    return null;
                }
                dst[out_pos] = src[in_pos];
                out_pos += 1;
                in_pos += 1;
            } else {
                if (in_pos + 2 > src.len) {
                    return null;
                }
                const b0 = src[in_pos] as usize;
                const b1 = src[in_pos + 1] as usize;
                in_pos += 2;

                const dist: usize = (b0 | ((b1 & 0x0F) << 8)) + 1;
                const len: usize = ((b1 >> 4) & 0x0F) + LZSS_MIN_MATCH;

                if (dist > out_pos) {
                    return null;
                }
                if (len > dst.len - out_pos) {
                    return null;
                }

                let k: usize = 0;
                while (k < len) {
                    dst[out_pos] = dst[out_pos - dist];
                    out_pos += 1;
                    k += 1;
                }
            }
            bit += 1;
        }
    }

    return out_pos;
}

/// Compresses `src` into `dst` prefixed with a 4-byte little-endian uncompressed length header.
pub fn lzssCompressFramed(src: []const u8, dst: []u8): usize? {
    if (dst.len < LZSS_FRAME_HEADER_SIZE || (src.len as u64) > 0xFFFFFFFF) {
        return null;
    }
    writeU32Le(dst, 0, src.len as u32);
    if (n := lzssCompress(src, dst[LZSS_FRAME_HEADER_SIZE..dst.len])) {
        return LZSS_FRAME_HEADER_SIZE + n;
    }
    return null;
}

/// Decompresses a framed LZSS stream (4-byte LE uncompressed length + payload) into `dst`,
/// verifying both target buffer capacity and exact decompressed byte count.
pub fn lzssDecompressFramed(src: []const u8, dst: []u8): usize? {
    if (src.len < LZSS_FRAME_HEADER_SIZE) {
        return null;
    }
    const orig_len = readU32Le(src, 0) as usize;
    if (orig_len > dst.len) {
        return null;
    }
    if (n := lzssDecompress(src[LZSS_FRAME_HEADER_SIZE..src.len], dst[0..orig_len])) {
        if (n != orig_len) {
            return null;
        }
        return n;
    }
    return null;
}
