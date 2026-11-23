// Origin: Pure-CP Stdlib (Zero-Alloc LZW Compressor & Decompressor) | Created: 2026-11-21

pub const LZW_MAX_BITS: u8 = 12;
pub const LZW_TABLE_SIZE: usize = 4096;
pub const LZW_MIN_CODE_BITS: u8 = 2;
pub const LZW_MAX_INITIAL_BITS: u8 = 8;

const LZW_HASH_SIZE: usize = 8192;
const HASH_MASK: usize = 8191;
const EMPTY_KEY: u32 = 0xFFFFFFFF;
const NO_PREV: u16 = 0xFFFF;

/// Bit-packing order for LZW streams.
/// - `lsb`: GIF89a style (LSB-first packing, late code-width increase)
/// - `msb`: TIFF 6.0 / PDF style (MSB-first packing, early code-width increase)
pub enum LzwBitOrder: u8 {
    lsb = 0,
    msb = 1,
}

struct LzwBitWriter {
    dst: []u8,
    pos: usize,
    acc: u32,
    bits: u8,
    order: LzwBitOrder,

    static fn init(dst: []u8, order: LzwBitOrder): LzwBitWriter {
        return LzwBitWriter {
            dst: dst,
            pos: 0,
            acc: 0,
            bits: 0,
            order: order,
        };
    }

    fn writeCode(*_, code: u16, width: u8): bool {
        if (_.order == LzwBitOrder.lsb) {
            _.acc |= (code as u32) << (_.bits as u5);
            _.bits += width;
            while (_.bits >= 8) {
                if (_.pos >= _.dst.len) {
                    return false;
                }
                _.dst[_.pos] = (_.acc & 0xFF) as u8;
                _.pos += 1;
                _.acc >>= 8;
                _.bits -= 8;
            }
            return true;
        }
        // MSB-first
        _.acc = (_.acc << (width as u5)) | (code as u32);
        _.bits += width;
        while (_.bits >= 8) {
            if (_.pos >= _.dst.len) {
                return false;
            }
            _.bits -= 8;
            _.dst[_.pos] = ((_.acc >> (_.bits as u5)) & 0xFF) as u8;
            _.pos += 1;
        }
        return true;
    }

    fn flush(*_): usize? {
        if (_.bits > 0) {
            if (_.pos >= _.dst.len) {
                return null;
            }
            if (_.order == LzwBitOrder.lsb) {
                _.dst[_.pos] = (_.acc & 0xFF) as u8;
            } else {
                const shift = (8 - _.bits) as u5;
                _.dst[_.pos] = ((_.acc << shift) & 0xFF) as u8;
            }
            _.pos += 1;
            _.bits = 0;
            _.acc = 0;
        }
        return _.pos;
    }
}

struct LzwBitReader {
    src: []const u8,
    pos: usize,
    acc: u32,
    bits: u8,
    order: LzwBitOrder,

    static fn init(src: []const u8, order: LzwBitOrder): LzwBitReader {
        return LzwBitReader {
            src: src,
            pos: 0,
            acc: 0,
            bits: 0,
            order: order,
        };
    }

    fn readCode(*_, width: u8): u16? {
        while (_.bits < width) {
            if (_.pos >= _.src.len) {
                return null;
            }
            const b = _.src[_.pos] as u32;
            _.pos += 1;
            if (_.order == LzwBitOrder.lsb) {
                _.acc |= b << (_.bits as u5);
            } else {
                _.acc = (_.acc << 8) | b;
            }
            _.bits += 8;
        }

        const mask: u32 = ((1 as u32) << (width as u5)) - 1;
        if (_.order == LzwBitOrder.lsb) {
            const code = (_.acc & mask) as u16;
            _.acc >>= (width as u5);
            _.bits -= width;
            return code;
        }
        _.bits -= width;
        const code = ((_.acc >> (_.bits as u5)) & mask) as u16;
        return code;
    }
}

fn hashSlot(key: u32): usize {
    return (((key *% 2654435761) >> 19) as usize) & HASH_MASK;
}

