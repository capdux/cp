// @cap/lz4-block — Zero-allocation LZ4 Raw Block compressor & decompressor for CP
// Clean-room MIT implementation of the official LZ4 Block Format (Rust `lz4_flex` parity).
//
// Features:
// - 100% wire-compatible with standard LZ4 Raw Block format (token high4=lit, low4=match-4,
//   2-byte LE offset, last 5 bytes literals only, last 12 bytes MFLIMIT).
// - Fixed stack 4096-entry `[4096]u16` hash table (8KB stack, zero heap allocation).
// - Strict bounds checking on decompression with overlapping RLE match copy (`offset < match_len`).
// - Size-prefixed framing helpers `compressWithSize` / `decompressWithSize`.

const MIN_MATCH: usize = 4;
const LAST_LITERALS: usize = 5;
const MFLIMIT: usize = 12;
const MAX_DISTANCE: usize = 65535;
const HASH_SIZE: usize = 4096;

/// Returns the worst-case compressed output size in bytes for an input of `input_len` bytes.
pub fn maxCompressedLen(input_len: usize) -> usize {
    return input_len + (input_len / 255) + 16;
}

fn readU32LE(buf: []const u8, pos: usize) -> u32 {
    return (buf[pos] as u32) |
        ((buf[pos + 1] as u32) << 8) |
        ((buf[pos + 2] as u32) << 16) |
        ((buf[pos + 3] as u32) << 24);
}

/// Overflow-free 4-byte multiplicative hash into `0..4095`.
fn hash4(buf: []const u8, pos: usize) -> usize {
    const v: u64 = readU32LE(buf, pos) as u64;
    // max product: 0xFFFFFFFF * 2654435761 < u64::MAX (never overflows)
    const prod: u64 = v * (2654435761 as u64);
    return ((prod >> 20) & (0xFFF as u64)) as usize;
}

fn match4(buf: []const u8, a: usize, b: usize) -> bool {
    return buf[a] == buf[b] &&
        buf[a + 1] == buf[b + 1] &&
        buf[a + 2] == buf[b + 2] &&
        buf[a + 3] == buf[b + 3];
}

fn writeExtLen(dst: []u8, pos: usize, extra: usize) -> i64 {
    let p = pos;
    let rem = extra;
    while (rem >= 255) {
        if (p >= dst.len) {
            return -1;
        }
        dst[p] = 255;
        p += 1;
        rem -= 255;
    }
    if (p >= dst.len) {
        return -1;
    }
    dst[p] = rem as u8;
    p += 1;
    return p as i64;
}

/// Compresses `src` into standard LZ4 Raw Block format inside caller-provided `dst`.
/// Returns compressed byte length, or `-1` if `dst` is too small.
pub fn compressBlock(src: []const u8, dst: []u8) -> i64 {
    if (src.len == 0) {
        return 0;
    }

    let table: [HASH_SIZE]u16 = undefined;
    let ti: usize = 0;
    while (ti < HASH_SIZE) {
        table[ti] = 0;
        ti += 1;
    }

    let anchor: usize = 0;
    let out_pos: usize = 0;

    if (src.len >= MFLIMIT) {
        const mf_limit = src.len - MFLIMIT;
        const match_limit = src.len - LAST_LITERALS;
        // Seed position 0 in hash table and start scanning from position 1
        table[hash4(src, 0)] = 0;
        let pos: usize = 1;

        while (pos <= mf_limit) {
            const h = hash4(src, pos);
            const cand_low = table[h] as usize;
            table[h] = (pos & (0xFFFF as usize)) as u16;

            // Reconstruct candidate position in the same or preceding 64KB window
            const base_hi: usize = pos - (pos & (0xFFFF as usize));
            let cand: usize = base_hi + cand_low;
            if (cand >= pos) {
                if (cand >= 65536) {
                    cand -= 65536;
                } else {
                    pos += 1;
                    continue;
                }
            }

            const dist = pos - cand;
            if (dist == 0 || dist > MAX_DISTANCE || !match4(src, cand, pos)) {
                pos += 1;
                continue;
            }

            // Found a >= 4-byte match; extend up to match_limit
            const lit_len = pos - anchor;
            let m_end = pos + MIN_MATCH;
            let c_end = cand + MIN_MATCH;
            while (m_end < match_limit && src[m_end] == src[c_end]) {
                m_end += 1;
                c_end += 1;
            }
            const ml_code = (m_end - pos) - MIN_MATCH;

            // Emit token byte
            if (out_pos >= dst.len) {
                return -1;
            }
            let lit_nib: u8 = 15;
            if (lit_len < 15) {
                lit_nib = lit_len as u8;
            }
            let ml_nib: u8 = 15;
            if (ml_code < 15) {
                ml_nib = ml_code as u8;
            }
            dst[out_pos] = (lit_nib << 4) | ml_nib;
            out_pos += 1;

            // Emit extended literal length if >= 15
            if (lit_len >= 15) {
                const next_p = writeExtLen(dst, out_pos, lit_len - 15);
                if (next_p < 0) {
                    return -1;
                }
                out_pos = next_p as usize;
            }

            // Copy literals
            if (lit_len > dst.len - out_pos) {
                return -1;
            }
            let k: usize = 0;
            while (k < lit_len) {
                dst[out_pos + k] = src[anchor + k];
                k += 1;
            }
            out_pos += lit_len;

            // Emit 2-byte little-endian match offset
            if (out_pos + 2 > dst.len) {
                return -1;
            }
            dst[out_pos] = (dist & 0xFF) as u8;
            dst[out_pos + 1] = ((dist >> 8) & 0xFF) as u8;
            out_pos += 2;

            // Emit extended match length if ml_code >= 15
            if (ml_code >= 15) {
                const next_p = writeExtLen(dst, out_pos, ml_code - 15);
                if (next_p < 0) {
                    return -1;
                }
                out_pos = next_p as usize;
            }

            pos = m_end;
            anchor = pos;
            if (pos <= mf_limit && pos >= 2) {
                table[hash4(src, pos - 2)] = ((pos - 2) & 0xFFFF) as u16;
            }
        }
    }

    // Emit final literals sequence (no trailing offset/match)
    const last_lit = src.len - anchor;
    if (out_pos >= dst.len) {
        return -1;
    }
    let last_nib: u8 = 15;
    if (last_lit < 15) {
        last_nib = last_lit as u8;
    }
    dst[out_pos] = last_nib << 4;
    out_pos += 1;

    if (last_lit >= 15) {
        const next_p = writeExtLen(dst, out_pos, last_lit - 15);
        if (next_p < 0) {
            return -1;
        }
        out_pos = next_p as usize;
    }

    if (last_lit > dst.len - out_pos) {
        return -1;
    }
    let k: usize = 0;
    while (k < last_lit) {
        dst[out_pos + k] = src[anchor + k];
        k += 1;
    }
    out_pos += last_lit;

    return out_pos as i64;
}

