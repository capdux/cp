// Origin: Pure-CP Stdlib Test Suite (lib/web/etag_precondition.cp) | Created: 2026-11-22
// Production Deep Verification Suite for RFC 9110 §13 Conditional Request & ETag Precondition Evaluator:
//   1. parseEntityTag strict RFC 9110 syntax & strong/weak comparison matrix
//   2. Quote-aware comma-separated ETag list ("a,b", W/"c") & validateEtagList
//   3. Step 1 If-Match (strong match, weak rejection -> 412, overrides If-Unmodified-Since)
//   4. Step 2 If-Unmodified-Since when If-Match is absent
//   5. Step 3 If-None-Match (GET/HEAD -> 304, PUT/POST/DELETE/PATCH -> 412, overrides If-Modified-Since)
//   6. Step 4 If-Modified-Since (GET/HEAD only, ignored on write methods)
//   7. Step 5 If-Range (strong ETag / exact date -> 206, weak/mismatch -> fallback_full_200)
//   8. Non-existent resource wildcard rules (If-Match:* -> 412, If-None-Match:* -> 200) & cap.web namespace

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PreconditionMethod,
    PreconditionVerdict,
    PreconditionTrigger,
    EntityTag,
    PreconditionRequest,
    ResourceValidators,
    parseEntityTag,
    etagStrongEqual,
    etagWeakEqual,
    etagListMatchesStrong,
    etagListMatchesWeak,
    validateEtagList,
    evaluatePreconditions,
} from "cap:web";

// ============================================================================
// Test 1: parseEntityTag Strict Syntax & Strong/Weak Comparison Matrix
// ============================================================================

fn testParseEntityTagAndComparators()! {
    const strong1 = parseEntityTag("  \"v1\"  ") ?? return error.ParseStrong1;
    const strong2 = parseEntityTag("\"v1\"") ?? return error.ParseStrong2;
    const weak1 = parseEntityTag("W/\"v1\"") ?? return error.ParseWeak1;
    const weak_empty = parseEntityTag("W/\"\"") ?? return error.ParseWeakEmpty;

    expect_eq(strong1.weak, false);
    expect_eq(weak1.weak, true);
    expect_eq(weak_empty.tag.len, 0);

    // RFC 9110 §8.8.3.2 comparison matrix
    expect_eq(etagStrongEqual(strong1, strong2), true);
    expect_eq(etagStrongEqual(strong1, weak1), false);
    expect_eq(etagStrongEqual(weak1, weak1), false);
    expect_eq(etagWeakEqual(strong1, strong2), true);
    expect_eq(etagWeakEqual(strong1, weak1), true);
    expect_eq(etagWeakEqual(weak1, weak1), true);
    expect_eq(etagWeakEqual(weak1, weak_empty), false);

    // Strict rejections
    expect_eq(parseEntityTag("w/\"v1\"") == null, true);       // lowercase w/ illegal
    expect_eq(parseEntityTag("\"unclosed") == null, true);     // unclosed quote
    expect_eq(parseEntityTag("v1") == null, true);             // missing quotes
    expect_eq(parseEntityTag("\"has space\"") == null, true);  // 0x20 not in etagc
    expect_eq(parseEntityTag("\"bad\"quote\"") == null, true); // 0x22 not in etagc
    expect_eq(parseEntityTag("") == null, true);
}

// ============================================================================
// Test 2: Quote-Aware Comma-Separated ETag List & validateEtagList
// ============================================================================

fn testQuoteAwareEtagList()! {
    const cur_comma = parseEntityTag("\"a,b\"") ?? return error.ParseCommaTag;
    const cur_c = parseEntityTag("\"c\"") ?? return error.ParseCTag;

    const list_hdr: []const u8 = "\"x\", \"a,b\", W/\"c\"";
    expect_eq(validateEtagList(list_hdr), true);
    expect_eq(validateEtagList("*"), true);
    expect_eq(validateEtagList("\"ok\", w/\"bad\""), false);
    expect_eq(validateEtagList("\"unclosed"), false);
    expect_eq(validateEtagList("   "), false);

    // Quoted comma inside `"a,b"` must NOT split the token
    expect_eq(etagListMatchesStrong(list_hdr, cur_comma, true), true);
    // `W/"c"` fails strong match against `"c"`, but passes weak match
    expect_eq(etagListMatchesStrong(list_hdr, cur_c, true), false);
    expect_eq(etagListMatchesWeak(list_hdr, cur_c, true), true);
}

