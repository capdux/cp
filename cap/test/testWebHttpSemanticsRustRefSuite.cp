// Origin: Wave 36 R2 Reference-Hardening Verification Suite (Lane 2F)
// Reference-hardened against kornelski/rusty-http-cache-semantics (BSD-2-Clause) & RFC 9111 / RFC 9110 §12 / IETF RateLimit
// Covers:
//   1. RFC 9111 cachePolicy: quoted-string/unterminated quotes, overflow/negative delta-seconds, no-store > public/max-age,
//      private vs shared scope, clock skew saturating age/freshness, SWR/SIE lookup verdicts
//   2. RFC 9110 §12 contentNeg: strict qvalue (0..1, <=3 decimals, reject 1.001/4-digit/negative), invalid MIME rejection,
//      specificity (type/subtype > type/* > */*), Vary case-insensitive dedup & '*' collapse
//   3. IETF RateLimit & RFC 9110 Retry-After: delta-seconds overflow/negative, IMF-fixdate leap-year/weekday bounds,
//      CRLF/CTL rejection, retry planner precedence & max_delay_ms clamp
//   4. 16-lane @Vector(16, u8) SIMD vs scalar 0-diff parity (all 256 byte values + lengths 0..=257)
//   5. 1,800 deterministic PRNG fuzz cases (600 cache + 600 neg + 600 rate), 0 panic / 0 OOB / 0 overflow

import { test, expect_eq } from "cap:test";
import {
    scalarWebSemAsciiLowerEql,
    simdWebSemAsciiLowerEql,
    scalarWebSemFindDelim,
    simdWebSemFindDelim,
    CacheScope,
    CacheLookupVerdict,
    StoredResponseMeta,
    CacheLookupRequest,
    parseCacheControl,
    isResponseStorable,
    computeFreshnessLifetime,
    computeCurrentAge,
    evaluateCacheLookup,
} from "../../lib/web/cachePolicy.cp";
import {
    NEG_Q_MAX,
    ContentEncoding,
    NegotiationMatchKind,
    parseQValue,
    negotiateMediaType,
    negotiateEncoding,
    negotiateLanguage,
    formatVaryHeader,
} from "../../lib/web/contentNeg.cp";
import {
    RetryAfterKind,
    RetryDelaySource,
    RateLimitHeaderInfo,
    parseRateLimitHeader,
    formatRateLimitHeader,
    parseRateLimitPolicyHeader,
    parseRetryAfter,
    formatRetryAfterHttpDate,
    parseLegacyRateLimitHeaders,
    planRateLimitRetry,
} from "../../lib/web/rateHeader.cp";

fn nextRand(s: u64): u64 {
    let x: u64 = s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x;
}

fn randOut(s: u64): u64 {
    return s *% @as(u64, 0x2545F4914F6CDD1D);
}

// ============================================================================
// 1. RFC 9111 Cache-Control & Freshness/Age Compliance Vectors
// ============================================================================

fn checkCachePolicyVectors(): u32 {
    // 1. Quoted values, case-insensitivity, unknown extensions
    const cc1 = parseCacheControl("PuBlIc, Max-Age=\"60\", S-MaxAge=120, Stale-While-Revalidate=30, x-ext=\"a,b\"") ?? return 1;
    if (!cc1.is_public) return 2;
    if ((cc1.max_age ?? 0) != 60 || (cc1.s_maxage ?? 0) != 120 || (cc1.stale_while_revalidate ?? 0) != 30) return 3;

    // 2. Reject unterminated quote, negative/overflow delta-seconds, CRLF/control chars
    if (parseCacheControl("max-age=\"60") != null) return 4;
    if (parseCacheControl("max-age=-5") != null) return 5;
    if (parseCacheControl("max-age=999999999999999999999999") != null) return 6;
    if (parseCacheControl("s-maxage=") != null) return 7;
    if (parseCacheControl("public\r\nX-Inject: 1") != null) return 8;
    if (parseCacheControl("public\x07") != null) return 9;

    // 3. no-store overrides public + max-age; private blocks shared_cache but allows private_cache
    const cc_ns = parseCacheControl("public, max-age=3600, no-store") ?? return 10;
    const r_ns = StoredResponseMeta{ status_code: 200, cache_control: cc_ns };
    if (isResponseStorable(r_ns, CacheScope.shared_cache) || isResponseStorable(r_ns, CacheScope.private_cache)) return 11;

    const cc_priv = parseCacheControl("private, max-age=60") ?? return 12;
    const r_priv = StoredResponseMeta{ status_code: 200, cache_control: cc_priv };
    if (isResponseStorable(r_priv, CacheScope.shared_cache)) return 13;
    if (!isResponseStorable(r_priv, CacheScope.private_cache)) return 14;

    // 4. Clock skew (now < response_time, date > response_time) saturates cleanly at 0 (no underflow!)
    const r_skew = StoredResponseMeta{
        status_code: 200,
        cache_control: cc1,
        date_sec: 2000,
        request_time_sec: 1500,
        response_time_sec: 1000,
        age_header_sec: 10,
    };
    if (computeCurrentAge(r_skew, 500) != 10) return 15;
    if (computeFreshnessLifetime(r_skew, CacheScope.shared_cache) != 120) return 16;
    if (computeFreshnessLifetime(r_skew, CacheScope.private_cache) != 60) return 17;

    // 5. SWR verdict
    const d_swr = evaluateCacheLookup(StoredResponseMeta{
        status_code: 200,
        cache_control: cc1,
        request_time_sec: 1000,
        response_time_sec: 1000,
    }, CacheLookupRequest{
        scope: CacheScope.shared_cache,
        now_sec: 1135, // age=135, lifetime=120 -> stale_for=15 <= swr=30
    });
    if (d_swr.verdict != CacheLookupVerdict.stale_while_revalidate) return 18;
    return 0;
}

