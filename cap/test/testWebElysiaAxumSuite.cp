// Comprehensive E2E Test Suite for Deepened `cap:web` (Elysia 8-Stage Lifecycle + Axum Composition + RadixRouter + RFC 6265 Cookies)
import {
    App,
    AppMethod,
    AppErrorCode,
    Context,
    Request,
    Response,
    Plugin,
    PluginScope,
    CookieOptions,
    CookieSameSite,
    formatSetCookie,
} from "../../lib/web/app.cp";

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// --- Lifecycle & Plugin Hooks ---

fn hookOnRequest(ctx: *Context): bool {
    * = ctx.setHeader("X-Stage-1", "onRequest");
    return true;
}

fn hookDerive(ctx: *Context): bool {
    if (tok := ctx.bearerToken()) {
        * = ctx.setDerived("auth_user", tok);
    } else {
        * = ctx.setDerived("auth_user", "guest");
    }
    return true;
}

fn hookBeforeHandle(ctx: *Context): bool {
    if (ctx.header("X-Force-Fail") != null) {
        ctx.fail(AppErrorCode.Unauthorized, "{\"code\":\"UNAUTHORIZED\"}");
        return false;
    }
    return true;
}

fn hookAfterHandle(ctx: *Context, resp: *Response) {
    * = ctx;
    * = resp.setHeader("X-Stage-After", "onAfterHandle");
}

fn hookMapResponse(ctx: *Context, resp: *Response) {
    * = ctx;
    * = resp.setHeader("X-Powered-By", "CP-Web-1.0");
}

fn hookOnError(ctx: *Context, code: AppErrorCode, msg: string): Response {
    * = ctx;
    if (code == AppErrorCode.Unauthorized) {
        return Response.json(401, msg);
    }
    if (code == AppErrorCode.NotFound) {
        return Response.json(404, "{\"handled_by\":\"onError\",\"code\":\"NOT_FOUND\"}");
    }
    return Response.json(code.defaultStatus(), msg);
}

// --- Handlers ---

fn handleHealth(ctx: *Context): Response {
    if (env := ctx.stateVal("env")) {
        if (strEql(env, "prod")) {
            return ctx.json(200, "{\"ok\":true,\"env\":\"prod\"}");
        }
    }
    return ctx.json(200, "{\"ok\":true}");
}

fn handleAxumBraceParam(ctx: *Context): Response {
    const id = ctx.paramI64("id") ?? -999;
    const active = ctx.queryBool("active") ?? false;
    const mp = ctx.matchedPath();
    if (id == -42 && active && strEql(mp, "/items/{id}")) {
        return ctx.json(200, "{\"id\":-42,\"active\":true}");
    }
    return ctx.json(400, "{\"bad\":true}");
}

fn handleCookieEcho(ctx: *Context): Response {
    if (sid := ctx.cookie("session_id")) {
        if (strEql(sid, "tok_xyz_123")) {
            return ctx.json(200, "{\"cookie\":\"ok\"}");
        }
    }
    return ctx.json(401, "{\"cookie\":\"missing\"}");
}

fn handleDerivedUser(ctx: *Context): Response {
    if (u := ctx.derived("auth_user")) {
        if (strEql(u, "alice-jwt")) {
            return ctx.json(200, "{\"user\":\"alice-jwt\"}");
        }
    }
    return ctx.json(200, "{\"user\":\"guest\"}");
}

fn handlePluginPing(ctx: *Context): Response {
    if (ver := ctx.stateVal("plugin_ver")) {
        if (strEql(ver, "2.0")) {
            return ctx.json(200, "{\"plugin\":\"pong\"}");
        }
    }
    return ctx.json(500, "{\"plugin\":\"no_state\"}");
}

fn handleNestedBilling(ctx: *Context): Response {
    if (inv := ctx.param("invoice_id")) {
        if (strEql(inv, "INV-900")) {
            return ctx.json(200, "{\"invoice\":\"INV-900\"}");
        }
    }
    return ctx.json(400, "{\"invoice\":\"bad\"}");
}

fn handleMergedMetrics(ctx: *Context): Response {
    return ctx.text(200, "cp_up 1");
}

fn handleCustomFallback(ctx: *Context): Response {
    return ctx.json(404, "{\"fallback\":true}");
}

