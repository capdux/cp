// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class CorsConfig {
    allow_origin: string,
    allow_methods: string,
    allow_headers: string,
    expose_headers: string,
    allow_credentials: bool,
    max_age: string,
}

export fn corsHasCtlOrCrlf(s: string): bool;
export fn corsBytesEql(a: string, b: string): bool;
export fn isOriginAllowed(allow_origin: string, origin: string): bool;
export fn applyCorsHeaders(ctx: &mut Context, origin: string);
export fn corsOnRequest(ctx: &mut Context): bool;
export fn createCorsPlugin(cfg: CorsConfig): Plugin;
export fn registerCors(app: &mut App, cfg: CorsConfig): &mut App;
