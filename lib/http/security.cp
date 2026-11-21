// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-21T20:23:41Z (HTTP Security Headers HSTS / Referrer / COOP / COEP)

const U64_MAX: u64 = 0xFFFF_FFFF_FFFF_FFFF;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn parseU64Decimal(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let val: u64 = 0;
    for (c of s) {
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u64 = u64(c - '0');
        if (val > (U64_MAX - digit) / 10) {
            return null;
        }
        val = val * 10 + digit;
    }
    return val;
}

fn stripSfvParams(header: []const u8): []const u8 {
    let s = trimAscii(header);
    if (semi := findByte(s, ';')) {
        s = trimAscii(s[0..semi]);
    }
    return s;
}

/// RFC 6797 Strict-Transport-Security (HSTS) 策略（零堆分配）
export struct HstsPolicy {
    max_age: u64;
    include_subdomains: bool;
    preload: bool;

    /// 解析 `Strict-Transport-Security` 头部（必须含合法非负 max-age，大小写无关，支持引号值）
    fn parse(header: []const u8): HstsPolicy? {
        let has_max_age = false;
        let max_age: u64 = 0;
        let include_subdomains = false;
        let preload = false;

        let start: usize = 0;
        let i: usize = 0;
        while (i <= header.len) {
            if (i == header.len || header[i] == ';') {
                const seg = trimAscii(header[start..i]);
                if (seg.len > 0) {
                    if (eq_idx := findByte(seg, '=')) {
                        const key = trimAscii(seg[0..eq_idx]);
                        let val = trimAscii(seg[eq_idx + 1..]);
                        if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                            val = val[1..val.len - 1];
                        }
                        if (asciiEqlIgnoreCase(key, "max-age")) {
                            if (has_max_age) {
                                return null;
                            }
                            max_age = parseU64Decimal(val) ?? return null;
                            has_max_age = true;
                        } else if (asciiEqlIgnoreCase(key, "includeSubDomains") || asciiEqlIgnoreCase(key, "preload")) {
                            return null;
                        }
                    } else {
                        if (asciiEqlIgnoreCase(seg, "includeSubDomains")) {
                            include_subdomains = true;
                        } else if (asciiEqlIgnoreCase(seg, "preload")) {
                            preload = true;
                        }
                    }
                }
                start = i + 1;
            }
            i += 1;
        }

        if (!has_max_age) {
            return null;
        }
        return HstsPolicy{
            max_age: max_age,
            include_subdomains: include_subdomains,
            preload: preload,
        };
    }

    /// 是否满足 HSTS Preload 列表准入标准（max-age >= 1 年且启用 includeSubDomains 与 preload）
    fn isPreloadEligible(): bool {
        return _.max_age >= 31_536_000 && _.include_subdomains && _.preload;
    }

    /// 规范化格式化为 `max-age=<N>[; includeSubDomains][; preload]`
    fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        const prefix = "max-age=";
        if (pos + prefix.len > out.len) return null;
        for (c of prefix) {
            out[pos] = c;
            pos += 1;
        }

        let tmp: [20]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let v = _.max_age;
        let dlen: usize = 0;
        if (v == 0) {
            tmp[0] = '0';
            dlen = 1;
        } else {
            while (v > 0) {
                tmp[dlen] = u8('0') + u8(v % 10);
                v /= 10;
                dlen += 1;
            }
        }
        if (pos + dlen > out.len) return null;
        let k: usize = dlen;
        while (k > 0) {
            k -= 1;
            out[pos] = tmp[k];
            pos += 1;
        }

        if (_.include_subdomains) {
            const sub = "; includeSubDomains";
            if (pos + sub.len > out.len) return null;
            for (c of sub) {
                out[pos] = c;
                pos += 1;
            }
        }
        if (_.preload) {
            const pre = "; preload";
            if (pos + pre.len > out.len) return null;
            for (c of pre) {
                out[pos] = c;
                pos += 1;
            }
        }
        return out[0..pos];
    }
}

/// W3C Referrer-Policy 8 种标准策略
export enum ReferrerPolicy {
    no_referrer,
    no_referrer_when_downgrade,
    origin,
    origin_when_cross_origin,
    same_origin,
    strict_origin,
    strict_origin_when_cross_origin,
    unsafe_url,

    fn asStr(): []const u8 {
        return switch (_) {
            .no_referrer => "no-referrer",
            .no_referrer_when_downgrade => "no-referrer-when-downgrade",
            .origin => "origin",
            .origin_when_cross_origin => "origin-when-cross-origin",
            .same_origin => "same-origin",
            .strict_origin => "strict-origin",
            .strict_origin_when_cross_origin => "strict-origin-when-cross-origin",
            .unsafe_url => "unsafe-url",
        };
    }
}

