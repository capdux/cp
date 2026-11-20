// Source: cp/lib/security/cors.cp (Fetch CORS Policy Engine)
// lib/security/cors.cp
// Fetch-Compliant CORS Policy Evaluator with Credentials/Wildcard Guard & Safe Subdomain Matching

const std = @import("std");

pub struct CorsHeader {
    name: []const u8 = "";
    value: []const u8 = "";
}

pub enum CorsDecisionKind {
    NotCors,
    SimpleAllowed,
    PreflightAllowed,
    Rejected,
}

pub enum CorsRejectReason {
    None,
    InvalidConfigCredentialsWithWildcard,
    UnsafeOrigin,
    OriginNotAllowed,
    MethodNotAllowed,
    HeaderNotAllowed,
    UnsafeHeaderInput,
}

pub struct CorsConfig {
    allowed_origins: []const []const u8 = [];
    allowed_methods: []const []const u8 = [];
    allowed_headers: []const []const u8 = [];
    exposed_headers: []const []const u8 = [];
    allow_credentials: bool = false;
    max_age_sec: u32 = 600;
    allow_any_origin: bool = false;

    static fn defaultStrict(): CorsConfig {
        return CorsConfig{
            allowed_origins: [],
            allowed_methods: [],
            allowed_headers: [],
            exposed_headers: [],
            allow_credentials: false,
            max_age_sec: 600,
            allow_any_origin: false,
        };
    }

    /// Rejects insecure `allow_credentials=true` combined with wildcard `*`.
    fn isValid(): bool {
        if (_.allow_credentials) {
            if (_.allow_any_origin) return false;
            for (orig of _.allowed_origins) {
                if (std.mem.eql(u8, orig, "*")) return false;
            }
        }
        return true;
    }
}

enum CorsHeaderSlotKind {
    Static,
    OriginEcho,
    MethodsBuf,
    HeadersBuf,
    ExposeBuf,
    MaxAgeBuf,
}

struct CorsHeaderSlot {
    name: []const u8 = "";
    static_val: []const u8 = "";
    kind: CorsHeaderSlotKind = .Static;
}

pub struct CorsDecision {
    allowed: bool = false;
    is_preflight: bool = false;
    kind: CorsDecisionKind = .NotCors;
    reject_reason: CorsRejectReason = .None;
    status_code: u16 = 200;

    origin_value: []const u8 = "";
    allow_credentials: bool = false;

    methods_buf: [256]u8 = undefined;
    methods_len: usize = 0;

    headers_buf: [512]u8 = undefined;
    headers_len: usize = 0;

    expose_buf: [512]u8 = undefined;
    expose_len: usize = 0;

    max_age_buf: [16]u8 = undefined;
    max_age_len: usize = 0;

    slots: [8]CorsHeaderSlot = undefined;
    slot_count: usize = 0;

    fn addStaticHeader(*_, name: []const u8, val: []const u8) {
        if (_.slot_count >= 8) return;
        _.slots[_.slot_count] = CorsHeaderSlot{
            name: name,
            static_val: val,
            kind: .Static,
        };
        _.slot_count += 1;
    }

    fn addSlotHeader(*_, name: []const u8, kind: CorsHeaderSlotKind) {
        if (_.slot_count >= 8) return;
        _.slots[_.slot_count] = CorsHeaderSlot{
            name: name,
            static_val: "",
            kind: kind,
        };
        _.slot_count += 1;
    }

    fn headerCount(): usize {
        return _.slot_count;
    }

    fn headerAt(idx: usize): CorsHeader? {
        if (idx >= _.slot_count) return null;
        const slot = _.slots[idx];
        const val: []const u8 = switch (slot.kind) {
            .Static => slot.static_val,
            .OriginEcho => _.origin_value,
            .MethodsBuf => _.methods_buf[0.._.methods_len],
            .HeadersBuf => _.headers_buf[0.._.headers_len],
            .ExposeBuf => _.expose_buf[0.._.expose_len],
            .MaxAgeBuf => _.max_age_buf[0.._.max_age_len],
        };
        return CorsHeader{
            name: slot.name,
            value: val,
        };
    }

    fn getHeader(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.slot_count) {
            if (std.ascii.eqlIgnoreCase(_.slots[i].name, name)) {
                const h = _.headerAt(i) ?? return null;
                return h.value;
            }
            i += 1;
        }
        return null;
    }

    fn writeHeaders(out: []CorsHeader): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.slot_count && written < out.len) {
            if (h := _.headerAt(i)) {
                out[written] = h;
                written += 1;
            }
            i += 1;
        }
        return written;
    }

    fn allowOrigin(): ([]const u8)? {
        return _.getHeader("Access-Control-Allow-Origin");
    }
}

