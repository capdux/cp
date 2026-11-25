// Official @web/basic-auth Plugin for `cap:web`
// RFC 7617 HTTP Basic Authentication (Base64 decode + constant-time verify + WWW-Authenticate)
// and X-API-Key dual-mode authentication (fusing @fastify/basic-auth + tower-http::auth).

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub struct BasicAuthConfig {
    username: string = "";
    password: string = "";
    api_key: string = "";
    api_key_header: string = "X-API-Key";
    realm: string = "Restricted";
    optional_auth: bool = false;
}

let g_ba_cfg: BasicAuthConfig = BasicAuthConfig{};

fn baConstTimeEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

fn baDecodeVal(c: u8): u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
    if (c >= '0' && c <= '9') return (c - '0') + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return null;
}

/// Zero-heap RFC 4648 Base64 decoder for `Authorization: Basic <b64>` credentials.
pub fn basicAuthDecodeBase64(input: string, out_buf: []u8): usize? {
    if (input.len == 0 || (input.len & 3) != 0) return null;
    let i: usize = 0;
    let o: usize = 0;
    while (i < input.len) : (i += 4) {
        const v0 = baDecodeVal(input[i]) ?? return null;
        const v1 = baDecodeVal(input[i + 1]) ?? return null;
        const pad2 = input[i + 2] == '=';
        const pad3 = input[i + 3] == '=';
        const v2: u8 = if (pad2) 0 else (baDecodeVal(input[i + 2]) ?? return null);
        const v3: u8 = if (pad3) 0 else (baDecodeVal(input[i + 3]) ?? return null);

        const triple: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12) | ((v2 as u32) << 6) | (v3 as u32);
        if (o >= out_buf.len) return null;
        out_buf[o] = ((triple >> 16) & 0xFF) as u8;
        o += 1;
        if (!pad2) {
            if (o >= out_buf.len) return null;
            out_buf[o] = ((triple >> 8) & 0xFF) as u8;
            o += 1;
        }
        if (!pad3) {
            if (o >= out_buf.len) return null;
            out_buf[o] = (triple & 0xFF) as u8;
            o += 1;
        }
    }
    return o;
}

fn baStartsWithIgnoreCase(s: string, prefix: string): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        const a = if (s[i] >= 'A' && s[i] <= 'Z') (s[i] + 32) else s[i];
        const b = if (prefix[i] >= 'A' && prefix[i] <= 'Z') (prefix[i] + 32) else prefix[i];
        if (a != b) return false;
    }
    return true;
}

/// Global `derive` hook: validates `X-API-Key` or `Authorization: Basic <b64>` in constant time.
pub fn basicAuthHook(ctx: *Context): bool {
    const cfg = g_ba_cfg;

    // 1. Optional X-API-Key fast-path
    if (cfg.api_key.len > 0) {
        if (k := ctx.header(cfg.api_key_header)) {
            if (baConstTimeEql(k, cfg.api_key)) {
                * = ctx.setDerived("auth_method", "api_key");
                * = ctx.setDerived("auth_user", "api_key");
                return true;
            }
        }
    }

    // 2. RFC 7617 HTTP Basic Auth
    if (cfg.username.len > 0) {
        if (auth := ctx.header("Authorization")) {
            if (baStartsWithIgnoreCase(auth, "Basic ")) {
                const b64 = auth[6..];
                let dec_buf: [256]u8 = undefined;
                if (dlen := basicAuthDecodeBase64(b64, dec_buf[0..])) {
                    const pair = dec_buf[0..dlen];
                    let colon: usize = 0;
                    while (colon < pair.len && pair[colon] != ':') : (colon += 1) {}
                    if (colon < pair.len) {
                        const u = pair[0..colon];
                        const p = pair[colon + 1 ..];
                        if (baConstTimeEql(u, cfg.username) && baConstTimeEql(p, cfg.password)) {
                            * = ctx.setDerived("auth_method", "basic");
                            * = ctx.setDerived("auth_user", cfg.username);
                            return true;
                        }
                    }
                }
            }
        }
    }

    if (cfg.optional_auth) {
        return true;
    }

    let r = Response.json(401, "{\"error\":\"unauthorized\"}");
    * = r.setHeader("WWW-Authenticate", "Basic realm=\"Restricted\"");
    ctx.abort(r);
    return false;
}

pub fn createBasicAuthPlugin(cfg: BasicAuthConfig): Plugin {
    g_ba_cfg = cfg;
    let p = Plugin.init("@web/basic-auth");
    * = p.withScope(PluginScope.Global).derive(basicAuthHook);
    return p;
}

pub fn registerBasicAuth(app: *App, cfg: BasicAuthConfig): *App {
    const p = createBasicAuthPlugin(cfg);
    return app.usePlugin(&p);
}
