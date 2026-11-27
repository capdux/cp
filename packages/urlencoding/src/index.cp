// @cap/urlencoding — Zero-heap RFC 3986 percent-encoding/decoding (crates.io:urlencoding v2)
// Accelerated with comptime 256-entry LUTs, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

type UencVec16 = @Vector(16, u8);

const UENC_HEX_UPPER: string = "0123456789ABCDEF";

fn uencBuildUnreservedLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57) ||
            c == 45 || c == 95 || c == 46 || c == 126)
        {
            lut[c] = 1;
        }
        c += 1;
    }
    return lut;
}

fn uencBuildHexValLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = (c - 48) as u8;
        c += 1;
    }
    c = 65;
    while (c <= 70) {
        lut[c] = (c - 55) as u8;
        c += 1;
    }
    c = 97;
    while (c <= 102) {
        lut[c] = (c - 87) as u8;
        c += 1;
    }
    return lut;
}

const UENC_UNRESERVED_LUT: [256]u8 = uencBuildUnreservedLut();
const UENC_HEX_VAL_LUT: [256]u8 = uencBuildHexValLut();

fn uencFindReserved(data: []const u8, start: usize): usize {
    let i = start;
    const len = data.len;
    const v_a: UencVec16 = @splat(97 as u8);
    const v_z: UencVec16 = @splat(122 as u8);
    const v_0: UencVec16 = @splat(48 as u8);
    const v_9: UencVec16 = @splat(57 as u8);
    const v_case: UencVec16 = @splat(32 as u8);
    const v_dash: UencVec16 = @splat(45 as u8);
    const v_dot: UencVec16 = @splat(46 as u8);
    const v_under: UencVec16 = @splat(95 as u8);
    const v_tilde: UencVec16 = @splat(126 as u8);
    while (i + 16 <= len) {
        const v: UencVec16 = data[i .. i + 16][0..16].*;
        const vl = v | v_case;
        const ok = ((vl >= v_a) & (vl <= v_z)) |
            ((v >= v_0) & (v <= v_9)) |
            (v == v_dash) | (v == v_dot) | (v == v_under) | (v == v_tilde);
        if (!@reduce(.And, ok)) {
            let k: usize = 0;
            while (k < 16) {
                if (UENC_UNRESERVED_LUT[data[i + k] as usize] == 0) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (UENC_UNRESERVED_LUT[data[i] as usize] == 0) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn uencFindPercent(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_pct: UencVec16 = @splat(37 as u8);
    while (i + 16 <= len) {
        const v: UencVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_pct)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == 37) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != 37) {
        i += 1;
    }
    return i;
}

/// Return true if byte `b` is an RFC 3986 unreserved character:
/// `A-Z`, `a-z`, `0-9`, `-`, `_`, `.`, `~`.
pub fn isUnreserved(b: u8): bool {
    return UENC_UNRESERVED_LUT[b as usize] != 0;
}

/// Compute the exact encoded byte length for `input` under RFC 3986.
pub fn encodedLen(input: string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const next = uencFindReserved(input, i);
        total += next - i;
        if (next >= input.len) {
            break;
        }
        total += 3;
        i = next + 1;
    }
    return total;
}

/// Percent-encode arbitrary bytes into `out` using uppercase hex `%XX`.
/// Returns the number of bytes written to `out`.
pub fn encodeBinary(data: []const u8, out: []u8): usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < data.len && w < out.len) {
        const next = uencFindReserved(data, i);
        if (next > i) {
            const chunk = next - i;
            const avail = out.len - w;
            const copy_len = if (chunk < avail) chunk else avail;
            @memcpy(out[w .. w + copy_len], data[i .. i + copy_len]);
            w += copy_len;
            i += copy_len;
            if (w >= out.len || i >= data.len) {
                break;
            }
        }
        if (w + 3 > out.len) {
            break;
        }
        const b = data[i];
        out[w] = '%' as u8;
        out[w + 1] = UENC_HEX_UPPER[(b >> 4) as usize];
        out[w + 2] = UENC_HEX_UPPER[(b & 0x0F) as usize];
        w += 3;
        i += 1;
    }
    return w;
}

/// Percent-encode a UTF-8 string into `out` using RFC 3986 unreserved rules and uppercase `%XX`.
/// Returns the number of bytes written to `out`.
pub fn encode(input: string, out: []u8): usize {
    return encodeBinary(input, out);
}

/// Strictly decode percent-encoded `input` into `out`.
/// Returns the number of bytes written, or `-1` on malformed `%XX` escape or insufficient `out` buffer.
pub fn decode(input: string, out: []u8): i64 {
    let w: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const next = uencFindPercent(input, i);
        if (next > i) {
            const chunk = next - i;
            if (w + chunk > out.len) {
                return -1;
            }
            @memcpy(out[w .. w + chunk], input[i..next]);
            w += chunk;
            i = next;
            if (i >= input.len) {
                break;
            }
        }
        if (i + 2 >= input.len) {
            return -1;
        }
        const hi = UENC_HEX_VAL_LUT[input[i + 1] as usize];
        const lo = UENC_HEX_VAL_LUT[input[i + 2] as usize];
        if (((hi | lo) & 0xF0) != 0) {
            return -1;
        }
        if (w >= out.len) {
            return -1;
        }
        out[w] = (hi << 4) | lo;
        w += 1;
        i += 3;
    }
    return w as i64;
}

/// Lossy percent-decoding: valid `%XX` escapes are decoded; invalid or truncated `%` sequences are passed through verbatim.
/// Returns the number of bytes written to `out`.
pub fn decodeLossy(input: string, out: []u8): usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < input.len && w < out.len) {
        const next = uencFindPercent(input, i);
        if (next > i) {
            const chunk = next - i;
            const avail = out.len - w;
            const copy_len = if (chunk < avail) chunk else avail;
            @memcpy(out[w .. w + copy_len], input[i .. i + copy_len]);
            w += copy_len;
            i += copy_len;
            if (w >= out.len || i >= input.len) {
                break;
            }
        }
        if (i + 2 < input.len) {
            const hi = UENC_HEX_VAL_LUT[input[i + 1] as usize];
            const lo = UENC_HEX_VAL_LUT[input[i + 2] as usize];
            if (((hi | lo) & 0xF0) == 0) {
                out[w] = (hi << 4) | lo;
                w += 1;
                i += 3;
                continue;
            }
        }
        out[w] = input[i];
        w += 1;
        i += 1;
    }
    return w;
}