fn hasUnsafeBytes(s: []const u8): bool {
    for (i of 0..s.len) {
        const c = s[i];
        if (c == '\r' || c == '\n' || c == 0) return true;
    }
    return false;
}

fn isValidHostChar(c: u8): bool {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= '0' && c <= '9') return true;
    return c == '-' || c == '.';
}

fn isSafeSubdomainPrefix(prefix: []const u8): bool {
    if (prefix.len == 0) return false;
    if (prefix[0] == '.' || prefix[0] == '-') return false;
    if (prefix[prefix.len - 1] == '.' || prefix[prefix.len - 1] == '-') return false;

    let prev_dot = false;
    for (i of 0..prefix.len) {
        const c = prefix[i];
        if (!isValidHostChar(c)) return false;
        if (c == '.') {
            if (prev_dot) return false;
            prev_dot = true;
        } else {
            prev_dot = false;
        }
    }
    return true;
}

struct ParsedOrigin {
    scheme: []const u8;
    host: []const u8;
    port: []const u8;
}

fn parseOrigin(origin: []const u8): ParsedOrigin? {
    if (origin.len == 0 || hasUnsafeBytes(origin)) return null;
    if (std.mem.eql(u8, origin, "null")) return null;

    const scheme_end = std.mem.indexOf(u8, origin, "://") ?? return null;
    if (scheme_end == 0) return null;
    const scheme = origin[0..scheme_end];
    if (!std.ascii.eqlIgnoreCase(scheme, "http") && !std.ascii.eqlIgnoreCase(scheme, "https")) {
        return null;
    }

    const authority = origin[scheme_end + 3 ..];
    if (authority.len == 0) return null;

    // Origin must not contain userinfo (@), path (/), query (?), fragment (#), wildcard (*), or whitespace
    for (i of 0..authority.len) {
        const c = authority[i];
        if (c == '@' || c == '/' || c == '?' || c == '#' || c == ' ' || c == '\t' || c == '*') {
            return null;
        }
    }

    // Check optional port (IPv6 bracketed host or host:port)
    if (authority[0] == '[') {
        const close_bracket = std.mem.indexOfScalar(u8, authority, ']') ?? return null;
        if (close_bracket + 1 == authority.len) {
            return ParsedOrigin{ scheme: scheme, host: authority, port: "" };
        }
        if (authority[close_bracket + 1] != ':') return null;
        const port_part = authority[close_bracket + 2 ..];
        if (!isNumericPort(port_part)) return null;
        return ParsedOrigin{
            scheme: scheme,
            host: authority[0 .. close_bracket + 1],
            port: port_part,
        };
    }

    if (colon := std.mem.indexOfScalar(u8, authority, ':')) {
        const host_part = authority[0..colon];
        const port_part = authority[colon + 1 ..];
        if (host_part.len == 0 || !isNumericPort(port_part)) return null;
        return ParsedOrigin{
            scheme: scheme,
            host: host_part,
            port: port_part,
        };
    }

    return ParsedOrigin{
        scheme: scheme,
        host: authority,
        port: "",
    };
}

fn isNumericPort(port: []const u8): bool {
    if (port.len == 0 || port.len > 5) return false;
    for (i of 0..port.len) {
        if (port[i] < '0' || port[i] > '9') return false;
    }
    return true;
}

