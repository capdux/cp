// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T14:26:00Z (HTTP User-Agent Product Token Parser & Classifier)
// Pure-CP zero-allocation RFC 9110 User-Agent product token iterator, browser/OS/bot classifier.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
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

export struct ProductToken {
    name: []const u8,
    version: ([]const u8)?,
}

/// Zero-copy iterator over RFC 9110 §10.1.5 `product[/version]` tokens (skipping parenthesized `(...)` comments).
export struct UserAgentIter {
    src: []const u8,
    pos: usize,

    pub fn init(ua: []const u8): UserAgentIter {
        return UserAgentIter { src: ua, pos: 0 };
    }

    pub fn next(*_): ProductToken? {
        while (_.pos < _.src.len) {
            while (_.pos < _.src.len && isWs(_.src[_.pos])) _.pos += 1;
            if (_.pos >= _.src.len) return null;

            // Skip nested parenthesized comments `(...)`
            if (_.src[_.pos] == '(') {
                let depth: usize = 0;
                while (_.pos < _.src.len) {
                    const c = _.src[_.pos];
                    _.pos += 1;
                    if (c == '\\' && _.pos < _.src.len) {
                        _.pos += 1;
                    } else if (c == '(') {
                        depth += 1;
                    } else if (c == ')') {
                        if (depth > 0) depth -= 1;
                        if (depth == 0) break;
                    }
                }
                continue;
            }

            const start = _.pos;
            while (_.pos < _.src.len && !isWs(_.src[_.pos]) && _.src[_.pos] != '(') {
                _.pos += 1;
            }
            const seg = _.src[start.._.pos];
            if (seg.len == 0) continue;

            let slash: usize? = null;
            let k: usize = 0;
            while (k < seg.len) {
                if (seg[k] == '/') {
                    slash = k;
                    break;
                }
                k += 1;
            }
            if (sp := slash) {
                const name = seg[0..sp];
                const ver = seg[sp + 1..];
                if (name.len == 0) continue;
                return ProductToken {
                    name: name,
                    version: if (ver.len > 0) ver else null,
                };
            }
            return ProductToken { name: seg, version: null };
        }
        return null;
    }
}

export enum BrowserFamily {
    unknown,
    chrome,
    edge,
    firefox,
    safari,
    opera,
    curl,
    bun,
    node,
    bot,
}

export enum OsFamily {
    unknown,
    windows,
    macos,
    linux,
    android,
    ios,
}

export fn isBotUserAgent(ua: []const u8): bool {
    return containsIgnoreCase(ua, "bot") ||
        containsIgnoreCase(ua, "crawler") ||
        containsIgnoreCase(ua, "spider") ||
        containsIgnoreCase(ua, "slurp") ||
        containsIgnoreCase(ua, "headlesschrome");
}

export fn isMobileUserAgent(ua: []const u8): bool {
    return containsIgnoreCase(ua, "mobile") ||
        containsIgnoreCase(ua, "iphone") ||
        containsIgnoreCase(ua, "android");
}

export fn detectBrowser(ua: []const u8): BrowserFamily {
    if (isBotUserAgent(ua)) return BrowserFamily.bot;
    if (containsIgnoreCase(ua, "Bun/")) return BrowserFamily.bun;
    if (containsIgnoreCase(ua, "Node.js/") || containsIgnoreCase(ua, "undici")) return BrowserFamily.node;
    if (containsIgnoreCase(ua, "curl/")) return BrowserFamily.curl;
    if (containsIgnoreCase(ua, "Edg/") || containsIgnoreCase(ua, "Edge/")) return BrowserFamily.edge;
    if (containsIgnoreCase(ua, "OPR/") || containsIgnoreCase(ua, "Opera")) return BrowserFamily.opera;
    if (containsIgnoreCase(ua, "Firefox/") || containsIgnoreCase(ua, "FxiOS/")) return BrowserFamily.firefox;
    if (containsIgnoreCase(ua, "Chrome/") || containsIgnoreCase(ua, "CriOS/")) return BrowserFamily.chrome;
    if (containsIgnoreCase(ua, "Safari/")) return BrowserFamily.safari;
    return BrowserFamily.unknown;
}

export fn detectOs(ua: []const u8): OsFamily {
    if (containsIgnoreCase(ua, "Android")) return OsFamily.android;
    if (containsIgnoreCase(ua, "iPhone") || containsIgnoreCase(ua, "iPad") || containsIgnoreCase(ua, "iPod")) {
        return OsFamily.ios;
    }
    if (containsIgnoreCase(ua, "Windows")) return OsFamily.windows;
    if (containsIgnoreCase(ua, "Mac OS X") || containsIgnoreCase(ua, "Macintosh")) return OsFamily.macos;
    if (containsIgnoreCase(ua, "Linux") || containsIgnoreCase(ua, "X11")) return OsFamily.linux;
    return OsFamily.unknown;
}

/// Find version string for a specific product token (e.g. `"Chrome"`, `"Bun"`, `"curl"`).
export fn findProductVersion(ua: []const u8, product_name: []const u8): ([]const u8)? {
    let it = UserAgentIter.init(ua);
    while (tok := it.next()) {
        if (eqIgnoreCase(tok.name, product_name)) return tok.version;
    }
    return null;
}
