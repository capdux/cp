// @cap/lzw — Zero-allocation GIF/LSB-first LZW dictionary compressor & decompressor for CP
// Clean-room MIT implementation inspired by Rust `weezl` and the GIF89a LZW specification.
//
// Parameters:
// - 8-bit byte alphabet (`min_code_size = 8`): literals `0..255`, `CLEAR_CODE = 256`,
//   `EOI_CODE = 257`, first dynamic code `258`, maximum table size `4096` (9..12-bit variable width).
// - LSB-first bit packing, automatic `CLEAR_CODE` table reset at 4096 entries, full `KwKwK`
//   self-referential decoding support, and strict malformed/truncated stream rejection (`-1`).
// - Zero heap allocation.

pub const CLEAR_CODE: u16 = 256;
pub const EOI_CODE: u16 = 257;
const FIRST_CODE: u16 = 258;
const MAX_TABLE: usize = 4096;
const HASH_SIZE: usize = 8192;
const EMPTY_KEY: u32 = 0xFFFFFFFF;

fn hashPair(key: u32) -> usize {
    const prod: u64 = (key as u64) * (2654435761 as u64);
    return ((prod >> 19) & (0x1FFF as u64)) as usize;
}

/// Compresses `src` into a standard LSB-first GIF LZW stream inside `dst`.
/// Starts with `CLEAR_CODE` (256), auto-clears when dictionary reaches 4096 entries,
/// and terminates with `EOI_CODE` (257). Returns compressed bytes written, or `-1` if `dst` is too small.
pub fn encode(src: []const u8, dst: []u8) -> i64 {
    let hash_keys: [HASH_SIZE]u32 = undefined;
    let hash_vals: [HASH_SIZE]u16 = undefined;

    let hi: usize = 0;
    while (hi < HASH_SIZE) {
        hash_keys[hi] = EMPTY_KEY;
        hash_vals[hi] = 0;
        hi += 1;
    }

    let bit_buf: u32 = 0;
    let bit_count: u8 = 0;
    let out_pos: usize = 0;
    let code_size: u8 = 9;
    let next_code: u16 = FIRST_CODE;

    // Emit initial CLEAR_CODE (9 bits)
    bit_buf = bit_buf | ((CLEAR_CODE as u32) << (bit_count as u5));
    bit_count += code_size;
    while (bit_count >= 8) {
        if (out_pos >= dst.len) {
            return -1;
        }
        dst[out_pos] = (bit_buf & 0xFF) as u8;
        out_pos += 1;
        bit_buf = bit_buf >> 8;
        bit_count -= 8;
    }

    if (src.len > 0) {
        let prefix: u16 = src[0] as u16;
        let i: usize = 1;
        while (i < src.len) {
            const suffix = src[i];
            const pair_key: u32 = ((prefix as u32) << 8) | (suffix as u32);

            // Open-addressing lookup in 8192-slot hash table
            let slot = hashPair(pair_key);
            let found = false;
            let found_code: u16 = 0;
            while (hash_keys[slot] != EMPTY_KEY) {
                if (hash_keys[slot] == pair_key) {
                    found = true;
                    found_code = hash_vals[slot];
                    break;
                }
                slot = (slot + 1) & (HASH_SIZE - 1);
            }

            if (found) {
                prefix = found_code;
            } else {
                // Emit `prefix` with current `code_size`
                bit_buf = bit_buf | ((prefix as u32) << (bit_count as u5));
                bit_count += code_size;
                while (bit_count >= 8) {
                    if (out_pos >= dst.len) {
                        return -1;
                    }
                    dst[out_pos] = (bit_buf & 0xFF) as u8;
                    out_pos += 1;
                    bit_buf = bit_buf >> 8;
                    bit_count -= 8;
                }

                if ((next_code as usize) < MAX_TABLE) {
                    hash_keys[slot] = pair_key;
                    hash_vals[slot] = next_code;
                    next_code += 1;
                    const limit: u16 = (1 as u16) << (code_size as u4);
                    if (next_code > limit && code_size < 12) {
                        code_size += 1;
                    }
                } else {
                    // Dictionary full (4096): emit CLEAR_CODE and reset table
                    bit_buf = bit_buf | ((CLEAR_CODE as u32) << (bit_count as u5));
                    bit_count += code_size;
                    while (bit_count >= 8) {
                        if (out_pos >= dst.len) {
                            return -1;
                        }
                        dst[out_pos] = (bit_buf & 0xFF) as u8;
                        out_pos += 1;
                        bit_buf = bit_buf >> 8;
                        bit_count -= 8;
                    }
                    let ci: usize = 0;
                    while (ci < HASH_SIZE) {
                        hash_keys[ci] = EMPTY_KEY;
                        ci += 1;
                    }
                    next_code = FIRST_CODE;
                    code_size = 9;
                }

                prefix = suffix as u16;
            }
            i += 1;
        }

        // Emit final `prefix`
        bit_buf = bit_buf | ((prefix as u32) << (bit_count as u5));
        bit_count += code_size;
        while (bit_count >= 8) {
            if (out_pos >= dst.len) {
                return -1;
            }
            dst[out_pos] = (bit_buf & 0xFF) as u8;
            out_pos += 1;
            bit_buf = bit_buf >> 8;
            bit_count -= 8;
        }
    }

    // Emit EOI_CODE and flush remaining bits
    bit_buf = bit_buf | ((EOI_CODE as u32) << (bit_count as u5));
    bit_count += code_size;
    while (bit_count >= 8) {
        if (out_pos >= dst.len) {
            return -1;
        }
        dst[out_pos] = (bit_buf & 0xFF) as u8;
        out_pos += 1;
        bit_buf = bit_buf >> 8;
        bit_count -= 8;
    }
    if (bit_count > 0) {
        if (out_pos >= dst.len) {
            return -1;
        }
        dst[out_pos] = (bit_buf & 0xFF) as u8;
        out_pos += 1;
    }

    return out_pos as i64;
}

