// Source: Absorbed from /data/dev/bun/src/runtime/webcore/CookieMap.cp | Synced: 2026-11-20T11:49:45Z
// lib/http/cookie.cp
// RFC 6265 HTTP Cookie Header Parser, CookieMap, CookieCodec & HttpCookieJar

const std = @import("std");

pub enum CookieSameSite: u8 {
    Lax = 0,
    Strict = 1,
    None = 2,
}

pub struct CookieEntry {
    pub name: []const u8 = "",
    pub value: []const u8 = "",
    pub path: []const u8 = "/",
    pub domain: []const u8 = "",
    pub max_age: i64 = -1,
    pub http_only: bool = false,
    pub secure: bool = false,
    pub same_site: CookieSameSite = CookieSameSite.Lax,
}

pub struct CookieMap {
    pub entries: [64]CookieEntry = undefined,
    pub count: usize = 0,

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

// ============================================================================
// Production RFC 6265 Cookie Codec & HttpCookieJar
// ============================================================================

pub enum SameSite: u8 {
    None = 0,
    Lax = 1,
    Strict = 2,
}

pub struct HttpCookie {
    pub name: []const u8 = "",
    pub value: []const u8 = "",
    pub domain: []const u8 = "",
    pub path: []const u8 = "/",
    pub expires_epoch_s: i64? = null,
    pub max_age: i64? = null,
    pub secure: bool = false,
    pub http_only: bool = false,
    pub same_site: SameSite = SameSite.Lax,
    pub host_only: bool = false,
}

pub struct CookiePair {
    pub name: []const u8 = "",
    pub value: []const u8 = "",
}

fn cookieIsValidName(name: []const u8): bool {
    if (name.len == 0) return false;
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        const c = name[i];
        if (c <= 0x20 || c >= 0x7F) return false;
        switch (c) {
            '(', ')', '<', '>', '@', ',', ';', ':', '\\', '"', '/', '[', ']', '?', '=', '{', '}' => return false,
            else => {},
        }
    }
    return true;
}

fn cookieIsValidValue(value: []const u8): bool {
    let i: usize = 0;
    while (i < value.len) : (i += 1) {
        const c = value[i];
        if (c == '\r' || c == '\n' || c == 0 || c == ';') return false;
    }
    return true;
}

priv struct CookieByteBuf {
    data: []u8 = [],
    len: usize = 0,

    fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.data.len > 0) {
            allocator.free(_.data);
        }
        _.data = [];
        _.len = 0;
    }

    fn appendBytes(*_, allocator: std.mem.Allocator, src: []const u8): ! {
        if (src.len == 0) return;
        const needed = _.len + src.len;
        if (needed > _.data.len) {
            let new_cap = if (_.data.len == 0) 256 else _.data.len * 2;
            while (new_cap < needed) {
                new_cap *= 2;
            }
            if (_.data.len == 0) {
                _.data = try allocator.alloc(u8, new_cap);
            } else {
                _.data = try allocator.realloc(_.data, new_cap);
            }
        }
        @memcpy(_.data[_.len .. _.len + src.len], src);
        _.len += src.len;
    }

    fn toOwnedSlice(*_, allocator: std.mem.Allocator): ([]const u8)! {
        if (_.len == 0) {
            if (_.data.len > 0) allocator.free(_.data);
            _.data = [];
            return "";
        }
        const res = try allocator.realloc(_.data, _.len);
        _.data = [];
        _.len = 0;
        return res;
    }
}

