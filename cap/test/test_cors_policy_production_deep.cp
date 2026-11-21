// Source: cp/cap/test/test_cors_policy_production_deep.cp (Production deep tests for cap:cors_policy)

import { test, expect_eq } from "cap:test";
import {
    CorsConfig,
    CorsDecision,
    CorsPolicy,
} from "cap:cors_policy";

test("cors_policy: wildcard origin without credentials vs with credentials echo and Vary: Origin") {
    // 1. 通配源 + 无凭证 -> 输出 "*", vary_origin = false
    const pub_policy = CorsPolicy.init(CorsConfig{
        allow_all_origins: true,
        allow_credentials: false,
    });
    const d1 = pub_policy.evaluateSimple("https://client.example.com");
    expect_eq(d1.allowed, true);
    expect_eq(std.mem.eql(u8, d1.allow_origin_header, "*"), true);
    expect_eq(d1.vary_origin, false);
    expect_eq(d1.allow_credentials, false);

    // 2. 通配源 + 有凭证 -> 禁止输出 "*", 必须回显具体 origin 且 vary_origin = true
    const cred_policy = CorsPolicy.init(CorsConfig{
        allow_all_origins: true,
        allow_credentials: true,
    });
    const d2 = cred_policy.evaluateSimple("https://app.example.com");
    expect_eq(d2.allowed, true);
    expect_eq(std.mem.eql(u8, d2.allow_origin_header, "https://app.example.com"), true);
    expect_eq(d2.vary_origin, true);
    expect_eq(d2.allow_credentials, true);
}

test("cors_policy: explicit origin whitelist allows matched origins and rejects unauthorized origins") {
    let cfg = CorsConfig{ allow_credentials: true };
    expect_eq(cfg.addOrigin("https://a.example.com"), true);
    expect_eq(cfg.addOrigin("https://b.example.com:8443"), true);

    const policy = CorsPolicy.init(cfg);

    const ok = policy.evaluateSimple("https://b.example.com:8443");
    expect_eq(ok.allowed, true);
    expect_eq(std.mem.eql(u8, ok.allow_origin_header, "https://b.example.com:8443"), true);
    expect_eq(ok.vary_origin, true);
    expect_eq(ok.allow_credentials, true);

    const denied = policy.evaluateSimple("https://evil.example.com");
    expect_eq(denied.allowed, false);
    expect_eq(denied.vary_origin, true);
    expect_eq(denied.allow_origin_header.len, 0);
}

test("cors_policy: preflight validates method, custom headers case-insensitively and sets max_age") {
    let cfg = CorsConfig{
        allow_credentials: true,
        max_age_secs: 3600,
    };
    * = cfg.addOrigin("https://web.example.com");
    * = cfg.addMethod("GET");
    * = cfg.addMethod("PUT");
    * = cfg.addHeader("Authorization");
    * = cfg.addHeader("X-Request-Id");

    const policy = CorsPolicy.init(cfg);

    // 方法大小写无关 + 混合 safelisted 与白名单自定义头
    const ok = policy.evaluatePreflight(
        "https://web.example.com",
        "put",
        "Accept, Content-Type, authorization, x-request-id"
    );
    expect_eq(ok.allowed, true);
    expect_eq(std.mem.eql(u8, ok.allow_origin_header, "https://web.example.com"), true);
    expect_eq(ok.vary_origin, true);
    expect_eq(ok.allow_credentials, true);
    expect_eq(ok.max_age_secs.?, 3600);

    // 未授权方法被拒绝
    const bad_method = policy.evaluatePreflight("https://web.example.com", "DELETE", "Authorization");
    expect_eq(bad_method.allowed, false);

    // 未授权自定义头被拒绝
    const bad_header = policy.evaluatePreflight(
        "https://web.example.com",
        "PUT",
        "Authorization, X-Forbidden-Header"
    );
    expect_eq(bad_header.allowed, false);
}

test("cors_policy: CORS-safelisted request headers auto-allowed even with empty allowed_headers") {
    let cfg = CorsConfig{ allow_all_origins: true };
    * = cfg.addMethod("POST");
    const policy = CorsPolicy.init(cfg);

    expect_eq(CorsPolicy.isSafelistedHeader("Accept"), true);
    expect_eq(CorsPolicy.isSafelistedHeader("ACCEPT-LANGUAGE"), true);
    expect_eq(CorsPolicy.isSafelistedHeader("content-language"), true);
    expect_eq(CorsPolicy.isSafelistedHeader("Content-Type"), true);
    expect_eq(CorsPolicy.isSafelistedHeader("X-Custom"), false);

    const ok = policy.evaluatePreflight(
        "https://any.example.com",
        "POST",
        "Accept, Accept-Language, Content-Language, Content-Type"
    );
    expect_eq(ok.allowed, true);

    const blocked = policy.evaluatePreflight(
        "https://any.example.com",
        "POST",
        "Content-Type, X-Custom"
    );
    expect_eq(blocked.allowed, false);
}

test("cors_policy: empty/wildcard literal origin and empty method guards") {
    const policy = CorsPolicy.init(CorsConfig{ allow_all_origins: true });

    expect_eq(policy.evaluateSimple("").allowed, false);
    expect_eq(policy.evaluateSimple("   ").allowed, false);
    expect_eq(policy.evaluateSimple("*").allowed, false);
    expect_eq(policy.evaluatePreflight("https://ok.example.com", "", "").allowed, false);

    // 未配置 allowed_methods 时默认放行 safelisted 方法 GET/HEAD/POST，拒绝 PATCH
    expect_eq(policy.evaluatePreflight("https://ok.example.com", "GET", "").allowed, true);
    expect_eq(policy.evaluatePreflight("https://ok.example.com", "PATCH", "").allowed, false);
}
