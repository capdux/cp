// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class SecurityConfig {
    hsts: string,
    csp: string,
    x_frame_options: string,
    x_content_type_options: string,
    referrer_policy: string,
    permissions_policy: string,
    enable_csrf: bool,
    csrf_cookie_name: string,
    csrf_header_name: string,
    csrf_min_token_len: usize,
}

export fn constantTimeEql(a: string, b: string): bool;
export fn isSafeHttpMethod(m: AppMethod): bool;
export fn securityOnRequest(ctx: &mut Context): bool;
export fn createSecurityPlugin(cfg: SecurityConfig): Plugin;
export fn registerSecurity(app: &mut App, cfg: SecurityConfig): &mut App;
