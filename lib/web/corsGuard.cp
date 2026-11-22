// Origin: Pure-CP Stdlib (Zero-Alloc W3C Fetch CORS Guard, Safe Subdomain Matcher & Header Synthesizer) | Created: 2026-11-22
// Pure CP zero-heap-allocation W3C Fetch CORS policy evaluator and HTTP header synthesizer:
//   - Supports exact Origins, safe wildcard subdomains (`https://*.example.com` enforcing scheme
//     and dot-boundary checks to prevent `evil-example.com` bypasses), and `*` wildcard
//   - Enforces W3C Fetch security invariant: when `allow_credentials == true`, NEVER reflects `*`;
//     always echoes the exact matched Origin and emits `Vary: Origin` (plus preflight Vary tokens)
//   - Distinguishes OPTIONS preflight (`evaluatePreflight`) vs actual requests (`evaluateActualRequest`)
//   - Synthesizes wire-ready HTTP response headers (`formatCorsResponseHeaders`) with full CRLF/control-char guards

// ============================================================================
// Constants & Internal String/Validation Helpers
// ============================================================================

pub const CORS_MAX_ORIGINS: usize = 32;
pub const CORS_MAX_HEADERS: usize = 32;
pub const CORS_MAX_ORIGIN_LEN: usize = 128;
pub const CORS_MAX_HEADER_NAME_LEN: usize = 64;
pub const CORS_HEADER_LIST_BUF_LEN: usize = 256;

pub const CORS_METHOD_GET: u16 = 1 << 0;
pub const CORS_METHOD_HEAD: u16 = 1 << 1;
pub const CORS_METHOD_POST: u16 = 1 << 2;
pub const CORS_METHOD_PUT: u16 = 1 << 3;
pub const CORS_METHOD_DELETE: u16 = 1 << 4;
pub const CORS_METHOD_PATCH: u16 = 1 << 5;
pub const CORS_METHOD_OPTIONS: u16 = 1 << 6;
pub const CORS_METHOD_ALL_STANDARD: u16 = 0x7F;

fn toLowerAscii(c: u8): u8 {
    if (c >= 65 && c <= 90) {
        return c + 32;
    }
    return c;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn bytesEqualIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn trimAsciiSpace(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == 32 || s[start] == 9)) : (start += 1) {}
    while (end > start && (s[end - 1] == 32 || s[end - 1] == 9)) : (end -= 1) {}
    return s[start..end];
}

/// Returns `true` if `s` contains CR, LF, NUL, or any ASCII control byte (< 0x20 or 0x7F).
pub fn hasControlOrCrlf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c < 0x20 || c == 0x7F) {
            return true;
        }
    }
    return false;
}

fn findSchemeSep(s: []const u8): usize? {
    if (s.len < 4) return null;
    let i: usize = 0;
    while (i + 2 < s.len) : (i += 1) {
        if (s[i] == 58 && s[i + 1] == 47 && s[i + 2] == 47) { // "://"
            return i;
        }
    }
    return null;
}

