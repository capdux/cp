// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (W3C Sec-CH-UA Client Hints Parser)
// Pure-CP zero-allocation W3C User-Agent Client Hints parser (`Sec-CH-UA`, `Sec-CH-UA-Mobile`, `Sec-CH-UA-Platform`).

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

fn containsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (eqIgnoreCase(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

fn unquoteSfString(s: []const u8): ([]const u8)? {
    const t = trimWs(s);
    if (t.len < 2 || t[0] != '"' || t[t.len - 1] != '"') return null;
    return t[1..t.len - 1];
}

export struct BrandVersion {
    brand: []const u8,
    version: []const u8,

    /// True for GREASE placeholder brands per WICG UA-CH (e.g. `"Not;A=Brand"`, `"Not/A)Brand"`).
    pub fn isGrease(): bool {
        return containsIgnoreCase(_.brand, "not") && containsIgnoreCase(_.brand, "brand");
    }
}

/// Zero-copy iterator over `Sec-CH-UA` or `Sec-CH-UA-Full-Version-List` structured-field brand lists:
/// `"Chromium";v="128", "Not;A=Brand";v="24", "Google Chrome";v="128"`.
export struct BrandListIter {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): BrandListIter {
        return BrandListIter { src: header, pos: 0 };
    }

    pub fn next(*_): BrandVersion? {
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
            if (_.pos < _.src.len && _.src[_.pos] == ',') _.pos += 1;

            const item = trimWs(_.src[start..end]);
            if (item.len == 0) continue;

            // Split at first ';' outside quotes
            let semi: usize? = null;
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
                        semi = k;
                        break;
                    }
                }
                k += 1;
            }
            if (semi == null) continue;
            const sp = semi.?;
            const brand = unquoteSfString(item[0..sp]) ?? continue;

            // Parse `;v="..."`
            const param = trimWs(item[sp + 1..]);
            let eq: usize? = null;
            let pi: usize = 0;
            while (pi < param.len) {
                if (param[pi] == '=') {
                    eq = pi;
                    break;
                }
                pi += 1;
            }
            if (eq == null) continue;
            const ep = eq.?;
            const pkey = trimWs(param[0..ep]);
            if (!eqIgnoreCase(pkey, "v")) continue;
            const ver = unquoteSfString(param[ep + 1..]) ?? continue;

            return BrandVersion { brand: brand, version: ver };
        }
        return null;
    }
}

/// Find version string for a specific brand in `Sec-CH-UA` (case-insensitive, e.g. `"Chromium"`, `"Google Chrome"`).
export fn findBrandVersion(sec_ch_ua: []const u8, brand_name: []const u8): ([]const u8)? {
    let it = BrandListIter.init(sec_ch_ua);
    while (bv := it.next()) {
        if (eqIgnoreCase(bv.brand, brand_name)) return bv.version;
    }
    return null;
}

/// Parse `Sec-CH-UA-Mobile` RFC 8941 boolean (`?1` -> true, `?0` -> false, invalid -> null).
export fn parseMobileHint(sec_ch_ua_mobile: []const u8): bool? {
    const s = trimWs(sec_ch_ua_mobile);
    if (s.len == 2 && s[0] == '?') {
        if (s[1] == '1') return true;
        if (s[1] == '0') return false;
    }
    return null;
}

/// Parse `Sec-CH-UA-Platform` quoted SF string (e.g. `"macOS"` -> `"macOS"`, `"Linux"` -> `"Linux"`).
export fn parsePlatformHint(sec_ch_ua_platform: []const u8): ([]const u8)? {
    return unquoteSfString(sec_ch_ua_platform);
}