fn cookieParseMonth(m: []const u8): i64? {
    if (std.ascii.eqlIgnoreCase(m, "Jan")) return 1;
    if (std.ascii.eqlIgnoreCase(m, "Feb")) return 2;
    if (std.ascii.eqlIgnoreCase(m, "Mar")) return 3;
    if (std.ascii.eqlIgnoreCase(m, "Apr")) return 4;
    if (std.ascii.eqlIgnoreCase(m, "May")) return 5;
    if (std.ascii.eqlIgnoreCase(m, "Jun")) return 6;
    if (std.ascii.eqlIgnoreCase(m, "Jul")) return 7;
    if (std.ascii.eqlIgnoreCase(m, "Aug")) return 8;
    if (std.ascii.eqlIgnoreCase(m, "Sep")) return 9;
    if (std.ascii.eqlIgnoreCase(m, "Oct")) return 10;
    if (std.ascii.eqlIgnoreCase(m, "Nov")) return 11;
    if (std.ascii.eqlIgnoreCase(m, "Dec")) return 12;
    return null;
}

fn cookieDaysFromCivil(year_in: i64, month_in: i64, day_in: i64): i64 {
    const y = if (month_in <= 2) year_in - 1 else year_in;
    const era = @divFloor(y, 400);
    const yoe = y - era * 400;
    const m_adj = if (month_in > 2) month_in - 3 else month_in + 9;
    const doy = @divFloor(153 * m_adj + 2, 5) + day_in - 1;
    const doe = yoe * 365 + @divFloor(yoe, 4) - @divFloor(yoe, 100) + doy;
    return era * 146097 + doe - 719468;
}

fn cookieParseHttpDate(raw: []const u8): i64? {
    // Supports IMF-fixdate: "Wed, 21 Oct 2015 07:28:00 GMT" and RFC 850: "Wednesday, 21-Oct-15 07:28:00 GMT"
    const s = std.mem.trim(u8, raw, " \t");
    const comma = std.mem.indexOfScalar(u8, s, ',') ?? return null;
    const rest = std.mem.trim(u8, s[comma + 1 ..], " \t");
    let tok_iter = std.mem.tokenizeAny(u8, rest, " -:\t");
    const day_s = tok_iter.next() ?? return null;
    const mon_s = tok_iter.next() ?? return null;
    const year_s = tok_iter.next() ?? return null;
    const hour_s = tok_iter.next() ?? return null;
    const min_s = tok_iter.next() ?? return null;
    const sec_s = tok_iter.next() ?? return null;

    const day = std.fmt.parseInt(i64, day_s, 10) catch return null;
    const month = cookieParseMonth(mon_s) ?? return null;
    let year = std.fmt.parseInt(i64, year_s, 10) catch return null;
    if (year_s.len == 2) {
        if (year >= 70) {
            year += 1900;
        } else {
            year += 2000;
        }
    }
    const hour = std.fmt.parseInt(i64, hour_s, 10) catch return null;
    const minute = std.fmt.parseInt(i64, min_s, 10) catch return null;
    const second = std.fmt.parseInt(i64, sec_s, 10) catch return null;

    if (day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60) {
        return null;
    }
    const days = cookieDaysFromCivil(year, month, day);
    return days * 86400 + hour * 3600 + minute * 60 + second;
}

fn cookieFormatHttpDate(epoch_s: i64, buf: *[40]u8): []const u8 {
    const days = @divFloor(epoch_s, 86400);
    const rem = epoch_s - days * 86400;
    const hour = @divFloor(rem, 3600);
    const minute = @divFloor(rem - hour * 3600, 60);
    const second = rem - hour * 3600 - minute * 60;

    // Day of week: 1970-01-01 was Thursday (4)
    const dow_idx = @mod(days + 4, 7) as usize;
    const dow_names: [7][]const u8 = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
    const mon_names: [12][]const u8 = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];

    const z = days + 719468;
    const era = @divFloor(z, 146097);
    const doe = z - era * 146097;
    const yoe = @divFloor(doe - @divFloor(doe, 1460) + @divFloor(doe, 36524) - @divFloor(doe, 146096), 365);
    const y = yoe + era * 400;
    const doy = doe - (365 * yoe + @divFloor(yoe, 4) - @divFloor(yoe, 100));
    const mp = @divFloor(5 * doy + 2, 153);
    const d = doy - @divFloor(153 * mp + 2, 5) + 1;
    const m = if (mp < 10) mp + 3 else mp - 9;
    const final_y = if (m <= 2) y + 1 else y;

    return std.fmt.bufPrint(
        buf,
        "{s}, {d:0>2} {s} {d:0>4} {d:0>2}:{d:0>2}:{d:0>2} GMT",
        .{ dow_names[dow_idx], d as u64, mon_names[(m - 1) as usize], final_y as u64, hour as u64, minute as u64, second as u64 },
    ) catch "Thu, 01 Jan 1970 00:00:00 GMT";
}

