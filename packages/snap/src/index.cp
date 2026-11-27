// @cap/snappy-block — Zero-allocation Google Snappy Raw Block codec for CP
// Clean-room MIT implementation of the official Google Snappy Format Specification (Rust `snap` parity).
//
// Features:
// - 100% wire-compatible with Google Snappy Raw Block format:
//   * Leading unsigned varint32 uncompressed length.
//   * Tag 00 (Literal with 1..4-byte extended length), Tag 01 (Copy-1), Tag 10 (Copy-2), Tag 11 (Copy-4).
// - Fixed stack 4096-entry `[4096]u16` hash table (8KB stack, zero heap allocation).
// - Strict decompression validation: rejects offset == 0, out-of-bounds offset, truncated streams,
//   and header/payload length mismatches.

const HASH_SIZE: usize = 4096;
const MAX_OFFSET: usize = 65535;

/// Returns the worst-case Snappy compressed size in bytes (`32 + input_len + input_len / 6`).
pub fn maxCompressedLen(input_len: usize) -> usize {
    return (32 as usize) + input_len + (input_len / 6);
}

struct VarintRes {
    ok: bool,
    value: u32,
    bytes_read: usize,
}

fn decodeVarint32(src: []const u8) -> VarintRes {
    let result: u32 = 0;
    let shift: u32 = 0;
    let i: usize = 0;

    while (i < src.len && i < 5) {
        const b = src[i];
        if (i == 4 && b > 0x0F) {
            return VarintRes{ ok: false, value: 0, bytes_read: 0 };
        }
        result = result | (((b & 0x7F) as u32) << (shift as u5));
        i += 1;
        if ((b & 0x80) == 0) {
            return VarintRes{ ok: true, value: result, bytes_read: i };
        }
        shift += 7;
    }

    return VarintRes{ ok: false, value: 0, bytes_read: 0 };
}

fn encodeVarint32(val: u32, dst: []u8, start_pos: usize) -> i64 {
    let v = val;
    let pos = start_pos;
    while (true) {
        if (pos >= dst.len) {
            return -1;
        }
        if (v < 0x80) {
            dst[pos] = v as u8;
            pos += 1;
            return pos as i64;
        }
        dst[pos] = ((v & 0x7F) | 0x80) as u8;
        pos += 1;
        v = v >> 7;
    }
}

/// Reads only the leading varint32 uncompressed length from a Snappy block.
/// Returns `-1` if the header is empty, truncated, or overflows `u32`.
pub fn uncompressedLen(src: []const u8) -> i64 {
    const r = decodeVarint32(src);
    if (!r.ok) {
        return -1;
    }
    return r.value as i64;
}

fn readU32LE(buf: []const u8, pos: usize) -> u32 {
    return (buf[pos] as u32) |
        ((buf[pos + 1] as u32) << 8) |
        ((buf[pos + 2] as u32) << 16) |
        ((buf[pos + 3] as u32) << 24);
}

fn hash4(buf: []const u8, pos: usize) -> usize {
    const v: u64 = readU32LE(buf, pos) as u64;
    const prod: u64 = v * (506832829 as u64);
    return ((prod >> 20) & (0xFFF as u64)) as usize;
}

fn match4(buf: []const u8, a: usize, b: usize) -> bool {
    return buf[a] == buf[b] &&
        buf[a + 1] == buf[b + 1] &&
        buf[a + 2] == buf[b + 2] &&
        buf[a + 3] == buf[b + 3];
}

