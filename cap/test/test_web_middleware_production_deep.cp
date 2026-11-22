// Origin: Pure-CP Stdlib Test Suite (lib/web/middleware.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc HTTP Onion Middleware Pipeline:
//   1. Full onion pass-through, X-Request-Id generation/pass-through & WebContext.isOk
//   2. 413 Payload Too Large body size guard (exact boundary & unlimited mode)
//   3. CORS allowlist, wildcard & OPTIONS 204 preflight / 403 rejection
//   4. Constant-time Bearer auth & /healthz,/metrics skip-path whitelist
//   5. Cumulative access, rejection, 5xx & latency telemetry stats
//   6. Standardized JSON error body RFC 8259 escaping & undersized buffer defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    WebContext,
    WebMiddlewareConfig,
    WebMiddlewareOutcome,
    WebMiddlewareStats,
    WebMiddlewarePipeline,
    renderJsonErrorBody,
} from "cap:web";

// ============================================================================
// Test 1: Full Onion Flow & X-Request-Id Pass-Through / Generation
// ============================================================================

fn testOnionHappyPathAndRequestId()! {
    let mw = cap.web.WebMiddlewarePipeline.init(WebMiddlewareConfig{
        max_body_bytes: 4096,
        allowed_origin: "https://app.example.com, https://admin.example.com",
        required_bearer_token: "tok-secret-123",
        auth_skip_paths_csv: "/healthz,/metrics",
    });

    // 1. Auto-generated X-Request-Id
    let ctx1 = WebContext{
        method: "POST",
        path: "/v1/agents",
        origin: "https://app.example.com",
        client_ip: "10.0.0.5",
        bearer_token: "tok-secret-123",
        body_len: 512,
    };
    let id_buf: [32]u8 = [0; 32];
    const req_id1 = mw.ensureRequestId(&ctx1, id_buf[0..]) ?? return error.IdFailed;
    expect_eq(std.mem.eql(u8, req_id1, "req-1"), true);
    expect_eq(std.mem.eql(u8, ctx1.request_id, "req-1"), true);

    expect_eq(mw.runBefore(&ctx1) == WebMiddlewareOutcome.Continue, true);
    expect_eq(ctx1.aborted, false);
    expect_eq(std.mem.eql(u8, ctx1.cors_allow_origin, "https://app.example.com"), true);
    mw.recordAfter(&ctx1, 201, 850);
    expect_eq(ctx1.isOk(), true);
    expect_eq(ctx1.status_code, 201);
    expect_eq(ctx1.elapsed_us, 850);

    // 2. Existing X-Request-Id passed through zero-copy
    let ctx2 = WebContext{
        method: "GET",
        path: "/v1/agents",
        request_id: "trace-abc-999",
        bearer_token: "tok-secret-123",
    };
    const req_id2 = mw.ensureRequestId(&ctx2, id_buf[0..]) ?? return error.IdFailed;
    expect_eq(std.mem.eql(u8, req_id2, "trace-abc-999"), true);
}

// ============================================================================
// Test 2: 413 Payload Too Large Body Size Guard
// ============================================================================

fn testBodySizeLimitGuard()! {
    let mw = WebMiddlewarePipeline.init(WebMiddlewareConfig{
        max_body_bytes: 1024,
        allowed_origin: "",
        required_bearer_token: "",
        auth_skip_paths_csv: "",
    });

    // Exact boundary (1024 bytes) passes
    let ctx_ok = WebContext{ method: "POST", path: "/upload", body_len: 1024 };
    expect_eq(mw.runBefore(&ctx_ok) == WebMiddlewareOutcome.Continue, true);
    expect_eq(ctx_ok.isOk(), true);

    // 1025 bytes -> 413 Payload Too Large
    let ctx_big = WebContext{ method: "POST", path: "/upload", body_len: 1025 };
    expect_eq(mw.runBefore(&ctx_big) == WebMiddlewareOutcome.RejectedPayloadTooLarge, true);
    expect_eq(ctx_big.aborted, true);
    expect_eq(ctx_big.status_code, 413);
    expect_eq(ctx_big.isOk(), false);

    // max_body_bytes == 0 disables body limit
    let mw_unlim = WebMiddlewarePipeline.init(WebMiddlewareConfig{
        max_body_bytes: 0,
        allowed_origin: "",
        required_bearer_token: "",
        auth_skip_paths_csv: "",
    });
    let ctx_huge = WebContext{ method: "POST", path: "/upload", body_len: 99999999 };
    expect_eq(mw_unlim.runBefore(&ctx_huge) == WebMiddlewareOutcome.Continue, true);
}