pub struct CookieCodec {
    /// Parses a single `Set-Cookie` response header line per RFC 6265 §5.2.
    pub static fn parseSetCookie(header: []const u8): HttpCookie! {
        const trimmed = std.mem.trim(u8, header, " \t\r\n");
        if (trimmed.len == 0) return error.InvalidSetCookie;

        let iter = std.mem.splitScalar(u8, trimmed, ';');
        const nv_raw = iter.next() ?? return error.InvalidSetCookie;
        const eq = std.mem.indexOfScalar(u8, nv_raw, '=') ?? return error.InvalidSetCookie;

        const name = std.mem.trim(u8, nv_raw[0..eq], " \t");
        let value = std.mem.trim(u8, nv_raw[eq + 1 ..], " \t");
        if (!cookieIsValidName(name) || !cookieIsValidValue(value)) {
            return error.InvalidSetCookie;
        }
        if (value.len >= 2 && value[0] == '"' && value[value.len - 1] == '"') {
            value = value[1 .. value.len - 1];
        }

        let c = HttpCookie{
            name: name,
            value: value,
            domain: "",
            path: "/",
            expires_epoch_s: null,
            max_age: null,
            secure: false,
            http_only: false,
            same_site: SameSite.Lax,
            host_only: true,
        };

        while (attr_raw := iter.next()) {
            const attr = std.mem.trim(u8, attr_raw, " \t");
            if (attr.len == 0) continue;

            if (aeq := std.mem.indexOfScalar(u8, attr, '=')) {
                const k = std.mem.trim(u8, attr[0..aeq], " \t");
                const v = std.mem.trim(u8, attr[aeq + 1 ..], " \t");
                if (std.ascii.eqlIgnoreCase(k, "Domain")) {
                    const dom = std.mem.trimStart(u8, v, ".");
                    if (dom.len > 0 && !cookieIsValidValue(dom)) return error.InvalidSetCookie;
                    if (dom.len > 0) {
                        c.domain = dom;
                        c.host_only = false;
                    }
                } else if (std.ascii.eqlIgnoreCase(k, "Path")) {
                    if (v.len > 0 && v[0] == '/') {
                        c.path = v;
                    }
                } else if (std.ascii.eqlIgnoreCase(k, "Max-Age")) {
                    if (parsed := std.fmt.parseInt(i64, v, 10) catch null) {
                        c.max_age = parsed;
                    }
                } else if (std.ascii.eqlIgnoreCase(k, "Expires")) {
                    if (ep := cookieParseHttpDate(v)) {
                        c.expires_epoch_s = ep;
                    }
                } else if (std.ascii.eqlIgnoreCase(k, "SameSite")) {
                    if (std.ascii.eqlIgnoreCase(v, "None")) {
                        c.same_site = SameSite.None;
                    } else if (std.ascii.eqlIgnoreCase(v, "Strict")) {
                        c.same_site = SameSite.Strict;
                    } else if (std.ascii.eqlIgnoreCase(v, "Lax")) {
                        c.same_site = SameSite.Lax;
                    }
                }
            } else {
                if (std.ascii.eqlIgnoreCase(attr, "Secure")) {
                    c.secure = true;
                } else if (std.ascii.eqlIgnoreCase(attr, "HttpOnly")) {
                    c.http_only = true;
                }
            }
        }
        return c;
    }

