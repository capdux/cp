// Origin: Pure-CP Stdlib (Zero-Alloc W3C Content-Security-Policy Level 3 Parser, Evaluator & Builder) | Created: 2026-11-23
// Reference-hardened against tower-rs/tower-http (MIT) & W3C Fetch/CSP3/RFC 9110
// Pure CP zero-heap-allocation W3C CSP Level 3 engine:
//   - Fixed-capacity parser (`parseHeader`) with `;` directive splitting, W3C first-directive-wins
//     duplicate handling, ASCII case-insensitive directive matching, and 16-lane SIMD CRLF/CTL rejection
//   - Exact W3C CSP3 fallback chain (`effectiveDirective`): fetch directives (`script-src`, `style-src`,
//     `img-src`, `connect-src`, `font-src`, `frame-src`, `object-src`) fall back to `default-src`,
//     whereas `base-uri`, `form-action`, and `frame-ancestors` NEVER fall back to `default-src`
//   - Strict CSP3 `'none'` coexistence rule (`isEffectiveNone`), Base64/Base64URL validation for
//     `'nonce-'` / `'sha256-'` / `'sha384-'` / `'sha512-'`, and `'unsafe-inline'` suppression
//   - Zero-allocation canonical header serializer (`formatHeader`)

import {
    simdWebHasControlOrCrlf,
    simdWebBytesEqual,
    simdWebAsciiLowerEql,
} from "./corsGuard.cp";

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of directives stored inline in a `CspPolicy`.
pub const CSP_MAX_DIRECTIVES: usize = 16;

/// Maximum number of source expressions stored inline per `CspDirective`.
pub const CSP_MAX_SOURCES_PER_DIR: usize = 16;

/// Maximum UTF-8 byte length of a single CSP source token.
pub const CSP_MAX_TOKEN_LEN: usize = 96;

/// Supported W3C Content-Security-Policy Level 3 directives.
pub enum CspDirectiveKind {
    default_src,
    script_src,
    style_src,
    img_src,
    connect_src,
    font_src,
    frame_src,
    frame_ancestors,
    object_src,
    base_uri,
    form_action,
    upgrade_insecure_requests,
}

/// Classification of a single CSP source expression token.
pub enum CspSourceKind {
    kw_none,
    kw_self,
    kw_unsafe_inline,
    kw_unsafe_eval,
    kw_strict_dynamic,
    wildcard,
    scheme,
    host_expr,
    nonce,
    hash_sha256,
    hash_sha384,
    hash_sha512,
}

/// Evaluation verdict returned by `CspPolicy.checkResourceUrl` and `checkInlineScript`.
pub enum CspEvalVerdict {
    allowed,
    blocked_by_none,
    blocked_no_match,
    blocked_unsafe_inline,
    invalid_input,
}

// ============================================================================
// 2. Pure ASCII & URL Parsing Helpers
// ============================================================================

fn cspToLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn cspIsAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn cspHasControlOrCrlfChar(s: []const u8): bool {
    return simdWebHasControlOrCrlf(s);
}

fn cspHasHeaderIllegalCtrl(s: []const u8): bool {
    if (!simdWebHasControlOrCrlf(s)) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if ((c < 0x20 && c != '\t') || c == 0x7F) return true;
    }
    return false;
}

fn cspTrimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && cspIsAsciiSpace(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && cspIsAsciiSpace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn cspBytesEqual(a: []const u8, b: []const u8): bool {
    return simdWebBytesEqual(a, b);
}

fn cspEqIgnoreCase(a: []const u8, b: []const u8): bool {
    return simdWebAsciiLowerEql(a, b);
}

fn cspStartsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return simdWebAsciiLowerEql(s[0..prefix.len], prefix);
}

