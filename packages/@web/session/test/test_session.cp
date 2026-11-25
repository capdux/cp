// Unit tests for @web/session
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    SessionConfig,
    sessionCreate,
    sessionGet,
    sessionDestroy,
    formatSessionSetCookie,
    sessionHook,
    setSessionTestClock,
    resetSessionStore,
    createSessionPlugin,
    registerSession,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn strContains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (strEql(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

fn profileHandler(ctx: *Context): Response {
    if (sid := ctx.derived("session_id")) {
        if (sval := ctx.derived("session_val")) {
            if (sid.len == 16 && strEql(sval, "uid=42;role=admin")) {
                return Response.json(200, "{\"ok\":true,\"user\":\"uid=42;role=admin\"}");
            }
        }
    }
    return Response.json(401, "{\"ok\":false,\"error\":\"no_session\"}");
}

pub fn main() {
    resetSessionStore();
    setSessionTestClock(1000);

    const cfg = SessionConfig{
        secret: "prod-session-secret-key",
        cookie_name: "cp_sid",
        ttl_secs: 600,
        secure: true,
        http_only: true,
        same_site: "Strict",
    };

    let app = App.init();
    * = registerSession(*app, cfg);
    * = app.get("/profile", profileHandler);

    // 1. Create signed session cookie `<id>.<hex_sig>`
    let cookie_buf: [128]u8 = [0; 128];
    const clen = sessionCreate("uid=42;role=admin", 1000, cookie_buf[0..]) orelse 0;
    assertTrue(clen == 49, "signed session cookie length == 16 + 1 + 32");
    assertTrue(cookie_buf[16] == '.', "signed session cookie has dot separator");
    const signed_cookie = cookie_buf[0..clen];

    // 2. sessionGet succeeds before TTL
    const got = sessionGet(signed_cookie, 1200) orelse "";
    assertTrue(strEql(got, "uid=42;role=admin"), "sessionGet returns stored value");

    // 3. Tampered signature rejected
    let bad_cookie: [128]u8 = [0; 128];
    let i: usize = 0;
    while (i < clen) : (i += 1) {
        bad_cookie[i] = cookie_buf[i];
    }
    bad_cookie[clen - 1] = if (bad_cookie[clen - 1] == 'a') 'b' else 'a';
    assertTrue(sessionGet(bad_cookie[0..clen], 1200) == null, "tampered signature rejected");

    // 4. formatSessionSetCookie formats RFC 6265 Set-Cookie header
    let set_cookie_buf: [256]u8 = [0; 256];
    const sclen = formatSessionSetCookie(cfg, signed_cookie, set_cookie_buf[0..]) orelse 0;
    assertTrue(sclen > 0, "formatSessionSetCookie non-empty");
    const sc = set_cookie_buf[0..sclen];
    assertTrue(strContains(sc, "cp_sid="), "Set-Cookie contains cookie name");
    assertTrue(strContains(sc, "Max-Age=600"), "Set-Cookie contains Max-Age");
    assertTrue(strContains(sc, "HttpOnly"), "Set-Cookie contains HttpOnly");
    assertTrue(strContains(sc, "Secure"), "Set-Cookie contains Secure");
    assertTrue(strContains(sc, "SameSite=Strict"), "Set-Cookie contains SameSite");

    // 5. App dispatch without cookie -> 401
    const r_anon = app.dispatch(Request.init(AppMethod.GET, "/profile"));
    assertTrue(r_anon.status == 401, "missing cookie returns 401");

    // 6. App dispatch with valid cookie -> 200 + derived session_id & session_val
    let hdr_buf: [200]u8 = [0; 200];
    const prefix = "cp_sid=";
    i = 0;
    while (i < prefix.len) : (i += 1) {
        hdr_buf[i] = prefix[i];
    }
    let j: usize = 0;
    while (j < clen) : (j += 1) {
        hdr_buf[prefix.len + j] = cookie_buf[j];
    }
    let req_ok = Request.init(AppMethod.GET, "/profile");
    * = req_ok.withHeader("Cookie", hdr_buf[0 .. prefix.len + clen]);
    setSessionTestClock(1300);
    const r_ok = app.dispatch(req_ok);
    assertTrue(r_ok.status == 200, "valid session cookie returns 200");
    assertTrue(strEql(r_ok.body, "{\"ok\":true,\"user\":\"uid=42;role=admin\"}"), "derived session_val matched");

    // 7. TTL expiration -> rejected
    setSessionTestClock(1700);
    const r_exp = app.dispatch(req_ok);
    assertTrue(r_exp.status == 401, "expired session returns 401");
    assertTrue(sessionGet(signed_cookie, 1700) == null, "expired session evicted");

    // 8. sessionDestroy removes active session
    setSessionTestClock(2000);
    const clen2 = sessionCreate("uid=99", 2000, cookie_buf[0..]) orelse 0;
    assertTrue(clen2 == 49, "second session created");
    assertTrue(strEql(sessionGet(cookie_buf[0..clen2], 2010) orelse "", "uid=99"), "second session readable");
    assertTrue(sessionDestroy(cookie_buf[0..clen2]), "sessionDestroy returns true");
    assertTrue(sessionGet(cookie_buf[0..clen2], 2010) == null, "destroyed session returns null");

    // 9. createSessionPlugin works standalone
    let app2 = App.init();
    const plug = createSessionPlugin(cfg);
    * = app2.usePlugin(&plug);
    * = app2.get("/check", profileHandler);
    const r2 = app2.dispatch(Request.init(AppMethod.GET, "/check"));
    assertTrue(r2.status == 401, "createSessionPlugin attaches hook");

    std.debug.print("PASS: @web/session all tests passed!\n", .{});
}
