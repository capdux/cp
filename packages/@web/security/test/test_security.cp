// Regression Test Suite for `@web/security`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import { SecurityConfig, registerSecurity, constantTimeEql } from "../src/index.cp";

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn hRead(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"ok\":true}");
}

fn hWrite(ctx: *Context): Response {
    * = ctx;
    return Response.json(201, "{\"created\":true}");
}

fn main() {
    // 1. Constant-time comparison check
    if (!constantTimeEql("abcdef0123456789", "abcdef0123456789")) {
        @panic("Test 1a failed");
    }
    if (constantTimeEql("abcdef0123456789", "abcdef0123456780")) {
        @panic("Test 1b failed");
    }

    // 2. Register plugin with Helmet headers + CSRF enabled
    let app = App.init();
    * = registerSecurity(*app, SecurityConfig{ enable_csrf: true });
    * = app.get("/items", hRead).post("/items", hWrite);

    // 3. Safe GET passes without CSRF and receives full Helmet headers
    const get_resp = app.dispatch(Request.init(AppMethod.GET, "/items"));
    if (get_resp.status != 200) @panic("Test 3a failed: expected 200");
    if (!strEql(get_resp.header("X-Content-Type-Options") ?? "", "nosniff")) {
        @panic("Test 3b failed: missing X-Content-Type-Options");
    }
    if (!strEql(get_resp.header("X-Frame-Options") ?? "", "DENY")) {
        @panic("Test 3c failed: missing X-Frame-Options");
    }
    if (!strEql(get_resp.header("Content-Security-Policy") ?? "", "default-src 'self'")) {
        @panic("Test 3d failed: missing CSP");
    }
    if ((get_resp.header("Strict-Transport-Security") ?? "").len == 0) {
        @panic("Test 3e failed: missing HSTS");
    }

    // 4. POST without CSRF token is rejected with 403
    const bad_post = app.dispatch(Request.init(AppMethod.POST, "/items"));
    if (bad_post.status != 403) @panic("Test 4 failed: expected 403 without CSRF");

    // 5. POST with mismatched CSRF token is rejected with 403
    let mismatch_req = Request.init(AppMethod.POST, "/items");
    * = mismatch_req
        .withHeader("Cookie", "csrf_token=tok_0123456789abcdef")
        .withHeader("X-CSRF-Token", "tok_0123456789abcde0");
    if (app.dispatch(mismatch_req).status != 403) {
        @panic("Test 5 failed: expected 403 on mismatched CSRF");
    }

    // 6. POST with matching Double-Submit Cookie + Header passes with 201 + Helmet headers
    let ok_req = Request.init(AppMethod.POST, "/items");
    * = ok_req
        .withHeader("Cookie", "sid=1; csrf_token=tok_0123456789abcdef")
        .withHeader("X-CSRF-Token", "tok_0123456789abcdef");
    const ok_resp = app.dispatch(ok_req);
    if (ok_resp.status != 201) @panic("Test 6a failed: expected 201");
    if (!strEql(ok_resp.header("X-Frame-Options") ?? "", "DENY")) {
        @panic("Test 6b failed: Helmet header missing on 201");
    }

    @print("PASS @web/security (6/6 tests)");
}
