// @cap/cors-headers — Zero-heap Fetch/W3C CORS policy evaluator & response header builder.
// Clean-room MIT port of npm `cors`; reuses `@cap/unicase` and `@cap/wildmatch`.

import { eqAscii } from "../../unicase/src/index.cp";
import { wildmatchIgnoreCase } from "../../wildmatch/src/index.cp";

pub const CORS_MAX_ORIGINS: usize = 16;

pub struct CorsOptions {
    pub allow_any_origin: bool,
    pub reflect_origin: bool,
    pub allowed_origins: [16]string,
    pub allowed_origin_count: usize,
    pub allowed_methods: string,
    pub allowed_headers: string,
    pub exposed_headers: string,
    pub allow_credentials: bool,
    pub max_age: u32,
    pub has_max_age: bool,

    pub static fn permissive(): CorsOptions {
        let arr: [16]string = undefined;
        let i: usize = 0;
        while (i < 16) { arr[i] = ""; i = i + 1; }
        return CorsOptions {
            allow_any_origin: true,
            reflect_origin: false,
            allowed_origins: arr,
            allowed_origin_count: 0,
            allowed_methods: "GET,HEAD,PUT,PATCH,POST,DELETE",
            allowed_headers: "",
            exposed_headers: "",
            allow_credentials: false,
            max_age: 0,
            has_max_age: false,
        };
    }

    pub static fn restrictive(): CorsOptions {
        let arr: [16]string = undefined;
        let i: usize = 0;
        while (i < 16) { arr[i] = ""; i = i + 1; }
        return CorsOptions {
            allow_any_origin: false,
            reflect_origin: false,
            allowed_origins: arr,
            allowed_origin_count: 0,
            allowed_methods: "GET,HEAD,POST",
            allowed_headers: "",
            exposed_headers: "",
            allow_credentials: false,
            max_age: 0,
            has_max_age: false,
        };
    }

    pub fn addOrigin(*_, pattern: string): bool {
        if (_.allowed_origin_count >= CORS_MAX_ORIGINS) { return false; }
        _.allowed_origins[_.allowed_origin_count] = pattern;
        _.allowed_origin_count = _.allowed_origin_count + 1;
        return true;
    }
}

pub struct CorsDecision {
    pub allowed: bool,
    pub is_preflight: bool,
    pub allow_origin: string,
    pub vary_origin: bool,
    pub allow_credentials: bool,
    pub allow_methods: string,
    pub allow_headers: string,
    pub expose_headers: string,
    pub max_age: u32,
    pub has_max_age: bool,
}

fn corsPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn corsPushU32(out: []u8, pos: usize, v: u32): usize {
    let p = pos;
    let tmp: [12]u8 = undefined;
    let len: usize = 0;
    if (v == 0) {
        tmp[0] = 48;
        len = 1;
    } else {
        let cur = v;
        while (cur > 0 && len < 12) {
            tmp[len] = (48 + (cur % 10)) as u8;
            cur = cur / 10;
            len = len + 1;
        }
    }
    let k = len;
    while (k > 0 && p < out.len) {
        k = k - 1;
        out[p] = tmp[k];
        p = p + 1;
    }
    return p;
}

fn corsIsWs(b: u8): bool {
    return b == 32 || b == 9;
}

fn corsTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && corsIsWs(s[start])) { start = start + 1; }
    let end: usize = s.len;
    while (end > start && corsIsWs(s[end - 1])) { end = end - 1; }
    return s[start..end];
}

