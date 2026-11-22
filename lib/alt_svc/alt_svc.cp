// Source: Absorbed from /data/dev/bun/src/http/h3_client/AltSvc.cp | Synced: 2026-11-22T11:12:13Z
// Pure-CP zero-allocation RFC 7838 HTTP `Alt-Svc` header parser and TTL cache
// for HTTP/3 / alternative service discovery.

pub const DEFAULT_MAX_AGE: u32 = 86400;

pub struct AltSvcEntry {
    /// Protocol ID (e.g. `"h3"`, `"h3-29"`, `"h2"`).
    protocol: []const u8,
    /// Optional alternative host (empty means same origin host).
    host: []const u8,
    /// Target port (1..65535).
    port: u16,
    /// Freshness lifetime in seconds (`ma=`, defaults to 86400 per RFC 7838 §3.1).
    max_age: u32,
    /// Whether `persist=1` was specified.
    persist: bool,

    pub fn isH3(): bool {
        return eqIgnoreCase(_.protocol, "h3");
    }

    pub fn isSameHost(): bool {
        return _.host.len == 0;
    }
}

pub struct AltSvcHeader {
    /// True if header value was `clear`.
    is_clear: bool,
    entries: [8]AltSvcEntry,
    len: usize,

    /// Parse an RFC 7838 `Alt-Svc` header field value.
    pub static fn parse(field_value: []const u8): AltSvcHeader {
        let res = AltSvcHeader{
            is_clear: false,
            entries: [AltSvcEntry{ protocol: "", host: "", port: 0, max_age: DEFAULT_MAX_AGE, persist: false }; 8],
            len: 0,
        };
        const s = trimOws(field_value);
        if (s.len == 0) return res;
        if (eqIgnoreCase(s, "clear")) {
            res.is_clear = true;
            return res;
        }

        let pos: usize = 0;
        while (pos < s.len) {
            const start = pos;
            let in_quote = false;
            while (pos < s.len) : (pos += 1) {
                if (s[pos] == '"') {
                    in_quote = !in_quote;
                } else if (s[pos] == ',' && !in_quote) {
                    break;
                }
            }
            const item = trimOws(s[start..pos]);
            if (pos < s.len && s[pos] == ',') pos += 1;
            if (item.len == 0) continue;

            if (entry := parseSingleEntry(item)) {
                if (res.len < 8) {
                    res.entries[res.len] = entry;
                    res.len += 1;
                }
            }
        }
        return res;
    }

    /// Find the first same-host HTTP/3 (`h3`) entry (matching Bun's AltSvc.parse behavior).
    pub fn firstSameHostH3(): AltSvcEntry? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const e = _.entries[i];
            if (e.isH3() && e.isSameHost()) return e;
        }
        return null;
    }
}

struct CacheSlot {
    occupied: bool,
    origin_host: [128]u8,
    origin_host_len: u8,
    origin_port: u16,
    h3_port: u16,
    expires_at: i64,
}

/// Fixed-capacity zero-allocation Alt-Svc origin -> H3 port cache (up to 64 origins).
pub struct AltSvcCache {
    slots: [64]CacheSlot,

    pub static fn init(): AltSvcCache {
        return AltSvcCache{
            slots: [CacheSlot{
                occupied: false,
                origin_host: [0; 128],
                origin_host_len: 0,
                origin_port: 0,
                h3_port: 0,
                expires_at: 0,
            }; 64],
        };
    }

