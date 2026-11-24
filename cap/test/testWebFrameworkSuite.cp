// Source: cp/cap/test/testWebFrameworkSuite.cp (End-to-End Suite & Benchmark for CP v1.0 Flagship REST Framework)

import { test, expect_eq } from "cap:test";
import {
    App,
    AppMethod,
    Request,
    Response,
    Context,
    RouteGroup,
} from "../../lib/web/app.cp";

fn containsSubstr(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (std.mem.eql(u8, haystack[i .. i + needle.len], needle)) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Shared Handlers & Middlewares for E2E Tests
// ============================================================================

struct AppMetrics {
    req_count: u64 = 0;
    auth_blocked: u64 = 0;
}

fn handleHealth(ctx: *Context): Response {
    return ctx.json(200, "{\"status\":\"ok\"}");
}

fn handleCreateItem(ctx: *Context): Response {
    if (ctx.body.len == 0) {
        return ctx.json(400, "{\"error\":\"empty body\"}");
    }
    return ctx.json(201, ctx.body);
}

fn handleUpdateItem(ctx: *Context): Response {
    if (id := ctx.param("id")) {
        return ctx.text(200, id);
    }
    return ctx.status(400);
}

fn handleDeleteItem(ctx: *Context): Response {
    if (ctx.param("id") != null) {
        return ctx.noContent();
    }
    return ctx.status(400);
}

fn handlePatchItem(ctx: *Context): Response {
    return ctx.json(200, "{\"patched\":true}");
}

fn handleUserPost(ctx: *Context): Response {
    const uid = ctx.param("id") ?? "none";
    const pid = ctx.param("postId") ?? "none";
    if (std.mem.eql(u8, uid, "42") && std.mem.eql(u8, pid, "99")) {
        const uid_num = ctx.paramU64("id") ?? 0;
        const pid_num = ctx.paramU64("postId") ?? 0;
        if (uid_num == 42 && pid_num == 99) {
            return ctx.json(200, "{\"user\":42,\"post\":99}");
        }
    }
    return ctx.json(400, "{\"error\":\"param mismatch\"}");
}

fn handleWildcardFiles(ctx: *Context): Response {
    const fpath = ctx.param("filepath") ?? "";
    return ctx.text(200, fpath);
}

fn handleSearch(ctx: *Context): Response {
    const page = ctx.queryU64("page") ?? 0;
    const limit = ctx.queryU64("limit") ?? 0;
    const tag = ctx.query("tag") ?? "";
    const req_id = ctx.header("X-Request-Id") ?? "";
    if (page == 2 && limit == 10 && std.mem.eql(u8, tag, "rust") && std.mem.eql(u8, req_id, "req-abc")) {
        return ctx.json(200, "{\"page\":2,\"limit\":10,\"tag\":\"rust\"}");
    }
    return ctx.json(400, "{\"error\":\"query mismatch\"}");
}

fn handleV1Users(ctx: *Context): Response {
    return ctx.json(200, "[{\"id\":1}]");
}

fn handleV1AdminAudit(ctx: *Context): Response {
    return ctx.json(200, "{\"audit\":\"enabled\"}");
}

fn handleStateProbe(ctx: *Context): Response {
    const addr = ctx.stateAddr();
    if (addr == 0) {
        return ctx.json(500, "{\"error\":\"no state\"}");
    }
    return ctx.json(200, "{\"state\":\"attached\"}");
}

fn mwCors(ctx: *Context): bool {
    * = ctx.setHeader("Access-Control-Allow-Origin", "*");
    * = ctx.setHeader("X-Powered-By", "CP-Web-v1.0");
    return true;
}

fn mwAuthBearer(ctx: *Context): bool {
    if (std.mem.eql(u8, ctx.path, "/health")) {
        return true;
    }
    if (auth := ctx.header("Authorization")) {
        if (std.mem.eql(u8, auth, "Bearer secret-token-123")) {
            return true;
        }
    }
    ctx.abort(ctx.json(401, "{\"error\":\"Unauthorized\"}"));
    return false;
}

fn mwCounter(ctx: *Context): bool {
    const addr = ctx.stateAddr();
    if (addr != 0) {
        unsafe {
            const metrics: *AppMetrics = @ptrFromInt(addr);
            metrics.req_count += 1;
        }
    }
    return true;
}

// ============================================================================
// Test Suite
// ============================================================================

fn main() {
    test("1. web_app: fluent chainable GET/POST/PUT/DELETE/PATCH routing", () => {
        let app = App.init();
        * = app
            .get("/health", handleHealth)
            .post("/items", handleCreateItem)
            .put("/items/:id", handleUpdateItem)
            .delete("/items/:id", handleDeleteItem)
            .patch("/items/:id", handlePatchItem);

        expect_eq(app.routeCount(), 5 as usize);

        const r_get = app.dispatch(Request.init(AppMethod.GET, "/health"));
        expect_eq(r_get.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_get.body, "{\"status\":\"ok\"}"), true);
        expect_eq(std.mem.eql(u8, r_get.content_type, "application/json; charset=utf-8"), true);

        let post_req = Request.init(AppMethod.POST, "/items");
        * = post_req.withBody("{\"name\":\"widget\"}");
        const r_post = app.dispatch(post_req);
        expect_eq(r_post.status, 201 as u16);
        expect_eq(std.mem.eql(u8, r_post.body, "{\"name\":\"widget\"}"), true);

        const r_put = app.dispatch(Request.init(AppMethod.PUT, "/items/item-77"));
        expect_eq(r_put.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_put.body, "item-77"), true);

        const r_del = app.dispatch(Request.init(AppMethod.DELETE, "/items/item-77"));
        expect_eq(r_del.status, 204 as u16);
        expect_eq(r_del.body.len, 0 as usize);

        const r_patch = app.dispatch(Request.init(AppMethod.PATCH, "/items/item-77"));
        expect_eq(r_patch.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_patch.body, "{\"patched\":true}"), true);
    });

    test("2. web_app: multi-param :id/:postId, *wildcard, query ?page=2&limit=10 & header extraction", () => {
        let app = App.init();
        * = app
            .get("/users/:id/posts/:postId", handleUserPost)
            .get("/files/*filepath", handleWildcardFiles)
            .get("/search", handleSearch);

        // Multi-param extraction + typed paramU64
        const r_mp = app.dispatch(Request.init(AppMethod.GET, "/users/42/posts/99"));
        expect_eq(r_mp.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_mp.body, "{\"user\":42,\"post\":99}"), true);

        // Wildcard extraction
        const r_wc = app.dispatch(Request.init(AppMethod.GET, "/files/docs/2026/guide.md"));
        expect_eq(r_wc.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_wc.body, "docs/2026/guide.md"), true);

        // Query params + case-insensitive header extraction
        let q_req = Request.init(AppMethod.GET, "/search?page=2&limit=10&tag=rust");
        * = q_req.withHeader("x-request-id", "req-abc");
        const r_q = app.dispatch(q_req);
        expect_eq(r_q.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_q.body, "{\"page\":2,\"limit\":10,\"tag\":\"rust\"}"), true);
    });

    test("3. web_app: scoped route groups /api/v1 and nested /api/v1/admin", () => {
        let app = App.init();
        let v1 = app.group("/api/v1");
        * = v1
            .get("/users", handleV1Users)
            .post("/users", handleCreateItem);

        let admin = v1.group("/admin");
        * = admin.get("/audit", handleV1AdminAudit);

        const r_u = app.dispatch(Request.init(AppMethod.GET, "/api/v1/users"));
        expect_eq(r_u.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_u.body, "[{\"id\":1}]"), true);

        const r_a = app.dispatch(Request.init(AppMethod.GET, "/api/v1/admin/audit"));
        expect_eq(r_a.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_a.body, "{\"audit\":\"enabled\"}"), true);
    });

    test("4. web_app: Auth/CORS/Counter middleware chain short-circuit, pass-through & state(ptr)", () => {
        let metrics = AppMetrics{ req_count: 0, auth_blocked: 0 };
        let metrics_addr: usize = 0;
        unsafe {
            metrics_addr = @intFromPtr(&metrics);
        }
        let app = App.init();
        * = app
            .state(metrics_addr)
            .use(mwCounter)
            .use(mwCors)
            .use(mwAuthBearer)
            .get("/health", handleHealth)
            .get("/private/state", handleStateProbe);

        expect_eq(app.middlewareCount(), 3 as usize);

        // 1. Whitelisted /health bypasses auth and gets CORS headers
        const r_pub = app.dispatch(Request.init(AppMethod.GET, "/health"));
        expect_eq(r_pub.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_pub.header("Access-Control-Allow-Origin").?, "*"), true);
        expect_eq(std.mem.eql(u8, r_pub.header("X-Powered-By").?, "CP-Web-v1.0"), true);

        // 2. Protected route without token -> 401 short-circuit (with CORS headers preserved)
        const r_unauth = app.dispatch(Request.init(AppMethod.GET, "/private/state"));
        expect_eq(r_unauth.status, 401 as u16);
        expect_eq(std.mem.eql(u8, r_unauth.body, "{\"error\":\"Unauthorized\"}"), true);
        expect_eq(std.mem.eql(u8, r_unauth.header("Access-Control-Allow-Origin").?, "*"), true);

        // 3. Protected route with valid Bearer token -> 200 + state attached
        let auth_req = Request.init(AppMethod.GET, "/private/state");
        * = auth_req.withHeader("authorization", "Bearer secret-token-123");
        const r_ok = app.dispatch(auth_req);
        expect_eq(r_ok.status, 200 as u16);
        expect_eq(std.mem.eql(u8, r_ok.body, "{\"state\":\"attached\"}"), true);
        expect_eq(metrics.req_count, 3 as u64);
    });

    test("5. web_app: 404 Not Found vs 405 Method Not Allowed distinction", () => {
        let app = App.init();
        * = app
            .get("/articles/:id", handleUpdateItem)
            .delete("/articles/:id", handleDeleteItem);

        // Non-existent path -> 404
        const r_404 = app.dispatch(Request.init(AppMethod.GET, "/does-not-exist"));
        expect_eq(r_404.status, 404 as u16);

        // Existing path with unregistered method -> 405 + Allow header
        const r_405 = app.dispatch(Request.init(AppMethod.POST, "/articles/123"));
        expect_eq(r_405.status, 405 as u16);
        const allow = r_405.header("Allow").?;
        expect_eq(containsSubstr(allow, "GET"), true);
        expect_eq(containsSubstr(allow, "DELETE"), true);
    });

    test("6. web_app: automatic OpenAPI 3.1.0 JSON spec generation", () => {
        let app = App.init();
        * = app
            .routeDoc(AppMethod.GET, "/health", "Liveness probe", handleHealth)
            .get("/api/v1/users/:id", handleUpdateItem)
            .delete("/api/v1/users/:id", handleDeleteItem)
            .get("/files/*filepath", handleWildcardFiles);

        let out_buf: [4096]u8 = [0; 4096];
        const written = app.toOpenApiJson("CP Flagship API", "1.0.0", out_buf[0..]).?;
        const doc = out_buf[0..written];

        expect_eq(containsSubstr(doc, "\"openapi\":\"3.1.0\""), true);
        expect_eq(containsSubstr(doc, "\"title\":\"CP Flagship API\""), true);
        expect_eq(containsSubstr(doc, "\"version\":\"1.0.0\""), true);
        expect_eq(containsSubstr(doc, "\"/health\":{\"get\":{\"summary\":\"Liveness probe\""), true);
        expect_eq(containsSubstr(doc, "\"/api/v1/users/{id}\":{"), true);
        expect_eq(containsSubstr(doc, "\"name\":\"id\",\"in\":\"path\",\"required\":true"), true);
        expect_eq(containsSubstr(doc, "\"delete\":{"), true);
        expect_eq(containsSubstr(doc, "\"/files/{filepath}\":{"), true);

        // Buffer-too-small safety check
        let tiny: [16]u8 = [0; 16];
        expect_eq(app.toOpenApiJson("Tiny", "1.0", tiny[0..]) == null, true);
    });

    test("7. web_app: 100,000 route dispatch micro-benchmark", () => {
        let app = App.init();
        * = app
            .use(mwCors)
            .get("/health", handleHealth)
            .get("/api/v1/users", handleV1Users)
            .get("/users/:id/posts/:postId", handleUserPost)
            .get("/files/*filepath", handleWildcardFiles)
            .post("/items", handleCreateItem);

        const req = Request.init(AppMethod.GET, "/users/42/posts/99?page=2&limit=10");
        const iters: usize = 100000;

        let ts0: std.os.linux.timespec = .{ sec: 0, nsec: 0 };
        let ts1: std.os.linux.timespec = .{ sec: 0, nsec: 0 };
        * = std.os.linux.clock_gettime(std.os.linux.CLOCK.MONOTONIC, &ts0);

        let ok_count: usize = 0;
        let i: usize = 0;
        while (i < iters) : (i += 1) {
            const resp = app.dispatch(req);
            if (resp.status == 200) {
                ok_count += 1;
            }
        }

        * = std.os.linux.clock_gettime(std.os.linux.CLOCK.MONOTONIC, &ts1);
        const t0_ns: u128 = ((ts0.sec as u128) * 1000000000) + (ts0.nsec as u128);
        const t1_ns: u128 = ((ts1.sec as u128) * 1000000000) + (ts1.nsec as u128);
        const elapsed_ns: u128 = if (t1_ns > t0_ns) (t1_ns - t0_ns) else 1;
        const req_per_sec: u128 = ((iters as u128) * 1000000000) / elapsed_ns;

        std.debug.print("\n[BENCH] web_app dispatch: {} iters in {} ns => {} req/sec\n", .{ iters, elapsed_ns, req_per_sec });
        expect_eq(ok_count, iters);
    });
}
