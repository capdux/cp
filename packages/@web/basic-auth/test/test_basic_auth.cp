// Regression Test Suite for `@web/basic-auth`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import { BasicAuthConfig, registerBasicAuth } from "../src/index.cp";

fn hAdmin(ctx: *Context): Response {
    const u = ctx.derived("auth_user") ?? "";
    if (u.len > 0) {
        return Response.json(200, "{\"ok\":true}");
    }
    return Response.json(401, "{\"ok\":false}");
}

fn main() {
    let app = App.init();
    * = registerBasicAuth(*app, BasicAuthConfig{
        username: "admin",
        password: "secret",
        api_key: "sk_live_999",
    });
    * = app.get("/admin", hAdmin);

    // 1. Missing credentials -> 401 + WWW-Authenticate
    const r1 = app.dispatch(Request.init(AppMethod.GET, "/admin"));
    if (r1.status != 401 || (r1.header("WWW-Authenticate") ?? "").len == 0) {
        @panic("Test 1 failed: expected 401 + WWW-Authenticate");
    }

    // 2. Valid Basic auth ("admin:secret" -> "YWRtaW46c2VjcmV0") -> 200
    let r2_req = Request.init(AppMethod.GET, "/admin");
    * = r2_req.withHeader("Authorization", "Basic YWRtaW46c2VjcmV0");
    if (app.dispatch(r2_req).status != 200) {
        @panic("Test 2 failed: valid Basic auth rejected");
    }

    // 3. Wrong Basic auth ("admin:wrong" -> "YWRtaW46d3Jvbmc=") -> 401
    let r3_req = Request.init(AppMethod.GET, "/admin");
    * = r3_req.withHeader("Authorization", "Basic YWRtaW46d3Jvbmc=");
    if (app.dispatch(r3_req).status != 401) {
        @panic("Test 3 failed: wrong Basic auth accepted");
    }

    // 4. Valid X-API-Key -> 200
    let r4_req = Request.init(AppMethod.GET, "/admin");
    * = r4_req.withHeader("X-API-Key", "sk_live_999");
    if (app.dispatch(r4_req).status != 200) {
        @panic("Test 4 failed: valid X-API-Key rejected");
    }

    @print("PASS @web/basic-auth (4/4 tests)");
}
