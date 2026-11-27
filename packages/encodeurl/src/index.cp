// @cap/encodeurl — Zero-heap idempotent URL encoder modeled on npm `encodeurl` v2 (180M+/week).
// Clean-room MIT implementation: percent-encodes spaces, control chars, non-ASCII UTF-8 bytes,
// quotes, `<`, `>`, `\`, `^`, `` ` ``, `{`, `|`, `}`, and lone `%`, while preserving valid `%XX` sequences.
// Accelerated with comptime 256-entry LUTs, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

type EuVec16 = @Vector(16, u8);

const EU_HEX_UPPER = "0123456789ABCDEF";

fn euBuildNeedEncLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if (c <= 32 || c >= 127 || c == 34 || c == 60 || c == 62 ||
            c == 92 || c == 94 || c == 96 || c == 123 || c == 124 || c == 125)
        {
            lut[c] = 1;
        }
        c += 1;
    }
    return lut;
}

fn euBuildHexValLut(): [256]u8 {
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

fn euBuildUpperLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if (c >= 97 && c <= 122) {
            lut[c] = (c - 32) as u8;
        } else {
            lut[c] = c as u8;
        }
        c += 1;
    }
    return lut;
}

const EU_NEED_ENC_LUT: [256]u8 = euBuildNeedEncLut();
const EU_HEX_VAL_LUT: [256]u8 = euBuildHexValLut();
const EU_UPPER_LUT: [256]u8 = euBuildUpperLut();