    /// Record an `Alt-Svc` header received from `(origin_host, origin_port)` at `now_sec`.
    /// Handles `Alt-Svc: clear` by evicting any cached entry for that origin.
    pub fn record(*_, origin_host: []const u8, origin_port: u16, field_value: []const u8, now_sec: i64) {
        if (origin_host.len == 0 || origin_host.len > 128) return;
        const parsed = AltSvcHeader.parse(field_value);
        if (parsed.is_clear) {
            _.remove(origin_host, origin_port);
            return;
        }
        const h3 = parsed.firstSameHostH3() ?? return;
        const exp = now_sec + @as(i64, h3.max_age);

        // Update existing slot if present
        let empty_idx: usize? = null;
        let i: usize = 0;
        while (i < 64) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].expires_at <= now_sec) {
                    _.slots[i].occupied = false;
                    if (empty_idx == null) empty_idx = i;
                    continue;
                }
                if (_.slots[i].origin_port == origin_port &&
                    strEq(_.slots[i].origin_host[0.._.slots[i].origin_host_len], origin_host))
                {
                    _.slots[i].h3_port = h3.port;
                    _.slots[i].expires_at = exp;
                    return;
                }
            } else if (empty_idx == null) {
                empty_idx = i;
            }
        }

        if (idx := empty_idx) {
            _.slots[idx].occupied = true;
            _.slots[idx].origin_host_len = @intCast(origin_host.len);
            let k: usize = 0;
            while (k < origin_host.len) : (k += 1) {
                _.slots[idx].origin_host[k] = origin_host[k];
            }
            _.slots[idx].origin_port = origin_port;
            _.slots[idx].h3_port = h3.port;
            _.slots[idx].expires_at = exp;
        }
    }

    /// Look up a cached H3 port for `(origin_host, origin_port)` at `now_sec`.
    pub fn lookup(*_, origin_host: []const u8, origin_port: u16, now_sec: i64): u16? {
        let i: usize = 0;
        while (i < 64) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            if (_.slots[i].origin_port == origin_port &&
                strEq(_.slots[i].origin_host[0.._.slots[i].origin_host_len], origin_host))
            {
                if (now_sec >= _.slots[i].expires_at) {
                    _.slots[i].occupied = false;
                    return null;
                }
                return _.slots[i].h3_port;
            }
        }
        return null;
    }

    pub fn remove(*_, origin_host: []const u8, origin_port: u16) {
        let i: usize = 0;
        while (i < 64) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            if (_.slots[i].origin_port == origin_port &&
                strEq(_.slots[i].origin_host[0.._.slots[i].origin_host_len], origin_host))
            {
                _.slots[i].occupied = false;
                return;
            }
        }
    }
}

// ---------- Internal parsing helpers ----------

fn parseSingleEntry(item: []const u8): AltSvcEntry? {
    let semi_pos: usize = 0;
    let in_quote = false;
    while (semi_pos < item.len) : (semi_pos += 1) {
        if (item[semi_pos] == '"') {
            in_quote = !in_quote;
        } else if (item[semi_pos] == ';' && !in_quote) {
            break;
        }
    }

    const alt = trimOws(item[0..semi_pos]);
    const eq_pos = findChar(alt, '=') ?? return null;
    const proto = trimOws(alt[0..eq_pos]);
    if (proto.len == 0) return null;

    let auth = trimOws(alt[eq_pos + 1 ..]);
    if (auth.len >= 2 && auth[0] == '"' && auth[auth.len - 1] == '"') {
        auth = auth[1 .. auth.len - 1];
    }
    const colon_pos = findLastChar(auth, ':') ?? return null;
    const host = auth[0..colon_pos];
    const port = parseU16(auth[colon_pos + 1 ..]) ?? return null;
    if (port == 0) return null;

    let max_age: u32 = DEFAULT_MAX_AGE;
    let persist = false;

    let p = semi_pos;
    while (p < item.len) {
        if (item[p] == ';') p += 1;
        const p_start = p;
        while (p < item.len && item[p] != ';') : (p += 1) {}
        const param = trimOws(item[p_start..p]);
        if (peq := findChar(param, '=')) {
            const pk = trimOws(param[0..peq]);
            const pv = trimOws(param[peq + 1 ..]);
            if (eqIgnoreCase(pk, "ma")) {
                if (v := parseU32(pv)) max_age = v;
            } else if (eqIgnoreCase(pk, "persist")) {
                if (strEq(pv, "1")) persist = true;
            }
        }
    }

    return AltSvcEntry{
        protocol: proto,
        host: host,
        port: port,
        max_age: max_age,
        persist: persist,
    };
}

fn findChar(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return null;
}

fn findLastChar(s: []const u8, ch: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) return i;
    }
    return null;
}

fn parseU16(s: []const u8): u16? {
    if (s.len == 0) return null;
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return null;
        acc = acc * 10 + @as(u32, s[i] - '0');
        if (acc > 65535) return null;
    }
    return @intCast(acc);
}

fn parseU32(s: []const u8): u32? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return null;
        acc = acc * 10 + @as(u64, s[i] - '0');
        if (acc > 4294967295) return null;
    }
    return @intCast(acc);
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