// ============================================================================
// 2. RFC 9110 §12 Content Negotiation & Vary Compliance Vectors
// ============================================================================

fn checkContentNegVectors(): u32 {
    // 1. Strict qvalue grammar
    if ((parseQValue("0") ?? 999) != 0) return 1;
    if ((parseQValue("0.5") ?? 999) != 500) return 2;
    if ((parseQValue("0.123") ?? 999) != 123) return 3;
    if ((parseQValue("1") ?? 999) != NEG_Q_MAX) return 4;
    if ((parseQValue("1.000") ?? 999) != NEG_Q_MAX) return 5;
    if (parseQValue("1.001") != null) return 6;
    if (parseQValue("0.1234") != null) return 7;
    if (parseQValue("-0.5") != null) return 8;
    if (parseQValue("2") != null) return 9;
    if (parseQValue("abc") != null) return 10;

    // 2. Invalid MIME rejected; specificity type/subtype > type/* > */*
    const offered: [3][]const u8 = ["text/plain", "text/html", "application/json"];
    if (negotiateMediaType("invalidmime", offered[0..]).status != NegotiationMatchKind.invalid_header) return 11;
    if (negotiateMediaType("*/html", offered[0..]).status != NegotiationMatchKind.invalid_header) return 12;
    if (negotiateMediaType("text/html\r\n", offered[0..]).status != NegotiationMatchKind.invalid_header) return 13;

    const m = negotiateMediaType("*/*;q=0.8, text/*;q=0.8, text/html;q=0.8", offered[0..]);
    if (m.status != NegotiationMatchKind.matched || m.offered_index != 1 || m.specificity != 2) return 14;

    // 3. Encoding & Language
    const encs: [2]ContentEncoding = [ContentEncoding.br, ContentEncoding.identity];
    const e1 = negotiateEncoding("identity;q=0, gzip;q=1", encs[0..]);
    if (e1.status != NegotiationMatchKind.not_acceptable) return 15;

    const langs: [2][]const u8 = ["en", "zh-Hans"];
    const l1 = negotiateLanguage("zh-Hans-CN, en;q=0.5", langs[0..]);
    if (l1.status != NegotiationMatchKind.matched || l1.offered_index != 1) return 16;

    // 4. Vary case-insensitive dedup & '*' absorption
    let vbuf: [128]u8 = [0; 128];
    let vs: []u8 = vbuf[0..];
    const add1: [2][]const u8 = ["accept-encoding", "Origin"];
    const vn = formatVaryHeader("Accept-Encoding, User-Agent", add1[0..], vs) ?? return 17;
    if (!simdWebSemAsciiLowerEql(vbuf[0..vn], "Accept-Encoding, User-Agent, Origin")) return 18;

    const add_star: [1][]const u8 = ["*"];
    const vn_star = formatVaryHeader("Origin, Accept", add_star[0..], vs) ?? return 19;
    if (vn_star != 1 || vbuf[0] != '*') return 20;
    return 0;
}

// ============================================================================
// 3. IETF RateLimit & RFC 9110 Retry-After Compliance Vectors
// ============================================================================

