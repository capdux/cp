// Regression Test Suite for `lib/web/comptimeRouter.cp`
// Verifies:
//   1. True `comptime` compilation of NestJS-style declarative `StaticRouteSpec` table
//   2. Length bitmask pre-filter + FNV-1a O(1) `dispatchStatic` + HEAD->GET fallback + 405 allowed_mask
//   3. `mountInto(app)` & `mountIntoWithPrefix(app, prefix)` with full Elysia lifecycle + OpenAPI 3.1.0
//   4. `TraceSpan` & `LifecycleTrace` deterministic nanosecond timing + W3C `Server-Timing` header
//   5. Live monotonic clock `dispatchStaticWithTrace` + `attachServerTiming`

import {
    App,
    AppMethod,
    Context,
    Request,
    Response,
} from "../../lib/web/app.cp";

import {
    StaticRouteSpec,
    StaticLookupResult,
    ComptimeRouteTable,
    TraceSpan,
    LifecycleTrace,
    routeGet,
    routePost,
    routeDelete,
    fnv1a64,
    monotonicNowNs,
} from "../../lib/web/comptimeRouter.cp";

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

// --- Handlers ---

fn handleHealth(ctx: *Context): Response {
    return ctx.json(200, "{\"status\":\"ok\"}");
}

fn handleUsersList(ctx: *Context): Response {
    return ctx.json(200, "{\"users\":[\"alice\",\"bob\"]}");
}

fn handleUsersCreate(ctx: *Context): Response {
    return ctx.json(201, "{\"created\":true}");
}

fn handleUsersDelete(ctx: *Context): Response {
    return ctx.json(200, "{\"deleted\":true}");
}

fn handleMetrics(ctx: *Context): Response {
    return ctx.text(200, "cp_requests_total 42");
}

fn hookOnRequest(ctx: *Context): bool {
    * = ctx.setHeader("X-Comptime-Lifecycle", "active");
    return true;
}

// --- Compile-time constant NestJS-style declarative route table ---

const API_SPECS: [5]StaticRouteSpec = [
    routeGet("/health", handleHealth, "Health check"),
    routeGet("/api/users", handleUsersList, "List all users"),
    routePost("/api/users", handleUsersCreate, "Create user"),
    routeDelete("/api/users", handleUsersDelete, "Delete user"),
    routeGet("/metrics", handleMetrics, "Prometheus metrics"),
];

const COMPTIME_TABLE: ComptimeRouteTable = ComptimeRouteTable.compile(API_SPECS[0..]);

