// Official @web/sensible Plugin for `cap:web`
// RFC 9457 Problem Details (`application/problem+json`) & standard HTTP error helpers
// (combining @fastify/sensible ergonomics with Axum zero-alloc response formatting).

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub struct SensibleConfig {
    expose_problem_type_base: string = "https://httpstatuses.io/";
    default_error_header: string = "X-Error-Format";
}

let g_sens_cfg: SensibleConfig = SensibleConfig{};

fn sensWriteRaw(dst: []u8, pos: *usize, s: string): bool {
    if (s.len > dst.len || pos.* > dst.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        dst[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn sensWriteEscaped(dst: []u8, pos: *usize, s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"') {
            if (!sensWriteRaw(dst, pos, "\\\"")) return false;
        } else if (c == '\\') {
            if (!sensWriteRaw(dst, pos, "\\\\")) return false;
        } else if (c >= 0x20) {
            if (pos.* >= dst.len) return false;
            dst[pos.*] = c;
            pos.* += 1;
        }
    }
    return true;
}

fn sensWriteU16(dst: []u8, pos: *usize, v: u16): bool {
    let tmp: [8]u8 = undefined;
    let n: usize = 0;
    let x: u16 = v;
    if (x == 0) {
        if (pos.* >= dst.len) return false;
        dst[pos.*] = '0';
        pos.* += 1;
        return true;
    }
    while (x > 0) {
        tmp[n] = (48 + ((x % 10) as u8)) as u8;
        n += 1;
        x = x / 10;
    }
    if (pos.* + n > dst.len) return false;
    while (n > 0) {
        n -= 1;
        dst[pos.*] = tmp[n];
        pos.* += 1;
    }
    return true;
}

/// Build an RFC 9457 `application/problem+json` response into `Response.body_buf` (zero heap).
pub fn problemResponse(status_code: u16, title: string, detail: string): Response {
    let r = Response{
        status: status_code,
        content_type: "application/problem+json; charset=utf-8",
        body: "",
        body_buf: undefined,
        body_buf_len: 0,
        headers: undefined,
        header_len: 0,
    };
    let pos: usize = 0;
    * = sensWriteRaw(r.body_buf[0..], *pos, "{\"status\":");
    * = sensWriteU16(r.body_buf[0..], *pos, status_code);
    * = sensWriteRaw(r.body_buf[0..], *pos, ",\"title\":\"");
    * = sensWriteEscaped(r.body_buf[0..], *pos, title);
    * = sensWriteRaw(r.body_buf[0..], *pos, "\",\"detail\":\"");
    * = sensWriteEscaped(r.body_buf[0..], *pos, detail);
    * = sensWriteRaw(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

pub fn httpBadRequest(detail: string): Response {
    return problemResponse(400, "Bad Request", detail);
}

pub fn httpUnauthorized(detail: string): Response {
    return problemResponse(401, "Unauthorized", detail);
}

pub fn httpForbidden(detail: string): Response {
    return problemResponse(403, "Forbidden", detail);
}

pub fn httpNotFound(detail: string): Response {
    return problemResponse(404, "Not Found", detail);
}

pub fn httpConflict(detail: string): Response {
    return problemResponse(409, "Conflict", detail);
}

pub fn httpUnprocessable(detail: string): Response {
    return problemResponse(422, "Unprocessable Entity", detail);
}

pub fn httpTooManyRequests(detail: string): Response {
    return problemResponse(429, "Too Many Requests", detail);
}

pub fn httpInternalError(detail: string): Response {
    return problemResponse(500, "Internal Server Error", detail);
}

/// Guard assertion helper (like `fastify.assert`): aborts `ctx` with RFC 9457 error when `cond == false`.
pub fn sensibleAssert(ctx: *Context, cond: bool, status_code: u16, title: string, detail: string): bool {
    if (cond) return true;
    ctx.abort(problemResponse(status_code, title, detail));
    return false;
}

pub fn sensibleAfterHook(ctx: *Context, resp: *Response) {
    * = ctx;
    if (resp.status >= 400 && g_sens_cfg.default_error_header.len > 0) {
        * = resp.setHeader(g_sens_cfg.default_error_header, "rfc9457");
    }
}

pub fn createSensiblePlugin(cfg: SensibleConfig): Plugin {
    g_sens_cfg = cfg;
    let p = Plugin.init("@web/sensible");
    * = p.withScope(PluginScope.Global).onAfterHandle(sensibleAfterHook);
    return p;
}

pub fn registerSensible(app: *App, cfg: SensibleConfig): *App {
    const p = createSensiblePlugin(cfg);
    return app.usePlugin(&p);
}
