// Source: cp/cap/test/test_http_cache_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:http_cache & cap.http_cache

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HttpCacheError,
    FreshnessStatus,
    CacheHeader,
    CachePolicyControl as CacheControl,
    EntityTag,
    HttpCacheEvaluator,
} from "cap:http";

fn strEql(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

test("1. CacheControl.parse & toHeader Full Directive Round-Trip") {
    const alloc = std.testing.allocator;

    const raw = "PUBLIC, max-age=\"3600\", s-maxage=7200, stale-while-revalidate=60, min-fresh=15, max-stale=120, must-revalidate, immutable, no-cache=\"Set-Cookie\", custom-ext=\"v1,v2\"";
    const cc1 = CacheControl.parse(raw) catch unreachable;
    expect_eq(cc1.public_flag, true);
    expect_eq(cc1.private_flag, false);
    expect_eq(cc1.no_store, false);
    expect_eq(cc1.no_cache, true);
    expect_eq(cc1.must_revalidate, true);
    expect_eq(cc1.immutable, true);
    expect_eq(cc1.max_age.?, 3600);
    expect_eq(cc1.s_maxage.?, 7200);
    expect_eq(cc1.stale_while_revalidate.?, 60);
    expect_eq(cc1.min_fresh.?, 15);
    expect_eq(cc1.max_stale.?, 120);

    const header1 = cc1.toHeader(alloc) catch unreachable;
    defer alloc.free(header1);
    expect_eq(
        strEql(
            header1,
            "public, no-cache, must-revalidate, immutable, max-age=3600, s-maxage=7200, stale-while-revalidate=60, min-fresh=15, max-stale=120",
        ),
        true,
    );

    // Round-trip re-parse equivalence
    const cc2 = CacheControl.parse(header1) catch unreachable;
    expect_eq(cc2.public_flag, cc1.public_flag);
    expect_eq(cc2.no_cache, cc1.no_cache);
    expect_eq(cc2.must_revalidate, cc1.must_revalidate);
    expect_eq(cc2.immutable, cc1.immutable);
    expect_eq(cc2.max_age.?, 3600);
    expect_eq(cc2.s_maxage.?, 7200);
    expect_eq(cc2.stale_while_revalidate.?, 60);
    expect_eq(cc2.min_fresh.?, 15);
    expect_eq(cc2.max_stale.?, 120);

    // Private + no-store + valueless max-stale
    const cc3 = CacheControl.parse("private, no-store, max-stale") catch unreachable;
    expect_eq(cc3.private_flag, true);
    expect_eq(cc3.no_store, true);
    expect_eq(cc3.max_stale.?, std.math.maxInt(u64));
    const header3 = cc3.toHeader(alloc) catch unreachable;
    defer alloc.free(header3);
    expect_eq(strEql(header3, "private, no-store, max-stale"), true);

    // Malformed directives must be rejected with InvalidCacheControl
    const bad_inputs: []const []const u8 = [
        "max-age",
        "max-age=",
        "max-age=-10",
        "max-age=abc",
        "s-maxage=99999999999999999999999999",
        "public=true",
        "no-store=1",
        "max-age=\"unterminated",
        "=123",
    ];
    for (bad of bad_inputs) {
        let rejected = false;
        * = CacheControl.parse(bad) catch {
            rejected = true;
        };
        expect_eq(rejected, true);
    }
}

test("2. EntityTag Strong/Weak Comparison, If-None-Match (304) & If-Match (412)") {
    const strong_a = EntityTag.parse("\"v1-abc\"") catch unreachable;
    const strong_a2 = EntityTag.parse("  \"v1-abc\"  ") catch unreachable;
    const weak_a = EntityTag.parse("W/\"v1-abc\"") catch unreachable;
    const strong_b = EntityTag.parse("\"v2-xyz\"") catch unreachable;

    expect_eq(strong_a.weak, false);
    expect_eq(weak_a.weak, true);
    expect_eq(strEql(strong_a.opaque_tag, "v1-abc"), true);
    expect_eq(strEql(weak_a.opaque_tag, "v1-abc"), true);

    // Strong vs weak comparison
    expect_eq(EntityTag.strongEquals(strong_a, strong_a2), true);
    expect_eq(EntityTag.strongEquals(strong_a, weak_a), false);
    expect_eq(EntityTag.strongEquals(weak_a, weak_a), false);
    expect_eq(EntityTag.weakEquals(strong_a, weak_a), true);
    expect_eq(EntityTag.weakEquals(weak_a, strong_b), false);

    // If-None-Match uses WEAK comparison -> 304 Not Modified hit even across W/"..." and "..."
    expect_eq(EntityTag.matchesIfNoneMatch("W/\"v1-abc\"", strong_a) catch unreachable, true);
    expect_eq(EntityTag.matchesIfNoneMatch("\"v2-xyz\", W/\"v1-abc\", \"v3\"", strong_a) catch unreachable, true);
    expect_eq(EntityTag.matchesIfNoneMatch("*", strong_b) catch unreachable, true);
    expect_eq(EntityTag.matchesIfNoneMatch("\"other,with,comma\", \"v2-xyz\"", strong_a) catch unreachable, false);

    // If-Match uses STRONG comparison -> rejects weak ETag (returns false -> 412 Precondition Failed)
    expect_eq(EntityTag.matchesIfMatch("\"v1-abc\"", strong_a) catch unreachable, true);
    expect_eq(EntityTag.matchesIfMatch("W/\"v1-abc\"", strong_a) catch unreachable, false);
    expect_eq(EntityTag.matchesIfMatch("\"v1-abc\"", weak_a) catch unreachable, false);
    expect_eq(EntityTag.matchesIfMatch("W/\"v1-abc\", \"v2-xyz\"", strong_a) catch unreachable, false);
    expect_eq(EntityTag.matchesIfMatch("\"v2-xyz\", \"v1-abc\"", strong_a) catch unreachable, true);
    expect_eq(EntityTag.matchesIfMatch("*", strong_a) catch unreachable, true);

    // Invalid ETag formats must be rejected
    const bad_etags: []const []const u8 = [
        "",
        "unquoted",
        "w/\"lowercase-w\"",
        "\"unclosed",
        "W/unquoted",
        "\"has space\"",
        "\"has\nnewline\"",
        "\"inner\"quote\"",
    ];
    for (bad of bad_etags) {
        let rejected = false;
        * = EntityTag.parse(bad) catch {
            rejected = true;
        };
        expect_eq(rejected, true);
    }

    // If-Modified-Since helper check
    expect_eq(HttpCacheEvaluator.isModifiedSince(1700000000, 1700000000), false);
    expect_eq(HttpCacheEvaluator.isModifiedSince(1700000010, 1700000000), true);
}

test("3. HttpCacheEvaluator.evaluateResponse Freshness, s-maxage, private, no-store & min-fresh") {
    const empty_req = CacheControl{};
    const shared_resp = CacheControl.parse("public, max-age=60, s-maxage=300") catch unreachable;

    // Shared cache prioritizes s-maxage (300) over max-age (60)
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, shared_resp, true, 120, null) == .Fresh, true);
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, shared_resp, true, 300, null) == .Stale, true);

    // Private cache ignores s-maxage and uses max-age (60)
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, shared_resp, false, 120, null) == .Stale, true);
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, shared_resp, false, 30, null) == .Fresh, true);

    // private response is Uncacheable on shared cache, Fresh on private cache
    const priv_resp = CacheControl.parse("private, max-age=120") catch unreachable;
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, priv_resp, true, 10, null) == .Uncacheable, true);
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, priv_resp, false, 10, null) == .Fresh, true);

    // no-store on either request or response yields Uncacheable
    const nostore_resp = CacheControl.parse("no-store, max-age=3600") catch unreachable;
    const nostore_req = CacheControl.parse("no-store") catch unreachable;
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, nostore_resp, false, 0, null) == .Uncacheable, true);
    expect_eq(HttpCacheEvaluator.evaluateResponse(nostore_req, shared_resp, false, 0, null) == .Uncacheable, true);

    // no-cache forces revalidation (Stale)
    const nocache_req = CacheControl.parse("no-cache") catch unreachable;
    const nocache_resp = CacheControl.parse("no-cache, max-age=3600") catch unreachable;
    expect_eq(HttpCacheEvaluator.evaluateResponse(nocache_req, shared_resp, false, 0, null) == .Stale, true);
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, nocache_resp, false, 0, null) == .Stale, true);

    // min-fresh requirement: remaining = 300 - 250 = 50 < min-fresh(60) => Stale
    const minfresh_req = CacheControl.parse("min-fresh=60") catch unreachable;
    expect_eq(HttpCacheEvaluator.evaluateResponse(minfresh_req, shared_resp, true, 250, null) == .Stale, true);
    expect_eq(HttpCacheEvaluator.evaluateResponse(minfresh_req, shared_resp, true, 240, null) == .Fresh, true);

    // Expires - Date fallback when no max-age/s-maxage present
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, CacheControl{}, true, 40, 100) == .Fresh, true);
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, CacheControl{}, true, 120, 100) == .Stale, true);
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, CacheControl{}, true, 0, -5) == .Stale, true);
    expect_eq(HttpCacheEvaluator.evaluateResponse(empty_req, CacheControl{}, true, 0, null) == .Stale, true);
}