fn isOriginMatched(origin: []const u8, parsed_origin: ParsedOrigin, cfg: CorsConfig): bool {
    if (cfg.allow_any_origin) return true;

    for (pattern of cfg.allowed_origins) {
        if (std.mem.eql(u8, pattern, "*")) {
            return true;
        }

        // Exact origin match (case-insensitive scheme/host, exact port)
        if (parsed_pat := parseOrigin(pattern)) {
            if (std.ascii.eqlIgnoreCase(parsed_origin.scheme, parsed_pat.scheme)
                && std.ascii.eqlIgnoreCase(parsed_origin.host, parsed_pat.host)
                && std.mem.eql(u8, parsed_origin.port, parsed_pat.port))
            {
                return true;
            }
            continue;
        }

        // Safe subdomain wildcard pattern: e.g. "https://*.example.com" or "https://*.example.com:8443"
        const scheme_end = std.mem.indexOf(u8, pattern, "://") ?? continue;
        const pat_scheme = pattern[0..scheme_end];
        if (!std.ascii.eqlIgnoreCase(parsed_origin.scheme, pat_scheme)) continue;

        const pat_auth = pattern[scheme_end + 3 ..];
        if (!std.mem.startsWith(u8, pat_auth, "*.")) continue;

        const after_star = pat_auth[2..];
        let pat_domain = after_star;
        let pat_port: []const u8 = "";
        if (colon := std.mem.indexOfScalar(u8, after_star, ':')) {
            pat_domain = after_star[0..colon];
            pat_port = after_star[colon + 1 ..];
            if (!isNumericPort(pat_port)) continue;
        }
        if (pat_domain.len == 0) continue;
        if (!std.mem.eql(u8, parsed_origin.port, pat_port)) continue;

        // Domain suffix must be ".<pat_domain>" so "evil-example.com" never matches "*.example.com"
        if (parsed_origin.host.len <= pat_domain.len + 1) continue;
        const split_idx = parsed_origin.host.len - pat_domain.len;
        const suffix_host = parsed_origin.host[split_idx..];
        if (!std.ascii.eqlIgnoreCase(suffix_host, pat_domain)) continue;
        if (parsed_origin.host[split_idx - 1] != '.') continue;

        const sub_prefix = parsed_origin.host[0 .. split_idx - 1];
        if (isSafeSubdomainPrefix(sub_prefix)) {
            return true;
        }
    }

    return false;
}

fn isSafelistedCorsHeader(h: []const u8): bool {
    return std.ascii.eqlIgnoreCase(h, "accept")
        || std.ascii.eqlIgnoreCase(h, "accept-language")
        || std.ascii.eqlIgnoreCase(h, "content-language")
        || std.ascii.eqlIgnoreCase(h, "content-type");
}

fn isMethodAllowed(method: []const u8, allowed_methods: []const []const u8): bool {
    if (allowed_methods.len == 0) return true;
    for (m of allowed_methods) {
        if (std.ascii.eqlIgnoreCase(m, method)) return true;
    }
    return false;
}

fn areHeadersAllowed(req_headers: []const []const u8, cfg: CorsConfig): bool {
    for (rh of req_headers) {
        if (rh.len == 0 || hasUnsafeBytes(rh)) return false;
        if (cfg.allowed_headers.len == 0) {
            if (!isSafelistedCorsHeader(rh)) return false;
            continue;
        }
        let matched = false;
        for (ah of cfg.allowed_headers) {
            if (std.mem.eql(u8, ah, "*") && !cfg.allow_credentials) {
                matched = true;
                break;
            }
            if (std.ascii.eqlIgnoreCase(ah, rh)) {
                matched = true;
                break;
            }
        }
        if (!matched) return false;
    }
    return true;
}

fn joinList(items: []const []const u8, out: []u8): usize {
    let pos: usize = 0;
    for (i of 0..items.len) {
        const item = items[i];
        if (i > 0) {
            if (pos + 2 > out.len) break;
            out[pos] = ',';
            out[pos + 1] = ' ';
            pos += 2;
        }
        const avail = out.len - pos;
        const copy_len = if (item.len <= avail) item.len else avail;
        if (copy_len > 0) {
            @memcpy(out[pos .. pos + copy_len], item[0..copy_len]);
            pos += copy_len;
        }
    }
    return pos;
}

pub struct CorsPolicy {
    config: CorsConfig;

    static fn init(config: CorsConfig): CorsPolicy {
        return CorsPolicy{ config: config };
    }

    fn evaluate(origin: ([]const u8)?, method: []const u8, req_headers: []const []const u8): CorsDecision {
        return _.evaluateInternal(origin, method, null, req_headers);
    }

    fn evaluatePreflight(origin: ([]const u8)?, request_method: []const u8, req_headers: []const []const u8): CorsDecision {
        return _.evaluateInternal(origin, "OPTIONS", request_method, req_headers);
    }

