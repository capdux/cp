// @cap/error-ex — Zero-heap structured error and context formatter.

pub struct ErrorField {
    key: string,
    value: string,
    line_mode: bool,
}

pub struct StructuredError {
    name: string,
    code: i32,
    message: string,
}

fn erexWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn erexWriteStr(out: []u8, pos: usize, s: string): usize {
    let cur = pos;
    let i: usize = 0;
    while (i < s.len && cur < out.len) {
        out[cur] = s[i];
        cur += 1;
        i += 1;
    }
    return cur;
}

fn erexWriteI32(out: []u8, pos: usize, val: i32): usize {
    let cur = pos;
    let v: i64 = i64(val);
    if (v == 0) {
        return erexWriteByte(out, cur, '0');
    }
    if (v < 0) {
        cur = erexWriteByte(out, cur, '-');
        v = -v;
    }
    let u: u64 = u64(v);
    let digits: [16]u8 = undefined;
    let n: usize = 0;
    while (u > 0 && n < 16) {
        const rem: u8 = u8(u % 10);
        digits[n] = u8(48) + rem;
        u = u / 10;
        n += 1;
    }
    while (n > 0) {
        n -= 1;
        cur = erexWriteByte(out, cur, digits[n]);
    }
    return cur;
}

pub fn formatStructuredError(e: StructuredError, fields: []const ErrorField, out: []u8): usize {
    let pos: usize = 0;
    if (e.name.len > 0) {
        pos = erexWriteStr(out, pos, e.name);
    } else {
        pos = erexWriteStr(out, pos, "Error");
    }

    if (e.code != 0) {
        pos = erexWriteStr(out, pos, " [");
        pos = erexWriteI32(out, pos, e.code);
        pos = erexWriteByte(out, pos, ']');
    }

    pos = erexWriteStr(out, pos, ": ");
    pos = erexWriteStr(out, pos, e.message);

    // Inline fields: (key=value, ...)
    let inline_count: usize = 0;
    let i: usize = 0;
    while (i < fields.len) {
        if (!fields[i].line_mode) {
            if (inline_count == 0) {
                pos = erexWriteStr(out, pos, " (");
            } else {
                pos = erexWriteStr(out, pos, ", ");
            }
            pos = erexWriteStr(out, pos, fields[i].key);
            pos = erexWriteByte(out, pos, '=');
            pos = erexWriteStr(out, pos, fields[i].value);
            inline_count += 1;
        }
        i += 1;
    }
    if (inline_count > 0) {
        pos = erexWriteByte(out, pos, ')');
    }

    // Line-mode fields: \n  key: value
    i = 0;
    while (i < fields.len) {
        if (fields[i].line_mode) {
            pos = erexWriteStr(out, pos, "\n  ");
            pos = erexWriteStr(out, pos, fields[i].key);
            pos = erexWriteStr(out, pos, ": ");
            pos = erexWriteStr(out, pos, fields[i].value);
        }
        i += 1;
    }

    return pos;
}

fn erexMatchesPlaceholder(tmpl: string, start: usize, key: string): bool {
    if (key.len == 0) return false;
    if (start + 2 + key.len > tmpl.len) return false;
    if (tmpl[start] != '{') return false;
    if (tmpl[start + 1 + key.len] != '}') return false;
    let i: usize = 0;
    while (i < key.len) {
        if (tmpl[start + 1 + i] != key[i]) return false;
        i += 1;
    }
    return true;
}

pub fn formatErrorTemplate(tmpl: string, key: string, val: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < tmpl.len && pos < out.len) {
        if (erexMatchesPlaceholder(tmpl, i, key)) {
            pos = erexWriteStr(out, pos, val);
            i += 2 + key.len;
        } else {
            pos = erexWriteByte(out, pos, tmpl[i]);
            i += 1;
        }
    }
    return pos;
}
