// Origin: Official @web/proxy Plugin (Pure-CP Zero-Heap HTTP Reverse Proxy) | Created: 2026-11-25
// ============================================================================
// @web/proxy — Zero-Heap HTTP Reverse Proxy Plugin for cap:web
//   - Inspired by Fastify `@fastify/http-proxy`
//   - Safe prefix stripping, query preservation & path-traversal rejection
//   - Standard HTTP/1.1 upstream request formatter (`Host`, `X-Forwarded-*`, `Content-Length`)
//   - Pluggable `ProxyTransportFn` callback for zero-network unit tests & live forwarding
// ============================================================================

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub struct ProxyConfig {
    prefix: string = "/proxy";
    upstream_host: string = "127.0.0.1";
    upstream_port: u16 = 8080;
    strip_prefix: bool = true;
    add_forwarded_headers: bool = true;
}

pub struct ProxyRequestPlan {
    target_path: [256]u8 = [0; 256];
    target_path_len: usize = 0;
    host_header: [128]u8 = [0; 128];
    host_header_len: usize = 0;
    forwarded_for: string = "";
    forwarded_proto: string = "http";

    pub fn targetPathSlice(): string {
        return _.target_path[0.._.target_path_len];
    }

    pub fn hostHeaderSlice(): string {
        return _.host_header[0.._.host_header_len];
    }
}

pub type ProxyTransportFn = *const fn (*const ProxyConfig, *Context, string): Response;

// ============================================================================
// Internal Prefix-Scoped Helpers (px*)
// ============================================================================

let g_px_cfg: ProxyConfig = ProxyConfig{};
let g_px_transport: ProxyTransportFn? = null;
let g_px_target_buf: [512]u8 = [0; 512];

fn pxAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn pxWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn pxWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn pxWriteU64(out_buf: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        return pxWriteByte(out_buf, pos, '0');
    }
    while (v > 0) {
        tmp[n] = (48 + @as(u8, @truncate(v % 10))) as u8;
        n += 1;
        v = v / 10;
    }
    if (pos.* + n > out_buf.len) return false;
    while (n > 0) {
        n -= 1;
        out_buf[pos.*] = tmp[n];
        pos.* += 1;
    }
    return true;
}

fn pxHasControlChars(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == 0 || c == '\r' || c == '\n') return true;
    }
    return false;
}

fn pxHasTraversal(path: string): bool {
    if (pxHasControlChars(path)) return true;
    let i: usize = 0;
    while (i < path.len) : (i += 1) {
        const c = path[i];
        if (c == '\\') return true;
        // Reject percent-encoded traversal probes (%2e, %2f, %5c)
        if (c == '%' && i + 2 < path.len) {
            const h1 = pxAsciiLower(path[i + 1]);
            const h2 = pxAsciiLower(path[i + 2]);
            if (h1 == '2' && (h2 == 'e' || h2 == 'f')) return true;
            if (h1 == '5' && h2 == 'c') return true;
        }
        // Reject `..` path segment
        if (c == '.' && i + 1 < path.len && path[i + 1] == '.') {
            const prev_ok = (i == 0 || path[i - 1] == '/');
            const next_ok = (i + 2 == path.len || path[i + 2] == '/');
            if (prev_ok && next_ok) return true;
        }
    }
    return false;
}

fn pxTrimTrailingSlash(s: string): string {
    let end = s.len;
    while (end > 1 && s[end - 1] == '/') : (end -= 1) {}
    return s[0..end];
}

fn pxMatchesPrefix(req_path: string, raw_prefix: string): bool {
    if (raw_prefix.len == 0) return true;
    const prefix = pxTrimTrailingSlash(raw_prefix);
    if (prefix.len == 1 && prefix[0] == '/') {
        return req_path.len > 0 && req_path[0] == '/';
    }
    if (req_path.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (req_path[i] != prefix[i]) return false;
    }
    if (req_path.len == prefix.len) return true;
    return req_path[prefix.len] == '/';
}

fn pxDefaultTransport(cfg: *const ProxyConfig, ctx: *Context, target_path: string): Response {
    * = cfg;
    * = ctx;
    * = target_path;
    return Response.json(502, "{\"error\":\"Bad Gateway\",\"message\":\"No upstream proxy transport configured\"}");
}

// ============================================================================
// Public @web/proxy API
// ============================================================================

/// Safely strip configured prefix, guard against path traversal, and preserve raw query string.
pub fn buildProxyTargetPath(req_path: string, raw_query: string, cfg: ProxyConfig, out_buf: []u8): usize? {
    if (req_path.len == 0 || req_path[0] != '/') return null;
    if (pxHasTraversal(req_path)) return null;
    if (pxHasControlChars(raw_query)) return null;

    let base_path: string = req_path;
    if (cfg.strip_prefix && cfg.prefix.len > 0) {
        const prefix = pxTrimTrailingSlash(cfg.prefix);
        if (!(prefix.len == 1 && prefix[0] == '/')) {
            if (!pxMatchesPrefix(req_path, prefix)) return null;
            if (req_path.len == prefix.len) {
                base_path = "/";
            } else {
                base_path = req_path[prefix.len..];
            }
        }
    }

    if (base_path.len == 0) {
        base_path = "/";
    }
    if (pxHasTraversal(base_path)) return null;

    let pos: usize = 0;
    if (!pxWriteRaw(out_buf, *pos, base_path)) return null;
    if (raw_query.len > 0) {
        if (!pxWriteByte(out_buf, *pos, '?')) return null;
        if (!pxWriteRaw(out_buf, *pos, raw_query)) return null;
    }
    return pos;
}

