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

// --- New Elysia & Axum Gap Coverage Handlers & Guards ---

fn handleRedirectTemp(ctx: *Context): Response {
    * = ctx;
    return Response.redirectTemporary("/login");
}

fn handleRedirectPerm(ctx: *Context): Response {
    * = ctx;
    return Response.redirectPermanent("https://cp-lang.dev/docs");
}

fn handleRedirectCustom(ctx: *Context): Response {
    * = ctx;
    return Response.redirect(307, "/v2/submit");
}

fn handleHtmlPage(ctx: *Context): Response {
    return ctx.html(200, "<h1>Hello CP</h1>");
}

fn handleBinaryBytes(ctx: *Context): Response {
    return Response.bytes(200, "\x89PNG\r\n", "image/png");
}

fn handleFormSubmit(ctx: *Context): Response {
    const user = ctx.form("user") ?? "";
    const role = ctx.form("role") ?? "";
    const flag = ctx.form("remember") ?? "missing";
    const absent = ctx.form("nope");
    if (strEql(user, "alice") && strEql(role, "admin") && strEql(flag, "") && absent == null) {
        return ctx.json(200, "{\"form\":\"ok\"}");
    }
    return ctx.json(400, "{\"form\":\"bad\"}");
}

fn handleMultipartCheck(ctx: *Context): Response {
    if (!ctx.isMultipart()) {
        return ctx.json(415, "{\"multipart\":false}");
    }
    if (b := ctx.multipartBoundary()) {
        return ctx.text(200, b);
    }
    return ctx.json(400, "{\"multipart\":\"no_boundary\"}");
}

fn guardAdminBefore(ctx: *Context): bool {
    if (key := ctx.header("X-Admin-Key")) {
        if (strEql(key, "s3cr3t")) {
            * = ctx.setHeader("X-Guard-Before", "passed");
            return true;
        }
    }
    ctx.abort(Response.json(403, "{\"guard\":\"denied\"}"));
    return false;
}

fn guardAdminAfter(ctx: *Context, resp: *Response) {
    * = ctx;
    * = resp.setHeader("X-Guard-After", "audited");
}

fn handleSecretPanel(ctx: *Context): Response {
    return ctx.json(200, "{\"panel\":\"secret\"}");
}

fn handlePublicPing(ctx: *Context): Response {
    * = ctx.setHeader("X-Handler-Hdr", "yes");
    return ctx.json(200, "{\"pong\":true}");
}

fn handleFastParam(ctx: *Context): Response {
    if (id := ctx.param("id")) {
        return ctx.text(200, id);
    }
    return ctx.status(400);
}

fn noopOnRequest(ctx: *Context): bool {
    * = ctx;
    return true;
}

