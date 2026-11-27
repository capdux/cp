// @cap/lz-string — Zero-heap LZW compression with Base64 & URI-safe encodings
// Clean-room MIT implementation inspired by npm:lz-string (pieroxy/lz-string).
// Accelerated with comptime 256-entry Base64/URI decode LUTs, 4-lane @Vector(4, u32)
// SIMD hash-table reset, 16-lane @Vector(16, u8) SIMD Base64 validation, and undefined buffers.

type LzsVec4U32 = @Vector(4, u32);
type LzsVec16U8 = @Vector(16, u8);

const LZS_DICT_CAP: usize = 4096;
const LZS_HT_SIZE: usize = 8192;
const LZS_HT_MASK: usize = 8191;
const LZS_HT_EMPTY: u32 = 0xFFFF_FFFF;
const LZS_CLEAR: u16 = 256;
const LZS_EOF: u16 = 257;
const LZS_FIRST_CODE: u16 = 258;
const LZS_TMP_CAP: usize = 32768;

const LZS_B64_ALPHA: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const LZS_URI_ALPHA: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-$";

fn lzsBuildB64Lut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0xFF; 256];
    let c: usize = 0;
    while (c < 26) : (c += 1) {
        lut[65 + c] = c as u8;
        lut[97 + c] = (26 + c) as u8;
    }
    let d: usize = 0;
    while (d < 10) : (d += 1) {
        lut[48 + d] = (52 + d) as u8;
    }
    lut[43] = 62; // '+'
    lut[47] = 63; // '/'
    return lut;
}

fn lzsBuildUriLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0xFF; 256];
    let c: usize = 0;
    while (c < 26) : (c += 1) {
        lut[65 + c] = c as u8;
        lut[97 + c] = (26 + c) as u8;
    }
    let d: usize = 0;
    while (d < 10) : (d += 1) {
        lut[48 + d] = (52 + d) as u8;
    }
    lut[43] = 62; // '+'
    lut[45] = 63; // '-'
    return lut;
}

const LZS_B64_DEC_LUT: [256]u8 = lzsBuildB64Lut();
const LZS_URI_DEC_LUT: [256]u8 = lzsBuildUriLut();

fn lzsHash(key: u32): usize {
    const mixed = (key *% 2654435761) >> 19;
    return usize(mixed) & LZS_HT_MASK;
}

fn lzsClearTable(ht_key: []u32) {
    const empty_v: LzsVec4U32 = @splat(LZS_HT_EMPTY);
    const empty_a: [4]u32 = empty_v;
    let i: usize = 0;
    while (i < LZS_HT_SIZE) : (i += 4) {
        @memcpy(ht_key[i .. i + 4], empty_a[0..4]);
    }
}

fn lzsFind(ht_key: []const u32, ht_val: []const u16, key: u32): i32 {
    let slot = lzsHash(key);
    let probes: usize = 0;
    while (probes < LZS_HT_SIZE) : (probes += 1) {
        const k = ht_key[slot];
        if (k == LZS_HT_EMPTY) {
            return -1;
        }
        if (k == key) {
            return i32(ht_val[slot]);
        }
        slot = (slot + 1) & LZS_HT_MASK;
    }
    return -1;
}

fn lzsInsert(ht_key: []u32, ht_val: []u16, key: u32, code: u16) {
    let slot = lzsHash(key);
    let probes: usize = 0;
    while (probes < LZS_HT_SIZE) : (probes += 1) {
        if (ht_key[slot] == LZS_HT_EMPTY) {
            ht_key[slot] = key;
            ht_val[slot] = code;
            return;
        }
        slot = (slot + 1) & LZS_HT_MASK;
    }
}

