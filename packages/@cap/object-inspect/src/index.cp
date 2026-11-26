// @cap/object-inspect — Zero-heap debug value inspector & formatter in pure CP
// Clean-room MIT implementation of npm `object-inspect` (#136, 262.6M/wk).

pub struct InspectOptions {
    pub quote_style_double: bool, // false = single quotes (default), true = double quotes
    pub max_string_len: usize,    // 0 = unlimited
}

pub fn defaultInspectOptions(): InspectOptions {
    return InspectOptions {
        quote_style_double: false,
        max_string_len: 0,
    };
}

fn oinWriteStr(s: string, out: []u8, pos: usize): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn oinHex(n: u8): u8 {
    if (n < 10) { return 48 + n; }
    return 87 + n; // 'a'..'f'
}

/// Formats a string with quotes, C/JS escape sequences (`\n`, `\r`, `\t`, `\\`, `\xHH`), and optional truncation.
pub fn inspectString(s: string, opts: InspectOptions, out: []u8): usize {
    const q: u8 = if (opts.quote_style_double) 34 else 39;
    let p: usize = 0;
    if (p < out.len) { out[p] = q; p += 1; }

    let limit = s.len;
    let truncated = false;
    if (opts.max_string_len > 0 && limit > opts.max_string_len) {
        limit = opts.max_string_len;
        truncated = true;
    }

    let i: usize = 0;
    while (i < limit) {
        const b = s[i];
        if (b == 92) {
            p = oinWriteStr("\\\\", out, p);
        } else if (b == 10) {
            p = oinWriteStr("\\n", out, p);
        } else if (b == 13) {
            p = oinWriteStr("\\r", out, p);
        } else if (b == 9) {
            p = oinWriteStr("\\t", out, p);
        } else if (b == q) {
            if (p + 1 < out.len) {
                out[p] = 92;
                out[p + 1] = q;
                p += 2;
            }
        } else if (b < 32 || b == 127) {
            if (p + 3 < out.len) {
                out[p] = 92;
                out[p + 1] = 120; // 'x'
                out[p + 2] = oinHex((b >> 4) & 0x0F);
                out[p + 3] = oinHex(b & 0x0F);
                p += 4;
            }
        } else {
            if (p < out.len) { out[p] = b; p += 1; }
        }
        i += 1;
    }

    if (truncated) {
        p = oinWriteStr("...", out, p);
    }
    if (p < out.len) { out[p] = q; p += 1; }
    return p;
}

/// Formats a boolean value (`"true"` / `"false"`).
pub fn inspectBool(v: bool, out: []u8): usize {
    if (v) {
        return oinWriteStr("true", out, 0);
    }
    return oinWriteStr("false", out, 0);
}

/// Formats a signed 64-bit integer into `out`.
pub fn inspectInt(val: i64, out: []u8): usize {
    let p: usize = 0;
    let mag: u64 = 0;
    if (val < 0) {
        p = oinWriteStr("-", out, p);
        mag = (0 - val) as u64;
    } else {
        mag = val as u64;
    }
    let digits: [24]u8 = [0; 24];
    let n: usize = 0;
    if (mag == 0) {
        digits[0] = 48;
        n = 1;
    } else {
        while (mag > 0 && n < 24) {
            digits[n] = ((mag % 10) as u8) + 48;
            mag = mag / 10;
            n += 1;
        }
    }
    let k = n;
    while (k > 0 && p < out.len) {
        k -= 1;
        out[p] = digits[k];
        p += 1;
    }
    return p;
}

/// Formats a byte slice as `<Buffer de ad be ef ...>` up to `max_bytes` preview bytes.
pub fn inspectBytes(data: []const u8, max_bytes: usize, out: []u8): usize {
    let p = oinWriteStr("<Buffer", out, 0);
    let limit = data.len;
    let truncated = false;
    if (max_bytes > 0 && limit > max_bytes) {
        limit = max_bytes;
        truncated = true;
    }
    let i: usize = 0;
    while (i < limit) {
        if (p + 2 < out.len) {
            out[p] = 32;
            out[p + 1] = oinHex((data[i] >> 4) & 0x0F);
            out[p + 2] = oinHex(data[i] & 0x0F);
            p += 3;
        }
        i += 1;
    }
    if (truncated) {
        p = oinWriteStr(" ...", out, p);
    }
    p = oinWriteStr(">", out, p);
    return p;
}

/// Formats an array of strings as `[ 'a', 'b', 'c' ]`.
pub fn inspectStringArray(items: []const string, opts: InspectOptions, out: []u8): usize {
    if (items.len == 0) {
        return oinWriteStr("[]", out, 0);
    }
    let p = oinWriteStr("[ ", out, 0);
    let i: usize = 0;
    while (i < items.len) {
        if (i > 0) {
            p = oinWriteStr(", ", out, p);
        }
        if (p < out.len) {
            p += inspectString(items[i], opts, out[p..out.len]);
        }
        i += 1;
    }
    p = oinWriteStr(" ]", out, p);
    return p;
}

export {
    InspectOptions,
    defaultInspectOptions,
    inspectString,
    inspectBool,
    inspectInt,
    inspectBytes,
    inspectStringArray,
};