fn cspIsValidBase64Value(s: []const u8): bool {
    if (s.len == 0) return false;
    let seen_pad = false;
    let pad_count: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '=') {
            seen_pad = true;
            pad_count += 1;
            if (pad_count > 2) return false;
        } else {
            if (seen_pad) return false;
            const ok = (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                c == '+' || c == '/' || c == '-' || c == '_';
            if (!ok) return false;
        }
    }
    return pad_count < s.len;
}

fn cspIsValidDirectiveName(s: []const u8): bool {
    if (s.len == 0 || s.len > 64) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        const ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
        if (!ok) return false;
    }
    return true;
}

fn cspIsValidSchemeName(s: []const u8): bool {
    if (s.len == 0) return false;
    const c0 = cspToLowerAscii(s[0]);
    if (c0 < 'a' || c0 > 'z') return false;
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        const c = cspToLowerAscii(s[i]);
        const ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
        if (!ok) return false;
    }
    return true;
}

fn parseDirectiveKind(name: []const u8): CspDirectiveKind? {
    if (cspEqIgnoreCase(name, "default-src")) return CspDirectiveKind.default_src;
    if (cspEqIgnoreCase(name, "script-src")) return CspDirectiveKind.script_src;
    if (cspEqIgnoreCase(name, "style-src")) return CspDirectiveKind.style_src;
    if (cspEqIgnoreCase(name, "img-src")) return CspDirectiveKind.img_src;
    if (cspEqIgnoreCase(name, "connect-src")) return CspDirectiveKind.connect_src;
    if (cspEqIgnoreCase(name, "font-src")) return CspDirectiveKind.font_src;
    if (cspEqIgnoreCase(name, "frame-src")) return CspDirectiveKind.frame_src;
    if (cspEqIgnoreCase(name, "frame-ancestors")) return CspDirectiveKind.frame_ancestors;
    if (cspEqIgnoreCase(name, "object-src")) return CspDirectiveKind.object_src;
    if (cspEqIgnoreCase(name, "base-uri")) return CspDirectiveKind.base_uri;
    if (cspEqIgnoreCase(name, "form-action")) return CspDirectiveKind.form_action;
    if (cspEqIgnoreCase(name, "upgrade-insecure-requests")) return CspDirectiveKind.upgrade_insecure_requests;
    return null;
}

fn formatDirectiveName(kind: CspDirectiveKind): []const u8 {
    if (kind == CspDirectiveKind.default_src) return "default-src";
    if (kind == CspDirectiveKind.script_src) return "script-src";
    if (kind == CspDirectiveKind.style_src) return "style-src";
    if (kind == CspDirectiveKind.img_src) return "img-src";
    if (kind == CspDirectiveKind.connect_src) return "connect-src";
    if (kind == CspDirectiveKind.font_src) return "font-src";
    if (kind == CspDirectiveKind.frame_src) return "frame-src";
    if (kind == CspDirectiveKind.frame_ancestors) return "frame-ancestors";
    if (kind == CspDirectiveKind.object_src) return "object-src";
    if (kind == CspDirectiveKind.base_uri) return "base-uri";
    if (kind == CspDirectiveKind.form_action) return "form-action";
    return "upgrade-insecure-requests";
}

fn fallsBackToDefaultSrc(kind: CspDirectiveKind): bool {
    return kind == CspDirectiveKind.script_src ||
        kind == CspDirectiveKind.style_src ||
        kind == CspDirectiveKind.img_src ||
        kind == CspDirectiveKind.connect_src ||
        kind == CspDirectiveKind.font_src ||
        kind == CspDirectiveKind.frame_src ||
        kind == CspDirectiveKind.object_src;
}

fn defaultPortForScheme(scheme: []const u8): u16 {
    if (cspEqIgnoreCase(scheme, "http") || cspEqIgnoreCase(scheme, "ws")) return 80;
    if (cspEqIgnoreCase(scheme, "https") || cspEqIgnoreCase(scheme, "wss")) return 443;
    return 0;
}