/// Validates RFC 6454 serialized Origin syntax (`scheme://host[:port]`), rejecting CRLF, spaces,
/// path slashes, query/fragment delimiters, and overlong values.
pub fn isValidOriginSyntax(origin: []const u8): bool {
    if (origin.len == 0 || origin.len > CORS_MAX_ORIGIN_LEN || hasControlOrCrlf(origin)) {
        return false;
    }
    const sep = findSchemeSep(origin) ?? return false;
    if (sep == 0) {
        return false;
    }
    // Validate scheme characters: ALPHA / DIGIT / '+' / '-' / '.'
    let i: usize = 0;
    while (i < sep) : (i += 1) {
        const c: u8 = origin[i];
        const is_alpha: bool = (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
        const is_digit: bool = c >= 48 && c <= 57;
        if (!is_alpha && !is_digit && c != 43 && c != 45 && c != 46) {
            return false;
        }
    }
    const host_start: usize = sep + 3;
    if (host_start >= origin.len) {
        return false;
    }
    // Host[:port] must not contain '/', '?', '#', '@', whitespace, or '*'
    i = host_start;
    while (i < origin.len) : (i += 1) {
        const c: u8 = origin[i];
        if (c == 47 || c == 63 || c == 35 || c == 64 || c == 32 || c == 42) {
            return false;
        }
    }
    return true;
}

fn isValidOriginPattern(pattern: []const u8): bool {
    if (pattern.len == 0 || pattern.len > CORS_MAX_ORIGIN_LEN || hasControlOrCrlf(pattern)) {
        return false;
    }
    if (bytesEqual(pattern, "*")) {
        return true;
    }
    const sep = findSchemeSep(pattern) ?? return false;
    const host_start: usize = sep + 3;
    if (host_start >= pattern.len) {
        return false;
    }
    // Wildcard subdomain pattern: `scheme://*.suffix`
    if (host_start + 2 < pattern.len && pattern[host_start] == 42 && pattern[host_start + 1] == 46) {
        // Ensure no additional '*' or forbidden chars in suffix and suffix is non-empty
        let i: usize = host_start + 2;
        if (i >= pattern.len || pattern[i] == 46) {
            return false;
        }
        while (i < pattern.len) : (i += 1) {
            const c: u8 = pattern[i];
            if (c == 42 || c == 47 || c == 63 || c == 35 || c == 64 || c == 32) {
                return false;
            }
        }
        return true;
    }
    return isValidOriginSyntax(pattern);
}

/// Safely matches `origin` against a validated `pattern` (exact, `scheme://*.domain`, or `*`).
/// Enforces identical scheme and strict dot boundary so `https://evil-example.com` NEVER matches
/// `https://*.example.com`.
pub fn matchOriginPattern(pattern: []const u8, origin: []const u8): bool {
    if (!isValidOriginSyntax(origin)) {
        return false;
    }
    if (bytesEqual(pattern, "*")) {
        return true;
    }
    const p_sep = findSchemeSep(pattern) ?? return false;
    const o_sep = findSchemeSep(origin) ?? return false;
    if (p_sep != o_sep || !bytesEqualIgnoreCase(pattern[0..p_sep], origin[0..o_sep])) {
        return false;
    }

    const host_start: usize = p_sep + 3;
    // Check wildcard subdomain `scheme://*.suffix`
    if (host_start + 2 < pattern.len && pattern[host_start] == 42 && pattern[host_start + 1] == 46) {
        const dot_suffix: []const u8 = pattern[(host_start + 1)..]; // includes leading '.'
        const o_host: []const u8 = origin[host_start..];
        if (o_host.len <= dot_suffix.len) {
            return false; // must have non-empty subdomain label before '.'
        }
        const sub_len: usize = o_host.len - dot_suffix.len;
        const sub_part: []const u8 = o_host[0..sub_len];
        const tail_part: []const u8 = o_host[sub_len..];
        // Subdomain must not start/end with '.' or contain ':' (port belongs in suffix)
        if (sub_part[0] == 46 || sub_part[sub_len - 1] == 46) {
            return false;
        }
        let i: usize = 0;
        while (i < sub_len) : (i += 1) {
            if (sub_part[i] == 58) {
                return false;
            }
        }
        return bytesEqualIgnoreCase(tail_part, dot_suffix);
    }

    return bytesEqualIgnoreCase(pattern[host_start..], origin[host_start..]);
}

/// Parses an HTTP method token into its `CORS_METHOD_*` bitmask (returns `0` if unrecognized).
pub fn parseCorsMethodMask(method: []const u8): u16 {
    if (bytesEqual(method, "GET")) return CORS_METHOD_GET;
    if (bytesEqual(method, "HEAD")) return CORS_METHOD_HEAD;
    if (bytesEqual(method, "POST")) return CORS_METHOD_POST;
    if (bytesEqual(method, "PUT")) return CORS_METHOD_PUT;
    if (bytesEqual(method, "DELETE")) return CORS_METHOD_DELETE;
    if (bytesEqual(method, "PATCH")) return CORS_METHOD_PATCH;
    if (bytesEqual(method, "OPTIONS")) return CORS_METHOD_OPTIONS;
    return 0;
}

fn isValidHeaderToken(name: []const u8): bool {
    if (name.len == 0 || name.len > CORS_MAX_HEADER_NAME_LEN || hasControlOrCrlf(name)) {
        return false;
    }
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        const c: u8 = name[i];
        if (c <= 0x20 || c == 44 || c == 58) { // disallow spaces, commas, colons
            return false;
        }
    }
    return true;
}