fn euFindSpecial(url: string, start: usize): usize {
    let i: usize = start;
    const len = url.len;
    const v_sp: EuVec16 = @splat(32 as u8);
    const v_lb: EuVec16 = @splat(123 as u8);
    const v_pct: EuVec16 = @splat(37 as u8);
    const v_qt: EuVec16 = @splat(34 as u8);
    const v_lt: EuVec16 = @splat(60 as u8);
    const v_gt: EuVec16 = @splat(62 as u8);
    const v_bs: EuVec16 = @splat(92 as u8);
    const v_ct: EuVec16 = @splat(94 as u8);
    const v_bt: EuVec16 = @splat(96 as u8);
    while (i + 16 <= len) {
        const v: EuVec16 = url[i .. i + 16][0..16].*;
        if (@reduce(.Or, v <= v_sp) ||
            @reduce(.Or, v >= v_lb) ||
            @reduce(.Or, v == v_pct) ||
            @reduce(.Or, v == v_qt) ||
            @reduce(.Or, v == v_lt) ||
            @reduce(.Or, v == v_gt) ||
            @reduce(.Or, v == v_bs) ||
            @reduce(.Or, v == v_ct) ||
            @reduce(.Or, v == v_bt))
        {
            let k: usize = 0;
            while (k < 16) {
                const b = url[i + k];
                if (b == 37 || EU_NEED_ENC_LUT[b as usize] != 0) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const b = url[i];
        if (b == 37 || EU_NEED_ENC_LUT[b as usize] != 0) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn euFindPercent(url: string, start: usize): usize {
    let i: usize = start;
    const len = url.len;
    const v_pct: EuVec16 = @splat(37 as u8);
    while (i + 16 <= len) {
        const v: EuVec16 = url[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_pct)) {
            let k: usize = 0;
            while (k < 16) {
                if (url[i + k] == 37) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (url[i] == 37) {
            return i;
        }
        i += 1;
    }
    return len;
}

/// Returns true if byte `b` must be percent-encoded by `encodeUrl`.
/// Encodes CTL (<= 0x20, 0x7F), non-ASCII (>= 0x80), and unsafe ASCII:
/// `"` (34), `<` (60), `>` (62), `\` (92), `^` (94), `` ` `` (96), `{` (123), `|` (124), `}` (125).
pub fn needsUrlEncode(b: u8): bool {
    return EU_NEED_ENC_LUT[b as usize] != 0;
}

/// Computes the exact byte length `encodeUrl` will produce for `url` without writing any buffer.
pub fn encodedUrlLen(url: string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < url.len) {
        const next_sp = euFindSpecial(url, i);
        total = total + (next_sp - i);
        i = next_sp;
        if (i >= url.len) { break; }
        const b = url[i];
        if (b == 37) {
            if (i + 2 < url.len && EU_HEX_VAL_LUT[url[i + 1] as usize] != 255 && EU_HEX_VAL_LUT[url[i + 2] as usize] != 255) {
                total = total + 3;
                i = i + 3;
                continue;
            }
            total = total + 3;
            i = i + 1;
            continue;
        }
        total = total + 3;
        i = i + 1;
    }
    return total;
}

/// Encodes a URL string into `out`, preserving existing valid `%XX` sequences (idempotent).
pub fn encodeUrl(url: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < url.len) {
        const next_sp = euFindSpecial(url, i);
        if (next_sp > i) {
            const seg_len = next_sp - i;
            if (p < out.len) {
                const avail = out.len - p;
                const n = if (seg_len <= avail) { seg_len } else { avail };
                @memcpy(out[p .. p + n], url[i .. i + n]);
                p = p + n;
            }
            i = next_sp;
            if (i >= url.len) { break; }
        }
        const b = url[i];
        if (b == 37) { // '%'
            if (i + 2 < url.len && EU_HEX_VAL_LUT[url[i + 1] as usize] != 255 && EU_HEX_VAL_LUT[url[i + 2] as usize] != 255) {
                // Already valid %XX sequence — preserve verbatim
                if (p + 3 <= out.len) {
                    out[p] = 37;
                    out[p + 1] = url[i + 1];
                    out[p + 2] = url[i + 2];
                    p = p + 3;
                }
                i = i + 3;
                continue;
            }
            // Lone '%' -> "%25"
            if (p + 3 <= out.len) {
                out[p] = 37;
                out[p + 1] = 50; // '2'
                out[p + 2] = 53; // '5'
                p = p + 3;
            }
            i = i + 1;
            continue;
        }
        if (p + 3 <= out.len) {
            out[p] = 37;
            out[p + 1] = EU_HEX_UPPER[(b >> 4) as usize];
            out[p + 2] = EU_HEX_UPPER[(b & 15) as usize];
            p = p + 3;
        }
        i = i + 1;
    }
    return p;
}

/// Checked variant of `encodeUrl`: returns `null` if `out` cannot hold the full encoded URL.
pub fn encodeUrlChecked(url: string, out: []u8): usize? {
    let req = encodedUrlLen(url);
    if (req > out.len) { return null; }
    return encodeUrl(url, out);
}

/// Encodes `url` like `encodeUrl` and additionally normalizes existing `%xx` hex digits
/// to uppercase `%XX` per RFC 3986 §6.2.2.1.
pub fn encodeUrlNormalized(url: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < url.len) {
        const next_sp = euFindSpecial(url, i);
        if (next_sp > i) {
            const seg_len = next_sp - i;
            if (p < out.len) {
                const avail = out.len - p;
                const n = if (seg_len <= avail) { seg_len } else { avail };
                @memcpy(out[p .. p + n], url[i .. i + n]);
                p = p + n;
            }
            i = next_sp;
            if (i >= url.len) { break; }
        }
        const b = url[i];
        if (b == 37) {
            if (i + 2 < url.len && EU_HEX_VAL_LUT[url[i + 1] as usize] != 255 && EU_HEX_VAL_LUT[url[i + 2] as usize] != 255) {
                if (p + 3 <= out.len) {
                    out[p] = 37;
                    out[p + 1] = EU_UPPER_LUT[url[i + 1] as usize];
                    out[p + 2] = EU_UPPER_LUT[url[i + 2] as usize];
                    p = p + 3;
                }
                i = i + 3;
                continue;
            }
            if (p + 3 <= out.len) {
                out[p] = 37;
                out[p + 1] = 50; // '2'
                out[p + 2] = 53; // '5'
                p = p + 3;
            }
            i = i + 1;
            continue;
        }
        if (p + 3 <= out.len) {
            out[p] = 37;
            out[p + 1] = EU_HEX_UPPER[(b >> 4) as usize];
            out[p + 2] = EU_HEX_UPPER[(b & 15) as usize];
            p = p + 3;
        }
        i = i + 1;
    }
    return p;
}

/// Percent-decodes valid `%XX` sequences in `url` into `out`, preserving malformed `%` verbatim.
pub fn decodeUrl(url: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < url.len && p < out.len) {
        const next_pct = euFindPercent(url, i);
        if (next_pct > i) {
            const seg_len = next_pct - i;
            const avail = out.len - p;
            const n = if (seg_len <= avail) { seg_len } else { avail };
            @memcpy(out[p .. p + n], url[i .. i + n]);
            p = p + n;
            i = i + n;
            if (i >= url.len || p >= out.len) { break; }
        }
        // url[i] == '%'
        if (i + 2 < url.len) {
            const hi = EU_HEX_VAL_LUT[url[i + 1] as usize];
            const lo = EU_HEX_VAL_LUT[url[i + 2] as usize];
            if (hi != 255 && lo != 255) {
                out[p] = (hi << 4) | lo;
                p = p + 1;
                i = i + 3;
                continue;
            }
        }
        out[p] = 37;
        p = p + 1;
        i = i + 1;
    }
    return p;
}

/// Returns true if `url` is already safely encoded and `encodeUrl` would not alter it.
pub fn isEncodedUrl(url: string): bool {
    let i: usize = 0;
    while (i < url.len) {
        const next_sp = euFindSpecial(url, i);
        if (next_sp >= url.len) { return true; }
        i = next_sp;
        const b = url[i];
        if (b == 37) {
            if (i + 2 >= url.len || EU_HEX_VAL_LUT[url[i + 1] as usize] == 255 || EU_HEX_VAL_LUT[url[i + 2] as usize] == 255) {
                return false;
            }
            i = i + 3;
            continue;
        }
        return false;
    }
    return true;
}
