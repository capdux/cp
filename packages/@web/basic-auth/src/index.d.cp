// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class BasicAuthConfig {
    username: string,
    password: string,
    api_key: string,
    api_key_header: string,
    realm: string,
    optional_auth: bool,
}

export fn baConstTimeEql(a: string, b: string): bool;
export fn baDecodeVal(c: u8): u8?;
export fn basicAuthDecodeBase64(input: string, out_buf: []u8): usize?;
export fn baStartsWithIgnoreCase(s: string, prefix: string): bool;
export fn basicAuthHook(ctx: &mut Context): bool;
export fn createBasicAuthPlugin(cfg: BasicAuthConfig): Plugin;
export fn registerBasicAuth(app: &mut App, cfg: BasicAuthConfig): &mut App;
