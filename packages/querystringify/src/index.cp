// @cap/form-urlencoded — Zero-allocation WHATWG application/x-www-form-urlencoded & percent-encoding
// Inspired by Rust `form_urlencoded` & `percent-encoding`
// MIT License
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry LUTs, and @memcpy.

type QsVec16 = @Vector(16, u8);

const QS_HEX_UPPER = "0123456789ABCDEF";

fn qsBuildFormSafeLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = 1;
        c += 1;
    }
    c = 65;
    while (c <= 90) {
        lut[c] = 1;
        c += 1;
    }
    c = 97;
    while (c <= 122) {
        lut[c] = 1;
        c += 1;
    }
    lut[45] = 1; // '-'
    lut[46] = 1; // '.'
    lut[95] = 1; // '_'
    lut[42] = 1; // '*'
    return lut;
}

fn qsBuildHexDecodeLut(): [256]u8 {
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

const QS_FORM_SAFE_LUT: [256]u8 = qsBuildFormSafeLut();
const QS_HEX_DECODE_LUT: [256]u8 = qsBuildHexDecodeLut();

fn isFormSafe(b: u8): bool {
    return QS_FORM_SAFE_LUT[b as usize] != 0;
}

fn qsFindPercent(input: string, start: usize): usize {
    let i: usize = start;
    const len = input.len;
    const v_pct: QsVec16 = @splat(37 as u8);
    while (i + 16 <= len) {
        const v: QsVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_pct)) {
            let k: usize = 0;
            while (k < 16) {
                if (input[i + k] == 37) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (input[i] == 37) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn qsFindPercentOrPlus(input: string, start: usize): usize {
    let i: usize = start;
    const len = input.len;
    const v_pct: QsVec16 = @splat(37 as u8);
    const v_plus: QsVec16 = @splat(43 as u8);
    while (i + 16 <= len) {
        const v: QsVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_pct) || @reduce(.Or, v == v_plus)) {
            let k: usize = 0;
            while (k < 16) {
                const c = input[i + k];
                if (c == 37 || c == 43) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = input[i];
        if (c == 37 || c == 43) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn qsFindAmp(input: string, start: usize): usize {
    let i: usize = start;
    const len = input.len;
    const v_amp: QsVec16 = @splat(38 as u8);
    while (i + 16 <= len) {
        const v: QsVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_amp)) {
            let k: usize = 0;
            while (k < 16) {
                if (input[i + k] == 38) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (input[i] == 38) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn qsFindEq(input: string, start: usize, end: usize): i64 {
    let i: usize = start;
    const v_eq: QsVec16 = @splat(61 as u8);
    while (i + 16 <= end) {
        const v: QsVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_eq)) {
            let k: usize = 0;
            while (k < 16) {
                if (input[i + k] == 61) {
                    return (i + k) as i64;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < end) {
        if (input[i] == 61) {
            return i as i64;
        }
        i += 1;
    }
    return -1;
}

fn encodedSize(input: string, space_as_plus: bool): usize {
    let need: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const b: u8 = input[i];
        if ((b == 32 && space_as_plus) || QS_FORM_SAFE_LUT[b as usize] != 0) {
            need += 1;
        } else {
            need += 3;
        }
        i += 1;
    }
    return need;
}

pub fn percentEncode(input: string, space_as_plus: bool, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    const need: usize = encodedSize(input, space_as_plus);
    if (out.len < need) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const safe_start: usize = i;
        while (i < input.len && QS_FORM_SAFE_LUT[input[i] as usize] != 0) {
            i += 1;
        }
        const safe_len: usize = i - safe_start;
        if (safe_len > 0) {
            @memcpy(out[pos .. pos + safe_len], input[safe_start .. i]);
            pos += safe_len;
        }
        if (i >= input.len) {
            break;
        }
        const b: u8 = input[i];
        if (b == 32 && space_as_plus) {
            out[pos] = 43; // '+'
            pos += 1;
        } else {
            out[pos] = 37; // '%'
            out[pos + 1] = QS_HEX_UPPER[((b >> 4) & 15) as usize];
            out[pos + 2] = QS_HEX_UPPER[(b & 15) as usize];
            pos += 3;
        }
        i += 1;
    }
    return pos;
}

fn decodedSize(input: string): usize {
    let need: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const pct: usize = qsFindPercent(input, i);
        need += pct - i;
        i = pct;
        if (i >= input.len) {
            break;
        }
        if (i + 2 < input.len) {
            const h1 = QS_HEX_DECODE_LUT[input[i + 1] as usize];
            const h2 = QS_HEX_DECODE_LUT[input[i + 2] as usize];
            if ((h1 | h2) < 16) {
                need += 1;
                i += 3;
                continue;
            }
        }
        need += 1;
        i += 1;
    }
    return need;
}

pub fn percentDecode(input: string, plus_as_space: bool, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    const need: usize = decodedSize(input);
    if (out.len < need) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const stop: usize = if (plus_as_space) qsFindPercentOrPlus(input, i) else qsFindPercent(input, i);
        const chunk: usize = stop - i;
        if (chunk > 0) {
            @memcpy(out[pos .. pos + chunk], input[i .. stop]);
            pos += chunk;
            i = stop;
        }
        if (i >= input.len) {
            break;
        }
        const b: u8 = input[i];
        if (plus_as_space && b == 43) {
            out[pos] = 32;
            pos += 1;
            i += 1;
        } else if (b == 37 && i + 2 < input.len) {
            const h1 = QS_HEX_DECODE_LUT[input[i + 1] as usize];
            const h2 = QS_HEX_DECODE_LUT[input[i + 2] as usize];
            if ((h1 | h2) < 16) {
                out[pos] = (h1 << 4) | h2;
                pos += 1;
                i += 3;
            } else {
                out[pos] = b;
                pos += 1;
                i += 1;
            }
        } else {
            out[pos] = b;
            pos += 1;
            i += 1;
        }
    }
    return pos;
}

pub struct FormPair {
    pub key_start: usize,
    pub key_len: usize,
    pub val_start: usize,
    pub val_len: usize,
}

pub fn parseFormPairs(query: string, out: []FormPair): usize {
    if (query.len == 0 || out.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    if (query[0] == 63) { // '?'
        pos = 1;
    }
    let count: usize = 0;
    while (pos < query.len && count < out.len) {
        const seg_start: usize = pos;
        const seg_end: usize = qsFindAmp(query, pos);
        const eq_pos: i64 = qsFindEq(query, seg_start, seg_end);
        pos = seg_end;
        if (pos < query.len && query[pos] == 38) {
            pos += 1;
        }
        if (seg_end > seg_start) {
            if (eq_pos >= 0) {
                const eq: usize = eq_pos as usize;
                out[count] = FormPair {
                    key_start: seg_start,
                    key_len: eq - seg_start,
                    val_start: eq + 1,
                    val_len: seg_end - (eq + 1),
                };
            } else {
                out[count] = FormPair {
                    key_start: seg_start,
                    key_len: seg_end - seg_start,
                    val_start: seg_end,
                    val_len: 0,
                };
            }
            count += 1;
        }
    }
    return count;
}

pub struct FormSerializer {
    pub len: usize,
    pub count: usize,

    pub static fn new(): FormSerializer {
        return FormSerializer { len: 0, count: 0 };
    }

    pub fn appendPair(*_, out: []u8, key: string, val: string): bool {
        let sep_len: usize = 0;
        if (_.count > 0) {
            sep_len = 1;
        }
        const k_len: usize = encodedSize(key, true);
        const v_len: usize = encodedSize(val, true);
        const needed: usize = sep_len + k_len + 1 + v_len;
        if (needed > out.len - _.len) {
            return false;
        }
        let pos: usize = _.len;
        if (sep_len == 1) {
            out[pos] = 38; // '&'
            pos += 1;
        }
        if (k_len > 0) {
            const kn: usize = percentEncode(key, true, out[pos..out.len]);
            pos += kn;
        }
        out[pos] = 61; // '='
        pos += 1;
        if (v_len > 0) {
            const vn: usize = percentEncode(val, true, out[pos..out.len]);
            pos += vn;
        }
        _.len = pos;
        _.count += 1;
        return true;
    }
}

fn decodedMatches(raw: string, plus_as_space: bool, target: string): bool {
    let i: usize = 0;
    let ti: usize = 0;
    while (i < raw.len) {
        let ch: u8 = raw[i];
        let step: usize = 1;
        if (plus_as_space && ch == 43) {
            ch = 32;
        } else if (ch == 37 && i + 2 < raw.len) {
            const h1 = QS_HEX_DECODE_LUT[raw[i + 1] as usize];
            const h2 = QS_HEX_DECODE_LUT[raw[i + 2] as usize];
            if ((h1 | h2) < 16) {
                ch = (h1 << 4) | h2;
                step = 3;
            }
        }
        if (ti >= target.len || target[ti] != ch) {
            return false;
        }
        ti += 1;
        i += step;
    }
    return ti == target.len;
}

pub fn getParamDecoded(query: string, target_key: string, out: []u8): i64 {
    if (query.len == 0) {
        return -1;
    }
    let pos: usize = 0;
    if (query[0] == 63) {
        pos = 1;
    }
    while (pos < query.len) {
        const seg_start: usize = pos;
        const seg_end: usize = qsFindAmp(query, pos);
        const eq_pos: i64 = qsFindEq(query, seg_start, seg_end);
        pos = seg_end;
        if (pos < query.len && query[pos] == 38) {
            pos += 1;
        }
        if (seg_end > seg_start) {
            let k_end: usize = seg_end;
            let v_start: usize = seg_end;
            if (eq_pos >= 0) {
                k_end = eq_pos as usize;
                v_start = k_end + 1;
            }
            const raw_key: string = query[seg_start..k_end];
            if (decodedMatches(raw_key, true, target_key)) {
                const raw_val: string = query[v_start..seg_end];
                if (raw_val.len == 0) {
                    return 0;
                }
                const need: usize = decodedSize(raw_val);
                if (out.len < need) {
                    return -1;
                }
                const written: usize = percentDecode(raw_val, true, out);
                return written as i64;
            }
        }
    }
    return -1;
}