fn checkRateHeaderVectors(): u32 {
    // 1. RateLimit both draft & SF forms + CRLF/overflow/negative guards
    const rl1 = parseRateLimitHeader("limit=100, remaining=42, reset=15") ?? return 1;
    if (rl1.limit != 100 || rl1.remaining != 42 || rl1.reset_s != 15) return 2;

    const rl2 = parseRateLimitHeader("\"burst\";r=5;t=30;q=100") ?? return 3;
    if (rl2.remaining != 5 || rl2.reset_s != 30 || rl2.limit != 100) return 4;

    if (parseRateLimitHeader("limit=-1") != null) return 5;
    if (parseRateLimitHeader("limit=99999999999999999999999") != null) return 6;
    if (parseRateLimitHeader("limit=10\r\nX-Inj: 1") != null) return 7;
    if (parseRateLimitPolicyHeader("100;w=0") != null) return 8;

    let out: [128]u8 = [0; 128];
    let os: []u8 = out[0..];
    const rn = formatRateLimitHeader(rl2, true, os) ?? return 9;
    if (!simdWebSemAsciiLowerEql(out[0..rn], "\"burst\";r=5;t=30;q=100")) return 10;

    // 2. Retry-After delta-seconds & IMF-fixdate leap-year/weekday validation
    const ra1 = parseRetryAfter("120") ?? return 11;
    if (ra1.kind != RetryAfterKind.delta_seconds || ra1.delta_seconds != 120) return 12;
    if (parseRetryAfter("-5") != null) return 13;
    if (parseRetryAfter("99999999999999999999999") != null) return 14;

    const ra_date = parseRetryAfter("Wed, 21 Oct 2015 07:28:00 GMT") ?? return 15;
    if (ra_date.kind != RetryAfterKind.http_date || ra_date.http_date_epoch_s != 1445412480) return 16;
    // Wrong weekday ("Thu" instead of "Wed") & non-leap Feb 29 rejected
    if (parseRetryAfter("Thu, 21 Oct 2015 07:28:00 GMT") != null) return 17;
    if (parseRetryAfter("Thu, 29 Feb 2015 00:00:00 GMT") != null) return 18;
    const dn = formatRetryAfterHttpDate(1445412480, os) ?? return 19;
    if (!simdWebSemAsciiLowerEql(out[0..dn], "Wed, 21 Oct 2015 07:28:00 GMT")) return 20;

    // 3. Retry planner precedence & max_delay_ms clamp + extreme attempt shift safety
    const plan1 = planRateLimitRetry(RateLimitHeaderInfo{
        has_retry_after: true,
        retry_after: ra1,
        has_rate_limit: true,
        rate_limit: rl1,
    }, 0, 1000, 60_000);
    if (plan1.source != RetryDelaySource.retry_after_delta || plan1.delay_ms != 60_000 || !plan1.clamped) return 21;

    const plan_exp = planRateLimitRetry(RateLimitHeaderInfo{ base_backoff_ms: 0 }, 999, 1000, 30_000);
    if (plan_exp.source != RetryDelaySource.exponential_backoff || plan_exp.delay_ms != 30_000 || !plan_exp.clamped) return 22;

    if (parseLegacyRateLimitHeaders("100", "50", "1700000100", 1700000000) == null) return 23;
    return 0;
}

// ============================================================================
// 4. 16-Lane SIMD vs Scalar 0-Diff Parity (All 256 Bytes + Lengths 0..=257)
// ============================================================================

fn checkSimdParity(): u32 {
    let a: [32]u8 = ['a'; 32];
    let b: [32]u8 = ['A'; 32];
    let bi: usize = 0;
    while (bi < 256) : (bi += 1) {
        const ch: u8 = @as(u8, @intCast(bi));
        const lane: usize = bi % 17;
        a[lane] = ch;
        b[lane] = ch ^ 0x20;
        if (simdWebSemAsciiLowerEql(a[0..17], b[0..17]) != scalarWebSemAsciiLowerEql(a[0..17], b[0..17])) return 1;
        if (simdWebSemFindDelim(a[0..17], 0) != scalarWebSemFindDelim(a[0..17], 0)) return 2;
        if (simdWebSemFindDelim(a[0..17], 3) != scalarWebSemFindDelim(a[0..17], 3)) return 3;
        a[lane] = 'a';
        b[lane] = 'A';
    }

    let s1: [258]u8 = ['x'; 258];
    let s2: [258]u8 = ['X'; 258];
    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        if (len > 0) {
            s1[len - 1] = @as(u8, @intCast('a' + ((len - 1) % 26)));
            s2[len - 1] = @as(u8, @intCast('A' + ((len - 1) % 26)));
        }
        if (simdWebSemAsciiLowerEql(s1[0..len], s2[0..len]) != scalarWebSemAsciiLowerEql(s1[0..len], s2[0..len])) return 4;
        if (simdWebSemFindDelim(s1[0..len], 0) != scalarWebSemFindDelim(s1[0..len], 0)) return 5;
        if (len > 0) {
            const saved = s1[len - 1];
            const delims: [4]u8 = [',', ';', '=', '"'];
            s1[len - 1] = delims[len & 3];
            if (simdWebSemFindDelim(s1[0..len], 0) != scalarWebSemFindDelim(s1[0..len], 0)) return 6;
            if (simdWebSemFindDelim(s1[0..len], len / 2) != scalarWebSemFindDelim(s1[0..len], len / 2)) return 7;
            s1[len - 1] = saved;
        }
    }
    return 0;
}

