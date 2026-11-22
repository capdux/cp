// Origin: Pure-CP Stdlib Test Suite (lib/web/cachePolicy.cp) | Created: 2026-11-22
// Production Deep Verification Suite for RFC 9111 HTTP Cache Freshness & Directive Evaluator:
//   1. Case-insensitive Cache-Control parsing, quoted values, max-stale bare/valued & unknown extension skipping
//   2. RFC 9111 §4.2 freshness lifetime precedence (s-maxage > max-age > Expires-Date > 10% Last-Modified)
//   3. RFC 9111 §4.2.3 current age calculation (apparent_age, response_delay, Age header & resident_time)
//   4. Shared vs private storability checks (no-store, private, Authorization & status codes)
//   5. Lookup verdicts: fresh_hit, stale_while_revalidate, stale_if_error & revalidate_required
//   6. Request directives (no-store, no-cache, min-fresh, max-age, max-stale) vs must-revalidate/proxy-revalidate
//   7. Malformed header/numeric/quote/CRLF boundary guards & cap.web facade verification

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CacheScope,
    CacheLookupVerdict,
    CacheControl,
    StoredResponseMeta,
    CacheLookupRequest,
    parseCacheControl,
    isResponseStorable,
    computeFreshnessLifetime,
    computeCurrentAge,
    evaluateCacheLookup,
} from "cap:web";

// ============================================================================
// Test 1: Case-Insensitive Cache-Control Parser, Quoted Values & Extensions
// ============================================================================
test("cachePolicy: parseCacheControl handles case-insensitivity, quotes, max-stale & extensions", () => {
    const hdr = "Public, MAX-AGE=\"120\", s-MaxAge=300, Stale-While-Revalidate=45, stale-if-error=\"600\", immutable, ext-foo=\"a, b=1\", bare-ext, max-stale";
    const cc = parseCacheControl(hdr) ?? return;

    expect_true(cc.is_public);
    expect_true(!cc.is_private);
    expect_true(!cc.no_store);
    expect_true(cc.immutable);
    expect_true(cc.has_max_stale);
    expect_true(cc.max_stale == null);
    expect_eq(cc.max_age ?? 0, 120);
    expect_eq(cc.s_maxage ?? 0, 300);
    expect_eq(cc.stale_while_revalidate ?? 0, 45);
    expect_eq(cc.stale_if_error ?? 0, 600);

    // Valued max-stale + field-qualified no-cache="Set-Cookie" + only-if-cached + min-fresh
    const req_cc = parseCacheControl("no-cache=\"Set-Cookie\", min-fresh=15, max-stale=90, only-if-cached") ?? return;
    expect_true(req_cc.no_cache);
    expect_true(req_cc.only_if_cached);
    expect_eq(req_cc.min_fresh ?? 0, 15);
    expect_true(req_cc.has_max_stale);
    expect_eq(req_cc.max_stale ?? 0, 90);
});

// ============================================================================
// Test 2: RFC 9111 §4.2 Freshness Lifetime Precedence
// ============================================================================
test("cachePolicy: freshness lifetime follows s-maxage > max-age > Expires-Date > 10% Last-Modified", () => {
    const cc_both = parseCacheControl("max-age=60, s-maxage=180") ?? return;
    const resp_both = StoredResponseMeta{
        status_code: 200,
        cache_control: cc_both,
        date_sec: 1000,
        expires_sec: 2000,
        last_modified_sec: 0,
    };
    // Shared cache prefers s-maxage (180); private cache ignores s-maxage and uses max-age (60)
    expect_eq(computeFreshnessLifetime(resp_both, CacheScope.shared_cache), 180);
    expect_eq(computeFreshnessLifetime(resp_both, CacheScope.private_cache), 60);

    // Expires - Date when no max-age
    const resp_exp = StoredResponseMeta{
        status_code: 200,
        date_sec: 1000,
        expires_sec: 1250,
    };
    expect_eq(computeFreshnessLifetime(resp_exp, CacheScope.shared_cache), 250);

    // Date >= Expires yields 0 freshness lifetime
    const resp_past_exp = StoredResponseMeta{
        status_code: 200,
        date_sec: 1500,
        expires_sec: 1200,
    };
    expect_eq(computeFreshnessLifetime(resp_past_exp, CacheScope.shared_cache), 0);

    // Heuristic 10% of (Date - Last-Modified)
    const resp_heur = StoredResponseMeta{
        status_code: 200,
        date_sec: 2000,
        last_modified_sec: 1000,
    };
    expect_eq(computeFreshnessLifetime(resp_heur, CacheScope.shared_cache), 100);
});

