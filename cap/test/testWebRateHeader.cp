// Origin: Pure-CP Stdlib Test Suite (lib/web/rateHeader.cp) | Created: 2026-11-23
// Production Deep Verification Suite for IETF RateLimit / RateLimit-Policy / Retry-After Codec & Retry Planner:
//   1. IETF `RateLimit` parsing & formatting (key-value & structured-field quoted policy forms)
//   2. IETF `RateLimit-Policy` parsing & formatting (quota, window, concurrency & partition key)
//   3. RFC 9110 `Retry-After` delta-seconds & IMF-fixdate HTTP-date bidirectional round-trip + leap-year check
//   4. Legacy `X-RateLimit-Limit` / `Remaining` / `Reset` (epoch vs delta) parsing & formatting
//   5. Unified `planRateLimitRetry` precedence (Retry-After > RateLimit reset > exponential backoff) & clamping
//   6. Buffer-overflow, integer-overflow, CRLF injection guards & `cap.web` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    RATE_HDR_MAX_NAME_LEN,
    RATE_HDR_MAX_PK_LEN,
    RateLimitHeader,
    RateLimitPolicyHeader,
    RetryAfterKind,
    RetryAfterValue,
    RetryDelaySource,
    RateLimitHeaderInfo,
    RetryPlan,
    parseRateLimitHeader,
    formatRateLimitHeader,
    parseRateLimitPolicyHeader,
    formatRateLimitPolicyHeader,
    parseRetryAfter,
    formatRetryAfterDelta,
    formatRetryAfterHttpDate,
    parseLegacyRateLimitHeaders,
    formatLegacyRateLimitHeaders,
    planRateLimitRetry,
} from "cap:web";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("RateHeader: IETF RateLimit key-value & structured-field quoted policy round-trip") {
    // Key-value form
    const rl1 = parseRateLimitHeader("limit=100, remaining=42, reset=15") ?? RateLimitHeader{};
    expect_true(rl1.has_limit && rl1.has_remaining && rl1.has_reset);
    expect_eq(rl1.limit, 100 as u64);
    expect_eq(rl1.remaining, 42 as u64);
    expect_eq(rl1.reset_s, 15 as u64);

    let buf: [128]u8 = [0; 128];
    let buf_s: []u8 = buf[0..];
    const n1 = formatRateLimitHeader(rl1, false, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n1], "limit=100, remaining=42, reset=15"));

    // Structured-field quoted policy form: `"default";r=42;t=15`
    const rl2 = parseRateLimitHeader("\"burst-tier\";r=7;t=25;q=50") ?? RateLimitHeader{};
    expect_true(rl2.has_policy_name);
    expect_true(bytesEq(rl2.policyNameSlice(), "burst-tier"));
    expect_eq(rl2.remaining, 7 as u64);
    expect_eq(rl2.reset_s, 25 as u64);
    expect_eq(rl2.limit, 50 as u64);

    const n2 = formatRateLimitHeader(rl2, true, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n2], "\"burst-tier\";r=7;t=25;q=50"));

    // Reject malformed inputs
    expect_true(parseRateLimitHeader("") == null);
    expect_true(parseRateLimitHeader("limit=-5, remaining=1") == null);
    expect_true(parseRateLimitHeader("\"unclosed;r=1;t=2") == null);
    expect_true(parseRateLimitHeader("\"empty\"") == null);
}

test("RateHeader: IETF RateLimit-Policy quota, window, concurrency & partition key codec") {
    const p1 = parseRateLimitPolicyHeader("100;w=60;c=12;pk=:c2hhcmQx:") ?? RateLimitPolicyHeader{};
    expect_eq(p1.quota, 100 as u64);
    expect_eq(p1.window_s, 60 as u64);
    expect_true(p1.has_concurrency);
    expect_eq(p1.concurrency, 12 as u32);
    expect_true(p1.has_partition_key);
    expect_true(bytesEq(p1.partitionKeySlice(), ":c2hhcmQx:"));

    let buf: [128]u8 = [0; 128];
    let buf_s: []u8 = buf[0..];
    const n1 = formatRateLimitPolicyHeader(p1, false, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n1], "100;w=60;c=12;pk=:c2hhcmQx:"));

    // Structured-field quoted policy form
    const p2 = parseRateLimitPolicyHeader("\"api-v2\";q=500;w=300;c=32") ?? RateLimitPolicyHeader{};
    expect_true(p2.has_policy_name);
    expect_true(bytesEq(p2.policyNameSlice(), "api-v2"));
    expect_eq(p2.quota, 500 as u64);
    expect_eq(p2.window_s, 300 as u64);
    expect_eq(p2.concurrency, 32 as u32);

    const n2 = formatRateLimitPolicyHeader(p2, true, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n2], "\"api-v2\";q=500;w=300;c=32"));

    // Reject zero window, missing window, or malformed quota
    expect_true(parseRateLimitPolicyHeader("100;w=0") == null);
    expect_true(parseRateLimitPolicyHeader("100") == null);
    expect_true(parseRateLimitPolicyHeader("abc;w=60") == null);
}

