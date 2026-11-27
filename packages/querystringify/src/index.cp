// @cap/form-urlencoded — Zero-allocation WHATWG application/x-www-form-urlencoded & percent-encoding
// Inspired by Rust `form_urlencoded` & `percent-encoding`
// MIT License

fn isFormSafe(b: u8): bool {
    if (b >= 65 && b <= 90) { return true; }  // A-Z
    if (b >= 97 && b <= 122) { return true; } // a-z
    if (b >= 48 && b <= 57) { return true; }  // 0-9
    // '-', '.', '_', '*'
    return b == 45 || b == 46 || b == 95 || b == 42;
}

fn upperHex(v: u8): u8 {
    if (v < 10) {
        return v + 48;
    }
    return (v - 10) + 65;
}

fn hexVal(c: u8): i32 {
    if (c >= 48 && c <= 57) { return (c - 48) as i32; }
    if (c >= 65 && c <= 70) { return (c - 65 + 10) as i32; }
    if (c >= 97 && c <= 102) { return (c - 97 + 10) as i32; }
    return -1;
}

fn encodedSize(input: string, space_as_plus: bool): usize {
    let need: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let b: u8 = input[i];
        if ((b == 32 && space_as_plus) || isFormSafe(b)) {
            need = need + 1;
        } else {
            need = need + 3;
        }
        i = i + 1;
    }
    return need;
}

pub fn percentEncode(input: string, space_as_plus: bool, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    let need: usize = encodedSize(input, space_as_plus);
    if (out.len < need) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let b: u8 = input[i];
        if (b == 32 && space_as_plus) {
            out[pos] = 43; // '+'
            pos = pos + 1;
        } else if (isFormSafe(b)) {
            out[pos] = b;
            pos = pos + 1;
        } else {
            out[pos] = 37; // '%'
            out[pos + 1] = upperHex((b >> 4) & 15);
            out[pos + 2] = upperHex(b & 15);
            pos = pos + 3;
        }
        i = i + 1;
    }
    return pos;
}

fn decodedSize(input: string): usize {
    let need: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == 37 && i + 2 < input.len) {
            let h1: i32 = hexVal(input[i + 1]);
            let h2: i32 = hexVal(input[i + 2]);
            if (h1 >= 0 && h2 >= 0) {
                need = need + 1;
                i = i + 3;
                continue;
            }
        }
        need = need + 1;
        i = i + 1;
    }
    return need;
}

pub fn percentDecode(input: string, plus_as_space: bool, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    let need: usize = decodedSize(input);
    if (out.len < need) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let b: u8 = input[i];
        if (plus_as_space && b == 43) {
            out[pos] = 32;
            pos = pos + 1;
            i = i + 1;
        } else if (b == 37 && i + 2 < input.len) {
            let h1: i32 = hexVal(input[i + 1]);
            let h2: i32 = hexVal(input[i + 2]);
            if (h1 >= 0 && h2 >= 0) {
                out[pos] = (((h1 as u8) << 4) | (h2 as u8));
                pos = pos + 1;
                i = i + 3;
            } else {
                out[pos] = b;
                pos = pos + 1;
                i = i + 1;
            }
        } else {
            out[pos] = b;
            pos = pos + 1;
            i = i + 1;
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
        let seg_start: usize = pos;
        let eq_pos: i64 = -1;
        while (pos < query.len && query[pos] != 38) { // '&'
            if (eq_pos < 0 && query[pos] == 61) { // '='
                eq_pos = pos as i64;
            }
            pos = pos + 1;
        }
        let seg_end: usize = pos;
        if (pos < query.len && query[pos] == 38) {
            pos = pos + 1;
        }
        if (seg_end > seg_start) {
            if (eq_pos >= 0) {
                let eq: usize = eq_pos as usize;
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
            count = count + 1;
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
        let k_len: usize = encodedSize(key, true);
        let v_len: usize = encodedSize(val, true);
        let needed: usize = sep_len + k_len + 1 + v_len;
        if (needed > out.len - _.len) {
            return false;
        }
        let pos: usize = _.len;
        if (sep_len == 1) {
            out[pos] = 38; // '&'
            pos = pos + 1;
        }
        if (k_len > 0) {
            let kn: usize = percentEncode(key, true, out[pos..out.len]);
            pos = pos + kn;
        }
        out[pos] = 61; // '='
        pos = pos + 1;
        if (v_len > 0) {
            let vn: usize = percentEncode(val, true, out[pos..out.len]);
            pos = pos + vn;
        }
        _.len = pos;
        _.count = _.count + 1;
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
            let h1: i32 = hexVal(raw[i + 1]);
            let h2: i32 = hexVal(raw[i + 2]);
            if (h1 >= 0 && h2 >= 0) {
                ch = ((h1 as u8) << 4) | (h2 as u8);
                step = 3;
            }
        }
        if (ti >= target.len || target[ti] != ch) {
            return false;
        }
        ti = ti + 1;
        i = i + step;
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
        let seg_start: usize = pos;
        let eq_pos: i64 = -1;
        while (pos < query.len && query[pos] != 38) {
            if (eq_pos < 0 && query[pos] == 61) {
                eq_pos = pos as i64;
            }
            pos = pos + 1;
        }
        let seg_end: usize = pos;
        if (pos < query.len && query[pos] == 38) {
            pos = pos + 1;
        }
        if (seg_end > seg_start) {
            let k_end: usize = seg_end;
            let v_start: usize = seg_end;
            if (eq_pos >= 0) {
                k_end = eq_pos as usize;
                v_start = k_end + 1;
            }
            let raw_key: string = query[seg_start..k_end];
            if (decodedMatches(raw_key, true, target_key)) {
                let raw_val: string = query[v_start..seg_end];
                if (raw_val.len == 0) {
                    return 0;
                }
                let need: usize = decodedSize(raw_val);
                if (out.len < need) {
                    return -1;
                }
                let written: usize = percentDecode(raw_val, true, out);
                return written as i64;
            }
        }
    }
    return -1;
}