// ============================================================================
// Test 3: CORS Origin Allowlist, Wildcard & OPTIONS Preflight
// ============================================================================

fn testCorsPreflightAndRejection()! {
    let mw = WebMiddlewarePipeline.init(WebMiddlewareConfig{
        max_body_bytes: 4096,
        allowed_origin: "https://app.example.com,https://docs.example.com",
        required_bearer_token: "secret",
        auth_skip_paths_csv: "/healthz",
    });

    // 1. Valid OPTIONS preflight short-circuits with 204 BEFORE Bearer auth check
    let ctx_pre = WebContext{
        method: "OPTIONS",
        path: "/v1/chat",
        origin: "https://docs.example.com",
        bearer_token: "",
    };
    expect_eq(mw.runBefore(&ctx_pre) == WebMiddlewareOutcome.PreflightHandled, true);
    expect_eq(ctx_pre.status_code, 204);
    expect_eq(ctx_pre.aborted, true);
    expect_eq(std.mem.eql(u8, ctx_pre.cors_allow_origin, "https://docs.example.com"), true);

    // 2. Disallowed Origin rejected with 403
    let ctx_evil = WebContext{
        method: "GET",
        path: "/v1/chat",
        origin: "https://evil.example.org",
        bearer_token: "secret",
    };
    expect_eq(mw.runBefore(&ctx_evil) == WebMiddlewareOutcome.RejectedCors, true);
    expect_eq(ctx_evil.status_code, 403);

    // 3. Wildcard "*" allows any origin
    let mw_wild = WebMiddlewarePipeline.init(WebMiddlewareConfig{
        max_body_bytes: 4096,
        allowed_origin: "*",
        required_bearer_token: "",
        auth_skip_paths_csv: "",
    });
    let ctx_any = WebContext{
        method: "GET",
        path: "/public",
        origin: "https://random.dev",
    };
    expect_eq(mw_wild.runBefore(&ctx_any) == WebMiddlewareOutcome.Continue, true);
    expect_eq(std.mem.eql(u8, ctx_any.cors_allow_origin, "*"), true);
}

// ============================================================================
// Test 4: Bearer Token Auth & /healthz,/metrics Skip-Path Whitelist
// ============================================================================

fn testBearerAuthAndSkipPaths()! {
    let mw = WebMiddlewarePipeline.init(WebMiddlewareConfig{
        max_body_bytes: 4096,
        allowed_origin: "",
        required_bearer_token: "sk-prod-token",
        auth_skip_paths_csv: "/healthz, /metrics",
    });

    // Missing or wrong token on protected route -> 401
    let ctx_no_tok = WebContext{ method: "GET", path: "/v1/models", bearer_token: "" };
    expect_eq(mw.runBefore(&ctx_no_tok) == WebMiddlewareOutcome.RejectedUnauthorized, true);
    expect_eq(ctx_no_tok.status_code, 401);

    let ctx_bad_tok = WebContext{ method: "GET", path: "/v1/models", bearer_token: "sk-prod-tokeX" };
    expect_eq(mw.runBefore(&ctx_bad_tok) == WebMiddlewareOutcome.RejectedUnauthorized, true);
    expect_eq(ctx_bad_tok.status_code, 401);

    // Whitelisted /healthz & /metrics pass without token
    let ctx_health = WebContext{ method: "GET", path: "/healthz", bearer_token: "" };
    expect_eq(mw.runBefore(&ctx_health) == WebMiddlewareOutcome.Continue, true);

    let ctx_metrics = WebContext{ method: "GET", path: "/metrics", bearer_token: "" };
    expect_eq(mw.runBefore(&ctx_metrics) == WebMiddlewareOutcome.Continue, true);
}