test("4. HttpCacheEvaluator.buildVaryKey Case-Insensitive Determinism & Zero-Leak") {
    const alloc = std.testing.allocator;

    const req1: []const CacheHeader = [
        CacheHeader{ name: "Accept-Encoding", value: "gzip, br" },
        CacheHeader{ name: "User-Agent", value: "CP-Agent/1.0" },
        CacheHeader{ name: "X-Ignored", value: "foo" },
    ];
    const req2: []const CacheHeader = [
        CacheHeader{ name: "user-agent", value: "  CP-Agent/1.0  " },
        CacheHeader{ name: "ACCEPT-ENCODING", value: "gzip, br" },
        CacheHeader{ name: "X-Other", value: "bar" },
    ];

    // Different Vary header order & casing must produce identical deterministic cache keys
    const key1 = (HttpCacheEvaluator.buildVaryKey(alloc, "GET", "/v1/items", "User-Agent, Accept-Encoding", req1) catch unreachable).?;
    defer alloc.free(key1);
    const key2 = (HttpCacheEvaluator.buildVaryKey(alloc, "GET", "/v1/items", "accept-encoding, user-agent, Accept-Encoding", req2) catch unreachable).?;
    defer alloc.free(key2);
    expect_eq(strEql(key1, key2), true);

    // Different header value must produce a different key
    const req3: []const CacheHeader = [
        CacheHeader{ name: "Accept-Encoding", value: "identity" },
        CacheHeader{ name: "User-Agent", value: "CP-Agent/1.0" },
    ];
    const key3 = (HttpCacheEvaluator.buildVaryKey(alloc, "GET", "/v1/items", "Accept-Encoding, User-Agent", req3) catch unreachable).?;
    defer alloc.free(key3);
    expect_eq(strEql(key1, key3), false);

    // Vary: * must return null (unshareable) without leaking memory
    const star1 = HttpCacheEvaluator.buildVaryKey(alloc, "GET", "/v1/items", "*", req1) catch unreachable;
    expect_eq(star1 == null, true);
    const star2 = HttpCacheEvaluator.buildVaryKey(alloc, "GET", "/v1/items", "Accept-Encoding, *", req1) catch unreachable;
    expect_eq(star2 == null, true);
}

test("5. import cap from \"cap\"; cap.http_cache Dynamic Lazy Mounting") {
    const alloc = std.testing.allocator;

    const cc = cap.http_cache.CacheControl.parse("public, s-maxage=120, max-age=60") catch unreachable;
    const etag = cap.http_cache.EntityTag.parse("\"dyn-1\"") catch unreachable;
    expect_eq(cap.http_cache.EntityTag.matchesIfNoneMatch("W/\"dyn-1\"", etag) catch unreachable, true);
    expect_eq(cap.http_cache.EntityTag.matchesIfMatch("W/\"dyn-1\"", etag) catch unreachable, false);

    const status = cap.http_cache.HttpCacheEvaluator.evaluateResponse(
        cap.http_cache.CacheControl{},
        cc,
        true,
        90,
        null,
    );
    expect_eq(status == .Fresh, true);

    const hdrs: []const cap.http_cache.CacheHeader = [
        cap.http_cache.CacheHeader{ name: "Accept", value: "application/json" },
    ];
    const k = (cap.http_cache.HttpCacheEvaluator.buildVaryKey(alloc, "GET", "/health", "Accept", hdrs) catch unreachable).?;
    defer alloc.free(k);
    expect_eq(k.len > 0, true);
}
