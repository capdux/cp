// Source: Absorbed from /data/dev/bun/src/url.zig | Synced: 2026-11-21T16:45:50Z (WHATWG application/x-www-form-urlencoded Parser & Builder)
// Pure-CP zero-allocation application/x-www-form-urlencoded parser, lookup, percent-codec, and builder.

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn hexUpper(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'A') + (v - 10);
}

fn isFormSafe(b: u8): bool {
    if ((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9')) return true;
    return b == '*' || b == '-' || b == '.' || b == '_';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// WHATWG application/x-www-form-urlencoded encoder (space -> `+`, unsafe bytes -> `%XX`).
export fn encodeFormComponent(raw: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c == ' ') {
            if (pos >= out.len) return null;
            out[pos] = '+';
            pos += 1;
        } else if (isFormSafe(c)) {
            if (pos >= out.len) return null;
            out[pos] = c;
            pos += 1;
        } else {
            if (pos + 3 > out.len) return null;
            out[pos] = '%';
            out[pos + 1] = hexUpper((c >> 4) & 0x0F);
            out[pos + 2] = hexUpper(c & 0x0F);
            pos += 3;
        }
        i += 1;
    }
    return out[0..pos];
}

/// WHATWG application/x-www-form-urlencoded decoder (`+` -> space, `%XX` -> byte).
/// Invalid `%` sequences are preserved verbatim per WHATWG URL Standard.
export fn decodeFormComponent(encoded: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < encoded.len) {
        const c = encoded[i];
        if (c == '+') {
            if (pos >= out.len) return null;
            out[pos] = ' ';
            pos += 1;
            i += 1;
        } else if (c == '%' && i + 2 < encoded.len) {
            const hi = hexVal(encoded[i + 1]);
            const lo = hexVal(encoded[i + 2]);
            if (hi != null && lo != null) {
                if (pos >= out.len) return null;
                out[pos] = (hi.? << 4) | lo.?;
                pos += 1;
                i += 3;
            } else {
                if (pos >= out.len) return null;
                out[pos] = c;
                pos += 1;
                i += 1;
            }
        } else {
            if (pos >= out.len) return null;
            out[pos] = c;
            pos += 1;
            i += 1;
        }
    }
    return out[0..pos];
}

export struct FormPairRaw {
    raw_key: []const u8,
    raw_value: []const u8,

    pub fn decodeKey(out: []u8): ([]const u8)? {
        return decodeFormComponent(_.raw_key, out);
    }

    pub fn decodeValue(out: []u8): ([]const u8)? {
        return decodeFormComponent(_.raw_value, out);
    }
}

export struct FormIter {
    src: []const u8,
    pos: usize,

    pub static fn init(body: []const u8): FormIter {
        let start: usize = 0;
        if (body.len > 0 && body[0] == '?') start = 1;
        return FormIter { src: body, pos: start };
    }

    pub fn next(*_): FormPairRaw? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '&') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '&') {
                _.pos += 1;
            }
            const seg = _.src[start..end];
            if (seg.len == 0) continue;

            let eq_pos: usize? = null;
            let k: usize = 0;
            while (k < seg.len) {
                if (seg[k] == '=') {
                    eq_pos = k;
                    break;
                }
                k += 1;
            }
            if (ep := eq_pos) {
                return FormPairRaw {
                    raw_key: seg[0..ep],
                    raw_value: seg[ep + 1..],
                };
            }
            return FormPairRaw {
                raw_key: seg,
                raw_value: "",
            };
        }
        return null;
    }
}

/// Lookup the first value whose decoded key matches `target_key` and write its decoded value into `out`.
export fn getFormParam(body: []const u8, target_key: []const u8, out: []u8): ([]const u8)? {
    let key_buf: [256]u8 = undefined;
    let it = FormIter.init(body);
    while (pair := it.next()) {
        if (dk := pair.decodeKey(key_buf[0..])) {
            if (strEq(dk, target_key)) {
                return pair.decodeValue(out);
            }
        }
    }
    return null;
}

/// Append an encoded `key=value` entry to `out` at `pos` (inserting `&` if `pos > 0`).
export fn appendFormParam(out: []u8, pos: *usize, key: []const u8, val: []const u8): bool {
    if (pos.* > 0) {
        if (pos.* >= out.len) return false;
        out[pos.*] = '&';
        pos.* += 1;
    }
    const ek = encodeFormComponent(key, out[pos.*..]) ?? return false;
    pos.* += ek.len;
    if (pos.* >= out.len) return false;
    out[pos.*] = '=';
    pos.* += 1;
    const ev = encodeFormComponent(val, out[pos.*..]) ?? return false;
    pos.* += ev.len;
    return true;
}