// ============================================================================
// 5. 1,800 Deterministic PRNG Fuzz Cases (600 Cache + 600 Neg + 600 Rate)
// ============================================================================

fn checkPrngFuzz(): u32 {
    let seed: u64 = 0x3602_2026_9111_BEEF;
    let buf: [128]u8 = [0; 128];
    let out: [256]u8 = [0; 256];

    // 600 Cache-Control fuzz cases
    let ci: usize = 0;
    while (ci < 600) : (ci += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 96));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            const r = randOut(seed);
            if ((ci & 1) == 0) {
                const alph: []const u8 = "max-age=60,no-store,private,\"\\;\r\n\x00";
                buf[j] = alph[@as(usize, @intCast(r % alph.len))];
            } else {
                buf[j] = @as(u8, @intCast(r & 0xFF));
            }
        }
        if (cc := parseCacheControl(buf[0..n])) {
            const meta = StoredResponseMeta{
                status_code: 200,
                cache_control: cc,
                request_time_sec: 1000,
                response_time_sec: 1005,
            };
            * = evaluateCacheLookup(meta, CacheLookupRequest{ now_sec: 1050 });
        }
    }

    // 600 Content Negotiation + Vary fuzz cases
    const med_off: [2][]const u8 = ["application/json", "text/html"];
    const enc_off: [2]ContentEncoding = [ContentEncoding.gzip, ContentEncoding.identity];
    const lng_off: [2][]const u8 = ["en-US", "zh-CN"];
    const vary_add: [1][]const u8 = ["Accept-Encoding"];
    let ni: usize = 0;
    while (ni < 600) : (ni += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 96));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            const r = randOut(seed);
            if ((ni & 1) == 0) {
                const alph: []const u8 = "text/html;q=0.9,*/*,gzip,en-US,\"\r\n\x00";
                buf[j] = alph[@as(usize, @intCast(r % alph.len))];
            } else {
                buf[j] = @as(u8, @intCast(r & 0xFF));
            }
        }
        const s = buf[0..n];
        * = parseQValue(s);
        * = negotiateMediaType(s, med_off[0..]);
        * = negotiateEncoding(s, enc_off[0..]);
        * = negotiateLanguage(s, lng_off[0..]);
        let os: []u8 = out[0..];
        * = formatVaryHeader(s, vary_add[0..], os);
    }

    // 600 RateLimit + Retry-After fuzz cases
    let ri: usize = 0;
    while (ri < 600) : (ri += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 96));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            const r = randOut(seed);
            if ((ri & 1) == 0) {
                const alph: []const u8 = "\"def\";r=10;t=5;w=60,limit=99,Wed, 21 Oct 2015 07:28:00 GMT\r\n\x00";
                buf[j] = alph[@as(usize, @intCast(r % alph.len))];
            } else {
                buf[j] = @as(u8, @intCast(r & 0xFF));
            }
        }
        const s = buf[0..n];
        let info = RateLimitHeaderInfo{};
        if (rl := parseRateLimitHeader(s)) {
            info.has_rate_limit = true;
            info.rate_limit = rl;
            let os: []u8 = out[0..];
            * = formatRateLimitHeader(rl, (ri & 1) == 0, os);
        }
        * = parseRateLimitPolicyHeader(s);
        if (ra := parseRetryAfter(s)) {
            info.has_retry_after = true;
            info.retry_after = ra;
        }
        * = planRateLimitRetry(info, @as(u32, @intCast(ri % 40)), 1700000000, 60_000);
    }

    return 0;
}

fn main() {
    test("1. RFC 9111 cachePolicy: quoted values, no-store, private/shared, clock skew & SWR", () => {
        expect_eq(checkCachePolicyVectors(), @as(u32, 0));
    });
    test("2. RFC 9110 §12 contentNeg: strict qvalue, MIME specificity, Vary dedup & '*' collapse", () => {
        expect_eq(checkContentNegVectors(), @as(u32, 0));
    });
    test("3. IETF RateLimit & Retry-After: delta/IMF-fixdate bounds, CRLF & backoff planner", () => {
        expect_eq(checkRateHeaderVectors(), @as(u32, 0));
    });
    test("4. 16-lane SIMD vs Scalar 0-diff parity (all 256 bytes + lengths 0..=257)", () => {
        expect_eq(checkSimdParity(), @as(u32, 0));
    });
    test("5. PRNG Fuzz: 1,800 deterministic malformed cases (600 cache + 600 neg + 600 rate)", () => {
        expect_eq(checkPrngFuzz(), @as(u32, 0));
    });
}
