// Origin: Official @web/logger Plugin (Observability, Trace & OpenMetrics) | Created: 2026-11-25
// ============================================================================
// @web/logger — Production Observability Plugin for cap:web
//   - Pass-through or auto-generated monotonic `X-Request-Id`
//   - W3C Trace Context `traceparent` response header
//   - Zero-heap structured JSON access log formatter
//   - Automatic `GET /metrics` OpenMetrics / Prometheus exposition endpoint
// ============================================================================

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub struct LoggerConfig {
    request_id_header: string = "X-Request-Id";
    metrics_path: string = "/metrics";
    enable_trace: bool = true;
}

pub struct MetricsSnapshot {
    requests_total: u64 = 0;
    responses_2xx_total: u64 = 0;
    responses_4xx_total: u64 = 0;
    responses_5xx_total: u64 = 0;
}

// ============================================================================
// Zero-Heap Formatting Helpers
// ============================================================================

fn logWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn logWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn logHexDigitLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn logWriteU64Decimal(out_buf: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        return logWriteByte(out_buf, pos, '0');
    }
    while (v > 0) {
        tmp[n] = (48 + @as(u8, @truncate(v % 10))) as u8;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        if (!logWriteByte(out_buf, pos, tmp[n])) return false;
    }
    return true;
}

fn logWriteU64HexFixed16(out_buf: []u8, pos: *usize, val: u64): bool {
    if (pos.* + 16 > out_buf.len) return false;
    let shift: u6 = 60;
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const nib: u8 = @truncate((val >> shift) & 0x0F);
        out_buf[pos.*] = logHexDigitLower(nib);
        pos.* += 1;
        if (shift >= 4) {
            shift -= 4;
        }
    }
    return true;
}

fn logWriteJsonEscaped(out_buf: []u8, pos: *usize, s: string): bool {
    if (!logWriteByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            if (!logWriteRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!logWriteRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!logWriteRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!logWriteRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!logWriteRaw(out_buf, pos, "\\t")) return false;
        } else if (b < 0x20) {
            if (!logWriteRaw(out_buf, pos, "\\u00")) return false;
            if (!logWriteByte(out_buf, pos, logHexDigitLower((b >> 4) & 0x0F))) return false;
            if (!logWriteByte(out_buf, pos, logHexDigitLower(b & 0x0F))) return false;
        } else {
            if (!logWriteByte(out_buf, pos, b)) return false;
        }
    }
    return logWriteByte(out_buf, pos, '"');
}

// ============================================================================
// Public Pure Formatters (Zero Heap)
// ============================================================================

/// Format a structured JSON access log line into `out_buf`.
pub fn formatAccessLogJson(method: string, path: string, status: u16, req_id: string, out_buf: []u8): usize? {
    let pos: usize = 0;
    if (!logWriteRaw(out_buf, *pos, "{\"method\":")) return null;
    if (!logWriteJsonEscaped(out_buf, *pos, method)) return null;
    if (!logWriteRaw(out_buf, *pos, ",\"path\":")) return null;
    if (!logWriteJsonEscaped(out_buf, *pos, path)) return null;
    if (!logWriteRaw(out_buf, *pos, ",\"status\":")) return null;
    if (!logWriteU64Decimal(out_buf, *pos, @as(u64, status))) return null;
    if (!logWriteRaw(out_buf, *pos, ",\"request_id\":")) return null;
    if (!logWriteJsonEscaped(out_buf, *pos, req_id)) return null;
    if (!logWriteByte(out_buf, *pos, '}')) return null;
    return pos;
}

/// Format a W3C Trace Context `traceparent` header value (`00-<32hex>-<16hex>-01`).
pub fn formatTraceparent(seq: u64, out_buf: []u8): usize? {
    let pos: usize = 0;
    const seed_hi: u64 = 0x4bf92f3577b34da6;
    const seed_lo: u64 = 0x23ce929d0e0e4736;
    const seed_span: u64 = 0x00f067aa0ba902b7;
    const mul_hi: u64 = 0x1e3779b97f4a7c15;
    const mul_span: u64 = 0x3f58476d1ce4e5b9;
    const hi: u64 = seed_hi ^ (seq *% mul_hi);
    const lo: u64 = seed_lo +% seq;
    const span_id: u64 = seed_span ^ (seq *% mul_span);
    if (!logWriteRaw(out_buf, *pos, "00-")) return null;
    if (!logWriteU64HexFixed16(out_buf, *pos, hi)) return null;
    if (!logWriteU64HexFixed16(out_buf, *pos, lo)) return null;
    if (!logWriteByte(out_buf, *pos, '-')) return null;
    if (!logWriteU64HexFixed16(out_buf, *pos, span_id)) return null;
    if (!logWriteRaw(out_buf, *pos, "-01")) return null;
    return pos;
}