fn copyFixed(dst: *[]u8, src: []const u8): usize {
    const n: usize = if (src.len < dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

fn appendBytes(dst: *[]u8, pos: usize, src: []const u8): usize? {
    if (pos + src.len > dst.len) return null;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[pos + i] = src[i];
    }
    return pos + src.len;
}

fn appendU32Dec(dst: *[]u8, pos: usize, val_in: u32): usize? {
    let tmp: [10]u8 = [0; 10];
    let len: usize = 0;
    let v: u32 = val_in;
    if (v == 0) {
        tmp[0] = 48;
        len = 1;
    } else {
        while (v > 0) : (v /= 10) {
            tmp[len] = (48 + (v % 10)) as u8;
            len += 1;
        }
    }
    if (pos + len > dst.len) return null;
    let i: usize = 0;
    while (i < len) : (i += 1) {
        dst[pos + i] = tmp[len - 1 - i];
    }
    return pos + len;
}

// ============================================================================
// 1. CORS Decision & Policy Engine Models
// ============================================================================

/// Verdict produced by `CorsGuard.evaluatePreflight` or `CorsGuard.evaluateActualRequest`.
pub enum CorsDecisionKind {
    preflight_allow_204,
    preflight_reject_origin,
    preflight_reject_method,
    preflight_reject_header,
    actual_allow,
    actual_reject_origin,
    not_cors_request,
}

/// Self-contained CORS evaluation decision ready for HTTP response header synthesis.
pub struct CorsDecision {
    kind: CorsDecisionKind = CorsDecisionKind.not_cors_request;
    http_status: u16 = 200;
    allow_origin_buf: [CORS_MAX_ORIGIN_LEN]u8 = [0; CORS_MAX_ORIGIN_LEN];
    allow_origin_len: usize = 0;
    allow_credentials: bool = false;
    vary_origin: bool = false;
    vary_preflight: bool = false;
    allowed_methods_mask: u16 = 0;
    allow_headers_buf: [CORS_HEADER_LIST_BUF_LEN]u8 = [0; CORS_HEADER_LIST_BUF_LEN];
    allow_headers_len: usize = 0;
    expose_headers_buf: [CORS_HEADER_LIST_BUF_LEN]u8 = [0; CORS_HEADER_LIST_BUF_LEN];
    expose_headers_len: usize = 0;
    max_age_secs: u32 = 0;

    fn isAllowed(): bool {
        return _.kind == CorsDecisionKind.preflight_allow_204
            || _.kind == CorsDecisionKind.actual_allow
            || _.kind == CorsDecisionKind.not_cors_request;
    }

    fn allowOriginSlice(): []const u8 {
        return _.allow_origin_buf[0.._.allow_origin_len];
    }

    fn allowHeadersSlice(): []const u8 {
        return _.allow_headers_buf[0.._.allow_headers_len];
    }

    fn exposeHeadersSlice(): []const u8 {
        return _.expose_headers_buf[0.._.expose_headers_len];
    }
}

/// Zero-heap-allocation W3C Fetch CORS Guard & Policy Evaluator.
pub struct CorsGuard {
    origins: [CORS_MAX_ORIGINS][CORS_MAX_ORIGIN_LEN]u8 = [[0; CORS_MAX_ORIGIN_LEN]; CORS_MAX_ORIGINS];
    origin_lens: [CORS_MAX_ORIGINS]usize = [0; CORS_MAX_ORIGINS];
    origin_count: usize = 0;
    has_wildcard_origin: bool = false;

    allowed_methods_mask: u16 = CORS_METHOD_GET | CORS_METHOD_HEAD | CORS_METHOD_POST;

    allowed_headers: [CORS_MAX_HEADERS][CORS_MAX_HEADER_NAME_LEN]u8 = [[0; CORS_MAX_HEADER_NAME_LEN]; CORS_MAX_HEADERS];
    allowed_header_lens: [CORS_MAX_HEADERS]usize = [0; CORS_MAX_HEADERS];
    allowed_header_count: usize = 0;
    allow_any_header: bool = false;

    exposed_headers: [CORS_MAX_HEADERS][CORS_MAX_HEADER_NAME_LEN]u8 = [[0; CORS_MAX_HEADER_NAME_LEN]; CORS_MAX_HEADERS];
    exposed_header_lens: [CORS_MAX_HEADERS]usize = [0; CORS_MAX_HEADERS];
    exposed_header_count: usize = 0;

    allow_credentials: bool = false;
    max_age_secs: u32 = 0;

    fn init(): CorsGuard {
        return CorsGuard {};
    }

    /// Registers an allowed Origin pattern (`*`, exact `https://app.example.com`, or safe
    /// wildcard subdomain `https://*.example.com`). Rejects malformed patterns, CRLF, or full table.
    fn addAllowedOrigin(*_, pattern: []const u8): bool {
        if (!isValidOriginPattern(pattern) || _.origin_count >= CORS_MAX_ORIGINS) {
            return false;
        }
        if (bytesEqual(pattern, "*")) {
            _.has_wildcard_origin = true;
        }
        let dst: []u8 = _.origins[_.origin_count][0..];
        _.origin_lens[_.origin_count] = copyFixed(*dst, pattern);
        _.origin_count += 1;
        return true;
    }

    fn setAllowedMethods(*_, methods_mask: u16) {
        _.allowed_methods_mask = methods_mask;
    }

    /// Registers an allowed request header name (or `"*"` for any header).
    fn addAllowedHeader(*_, header_name: []const u8): bool {
        if (bytesEqual(header_name, "*")) {
            _.allow_any_header = true;
            return true;
        }
        if (!isValidHeaderToken(header_name) || _.allowed_header_count >= CORS_MAX_HEADERS) {
            return false;
        }
        let dst: []u8 = _.allowed_headers[_.allowed_header_count][0..];
        _.allowed_header_lens[_.allowed_header_count] = copyFixed(*dst, header_name);
        _.allowed_header_count += 1;
        return true;
    }

    /// Registers a response header to expose via `Access-Control-Expose-Headers`.
    fn addExposedHeader(*_, header_name: []const u8): bool {
        if (!isValidHeaderToken(header_name) || _.exposed_header_count >= CORS_MAX_HEADERS) {
            return false;
        }
        let dst: []u8 = _.exposed_headers[_.exposed_header_count][0..];
        _.exposed_header_lens[_.exposed_header_count] = copyFixed(*dst, header_name);
        _.exposed_header_count += 1;
        return true;
    }

    fn setAllowCredentials(*_, enabled: bool) {
        _.allow_credentials = enabled;
    }

    fn setMaxAgeSecs(*_, secs: u32) {
        _.max_age_secs = secs;
    }

    fn isOriginMatched(origin: []const u8): bool {
        if (!isValidOriginSyntax(origin)) {
            return false;
        }
        let i: usize = 0;
        while (i < _.origin_count) : (i += 1) {
            const pat: []const u8 = _.origins[i][0.._.origin_lens[i]];
            if (matchOriginPattern(pat, origin)) {
                return true;
            }
        }
        return false;
    }

    fn isSingleHeaderAllowed(name: []const u8): bool {
        if (_.allow_any_header) {
            return true;
        }
        let i: usize = 0;
        while (i < _.allowed_header_count) : (i += 1) {
            const h: []const u8 = _.allowed_headers[i][0.._.allowed_header_lens[i]];
            if (bytesEqualIgnoreCase(h, name)) {
                return true;
            }
        }
        return false;
    }

    /// Populates `allow_origin_buf` and `vary_origin` strictly enforcing W3C Fetch credential invariant.
    fn populateOriginHeader(origin: []const u8, decision: *CorsDecision) {
        // W3C Fetch invariant: when allow_credentials == true, NEVER emit "*"; echo exact Origin + Vary: Origin.
        if (_.has_wildcard_origin && !_.allow_credentials && _.origin_count == 1) {
            let dst: []u8 = decision.allow_origin_buf[0..];
            decision.allow_origin_len = copyFixed(*dst, "*");
            decision.vary_origin = false;
        } else {
            let dst: []u8 = decision.allow_origin_buf[0..];
            decision.allow_origin_len = copyFixed(*dst, origin);
            decision.vary_origin = true;
        }
        decision.allow_credentials = _.allow_credentials;
    }

    /// Evaluates an HTTP `OPTIONS` CORS preflight request.
    fn evaluatePreflight(
        origin: []const u8,
        req_method: []const u8,
        req_headers_csv: []const u8,
    ): CorsDecision {
        if (!_.isOriginMatched(origin)) {
            return CorsDecision {
                kind: CorsDecisionKind.preflight_reject_origin,
                http_status: 403,
                vary_origin: true,
                vary_preflight: true,
            };
        }

        const method_bit: u16 = parseCorsMethodMask(req_method);
        if (method_bit == 0 || (_.allowed_methods_mask & method_bit) == 0) {
            return CorsDecision {
                kind: CorsDecisionKind.preflight_reject_method,
                http_status: 403,
                vary_origin: true,
                vary_preflight: true,
            };
        }

        if (hasControlOrCrlf(req_headers_csv) || req_headers_csv.len > CORS_HEADER_LIST_BUF_LEN) {
            return CorsDecision {
                kind: CorsDecisionKind.preflight_reject_header,
                http_status: 403,
                vary_origin: true,
                vary_preflight: true,
            };
        }

        // Validate every comma-separated requested header token.
        let decision = CorsDecision {
            kind: CorsDecisionKind.preflight_allow_204,
            http_status: 204,
            vary_preflight: true,
            allowed_methods_mask: _.allowed_methods_mask,
            max_age_secs: _.max_age_secs,
        };

        let hdr_out_pos: usize = 0;
        if (req_headers_csv.len > 0) {
            let start: usize = 0;
            let pos: usize = 0;
            while (pos <= req_headers_csv.len) : (pos += 1) {
                const at_end: bool = pos == req_headers_csv.len;
                if (at_end || req_headers_csv[pos] == 44) { // ','
                    const tok: []const u8 = trimAsciiSpace(req_headers_csv[start..pos]);
                    if (tok.len > 0) {
                        if (!isValidHeaderToken(tok) || !_.isSingleHeaderAllowed(tok)) {
                            return CorsDecision {
                                kind: CorsDecisionKind.preflight_reject_header,
                                http_status: 403,
                                vary_origin: true,
                                vary_preflight: true,
                            };
                        }
                        if (hdr_out_pos > 0) {
                            let h_s: []u8 = decision.allow_headers_buf[0..];
                            hdr_out_pos = appendBytes(*h_s, hdr_out_pos, ", ") ?? return CorsDecision {
                                kind: CorsDecisionKind.preflight_reject_header,
                                http_status: 403,
                                vary_origin: true,
                                vary_preflight: true,
                            };
                        }
                        let h_s2: []u8 = decision.allow_headers_buf[0..];
                        hdr_out_pos = appendBytes(*h_s2, hdr_out_pos, tok) ?? return CorsDecision {
                            kind: CorsDecisionKind.preflight_reject_header,
                            http_status: 403,
                            vary_origin: true,
                            vary_preflight: true,
                        };
                    }
                    start = pos + 1;
                }
            }
        }
        decision.allow_headers_len = hdr_out_pos;
        _.populateOriginHeader(origin, *decision);
        return decision;
    }

    /// Evaluates a non-preflight actual HTTP request carrying an optional `Origin` header.
    fn evaluateActualRequest(origin: []const u8, method: []const u8): CorsDecision {
        if (origin.len == 0) {
            return CorsDecision {
                kind: CorsDecisionKind.not_cors_request,
                http_status: 200,
            };
        }
        if (!_.isOriginMatched(origin)) {
            return CorsDecision {
                kind: CorsDecisionKind.actual_reject_origin,
                http_status: 403,
                vary_origin: true,
            };
        }
        const m_bit: u16 = parseCorsMethodMask(method);
        if (m_bit == 0 || (_.allowed_methods_mask & m_bit) == 0) {
            return CorsDecision {
                kind: CorsDecisionKind.actual_reject_origin,
                http_status: 403,
                vary_origin: true,
            };
        }

        let decision = CorsDecision {
            kind: CorsDecisionKind.actual_allow,
            http_status: 200,
        };
        _.populateOriginHeader(origin, *decision);

        // Copy configured Access-Control-Expose-Headers into decision.
        let exp_pos: usize = 0;
        let i: usize = 0;
        while (i < _.exposed_header_count) : (i += 1) {
            const h: []const u8 = _.exposed_headers[i][0.._.exposed_header_lens[i]];
            let e_s: []u8 = decision.expose_headers_buf[0..];
            if (exp_pos > 0) {
                if (next_p := appendBytes(*e_s, exp_pos, ", ")) {
                    exp_pos = next_p;
                } else {
                    break;
                }
            }
            if (next_p2 := appendBytes(*e_s, exp_pos, h)) {
                exp_pos = next_p2;
            } else {
                break;
            }
        }
        decision.expose_headers_len = exp_pos;
        return decision;
    }
}

// ============================================================================
// 2. Wire-Ready HTTP CORS Response Header Synthesizer
// ============================================================================

fn appendMethodsList(dst: *[]u8, pos_in: usize, mask: u16): usize? {
    const names: [7][]const u8 = ["GET", "HEAD", "POST", "PUT", "DELETE", "PATCH", "OPTIONS"];
    const bits: [7]u16 = [
        CORS_METHOD_GET,
        CORS_METHOD_HEAD,
        CORS_METHOD_POST,
        CORS_METHOD_PUT,
        CORS_METHOD_DELETE,
        CORS_METHOD_PATCH,
        CORS_METHOD_OPTIONS,
    ];
    let pos: usize = pos_in;
    let first: bool = true;
    let i: usize = 0;
    while (i < 7) : (i += 1) {
        if ((mask & bits[i]) != 0) {
            if (!first) {
                pos = appendBytes(dst, pos, ", ") ?? return null;
            }
            pos = appendBytes(dst, pos, names[i]) ?? return null;
            first = false;
        }
    }
    return pos;
}

/// Synthesizes standard CRLF-delimited HTTP CORS response headers for `decision` into `out_buf`.
/// Returns total bytes written, or `null` if `out_buf` is too small.
pub fn formatCorsResponseHeaders(decision: CorsDecision, out_buf: *[]u8): usize? {
    let pos: usize = 0;

    if (decision.allow_origin_len > 0) {
        pos = appendBytes(out_buf, pos, "Access-Control-Allow-Origin: ") ?? return null;
        pos = appendBytes(out_buf, pos, decision.allowOriginSlice()) ?? return null;
        pos = appendBytes(out_buf, pos, "\r\n") ?? return null;
    }

    if (decision.allow_credentials && decision.allow_origin_len > 0) {
        pos = appendBytes(out_buf, pos, "Access-Control-Allow-Credentials: true\r\n") ?? return null;
    }

    if (decision.kind == CorsDecisionKind.preflight_allow_204) {
        if (decision.allowed_methods_mask != 0) {
            pos = appendBytes(out_buf, pos, "Access-Control-Allow-Methods: ") ?? return null;
            pos = appendMethodsList(out_buf, pos, decision.allowed_methods_mask) ?? return null;
            pos = appendBytes(out_buf, pos, "\r\n") ?? return null;
        }
        if (decision.allow_headers_len > 0) {
            pos = appendBytes(out_buf, pos, "Access-Control-Allow-Headers: ") ?? return null;
            pos = appendBytes(out_buf, pos, decision.allowHeadersSlice()) ?? return null;
            pos = appendBytes(out_buf, pos, "\r\n") ?? return null;
        }
        if (decision.max_age_secs > 0) {
            pos = appendBytes(out_buf, pos, "Access-Control-Max-Age: ") ?? return null;
            pos = appendU32Dec(out_buf, pos, decision.max_age_secs) ?? return null;
            pos = appendBytes(out_buf, pos, "\r\n") ?? return null;
        }
    } else if (decision.kind == CorsDecisionKind.actual_allow && decision.expose_headers_len > 0) {
        pos = appendBytes(out_buf, pos, "Access-Control-Expose-Headers: ") ?? return null;
        pos = appendBytes(out_buf, pos, decision.exposeHeadersSlice()) ?? return null;
        pos = appendBytes(out_buf, pos, "\r\n") ?? return null;
    }

    if (decision.vary_origin || decision.vary_preflight) {
        pos = appendBytes(out_buf, pos, "Vary: ") ?? return null;
        let wrote_vary: bool = false;
        if (decision.vary_origin) {
            pos = appendBytes(out_buf, pos, "Origin") ?? return null;
            wrote_vary = true;
        }
        if (decision.vary_preflight) {
            if (wrote_vary) {
                pos = appendBytes(out_buf, pos, ", ") ?? return null;
            }
            pos = appendBytes(out_buf, pos, "Access-Control-Request-Method, Access-Control-Request-Headers") ?? return null;
        }
        pos = appendBytes(out_buf, pos, "\r\n") ?? return null;
    }

    return pos;
}