fn isNetworkScheme(scheme: []const u8): bool {
    return cspEqIgnoreCase(scheme, "http") ||
        cspEqIgnoreCase(scheme, "https") ||
        cspEqIgnoreCase(scheme, "ws") ||
        cspEqIgnoreCase(scheme, "wss");
}

fn parsePortU16(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return null;
        val = val * 10 + @as(u32, s[i] - '0');
        if (val > 65535) return null;
    }
    return @as(u16, @intCast(val));
}

struct ParsedUrlParts {
    scheme: []const u8 = "",
    host: []const u8 = "",
    port: u16 = 0,
    has_authority: bool = false,
}

fn parseUrlParts(url: []const u8): ParsedUrlParts? {
    if (url.len == 0 || cspHasControlOrCrlfChar(url)) return null;

    let colon_idx: usize = url.len;
    let i: usize = 0;
    while (i < url.len) : (i += 1) {
        if (url[i] == ':') {
            colon_idx = i;
            break;
        }
    }
    if (colon_idx == url.len || colon_idx == 0) return null;

    const scheme: []const u8 = url[0..colon_idx];
    if (!cspIsValidSchemeName(scheme)) return null;

    let parts = ParsedUrlParts{};
    parts.scheme = scheme;
    parts.port = defaultPortForScheme(scheme);

    if (colon_idx + 2 < url.len && url[colon_idx + 1] == '/' && url[colon_idx + 2] == '/') {
        parts.has_authority = true;
        const auth_start = colon_idx + 3;
        let auth_end = auth_start;
        while (auth_end < url.len && url[auth_end] != '/' && url[auth_end] != '?' && url[auth_end] != '#') : (auth_end += 1) {}
        const auth: []const u8 = url[auth_start..auth_end];
        if (auth.len == 0) return null;

        // Split host[:port] (simple non-IPv6 or bracketed IPv6)
        let port_colon: usize = auth.len;
        let k: usize = 0;
        while (k < auth.len) : (k += 1) {
            if (auth[k] == ':') port_colon = k;
        }
        if (port_colon < auth.len) {
            parts.host = auth[0..port_colon];
            const parsed_port = parsePortU16(auth[port_colon + 1 .. auth.len]) ?? return null;
            parts.port = parsed_port;
        } else {
            parts.host = auth;
        }
        if (parts.host.len == 0) return null;
    } else if (isNetworkScheme(scheme)) {
        return null; // network URLs must have ://authority
    }

    return parts;
}

fn matchHostPattern(pattern: []const u8, target_host: []const u8): bool {
    if (pattern.len == 0 || target_host.len == 0) return false;
    if (cspBytesEqual(pattern, "*")) return true;

    // Strict subdomain wildcard: `*.example.com` matches `a.example.com` and `a.b.example.com`,
    // NOT `example.com` and NOT `evil-example.com`.
    if (pattern.len > 2 && pattern[0] == '*' && pattern[1] == '.') {
        const suffix: []const u8 = pattern[1..]; // `.example.com`
        if (target_host.len <= suffix.len) return false;
        const tail: []const u8 = target_host[target_host.len - suffix.len .. target_host.len];
        return cspEqIgnoreCase(tail, suffix);
    }

    return cspEqIgnoreCase(pattern, target_host);
}