/// Format Prometheus / OpenMetrics text exposition into `out_buf`.
pub fn formatOpenMetrics(snap: MetricsSnapshot, out_buf: []u8): usize? {
    let pos: usize = 0;
    if (!logWriteRaw(out_buf, *pos, "# HELP http_requests_total Total HTTP requests handled\n")) return null;
    if (!logWriteRaw(out_buf, *pos, "# TYPE http_requests_total counter\n")) return null;
    if (!logWriteRaw(out_buf, *pos, "http_requests_total ")) return null;
    if (!logWriteU64Decimal(out_buf, *pos, snap.requests_total)) return null;
    if (!logWriteByte(out_buf, *pos, '\n')) return null;

    if (!logWriteRaw(out_buf, *pos, "# TYPE http_responses_2xx_total counter\n")) return null;
    if (!logWriteRaw(out_buf, *pos, "http_responses_2xx_total ")) return null;
    if (!logWriteU64Decimal(out_buf, *pos, snap.responses_2xx_total)) return null;
    if (!logWriteByte(out_buf, *pos, '\n')) return null;

    if (!logWriteRaw(out_buf, *pos, "# TYPE http_responses_4xx_total counter\n")) return null;
    if (!logWriteRaw(out_buf, *pos, "http_responses_4xx_total ")) return null;
    if (!logWriteU64Decimal(out_buf, *pos, snap.responses_4xx_total)) return null;
    if (!logWriteByte(out_buf, *pos, '\n')) return null;

    if (!logWriteRaw(out_buf, *pos, "# TYPE http_responses_5xx_total counter\n")) return null;
    if (!logWriteRaw(out_buf, *pos, "http_responses_5xx_total ")) return null;
    if (!logWriteU64Decimal(out_buf, *pos, snap.responses_5xx_total)) return null;
    if (!logWriteByte(out_buf, *pos, '\n')) return null;

    return pos;
}

// ============================================================================
// Module-Level Zero-Heap Observability State & Lifecycle Hooks
// ============================================================================

let g_logger_cfg: LoggerConfig = LoggerConfig{};
let g_req_seq: u64 = 0;
let g_metrics: MetricsSnapshot = MetricsSnapshot{};
let g_req_id_buf: [64]u8 = [0; 64];
let g_trace_buf: [64]u8 = [0; 64];
let g_last_log_buf: [512]u8 = [0; 512];
let g_last_log_len: usize = 0;
let g_metrics_buf: [1024]u8 = [0; 1024];

pub fn resetState() {
    g_req_seq = 0;
    g_metrics = MetricsSnapshot{};
    g_last_log_len = 0;
}

pub fn getMetrics(): MetricsSnapshot {
    return g_metrics;
}

pub fn lastAccessLog(): string {
    return g_last_log_buf[0..g_last_log_len];
}

pub fn onRequestHook(ctx: *Context): bool {
    g_req_seq += 1;
    const id_hdr = if (g_logger_cfg.request_id_header.len > 0) g_logger_cfg.request_id_header else "X-Request-Id";

    let req_id: string = "";
    if (incoming := ctx.header(id_hdr)) {
        if (incoming.len > 0) {
            req_id = incoming;
        }
    }
    if (req_id.len == 0) {
        let pos: usize = 0;
        * = logWriteRaw(g_req_id_buf[0..], *pos, "req-");
        * = logWriteU64Decimal(g_req_id_buf[0..], *pos, g_req_seq);
        req_id = g_req_id_buf[0..pos];
    }

    * = ctx.setDerived("request_id", req_id);
    * = ctx.setHeader(id_hdr, req_id);

    if (g_logger_cfg.enable_trace) {
        let tp: string = "";
        if (in_tp := ctx.header("traceparent")) {
            if (in_tp.len > 0) {
                tp = in_tp;
            }
        }
        if (tp.len == 0) {
            if (tp_len := formatTraceparent(g_req_seq, g_trace_buf[0..])) {
                tp = g_trace_buf[0..tp_len];
            }
        }
        if (tp.len > 0) {
            * = ctx.setDerived("traceparent", tp);
            * = ctx.setHeader("traceparent", tp);
        }
    }

    return true;
}

pub fn onAfterHook(ctx: *Context, resp: *Response) {
    g_metrics.requests_total += 1;
    if (resp.status >= 200 && resp.status < 300) {
        g_metrics.responses_2xx_total += 1;
    } else if (resp.status >= 400 && resp.status < 500) {
        g_metrics.responses_4xx_total += 1;
    } else if (resp.status >= 500 && resp.status < 600) {
        g_metrics.responses_5xx_total += 1;
    }

    const req_id = ctx.derived("request_id") orelse "unknown";
    if (log_len := formatAccessLogJson(ctx.method.asString(), ctx.path, resp.status, req_id, g_last_log_buf[0..])) {
        g_last_log_len = log_len;
    }
}

pub fn metricsHandler(ctx: *Context): Response {
    * = ctx;
    if (n := formatOpenMetrics(g_metrics, g_metrics_buf[0..])) {
        return Response.bytes(200, g_metrics_buf[0..n], "text/plain; version=0.0.4; charset=utf-8");
    }
    return Response.text(500, "metrics buffer overflow");
}

/// Create a composable Elysia-style `Plugin` for `@web/logger`.
pub fn createLoggerPlugin(cfg: LoggerConfig): Plugin {
    g_logger_cfg = cfg;
    let p = Plugin.init("@web/logger");
    * = p.withScope(PluginScope.Global);
    * = p.onRequest(onRequestHook);
    * = p.onAfterHandle(onAfterHook);
    if (cfg.metrics_path.len > 0) {
        * = p.get(cfg.metrics_path, metricsHandler);
    }
    return p;
}

/// Register `@web/logger` directly onto `app` (fluent chainable).
pub fn registerLogger(app: *App, cfg: LoggerConfig): *App {
    const p = createLoggerPlugin(cfg);
    return app.usePlugin(&p);
}