    /// Formats an `HttpCookie` into a `Set-Cookie` header value using `alloc`.
    pub static fn formatSetCookie(alloc: std.mem.Allocator, cookie: HttpCookie): ([]const u8)! {
        if (!cookieIsValidName(cookie.name) || !cookieIsValidValue(cookie.value)) {
            return error.InvalidCookie;
        }
        if (cookieIsValidValue(cookie.domain) == false || cookieIsValidValue(cookie.path) == false) {
            return error.InvalidCookie;
        }

        let out = CookieByteBuf{};
        errdefer out.deinit(alloc);

        try out.appendBytes(alloc, cookie.name);
        try out.appendBytes(alloc, "=");
        try out.appendBytes(alloc, cookie.value);

        if (cookie.domain.len > 0) {
            try out.appendBytes(alloc, "; Domain=");
            try out.appendBytes(alloc, cookie.domain);
        }
        if (cookie.path.len > 0) {
            try out.appendBytes(alloc, "; Path=");
            try out.appendBytes(alloc, cookie.path);
        }
        if (exp := cookie.expires_epoch_s) {
            let dbuf: [40]u8 = undefined;
            const dstr = cookieFormatHttpDate(exp, &dbuf);
            try out.appendBytes(alloc, "; Expires=");
            try out.appendBytes(alloc, dstr);
        }
        if (ma := cookie.max_age) {
            let nbuf: [32]u8 = undefined;
            const nstr = std.fmt.bufPrint(&nbuf, "{d}", .{ma}) catch return error.FormatFailed;
            try out.appendBytes(alloc, "; Max-Age=");
            try out.appendBytes(alloc, nstr);
        }
        if (cookie.secure) {
            try out.appendBytes(alloc, "; Secure");
        }
        if (cookie.http_only) {
            try out.appendBytes(alloc, "; HttpOnly");
        }
        switch (cookie.same_site) {
            .None => try out.appendBytes(alloc, "; SameSite=None"),
            .Strict => try out.appendBytes(alloc, "; SameSite=Strict"),
            .Lax => try out.appendBytes(alloc, "; SameSite=Lax"),
        }

        return try out.toOwnedSlice(alloc);
    }

    /// Parses an HTTP `Cookie:` request header (`k1=v1; k2=v2`) into an owned slice of `CookiePair`.
    pub static fn parseCookieHeader(alloc: std.mem.Allocator, header: []const u8): ([]CookiePair)! {
        let pairs: []CookiePair = [];
        let len: usize = 0;
        let cap: usize = 0;
        errdefer if (cap > 0) alloc.free(pairs);

        let iter = std.mem.splitScalar(u8, header, ';');
        while (raw_pair := iter.next()) {
            const pair = std.mem.trim(u8, raw_pair, " \t\r\n");
            if (pair.len == 0) continue;
            if (eq := std.mem.indexOfScalar(u8, pair, '=')) {
                const k = std.mem.trim(u8, pair[0..eq], " \t");
                let v = std.mem.trim(u8, pair[eq + 1 ..], " \t");
                if (v.len >= 2 && v[0] == '"' && v[v.len - 1] == '"') {
                    v = v[1 .. v.len - 1];
                }
                if (k.len == 0) continue;
                if (len >= cap) {
                    const new_cap: usize = if (cap == 0) 8 else cap * 2;
                    if (cap == 0) {
                        pairs = try alloc.alloc(CookiePair, new_cap);
                    } else {
                        pairs = try alloc.realloc(pairs, new_cap);
                    }
                    cap = new_cap;
                }
                pairs[len] = CookiePair{ name: k, value: v };
                len += 1;
            }
        }
        if (len == 0) {
            if (cap > 0) alloc.free(pairs);
            return [];
        }
        if (len < cap) {
            return try alloc.realloc(pairs, len);
        }
        return pairs[0..len];
    }
}

priv struct StoredJarCookie {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "/",
    effective_expires_s: i64? = null,
    secure: bool = false,
    http_only: bool = false,
    same_site: SameSite = SameSite.Lax,
    host_only: bool = false,
    creation_seq: u64 = 0,
}