/// Decompresses a standard LZ4 Raw Block from `src` into `dst`.
/// Supports overlapping RLE matches (`offset < match_len`).
/// Returns decompressed byte count, or `-1` on malformed/truncated input or insufficient `dst`.
pub fn decompressBlock(src: []const u8, dst: []u8) -> i64 {
    if (src.len == 0) {
        return 0;
    }

    let in_pos: usize = 0;
    let out_pos: usize = 0;

    while (in_pos < src.len) {
        const token = src[in_pos];
        in_pos += 1;

        // Decode literal length
        let lit_len: usize = (token >> 4) as usize;
        if (lit_len == 15) {
            while (true) {
                if (in_pos >= src.len) {
                    return -1;
                }
                const extra = src[in_pos] as usize;
                in_pos += 1;
                lit_len += extra;
                if (extra != 255) {
                    break;
                }
            }
        }

        if (lit_len > src.len - in_pos || lit_len > dst.len - out_pos) {
            return -1;
        }
        let k: usize = 0;
        while (k < lit_len) {
            dst[out_pos + k] = src[in_pos + k];
            k += 1;
        }
        in_pos += lit_len;
        out_pos += lit_len;

        // End of block reached right after last literals
        if (in_pos == src.len) {
            break;
        }

        // Read 2-byte little-endian match offset
        if (in_pos + 2 > src.len) {
            return -1;
        }
        const offset: usize = (src[in_pos] as usize) | ((src[in_pos + 1] as usize) << 8);
        in_pos += 2;
        if (offset == 0 || offset > out_pos) {
            return -1;
        }

        // Decode match length
        let match_len: usize = ((token & 0x0F) as usize) + MIN_MATCH;
        if ((token & 0x0F) == 15) {
            while (true) {
                if (in_pos >= src.len) {
                    return -1;
                }
                const extra = src[in_pos] as usize;
                in_pos += 1;
                match_len += extra;
                if (extra != 255) {
                    break;
                }
            }
        }

        if (match_len > dst.len - out_pos) {
            return -1;
        }

        // Byte-by-byte copy handles overlapping RLE matches (offset < match_len) accurately
        let m: usize = 0;
        while (m < match_len) {
            dst[out_pos] = dst[out_pos - offset];
            out_pos += 1;
            m += 1;
        }
    }

    return out_pos as i64;
}

/// Compresses `src` with a 4-byte little-endian uncompressed size prefix into `dst`.
/// Returns total bytes written, or `-1` on insufficient `dst`.
pub fn compressWithSize(src: []const u8, dst: []u8) -> i64 {
    if (dst.len < 4) {
        return -1;
    }
    const orig_len: u32 = src.len as u32;
    dst[0] = (orig_len & 0xFF) as u8;
    dst[1] = ((orig_len >> 8) & 0xFF) as u8;
    dst[2] = ((orig_len >> 16) & 0xFF) as u8;
    dst[3] = ((orig_len >> 24) & 0xFF) as u8;

    const comp_len = compressBlock(src, dst[4..dst.len]);
    if (comp_len < 0) {
        return -1;
    }
    return comp_len + 4;
}

/// Decompresses a size-prefixed LZ4 block into `dst` and strictly verifies the decoded size matches the header.
/// Returns decoded byte count, or `-1` on mismatch, malformed input, or insufficient `dst`.
pub fn decompressWithSize(src: []const u8, dst: []u8) -> i64 {
    if (src.len < 4) {
        return -1;
    }
    const expected_len: usize = readU32LE(src, 0) as usize;
    if (expected_len > dst.len) {
        return -1;
    }
    if (expected_len == 0) {
        if (src.len != 4) {
            return -1;
        }
        return 0;
    }
    const dec_len = decompressBlock(src[4..src.len], dst[0..expected_len]);
    if (dec_len != (expected_len as i64)) {
        return -1;
    }
    return dec_len;
}
