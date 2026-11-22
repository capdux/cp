// Origin: Pure-CP Stdlib (Zero-Alloc HTTP Onion Middleware Pipeline & WebContext Facade) | Created: 2026-11-22
// Pure CP zero-heap-allocation HTTP API middleware chain:
//   1. `WebContext`: typed request/response envelope with abort state, CORS header & X-Request-Id tracking
//   2. `WebMiddlewarePipeline`: onion-style pre/post middleware chain (413 body limit, CORS whitelist & OPTIONS 204 preflight,
//      constant-time Bearer auth with skip-path whitelist, latency & status telemetry, X-Request-Id pass-through/generation)
//   3. `renderJsonErrorBody`: RFC 8259 escaped standardized JSON error body renderer

const std = @import("std");

// ============================================================================
// Internal Buffer, CSV, Constant-Time & JSON Escaping Helpers
// ============================================================================

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn writeByte(out_buf: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeRaw(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn hexDigitLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn writeU64Dec(out_buf: *[]u8, pos: *usize, value: u64): bool {
    let digits: [20]u8 = [0; 20];
    let len: usize = 0;
    let v: u64 = value;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) : (len += 1) {
            digits[len] = ('0' as u8) + ((v % 10) as u8);
            v = v / 10;
        }
    }
    while (len > 0) {
        len -= 1;
        if (!writeByte(out_buf, pos, digits[len])) return false;
    }
    return true;
}

fn writeJsonString(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (!writeByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b: u8 = s[i];
        if (b == '"') {
            if (!writeRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!writeRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!writeRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!writeRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!writeRaw(out_buf, pos, "\\t")) return false;
        } else if (b == 0x08) {
            if (!writeRaw(out_buf, pos, "\\b")) return false;
        } else if (b == 0x0C) {
            if (!writeRaw(out_buf, pos, "\\f")) return false;
        } else if (b < 0x20) {
            if (!writeRaw(out_buf, pos, "\\u00")) return false;
            if (!writeByte(out_buf, pos, hexDigitLower((b >> 4) & 0x0F))) return false;
            if (!writeByte(out_buf, pos, hexDigitLower(b & 0x0F))) return false;
        } else {
            if (!writeByte(out_buf, pos, b)) return false;
        }
    }
    return writeByte(out_buf, pos, '"');
}

/// Constant-time byte slice equality check for Bearer token authentication.
fn constantTimeEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

/// Checks whether `needle` equals any trimmed comma-separated item in `csv`.
fn valueInCsv(needle: string, csv: string): bool {
    if (needle.len == 0 || csv.len == 0) return false;
    let p: usize = 0;
    while (p < csv.len) {
        let seg_end: usize = p;
        while (seg_end < csv.len && csv[seg_end] != ',') : (seg_end += 1) {}
        let s: usize = p;
        let e: usize = seg_end;
        while (s < e && isWs(csv[s])) : (s += 1) {}
        while (e > s && isWs(csv[e - 1])) : (e -= 1) {}
        if (e > s && std.mem.eql(u8, needle, csv[s..e])) {
            return true;
        }
        p = if (seg_end < csv.len) seg_end + 1 else seg_end;
    }
    return false;
}

// ============================================================================
// 1. Typed HTTP Request / Response Context (`WebContext`)
// ============================================================================

/// Zero-heap-allocation request/response context flowing through `WebMiddlewarePipeline`.
pub struct WebContext {
    method: string = "GET";
    path: string = "/";
    origin: string = "";
    client_ip: string = "";
    bearer_token: string = "";
    request_id: string = "";
    body_len: usize = 0;
    status_code: u16 = 200;
    elapsed_us: u64 = 0;
    aborted: bool = false;
    abort_reason: string = "";
    cors_allow_origin: string = "";

    /// Short-circuits the middleware chain with an HTTP error status and reason.
    fn abort(*_, status_code: u16, reason: string) {
        _.status_code = status_code;
        _.aborted = true;
        _.abort_reason = reason;
    }