fn emitLiteral(src: []const u8, start: usize, len: usize, dst: []u8, start_pos: usize) -> i64 {
    if (len == 0) {
        return start_pos as i64;
    }
    let pos = start_pos;
    const n = len - 1;

    if (n < 60) {
        if (pos >= dst.len) {
            return -1;
        }
        dst[pos] = ((n as u8) << 2);
        pos += 1;
    } else if (n <= 0xFF) {
        if (pos + 2 > dst.len) {
            return -1;
        }
        dst[pos] = (60 as u8) << 2;
        dst[pos + 1] = (n & 0xFF) as u8;
        pos += 2;
    } else if (n <= 0xFFFF) {
        if (pos + 3 > dst.len) {
            return -1;
        }
        dst[pos] = (61 as u8) << 2;
        dst[pos + 1] = (n & 0xFF) as u8;
        dst[pos + 2] = ((n >> 8) & 0xFF) as u8;
        pos += 3;
    } else if (n <= 0xFFFFFF) {
        if (pos + 4 > dst.len) {
            return -1;
        }
        dst[pos] = (62 as u8) << 2;
        dst[pos + 1] = (n & 0xFF) as u8;
        dst[pos + 2] = ((n >> 8) & 0xFF) as u8;
        dst[pos + 3] = ((n >> 16) & 0xFF) as u8;
        pos += 4;
    } else {
        if (pos + 5 > dst.len) {
            return -1;
        }
        dst[pos] = (63 as u8) << 2;
        dst[pos + 1] = (n & 0xFF) as u8;
        dst[pos + 2] = ((n >> 8) & 0xFF) as u8;
        dst[pos + 3] = ((n >> 16) & 0xFF) as u8;
        dst[pos + 4] = ((n >> 24) & 0xFF) as u8;
        pos += 5;
    }

    if (len > dst.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < len) {
        dst[pos + i] = src[start + i];
        i += 1;
    }
    pos += len;
    return pos as i64;
}

fn emitCopyChunk(offset: usize, chunk_len: usize, dst: []u8, start_pos: usize) -> i64 {
    let pos = start_pos;
    if (offset < 2048 && chunk_len >= 4 && chunk_len <= 11) {
        if (pos + 2 > dst.len) {
            return -1;
        }
        const hi = (((offset >> 8) & 0x07) as u8) << 5;
        const ml = (((chunk_len - 4) & 0x07) as u8) << 2;
        dst[pos] = hi | ml | 0x01;
        dst[pos + 1] = (offset & 0xFF) as u8;
        pos += 2;
        return pos as i64;
    }

    if (pos + 3 > dst.len) {
        return -1;
    }
    dst[pos] = ((((chunk_len - 1) & 0x3F) as u8) << 2) | 0x02;
    dst[pos + 1] = (offset & 0xFF) as u8;
    dst[pos + 2] = ((offset >> 8) & 0xFF) as u8;
    pos += 3;
    return pos as i64;
}

fn emitCopy(offset: usize, total_len: usize, dst: []u8, start_pos: usize) -> i64 {
    let pos = start_pos;
    let rem = total_len;
    while (rem >= 68) {
        const next_p = emitCopyChunk(offset, 64 as usize, dst, pos);
        if (next_p < 0) {
            return -1;
        }
        pos = next_p as usize;
        rem -= 64;
    }
    if (rem > 64) {
        const next_p = emitCopyChunk(offset, 60 as usize, dst, pos);
        if (next_p < 0) {
            return -1;
        }
        pos = next_p as usize;
        rem -= 60;
    }
    if (rem > 0) {
        const next_p = emitCopyChunk(offset, rem, dst, pos);
        if (next_p < 0) {
            return -1;
        }
        pos = next_p as usize;
    }
    return pos as i64;
}

/// Compresses `src` into standard Google Snappy Raw Block format inside `dst`.
/// Returns compressed byte length, or `-1` if `dst` is too small.
pub fn compress(src: []const u8, dst: []u8) -> i64 {
    const hdr_end = encodeVarint32(src.len as u32, dst, 0 as usize);
    if (hdr_end < 0) {
        return -1;
    }
    let out_pos = hdr_end as usize;
    if (src.len == 0) {
        return out_pos as i64;
    }

    let table: [HASH_SIZE]u16 = undefined;
    let ti: usize = 0;
    while (ti < HASH_SIZE) {
        table[ti] = 0;
        ti += 1;
    }

    let anchor: usize = 0;
    if (src.len >= 8) {
        const limit = src.len - 4;
        table[hash4(src, 0 as usize)] = 0;
        let pos: usize = 1;

        while (pos <= limit) {
            const h = hash4(src, pos);
            const cand_low = table[h] as usize;
            table[h] = (pos & (0xFFFF as usize)) as u16;

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
            if (dist == 0 || dist > MAX_OFFSET || !match4(src, cand, pos)) {
                pos += 1;
                continue;
            }

            // Emit pending literals before match
            if (pos > anchor) {
                const lp = emitLiteral(src, anchor, pos - anchor, dst, out_pos);
                if (lp < 0) {
                    return -1;
                }
                out_pos = lp as usize;
            }

            // Extend match greedily
            let m_end = pos + 4;
            let c_end = cand + 4;
            while (m_end < src.len && src[m_end] == src[c_end]) {
                m_end += 1;
                c_end += 1;
            }
            const match_len = m_end - pos;
            const cp = emitCopy(dist, match_len, dst, out_pos);
            if (cp < 0) {
                return -1;
            }
            out_pos = cp as usize;

            pos = m_end;
            anchor = pos;
            if (pos <= limit && pos >= 2) {
                table[hash4(src, pos - 2)] = ((pos - 2) & (0xFFFF as usize)) as u16;
            }
        }
    }

    // Emit remaining tail literals
    if (anchor < src.len) {
        const lp = emitLiteral(src, anchor, src.len - anchor, dst, out_pos);
        if (lp < 0) {
            return -1;
        }
        out_pos = lp as usize;
    }

    return out_pos as i64;
}