/// Decompresses a standard LSB-first GIF LZW stream from `src` into `dst`.
/// Handles `CLEAR_CODE`, `EOI_CODE`, variable 9..12-bit widths, and `KwKwK` self-referential codes.
/// Returns decompressed byte count, or `-1` on invalid code, truncated stream, or insufficient `dst`.
pub fn decode(src: []const u8, dst: []u8) -> i64 {
    if (src.len == 0) {
        return -1;
    }

    let prefixes: [MAX_TABLE]u16 = undefined;
    let suffixes: [MAX_TABLE]u8 = undefined;
    let rev_stack: [MAX_TABLE]u8 = undefined;

    let init_i: usize = 0;
    while (init_i < 256) {
        prefixes[init_i] = 0;
        suffixes[init_i] = init_i as u8;
        init_i += 1;
    }

    let in_pos: usize = 0;
    let bit_buf: u32 = 0;
    let bit_count: u8 = 0;
    let out_pos: usize = 0;

    let code_size: u8 = 9;
    let next_code: u16 = FIRST_CODE;
    let has_prev = false;
    let prev_code: u16 = 0;
    let prev_first: u8 = 0;

    while (true) {
        // Fill bit buffer until at least `code_size` bits are available
        while (bit_count < code_size) {
            if (in_pos >= src.len) {
                return -1; // truncated stream before EOI_CODE
            }
            bit_buf = bit_buf | ((src[in_pos] as u32) << (bit_count as u5));
            in_pos += 1;
            bit_count += 8;
        }

        const mask: u32 = ((1 as u32) << (code_size as u5)) - 1;
        const code: u16 = (bit_buf & mask) as u16;
        bit_buf = bit_buf >> (code_size as u5);
        bit_count -= code_size;

        if (code == EOI_CODE) {
            return out_pos as i64;
        }

        if (code == CLEAR_CODE) {
            code_size = 9;
            next_code = FIRST_CODE;
            has_prev = false;
            continue;
        }

        if (!has_prev) {
            if (code >= 256) {
                return -1;
            }
            if (out_pos >= dst.len) {
                return -1;
            }
            const ch = code as u8;
            dst[out_pos] = ch;
            out_pos += 1;
            prev_code = code;
            prev_first = ch;
            has_prev = true;
            continue;
        }

        // Reject out-of-range / undefined dictionary codes
        if (code > next_code || (code as usize) >= MAX_TABLE) {
            return -1;
        }

        let slen: usize = 0;
        let cur: u16 = code;
        if (code == next_code) {
            // Classic KwKwK self-referential case
            rev_stack[slen] = prev_first;
            slen += 1;
            cur = prev_code;
        }

        while (cur >= 256) {
            if (slen >= MAX_TABLE) {
                return -1;
            }
            rev_stack[slen] = suffixes[cur as usize];
            slen += 1;
            cur = prefixes[cur as usize];
        }
        if (slen >= MAX_TABLE) {
            return -1;
        }
        const first_ch = cur as u8;
        rev_stack[slen] = first_ch;
        slen += 1;

        if (slen > dst.len - out_pos) {
            return -1;
        }
        let k = slen;
        while (k > 0) {
            k -= 1;
            dst[out_pos] = rev_stack[k];
            out_pos += 1;
        }

        if ((next_code as usize) < MAX_TABLE) {
            prefixes[next_code as usize] = prev_code;
            suffixes[next_code as usize] = first_ch;
            next_code += 1;
            const limit: u16 = (1 as u16) << (code_size as u4);
            if (next_code == limit && code_size < 12) {
                code_size += 1;
            }
        }

        prev_code = code;
        prev_first = first_ch;
    }
}