    /// Returns `true` if the context has not been aborted and holds a 2xx/3xx status code.
    fn isOk(): bool {
        return !_.aborted && _.status_code >= 200 && _.status_code < 400;
    }
}

// ============================================================================
// 2. Onion Middleware Pipeline (`WebMiddlewarePipeline`)
// ============================================================================

/// Configuration for the built-in production HTTP middleware pipeline.
pub struct WebMiddlewareConfig {
    /// Maximum allowed request body size in bytes (`0` disables the body limit check).
    max_body_bytes: usize = 1048576;
    /// Allowed CORS origin (`""` disables CORS; `"*"` allows all; or CSV allowlist).
    allowed_origin: string = "";
    /// Expected Bearer token (`""` disables Bearer authentication).
    required_bearer_token: string = "";
    /// Comma-separated exact paths bypassing Bearer authentication (e.g. `"/healthz,/metrics"`).
    auth_skip_paths_csv: string = "/healthz,/metrics";

    static fn default(): WebMiddlewareConfig {
        return WebMiddlewareConfig{
            max_body_bytes: 1048576,
            allowed_origin: "",
            required_bearer_token: "",
            auth_skip_paths_csv: "/healthz,/metrics",
        };
    }
}

/// Pre-handler routing decision returned by `WebMiddlewarePipeline.runBefore`.
pub enum WebMiddlewareOutcome {
    Continue,
    PreflightHandled,
    RejectedPayloadTooLarge,
    RejectedCors,
    RejectedUnauthorized,
}

/// Cumulative HTTP access, rejection, and latency metrics tracked by `WebMiddlewarePipeline`.
pub struct WebMiddlewareStats {
    total_requests: u64 = 0;
    passed_requests: u64 = 0;
    preflight_204: u64 = 0;
    rejected_401: u64 = 0;
    rejected_403: u64 = 0;
    rejected_413: u64 = 0;
    server_5xx: u64 = 0;
    total_latency_us: u64 = 0;
    max_latency_us: u64 = 0;
}

/// Zero-heap-allocation HTTP onion middleware pipeline enforcing:
///   1. Request body size cap (`413 Payload Too Large`)
///   2. CORS origin allowlist & `OPTIONS` preflight short-circuit (`204` / `403`)
///   3. Constant-time Bearer token auth with skip-path allowlist (`401 Unauthorized`)
///   4. `X-Request-Id` pass-through or zero-heap generation
///   5. Post-handler status & latency telemetry (`recordAfter`)
pub struct WebMiddlewarePipeline {
    config: WebMiddlewareConfig;
    stats_acc: WebMiddlewareStats = WebMiddlewareStats{};
    next_req_seq: u64 = 1;

    static fn init(config: WebMiddlewareConfig): WebMiddlewarePipeline {
        return WebMiddlewarePipeline{
            config: config,
            stats_acc: WebMiddlewareStats{},
            next_req_seq: 1,
        };
    }

    /// Preserves an existing `ctx.request_id` zero-copy, or generates `"req-<seq>"` into `id_buf`
    /// and binds `ctx.request_id` to it. Returns `null` if `id_buf` is too small.
    fn ensureRequestId(*_, ctx: *WebContext, id_buf: *[]u8): string? {
        if (ctx.request_id.len > 0) {
            return ctx.request_id;
        }
        let pos: usize = 0;
        if (!writeRaw(id_buf, &pos, "req-")) return null;
        if (!writeU64Dec(id_buf, &pos, _.next_req_seq)) return null;
        _.next_req_seq += 1;
        ctx.request_id = id_buf[0..pos];
        return ctx.request_id;
    }

