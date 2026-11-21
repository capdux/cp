// Source: Absorbed from /data/dev/bun/src/jsc/bindings/Cookie.cpp | Synced: 2026-11-21T16:45:50Z
// Pure-CP zero-allocation RFC 6265 / 6265bis HTTP Cookie & Set-Cookie parser,
// domain/path matcher, and Set-Cookie header serializer.

pub enum SameSite: u8 {
    unset = 0,
    strict = 1,
    lax = 2,
    none = 3,

    pub static fn parse(s: []const u8): SameSite? {
        const t = trimOws(s);
        if (eqIgnoreCase(t, "strict")) return .strict;
        if (eqIgnoreCase(t, "lax")) return .lax;
        if (eqIgnoreCase(t, "none")) return .none;
        return null;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .unset => "",
            .strict => "Strict",
            .lax => "Lax",
            .none => "None",
        };
    }
}

pub struct Cookie {
    name: []const u8,
    value: []const u8,
    domain: []const u8,
    path: []const u8,
    expires: []const u8,
    max_age: i64?,
    secure: bool,
    http_only: bool,
    partitioned: bool,
    same_site: SameSite,

    pub static fn init(name: []const u8, value: []const u8): Cookie {
        return Cookie{
            name: name,
            value: value,
            domain: "",
            path: "",
            expires: "",
            max_age: null,
            secure: false,
            http_only: false,
            partitioned: false,
            same_site: .unset,
        };
    }

    /// Parse an HTTP `Set-Cookie` header value per RFC 6265 §5.2.
    pub static fn parseSetCookie(header: []const u8): Cookie? {
        const s = trimOws(header);
        if (s.len == 0) return null;

        let pos: usize = 0;
        const first_seg = nextSemiSegment(s, &pos) ?? return null;
        const eq = findChar(first_seg, '=') ?? return null;
        const name = trimOws(first_seg[0..eq]);
        if (name.len == 0 || !isValidCookieName(name)) return null;

        let val = trimOws(first_seg[eq + 1 ..]);
        if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
            val = val[1 .. val.len - 1];
        }

        let cookie = Cookie.init(name, val);
        while (attr := nextSemiSegment(s, &pos)) {
            const a = trimOws(attr);
            if (a.len == 0) continue;
            if (aeq := findChar(a, '=')) {
                const ak = trimOws(a[0..aeq]);
                const av = trimOws(a[aeq + 1 ..]);
                if (eqIgnoreCase(ak, "domain")) {
                    // RFC 6265 §5.2.3: strip leading '.'
                    cookie.domain = if (av.len > 0 && av[0] == '.') av[1..] else av;
                } else if (eqIgnoreCase(ak, "path")) {
                    if (av.len > 0 && av[0] == '/') cookie.path = av;
                } else if (eqIgnoreCase(ak, "expires")) {
                    cookie.expires = av;
                } else if (eqIgnoreCase(ak, "max-age")) {
                    if (ma := parseI64(av)) cookie.max_age = ma;
                } else if (eqIgnoreCase(ak, "samesite")) {
                    if (ss := SameSite.parse(av)) cookie.same_site = ss;
                }
            } else {
                if (eqIgnoreCase(a, "secure")) {
                    cookie.secure = true;
                } else if (eqIgnoreCase(a, "httponly")) {
                    cookie.http_only = true;
                } else if (eqIgnoreCase(a, "partitioned")) {
                    cookie.partitioned = true;
                }
            }
        }
        return cookie;
    }

    /// Serialize this cookie to an HTTP `Set-Cookie` header value into `out`.
    pub fn toSetCookieHeader(out: []u8): ([]const u8)? {
        if (_.name.len == 0 || !isValidCookieName(_.name)) return null;
        let p: usize = 0;
        if (!appendTo(out, &p, _.name)) return null;
        if (!appendTo(out, &p, "=")) return null;
        if (!appendTo(out, &p, _.value)) return null;

        if (_.domain.len > 0) {
            if (!appendTo(out, &p, "; Domain=")) return null;
            if (!appendTo(out, &p, _.domain)) return null;
        }
        if (_.path.len > 0) {
            if (!appendTo(out, &p, "; Path=")) return null;
            if (!appendTo(out, &p, _.path)) return null;
        }
        if (_.expires.len > 0) {
            if (!appendTo(out, &p, "; Expires=")) return null;
            if (!appendTo(out, &p, _.expires)) return null;
        }
        if (ma := _.max_age) {
            if (!appendTo(out, &p, "; Max-Age=")) return null;
            if (!appendI64To(out, &p, ma)) return null;
        }
        if (_.secure) {
            if (!appendTo(out, &p, "; Secure")) return null;
        }
        if (_.http_only) {
            if (!appendTo(out, &p, "; HttpOnly")) return null;
        }
        if (_.partitioned) {
            if (!appendTo(out, &p, "; Partitioned")) return null;
        }
        if (_.same_site != .unset) {
            if (!appendTo(out, &p, "; SameSite=")) return null;
            if (!appendTo(out, &p, _.same_site.asStr())) return null;
        }
        return out[0..p];
    }

    /// RFC 6265 §5.1.3 domain-match: host equals domain or ends with `.domain` (case-insensitive).
    pub fn matchesDomain(host: []const u8): bool {
        if (_.domain.len == 0) return true;
        if (eqIgnoreCase(host, _.domain)) return true;
        if (host.len > _.domain.len + 1) {
            const offset = host.len - _.domain.len;
            if (host[offset - 1] == '.' && eqIgnoreCase(host[offset..], _.domain)) {
                return true;
            }
        }
        return false;
    }

    /// RFC 6265 §5.1.4 path-match.
    pub fn matchesPath(req_path: []const u8): bool {
        if (_.path.len == 0 || strEq(_.path, "/")) return true;
        if (strEq(req_path, _.path)) return true;
        if (req_path.len > _.path.len && strEq(req_path[0.._.path.len], _.path)) {
            if (_.path[_.path.len - 1] == '/' || req_path[_.path.len] == '/') return true;
        }
        return false;
    }
}