fn runTestResponseUtilsAndForms() {
    // Test 8: Response.redirect / redirectTemporary / redirectPermanent / html / bytes
    let util_app = App.init();
    * = util_app
        .get("/old", handleRedirectTemp)
        .get("/moved", handleRedirectPerm)
        .post("/submit", handleRedirectCustom)
        .get("/page", handleHtmlPage)
        .get("/logo.png", handleBinaryBytes)
        .post("/form", handleFormSubmit)
        .post("/upload", handleMultipartCheck);

    let r8a = util_app.dispatch(Request.init(AppMethod.GET, "/old"));
    if (r8a.status != 302 || r8a.body.len != 0) @panic("Test 8a failed: redirectTemporary status/body");
    if (loc := r8a.header("Location")) {
        if (!strEql(loc, "/login")) @panic("Test 8a failed: Location mismatch");
    } else {
        @panic("Test 8a failed: missing Location");
    }

    let r8b = util_app.dispatch(Request.init(AppMethod.GET, "/moved"));
    if (r8b.status != 308 || r8b.body.len != 0) @panic("Test 8b failed: redirectPermanent status/body");
    if (loc := r8b.header("Location")) {
        if (!strEql(loc, "https://cp-lang.dev/docs")) @panic("Test 8b failed: Location mismatch");
    } else {
        @panic("Test 8b failed: missing Location");
    }

    let r8c = util_app.dispatch(Request.init(AppMethod.POST, "/submit"));
    if (r8c.status != 307 || r8c.body.len != 0) @panic("Test 8c failed: redirect(307) status/body");
    if (loc := r8c.header("Location")) {
        if (!strEql(loc, "/v2/submit")) @panic("Test 8c failed: Location mismatch");
    } else {
        @panic("Test 8c failed: missing Location");
    }

    let r8d = util_app.dispatch(Request.init(AppMethod.GET, "/page"));
    if (r8d.status != 200 || !strEql(r8d.content_type, "text/html; charset=utf-8") || !strEql(r8d.body, "<h1>Hello CP</h1>")) {
        @panic("Test 8d failed: Response.html");
    }

    let r8e = util_app.dispatch(Request.init(AppMethod.GET, "/logo.png"));
    if (r8e.status != 200 || !strEql(r8e.content_type, "image/png") || !strEql(r8e.body, "\x89PNG\r\n")) {
        @panic("Test 8e failed: Response.bytes");
    }

    // Test 9: Context.form() & Context.multipartBoundary() / isMultipart()
    let req9a = Request.init(AppMethod.POST, "/form");
    * = req9a.withHeader("Content-Type", "application/x-www-form-urlencoded");
    * = req9a.withBody("user=alice&role=admin&remember");
    let r9a = util_app.dispatch(req9a);
    if (r9a.status != 200 || !strEql(r9a.body, "{\"form\":\"ok\"}")) {
        @panic("Test 9a failed: ctx.form()");
    }

    let req9b = Request.init(AppMethod.POST, "/upload");
    * = req9b.withHeader("Content-Type", "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxk");
    let r9b = util_app.dispatch(req9b);
    if (r9b.status != 200 || !strEql(r9b.body, "----WebKitFormBoundary7MA4YWxk")) {
        @panic("Test 9b failed: ctx.multipartBoundary() unquoted");
    }

    let req9c = Request.init(AppMethod.POST, "/upload");
    * = req9c.withHeader("Content-Type", "multipart/form-data; charset=utf-8; boundary=\"----QuotedBoundary123\"");
    let r9c = util_app.dispatch(req9c);
    if (r9c.status != 200 || !strEql(r9c.body, "----QuotedBoundary123")) {
        @panic("Test 9c failed: ctx.multipartBoundary() quoted");
    }

    let req9d = Request.init(AppMethod.POST, "/upload");
    * = req9d.withHeader("Content-Type", "application/json");
    let r9d = util_app.dispatch(req9d);
    if (r9d.status != 415 || !strEql(r9d.body, "{\"multipart\":false}")) {
        @panic("Test 9d failed: ctx.isMultipart() false case");
    }
}