// ============================================================================
// Test 3: Step 1 If-Match Precedence & Strong Comparison
// ============================================================================

fn testStep1IfMatch()! {
    const res = ResourceValidators{
        resource_exists: true,
        etag: "\"rev-10\"",
        last_modified_sec: 2000,
    };

    // Weak ETag in If-Match must fail strong comparison -> 412
    const r_weak = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.put,
        if_match: "W/\"rev-10\"",
    }, res);
    expect_eq(r_weak.verdict == PreconditionVerdict.precondition_failed_412, true);
    expect_eq(r_weak.http_status, 412);
    expect_eq(r_weak.trigger == PreconditionTrigger.if_match, true);

    // Strong match passes AND suppresses If-Unmodified-Since even though lm(2000) > ius(1000)
    const r_pass = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.put,
        if_match: "\"rev-09\", \"rev-10\"",
        if_unmodified_since_sec: 1000,
    }, res);
    expect_eq(r_pass.verdict == PreconditionVerdict.proceed_200, true);
    expect_eq(r_pass.http_status, 200);
}

// ============================================================================
// Test 4: Step 2 If-Unmodified-Since (When If-Match Absent)
// ============================================================================

fn testStep2IfUnmodifiedSince()! {
    const res = ResourceValidators{
        resource_exists: true,
        etag: "\"rev-10\"",
        last_modified_sec: 2000,
    };

    // Modified after ius (2000 > 1500) -> 412
    const r_fail = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.patch,
        if_unmodified_since_sec: 1500,
    }, res);
    expect_eq(r_fail.verdict == PreconditionVerdict.precondition_failed_412, true);
    expect_eq(r_fail.trigger == PreconditionTrigger.if_unmodified_since, true);

    // Not modified after ius (2000 <= 2000) -> 200
    const r_ok = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.patch,
        if_unmodified_since_sec: 2000,
    }, res);
    expect_eq(r_ok.verdict == PreconditionVerdict.proceed_200, true);
}

// ============================================================================
// Test 5: Step 3 If-None-Match (GET/HEAD -> 304, Write -> 412, Overrides IMS)
// ============================================================================

fn testStep3IfNoneMatch()! {
    const res = ResourceValidators{
        resource_exists: true,
        etag: "\"v5\"",
        last_modified_sec: 1000,
    };

    // GET with weak match -> 304 Not Modified
    const r_get = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        if_none_match: "W/\"v5\"",
    }, res);
    expect_eq(r_get.verdict == PreconditionVerdict.not_modified_304, true);
    expect_eq(r_get.http_status, 304);
    expect_eq(r_get.trigger == PreconditionTrigger.if_none_match, true);

    // HEAD with weak match -> 304 Not Modified
    const r_head = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.head,
        if_none_match: "\"v5\"",
    }, res);
    expect_eq(r_head.verdict == PreconditionVerdict.not_modified_304, true);

    // Write methods (PUT / DELETE / POST / PATCH) hitting If-None-Match -> 412!
    const r_put = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.put,
        if_none_match: "*",
    }, res);
    expect_eq(r_put.verdict == PreconditionVerdict.precondition_failed_412, true);
    expect_eq(r_put.http_status, 412);
    expect_eq(r_put.trigger == PreconditionTrigger.if_none_match, true);

    // If-None-Match present & does NOT match -> overrides If-Modified-Since (must proceed 200!)
    const r_override = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        if_none_match: "\"v4\"",
        if_modified_since_sec: 9999,
    }, res);
    expect_eq(r_override.verdict == PreconditionVerdict.proceed_200, true);
}

// ============================================================================
// Test 6: Step 4 If-Modified-Since (GET/HEAD Only)
// ============================================================================

fn testStep4IfModifiedSince()! {
    const res = ResourceValidators{
        resource_exists: true,
        etag: "\"v5\"",
        last_modified_sec: 1000,
    };

    // GET & lm(1000) <= ims(1000) -> 304
    const r_304 = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        if_modified_since_sec: 1000,
    }, res);
    expect_eq(r_304.verdict == PreconditionVerdict.not_modified_304, true);
    expect_eq(r_304.trigger == PreconditionTrigger.if_modified_since, true);

    // GET & lm(1000) > ims(900) -> 200
    const r_200 = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        if_modified_since_sec: 900,
    }, res);
    expect_eq(r_200.verdict == PreconditionVerdict.proceed_200, true);

    // PUT ignores If-Modified-Since -> 200
    const r_put = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.put,
        if_modified_since_sec: 5000,
    }, res);
    expect_eq(r_put.verdict == PreconditionVerdict.proceed_200, true);
}