/// Compress raw bytes using fixed-capacity 4096-entry LZW (9..12-bit variable width).
/// Returns number of compressed bytes written to `out` (0 for empty input or overflow).
pub fn compressBytes(input: []const u8, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }

    let ht_key: [8192]u32 = undefined;
    let ht_val: [8192]u16 = undefined;
    lzsClearTable(ht_key[0..]);

    let next_code: u16 = LZS_FIRST_CODE;
    let bit_width: u5 = 9;
    let bit_buf: u32 = 0;
    let bit_count: u5 = 0;
    let out_pos: usize = 0;

    let w: u16 = u16(input[0]);
    let i: usize = 1;
    while (i < input.len) : (i += 1) {
        const c = input[i];
        const key: u32 = (u32(w) << 8) | u32(c);
        const found = lzsFind(ht_key[0..], ht_val[0..], key);
        if (found >= 0) {
            w = u16(found);
        } else {
            // Emit code `w` using `bit_width` bits
            bit_buf |= u32(w) << bit_count;
            bit_count += bit_width;
            while (bit_count >= 8) {
                if (out_pos >= out.len) {
                    return 0;
                }
                out[out_pos] = u8(bit_buf & 0xFF);
                out_pos += 1;
                bit_buf >>= 8;
                bit_count -= 8;
            }

            // Insert (w, c) into dictionary
            lzsInsert(ht_key[0..], ht_val[0..], key, next_code);
            next_code += 1;

            if (next_code == u16(LZS_DICT_CAP)) {
                // Emit CLEAR and reset dictionary
                bit_buf |= u32(LZS_CLEAR) << bit_count;
                bit_count += bit_width;
                while (bit_count >= 8) {
                    if (out_pos >= out.len) {
                        return 0;
                    }
                    out[out_pos] = u8(bit_buf & 0xFF);
                    out_pos += 1;
                    bit_buf >>= 8;
                    bit_count -= 8;
                }
                lzsClearTable(ht_key[0..]);
                next_code = LZS_FIRST_CODE;
                bit_width = 9;
            } else if (bit_width < 12) {
                const limit: u16 = u16(1) << bit_width;
                if (next_code == limit) {
                    bit_width += 1;
                }
            }

            w = u16(c);
        }
    }

    // Emit final `w`
    bit_buf |= u32(w) << bit_count;
    bit_count += bit_width;
    while (bit_count >= 8) {
        if (out_pos >= out.len) {
            return 0;
        }
        out[out_pos] = u8(bit_buf & 0xFF);
        out_pos += 1;
        bit_buf >>= 8;
        bit_count -= 8;
    }

    // Emit EOF marker
    bit_buf |= u32(LZS_EOF) << bit_count;
    bit_count += bit_width;
    while (bit_count >= 8) {
        if (out_pos >= out.len) {
            return 0;
        }
        out[out_pos] = u8(bit_buf & 0xFF);
        out_pos += 1;
        bit_buf >>= 8;
        bit_count -= 8;
    }

    if (bit_count > 0) {
        if (out_pos >= out.len) {
            return 0;
        }
        out[out_pos] = u8(bit_buf & 0xFF);
        out_pos += 1;
    }

    return out_pos;
}

/// Decompress LZW byte stream into `out`.
/// Returns decompressed byte length, or -1 on invalid/truncated input or output overflow.
pub fn decompressBytes(compressed: []const u8, out: []u8): i64 {
    if (compressed.len == 0) {
        return 0;
    }

    let prefix: [4096]u16 = undefined;
    let suffix: [4096]u8 = undefined;
    let stack: [4096]u8 = undefined;

    let next_code: u16 = LZS_FIRST_CODE;
    let bit_width: u5 = 9;
    let prev_code: u16 = 0xFFFF;
    let first_byte: u8 = 0;

    let in_pos: usize = 0;
    let bit_buf: u32 = 0;
    let bit_count: u5 = 0;
    let out_pos: usize = 0;
    let saw_eof: bool = false;

    while (true) {
        while (bit_count < bit_width) {
            if (in_pos >= compressed.len) {
                return -1;
            }
            bit_buf |= u32(compressed[in_pos]) << bit_count;
            in_pos += 1;
            bit_count += 8;
        }

        const mask: u32 = (u32(1) << bit_width) - 1;
        const code: u16 = u16(bit_buf & mask);
        bit_buf >>= bit_width;
        bit_count -= bit_width;

        if (code == LZS_EOF) {
            saw_eof = true;
            break;
        }

        if (code == LZS_CLEAR) {
            next_code = LZS_FIRST_CODE;
            bit_width = 9;
            prev_code = 0xFFFF;
            continue;
        }

        if (code > next_code) {
            return -1;
        }
        if (code == next_code && prev_code == 0xFFFF) {
            return -1;
        }

        let cur: u16 = code;
        let sp: usize = 0;
        if (cur == next_code) {
            stack[sp] = first_byte;
            sp += 1;
            cur = prev_code;
        }

        while (cur >= LZS_FIRST_CODE) {
            if (sp >= LZS_DICT_CAP) {
                return -1;
            }
            stack[sp] = suffix[usize(cur)];
            sp += 1;
            cur = prefix[usize(cur)];
        }

        if (cur >= 256) {
            return -1;
        }

        first_byte = u8(cur);
        if (sp >= LZS_DICT_CAP) {
            return -1;
        }
        stack[sp] = first_byte;
        sp += 1;

        if (out_pos + sp > out.len) {
            return -1;
        }
        let k: usize = sp;
        while (k > 0) {
            k -= 1;
            out[out_pos] = stack[k];
            out_pos += 1;
        }

        if (prev_code != 0xFFFF && next_code < u16(LZS_DICT_CAP)) {
            prefix[usize(next_code)] = prev_code;
            suffix[usize(next_code)] = first_byte;
            next_code += 1;
            if (bit_width < 12) {
                const limit: u16 = (u16(1) << bit_width) - 1;
                if (next_code == limit) {
                    bit_width += 1;
                }
            }
        }

        prev_code = code;
    }

    if (!saw_eof) {
        return -1;
    }
    return i64(out_pos);
}

