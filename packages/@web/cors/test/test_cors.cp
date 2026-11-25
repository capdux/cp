// Regression Test Suite for `@web/cors`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import { CorsConfig, registerCors, isOriginAllowed } from "../src/index.cp";

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn hPing(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"ok\":true}");
}

fn main() {
    // 1. Origin allowlist & CRLF injection defense
    if (!isOriginAllowed("https://app.cp.dev, https://admin.cp.dev", "https://admin.cp.dev")) {
        @panic("Test 1a failed: allowed origin rejected");
    }
    if (isOriginAllowed("https://app.cp.dev", "https://evil.example")) {
        @panic("Test 1b failed: disallowed origin accepted");
    }
    if (isOriginAllowed("*", "https://evil.com\r\nX-Injected: 1")) {
        @panic("Test 1c failed: CRLF origin accepted");
    }

    // 2. Register plugin with allowlist + credentials
    let app = App.init();
    * = registerCors(*app, CorsConfig{
        allow_origin: "https://app.cp.dev, https://docs.cp.dev",
        allow_credentials: true,
        expose_headers: "X-Request-Id",
    });
    * = app.get("/api/ping", hPing);

    // 3. Preflight OPTIONS returns 204 + Access-Control-* headers
    let pre_req = Request.init(AppMethod.OPTIONS, "/api/ping");
    * = pre_req.withHeader("Origin", "https://app.cp.dev");
    const pre_resp = app.dispatch(pre_req);
    if (pre_resp.status != 204) @panic("Test 3a failed: expected 204 for preflight");
    if (!strEql(pre_resp.header("Access-Control-Allow-Origin") ?? "", "https://app.cp.dev")) {
        @panic("Test 3b failed: ACAO header mismatch");
    }
    if (!strEql(pre_resp.header("Access-Control-Allow-Credentials") ?? "", "true")) {
        @panic("Test 3c failed: ACAC header mismatch");
    }
    if (!strEql(pre_resp.header("Vary") ?? "", "Origin")) {
        @panic("Test 3d failed: Vary header missing");
    }

    // 4. Disallowed preflight OPTIONS returns 403
    let bad_pre = Request.init(AppMethod.OPTIONS, "/api/ping");
    * = bad_pre.withHeader("Origin", "https://evil.example");
    const bad_resp = app.dispatch(bad_pre);
    if (bad_resp.status != 403) @panic("Test 4 failed: expected 403 for disallowed preflight");

    // 5. Normal GET attaches ACAO + Expose-Headers
    let get_req = Request.init(AppMethod.GET, "/api/ping");
    * = get_req.withHeader("Origin", "https://docs.cp.dev");
    const get_resp = app.dispatch(get_req);
    if (get_resp.status != 200) @panic("Test 5a failed: expected 200");
    if (!strEql(get_resp.header("Access-Control-Allow-Origin") ?? "", "https://docs.cp.dev")) {
        @panic("Test 5b failed: GET ACAO mismatch");
    }
    if (!strEql(get_resp.header("Access-Control-Expose-Headers") ?? "", "X-Request-Id")) {
        @panic("Test 5c failed: Expose-Headers mismatch");
    }

    @print("PASS @web/cors (5/5 tests)");
}