test("RateHeader: RFC 9110 Retry-After delta-seconds & IMF-fixdate HTTP-date round-trip") {
    // Delta-seconds
    const ra_delta = parseRetryAfter("  120 ") ?? RetryAfterValue{};
    expect_eq(ra_delta.kind, RetryAfterKind.delta_seconds);
    expect_eq(ra_delta.delta_seconds, 120 as u64);

    let buf: [64]u8 = [0; 64];
    let buf_s: []u8 = buf[0..];
    const nd = formatRetryAfterDelta(120, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..nd], "120"));

    // Known RFC 9110 example: Wed, 21 Oct 2015 07:28:00 GMT == 1445412480
    const ra_date = parseRetryAfter("Wed, 21 Oct 2015 07:28:00 GMT") ?? RetryAfterValue{};
    expect_eq(ra_date.kind, RetryAfterKind.http_date);
    expect_eq(ra_date.http_date_epoch_s, 1445412480 as i64);

    const nf = formatRetryAfterHttpDate(1445412480, *buf_s) ?? 0;
    expect_eq(nf, 29 as usize);
    expect_true(bytesEq(buf[0..nf], "Wed, 21 Oct 2015 07:28:00 GMT"));

    // Leap-year date: Thu, 29 Feb 2024 12:00:00 GMT == 1709208000
    const ra_leap = parseRetryAfter("Thu, 29 Feb 2024 12:00:00 GMT") ?? RetryAfterValue{};
    expect_eq(ra_leap.http_date_epoch_s, 1709208000 as i64);
    const nl = formatRetryAfterHttpDate(1709208000, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..nl], "Thu, 29 Feb 2024 12:00:00 GMT"));

    // Reject non-leap Feb 29, wrong weekday, non-GMT, negative delta
    expect_true(parseRetryAfter("Sat, 29 Feb 2025 12:00:00 GMT") == null);
    expect_true(parseRetryAfter("Mon, 21 Oct 2015 07:28:00 GMT") == null);
    expect_true(parseRetryAfter("Wed, 21 Oct 2015 07:28:00 UTC") == null);
    expect_true(parseRetryAfter("-10") == null);
}

test("RateHeader: legacy X-RateLimit-* parsing (epoch vs delta) & CRLF formatting") {
    // Unix epoch reset timestamp: 1700000045 - 1700000000 == 45s
    const leg1 = parseLegacyRateLimitHeaders("1000", "0", "1700000045", 1700000000) ?? RateLimitHeader{};
    expect_eq(leg1.limit, 1000 as u64);
    expect_eq(leg1.remaining, 0 as u64);
    expect_eq(leg1.reset_s, 45 as u64);

    // Past epoch timestamp clamps cleanly to 0
    const leg_past = parseLegacyRateLimitHeaders("100", "5", "1699999900", 1700000000) ?? RateLimitHeader{};
    expect_eq(leg_past.reset_s, 0 as u64);

    // Small reset value (< 1e9) treated as delta-seconds directly
    const leg_delta = parseLegacyRateLimitHeaders("60", "12", "30", 1700000000) ?? RateLimitHeader{};
    expect_eq(leg_delta.reset_s, 30 as u64);

    // Format back as CRLF header lines (both delta and epoch modes)
    let buf: [160]u8 = [0; 160];
    let buf_s: []u8 = buf[0..];
    const n_ep = formatLegacyRateLimitHeaders(leg1, true, 1700000000, *buf_s) ?? 0;
    expect_true(bytesEq(
        buf[0..n_ep],
        "X-RateLimit-Limit: 1000\r\nX-RateLimit-Remaining: 0\r\nX-RateLimit-Reset: 1700000045\r\n",
    ));

    expect_true(parseLegacyRateLimitHeaders("", "", "", 1700000000) == null);
    expect_true(parseLegacyRateLimitHeaders("100", "bad", "10", 1700000000) == null);
}