fn matchHostSourceExpr(expr: []const u8, page_origin: []const u8, target: ParsedUrlParts): bool {
    if (!target.has_authority) return false;

    let rest: []const u8 = expr;
    let has_expr_scheme = false;
    let expr_scheme: []const u8 = "";

    // Check optional `scheme://`
    let i: usize = 0;
    while (i + 2 < rest.len) : (i += 1) {
        if (rest[i] == ':' && rest[i + 1] == '/' && rest[i + 2] == '/') {
            expr_scheme = rest[0..i];
            if (!cspIsValidSchemeName(expr_scheme)) return false;
            has_expr_scheme = true;
            rest = rest[i + 3 ..];
            break;
        }
    }

    if (has_expr_scheme) {
        if (!cspEqIgnoreCase(expr_scheme, target.scheme)) return false;
    } else {
        // Without explicit scheme, target scheme must match page origin (or https upgrade from http).
        if (origin := parseUrlParts(page_origin)) {
            const same = cspEqIgnoreCase(origin.scheme, target.scheme);
            const upgraded = cspEqIgnoreCase(origin.scheme, "http") && cspEqIgnoreCase(target.scheme, "https");
            if (!same && !upgraded) return false;
        }
    }

    // Strip optional path suffix from host expression
    let host_port_end: usize = 0;
    while (host_port_end < rest.len && rest[host_port_end] != '/') : (host_port_end += 1) {}
    const host_port: []const u8 = rest[0..host_port_end];
    if (host_port.len == 0) return false;

    let expr_host: []const u8 = host_port;
    let has_expr_port = false;
    let expr_port_wildcard = false;
    let expr_port: u16 = 0;

    let p_colon: usize = host_port.len;
    let k: usize = 0;
    while (k < host_port.len) : (k += 1) {
        if (host_port[k] == ':') p_colon = k;
    }
    if (p_colon < host_port.len) {
        expr_host = host_port[0..p_colon];
        const p_str: []const u8 = host_port[p_colon + 1 ..];
        has_expr_port = true;
        if (cspBytesEqual(p_str, "*")) {
            expr_port_wildcard = true;
        } else {
            expr_port = parsePortU16(p_str) ?? return false;
        }
    }

    if (!matchHostPattern(expr_host, target.host)) return false;

    if (has_expr_port) {
        if (!expr_port_wildcard && expr_port != target.port) return false;
    } else {
        // Default port match
        if (target.port != defaultPortForScheme(target.scheme)) return false;
    }

    return true;
}

fn cspAppendBytes(out_buf: *[]u8, pos: usize, s: []const u8): usize? {
    if (pos + s.len > out_buf.len) return null;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos + i] = s[i];
    }
    return pos + s.len;
}

// ============================================================================
// 3. CspSourceExpr & CspDirective
// ============================================================================

/// Parsed CSP source expression stored inline without heap allocation.
pub struct CspSourceExpr {
    kind: CspSourceKind = CspSourceKind.kw_none;
    token_buf: [CSP_MAX_TOKEN_LEN]u8 = [0; CSP_MAX_TOKEN_LEN];
    token_len: u16 = 0;

    /// Return the stored source token slice.
    fn token(): []const u8 {
        return token_buf[0..@as(usize, token_len)];
    }