fn lzsEncode64(raw: []const u8, alphabet: string, pad: bool, out: []u8): usize {
    let i: usize = 0;
    let o: usize = 0;
    while (i + 3 <= raw.len) : (i += 3) {
        if (o + 4 > out.len) {
            return 0;
        }
        const b0 = u32(raw[i]);
        const b1 = u32(raw[i + 1]);
        const b2 = u32(raw[i + 2]);
        const triple = (b0 << 16) | (b1 << 8) | b2;
        out[o] = alphabet[usize((triple >> 18) & 0x3F)];
        out[o + 1] = alphabet[usize((triple >> 12) & 0x3F)];
        out[o + 2] = alphabet[usize((triple >> 6) & 0x3F)];
        out[o + 3] = alphabet[usize(triple & 0x3F)];
        o += 4;
    }

    const rem = raw.len - i;
    if (rem == 1) {
        const need: usize = if (pad) 4 else 2;
        if (o + need > out.len) {
            return 0;
        }
        const b0 = u32(raw[i]);
        out[o] = alphabet[usize((b0 >> 2) & 0x3F)];
        out[o + 1] = alphabet[usize((b0 << 4) & 0x3F)];
        o += 2;
        if (pad) {
            out[o] = '=';
            out[o + 1] = '=';
            o += 2;
        }
    } else if (rem == 2) {
        const need: usize = if (pad) 4 else 3;
        if (o + need > out.len) {
            return 0;
        }
        const b0 = u32(raw[i]);
        const b1 = u32(raw[i + 1]);
        out[o] = alphabet[usize((b0 >> 2) & 0x3F)];
        out[o + 1] = alphabet[usize(((b0 << 4) | (b1 >> 4)) & 0x3F)];
        out[o + 2] = alphabet[usize((b1 << 2) & 0x3F)];
        o += 3;
        if (pad) {
            out[o] = '=';
            o += 1;
        }
    }

    return o;
}