fn runTestRouteAndGroupGuards() {
    // Test 10: Single-route guard / routeLayer & RouteGroup.guard (only triggers on guarded routes)
    let guard_app = App.init();
    * = guard_app
        .get("/public", handlePublicPing)
        .getWithGuard("/single-secret", handleSecretPanel, guardAdminBefore, guardAdminAfter)
        .get("/layer-secret", handleSecretPanel)
        .routeLayer(guardAdminBefore, guardAdminAfter);

    let admin_grp = guard_app.group("/admin");
    * = admin_grp
        .guard(guardAdminBefore, guardAdminAfter)
        .get("/dashboard", handleSecretPanel);

    // Nested group inherits guard
    let super_grp = admin_grp.group("/super");
    * = super_grp.get("/keys", handleSecretPanel);

    // Public route must NOT trigger route-level guard
    let r10a = guard_app.dispatch(Request.init(AppMethod.GET, "/public"));
    if (r10a.status != 200 || r10a.header("X-Guard-Before") != null || r10a.header("X-Guard-After") != null) {
        @panic("Test 10a failed: unguarded route triggered guard");
    }

    // Guarded single route without key -> 403
    let r10b = guard_app.dispatch(Request.init(AppMethod.GET, "/single-secret"));
    if (r10b.status != 403 || !strEql(r10b.body, "{\"guard\":\"denied\"}")) {
        @panic("Test 10b failed: getWithGuard deny");
    }

    // Guarded single route with key -> 200 + before/after headers
    let req10c = Request.init(AppMethod.GET, "/single-secret");
    * = req10c.withHeader("X-Admin-Key", "s3cr3t");
    let r10c = guard_app.dispatch(req10c);
    if (r10c.status != 200 || !strEql(r10c.body, "{\"panel\":\"secret\"}")) {
        @panic("Test 10c failed: getWithGuard allow");
    }
    if (hb := r10c.header("X-Guard-Before")) {
        if (!strEql(hb, "passed")) @panic("Test 10c failed: X-Guard-Before mismatch");
    } else {
        @panic("Test 10c failed: missing X-Guard-Before");
    }
    if (ha := r10c.header("X-Guard-After")) {
        if (!strEql(ha, "audited")) @panic("Test 10c failed: X-Guard-After mismatch");
    } else {
        @panic("Test 10c failed: missing X-Guard-After");
    }

    // Axum routeLayer deny & allow
    let r10d = guard_app.dispatch(Request.init(AppMethod.GET, "/layer-secret"));
    if (r10d.status != 403) @panic("Test 10d failed: routeLayer deny");
    let req10e = Request.init(AppMethod.GET, "/layer-secret");
    * = req10e.withHeader("X-Admin-Key", "s3cr3t");
    let r10e = guard_app.dispatch(req10e);
    if (r10e.status != 200 || r10e.header("X-Guard-After") == null) {
        @panic("Test 10e failed: routeLayer allow");
    }

    // RouteGroup.guard & nested group inheritance
    let r10f = guard_app.dispatch(Request.init(AppMethod.GET, "/admin/dashboard"));
    if (r10f.status != 403) @panic("Test 10f failed: RouteGroup.guard deny");
    let req10g = Request.init(AppMethod.GET, "/admin/super/keys");
    * = req10g.withHeader("X-Admin-Key", "s3cr3t");
    let r10g = guard_app.dispatch(req10g);
    if (r10g.status != 200 || r10g.header("X-Guard-After") == null) {
        @panic("Test 10g failed: nested RouteGroup.guard allow");
    }
}

fn runTestFastVsSlowConsistency() {
    // Test 11: hook_mask == 0 Zero-Hook Fast-Path vs Slow-Path Consistency
    let fast_app = App.init();
    * = fast_app
        .get("/ping", handlePublicPing)
        .get("/users/:id", handleFastParam);
    if (fast_app.hookMask() != 0) @panic("Test 11 failed: fast_app.hookMask() should be 0");

    let slow_app = App.init();
    * = slow_app
        .onRequest(noopOnRequest)
        .get("/ping", handlePublicPing)
        .get("/users/:id", handleFastParam);
    if (slow_app.hookMask() == 0) @panic("Test 11 failed: slow_app.hookMask() should be non-zero");

    // Static GET consistency (including handler ctx.setHeader merge)
    let rf1 = fast_app.dispatch(Request.init(AppMethod.GET, "/ping"));
    let rs1 = slow_app.dispatch(Request.init(AppMethod.GET, "/ping"));
    if (rf1.status != rs1.status || !strEql(rf1.body, rs1.body)) {
        @panic("Test 11a failed: fast vs slow static GET mismatch");
    }
    if (hf1 := rf1.header("X-Handler-Hdr")) {
        if (!strEql(hf1, "yes")) @panic("Test 11a failed: fast path X-Handler-Hdr");
    } else {
        @panic("Test 11a failed: fast path missing X-Handler-Hdr");
    }

    // Dynamic param GET consistency
    let rf2 = fast_app.dispatch(Request.init(AppMethod.GET, "/users/777"));
    let rs2 = slow_app.dispatch(Request.init(AppMethod.GET, "/users/777"));
    if (rf2.status != rs2.status || !strEql(rf2.body, rs2.body) || !strEql(rf2.body, "777")) {
        @panic("Test 11b failed: fast vs slow param GET mismatch");
    }

    // HEAD -> GET fallback consistency
    let rf3 = fast_app.dispatch(Request.init(AppMethod.HEAD, "/ping"));
    let rs3 = slow_app.dispatch(Request.init(AppMethod.HEAD, "/ping"));
    if (rf3.status != 200 || rs3.status != 200 || rf3.body.len != 0 || rs3.body.len != 0) {
        @panic("Test 11c failed: fast vs slow HEAD fallback mismatch");
    }

    // 405 & 404 consistency
    let rf4 = fast_app.dispatch(Request.init(AppMethod.POST, "/ping"));
    let rs4 = slow_app.dispatch(Request.init(AppMethod.POST, "/ping"));
    if (rf4.status != 405 || rs4.status != 405 || rf4.header("Allow") == null || rs4.header("Allow") == null) {
        @panic("Test 11d failed: fast vs slow 405 mismatch");
    }
    let rf5 = fast_app.dispatch(Request.init(AppMethod.GET, "/missing"));
    let rs5 = slow_app.dispatch(Request.init(AppMethod.GET, "/missing"));
    if (rf5.status != 404 || rs5.status != 404) {
        @panic("Test 11e failed: fast vs slow 404 mismatch");
    }
}