    /// Parse a single CSP source token into a `CspSourceExpr`.
    /// Returns `null` on empty, oversized, control-character, or malformed tokens.
    static fn parse(raw: []const u8): CspSourceExpr? {
        if (raw.len == 0 || raw.len > CSP_MAX_TOKEN_LEN) return null;
        if (cspHasControlOrCrlfChar(raw)) return null;

        let i: usize = 0;
        while (i < raw.len) : (i += 1) {
            if (raw[i] == ' ' || raw[i] == '\t' || raw[i] == ';' || raw[i] == ',') return null;
        }

        let expr = CspSourceExpr{};
        let k: usize = 0;
        while (k < raw.len) : (k += 1) {
            expr.token_buf[k] = raw[k];
        }
        expr.token_len = @as(u16, @intCast(raw.len));

        if (raw[0] == '\'') {
            if (raw.len < 3 || raw[raw.len - 1] != '\'') return null;
            if (cspEqIgnoreCase(raw, "'none'")) {
                expr.kind = CspSourceKind.kw_none;
                return expr;
            }
            if (cspEqIgnoreCase(raw, "'self'")) {
                expr.kind = CspSourceKind.kw_self;
                return expr;
            }
            if (cspEqIgnoreCase(raw, "'unsafe-inline'")) {
                expr.kind = CspSourceKind.kw_unsafe_inline;
                return expr;
            }
            if (cspEqIgnoreCase(raw, "'unsafe-eval'")) {
                expr.kind = CspSourceKind.kw_unsafe_eval;
                return expr;
            }
            if (cspEqIgnoreCase(raw, "'strict-dynamic'")) {
                expr.kind = CspSourceKind.kw_strict_dynamic;
                return expr;
            }
            if (cspStartsWithIgnoreCase(raw, "'nonce-") && raw.len > 8) {
                if (!cspIsValidBase64Value(raw[7 .. raw.len - 1])) return null;
                expr.kind = CspSourceKind.nonce;
                return expr;
            }
            if (cspStartsWithIgnoreCase(raw, "'sha256-") && raw.len > 9) {
                if (!cspIsValidBase64Value(raw[8 .. raw.len - 1])) return null;
                expr.kind = CspSourceKind.hash_sha256;
                return expr;
            }
            if (cspStartsWithIgnoreCase(raw, "'sha384-") && raw.len > 9) {
                if (!cspIsValidBase64Value(raw[8 .. raw.len - 1])) return null;
                expr.kind = CspSourceKind.hash_sha384;
                return expr;
            }
            if (cspStartsWithIgnoreCase(raw, "'sha512-") && raw.len > 9) {
                if (!cspIsValidBase64Value(raw[8 .. raw.len - 1])) return null;
                expr.kind = CspSourceKind.hash_sha512;
                return expr;
            }
            return null;
        }

        if (cspBytesEqual(raw, "*")) {
            expr.kind = CspSourceKind.wildcard;
            return expr;
        }

        if (raw[raw.len - 1] == ':') {
            if (!cspIsValidSchemeName(raw[0 .. raw.len - 1])) return null;
            expr.kind = CspSourceKind.scheme;
            return expr;
        }

        expr.kind = CspSourceKind.host_expr;
        return expr;
    }
}

/// Single parsed CSP directive and its inline source list.
pub struct CspDirective {
    active: bool = false;
    kind: CspDirectiveKind = CspDirectiveKind.default_src;
    source_count: usize = 0;
    sources: [CSP_MAX_SOURCES_PER_DIR]CspSourceExpr = [CspSourceExpr{}; CSP_MAX_SOURCES_PER_DIR];

    /// Return true if this directive contains `'none'`.
    fn hasNone(): bool {
        let i: usize = 0;
        while (i < source_count) : (i += 1) {
            if (sources[i].kind == CspSourceKind.kw_none) return true;
        }
        return false;
    }

    /// W3C CSP3 §2.3.1: `'none'` only blocks all sources when it is the SOLE source expression.
    /// When coexisting with other sources (e.g. `'none' 'self'`), `'none'` is ignored.
    fn isEffectiveNone(): bool {
        return source_count == 1 && sources[0].kind == CspSourceKind.kw_none;
    }

    /// Return true if this directive contains `'strict-dynamic'`.
    fn hasStrictDynamic(): bool {
        let i: usize = 0;
        while (i < source_count) : (i += 1) {
            if (sources[i].kind == CspSourceKind.kw_strict_dynamic) return true;
        }
        return false;
    }

    /// Return true if this directive contains `'unsafe-inline'`.
    fn hasUnsafeInline(): bool {
        let i: usize = 0;
        while (i < source_count) : (i += 1) {
            if (sources[i].kind == CspSourceKind.kw_unsafe_inline) return true;
        }
        return false;
    }

    /// Return true if this directive contains any nonce or cryptographic hash source.
    fn hasNonceOrHash(): bool {
        let i: usize = 0;
        while (i < source_count) : (i += 1) {
            const k = sources[i].kind;
            if (k == CspSourceKind.nonce ||
                k == CspSourceKind.hash_sha256 ||
                k == CspSourceKind.hash_sha384 ||
                k == CspSourceKind.hash_sha512)
            {
                return true;
            }
        }
        return false;
    }
}