fn clearHashTable(keys: *[LZW_HASH_SIZE]u32) {
    let i: usize = 0;
    while (i < LZW_HASH_SIZE) {
        keys.*[i] = EMPTY_KEY;
        i += 1;
    }
}

/// Zero-allocation LZW compressor using a fixed 4096-entry stack dictionary.
/// Validates `2 <= lit_width <= 8`, emits initial Clear code, auto-widens codes up to 12 bits,
/// emits Clear when the 4096-entry dictionary fills, and terminates with EOI.
pub fn lzwCompress(src: []const u8, dst: *[]u8, lit_width: u8, order: LzwBitOrder): usize? {
    if (lit_width < LZW_MIN_CODE_BITS || lit_width > LZW_MAX_INITIAL_BITS) {
        return null;
    }

    const clear_code: u16 = (1 as u16) << (lit_width as u4);
    const eoi_code: u16 = clear_code + 1;
    const first_code: u16 = clear_code + 2;
    const max_sym: u16 = clear_code - 1;

    let bw = LzwBitWriter.init(dst, order);
    let code_size: u8 = lit_width + 1;
    let next_code: u16 = first_code;

    // Emit initial Clear code
    if (!bw.writeCode(clear_code, code_size)) {
        return null;
    }

    if (src.len == 0) {
        if (!bw.writeCode(eoi_code, code_size)) {
            return null;
        }
        return bw.flush();
    }

    let hash_keys: [LZW_HASH_SIZE]u32 = undefined;
    let hash_vals: [LZW_HASH_SIZE]u16 = undefined;
    clearHashTable(*hash_keys);

    if ((src[0] as u16) > max_sym) {
        return null;
    }
    let prefix: u16 = src[0] as u16;
    let i: usize = 1;

    while (i < src.len) {
        const ch = src[i];
        if ((ch as u16) > max_sym) {
            return null;
        }
        const key: u32 = ((prefix as u32) << 8) | (ch as u32);
        let slot = hashSlot(key);
        let found = false;
        let found_code: u16 = 0;

        while (hash_keys[slot] != EMPTY_KEY) {
            if (hash_keys[slot] == key) {
                found = true;
                found_code = hash_vals[slot];
                break;
            }
            slot = (slot + 1) & HASH_MASK;
        }

        if (found) {
            prefix = found_code;
        } else {
            if (!bw.writeCode(prefix, code_size)) {
                return null;
            }

            if ((next_code as usize) < LZW_TABLE_SIZE) {
                hash_keys[slot] = key;
                hash_vals[slot] = next_code;
                next_code += 1;

                const limit: u16 = (1 as u16) << (code_size as u4);
                if (order == LzwBitOrder.msb) {
                    // TIFF / PDF early change: widen when next_code == (1 << code_size)
                    if (next_code == limit && code_size < LZW_MAX_BITS) {
                        code_size += 1;
                    }
                } else {
                    // GIF late change: widen when next_code > (1 << code_size)
                    if (next_code > limit && code_size < LZW_MAX_BITS) {
                        code_size += 1;
                    }
                }
            } else {
                // Dictionary full (4096 entries): emit Clear and reset
                if (!bw.writeCode(clear_code, code_size)) {
                    return null;
                }
                clearHashTable(*hash_keys);
                next_code = first_code;
                code_size = lit_width + 1;
            }

            prefix = ch as u16;
        }
        i += 1;
    }

    if (!bw.writeCode(prefix, code_size)) {
        return null;
    }
    if (!bw.writeCode(eoi_code, code_size)) {
        return null;
    }
    return bw.flush();
}

