// Source: Custom implementation by Antigravity AI
// lib/http/cookie.cp
// RFC 6265 HTTP Cookie Header Parser, CookieMap & Set-Cookie Serializer
// Absorbed from Bun src/http/CookieMap.cp

const std = @import("std");

pub enum CookieSameSite: u8 {
    Lax = 0,
    Strict = 1,
    None = 2,
}

pub struct CookieEntry {
    name: []const u8 = "";
    value: []const u8 = "";
    path: []const u8 = "/";
    domain: []const u8 = "";
    max_age: i64 = -1;
    http_only: bool = false;
    secure: bool = false;
    same_site: CookieSameSite = CookieSameSite.Lax;
}

pub struct CookieMap {
    entries: [64]CookieEntry = undefined;
    count: usize = 0;

    /// Parses an HTTP `Cookie:` request header (`k1=v1; k2=v2`) into a `CookieMap`.
    pub static fn parse(cookie_header: []const u8): CookieMap {
        let map = CookieMap {
            entries: undefined,
            count: 0,
        };
        let pair_iter = std.mem.splitScalar(u8, cookie_header, ';');
        while (raw_pair := pair_iter.next()) {
            const pair = std.mem.trim(u8, raw_pair, " \t\r\n");
            if (pair.len == 0) {
                continue;
            }
            if (eq_idx := std.mem.indexOfScalar(u8, pair, '=')) {
                const k = std.mem.trim(u8, pair[0..eq_idx], " \t");
                let v = std.mem.trim(u8, pair[eq_idx + 1 ..], " \t");
                if (v.len >= 2 && v[0] == '"' && v[v.len - 1] == '"') {
                    v = v[1 .. v.len - 1];
                }
                if (k.len > 0) {
                    map.set(k, v);
                }
            }
        }
        return map;
    }

    pub fn set(*_, name: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].name, name)) {
                _.entries[i].value = value;
                return;
            }
            i += 1;
        }
        if (_.count < 64) {
            _.entries[_.count] = CookieEntry {
                name: name,
                value: value,
            };
            _.count += 1;
        }
    }

    pub fn get(name: []const u8): ([]const u8)? {
        let i: usize = _.count;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.entries[i].name, name)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    pub fn has(name: []const u8): bool {
        return _.get(name) != null;
    }

    /// Formats a `Set-Cookie` response header line into `out_buf`.
    pub static fn formatSetCookie(entry: CookieEntry, out_buf: []u8): []const u8 {
        let pos: usize = 0;
        pos = CookieMap.appendBytes(out_buf, pos, entry.name);
        pos = CookieMap.appendBytes(out_buf, pos, "=");
        pos = CookieMap.appendBytes(out_buf, pos, entry.value);
        if (entry.path.len > 0) {
            pos = CookieMap.appendBytes(out_buf, pos, "; Path=");
            pos = CookieMap.appendBytes(out_buf, pos, entry.path);
        }
        if (entry.domain.len > 0) {
            pos = CookieMap.appendBytes(out_buf, pos, "; Domain=");
            pos = CookieMap.appendBytes(out_buf, pos, entry.domain);
        }
        if (entry.http_only) {
            pos = CookieMap.appendBytes(out_buf, pos, "; HttpOnly");
        }
        if (entry.secure) {
            pos = CookieMap.appendBytes(out_buf, pos, "; Secure");
        }
        if (entry.same_site == CookieSameSite.Strict) {
            pos = CookieMap.appendBytes(out_buf, pos, "; SameSite=Strict");
        } else if (entry.same_site == CookieSameSite.None) {
            pos = CookieMap.appendBytes(out_buf, pos, "; SameSite=None");
        } else {
            pos = CookieMap.appendBytes(out_buf, pos, "; SameSite=Lax");
        }
        return out_buf[0..pos];
    }

    pub static fn appendBytes(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }
}