pub struct CookiePair {
    name: []const u8,
    value: []const u8,
}

/// Zero-allocation iterator over an HTTP `Cookie:` request header (`a=1; b="2"`).
pub struct CookieRequestIterator {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): CookieRequestIterator {
        return CookieRequestIterator{ src: header, pos: 0 };
    }

    pub fn next(*_): CookiePair? {
        while (seg := nextSemiSegment(_.src, &_.pos)) {
            const s = trimOws(seg);
            if (s.len == 0) continue;
            if (eq := findChar(s, '=')) {
                const name = trimOws(s[0..eq]);
                if (name.len == 0) continue;
                let val = trimOws(s[eq + 1 ..]);
                if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                    val = val[1 .. val.len - 1];
                }
                return CookiePair{ name: name, value: val };
            }
        }
        return null;
    }
}

/// Zero-allocation lookup of a named cookie from an HTTP `Cookie:` request header.
pub fn getCookie(cookie_header: []const u8, name: []const u8): ([]const u8)? {
    let it = CookieRequestIterator.init(cookie_header);
    while (pair := it.next()) {
        if (strEq(pair.name, name)) return pair.value;
    }
    return null;
}

// ---------- Helpers ----------

fn isValidCookieName(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c <= 0x20 || c >= 0x7F) return false;
        if (c == '(' || c == ')' || c == '<' || c == '>' || c == '@' || c == ',' ||
            c == ';' || c == ':' || c == '\\' || c == '"' || c == '/' || c == '[' ||
            c == ']' || c == '?' || c == '=' || c == '{' || c == '}')
        {
            return false;
        }
    }
    return true;
}

fn nextSemiSegment(s: []const u8, pos: *usize): ([]const u8)? {
    if (pos.* >= s.len) return null;
    const start = pos.*;
    while (pos.* < s.len && s[pos.*] != ';') : (pos.* += 1) {}
    const seg = s[start..pos.*];
    if (pos.* < s.len && s[pos.*] == ';') pos.* += 1;
    return seg;
}

fn findChar(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return null;
}

fn parseI64(s: []const u8): i64? {
    if (s.len == 0) return null;
    let idx: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        idx = 1;
    }
    if (idx >= s.len) return null;
    let acc: i64 = 0;
    while (idx < s.len) : (idx += 1) {
        if (s[idx] < '0' || s[idx] > '9') return null;
        acc = acc * 10 + @as(i64, s[idx] - '0');
    }
    return if (neg) 0 - acc else acc;
}

fn appendTo(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.*] = s[i];
        pos.* += 1;
    }
    return true;
}

fn appendI64To(out: []u8, pos: *usize, v_in: i64): bool {
    let v = v_in;
    if (v < 0) {
        if (pos.* >= out.len) return false;
        out[pos.*] = '-';
        pos.* += 1;
        v = 0 - v;
    }
    if (v == 0) {
        if (pos.* >= out.len) return false;
        out[pos.*] = '0';
        pos.* += 1;
        return true;
    }
    let buf: [24]u8 = [0; 24];
    let n: usize = 0;
    while (v > 0) : (v /= 10) {
        buf[n] = @intCast('0' + @as(u8, @intCast(v % 10)));
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    while (n > 0) {
        n -= 1;
        out[pos.*] = buf[n];
        pos.* += 1;
    }
    return true;
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}