// ============================================================================
// Test 3: RFC 9111 §4.2.3 Current Age Calculation
// ============================================================================
test("cachePolicy: computeCurrentAge combines apparent_age, Age+delay and resident_time safely", () => {
    // date=1000, req=1002, resp=1005 (delay=3, apparent=5), Age=10 -> corrected_age=13 > 5 -> init=13
    // At now=1025 (resident=20) -> current_age = 33
    const resp = StoredResponseMeta{
        date_sec: 1000,
        age_header_sec: 10,
        request_time_sec: 1002,
        response_time_sec: 1005,
    };
    expect_eq(computeCurrentAge(resp, 1025), 33);

    // Clock skew where now_sec < response_time_sec does not underflow (resident_time = 0)
    expect_eq(computeCurrentAge(resp, 990), 13);
});

// ============================================================================
// Test 4: Shared vs Private Storability Rules
// ============================================================================
test("cachePolicy: isResponseStorable enforces no-store, private and Authorization rules", () => {
    const cc_nostore = parseCacheControl("no-store, max-age=60") ?? return;
    expect_true(!isResponseStorable(StoredResponseMeta{ cache_control: cc_nostore }, CacheScope.private_cache));
    expect_true(!isResponseStorable(StoredResponseMeta{ cache_control: cc_nostore }, CacheScope.shared_cache));

    const cc_priv = parseCacheControl("private, max-age=60") ?? return;
    expect_true(isResponseStorable(StoredResponseMeta{ cache_control: cc_priv }, CacheScope.private_cache));
    expect_true(!isResponseStorable(StoredResponseMeta{ cache_control: cc_priv }, CacheScope.shared_cache));

    // Authorization: forbidden in shared cache by default, allowed with public / must-revalidate / s-maxage
    const cc_auth_plain = parseCacheControl("max-age=60") ?? return;
    const resp_auth_plain = StoredResponseMeta{
        status_code: 200,
        cache_control: cc_auth_plain,
        had_authorization: true,
    };
    expect_true(!isResponseStorable(resp_auth_plain, CacheScope.shared_cache));
    expect_true(isResponseStorable(resp_auth_plain, CacheScope.private_cache));

    const cc_auth_pub = parseCacheControl("public, max-age=60") ?? return;
    const resp_auth_pub = StoredResponseMeta{
        status_code: 200,
        cache_control: cc_auth_pub,
        had_authorization: true,
    };
    expect_true(isResponseStorable(resp_auth_pub, CacheScope.shared_cache));
});

// ============================================================================
// Test 5: Fresh Hit, stale-while-revalidate & stale-if-error Verdicts
// ============================================================================
test("cachePolicy: evaluateCacheLookup resolves fresh_hit, SWR, SIE and revalidate_required", () => {
    const cc = parseCacheControl("public, s-maxage=60, stale-while-revalidate=30, stale-if-error=300") ?? return;
    const resp = StoredResponseMeta{
        status_code: 200,
        cache_control: cc,
        request_time_sec: 1000,
        response_time_sec: 1000,
    };

    // t=1040 (age=40 < 60) -> fresh_hit (remaining=20)
    const d1 = evaluateCacheLookup(resp, CacheLookupRequest{ scope: CacheScope.shared_cache, now_sec: 1040 });
    expect_eq(d1.verdict, CacheLookupVerdict.fresh_hit);
    expect_eq(d1.remaining_fresh_sec, 20);
    expect_eq(d1.stale_for_sec, 0);

    // t=1075 (age=75, stale_for=15 <= SWR 30) -> stale_while_revalidate
    const d2 = evaluateCacheLookup(resp, CacheLookupRequest{ scope: CacheScope.shared_cache, now_sec: 1075 });
    expect_eq(d2.verdict, CacheLookupVerdict.stale_while_revalidate);
    expect_eq(d2.stale_for_sec, 15);

    // t=1150 (stale_for=90 > SWR 30, upstream healthy) -> revalidate_required
    const d3 = evaluateCacheLookup(resp, CacheLookupRequest{ scope: CacheScope.shared_cache, now_sec: 1150, upstream_error: false });
    expect_eq(d3.verdict, CacheLookupVerdict.revalidate_required);

    // t=1150 (stale_for=90 <= SIE 300, upstream_error=true) -> stale_if_error
    const d4 = evaluateCacheLookup(resp, CacheLookupRequest{ scope: CacheScope.shared_cache, now_sec: 1150, upstream_error: true });
    expect_eq(d4.verdict, CacheLookupVerdict.stale_if_error);

    // t=1400 (stale_for=340 > SIE 300, upstream_error=true) -> revalidate_required
    const d5 = evaluateCacheLookup(resp, CacheLookupRequest{ scope: CacheScope.shared_cache, now_sec: 1400, upstream_error: true });
    expect_eq(d5.verdict, CacheLookupVerdict.revalidate_required);
});