// ============================================================================
// Test 7: Step 5 If-Range (Strong ETag & Exact Date -> 206 vs Fallback 200)
// ============================================================================

fn testStep5IfRange()! {
    const res = ResourceValidators{
        resource_exists: true,
        etag: "\"blob-9\"",
        last_modified_sec: 1700000100,
    };

    // 1. Strong ETag match -> 206 Partial Content
    const r_etag_ok = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        has_range: true,
        if_range_etag: "\"blob-9\"",
    }, res);
    expect_eq(r_etag_ok.verdict == PreconditionVerdict.serve_range_206, true);
    expect_eq(r_etag_ok.http_status, 206);
    expect_eq(r_etag_ok.serve_range, true);
    expect_eq(r_etag_ok.trigger == PreconditionTrigger.if_range_matched, true);

    // 2. Weak ETag in If-Range -> fallback full 200 (RFC 9110 §13.1.5 requires strong match)
    const r_etag_weak = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        has_range: true,
        if_range_etag: "W/\"blob-9\"",
    }, res);
    expect_eq(r_etag_weak.verdict == PreconditionVerdict.fallback_full_200, true);
    expect_eq(r_etag_weak.http_status, 200);
    expect_eq(r_etag_weak.serve_range, false);
    expect_eq(r_etag_weak.trigger == PreconditionTrigger.if_range_mismatched, true);

    // 3. Exact timestamp match -> 206; mismatched timestamp -> fallback 200
    const r_date_ok = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        has_range: true,
        if_range_date_sec: 1700000100,
    }, res);
    expect_eq(r_date_ok.verdict == PreconditionVerdict.serve_range_206, true);

    const r_date_miss = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        has_range: true,
        if_range_date_sec: 1700000099,
    }, res);
    expect_eq(r_date_miss.verdict == PreconditionVerdict.fallback_full_200, true);

    // 4. Range without If-Range -> 206
    const r_plain = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        has_range: true,
    }, res);
    expect_eq(r_plain.verdict == PreconditionVerdict.serve_range_206, true);
}

// ============================================================================
// Test 8: Missing Resource Wildcard Semantics & cap.web Namespace Access
// ============================================================================

fn testMissingResourceWildcardAndNamespace()! {
    const missing = ResourceValidators{
        resource_exists: false,
        etag: "",
        last_modified_sec: null,
    };

    // If-Match: * on non-existent resource MUST return 412
    const r_im = cap.web.evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.put,
        if_match: "*",
    }, missing);
    expect_eq(r_im.verdict == PreconditionVerdict.precondition_failed_412, true);
    expect_eq(r_im.http_status, 412);

    // If-None-Match: * on non-existent resource MUST allow creation (200 proceed)
    const r_inm = cap.web.evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.put,
        if_none_match: "*",
    }, missing);
    expect_eq(r_inm.verdict == PreconditionVerdict.proceed_200, true);
    expect_eq(r_inm.http_status, 200);
}

// ============================================================================
// Test Entrypoints
// ============================================================================

test("etag_precondition: parseEntityTag strict syntax & strong/weak comparators", testParseEntityTagAndComparators);
test("etag_precondition: quote-aware comma-separated ETag list & validateEtagList", testQuoteAwareEtagList);
test("etag_precondition: Step 1 If-Match strong comparison & override precedence", testStep1IfMatch);
test("etag_precondition: Step 2 If-Unmodified-Since when If-Match absent", testStep2IfUnmodifiedSince);
test("etag_precondition: Step 3 If-None-Match (304 vs 412) & override precedence", testStep3IfNoneMatch);
test("etag_precondition: Step 4 If-Modified-Since (GET/HEAD only)", testStep4IfModifiedSince);
test("etag_precondition: Step 5 If-Range (strong ETag / exact date -> 206 vs fallback 200)", testStep5IfRange);
test("etag_precondition: missing resource wildcard semantics & cap.web namespace", testMissingResourceWildcardAndNamespace);