fn parseSingleReferrerToken(tok: []const u8): ReferrerPolicy? {
    if (asciiEqlIgnoreCase(tok, "no-referrer")) return ReferrerPolicy.no_referrer;
    if (asciiEqlIgnoreCase(tok, "no-referrer-when-downgrade")) return ReferrerPolicy.no_referrer_when_downgrade;
    if (asciiEqlIgnoreCase(tok, "origin")) return ReferrerPolicy.origin;
    if (asciiEqlIgnoreCase(tok, "origin-when-cross-origin")) return ReferrerPolicy.origin_when_cross_origin;
    if (asciiEqlIgnoreCase(tok, "same-origin")) return ReferrerPolicy.same_origin;
    if (asciiEqlIgnoreCase(tok, "strict-origin")) return ReferrerPolicy.strict_origin;
    if (asciiEqlIgnoreCase(tok, "strict-origin-when-cross-origin")) return ReferrerPolicy.strict_origin_when_cross_origin;
    if (asciiEqlIgnoreCase(tok, "unsafe-url")) return ReferrerPolicy.unsafe_url;
    return null;
}

/// 解析 `Referrer-Policy` 头部（W3C §8.1：支持逗号分隔回退链，取最后一个合法策略）
export fn parseReferrerPolicy(header: []const u8): ReferrerPolicy? {
    let result: ReferrerPolicy? = null;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= header.len) {
        if (i == header.len || header[i] == ',') {
            const tok = trimAscii(header[start..i]);
            if (tok.len > 0) {
                if (pol := parseSingleReferrerToken(tok)) {
                    result = pol;
                }
            }
            start = i + 1;
        }
        i += 1;
    }
    return result;
}

/// RFC 7034 X-Frame-Options 策略
export enum FrameOptions {
    deny,
    sameorigin,

    fn asStr(): []const u8 {
        return switch (_) {
            .deny => "DENY",
            .sameorigin => "SAMEORIGIN",
        };
    }
}

/// 解析 `X-Frame-Options` 头部（大小写无关匹配 DENY / SAMEORIGIN）
export fn parseFrameOptions(header: []const u8): FrameOptions? {
    const s = trimAscii(header);
    if (asciiEqlIgnoreCase(s, "DENY")) return FrameOptions.deny;
    if (asciiEqlIgnoreCase(s, "SAMEORIGIN")) return FrameOptions.sameorigin;
    return null;
}

/// Cross-Origin-Opener-Policy (COOP)
export enum CoopPolicy {
    unsafe_none,
    same_origin_allow_popups,
    same_origin,

    fn asStr(): []const u8 {
        return switch (_) {
            .unsafe_none => "unsafe-none",
            .same_origin_allow_popups => "same-origin-allow-popups",
            .same_origin => "same-origin",
        };
    }
}

/// 解析 `Cross-Origin-Opener-Policy` 头部（自动忽略 `;report-to=...` 参数）
export fn parseCoopPolicy(header: []const u8): CoopPolicy? {
    const tok = stripSfvParams(header);
    if (asciiEqlIgnoreCase(tok, "unsafe-none")) return CoopPolicy.unsafe_none;
    if (asciiEqlIgnoreCase(tok, "same-origin-allow-popups")) return CoopPolicy.same_origin_allow_popups;
    if (asciiEqlIgnoreCase(tok, "same-origin")) return CoopPolicy.same_origin;
    return null;
}

/// Cross-Origin-Embedder-Policy (COEP)
export enum CoepPolicy {
    unsafe_none,
    require_corp,
    credentialless,

    fn asStr(): []const u8 {
        return switch (_) {
            .unsafe_none => "unsafe-none",
            .require_corp => "require-corp",
            .credentialless => "credentialless",
        };
    }
}

/// 解析 `Cross-Origin-Embedder-Policy` 头部（自动忽略 `;report-to=...` 参数）
export fn parseCoepPolicy(header: []const u8): CoepPolicy? {
    const tok = stripSfvParams(header);
    if (asciiEqlIgnoreCase(tok, "unsafe-none")) return CoepPolicy.unsafe_none;
    if (asciiEqlIgnoreCase(tok, "require-corp")) return CoepPolicy.require_corp;
    if (asciiEqlIgnoreCase(tok, "credentialless")) return CoepPolicy.credentialless;
    return null;
}

/// Cross-Origin-Resource-Policy (CORP)
export enum CorpPolicy {
    same_site,
    same_origin,
    cross_origin,

    fn asStr(): []const u8 {
        return switch (_) {
            .same_site => "same-site",
            .same_origin => "same-origin",
            .cross_origin => "cross-origin",
        };
    }
}

/// 解析 `Cross-Origin-Resource-Policy` 头部
export fn parseCorpPolicy(header: []const u8): CorpPolicy? {
    const tok = trimAscii(header);
    if (asciiEqlIgnoreCase(tok, "same-site")) return CorpPolicy.same_site;
    if (asciiEqlIgnoreCase(tok, "same-origin")) return CorpPolicy.same_origin;
    if (asciiEqlIgnoreCase(tok, "cross-origin")) return CorpPolicy.cross_origin;
    return null;
}

/// 判断给定 COOP + COEP 组合是否满足 WHATWG Cross-Origin Isolation 跨域隔离条件
export fn isCrossOriginIsolated(coop: CoopPolicy, coep: CoepPolicy): bool {
    if (coop != CoopPolicy.same_origin) {
        return false;
    }
    return coep == CoepPolicy.require_corp || coep == CoepPolicy.credentialless;
}