// ============================================================================
// Test 6: Request Directives vs must-revalidate & proxy-revalidate
// ============================================================================
test("cachePolicy: request no-store/no-cache/min-fresh/max-stale interact with must-revalidate", () => {
    const cc_base = parseCacheControl("max-age=60") ?? return;
    const resp = StoredResponseMeta{
        status_code: 200,
        cache_control: cc_base,
        request_time_sec: 1000,
        response_time_sec: 1000,
    };

    // Request no-store -> miss_bypass
    const req_ns = parseCacheControl("no-store") ?? return;
    const d_ns = evaluateCacheLookup(resp, CacheLookupRequest{
        scope: CacheScope.shared_cache,
        req_cc: req_ns,
        now_sec: 1010,
    });
    expect_eq(d_ns.verdict, CacheLookupVerdict.miss_bypass);

    // Request no-cache -> revalidate_required even when fresh
    const req_nc = parseCacheControl("no-cache") ?? return;
    const d_nc = evaluateCacheLookup(resp, CacheLookupRequest{
        scope: CacheScope.shared_cache,
        req_cc: req_nc,
        now_sec: 1010,
    });
    expect_eq(d_nc.verdict, CacheLookupVerdict.revalidate_required);

    // Request min-fresh=30 at age=40 (remaining=20 < 30) -> revalidate_required
    const req_mf = parseCacheControl("min-fresh=30") ?? return;
    const d_mf = evaluateCacheLookup(resp, CacheLookupRequest{
        scope: CacheScope.shared_cache,
        req_cc: req_mf,
        now_sec: 1040,
    });
    expect_eq(d_mf.verdict, CacheLookupVerdict.revalidate_required);

    // Stale at t=1080 (stale_for=20): client max-stale=30 allows fresh_hit without must-revalidate
    const req_ms = parseCacheControl("max-stale=30") ?? return;
    const d_ms_ok = evaluateCacheLookup(resp, CacheLookupRequest{
        scope: CacheScope.shared_cache,
        req_cc: req_ms,
        now_sec: 1080,
    });
    expect_eq(d_ms_ok.verdict, CacheLookupVerdict.fresh_hit);

    // With must-revalidate, client max-stale is forbidden -> revalidate_required
    const cc_mr = parseCacheControl("max-age=60, must-revalidate") ?? return;
    const resp_mr = StoredResponseMeta{
        status_code: 200,
        cache_control: cc_mr,
        request_time_sec: 1000,
        response_time_sec: 1000,
    };
    const d_ms_blocked = evaluateCacheLookup(resp_mr, CacheLookupRequest{
        scope: CacheScope.shared_cache,
        req_cc: req_ms,
        now_sec: 1080,
    });
    expect_eq(d_ms_blocked.verdict, CacheLookupVerdict.revalidate_required);
});

// ============================================================================
// Test 7: Malformed Syntax / Overflow / CRLF Boundary Guards & cap.web Facade
// ============================================================================
test("cachePolicy: boundary guards reject negative/overflow seconds, bad quotes, CRLF & facade works", () => {
    // Empty and whitespace-only headers are valid empty CacheControl
    expect_true(parseCacheControl("") != null);
    expect_true(parseCacheControl("   , , ") != null);

    // Negative numbers, non-digits, u64 overflow, missing value, unterminated quote, CRLF
    expect_true(parseCacheControl("max-age=-10") == null);
    expect_true(parseCacheControl("max-age=12abc") == null);
    expect_true(parseCacheControl("max-age=99999999999999999999999999") == null);
    expect_true(parseCacheControl("max-age") == null);
    expect_true(parseCacheControl("s-maxage=") == null);
    expect_true(parseCacheControl("max-age=\"60") == null);
    expect_true(parseCacheControl("public\r\nX-Injected: 1") == null);
    expect_true(parseCacheControl("max-stale=-5") == null);

    // cap.web facade smoke check
    const cc = cap.web.parseCacheControl("public, max-age=100") ?? return;
    const decision = cap.web.evaluateCacheLookup(
        StoredResponseMeta{ status_code: 200, cache_control: cc, request_time_sec: 500, response_time_sec: 500 },
        CacheLookupRequest{ scope: CacheScope.shared_cache, now_sec: 550 },
    );
    expect_eq(decision.verdict, CacheLookupVerdict.fresh_hit);
    expect_eq(decision.remaining_fresh_sec, 50);
});