fn cookieStripHostPort(host: []const u8): []const u8 {
    if (host.len > 0 && host[0] == '[') {
        if (rb := std.mem.indexOfScalar(u8, host, ']')) {
            return host[1..rb];
        }
    }
    if (colon := std.mem.indexOfScalar(u8, host, ':')) {
        return host[0..colon];
    }
    return host;
}

fn cookieIsIpLiteral(host: []const u8): bool {
    if (std.mem.indexOfScalar(u8, host, ':') != null) return true;
    let i: usize = 0;
    let all_ipv4_chars = host.len > 0;
    while (i < host.len) : (i += 1) {
        const c = host[i];
        if (!((c >= '0' && c <= '9') || c == '.')) {
            all_ipv4_chars = false;
            break;
        }
    }
    return all_ipv4_chars;
}

pub struct HttpCookieJar {
    pub allocator: std.mem.Allocator = undefined,
    priv items: []StoredJarCookie = [],
    priv len: usize = 0,
    priv cap: usize = 0,
    priv next_seq: u64 = 1,

    pub static fn init(allocator: std.mem.Allocator): HttpCookieJar {
        return HttpCookieJar{
            allocator: allocator,
            items: [],
            len: 0,
            cap: 0,
            next_seq: 1,
        };
    }

    fn freeStored(*_, item: StoredJarCookie) {
        if (item.name.len > 0) _.allocator.free(item.name);
        if (item.value.len > 0) _.allocator.free(item.value);
        if (item.domain.len > 0) _.allocator.free(item.domain);
        if (item.path.len > 0) _.allocator.free(item.path);
    }

