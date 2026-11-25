// Regression Test Suite for `@web/cache`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import {
    CacheConfig,
    setCacheTestClock,
    resetCacheStore,
    registerCache,
} from "../src/index.cp";

let g_get_hits: u32 = 0;
let g_post_hits: u32 = 0;

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn hItems(ctx: *Context): Response {
    * = ctx;
    g_get_hits += 1;
    if (g_get_hits == 1) {
        return Response.json(200, "{\"gen\":1}");
    }
    return Response.json(200, "{\"gen\":2}");
}

fn hCharge(ctx: *Context): Response {
    * = ctx;
    g_post_hits += 1;
    if (g_post_hits == 1) {
        return Response.json(201, "{\"charge_id\":\"ch_001\"}");
    }
    return Response.json(201, "{\"charge_id\":\"ch_002\"}");
}

fn main() {
    resetCacheStore();
    setCacheTestClock(1000);

    let app = App.init();
    * = registerCache(*app, CacheConfig{
        ttl_secs: 30,
        max_entries: 16,
        enable_idempotency: true,
        idempotency_header: "Idempotency-Key",
    });
    * = app.get("/api/items", hItems);
    * = app.post("/api/charges", hCharge);

    // 1. First GET -> MISS, populates cache & attaches Cache-Control
    const r1 = app.dispatch(Request.init(AppMethod.GET, "/api/items"));
    if (r1.status != 200 || !strEql(r1.body, "{\"gen\":1}")) {
        @panic("Test 1a failed: first GET body mismatch");
    }
    if (!strEql(r1.header("X-Cache") ?? "", "MISS")) {
        @panic("Test 1b failed: expected X-Cache: MISS");
    }
    if (!strEql(r1.header("Cache-Control") ?? "", "public, max-age=30")) {
        @panic("Test 1c failed: Cache-Control mismatch");
    }
    if (g_get_hits != 1) @panic("Test 1d failed: handler count mismatch");

    // 2. Second GET within TTL -> HIT, handler NOT re-executed
    setCacheTestClock(1015);
    const r2 = app.dispatch(Request.init(AppMethod.GET, "/api/items"));
    if (r2.status != 200 || !strEql(r2.body, "{\"gen\":1}")) {
        @panic("Test 2a failed: cached GET body mismatch");
    }
    if (!strEql(r2.header("X-Cache") ?? "", "HIT")) {
        @panic("Test 2b failed: expected X-Cache: HIT");
    }
    if (g_get_hits != 1) @panic("Test 2c failed: handler should not run on cache HIT");

    // 3. GET after TTL expiry -> MISS, handler re-executes
    setCacheTestClock(1035);
    const r3 = app.dispatch(Request.init(AppMethod.GET, "/api/items"));
    if (r3.status != 200 || !strEql(r3.body, "{\"gen\":2}")) {
        @panic("Test 3a failed: post-expiry GET body mismatch");
    }
    if (!strEql(r3.header("X-Cache") ?? "", "MISS")) {
        @panic("Test 3b failed: expected X-Cache: MISS after expiry");
    }
    if (g_get_hits != 2) @panic("Test 3c failed: handler should re-run after expiry");

    // 4. POST with Idempotency-Key: first run executes handler
    let p1_req = Request.init(AppMethod.POST, "/api/charges");
    * = p1_req.withHeader("Idempotency-Key", "idem-key-001");
    const p1 = app.dispatch(p1_req);
    if (p1.status != 201 || !strEql(p1.body, "{\"charge_id\":\"ch_001\"}")) {
        @panic("Test 4a failed: first POST mismatch");
    }
    if (p1.header("X-Idempotency-Replayed") != null) {
        @panic("Test 4b failed: first POST should not have replay header");
    }
    if (g_post_hits != 1) @panic("Test 4c failed: POST handler count mismatch");

    // 5. POST replay with same Idempotency-Key -> returns first result with X-Idempotency-Replayed: true
    let p2_req = Request.init(AppMethod.POST, "/api/charges");
    * = p2_req.withHeader("Idempotency-Key", "idem-key-001");
    const p2 = app.dispatch(p2_req);
    if (p2.status != 201 || !strEql(p2.body, "{\"charge_id\":\"ch_001\"}")) {
        @panic("Test 5a failed: replayed POST body mismatch");
    }
    if (!strEql(p2.header("X-Idempotency-Replayed") ?? "", "true")) {
        @panic("Test 5b failed: missing X-Idempotency-Replayed: true");
    }
    if (g_post_hits != 1) @panic("Test 5c failed: POST handler should not re-run on replay");

    // 6. POST with different Idempotency-Key -> executes handler
    let p3_req = Request.init(AppMethod.POST, "/api/charges");
    * = p3_req.withHeader("Idempotency-Key", "idem-key-002");
    const p3 = app.dispatch(p3_req);
    if (p3.status != 201 || !strEql(p3.body, "{\"charge_id\":\"ch_002\"}")) {
        @panic("Test 6a failed: new key POST body mismatch");
    }
    if (g_post_hits != 2) @panic("Test 6b failed: POST handler should run for new key");

    setCacheTestClock(0);
    @print("PASS @web/cache (6/6 tests)");
}