    fn evaluateInternal(
        origin_opt: ([]const u8)?,
        method: []const u8,
        preflight_req_method: ([]const u8)?,
        req_headers: []const []const u8,
    ): CorsDecision {
        let decision = CorsDecision{};

        // 1. Validate configuration safety invariant first
        if (!_.config.isValid()) {
            decision.allowed = false;
            decision.kind = .Rejected;
            decision.reject_reason = .InvalidConfigCredentialsWithWildcard;
            decision.status_code = 403;
            return decision;
        }

        // 2. Non-CORS request (missing or empty Origin header)
        const origin = origin_opt ?? "";
        if (origin.len == 0) {
            decision.allowed = true;
            decision.kind = .NotCors;
            decision.status_code = 200;
            return decision;
        }

        if (hasUnsafeBytes(method)) {
            decision.allowed = false;
            decision.kind = .Rejected;
            decision.reject_reason = .UnsafeHeaderInput;
            decision.status_code = 403;
            return decision;
        }

        // 3. Parse and validate Origin syntax
        const parsed_origin_opt = parseOrigin(origin);
        if (parsed_origin_opt == null) {
            decision.allowed = false;
            decision.kind = .Rejected;
            decision.reject_reason = .UnsafeOrigin;
            decision.status_code = 403;
            return decision;
        }
        const parsed_origin = parsed_origin_opt.?;

        // 4. Match Origin against policy
        if (!isOriginMatched(origin, parsed_origin, _.config)) {
            decision.allowed = false;
            decision.kind = .Rejected;
            decision.reject_reason = .OriginNotAllowed;
            decision.status_code = 403;
            return decision;
        }

        const is_options = std.ascii.eqlIgnoreCase(method, "OPTIONS");
        decision.is_preflight = is_options;

        // 5. Validate requested method & headers
        if (is_options) {
            if (req_m := preflight_req_method) {
                if (hasUnsafeBytes(req_m) || !isMethodAllowed(req_m, _.config.allowed_methods)) {
                    decision.allowed = false;
                    decision.kind = .Rejected;
                    decision.reject_reason = .MethodNotAllowed;
                    decision.status_code = 403;
                    return decision;
                }
            }
            if (!areHeadersAllowed(req_headers, _.config)) {
                decision.allowed = false;
                decision.kind = .Rejected;
                decision.reject_reason = .HeaderNotAllowed;
                decision.status_code = 403;
                return decision;
            }
        } else {
            if (!isMethodAllowed(method, _.config.allowed_methods)) {
                decision.allowed = false;
                decision.kind = .Rejected;
                decision.reject_reason = .MethodNotAllowed;
                decision.status_code = 403;
                return decision;
            }
        }

        // 6. Allowed — build CORS response headers
        decision.allowed = true;
        decision.allow_credentials = _.config.allow_credentials;

        const use_wildcard = _.config.allow_any_origin && !_.config.allow_credentials;
        if (use_wildcard) {
            decision.addStaticHeader("Access-Control-Allow-Origin", "*");
        } else {
            decision.origin_value = origin;
            decision.addSlotHeader("Access-Control-Allow-Origin", .OriginEcho);
        }

        if (_.config.allow_credentials) {
            decision.addStaticHeader("Access-Control-Allow-Credentials", "true");
        }

        if (is_options) {
            decision.kind = .PreflightAllowed;
            decision.status_code = 204;
            decision.addStaticHeader("Vary", "Origin, Access-Control-Request-Method, Access-Control-Request-Headers");

            if (_.config.allowed_methods.len > 0) {
                decision.methods_len = joinList(_.config.allowed_methods, &decision.methods_buf);
                decision.addSlotHeader("Access-Control-Allow-Methods", .MethodsBuf);
            } else {
                decision.addStaticHeader("Access-Control-Allow-Methods", "GET, HEAD, PUT, PATCH, POST, DELETE");
            }

            if (_.config.allowed_headers.len > 0) {
                decision.headers_len = joinList(_.config.allowed_headers, &decision.headers_buf);
                decision.addSlotHeader("Access-Control-Allow-Headers", .HeadersBuf);
            }

            if (_.config.max_age_sec > 0) {
                let v = _.config.max_age_sec;
                let tmp: [16]u8 = undefined;
                let n: usize = 0;
                while (v > 0) {
                    tmp[n] = ('0' as u8) + ((v % 10) as u8);
                    n += 1;
                    v /= 10;
                }
                let p: usize = 0;
                while (p < n && p < decision.max_age_buf.len) {
                    decision.max_age_buf[p] = tmp[n - 1 - p];
                    p += 1;
                }
                decision.max_age_len = p;
                if (decision.max_age_len > 0) {
                    decision.addSlotHeader("Access-Control-Max-Age", .MaxAgeBuf);
                }
            }
        } else {
            decision.kind = .SimpleAllowed;
            decision.status_code = 200;
            if (!use_wildcard) {
                decision.addStaticHeader("Vary", "Origin");
            }
            if (_.config.exposed_headers.len > 0) {
                decision.expose_len = joinList(_.config.exposed_headers, &decision.expose_buf);
                decision.addSlotHeader("Access-Control-Expose-Headers", .ExposeBuf);
            }
        }

        return decision;
    }
}