/// Decompresses a standard Google Snappy Raw Block from `src` into `dst`.
/// Supports Literal, Copy-1, Copy-2, Copy-4 tags and overlapping RLE copies.
/// Returns decoded byte count, or `-1` on malformed/truncated input, invalid offset, or length mismatch.
pub fn decompress(src: []const u8, dst: []u8) -> i64 {
    const hdr = decodeVarint32(src);
    if (!hdr.ok) {
        return -1;
    }
    const expected_len = hdr.value as usize;
    if (expected_len > dst.len) {
        return -1;
    }

    let in_pos = hdr.bytes_read;
    let out_pos: usize = 0;

    while (in_pos < src.len) {
        const tag = src[in_pos];
        in_pos += 1;
        const kind = tag & 0x03;

        if (kind == 0) {
            // Literal
            const n = (tag >> 2) as usize;
            let lit_len: usize = 0;
            if (n < 60) {
                lit_len = n + 1;
            } else {
                const extra_bytes = n - 59; // 1..4 bytes
                if (extra_bytes > src.len - in_pos) {
                    return -1;
                }
                let raw_n: u32 = 0;
                let k: usize = 0;
                while (k < extra_bytes) {
                    raw_n = raw_n | ((src[in_pos + k] as u32) << ((k * 8) as u5));
                    k += 1;
                }
                in_pos += extra_bytes;
                lit_len = (raw_n as usize) + 1;
            }

            if (lit_len > src.len - in_pos || lit_len > expected_len - out_pos) {
                return -1;
            }
            let i: usize = 0;
            while (i < lit_len) {
                dst[out_pos + i] = src[in_pos + i];
                i += 1;
            }
            in_pos += lit_len;
            out_pos += lit_len;
        } else if (kind == 1) {
            // Copy with 1-byte offset
            if (in_pos >= src.len) {
                return -1;
            }
            const copy_len: usize = (((tag >> 2) & 0x07) as usize) + 4;
            const offset: usize = ((((tag >> 5) & 0x07) as usize) << 8) | (src[in_pos] as usize);
            in_pos += 1;

            if (offset == 0 || offset > out_pos || copy_len > expected_len - out_pos) {
                return -1;
            }
            let m: usize = 0;
            while (m < copy_len) {
                dst[out_pos] = dst[out_pos - offset];
                out_pos += 1;
                m += 1;
            }
        } else if (kind == 2) {
            // Copy with 2-byte offset
            if (in_pos + 2 > src.len) {
                return -1;
            }
            const copy_len: usize = ((tag >> 2) as usize) + 1;
            const offset: usize = (src[in_pos] as usize) | ((src[in_pos + 1] as usize) << 8);
            in_pos += 2;

            if (offset == 0 || offset > out_pos || copy_len > expected_len - out_pos) {
                return -1;
            }
            let m: usize = 0;
            while (m < copy_len) {
                dst[out_pos] = dst[out_pos - offset];
                out_pos += 1;
                m += 1;
            }
        } else {
            // Copy with 4-byte offset (kind == 3)
            if (in_pos + 4 > src.len) {
                return -1;
            }
            const copy_len: usize = ((tag >> 2) as usize) + 1;
            const offset: usize = readU32LE(src, in_pos) as usize;
            in_pos += 4;

            if (offset == 0 || offset > out_pos || copy_len > expected_len - out_pos) {
                return -1;
            }
            let m: usize = 0;
            while (m < copy_len) {
                dst[out_pos] = dst[out_pos - offset];
                out_pos += 1;
                m += 1;
            }
        }
    }

    if (out_pos != expected_len) {
        return -1;
    }
    return out_pos as i64;
}