/// Checks whether `request_origin` matches the CORS policy (supports exact & wildcard patterns).
pub fn isOriginAllowed(opts: CorsOptions, request_origin: string): bool {
    if (request_origin.len == 0) { return false; }
    if (opts.allow_any_origin || opts.reflect_origin) { return true; }
    let i: usize = 0;
    while (i < opts.allowed_origin_count) {
        if (wildmatchIgnoreCase(opts.allowed_origins[i], request_origin)) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Checks whether `method` is present in a comma-separated `allowed_methods` list (case-insensitive).
pub fn isMethodAllowed(allowed_methods: string, method: string): bool {
    if (method.len == 0) { return false; }
    let pos: usize = 0;
    while (pos <= allowed_methods.len) {
        let sep = pos;
        while (sep < allowed_methods.len && allowed_methods[sep] != 44) { sep = sep + 1; }
        let tok = corsTrim(allowed_methods[pos..sep]);
        if (tok.len > 0 && eqAscii(tok, method)) {
            return true;
        }
        if (sep >= allowed_methods.len) { break; }
        pos = sep + 1;
    }
    return false;
}

/// Evaluates an incoming HTTP request (`method`, `origin`, `ac_req_method`, `ac_req_headers`)
/// against `opts` and returns the structured `CorsDecision`.
pub fn evaluateCors(
    opts: CorsOptions,
    method: string,
    origin: string,
    ac_req_method: string,
    ac_req_headers: string
): CorsDecision {
    let is_preflight = eqAscii(method, "OPTIONS") && ac_req_method.len > 0;
    let empty = CorsDecision {
        allowed: false,
        is_preflight: is_preflight,
        allow_origin: "",
        vary_origin: !opts.allow_any_origin || opts.allow_credentials,
        allow_credentials: false,
        allow_methods: "",
        allow_headers: "",
        expose_headers: "",
        max_age: 0,
        has_max_age: false,
    };

    if (!isOriginAllowed(opts, origin)) {
        return empty;
    }
    if (is_preflight && !isMethodAllowed(opts.allowed_methods, ac_req_method)) {
        return empty;
    }

    // Per Fetch spec: when allow_credentials is true, cannot emit literal "*"; must reflect Origin + Vary: Origin.
    let emit_star = opts.allow_any_origin && !opts.allow_credentials && !opts.reflect_origin;
    let resolved_origin = if (emit_star) { "*" } else { origin };
    let vary = !emit_star;
    let resolved_headers = if (opts.allowed_headers.len > 0) { opts.allowed_headers } else { ac_req_headers };

    return CorsDecision {
        allowed: true,
        is_preflight: is_preflight,
        allow_origin: resolved_origin,
        vary_origin: vary,
        allow_credentials: opts.allow_credentials,
        allow_methods: if (is_preflight) { opts.allowed_methods } else { "" },
        allow_headers: if (is_preflight) { resolved_headers } else { "" },
        expose_headers: if (!is_preflight) { opts.exposed_headers } else { "" },
        max_age: if (is_preflight) { opts.max_age } else { 0 },
        has_max_age: is_preflight && opts.has_max_age,
    };
}

/// Formats the HTTP response headers block (CRLF-delimited) for a `CorsDecision` into `out`.
pub fn formatCorsHeaders(d: CorsDecision, out: []u8): usize {
    if (!d.allowed) { return 0; }
    let p: usize = 0;
    p = corsPush(out, p, "Access-Control-Allow-Origin: ");
    p = corsPush(out, p, d.allow_origin);
    p = corsPush(out, p, "\r\n");

    if (d.vary_origin) {
        p = corsPush(out, p, "Vary: Origin\r\n");
    }
    if (d.allow_credentials) {
        p = corsPush(out, p, "Access-Control-Allow-Credentials: true\r\n");
    }
    if (d.is_preflight) {
        if (d.allow_methods.len > 0) {
            p = corsPush(out, p, "Access-Control-Allow-Methods: ");
            p = corsPush(out, p, d.allow_methods);
            p = corsPush(out, p, "\r\n");
        }
        if (d.allow_headers.len > 0) {
            p = corsPush(out, p, "Access-Control-Allow-Headers: ");
            p = corsPush(out, p, d.allow_headers);
            p = corsPush(out, p, "\r\n");
        }
        if (d.has_max_age) {
            p = corsPush(out, p, "Access-Control-Max-Age: ");
            p = corsPushU32(out, p, d.max_age);
            p = corsPush(out, p, "\r\n");
        }
    } else if (d.expose_headers.len > 0) {
        p = corsPush(out, p, "Access-Control-Expose-Headers: ");
        p = corsPush(out, p, d.expose_headers);
        p = corsPush(out, p, "\r\n");
    }
    return p;
}