/// Zero-allocation LZW decompressor using a fixed 4096-entry stack dictionary.
/// Handles the classic `KwKwK` (`code == next_code`) edge case, Clear resets, EOI termination,
/// and rejects invalid codes (`code > next_code`), truncation, or output buffer overflow.
pub fn lzwDecompress(src: []const u8, dst: *[]u8, lit_width: u8, order: LzwBitOrder): usize? {
    if (lit_width < LZW_MIN_CODE_BITS || lit_width > LZW_MAX_INITIAL_BITS) {
        return null;
    }

    const clear_code: u16 = (1 as u16) << (lit_width as u4);
    const eoi_code: u16 = clear_code + 1;
    const first_code: u16 = clear_code + 2;

    let prefix_tbl: [LZW_TABLE_SIZE]u16 = undefined;
    let suffix_tbl: [LZW_TABLE_SIZE]u8 = undefined;
    let stack: [LZW_TABLE_SIZE]u8 = undefined;

    let init_i: usize = 0;
    while (init_i < (clear_code as usize)) {
        prefix_tbl[init_i] = NO_PREV;
        suffix_tbl[init_i] = init_i as u8;
        init_i += 1;
    }

    let br = LzwBitReader.init(src, order);
    let code_size: u8 = lit_width + 1;
    let next_code: u16 = first_code;
    let prev_code: u16 = NO_PREV;
    let prev_first_byte: u8 = 0;
    let out_pos: usize = 0;

    while (true) {
        let code: u16 = 0;
        if (c := br.readCode(code_size)) {
            code = c;
        } else {
            return null;
        }

        if (code == clear_code) {
            code_size = lit_width + 1;
            next_code = first_code;
            prev_code = NO_PREV;
            continue;
        }
        if (code == eoi_code) {
            return out_pos;
        }

        if (code > next_code) {
            return null;
        }

        let sp: usize = 0;
        let cur: u16 = code;
        if (code == next_code) {
            // Classic KwKwK special case
            if (prev_code == NO_PREV) {
                return null;
            }
            stack[sp] = prev_first_byte;
            sp += 1;
            cur = prev_code;
        }

        while (cur >= clear_code) {
            if (sp >= LZW_TABLE_SIZE) {
                return null;
            }
            stack[sp] = suffix_tbl[cur as usize];
            sp += 1;
            cur = prefix_tbl[cur as usize];
            if (cur == NO_PREV) {
                return null;
            }
        }
        if (sp >= LZW_TABLE_SIZE) {
            return null;
        }
        const first_byte = suffix_tbl[cur as usize];
        stack[sp] = first_byte;
        sp += 1;

        if (dst.len - out_pos < sp) {
            return null;
        }
        let k = sp;
        while (k > 0) {
            k -= 1;
            dst[out_pos] = stack[k];
            out_pos += 1;
        }

        if (prev_code != NO_PREV && (next_code as usize) < LZW_TABLE_SIZE) {
            prefix_tbl[next_code as usize] = prev_code;
            suffix_tbl[next_code as usize] = first_byte;
            next_code += 1;

            const limit: u16 = (1 as u16) << (code_size as u4);
            if (order == LzwBitOrder.msb) {
                // TIFF / PDF early change
                if (next_code == limit - 1 && code_size < LZW_MAX_BITS) {
                    code_size += 1;
                }
            } else {
                // GIF late change
                if (next_code == limit && code_size < LZW_MAX_BITS) {
                    code_size += 1;
                }
            }
        }

        prev_code = code;
        prev_first_byte = first_byte;
    }
}

/// Convenience wrapper for GIF-style LZW compression (LSB-first).
pub fn lzwCompressGif(src: []const u8, dst: *[]u8, lit_width: u8): usize? {
    return lzwCompress(src, dst, lit_width, LzwBitOrder.lsb);
}

/// Convenience wrapper for GIF-style LZW decompression (LSB-first).
pub fn lzwDecompressGif(src: []const u8, dst: *[]u8, lit_width: u8): usize? {
    return lzwDecompress(src, dst, lit_width, LzwBitOrder.lsb);
}

/// Convenience wrapper for TIFF/PDF-style LZW compression (MSB-first, 8-bit literals).
pub fn lzwCompressTiff(src: []const u8, dst: *[]u8): usize? {
    return lzwCompress(src, dst, 8, LzwBitOrder.msb);
}

/// Convenience wrapper for TIFF/PDF-style LZW decompression (MSB-first, 8-bit literals).
pub fn lzwDecompressTiff(src: []const u8, dst: *[]u8): usize? {
    return lzwDecompress(src, dst, 8, LzwBitOrder.msb);
}