// ============================================================================
// 4. CspPolicy — W3C CSP Level 3 Parser, Evaluator & Header Serializer
// ============================================================================

/// Zero-heap-allocation W3C Content-Security-Policy Level 3 policy representation.
pub struct CspPolicy {
    directives: [CSP_MAX_DIRECTIVES]CspDirective = [CspDirective{}; CSP_MAX_DIRECTIVES];
    directive_count: usize = 0;
    upgrade_insecure: bool = false;

    /// Initialize an empty `CspPolicy`.
    static fn init(): CspPolicy {
        return CspPolicy{};
    }

    fn findDirectiveIndex(kind: CspDirectiveKind): usize? {
        let i: usize = 0;
        while (i < directive_count) : (i += 1) {
            if (directives[i].active && directives[i].kind == kind) return i;
        }
        return null;
    }

    /// Parse a `Content-Security-Policy` header string into a `CspPolicy`.
    ///   - Splits directives by `;`
    ///   - Follows W3C CSP first-directive-wins rule for duplicate directives
    ///   - Rejects CRLF / NUL control characters (`null`) to prevent header injection
    static fn parseHeader(header: []const u8): CspPolicy? {
        if (cspHasHeaderIllegalCtrl(header)) return null;

        let pol = CspPolicy.init();
        let seen_segments: usize = 0;
        let pos: usize = 0;
        while (pos < header.len) {
            const seg_start = pos;
            while (pos < header.len && header[pos] != ';') : (pos += 1) {}
            const seg = cspTrimSpaces(header[seg_start..pos]);
            if (pos < header.len && header[pos] == ';') pos += 1;
            if (seg.len == 0) continue;

            seen_segments += 1;
            if (seen_segments > 128) return null;

            // Extract directive name (first whitespace-delimited token)
            let p: usize = 0;
            while (p < seg.len && !cspIsAsciiSpace(seg[p])) : (p += 1) {}
            const dir_name: []const u8 = seg[0..p];
            if (!cspIsValidDirectiveName(dir_name)) return null;

            const kind = parseDirectiveKind(dir_name) ?? continue; // ignore unknown directives per CSP3

            // W3C CSP3 §2.2.1: Duplicate directives are ignored (first occurrence wins).
            if (pol.findDirectiveIndex(kind) != null) continue;

            if (pol.directive_count >= CSP_MAX_DIRECTIVES) return null;
            let d = CspDirective{};
            d.active = true;
            d.kind = kind;

            if (kind == CspDirectiveKind.upgrade_insecure_requests) {
                pol.upgrade_insecure = true;
                pol.directives[pol.directive_count] = d;
                pol.directive_count += 1;
                continue;
            }

            // Parse source tokens
            while (p < seg.len) {
                while (p < seg.len && cspIsAsciiSpace(seg[p])) : (p += 1) {}
                if (p >= seg.len) break;
                const tok_start = p;
                while (p < seg.len && !cspIsAsciiSpace(seg[p])) : (p += 1) {}
                const tok: []const u8 = seg[tok_start..p];

                const src = CspSourceExpr.parse(tok) ?? return null;
                if (d.source_count >= CSP_MAX_SOURCES_PER_DIR) return null;
                d.sources[d.source_count] = src;
                d.source_count += 1;
            }

            pol.directives[pol.directive_count] = d;
            pol.directive_count += 1;
        }

        return pol;
    }

