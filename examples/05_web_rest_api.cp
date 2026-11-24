// Example 05: Flagship Zero-Alloc REST API Framework (`cap:web`)
// Demonstrates: Elysia chainable routing + Plugin + derive + onError + Axum nest/{param} + RFC 6265 Cookie + OpenAPI 3.1.0
// Run with: cap run examples/05_web_rest_api.cp

import std from "std";
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
} from "../lib/web/app.cp";

// --- Elysia 8-Stage Lifecycle Hooks ---

fn onReqTrace(ctx: *Context): bool {
    * = ctx.setHeader("X-Framework", "cap:web");
    return true;
}

fn deriveAuth(ctx: *Context): bool {
    if (tok := ctx.bearerToken()) {
        * = ctx.setDerived("auth_user", tok);
    } else {
        * = ctx.setDerived("auth_user", "guest");
    }
    return true;
}

fn guardBeforeHandle(ctx: *Context): bool {
    if (ctx.header("X-Deny") != null) {
        ctx.fail(AppErrorCode.Unauthorized, "{\"error\":\"unauthorized\",\"reason\":\"blocked_by_guard\"}");
        return false;
    }
    return true;
}

fn afterHandleStamp(ctx: *Context, resp: *Response) {
    * = ctx;
    * = resp.setHeader("X-Lifecycle", "8-stage");
}

fn onUnifiedError(ctx: *Context, code: AppErrorCode, msg: string): Response {
    * = ctx;
    if (code == AppErrorCode.NotFound) {
        return Response.json(404, "{\"error\":\"not_found\",\"code\":\"NOT_FOUND\"}");
    }
    return Response.json(code.defaultStatus(), msg);
}

// --- Route Handlers ---

fn handleHealth(ctx: *Context): Response {
    const env = ctx.stateVal("env") ?? "dev";
    if (std.mem.eql(u8, env, "prod")) {
        return ctx.json(200, "{\"status\":\"ok\",\"env\":\"prod\"}");
    }
    return ctx.json(200, "{\"status\":\"ok\",\"env\":\"dev\"}");
}

fn handleMe(ctx: *Context): Response {
    const user = ctx.derived("auth_user") ?? "guest";
    if (std.mem.eql(u8, user, "alice-token")) {
        return ctx.json(200, "{\"user\":\"alice-token\",\"role\":\"admin\"}");
    }
    return ctx.json(200, "{\"user\":\"guest\",\"role\":\"anonymous\"}");
}

fn handlePluginStatus(ctx: *Context): Response {
    const ver = ctx.stateVal("auth_plugin_ver") ?? "0.0";
    if (std.mem.eql(u8, ver, "1.0")) {
        return ctx.json(200, "{\"plugin\":\"auth\",\"version\":\"1.0\"}");
    }
    return ctx.json(500, "{\"plugin\":\"auth\",\"error\":\"missing_state\"}");
}

fn handleNestedOrder(ctx: *Context): Response {
    const order_id = ctx.paramI64("id") ?? -1;
    const verbose = ctx.queryBool("verbose") ?? false;
    const matched = ctx.matchedPath();
    if (order_id == 42 && verbose && std.mem.eql(u8, matched, "/api/v1/orders/{id}")) {
        return ctx.json(200, "{\"order_id\":42,\"verbose\":true,\"matched\":\"/api/v1/orders/{id}\"}");
    }
    return ctx.json(400, "{\"error\":\"invalid_order_request\"}");
}

fn handleSessionCookie(ctx: *Context): Response {
    if (sid := ctx.cookie("session_id")) {
        if (std.mem.eql(u8, sid, "sess_9876")) {
            return ctx.json(200, "{\"session\":\"valid\",\"sid\":\"sess_9876\"}");
        }
    }
    return ctx.json(401, "{\"session\":\"missing\"}");
}