fn strContains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        let j: usize = 0;
        let matched: bool = true;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                matched = false;
                break;
            }
        }
        if (matched) return true;
    }
    return false;
}

fn runTestServerTimingTrace() {
    // Test 12: App.enableTrace(true/false) 8-stage Server-Timing header & zero-overhead toggle
    let trace_app = App.init();
    * = trace_app
        .get("/ping", handlePublicPing)
        .get("/users/:id", handleFastParam);

    // Default (disabled): zero-hook fast-path, no Server-Timing header
    if (trace_app.hookMask() != 0) @panic("Test 12a failed: default hookMask should be 0");
    let r12a = trace_app.dispatch(Request.init(AppMethod.GET, "/ping"));
    if (r12a.status != 200 || r12a.header("Server-Timing") != null) {
        @panic("Test 12a failed: Server-Timing should not be present when trace is disabled");
    }

    // Enable tracing: Server-Timing header with routing;dur= and handler;dur=
    * = trace_app.enableTrace(true);
    if (trace_app.hookMask() == 0) @panic("Test 12b failed: hookMask should be non-zero when trace is enabled");
    let r12b = trace_app.dispatch(Request.init(AppMethod.GET, "/users/42"));
    if (r12b.status != 200 || !strEql(r12b.body, "42")) {
        @panic("Test 12b failed: traced dispatch response mismatch");
    }
    if (st := r12b.header("Server-Timing")) {
        if (!strContains(st, "routing;dur=") || !strContains(st, "handler;dur=")) {
            @panic("Test 12b failed: Server-Timing missing routing/handler durations");
        }
    } else {
        @panic("Test 12b failed: missing Server-Timing header when trace is enabled");
    }

    // Disable tracing again: restores hookMask == 0 and omits Server-Timing
    * = trace_app.enableTrace(false);
    if (trace_app.hookMask() != 0) @panic("Test 12c failed: hookMask should return to 0 after enableTrace(false)");
    let r12c = trace_app.dispatch(Request.init(AppMethod.GET, "/ping"));
    if (r12c.status != 200 || r12c.header("Server-Timing") != null) {
        @panic("Test 12c failed: Server-Timing should not be present after enableTrace(false)");
    }
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

    runTestResponseUtilsAndForms();
    runTestRouteAndGroupGuards();
    runTestFastVsSlowConsistency();
    runTestServerTimingTrace();

    @print("PASS testWebElysiaAxumSuite (12/12 suites)");
}
