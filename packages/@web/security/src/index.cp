// Official @web/security Plugin for `cap:web`
// Zero-heap Helmet security headers (HSTS, CSP, X-Content-Type-Options, X-Frame-Options,
// Referrer-Policy, Permissions-Policy) + constant-time Double-Submit Cookie CSRF protection.

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub struct SecurityConfig {
    hsts: string = "max-age=31536000; includeSubDomains";
    csp: string = "default-src 'self'";
    x_frame_options: string = "DENY";
    x_content_type_options: string = "nosniff";
    referrer_policy: string = "strict-origin-when-cross-origin";
    permissions_policy: string = "geolocation=(), microphone=(), camera=()";
    enable_csrf: bool = false;
    csrf_cookie_name: string = "csrf_token";
    csrf_header_name: string = "X-CSRF-Token";
    csrf_min_token_len: usize = 16;
}

let g_sec_cfg: SecurityConfig = SecurityConfig{};

/// Constant-time byte comparison to prevent timing side-channels on CSRF tokens.
pub fn constantTimeEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

fn isSafeHttpMethod(m: AppMethod): bool {
    return m == AppMethod.GET || m == AppMethod.HEAD || m == AppMethod.OPTIONS;
}

/// Global `onRequest` hook: enforces Double-Submit Cookie CSRF on state-changing methods
/// and attaches Helmet security headers onto every response.
pub fn securityOnRequest(ctx: *Context): bool {
    const cfg = g_sec_cfg;

    if (cfg.hsts.len > 0) {
        * = ctx.setHeader("Strict-Transport-Security", cfg.hsts);
    }
    if (cfg.csp.len > 0) {
        * = ctx.setHeader("Content-Security-Policy", cfg.csp);
    }
    if (cfg.x_frame_options.len > 0) {
        * = ctx.setHeader("X-Frame-Options", cfg.x_frame_options);
    }
    if (cfg.x_content_type_options.len > 0) {
        * = ctx.setHeader("X-Content-Type-Options", cfg.x_content_type_options);
    }
    if (cfg.referrer_policy.len > 0) {
        * = ctx.setHeader("Referrer-Policy", cfg.referrer_policy);
    }
    if (cfg.permissions_policy.len > 0) {
        * = ctx.setHeader("Permissions-Policy", cfg.permissions_policy);
    }

    if (cfg.enable_csrf && !isSafeHttpMethod(ctx.method)) {
        const cookie_tok = ctx.cookie(cfg.csrf_cookie_name) ?? "";
        const header_tok = ctx.header(cfg.csrf_header_name) ?? "";
        if (cookie_tok.len < cfg.csrf_min_token_len || !constantTimeEql(cookie_tok, header_tok)) {
            ctx.abort(Response.json(403, "{\"error\":\"csrf_token_mismatch\"}"));
            return false;
        }
    }

    return true;
}

/// Build a reusable `cap:web` `Plugin` for `@web/security`.
pub fn createSecurityPlugin(cfg: SecurityConfig): Plugin {
    g_sec_cfg = cfg;
    let p = Plugin.init("@web/security");
    * = p.withScope(PluginScope.Global).onRequest(securityOnRequest);
    return p;
}

/// One-call registration onto a `cap:web` `App`.
pub fn registerSecurity(app: *App, cfg: SecurityConfig): *App {
    const p = createSecurityPlugin(cfg);
    return app.usePlugin(&p);
}