fn main() {
    // Test 1: Compile-time precomputed metadata (length bitmask, buckets, FNV-1a hash)
    if (COMPTIME_TABLE.count() != 5) {
        @panic("Test 1 failed: expected 5 routes in COMPTIME_TABLE");
    }
    if (!COMPTIME_TABLE.hasPathLen(7)) {
        @panic("Test 1 failed: expected path len 7 (/health) in len_mask");
    }
    if (!COMPTIME_TABLE.hasPathLen(10)) {
        @panic("Test 1 failed: expected path len 10 (/api/users) in len_mask");
    }
    if (COMPTIME_TABLE.hasPathLen(13)) {
        @panic("Test 1 failed: unexpected path len 13 in len_mask");
    }
    if (COMPTIME_TABLE.bucketCountForLen(7) != 1 || COMPTIME_TABLE.bucketCountForLen(8) != 1 || COMPTIME_TABLE.bucketCountForLen(10) != 1) {
        @panic("Test 1 failed: length bucket counts mismatch");
    }
    const h_health: u64 = comptime fnv1a64("/health");
    if (h_health != fnv1a64("/health")) {
        @panic("Test 1 failed: comptime vs runtime fnv1a64 mismatch");
    }

    // Test 2: O(1) dispatchStatic fast-path (GET, POST, DELETE, HEAD->GET fallback, miss & 405 mask)
    let ctx = Context{};
    if (resp := COMPTIME_TABLE.dispatchStatic(AppMethod.GET, "/health", *ctx)) {
        if (resp.status != 200 || !strEql(resp.body, "{\"status\":\"ok\"}") || !strEql(ctx.matchedPath(), "/health")) {
            @panic("Test 2a failed: GET /health");
        }
    } else {
        @panic("Test 2a failed: GET /health returned null");
    }

    if (resp := COMPTIME_TABLE.dispatchStatic(AppMethod.POST, "/api/users", *ctx)) {
        if (resp.status != 201 || !strEql(resp.body, "{\"created\":true}")) {
            @panic("Test 2b failed: POST /api/users");
        }
    } else {
        @panic("Test 2b failed: POST /api/users returned null");
    }

    if (resp := COMPTIME_TABLE.dispatchStatic(AppMethod.DELETE, "/api/users", *ctx)) {
        if (resp.status != 200 || !strEql(resp.body, "{\"deleted\":true}")) {
            @panic("Test 2c failed: DELETE /api/users");
        }
    } else {
        @panic("Test 2c failed: DELETE /api/users returned null");
    }

    // HEAD -> GET automatic fallback (status 200, empty body)
    if (resp := COMPTIME_TABLE.dispatchStatic(AppMethod.HEAD, "/health", *ctx)) {
        if (resp.status != 200 || resp.body.len != 0) {
            @panic("Test 2d failed: HEAD -> GET fallback");
        }
    } else {
        @panic("Test 2d failed: HEAD /health returned null");
    }

    // Fast-reject miss (length not in bitmask)
    if (COMPTIME_TABLE.dispatchStatic(AppMethod.GET, "/not-existing", *ctx) != null) {
        @panic("Test 2e failed: expected null for unknown route");
    }

    // Method not allowed lookup returns allowed_mask (GET|POST|DELETE = 1|2|8 = 11)
    const miss_method: StaticLookupResult = COMPTIME_TABLE.lookup(AppMethod.PUT, "/api/users");
    if (miss_method.matched || miss_method.allowed_mask != 11) {
        @panic("Test 2f failed: expected allowed_mask=11 for PUT /api/users");
    }

    // Test 3: mountInto(app) + mountIntoWithPrefix(app, "/v2") + Lifecycle + OpenAPI 3.1.0
    let app = App.init();
    * = app.onRequest(hookOnRequest);
    const mounted: usize = COMPTIME_TABLE.mountInto(*app);
    if (mounted != 5) {
        @panic("Test 3a failed: mountInto count mismatch");
    }
    const mounted_v2: usize = COMPTIME_TABLE.mountIntoWithPrefix(*app, "/v2");
    if (mounted_v2 != 5) {
        @panic("Test 3b failed: mountIntoWithPrefix count mismatch");
    }

    let r3a = app.dispatch(Request.init(AppMethod.GET, "/api/users"));
    if (r3a.status != 200 || !strEql(r3a.body, "{\"users\":[\"alice\",\"bob\"]}")) {
        @panic("Test 3c failed: App dispatch after mountInto");
    }
    if (hdr := r3a.header("X-Comptime-Lifecycle")) {
        if (!strEql(hdr, "active")) @panic("Test 3c failed: lifecycle header mismatch");
    } else {
        @panic("Test 3c failed: missing X-Comptime-Lifecycle header");
    }

    let r3b = app.dispatch(Request.init(AppMethod.GET, "/v2/metrics"));
    if (r3b.status != 200 || !strEql(r3b.body, "cp_requests_total 42")) {
        @panic("Test 3d failed: prefixed /v2/metrics dispatch");
    }

    let openapi_buf: [4096]u8 = [0; 4096];
    if (olen := app.toOpenApiJson("Comptime API", "1.0.0", openapi_buf[0..])) {
        if (!strContains(openapi_buf[0..olen], "\"openapi\":\"3.1.0\"")
            || !strContains(openapi_buf[0..olen], "List all users")
            || !strContains(openapi_buf[0..olen], "\"/v2/health\""))
        {
            @panic("Test 3e failed: OpenAPI 3.1.0 missing mounted route metadata");
        }
    } else {
        @panic("Test 3e failed: generateOpenApiJson returned null");
    }

    // Test 4: Deterministic LifecycleTrace & W3C Server-Timing formatting
    let tr = LifecycleTrace.init();
    tr.beginRouting(10000);
    tr.endRouting(11000); // 1000 ns = 0.001 ms
    tr.beginHandler(11000);
    tr.endHandler(13000); // 2000 ns = 0.002 ms

    let timing_buf: [256]u8 = [0; 256];
    if (tlen := tr.formatTraceLine(timing_buf[0..])) {
        if (!strEql(timing_buf[0..tlen], "routing;dur=0.001, handler;dur=0.002")) {
            @panic("Test 4a failed: Server-Timing format mismatch");
        }
    } else {
        @panic("Test 4a failed: formatTraceLine returned null");
    }

    // All 5 stages recorded
    let full_tr = LifecycleTrace.init();
    full_tr.beginRequest(1000);
    full_tr.beginOnRequest(1000);
    full_tr.endOnRequest(6000); // 5000 ns = 0.005 ms
    full_tr.beginRouting(6000);
    full_tr.endRouting(7000); // 1000 ns = 0.001 ms
    full_tr.beginDerive(7000);
    full_tr.endDerive(107000); // 100000 ns = 0.100 ms
    full_tr.beginHandler(107000);
    full_tr.endHandler(1357000); // 1250000 ns = 1.250 ms
    full_tr.beginResponse(1357000);
    full_tr.endResponse(1360000); // 3000 ns = 0.003 ms
    full_tr.endRequest(1360000);

    if (full_tr.totalDurationNs() != 1359000) {
        @panic("Test 4b failed: totalDurationNs mismatch");
    }
    if (flen := full_tr.formatTraceLine(timing_buf[0..])) {
        if (!strEql(timing_buf[0..flen], "onRequest;dur=0.005, routing;dur=0.001, derive;dur=0.100, handler;dur=1.250, response;dur=0.003")) {
            @panic("Test 4c failed: 5-stage Server-Timing mismatch");
        }
    } else {
        @panic("Test 4c failed: 5-stage formatTraceLine returned null");
    }

    // Buffer too small returns null safely (no overflow)
    let tiny_buf: [8]u8 = [0; 8];
    if (full_tr.formatTraceLine(tiny_buf[0..]) != null) {
        @panic("Test 4d failed: expected null for tiny_buf");
    }

    // Test 5: Live monotonic clock + dispatchStaticWithTrace + attachServerTiming
    let live_tr = LifecycleTrace.init();
    let mut_resp = Response{};
    let hdr_buf: [256]u8 = [0; 256];
    const t0: u64 = monotonicNowNs();
    if (resp := COMPTIME_TABLE.dispatchStaticWithTrace(AppMethod.GET, "/metrics", *ctx, *live_tr)) {
        mut_resp = resp;
        if (!live_tr.attachServerTiming(*mut_resp, hdr_buf[0..])) {
            @panic("Test 5a failed: attachServerTiming returned false");
        }
        if (st := mut_resp.header("Server-Timing")) {
            if (!strContains(st, "routing;dur=") || !strContains(st, "handler;dur=")) {
                @panic("Test 5b failed: Server-Timing header missing spans");
            }
        } else {
            @panic("Test 5b failed: missing Server-Timing header");
        }
    } else {
        @panic("Test 5a failed: dispatchStaticWithTrace returned null");
    }
    const t1: u64 = monotonicNowNs();
    if (t1 < t0) {
        @panic("Test 5c failed: monotonic clock went backwards");
    }

    // TraceSpan microsecond conversion sanity check
    let sp = TraceSpan.init("custom");
    sp.recordDurationNs(42500);
    if (sp.durationUs() != 42) {
        @panic("Test 5d failed: TraceSpan.durationUs mismatch");
    }

    // Test 6: Comptime Dynamic `:param` routes + zero-copy dispatchInto + ControllerSpec
    let dyn_table = ComptimeRouteTable.init();
    * = dyn_table.insert(routeGet("/health", handleHealth, "Health"));
    * = dyn_table.insert(routeGet("/api/v1/users/:id/posts/:postId", handleHealth, "User Post"));
    if (dyn_table.staticCount() != 1 || dyn_table.dynamicCount() != 1 || dyn_table.count() != 2) {
        @panic("Test 6a failed: static/dynamic route counts mismatch");
    }
    let dyn_req = Request.init(AppMethod.GET, "/api/v1/users/42/posts/99?lang=en");
    let dyn_ctx: Context = undefined;
    let dyn_resp: Response = undefined;
    if (!dyn_table.dispatchInto(&dyn_req, *dyn_ctx, *dyn_resp)) {
        @panic("Test 6b failed: dispatchInto missed comptime dynamic route");
    }
    if (dyn_resp.status != 200) {
        @panic("Test 6b failed: unexpected status");
    }
    if (uid := dyn_ctx.param("id")) {
        if (!strEql(uid, "42")) @panic("Test 6c failed: param id mismatch");
    } else {
        @panic("Test 6c failed: missing param id");
    }
    if (pid := dyn_ctx.param("postId")) {
        if (!strEql(pid, "99")) @panic("Test 6d failed: param postId mismatch");
    } else {
        @panic("Test 6d failed: missing param postId");
    }
    if (lang := dyn_ctx.query("lang")) {
        if (!strEql(lang, "en")) @panic("Test 6e failed: query lang mismatch");
    } else {
        @panic("Test 6e failed: missing query lang");
    }

    @print("PASS testWebComptimeRouter (6/6 suites)");
}