fn main() {
    // 1. Elysia-style chainable App setup + 8-stage lifecycle hooks
    let app = App.init();
    * = app
        .setState("env", "prod")
        .onRequest(onReqTrace)
        .derive(deriveAuth)
        .onBeforeHandle(guardBeforeHandle)
        .onAfterHandle(afterHandleStamp)
        .onError(onUnifiedError)
        .routeDoc(AppMethod.GET, "/health", "Service health check", handleHealth)
        .routeDoc(AppMethod.GET, "/me", "Current authenticated user", handleMe)
        .routeDoc(AppMethod.GET, "/session", "Verify RFC 6265 session cookie", handleSessionCookie);

    // 2. Elysia-style composable Plugin
    let auth_plugin = Plugin.init("auth-plugin");
    * = auth_plugin
        .withPrefix("/auth")
        .withScope(PluginScope.Global)
        .setState("auth_plugin_ver", "1.0")
        .route(AppMethod.GET, "/status", "Auth plugin status", handlePluginStatus);
    * = app.usePlugin(*auth_plugin);

    // 3. Axum-style Router::nest + `{param}` brace parameter syntax
    let orders_router = App.init();
    * = orders_router.routeDoc(AppMethod.GET, "/orders/{id}", "Get order by ID", handleNestedOrder);
    * = app.nest("/api/v1", *orders_router);

    // --- Zero-I/O In-Process Dispatch Verification ---

    const r_health = app.dispatch(Request.init(AppMethod.GET, "/health"));
    std.debug.print("[1] GET /health -> {d} {s} (X-Framework={s})\n", .{
        r_health.status,
        r_health.body,
        r_health.header("X-Framework") ?? "-",
    });

    let req_me = Request.init(AppMethod.GET, "/me");
    * = req_me.withHeader("Authorization", "Bearer alice-token");
    const r_me = app.dispatch(req_me);
    std.debug.print("[2] GET /me (derive) -> {d} {s}\n", .{ r_me.status, r_me.body });

    let req_deny = Request.init(AppMethod.GET, "/me");
    * = req_deny.withHeader("X-Deny", "1");
    const r_deny = app.dispatch(req_deny);
    std.debug.print("[3] GET /me (onError) -> {d} {s}\n", .{ r_deny.status, r_deny.body });

    const r_plug = app.dispatch(Request.init(AppMethod.GET, "/auth/status"));
    std.debug.print("[4] GET /auth/status (Plugin) -> {d} {s}\n", .{ r_plug.status, r_plug.body });

    const r_nest = app.dispatch(Request.init(AppMethod.GET, "/api/v1/orders/42?verbose=true"));
    std.debug.print("[5] GET /api/v1/orders/42 (Axum nest + {{id}}) -> {d} {s}\n", .{ r_nest.status, r_nest.body });

    // 4. RFC 6265 Cookie extraction & Set-Cookie formatting
    let req_cookie = Request.init(AppMethod.GET, "/session");
    * = req_cookie.withHeader("Cookie", "theme=dark; session_id=\"sess_9876\"");
    const r_cookie = app.dispatch(req_cookie);

    let set_cookie_buf: [256]u8 = [0; 256];
    const cookie_opts = CookieOptions{
        path: "/",
        domain: "api.cp.dev",
        max_age: 7200,
        http_only: true,
        secure: true,
        same_site: CookieSameSite.Strict,
    };
    const sc_len = formatSetCookie("session_id", "sess_9876", cookie_opts, set_cookie_buf[0..]) ?? 0;
    std.debug.print("[6] Cookie -> {d} {s} | Set-Cookie: {s}\n", .{
        r_cookie.status,
        r_cookie.body,
        set_cookie_buf[0..sc_len],
    });

    // 5. Automatic OpenAPI 3.1.0 specification export
    let openapi_buf: [4096]u8 = [0; 4096];
    if (oa_len := app.toOpenApiJson("CP Web Flagship API", "1.0.0", openapi_buf[0..])) {
        std.debug.print("[7] OpenAPI 3.1.0 ({d}B): {s}\n", .{ oa_len, openapi_buf[0..oa_len] });
    } else {
        @panic("Failed to export OpenAPI 3.1.0 document");
    }

    if (r_health.status != 200 || r_me.status != 200 || r_deny.status != 401 || r_plug.status != 200 || r_nest.status != 200 || r_cookie.status != 200) {
        @panic("Unexpected status in 05_web_rest_api example");
    }
}