    /// Executes the pre-handler middleware layers in deterministic onion order.
    fn runBefore(*_, ctx: *WebContext): WebMiddlewareOutcome {
        _.stats_acc.total_requests += 1;

        // 1. Body size guard (413 Payload Too Large)
        if (_.config.max_body_bytes > 0 && ctx.body_len > _.config.max_body_bytes) {
            ctx.abort(413, "Payload Too Large");
            _.stats_acc.rejected_413 += 1;
            return WebMiddlewareOutcome.RejectedPayloadTooLarge;
        }

        // 2. CORS origin allowlist & OPTIONS preflight
        if (_.config.allowed_origin.len > 0) {
            const allow_any: bool = std.mem.eql(u8, _.config.allowed_origin, "*");
            const is_options: bool = std.mem.eql(u8, ctx.method, "OPTIONS");

            if (ctx.origin.len > 0) {
                const origin_ok: bool = allow_any || valueInCsv(ctx.origin, _.config.allowed_origin);
                if (!origin_ok) {
                    ctx.abort(403, "Origin Not Allowed");
                    _.stats_acc.rejected_403 += 1;
                    return WebMiddlewareOutcome.RejectedCors;
                }
                ctx.cors_allow_origin = if (allow_any) "*" else ctx.origin;
            } else if (is_options && !allow_any) {
                ctx.abort(403, "Missing Origin");
                _.stats_acc.rejected_403 += 1;
                return WebMiddlewareOutcome.RejectedCors;
            } else if (is_options && allow_any) {
                ctx.cors_allow_origin = "*";
            }

            if (is_options) {
                ctx.status_code = 204;
                ctx.aborted = true;
                ctx.abort_reason = "CORS Preflight";
                _.stats_acc.preflight_204 += 1;
                return WebMiddlewareOutcome.PreflightHandled;
            }
        }

        // 3. Bearer token authentication with skip-path whitelist
        if (_.config.required_bearer_token.len > 0) {
            if (!valueInCsv(ctx.path, _.config.auth_skip_paths_csv)) {
                if (!constantTimeEql(ctx.bearer_token, _.config.required_bearer_token)) {
                    ctx.abort(401, "Unauthorized");
                    _.stats_acc.rejected_401 += 1;
                    return WebMiddlewareOutcome.RejectedUnauthorized;
                }
            }
        }

        _.stats_acc.passed_requests += 1;
        return WebMiddlewareOutcome.Continue;
    }

    /// Executes the post-handler onion phase, recording final HTTP status and latency metrics.
    fn recordAfter(*_, ctx: *WebContext, status_code: u16, elapsed_us: u64) {
        ctx.status_code = status_code;
        ctx.elapsed_us = elapsed_us;
        _.stats_acc.total_latency_us += elapsed_us;
        if (elapsed_us > _.stats_acc.max_latency_us) {
            _.stats_acc.max_latency_us = elapsed_us;
        }
        if (status_code >= 500 && status_code < 600) {
            _.stats_acc.server_5xx += 1;
        }
    }

    /// Returns a snapshot of cumulative request, rejection, and latency metrics.
    fn stats(): WebMiddlewareStats {
        return _.stats_acc;
    }
}

// ============================================================================
// 3. Standardized JSON Error Response Renderer (`renderJsonErrorBody`)
// ============================================================================

/// Renders a standardized, RFC 8259 escaped JSON error body into caller-provided `out_buf`:
/// `{"error":{"status":401,"message":"Unauthorized","request_id":"req-1"}}`
/// Returns the number of bytes written, or `null` if `out_buf` is too small.
pub fn renderJsonErrorBody(
    status_code: u16,
    error_message: string,
    request_id: string,
    out_buf: *[]u8,
): usize? {
    let pos: usize = 0;
    if (!writeRaw(out_buf, &pos, "{\"error\":{\"status\":")) return null;
    if (!writeU64Dec(out_buf, &pos, status_code as u64)) return null;
    if (!writeRaw(out_buf, &pos, ",\"message\":")) return null;
    if (!writeJsonString(out_buf, &pos, error_message)) return null;
    if (!writeRaw(out_buf, &pos, ",\"request_id\":")) return null;
    if (!writeJsonString(out_buf, &pos, request_id)) return null;
    if (!writeRaw(out_buf, &pos, "}}")) return null;
    return pos;
}