test("RateHeader: unified planRateLimitRetry precedence, past-date clamp & exponential backoff") {
    const now_s: i64 = 1700000000;

    // 1. Retry-After (delta) takes priority over RateLimit reset
    let info1 = RateLimitHeaderInfo{};
    info1.has_retry_after = true;
    info1.retry_after = RetryAfterValue{ kind: RetryAfterKind.delta_seconds, delta_seconds: 12 };
    info1.has_rate_limit = true;
    info1.rate_limit = RateLimitHeader{ has_reset: true, reset_s: 45 };

    const p1 = planRateLimitRetry(info1, 2, now_s, 60000);
    expect_eq(p1.delay_ms, 12000 as u64);
    expect_eq(p1.source, RetryDelaySource.retry_after_delta);
    expect_eq(p1.clamped, false);

    // 2. Retry-After (HTTP-date) clamped by max_delay_ms
    let info2 = RateLimitHeaderInfo{};
    info2.has_retry_after = true;
    info2.retry_after = RetryAfterValue{ kind: RetryAfterKind.http_date, http_date_epoch_s: now_s + 90 };

    const p2 = planRateLimitRetry(info2, 0, now_s, 30000);
    expect_eq(p2.delay_ms, 30000 as u64);
    expect_eq(p2.source, RetryDelaySource.retry_after_date);
    expect_true(p2.clamped);

    // Past HTTP-date -> retry immediately (0ms)
    info2.retry_after.http_date_epoch_s = now_s - 20;
    const p2_past = planRateLimitRetry(info2, 0, now_s, 30000);
    expect_eq(p2_past.delay_ms, 0 as u64);
    expect_true(p2_past.retry_immediately);

    // 3. Fallback to RateLimit reset when Retry-After absent
    let info3 = RateLimitHeaderInfo{};
    info3.has_rate_limit = true;
    info3.rate_limit = RateLimitHeader{ has_reset: true, reset_s: 8 };
    const p3 = planRateLimitRetry(info3, 1, now_s, 30000);
    expect_eq(p3.delay_ms, 8000 as u64);
    expect_eq(p3.source, RetryDelaySource.rate_limit_reset);

    // 4. Fallback to exponential backoff when both headers absent
    let info4 = RateLimitHeaderInfo{ base_backoff_ms: 250 };
    const b0 = planRateLimitRetry(info4, 0, now_s, 10000);
    const b3 = planRateLimitRetry(info4, 3, now_s, 10000);
    const b_hi = planRateLimitRetry(info4, 30, now_s, 10000);
    expect_eq(b0.delay_ms, 250 as u64);
    expect_eq(b0.source, RetryDelaySource.exponential_backoff);
    expect_eq(b3.delay_ms, 2000 as u64);
    expect_eq(b_hi.delay_ms, 10000 as u64);
    expect_true(b_hi.clamped);
}

test("RateHeader: CRLF injection, integer overflow, tiny buffer guards & cap.web facade parity") {
    // CRLF injection blocked across all parsers
    expect_true(parseRateLimitHeader("limit=10\r\nX-Injected: 1") == null);
    expect_true(parseRateLimitPolicyHeader("100;w=60\nInjected: 1") == null);
    expect_true(parseRetryAfter("10\r\nSet-Cookie: x") == null);
    expect_true(parseLegacyRateLimitHeaders("100\r\n", "50", "10", 1700000000) == null);

    // u64 overflow rejected strictly (18446744073709551616 == 2^64)
    expect_true(parseRetryAfter("18446744073709551616") == null);
    expect_true(parseRateLimitHeader("limit=99999999999999999999999") == null);

    // Tiny output buffers return null safely without overflow
    let tiny: [5]u8 = [0; 5];
    let tiny_s: []u8 = tiny[0..];
    const rl = parseRateLimitHeader("limit=100, remaining=42, reset=15") ?? RateLimitHeader{};
    expect_true(formatRateLimitHeader(rl, false, *tiny_s) == null);
    expect_true(formatRateLimitHeader(rl, true, *tiny_s) == null);
    expect_true(formatRetryAfterHttpDate(1445412480, *tiny_s) == null);
    expect_true(formatLegacyRateLimitHeaders(rl, false, 0, *tiny_s) == null);

    // cap.web namespace facade parity
    const facade_rl = cap.web.parseRateLimitHeader("\"edge\";r=9;t=5") ?? cap.web.RateLimitHeader{};
    expect_eq(facade_rl.remaining, 9 as u64);
    expect_eq(facade_rl.reset_s, 5 as u64);
    expect_eq(RATE_HDR_MAX_NAME_LEN, 48 as usize);
    expect_eq(RATE_HDR_MAX_PK_LEN, 64 as usize);
}