    /// Add a source expression to `dir` (creating the directive if not yet present).
    /// Returns `false` if `raw_source` is invalid or inline directive/source capacity is exceeded.
    fn addSource(*_, dir: CspDirectiveKind, raw_source: []const u8): bool {
        if (dir == CspDirectiveKind.upgrade_insecure_requests) {
            setUpgradeInsecure(true);
            return true;
        }
        const src = CspSourceExpr.parse(raw_source) ?? return false;

        let d_idx: usize = 0;
        if (existing := findDirectiveIndex(dir)) {
            d_idx = existing;
        } else {
            if (directive_count >= CSP_MAX_DIRECTIVES) return false;
            d_idx = directive_count;
            directives[d_idx] = CspDirective{};
            directives[d_idx].active = true;
            directives[d_idx].kind = dir;
            directive_count += 1;
        }

        if (directives[d_idx].source_count >= CSP_MAX_SOURCES_PER_DIR) return false;
        directives[d_idx].sources[directives[d_idx].source_count] = src;
        directives[d_idx].source_count += 1;
        return true;
    }

    /// Enable or disable the `upgrade-insecure-requests` directive.
    fn setUpgradeInsecure(*_, enabled: bool) {
        upgrade_insecure = enabled;
        if (enabled) {
            if (findDirectiveIndex(CspDirectiveKind.upgrade_insecure_requests) == null && directive_count < CSP_MAX_DIRECTIVES) {
                directives[directive_count] = CspDirective{};
                directives[directive_count].active = true;
                directives[directive_count].kind = CspDirectiveKind.upgrade_insecure_requests;
                directive_count += 1;
            }
        } else {
            if (idx := findDirectiveIndex(CspDirectiveKind.upgrade_insecure_requests)) {
                let i: usize = idx;
                while (i + 1 < directive_count) : (i += 1) {
                    directives[i] = directives[i + 1];
                }
                if (directive_count > 0) {
                    directive_count -= 1;
                    directives[directive_count] = CspDirective{};
                }
            }
        }
    }

    /// Resolve the effective directive for `dir` following the W3C CSP3 fallback chain:
    ///   - Fetch directives (`script-src`, `style-src`, `img-src`, `connect-src`, `font-src`,
    ///     `frame-src`, `object-src`) fall back to `default-src` when omitted.
    ///   - `base-uri`, `form-action`, and `frame-ancestors` NEVER fall back to `default-src`.
    fn effectiveDirective(dir: CspDirectiveKind): CspDirective? {
        if (idx := findDirectiveIndex(dir)) {
            return directives[idx];
        }
        if (fallsBackToDefaultSrc(dir)) {
            if (def_idx := findDirectiveIndex(CspDirectiveKind.default_src)) {
                return directives[def_idx];
            }
        }
        return null;
    }

    /// Evaluate whether loading `target_url` under `dir` from `page_origin` is permitted.
    fn checkResourceUrl(dir: CspDirectiveKind, page_origin: []const u8, target_url: []const u8): CspEvalVerdict {
        const target = parseUrlParts(target_url) ?? return CspEvalVerdict.invalid_input;
        const eff = effectiveDirective(dir) ?? return CspEvalVerdict.allowed;

        if (eff.isEffectiveNone()) return CspEvalVerdict.blocked_by_none;

        let i: usize = 0;
        while (i < eff.source_count) : (i += 1) {
            const src = eff.sources[i];
            const tok: []const u8 = src.token_buf[0..@as(usize, src.token_len)];

            if (src.kind == CspSourceKind.kw_self) {
                if (origin := parseUrlParts(page_origin)) {
                    if (origin.has_authority &&
                        target.has_authority &&
                        cspEqIgnoreCase(origin.scheme, target.scheme) &&
                        cspEqIgnoreCase(origin.host, target.host) &&
                        origin.port == target.port)
                    {
                        return CspEvalVerdict.allowed;
                    }
                }
            } else if (src.kind == CspSourceKind.wildcard) {
                if (isNetworkScheme(target.scheme)) {
                    return CspEvalVerdict.allowed;
                }
            } else if (src.kind == CspSourceKind.scheme) {
                if (tok.len > 1 && cspEqIgnoreCase(target.scheme, tok[0 .. tok.len - 1])) {
                    return CspEvalVerdict.allowed;
                }
            } else if (src.kind == CspSourceKind.host_expr) {
                if (matchHostSourceExpr(tok, page_origin, target)) {
                    return CspEvalVerdict.allowed;
                }
            }
        }

        return CspEvalVerdict.blocked_no_match;
    }