    pub fn deinit(*_) {
        _.clear();
        if (_.cap > 0) {
            _.allocator.free(_.items);
        }
        _.items = [];
        _.cap = 0;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            _.freeStored(_.items[i]);
        }
        _.len = 0;
    }

    pub fn count(): usize {
        return _.len;
    }

    /// Basic public suffix boundary check to prevent cross-TLD supercookies (e.g. `com`, `co.uk`, `com.cn`, `github.io`).
    pub static fn isPublicSuffix(raw_domain: []const u8): bool {
        const d = std.mem.trim(u8, raw_domain, ".");
        if (d.len == 0) return true;
        if (cookieIsIpLiteral(d)) return false;
        // Single-label domains (e.g. "com", "org", "net", "io", "dev", "localhost") cannot be shared suffix domains
        if (std.mem.indexOfScalar(u8, d, '.') == null) {
            return true;
        }
        const multi_suffixes: [24][]const u8 = [
            "co.uk", "org.uk", "ac.uk", "gov.uk", "me.uk",
            "com.cn", "net.cn", "org.cn", "gov.cn", "edu.cn",
            "com.au", "net.au", "org.au",
            "co.jp", "ne.jp", "or.jp",
            "com.br", "co.kr", "com.tw", "com.hk",
            "github.io", "herokuapp.com", "pages.dev", "workers.dev",
        ];
        let i: usize = 0;
        while (i < multi_suffixes.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(d, multi_suffixes[i])) return true;
        }
        return false;
    }

    /// RFC 6265 §5.1.3 domain-match.
    pub static fn domainMatches(raw_host: []const u8, cookie_domain: []const u8, host_only: bool): bool {
        const host = std.mem.trimEnd(u8, cookieStripHostPort(raw_host), ".");
        const dom = std.mem.trim(u8, cookie_domain, ".");
        if (host.len == 0 || dom.len == 0) return false;
        if (std.ascii.eqlIgnoreCase(host, dom)) return true;
        if (host_only) return false;
        if (cookieIsIpLiteral(host)) return false;
        if (host.len <= dom.len) return false;
        const prefix_len = host.len - dom.len;
        if (host[prefix_len - 1] != '.') return false;
        return std.ascii.eqlIgnoreCase(host[prefix_len..], dom);
    }

    /// RFC 6265 §5.1.4 path-match.
    pub static fn pathMatches(raw_req_path: []const u8, cookie_path: []const u8): bool {
        let req_path = raw_req_path;
        if (q := std.mem.indexOfScalar(u8, req_path, '?')) {
            req_path = req_path[0..q];
        }
        if (req_path.len == 0) req_path = "/";
        const cpath = if (cookie_path.len == 0) "/" else cookie_path;

        if (std.mem.eql(u8, req_path, cpath)) return true;
        if (std.mem.startsWith(u8, req_path, cpath)) {
            if (cpath[cpath.len - 1] == '/') return true;
            if (req_path[cpath.len] == '/') return true;
        }
        return false;
    }

    fn deleteExact(*_, name: []const u8, domain: []const u8, path: []const u8) {
        let i: usize = 0;
        while (i < _.len) {
            const it = _.items[i];
            if (std.mem.eql(u8, it.name, name) && std.ascii.eqlIgnoreCase(it.domain, domain) && std.mem.eql(u8, it.path, path)) {
                _.freeStored(it);
                let j = i;
                while (j + 1 < _.len) : (j += 1) {
                    _.items[j] = _.items[j + 1];
                }
                _.len -= 1;
            } else {
                i += 1;
            }
        }
    }

    /// Stores, overwrites, or deletes (`Max-Age <= 0` or expired) an `HttpCookie`.
    /// Enforces public-suffix rejection for non-host-only domain cookies.
    pub fn set(*_, cookie: HttpCookie, now_epoch_s: i64 = 0): ! {
        if (!cookieIsValidName(cookie.name) || !cookieIsValidValue(cookie.value)) {
            return error.InvalidCookie;
        }
        const norm_dom = std.mem.trim(u8, cookie.domain, ".");
        if (norm_dom.len == 0) return error.InvalidCookieDomain;

        if (!cookie.host_only && HttpCookieJar.isPublicSuffix(norm_dom)) {
            return error.PublicSuffixRejected;
        }

        const norm_path = if (cookie.path.len == 0) "/" else cookie.path;

        // Compute effective expiry; Max-Age takes precedence over Expires per RFC 6265 §5.3
        let eff_exp: i64? = null;
        if (ma := cookie.max_age) {
            if (ma <= 0) {
                _.deleteExact(cookie.name, norm_dom, norm_path);
                return;
            }
            eff_exp = now_epoch_s +| ma;
        } else if (exp := cookie.expires_epoch_s) {
            if (exp <= now_epoch_s) {
                _.deleteExact(cookie.name, norm_dom, norm_path);
                return;
            }
            eff_exp = exp;
        }

        const n_dup = try _.allocator.dupe(u8, cookie.name);
        errdefer _.allocator.free(n_dup);
        const v_dup = if (cookie.value.len > 0) try _.allocator.dupe(u8, cookie.value) else "";
        errdefer if (v_dup.len > 0) _.allocator.free(v_dup);
        let d_dup = try _.allocator.dupe(u8, norm_dom);
        errdefer _.allocator.free(d_dup);
        // Normalize stored domain to lowercase ASCII
        let k: usize = 0;
        while (k < d_dup.len) : (k += 1) {
            if (d_dup[k] >= 'A' && d_dup[k] <= 'Z') {
                d_dup[k] = d_dup[k] + 32;
            }
        }
        const p_dup = try _.allocator.dupe(u8, norm_path);
        errdefer _.allocator.free(p_dup);

        // Overwrite existing cookie with same (name, domain, path)
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const it = _.items[i];
            if (std.mem.eql(u8, it.name, n_dup) && std.ascii.eqlIgnoreCase(it.domain, d_dup) && std.mem.eql(u8, it.path, p_dup)) {
                const seq = it.creation_seq;
                _.freeStored(it);
                _.items[i] = StoredJarCookie{
                    name: n_dup,
                    value: v_dup,
                    domain: d_dup,
                    path: p_dup,
                    effective_expires_s: eff_exp,
                    secure: cookie.secure,
                    http_only: cookie.http_only,
                    same_site: cookie.same_site,
                    host_only: cookie.host_only,
                    creation_seq: seq,
                };
                return;
            }
        }

        if (_.len >= _.cap) {
            const new_cap: usize = if (_.cap == 0) 16 else _.cap * 2;
            if (_.cap == 0) {
                _.items = try _.allocator.alloc(StoredJarCookie, new_cap);
            } else {
                _.items = try _.allocator.realloc(_.items, new_cap);
            }
            _.cap = new_cap;
        }

        _.items[_.len] = StoredJarCookie{
            name: n_dup,
            value: v_dup,
            domain: d_dup,
            path: p_dup,
            effective_expires_s: eff_exp,
            secure: cookie.secure,
            http_only: cookie.http_only,
            same_site: cookie.same_site,
            host_only: cookie.host_only,
            creation_seq: _.next_seq,
        };
        _.len += 1;
        _.next_seq += 1;
    }

    /// Parses a `Set-Cookie` header for a given request `host` and `now_epoch_s` and stores it in the jar.
    pub fn setFromHeader(*_, request_host: []const u8, set_cookie_header: []const u8, now_epoch_s: i64 = 0): ! {
        let c = try CookieCodec.parseSetCookie(set_cookie_header);
        const clean_host = cookieStripHostPort(request_host);
        if (c.domain.len == 0) {
            c.domain = clean_host;
            c.host_only = true;
        } else {
            if (!HttpCookieJar.domainMatches(clean_host, c.domain, false)) {
                return error.DomainMismatch;
            }
            c.host_only = false;
        }
        try _.set(c, now_epoch_s);
    }

    /// Builds the `Cookie:` request header value (`k1=v1; k2=v2`) matching `host`, `path`, `is_https`, and `now_epoch_s`.
    /// Caller owns the returned slice (may be empty `""` if none match) and must free non-empty slices via `alloc.free`.
    pub fn cookieHeaderFor(*_, alloc: std.mem.Allocator, host: []const u8, path: []const u8, is_https: bool, now_epoch_s: i64): ([]const u8)! {
        if (_.len == 0) return "";

        let matched_idx = try alloc.alloc(usize, _.len);
        defer alloc.free(matched_idx);
        let mcount: usize = 0;

        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const it = _.items[i];
            if (exp := it.effective_expires_s) {
                if (exp <= now_epoch_s) continue;
            }
            if (it.secure && !is_https) continue;
            if (!HttpCookieJar.domainMatches(host, it.domain, it.host_only)) continue;
            if (!HttpCookieJar.pathMatches(path, it.path)) continue;
            matched_idx[mcount] = i;
            mcount += 1;
        }

        if (mcount == 0) return "";

        // RFC 6265 §5.4 step 2: longer path first, then earlier creation_seq
        let a: usize = 0;
        while (a < mcount) : (a += 1) {
            let b: usize = a + 1;
            while (b < mcount) : (b += 1) {
                const ia = _.items[matched_idx[a]];
                const ib = _.items[matched_idx[b]];
                const swap = (ib.path.len > ia.path.len) ||
                    (ib.path.len == ia.path.len && ib.creation_seq < ia.creation_seq);
                if (swap) {
                    const tmp = matched_idx[a];
                    matched_idx[a] = matched_idx[b];
                    matched_idx[b] = tmp;
                }
            }
        }

        let out = CookieByteBuf{};
        errdefer out.deinit(alloc);

        let k: usize = 0;
        while (k < mcount) : (k += 1) {
            if (k > 0) {
                try out.appendBytes(alloc, "; ");
            }
            const c = _.items[matched_idx[k]];
            try out.appendBytes(alloc, c.name);
            try out.appendBytes(alloc, "=");
            try out.appendBytes(alloc, c.value);
        }

        return try out.toOwnedSlice(alloc);
    }
}