/// Build a zero-heap `ProxyRequestPlan` for the given request path, query, config, and client IP.
pub fn buildProxyRequestPlan(req_path: string, raw_query: string, cfg: ProxyConfig, client_ip: string): ProxyRequestPlan? {
    let plan = ProxyRequestPlan{
        forwarded_for: if (client_ip.len > 0) client_ip else "127.0.0.1",
        forwarded_proto: "http",
    };
    const tlen = buildProxyTargetPath(req_path, raw_query, cfg, plan.target_path[0..]) orelse return null;
    plan.target_path_len = tlen;

    let hpos: usize = 0;
    if (!pxWriteRaw(plan.host_header[0..], *hpos, cfg.upstream_host)) return null;
    if (!pxWriteByte(plan.host_header[0..], *hpos, ':')) return null;
    if (!pxWriteU64(plan.host_header[0..], *hpos, @as(u64, cfg.upstream_port))) return null;
    plan.host_header_len = hpos;

    return plan;
}

/// Format a standard HTTP/1.1 upstream request with `Host`, optional `X-Forwarded-*`, and `Content-Length`.
pub fn formatUpstreamHttp1Request(
    method: AppMethod,
    target_path: string,
    body: string,
    cfg: ProxyConfig,
    client_ip: string,
    out_buf: []u8,
): usize? {
    if (target_path.len == 0 || target_path[0] != '/') return null;
    if (pxHasControlChars(target_path) || pxHasTraversal(target_path)) return null;
    if (cfg.upstream_host.len == 0 || pxHasControlChars(cfg.upstream_host)) return null;
    if (pxHasControlChars(client_ip)) return null;

    let pos: usize = 0;
    if (!pxWriteRaw(out_buf, *pos, method.asString())) return null;
    if (!pxWriteByte(out_buf, *pos, ' ')) return null;
    if (!pxWriteRaw(out_buf, *pos, target_path)) return null;
    if (!pxWriteRaw(out_buf, *pos, " HTTP/1.1\r\n")) return null;

    // Host header
    if (!pxWriteRaw(out_buf, *pos, "Host: ")) return null;
    if (!pxWriteRaw(out_buf, *pos, cfg.upstream_host)) return null;
    if (!pxWriteByte(out_buf, *pos, ':')) return null;
    if (!pxWriteU64(out_buf, *pos, @as(u64, cfg.upstream_port))) return null;
    if (!pxWriteRaw(out_buf, *pos, "\r\n")) return null;

    // Forwarded headers
    if (cfg.add_forwarded_headers) {
        const eff_ip = if (client_ip.len > 0) client_ip else "127.0.0.1";
        if (!pxWriteRaw(out_buf, *pos, "X-Forwarded-For: ")) return null;
        if (!pxWriteRaw(out_buf, *pos, eff_ip)) return null;
        if (!pxWriteRaw(out_buf, *pos, "\r\nX-Forwarded-Proto: http\r\n")) return null;
    }

    // Content-Length + Connection + body
    if (!pxWriteRaw(out_buf, *pos, "Content-Length: ")) return null;
    if (!pxWriteU64(out_buf, *pos, @as(u64, body.len))) return null;
    if (!pxWriteRaw(out_buf, *pos, "\r\nConnection: close\r\n\r\n")) return null;
    if (body.len > 0) {
        if (!pxWriteRaw(out_buf, *pos, body)) return null;
    }
    return pos;
}

/// Request hook that intercepts requests matching `cfg.prefix` and delegates to the configured transport.
pub fn proxyHook(ctx: *Context): bool {
    if (!pxMatchesPrefix(ctx.path, g_px_cfg.prefix)) {
        return true;
    }
    const tlen = buildProxyTargetPath(ctx.path, ctx.raw_query, g_px_cfg, g_px_target_buf[0..]) orelse {
        ctx.abort(Response.json(400, "{\"error\":\"Bad Request\",\"message\":\"Invalid proxy target path\"}"));
        return false;
    };
    const target_path = g_px_target_buf[0..tlen];
    const transport = g_px_transport orelse pxDefaultTransport;
    const resp = transport(&g_px_cfg, ctx, target_path);
    ctx.abort(resp);
    return false;
}

/// Create a composable Elysia-style `Plugin` for `@web/proxy`.
pub fn createProxyPlugin(cfg: ProxyConfig, transport: ProxyTransportFn?): Plugin {
    g_px_cfg = cfg;
    g_px_transport = transport;
    let p = Plugin.init("@web/proxy");
    * = p.withScope(PluginScope.Global);
    * = p.onRequest(proxyHook);
    return p;
}

/// Register `@web/proxy` with default transport (returns 502 if no live transport is attached).
pub fn registerProxy(app: *App, cfg: ProxyConfig): *App {
    const p = createProxyPlugin(cfg, null);
    return app.usePlugin(&p);
}

/// Register `@web/proxy` with a custom pluggable upstream `ProxyTransportFn`.
pub fn registerProxyWithTransport(app: *App, cfg: ProxyConfig, transport: ProxyTransportFn): *App {
    const p = createProxyPlugin(cfg, transport);
    return app.usePlugin(&p);
}