    /// Evaluate whether an inline `<script>` with optional `nonce_val` and `sha256_b64` is allowed.
    /// Enforces W3C CSP3 strict rule: if the effective `script-src` directive contains any nonce,
    /// hash, or `'strict-dynamic'`, `'unsafe-inline'` is ignored!
    fn checkInlineScript(nonce_val: []const u8, sha256_b64: []const u8): CspEvalVerdict {
        if (cspHasControlOrCrlfChar(nonce_val) || cspHasControlOrCrlfChar(sha256_b64)) {
            return CspEvalVerdict.invalid_input;
        }
        const eff = effectiveDirective(CspDirectiveKind.script_src) ?? return CspEvalVerdict.allowed;
        if (eff.isEffectiveNone()) return CspEvalVerdict.blocked_by_none;

        // 1. Check matching nonce (`'nonce-<nonce_val>'`)
        if (nonce_val.len > 0) {
            let i: usize = 0;
            while (i < eff.source_count) : (i += 1) {
                if (eff.sources[i].kind == CspSourceKind.nonce) {
                    const tok: []const u8 = eff.sources[i].token_buf[0..@as(usize, eff.sources[i].token_len)];
                    if (tok.len > 8) {
                        const expected_nonce: []const u8 = tok[7 .. tok.len - 1];
                        if (cspBytesEqual(expected_nonce, nonce_val)) {
                            return CspEvalVerdict.allowed;
                        }
                    }
                }
            }
        }

        // 2. Check matching SHA-256 hash (`'sha256-<sha256_b64>'`)
        if (sha256_b64.len > 0) {
            let i: usize = 0;
            while (i < eff.source_count) : (i += 1) {
                if (eff.sources[i].kind == CspSourceKind.hash_sha256) {
                    const tok: []const u8 = eff.sources[i].token_buf[0..@as(usize, eff.sources[i].token_len)];
                    if (tok.len > 9) {
                        const expected_hash: []const u8 = tok[8 .. tok.len - 1];
                        if (cspBytesEqual(expected_hash, sha256_b64)) {
                            return CspEvalVerdict.allowed;
                        }
                    }
                }
            }
        }

        // 3. CSP3 rule: `'unsafe-inline'` only applies when NO nonce, hash, or `'strict-dynamic'` is present.
        if (eff.hasUnsafeInline() && !eff.hasNonceOrHash() && !eff.hasStrictDynamic()) {
            return CspEvalVerdict.allowed;
        }

        return CspEvalVerdict.blocked_unsafe_inline;
    }

    /// Serialize this `CspPolicy` into a canonical `Content-Security-Policy` header string.
    /// Returns bytes written, or `null` if `out_buf` is too small.
    fn formatHeader(out_buf: *[]u8): usize? {
        let pos: usize = 0;
        let written_dirs: usize = 0;

        let i: usize = 0;
        while (i < directive_count) : (i += 1) {
            if (!directives[i].active) continue;
            if (written_dirs > 0) {
                pos = cspAppendBytes(out_buf, pos, "; ") ?? return null;
            }
            pos = cspAppendBytes(out_buf, pos, formatDirectiveName(directives[i].kind)) ?? return null;

            let s: usize = 0;
            while (s < directives[i].source_count) : (s += 1) {
                pos = cspAppendBytes(out_buf, pos, " ") ?? return null;
                const tok: []const u8 = directives[i].sources[s].token_buf[0..@as(usize, directives[i].sources[s].token_len)];
                pos = cspAppendBytes(out_buf, pos, tok) ?? return null;
            }
            written_dirs += 1;
        }

        return pos;
    }
}
