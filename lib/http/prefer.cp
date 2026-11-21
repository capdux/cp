// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-21T16:49:05Z (RFC 7240 Prefer & Preference-Applied HTTP Header)
// Pure-CP zero-allocation RFC 7240 Prefer header parser, query helpers, and Preference-Applied formatter.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn unquoteSimple(s: []const u8): []const u8 {
    if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
        return s[1..s.len - 1];
    }
    return s;
}

fn parseU64Strict(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = @as(u64, c - '0');
        if (val > 1844674407370955161) return null;
        if (val == 1844674407370955161 && d > 5) return null;
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

export struct Preference {
    name: []const u8,
    value: ([]const u8)?,
    params: []const u8,
}

export struct PreferIter {
    src: []const u8,
    pos: usize,

    pub fn init(header: []const u8): PreferIter {
        return PreferIter { src: header, pos: 0 };
    }

    pub fn next(*_): Preference? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            let in_quote = false;
            let escaped = false;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        in_quote = false;
                    }
                } else {
                    if (c == '"') {
                        in_quote = true;
                    } else if (c == ',') {
                        break;
                    }
                }
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            const item = trimWs(_.src[start..end]);
            if (item.len == 0) continue;

            // Split first ';' (outside quotes) into main token[=value] and params
            let semi_pos = item.len;
            let iq = false;
            let esc = false;
            let k: usize = 0;
            while (k < item.len) {
                const ch = item[k];
                if (iq) {
                    if (esc) {
                        esc = false;
                    } else if (ch == '\\') {
                        esc = true;
                    } else if (ch == '"') {
                        iq = false;
                    }
                } else {
                    if (ch == '"') {
                        iq = true;
                    } else if (ch == ';') {
                        semi_pos = k;
                        break;
                    }
                }
                k += 1;
            }

            const main = trimWs(item[0..semi_pos]);
            let params: []const u8 = "";
            if (semi_pos < item.len) {
                params = trimWs(item[semi_pos + 1..]);
            }
            if (main.len == 0) continue;

            let eq_pos: usize? = null;
            let j: usize = 0;
            while (j < main.len) {
                if (main[j] == '=') {
                    eq_pos = j;
                    break;
                }
                j += 1;
            }

            if (ep := eq_pos) {
                const name = trimWs(main[0..ep]);
                if (name.len == 0) continue;
                const val = unquoteSimple(trimWs(main[ep + 1..]));
                return Preference { name: name, value: val, params: params };
            }
            return Preference { name: main, value: null, params: params };
        }
        return null;
    }
}

export enum ReturnPref {
    none,
    representation,
    minimal,
}

export enum HandlingPref {
    none,
    strict,
    lenient,
}

export struct PreferHeader {
    respond_async: bool,
    return_pref: ReturnPref,
    handling: HandlingPref,
    wait_sec: u64?,

    /// Parse standard RFC 7240 preferences from a `Prefer` header value.
    /// First occurrence wins per RFC 7240 §2.
    pub fn parse(header: []const u8): PreferHeader {
        let res = PreferHeader {
            respond_async: false,
            return_pref: ReturnPref.none,
            handling: HandlingPref.none,
            wait_sec: null,
        };
        let it = PreferIter.init(header);
        while (pref := it.next()) {
            if (eqIgnoreCase(pref.name, "respond-async")) {
                res.respond_async = true;
            } else if (eqIgnoreCase(pref.name, "return") && res.return_pref == ReturnPref.none) {
                if (v := pref.value) {
                    if (eqIgnoreCase(v, "representation")) {
                        res.return_pref = ReturnPref.representation;
                    } else if (eqIgnoreCase(v, "minimal")) {
                        res.return_pref = ReturnPref.minimal;
                    }
                }
            } else if (eqIgnoreCase(pref.name, "handling") && res.handling == HandlingPref.none) {
                if (v := pref.value) {
                    if (eqIgnoreCase(v, "strict")) {
                        res.handling = HandlingPref.strict;
                    } else if (eqIgnoreCase(v, "lenient")) {
                        res.handling = HandlingPref.lenient;
                    }
                }
            } else if (eqIgnoreCase(pref.name, "wait") && res.wait_sec == null) {
                if (v := pref.value) {
                    res.wait_sec = parseU64Strict(v);
                }
            }
        }
        return res;
    }

    /// Format as a `Preference-Applied` response header value into `out`.
    pub fn formatApplied(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        let first = true;

        if (_.respond_async) {
            if (!appendPart(out, &pos, &first, "respond-async")) return null;
        }
        if (_.return_pref == ReturnPref.representation) {
            if (!appendPart(out, &pos, &first, "return=representation")) return null;
        } else if (_.return_pref == ReturnPref.minimal) {
            if (!appendPart(out, &pos, &first, "return=minimal")) return null;
        }
        if (_.handling == HandlingPref.strict) {
            if (!appendPart(out, &pos, &first, "handling=strict")) return null;
        } else if (_.handling == HandlingPref.lenient) {
            if (!appendPart(out, &pos, &first, "handling=lenient")) return null;
        }
        if (w := _.wait_sec) {
            if (!appendPart(out, &pos, &first, "wait=")) return null;
            if (!appendU64(out, &pos, w)) return null;
        }
        return out[0..pos];
    }
}

fn appendBytes(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn appendPart(out: []u8, pos: *usize, first: *bool, s: []const u8): bool {
    if (!first.*) {
        if (!appendBytes(out, pos, ", ")) return false;
    }
    first.* = false;
    return appendBytes(out, pos, s);
}

fn appendU64(out: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        return appendBytes(out, pos, "0");
    }
    while (v > 0) {
        tmp[n] = @as(u8, '0') + @as(u8, v % 10);
        v /= 10;
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}