// ============================================================================
// Test 5: Cumulative Access, Rejection, 5xx & Latency Telemetry
// ============================================================================

fn testPipelineTelemetryStats()! {
    let mw = WebMiddlewarePipeline.init(WebMiddlewareConfig{
        max_body_bytes: 100,
        allowed_origin: "https://ok.dev",
        required_bearer_token: "secret",
        auth_skip_paths_csv: "/healthz",
    });

    let c1 = WebContext{ method: "POST", path: "/api", body_len: 200 }; // 413
    * = mw.runBefore(&c1);
    let c2 = WebContext{ method: "GET", path: "/api", origin: "https://bad.dev" }; // 403
    * = mw.runBefore(&c2);
    let c3 = WebContext{ method: "OPTIONS", path: "/api", origin: "https://ok.dev" }; // 204
    * = mw.runBefore(&c3);
    let c4 = WebContext{ method: "GET", path: "/api", bearer_token: "wrong" }; // 401
    * = mw.runBefore(&c4);
    let c5 = WebContext{ method: "GET", path: "/api", bearer_token: "secret" }; // Continue -> 200
    * = mw.runBefore(&c5);
    mw.recordAfter(&c5, 200, 1200);
    let c6 = WebContext{ method: "GET", path: "/api", bearer_token: "secret" }; // Continue -> 503
    * = mw.runBefore(&c6);
    mw.recordAfter(&c6, 503, 4500);

    const st = mw.stats();
    expect_eq(st.total_requests, 6);
    expect_eq(st.passed_requests, 2);
    expect_eq(st.rejected_413, 1);
    expect_eq(st.rejected_403, 1);
    expect_eq(st.preflight_204, 1);
    expect_eq(st.rejected_401, 1);
    expect_eq(st.server_5xx, 1);
    expect_eq(st.total_latency_us, 5700);
    expect_eq(st.max_latency_us, 4500);
}

// ============================================================================
// Test 6: renderJsonErrorBody Escaping & Buffer-Too-Small Defensive Guards
// ============================================================================

fn testRenderJsonErrorBodyAndGuards()! {
    let out_buf: [256]u8 = [0; 256];
    const n = renderJsonErrorBody(401, "Invalid \"token\"\nretry", "req-42", out_buf[0..]) ?? return error.RenderFailed;
    const expected = "{\"error\":{\"status\":401,\"message\":\"Invalid \\\"token\\\"\\nretry\",\"request_id\":\"req-42\"}}";
    expect_eq(std.mem.eql(u8, out_buf[0..n], expected), true);

    // Undersized buffer returns null safely
    let tiny: [12]u8 = [0; 12];
    expect_eq(renderJsonErrorBody(500, "Internal Error", "req-1", tiny[0..]) == null, true);

    let mw = WebMiddlewarePipeline.init(WebMiddlewareConfig.default());
    let ctx = WebContext{};
    let tiny_id: [3]u8 = [0; 3];
    expect_eq(mw.ensureRequestId(&ctx, tiny_id[0..]) == null, true);
}

fn main() {
    test("1. web.middleware: onion happy path & X-Request-Id pass-through/gen", () => {
        testOnionHappyPathAndRequestId()?;
    });
    test("2. web.middleware: 413 Payload Too Large body size guard", () => {
        testBodySizeLimitGuard()?;
    });
    test("3. web.middleware: CORS allowlist, wildcard & OPTIONS preflight", () => {
        testCorsPreflightAndRejection()?;
    });
    test("4. web.middleware: Bearer auth & /healthz,/metrics skip whitelist", () => {
        testBearerAuthAndSkipPaths()?;
    });
    test("5. web.middleware: cumulative access, rejection, 5xx & latency stats", () => {
        testPipelineTelemetryStats()?;
    });
    test("6. web.middleware: renderJsonErrorBody escaping & buffer guards", () => {
        testRenderJsonErrorBodyAndGuards()?;
    });
}