fn lzsDecode64(text: string, uri_mode: bool, out: []u8): i64 {
    if (text.len == 0) {
        return 0;
    }

    let eff_len = text.len;
    if (!uri_mode) {
        if ((eff_len & 3) != 0) {
            return -1;
        }
        if (eff_len >= 1 && text[eff_len - 1] == '=') {
            eff_len -= 1;
            if (eff_len >= 1 && text[eff_len - 1] == '=') {
                eff_len -= 1;
            }
        }
    } else {
        // Trim optional '$' padding if present
        while (eff_len > 0 && text[eff_len - 1] == '$') {
            eff_len -= 1;
        }
    }

    if ((eff_len & 3) == 1) {
        return -1;
    }

    let i: usize = 0;
    let o: usize = 0;

    // 16-lane SIMD ASCII upper-bound fast-path + comptime 256-entry LUT decode (16 chars -> 12 bytes)
    const v_high: LzsVec16U8 = @splat(0x80 as u8);
    while (i + 16 <= eff_len && o + 12 <= out.len) {
        const chunk: LzsVec16U8 = text[i .. i + 16][0..16].*;
        if (@reduce(.Or, chunk >= v_high)) {
            return -1;
        }
        let g: usize = 0;
        while (g < 4) : (g += 1) {
            const p = i + g * 4;
            const v0: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[p]] else LZS_B64_DEC_LUT[text[p]];
            const v1: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[p + 1]] else LZS_B64_DEC_LUT[text[p + 1]];
            const v2: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[p + 2]] else LZS_B64_DEC_LUT[text[p + 2]];
            const v3: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[p + 3]] else LZS_B64_DEC_LUT[text[p + 3]];
            if (((v0 | v1 | v2 | v3) & 0xC0) != 0) {
                return -1;
            }
            const triple: u32 = (u32(v0) << 18) | (u32(v1) << 12) | (u32(v2) << 6) | u32(v3);
            out[o] = u8((triple >> 16) & 0xFF);
            out[o + 1] = u8((triple >> 8) & 0xFF);
            out[o + 2] = u8(triple & 0xFF);
            o += 3;
        }
        i += 16;
    }

    while (i + 4 <= eff_len) : (i += 4) {
        const v0: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[i]] else LZS_B64_DEC_LUT[text[i]];
        const v1: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[i + 1]] else LZS_B64_DEC_LUT[text[i + 1]];
        const v2: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[i + 2]] else LZS_B64_DEC_LUT[text[i + 2]];
        const v3: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[i + 3]] else LZS_B64_DEC_LUT[text[i + 3]];
        if (((v0 | v1 | v2 | v3) & 0xC0) != 0) {
            return -1;
        }
        if (o + 3 > out.len) {
            return -1;
        }
        const triple: u32 = (u32(v0) << 18) | (u32(v1) << 12) | (u32(v2) << 6) | u32(v3);
        out[o] = u8((triple >> 16) & 0xFF);
        out[o + 1] = u8((triple >> 8) & 0xFF);
        out[o + 2] = u8(triple & 0xFF);
        o += 3;
    }

    const rem = eff_len - i;
    if (rem == 2) {
        const v0: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[i]] else LZS_B64_DEC_LUT[text[i]];
        const v1: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[i + 1]] else LZS_B64_DEC_LUT[text[i + 1]];
        if (((v0 | v1) & 0xC0) != 0) {
            return -1;
        }
        if (o + 1 > out.len) {
            return -1;
        }
        out[o] = u8(((u32(v0) << 2) | (u32(v1) >> 4)) & 0xFF);
        o += 1;
    } else if (rem == 3) {
        const v0: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[i]] else LZS_B64_DEC_LUT[text[i]];
        const v1: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[i + 1]] else LZS_B64_DEC_LUT[text[i + 1]];
        const v2: u8 = if (uri_mode) LZS_URI_DEC_LUT[text[i + 2]] else LZS_B64_DEC_LUT[text[i + 2]];
        if (((v0 | v1 | v2) & 0xC0) != 0) {
            return -1;
        }
        if (o + 2 > out.len) {
            return -1;
        }
        out[o] = u8(((u32(v0) << 2) | (u32(v1) >> 4)) & 0xFF);
        out[o + 1] = u8(((u32(v1) << 4) | (u32(v2) >> 2)) & 0xFF);
        o += 2;
    }

    return i64(o);
}

/// Compress string to standard Base64. Returns encoded byte length.
pub fn compressToBase64(input: string, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    let tmp: [32768]u8 = undefined;
    const clen = compressBytes(input, tmp[0..]);
    if (clen == 0) {
        return 0;
    }
    return lzsEncode64(tmp[0..clen], LZS_B64_ALPHA, true, out);
}

/// Decompress standard Base64 string into `out`. Returns decompressed length or -1.
pub fn decompressFromBase64(b64: string, out: []u8): i64 {
    if (b64.len == 0) {
        return 0;
    }
    let tmp: [32768]u8 = undefined;
    const rlen = lzsDecode64(b64, false, tmp[0..]);
    if (rlen <= 0) {
        return -1;
    }
    return decompressBytes(tmp[0..usize(rlen)], out);
}

/// Compress string to URL-safe encoded component (alphabet A-Za-z0-9+-$).
/// Returns encoded byte length.
pub fn compressToEncodedURIComponent(input: string, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    let tmp: [32768]u8 = undefined;
    const clen = compressBytes(input, tmp[0..]);
    if (clen == 0) {
        return 0;
    }
    // Encode with URI-safe 64-char base (A-Za-z0-9+-) and '$' for 4-char alignment
    const elen = lzsEncode64(tmp[0..clen], LZS_URI_ALPHA, false, out);
    if (elen == 0) {
        return 0;
    }
    let o = elen;
    const rem = o & 3;
    if (rem != 0) {
        const pad_n = 4 - rem;
        if (o + pad_n > out.len) {
            return 0;
        }
        let p: usize = 0;
        while (p < pad_n) : (p += 1) {
            out[o] = '$';
            o += 1;
        }
    }
    return o;
}

/// Decompress URL-safe encoded component (alphabet A-Za-z0-9+-$) into `out`.
/// Returns decompressed length or -1 on invalid input.
pub fn decompressFromEncodedURIComponent(uri: string, out: []u8): i64 {
    if (uri.len == 0) {
        return 0;
    }
    let tmp: [32768]u8 = undefined;
    const rlen = lzsDecode64(uri, true, tmp[0..]);
    if (rlen <= 0) {
        return -1;
    }
    return decompressBytes(tmp[0..usize(rlen)], out);
}
