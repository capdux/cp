// Official @web/cors Plugin for `cap:web`
// Zero-heap W3C Fetch CORS middleware: preflight OPTIONS 204, origin allowlist,
// credentials reflection, CRLF injection rejection, and Vary: Origin handling.

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub struct CorsConfig {
    allow_origin: string = "*";
    allow_methods: string = "GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD";
    allow_headers: string = "Content-Type, Authorization, X-Requested-With, X-CSRF-Token";
    expose_headers: string = "";
    allow_credentials: bool = false;
    max_age: string = "86400";
}

let g_cors_cfg: CorsConfig = CorsConfig{};

fn corsHasCtlOrCrlf(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < 0x20 || c == 0x7F) return true;
    }
    return false;
}

fn corsBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Checks if `origin` is permitted by `allow_origin` (`"*"` or comma-separated exact origins).
pub fn isOriginAllowed(allow_origin: string, origin: string): bool {
    if (origin.len == 0 || corsHasCtlOrCrlf(origin)) return false;
    if (corsBytesEql(allow_origin, "*")) return true;

    let pos: usize = 0;
    while (pos < allow_origin.len) {
        let end = pos;
        while (end < allow_origin.len && allow_origin[end] != ',') : (end += 1) {}
        let start = pos;
        while (start < end && (allow_origin[start] == ' ' || allow_origin[start] == '\t')) : (start += 1) {}
        let trim_end = end;
        while (trim_end > start && (allow_origin[trim_end - 1] == ' ' || allow_origin[trim_end - 1] == '\t')) : (trim_end -= 1) {}
        if (trim_end > start && corsBytesEql(allow_origin[start..trim_end], origin)) {
            return true;
        }
        if (end >= allow_origin.len) break;
        pos = end + 1;
    }
    return false;
}

fn applyCorsHeaders(ctx: *Context, origin: string) {
    const cfg = g_cors_cfg;
    if (corsBytesEql(cfg.allow_origin, "*") && !cfg.allow_credentials) {
        * = ctx.setHeader("Access-Control-Allow-Origin", "*");
    } else {
        * = ctx.setHeader("Access-Control-Allow-Origin", origin);
        * = ctx.setHeader("Vary", "Origin");
    }
    if (cfg.allow_credentials) {
        * = ctx.setHeader("Access-Control-Allow-Credentials", "true");
    }
    if (cfg.expose_headers.len > 0) {
        * = ctx.setHeader("Access-Control-Expose-Headers", cfg.expose_headers);
    }
}

/// Global `onRequest` hook for CORS preflight (`OPTIONS`) & response headers.
pub fn corsOnRequest(ctx: *Context): bool {
    const origin = ctx.header("Origin") ?? return true;
    const cfg = g_cors_cfg;
    if (!isOriginAllowed(cfg.allow_origin, origin)) {
        if (ctx.method == AppMethod.OPTIONS) {
            ctx.abort(Response.statusCode(403));
            return false;
        }
        return true;
    }

    if (ctx.method == AppMethod.OPTIONS) {
        let resp = Response.noContent();
        if (corsBytesEql(cfg.allow_origin, "*") && !cfg.allow_credentials) {
            * = resp.setHeader("Access-Control-Allow-Origin", "*");
        } else {
            * = resp.setHeader("Access-Control-Allow-Origin", origin);
            * = resp.setHeader("Vary", "Origin");
        }
        if (cfg.allow_credentials) {
            * = resp.setHeader("Access-Control-Allow-Credentials", "true");
        }
        if (cfg.allow_methods.len > 0) {
            * = resp.setHeader("Access-Control-Allow-Methods", cfg.allow_methods);
        }
        if (cfg.allow_headers.len > 0) {
            * = resp.setHeader("Access-Control-Allow-Headers", cfg.allow_headers);
        }
        if (cfg.max_age.len > 0) {
            * = resp.setHeader("Access-Control-Max-Age", cfg.max_age);
        }
        ctx.abort(resp);
        return false;
    }

    applyCorsHeaders(ctx, origin);
    return true;
}

/// Build a reusable `cap:web` `Plugin` for `@web/cors`.
pub fn createCorsPlugin(cfg: CorsConfig): Plugin {
    g_cors_cfg = cfg;
    let p = Plugin.init("@web/cors");
    * = p.withScope(PluginScope.Global).onRequest(corsOnRequest);
    return p;
}

/// One-call registration onto a `cap:web` `App`.
pub fn registerCors(app: *App, cfg: CorsConfig): *App {
    const p = createCorsPlugin(cfg);
    return app.usePlugin(&p);
}
