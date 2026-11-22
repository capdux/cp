// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (RFC 6265 Cookie Domain/Path Match & Send Policy)

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return strEql(s[0..prefix.len], prefix);
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

fn isIpv4Literal(s: []const u8): bool {
    if (s.len < 7 || s.len > 15) {
        return false;
    }
    let parts: usize = 0;
    let i: usize = 0;
    while (parts < 4) {
        if (i >= s.len) return false;
        const start = i;
        let val: u32 = 0;
        while (i < s.len && s[i] != '.') {
            if (s[i] < '0' || s[i] > '9') return false;
            val = val * 10 + u32(s[i] - '0');
            if (val > 255) return false;
            i += 1;
        }
        if (i == start) return false;
        parts += 1;
        if (parts < 4) {
            if (i >= s.len || s[i] != '.') return false;
            i += 1;
        }
    }
    return i == s.len;
}

fn isIpLiteral(host: []const u8): bool {
    for (c of host) {
        if (c == ':') {
            return true;
        }
    }
    return isIpv4Literal(host);
}

fn stripIpv6Brackets(host: []const u8): []const u8 {
    if (host.len >= 2 && host[0] == '[' && host[host.len - 1] == ']') {
        return host[1..host.len - 1];
    }
    return host;
}

/// RFC 6265 §5.1.4 默认 Cookie 路径计算（零拷贝切片）
export fn defaultCookiePath(request_uri_path: []const u8): []const u8 {
    if (request_uri_path.len == 0 || request_uri_path[0] != '/') {
        return "/";
    }
    let last_slash: usize = 0;
    let i: usize = 1;
    while (i < request_uri_path.len) {
        if (request_uri_path[i] == '/') {
            last_slash = i;
        }
        i += 1;
    }
    if (last_slash == 0) {
        return "/";
    }
    return request_uri_path[0..last_slash];
}

/// RFC 6265 §5.1.3 Cookie 域名匹配
/// 忽略 cookie_domain 前导 `.`、大小写无关；host_only 或 IP 字面量强制仅精确匹配
export fn cookieDomainMatches(request_host: []const u8, cookie_domain: []const u8, host_only: bool): bool {
    const req = stripIpv6Brackets(request_host);
    let dom = stripIpv6Brackets(cookie_domain);
    if (dom.len > 0 && dom[0] == '.') {
        dom = dom[1..];
    }
    if (req.len == 0 || dom.len == 0) {
        return false;
    }

    if (asciiEqlIgnoreCase(req, dom)) {
        return true;
    }
    if (host_only || isIpLiteral(req)) {
        return false;
    }
    if (req.len > dom.len) {
        const dot_idx = req.len - dom.len - 1;
        if (req[dot_idx] == '.' && asciiEqlIgnoreCase(req[dot_idx + 1..], dom)) {
            return true;
        }
    }
    return false;
}

/// RFC 6265 §5.1.4 Cookie 路径匹配（严格按 `/` 边界防 `/admin2` 误匹配 `/admin`）
export fn cookiePathMatches(request_path: []const u8, cookie_path: []const u8): bool {
    if (cookie_path.len == 0) {
        return false;
    }
    if (strEql(request_path, cookie_path)) {
        return true;
    }
    if (request_path.len > cookie_path.len && startsWith(request_path, cookie_path)) {
        if (cookie_path[cookie_path.len - 1] == '/') {
            return true;
        }
        if (request_path[cookie_path.len] == '/') {
            return true;
        }
    }
    return false;
}

/// RFC 6265bis Cookie 安全名称前缀分类
export enum CookiePrefixKind {
    none,
    secure_prefix,
    host_prefix,
}

/// 检测 Cookie 名称是否带有 `__Secure-` 或 `__Host-` 前缀
export fn detectCookiePrefix(name: []const u8): CookiePrefixKind {
    if (startsWith(name, "__Host-")) {
        return CookiePrefixKind.host_prefix;
    }
    if (startsWith(name, "__Secure-")) {
        return CookiePrefixKind.secure_prefix;
    }
    return CookiePrefixKind.none;
}

/// 按 RFC 6265bis 校验 `__Secure-` 与 `__Host-` Cookie 前缀安全约束
export fn validateCookiePrefix(name: []const u8, secure: bool, domain_set: bool, path: []const u8): bool {
    const kind = detectCookiePrefix(name);
    return switch (kind) {
        .none => true,
        .secure_prefix => secure,
        .host_prefix => secure && !domain_set && strEql(path, "/"),
    };
}

/// 综合判定客户端在给定请求上下文中是否允许发送该 Cookie
export fn canSendCookie(
    request_host: []const u8,
    request_path: []const u8,
    is_https: bool,
    is_http_api: bool,
    cookie_domain: []const u8,
    host_only: bool,
    cookie_path: []const u8,
    secure_only: bool,
    http_only: bool,
): bool {
    if (secure_only && !is_https) {
        return false;
    }
    if (http_only && !is_http_api) {
        return false;
    }
    if (!cookieDomainMatches(request_host, cookie_domain, host_only)) {
        return false;
    }
    return cookiePathMatches(request_path, cookie_path);
}
