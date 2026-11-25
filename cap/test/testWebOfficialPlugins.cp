// Full-Stack Integration Test for all 7 Official `@web/*` Plugins on a single `cap:web` App:
//   @web/cors + @web/security + @web/jwt + @web/ratelimit + @web/logger + @web/static + @web/swagger
import { App, AppMethod, Context, Request, Response } from "../../lib/web/app.cp";
import { CorsConfig, registerCors } from "../../packages/@web/cors/src/index.cp";
import { SecurityConfig, registerSecurity } from "../../packages/@web/security/src/index.cp";
import { JwtConfig, signHs256, registerJwt } from "../../packages/@web/jwt/src/index.cp";
import { RateLimitConfig, registerRateLimit, setTestClock } from "../../packages/@web/ratelimit/src/index.cp";
import { LoggerConfig, registerLogger } from "../../packages/@web/logger/src/index.cp";
import { StaticConfig, registerStatic } from "../../packages/@web/static/src/index.cp";
import { SwaggerConfig, registerSwagger, refreshSpec } from "../../packages/@web/swagger/src/index.cp";

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn strContains(hay: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > hay.len) return false;
    let i: usize = 0;
    while (i + needle.len <= hay.len) : (i += 1) {
        if (strEql(hay[i .. i + needle.len], needle)) return true;
    }
    return false;
}

fn hProtected(ctx: *Context): Response {
    if (v := ctx.derived("jwt_verified")) {
        if (strEql(v, "1")) {
            return Response.json(200, "{\"access\":\"granted\"}");
        }
    }
    return Response.json(401, "{\"access\":\"denied\"}");
}

fn main() {
    const secret = "cp-production-secret-key-2026";
    setTestClock(1700000000);

    let app = App.init();
    * = registerCors(*app, CorsConfig{
        allow_origin: "https://app.cp.dev",
        allow_credentials: true,
    });
    * = registerSecurity(*app, SecurityConfig{ enable_csrf: false });
    * = registerRateLimit(*app, RateLimitConfig{ max_requests: 5, window_secs: 60 });
    * = registerLogger(*app, LoggerConfig{ enable_trace: true });
    * = registerJwt(*app, JwtConfig{ secret: secret, optional_auth: true });
    * = registerStatic(*app, StaticConfig{ root_dir: "/data/dev/cp/temp", url_prefix: "/static" });
    * = app.get("/api/secret", hProtected);
    * = registerSwagger(*app, SwaggerConfig{ title: "Unified 7-Plugin API", version: "1.0.0" });
    * = refreshSpec(*app);

    // 1. CORS Preflight OPTIONS -> 204
    let pre = Request.init(AppMethod.OPTIONS, "/api/secret");
    * = pre.withHeader("Origin", "https://app.cp.dev");
    const pre_resp = app.dispatch(pre);
    if (pre_resp.status != 204) @panic("Integration 1 failed: CORS preflight expected 204");

    // 2. Signed HS256 JWT + Helmet + Logger + CORS all active together
    let tok_buf: [512]u8 = undefined;
    let tlen: usize = 0;
    if (n := signHs256(secret, "{\"sub\":\"alice\"}", tok_buf[0..])) {
        tlen = n;
    } else {
        @panic("signHs256 failed");
    }
    let auth_hdr: [540]u8 = undefined;
    const prefix = "Bearer ";
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        auth_hdr[i] = prefix[i];
    }
    i = 0;
    while (i < tlen) : (i += 1) {
        auth_hdr[prefix.len + i] = tok_buf[i];
    }

    let req = Request.init(AppMethod.GET, "/api/secret");
    * = req
        .withHeader("Origin", "https://app.cp.dev")
        .withHeader("X-Forwarded-For", "10.0.0.1")
        .withHeader("Authorization", auth_hdr[0 .. prefix.len + tlen]);
    const resp = app.dispatch(req);
    if (resp.status != 200 || !strEql(resp.body, "{\"access\":\"granted\"}")) {
        @panic("Integration 2a failed: JWT protected route");
    }
    if (!strEql(resp.header("Access-Control-Allow-Origin") ?? "", "https://app.cp.dev")) {
        @panic("Integration 2b failed: missing CORS header");
    }
    if (!strEql(resp.header("X-Frame-Options") ?? "", "DENY")) {
        @panic("Integration 2c failed: missing Helmet header");
    }
    if ((resp.header("X-Request-Id") ?? "").len == 0 || (resp.header("traceparent") ?? "").len == 0) {
        @panic("Integration 2d failed: missing Logger request-id/traceparent");
    }
    if ((resp.header("RateLimit-Limit") ?? "").len == 0) {
        @panic("Integration 2e failed: missing RateLimit header");
    }

    // 3. Swagger /openapi.json & /docs
    const spec_resp = app.dispatch(Request.init(AppMethod.GET, "/openapi.json"));
    if (spec_resp.status != 200 || !strContains(spec_resp.body, "Unified 7-Plugin API")) {
        @panic("Integration 3a failed: /openapi.json");
    }
    const docs_resp = app.dispatch(Request.init(AppMethod.GET, "/docs"));
    if (docs_resp.status != 200 || !strContains(docs_resp.body, "/openapi.json")) {
        @panic("Integration 3b failed: /docs");
    }

    // 4. Static path traversal blocked with 403
    const trav_resp = app.dispatch(Request.init(AppMethod.GET, "/static/../etc/passwd"));
    if (trav_resp.status != 403) {
        @panic("Integration 4 failed: static path traversal expected 403");
    }

    // 5. Logger /metrics endpoint returns OpenMetrics text
    const met_resp = app.dispatch(Request.init(AppMethod.GET, "/metrics"));
    if (met_resp.status != 200 || !strContains(met_resp.body, "http_requests_total")) {
        @panic("Integration 5 failed: /metrics");
    }

    @print("PASS testWebOfficialPlugins (All 7 @web/* plugins working together!)");
}
