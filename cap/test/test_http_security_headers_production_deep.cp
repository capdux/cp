import std from "std";
import { test, expect_eq } from "cap:test";
import {
    HstsPolicy,
    ReferrerPolicy,
    parseReferrerPolicy,
    FrameOptions,
    parseFrameOptions,
    CoopPolicy,
    parseCoopPolicy,
    CoepPolicy,
    parseCoepPolicy,
    CorpPolicy,
    parseCorpPolicy,
    isCrossOriginIsolated,
} from "cap:http_security_headers";

test("1. HstsPolicy parse, quoted max-age, preload eligibility, and canonical formatting", () => {
    const h = HstsPolicy.parse("Max-Age=\"63072000\"; INCLUDEsubdomains; Preload") ?? unreachable;
    expect_eq(h.max_age, u64(63072000));
    expect_eq(h.include_subdomains, true);
    expect_eq(h.preload, true);
    expect_eq(h.isPreloadEligible(), true);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const fmt = h.format(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, fmt, "max-age=63072000; includeSubDomains; preload"), true);

    // 不满足 preload 准入（max-age < 1 年）
    const short_h = HstsPolicy.parse("max-age=86400; includeSubDomains; preload") ?? unreachable;
    expect_eq(short_h.isPreloadEligible(), false);

    // 缺失 max-age、重复 max-age 或负数均拒绝
    expect_eq(HstsPolicy.parse("includeSubDomains; preload") == null, true);
    expect_eq(HstsPolicy.parse("max-age=10; max-age=20") == null, true);
    expect_eq(HstsPolicy.parse("max-age=-1") == null, true);
});

test("2. parseReferrerPolicy handles fallback chain and all 8 W3C values", () => {
    // 逗号分隔回退链：取最后一个合法策略，忽略末尾未知策略
    const r = parseReferrerPolicy("no-referrer, strict-origin-when-cross-origin, future-unknown") ?? unreachable;
    expect_eq(r == ReferrerPolicy.strict_origin_when_cross_origin, true);
    expect_eq(std.mem.eql(u8, r.asStr(), "strict-origin-when-cross-origin"), true);

    expect_eq((parseReferrerPolicy("NO-REFERRER") ?? unreachable) == ReferrerPolicy.no_referrer, true);
    expect_eq((parseReferrerPolicy("no-referrer-when-downgrade") ?? unreachable) == ReferrerPolicy.no_referrer_when_downgrade, true);
    expect_eq((parseReferrerPolicy("origin") ?? unreachable) == ReferrerPolicy.origin, true);
    expect_eq((parseReferrerPolicy("origin-when-cross-origin") ?? unreachable) == ReferrerPolicy.origin_when_cross_origin, true);
    expect_eq((parseReferrerPolicy("same-origin") ?? unreachable) == ReferrerPolicy.same_origin, true);
    expect_eq((parseReferrerPolicy("strict-origin") ?? unreachable) == ReferrerPolicy.strict_origin, true);
    expect_eq((parseReferrerPolicy("unsafe-url") ?? unreachable) == ReferrerPolicy.unsafe_url, true);

    expect_eq(parseReferrerPolicy("") == null, true);
    expect_eq(parseReferrerPolicy("unknown-only") == null, true);
});

test("3. parseFrameOptions handles DENY and SAMEORIGIN case-insensitively", () => {
    const f1 = parseFrameOptions("  deny ") ?? unreachable;
    expect_eq(f1 == FrameOptions.deny, true);
    expect_eq(std.mem.eql(u8, f1.asStr(), "DENY"), true);

    const f2 = parseFrameOptions("SameOrigin") ?? unreachable;
    expect_eq(f2 == FrameOptions.sameorigin, true);
    expect_eq(std.mem.eql(u8, f2.asStr(), "SAMEORIGIN"), true);

    expect_eq(parseFrameOptions("ALLOW-FROM https://example.com") == null, true);
});

test("4. COOP, COEP, CORP parsing with report-to params and isCrossOriginIsolated", () => {
    const coop = parseCoopPolicy("same-origin; report-to=\"default\"") ?? unreachable;
    const coep_corp = parseCoepPolicy("require-corp; report-to=\"ep\"") ?? unreachable;
    const coep_cred = parseCoepPolicy("credentialless") ?? unreachable;
    const coep_none = parseCoepPolicy("unsafe-none") ?? unreachable;
    const coop_pop = parseCoopPolicy("same-origin-allow-popups") ?? unreachable;

    expect_eq(isCrossOriginIsolated(coop, coep_corp), true);
    expect_eq(isCrossOriginIsolated(coop, coep_cred), true);
    expect_eq(isCrossOriginIsolated(coop, coep_none), false);
    expect_eq(isCrossOriginIsolated(coop_pop, coep_corp), false);

    expect_eq((parseCorpPolicy("same-site") ?? unreachable) == CorpPolicy.same_site, true);
    expect_eq((parseCorpPolicy("same-origin") ?? unreachable) == CorpPolicy.same_origin, true);
    expect_eq((parseCorpPolicy("cross-origin") ?? unreachable) == CorpPolicy.cross_origin, true);
    expect_eq(parseCorpPolicy("invalid") == null, true);
});

test("5. HstsPolicy.format buffer-too-small guard", () => {
    const h = HstsPolicy{ max_age: 31536000, include_subdomains: true, preload: true };
    let tiny: [12]u8 = std.mem.zeroes([12]u8);
    expect_eq(h.format(&tiny) == null, true);
});