fn main() {
    let app = App.init();
    * = app
        .setState("env", "prod")
        .onRequest(hookOnRequest)
        .derive(hookDerive)
        .onBeforeHandle(hookBeforeHandle)
        .onAfterHandle(hookAfterHandle)
        .mapResponse(hookMapResponse)
        .onError(hookOnError)
        .get("/health", handleHealth)
        .get("/items/{id}", handleAxumBraceParam)
        .get("/cookie", handleCookieEcho)
        .get("/me", handleDerivedUser);

    // 1. Elysia Plugin composition
    let plug = Plugin.init("billing-plug");
    * = plug
        .withPrefix("/api/plug")
        .withScope(PluginScope.Global)
        .setState("plugin_ver", "2.0")
        .get("/ping", handlePluginPing);
    * = app.usePlugin(*plug);

    // 2. Axum Router::nest & Router::merge
    let sub_billing = App.init();
    * = sub_billing.get("/invoices/:invoice_id", handleNestedBilling);
    * = app.nest("/billing", *sub_billing);

    let sub_ops = App.init();
    * = sub_ops.get("/metrics", handleMergedMetrics);
    * = app.merge(*sub_ops);

    // Test 1: O(1) static route + stateVal + 8-stage lifecycle headers
    let r1 = app.dispatch(Request.init(AppMethod.GET, "/health"));
    if (r1.status != 200 || !strEql(r1.body, "{\"ok\":true,\"env\":\"prod\"}")) {
        @panic("Test 1 failed: static route + state");
    }
    if (h1 := r1.header("X-Stage-1")) {
        if (!strEql(h1, "onRequest")) @panic("Test 1 failed: X-Stage-1");
    } else {
        @panic("Test 1 failed: missing X-Stage-1");
    }
    if (h2 := r1.header("X-Powered-By")) {
        if (!strEql(h2, "CP-Web-1.0")) @panic("Test 1 failed: X-Powered-By");
    } else {
        @panic("Test 1 failed: missing X-Powered-By");
    }

    // Test 2: Axum HEAD -> GET automatic fallback (status 200, empty body)
    let r2 = app.dispatch(Request.init(AppMethod.HEAD, "/health"));
    if (r2.status != 200 || r2.body.len != 0) {
        @panic("Test 2 failed: HEAD->GET fallback");
    }

    // Test 3: Axum `{id}` brace param + paramI64(-42) + queryBool + matchedPath
    let r3 = app.dispatch(Request.init(AppMethod.GET, "/items/-42?active=true"));
    if (r3.status != 200 || !strEql(r3.body, "{\"id\":-42,\"active\":true}")) {
        @panic("Test 3 failed: Axum brace param & extractors");
    }

    // Test 4: RFC 6265 Cookie extraction & Set-Cookie formatting
    let req4 = Request.init(AppMethod.GET, "/cookie");
    * = req4.withHeader("Cookie", "theme=dark; session_id=\"tok_xyz_123\"; lang=en");
    let r4 = app.dispatch(req4);
    if (r4.status != 200 || !strEql(r4.body, "{\"cookie\":\"ok\"}")) {
        @panic("Test 4 failed: ctx.cookie()");
    }
    let cookie_buf: [256]u8 = [0; 256];
    let opts = CookieOptions{
        path: "/api",
        domain: "cp.dev",
        max_age: 3600,
        http_only: true,
        secure: true,
        same_site: CookieSameSite.Strict,
    };
    if (clen := formatSetCookie("sid", "abc", opts, cookie_buf[0..])) {
        if (!strEql(cookie_buf[0..clen], "sid=abc; Path=/api; Domain=cp.dev; Max-Age=3600; HttpOnly; Secure; SameSite=Strict")) {
            @panic("Test 4 failed: formatSetCookie mismatch");
        }
    } else {
        @panic("Test 4 failed: formatSetCookie returned null");
    }

    // Test 5: Elysia derive() + bearerToken()
    let req5 = Request.init(AppMethod.GET, "/me");
    * = req5.withHeader("Authorization", "Bearer alice-jwt");
    let r5 = app.dispatch(req5);
    if (r5.status != 200 || !strEql(r5.body, "{\"user\":\"alice-jwt\"}")) {
        @panic("Test 5 failed: derive + bearerToken");
    }

    // Test 6: Elysia onError + ctx.fail() short-circuit
    let req6 = Request.init(AppMethod.GET, "/me");
    * = req6.withHeader("X-Force-Fail", "1");
    let r6 = app.dispatch(req6);
    if (r6.status != 401 || !strEql(r6.body, "{\"code\":\"UNAUTHORIZED\"}")) {
        @panic("Test 6 failed: onError short-circuit");
    }

    // Test 7: Plugin + Axum nest + Axum merge + custom fallback
    let r7a = app.dispatch(Request.init(AppMethod.GET, "/api/plug/ping"));
    if (r7a.status != 200 || !strEql(r7a.body, "{\"plugin\":\"pong\"}")) {
        @panic("Test 7a failed: Plugin mount");
    }
    let r7b = app.dispatch(Request.init(AppMethod.GET, "/billing/invoices/INV-900"));
    if (r7b.status != 200 || !strEql(r7b.body, "{\"invoice\":\"INV-900\"}")) {
        @panic("Test 7b failed: Axum nest");
    }
    let r7c = app.dispatch(Request.init(AppMethod.GET, "/metrics"));
    if (r7c.status != 200 || !strEql(r7c.body, "cp_up 1")) {
        @panic("Test 7c failed: Axum merge");
    }
    let r7d = app.dispatch(Request.init(AppMethod.GET, "/does-not-exist"));
    if (r7d.status != 404 || !strEql(r7d.body, "{\"handled_by\":\"onError\",\"code\":\"NOT_FOUND\"}")) {
        @panic("Test 7d failed: onError 404");
    }
    * = app.fallback(handleCustomFallback);
    let r7e = app.dispatch(Request.init(AppMethod.GET, "/does-not-exist"));
    if (r7e.status != 404 || !strEql(r7e.body, "{\"fallback\":true}")) {
        @panic("Test 7e failed: Axum fallback");
    }

    @print("PASS testWebElysiaAxumSuite (7/7 suites)");
}
