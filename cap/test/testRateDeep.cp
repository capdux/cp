import { test, expect_eq } from "cap:test";
import {
    NS_PER_SECOND,
    NS_PER_MINUTE,
    Quota,
    RateDecision,
    GcraLimiter,
    RateLimitHeaderLengths,
    formatRetryAfterSeconds,
    formatResetAfterSeconds,
    formatRateLimitHeaders,
    formatRetryAfterHeader,
    RateLimitError,
    TokenBucket,
    BucketGcraLimiter,
    SlidingWindowCounter,
    SlidingGcraLimiter,
    SimpleTokenBucket,
    FixedWindowLimiter,
} from "cap:rate";

fn bytesEq(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. Quota construction, burst tolerance, and zero/overflow clamping", () => {
        const q1 = Quota.perSecond(10);
        expect_eq(q1.emissionIntervalNs(), u64(100_000_000));
        expect_eq(q1.burstCapacity(), u64(10));
        expect_eq(q1.burstToleranceNs(), u64(900_000_000));

        const q2 = Quota.perMinute(60);
        expect_eq(q2.emissionIntervalNs(), NS_PER_SECOND);
        expect_eq(q2.burstCapacity(), u64(60));
        expect_eq(q2.burstToleranceNs(), u64(59) * NS_PER_SECOND);

        const q3 = q1.withBurst(3);
        expect_eq(q3.emissionIntervalNs(), u64(100_000_000));
        expect_eq(q3.burstCapacity(), u64(3));
        expect_eq(q3.burstToleranceNs(), u64(200_000_000));

        const q_single = q1.withBurst(1);
        expect_eq(q_single.burstToleranceNs(), u64(0));

        // Zero arguments safely clamped to 1
        const q_zero_sec = Quota.perSecond(0);
        expect_eq(q_zero_sec.emissionIntervalNs(), NS_PER_SECOND);
        expect_eq(q_zero_sec.burstCapacity(), u64(1));
        expect_eq(q_zero_sec.burstToleranceNs(), u64(0));

        const q_zero_min = Quota.perMinute(0);
        expect_eq(q_zero_min.emissionIntervalNs(), NS_PER_MINUTE);
        expect_eq(q_zero_min.burstCapacity(), u64(1));

        const q_zero_burst = q1.withBurst(0);
        expect_eq(q_zero_burst.burstCapacity(), u64(1));

        // Extreme burst tolerance saturates instead of overflowing u64
        const max_u64: u64 = std.math.maxInt(u64);
        const q_huge = Quota.perIntervalNs(NS_PER_SECOND, max_u64);
        expect_eq(q_huge.burstToleranceNs(), max_u64);
    });

    test("2. GcraLimiter burst admission, smooth TAT pacing, and non-advancing rejection", () => {
        // 10 req/s (T = 100ms), burst = 3
        let lim = GcraLimiter.init(Quota.perSecond(10).withBurst(3));
        const t0: u64 = 1_000_000_000;

        // Burst 1..3 admitted immediately at t0
        const d1 = lim.checkAt(t0);
        expect_eq(d1.allowed, true);
        expect_eq(d1.limit, u64(3));
        expect_eq(d1.remaining, u64(2));
        expect_eq(d1.retry_after_ns, u64(0));
        expect_eq(d1.reset_after_ns, u64(100_000_000));

        const d2 = lim.checkAt(t0);
        expect_eq(d2.allowed, true);
        expect_eq(d2.remaining, u64(1));
        expect_eq(d2.reset_after_ns, u64(200_000_000));

        const d3 = lim.checkAt(t0);
        expect_eq(d3.allowed, true);
        expect_eq(d3.remaining, u64(0));
        expect_eq(d3.reset_after_ns, u64(300_000_000));

        // 4th immediate request rejected; TAT must NOT advance
        const d4 = lim.checkAt(t0);
        expect_eq(d4.allowed, false);
        expect_eq(d4.remaining, u64(0));
        expect_eq(d4.limit, u64(3));
        expect_eq(d4.retry_after_ns, u64(100_000_000));
        expect_eq(d4.reset_after_ns, u64(300_000_000));

        // Repeated rejections at t0 + 20ms and t0 + 50ms do not push TAT forward
        const d5 = lim.checkAt(t0 + 20_000_000);
        expect_eq(d5.allowed, false);
        expect_eq(d5.retry_after_ns, u64(80_000_000));
        expect_eq(d5.reset_after_ns, u64(280_000_000));

        const d6 = lim.checkAt(t0 + 50_000_000);
        expect_eq(d6.allowed, false);
        expect_eq(d6.retry_after_ns, u64(50_000_000));
        expect_eq(d6.reset_after_ns, u64(250_000_000));

        // Read-only peek does not mutate state
        const p1 = lim.peek(t0 + 100_000_000);
        expect_eq(p1.allowed, true);
        expect_eq(p1.remaining, u64(1));

        // At t0 + 100ms, exactly 1 cell is admitted
        const d7 = lim.checkAt(t0 + 100_000_000);
        expect_eq(d7.allowed, true);
        expect_eq(d7.remaining, u64(0));
        expect_eq(d7.retry_after_ns, u64(0));
        expect_eq(d7.reset_after_ns, u64(300_000_000));

        // Immediately rejected again
        expect_eq(lim.allow(t0 + 100_000_000), false);

        // After full idle recovery (>= 300ms), full burst is restored
        const d8 = lim.checkAt(t0 + 1_000_000_000);
        expect_eq(d8.allowed, true);
        expect_eq(d8.remaining, u64(2));
        expect_eq(d8.reset_after_ns, u64(100_000_000));

        // Reset restores full burst even at the same timestamp
        lim.reset();
        const d9 = lim.checkAt(t0);
        expect_eq(d9.allowed, true);
        expect_eq(d9.remaining, u64(2));
    });

    test("3. GcraLimiter batch quota checkNAt atomic admission and impossible batch guard", () => {
        // 10 req/s (T = 100ms), burst = 5
        let lim = GcraLimiter.init(Quota.perSecond(10).withBurst(5));
        const t0: u64 = 5_000_000_000;

        // n = 0 is always allowed without advancing TAT
        const z0 = lim.checkNAt(0, t0);
        expect_eq(z0.allowed, true);
        expect_eq(z0.remaining, u64(5));
        expect_eq(z0.reset_after_ns, u64(0));

        // Batch of 3 succeeds -> 2 remaining, reset = 300ms
        const b1 = lim.checkNAt(3, t0);
        expect_eq(b1.allowed, true);
        expect_eq(b1.remaining, u64(2));
        expect_eq(b1.reset_after_ns, u64(300_000_000));

        // Batch of 3 fails atomically (2 remain, does NOT debit partial tokens)
        const b2 = lim.checkNAt(3, t0);
        expect_eq(b2.allowed, false);
        expect_eq(b2.remaining, u64(2));
        expect_eq(b2.retry_after_ns, u64(100_000_000));
        expect_eq(b2.reset_after_ns, u64(300_000_000));

        // After waiting retry_after_ns (100ms), batch of 3 succeeds!
        const b3 = lim.checkNAt(3, t0 + 100_000_000);
        expect_eq(b3.allowed, true);
        expect_eq(b3.remaining, u64(0));
        expect_eq(b3.reset_after_ns, u64(500_000_000));

        // Batch > max_burst (6 > 5) is rejected with U64_MAX retry_after_ns and TAT unchanged
        lim.reset();
        const too_big = lim.checkNAt(6, t0);
        expect_eq(too_big.allowed, false);
        expect_eq(too_big.remaining, u64(5));
        expect_eq(too_big.retry_after_ns, std.math.maxInt(u64));
        expect_eq(too_big.reset_after_ns, u64(0));
    });

    test("4. GcraLimiter clock rollback protection and U64_MAX overflow immunity", () => {
        let lim = GcraLimiter.init(Quota.perSecond(10).withBurst(2));
        expect_eq(lim.checkAt(1_000_000_000).allowed, true);
        expect_eq(lim.checkAt(1_000_000_000).allowed, true);
        expect_eq(lim.checkAt(1_000_000_000).allowed, false);

        // Clock rollback to 500ms must NOT grant free quota or underflow
        const rb = lim.checkAt(500_000_000);
        expect_eq(rb.allowed, false);
        expect_eq(rb.remaining, u64(0));
        expect_eq(rb.retry_after_ns, u64(600_000_000));

        // Timestamps near U64_MAX must not overflow internal arithmetic
        let lim_hi = GcraLimiter.init(Quota.perIntervalNs(100, 3));
        const near_max: u64 = std.math.maxInt(u64) - 255;
        const h1 = lim_hi.checkAt(near_max);
        expect_eq(h1.allowed, true);
        expect_eq(h1.remaining, u64(2));
        const h2 = lim_hi.checkAt(near_max);
        expect_eq(h2.allowed, true);
        expect_eq(h2.remaining, u64(1));
        const h3 = lim_hi.checkAt(near_max);
        expect_eq(h3.allowed, true);
        expect_eq(h3.remaining, u64(0));
        const h4 = lim_hi.checkAt(near_max);
        expect_eq(h4.allowed, false);
        expect_eq(h4.retry_after_ns > 0, true);
    });

    test("5. IETF Draft RateLimit and Retry-After header formatters (zero-alloc)", () => {
        // 1 req / 1.5s, burst = 12
        let lim = GcraLimiter.init(Quota.perIntervalNs(1_500_000_000, 12));
        const t0: u64 = 10_000_000_000;

        // Allowed decision: retry-after is 0, reset-after ceil(1.5s) = 2s
        const d_ok = lim.checkAt(t0);
        expect_eq(d_ok.allowed, true);
        expect_eq(formatRetryAfterSeconds(d_ok), u64(0));
        expect_eq(formatResetAfterSeconds(d_ok), u64(2));
        expect_eq(d_ok.retryAfterSeconds(), u64(0));
        expect_eq(d_ok.resetAfterSeconds(), u64(2));

        let lim_buf: [32]u8 = [0; 32];
        let rem_buf: [32]u8 = [0; 32];
        let rst_buf: [32]u8 = [0; 32];
        let retry_buf: [32]u8 = [0; 32];

        const lens = formatRateLimitHeaders(d_ok, lim_buf[0..], rem_buf[0..], rst_buf[0..]);
        expect_eq(bytesEq(lim_buf[0..lens.limit_len], "12"), true);
        expect_eq(bytesEq(rem_buf[0..lens.remaining_len], "11"), true);
        expect_eq(bytesEq(rst_buf[0..lens.reset_len], "2"), true);

        // Exhaust remaining 11 cells
        const d_drain = lim.checkNAt(11, t0);
        expect_eq(d_drain.allowed, true);
        expect_eq(d_drain.remaining, u64(0));

        // Next request rejected: retry_after_ns = 1.5s -> Retry-After: 2, Reset: 18s (12 * 1.5s)
        const d_rej = lim.checkAt(t0);
        expect_eq(d_rej.allowed, false);
        expect_eq(formatRetryAfterSeconds(d_rej), u64(2));
        expect_eq(formatResetAfterSeconds(d_rej), u64(18));

        const lens_rej = formatRateLimitHeaders(d_rej, lim_buf[0..], rem_buf[0..], rst_buf[0..]);
        const retry_len = formatRetryAfterHeader(d_rej, retry_buf[0..]);
        expect_eq(bytesEq(lim_buf[0..lens_rej.limit_len], "12"), true);
        expect_eq(bytesEq(rem_buf[0..lens_rej.remaining_len], "0"), true);
        expect_eq(bytesEq(rst_buf[0..lens_rej.reset_len], "18"), true);
        expect_eq(bytesEq(retry_buf[0..retry_len], "2"), true);

        // Tiny buffer returns 0 safely without out-of-bounds write
        let tiny: [1]u8 = [0; 1];
        const tiny_lens = formatRateLimitHeaders(d_rej, tiny[0..], rem_buf[0..], rst_buf[0..]);
        expect_eq(tiny_lens.limit_len, usize(0));
    });

    test("6. Backward compatibility: TokenBucket, SlidingWindowCounter, SimpleTokenBucket, FixedWindowLimiter", () => {
        // TokenBucket (bucket.cp)
        let tb = TokenBucket.init(10.0, 2.0, 0) catch unreachable;
        expect_eq(tb.allow(1.0, 0) catch unreachable, true);
        expect_eq(tb.allow(1.0, 0) catch unreachable, true);
        expect_eq(tb.allow(1.0, 0) catch unreachable, false);
        expect_eq(tb.retryAfterNs(1.0, 0), u64(100_000_000));
        expect_eq(tb.allow(1.0, 100_000_000) catch unreachable, true);

        // BucketGcraLimiter (bucket.cp)
        let bg = BucketGcraLimiter.init(10.0, 2) catch unreachable;
        expect_eq(bg.allow(1_000_000_000), true);
        expect_eq(bg.allow(1_000_000_000), true);
        expect_eq(bg.allow(1_000_000_000), false);

        // SlidingWindowCounter & SlidingGcraLimiter (sliding.cp)
        let sw = SlidingWindowCounter.init(5, 1000, 1000);
        expect_eq(sw.tryAcquire(5, 1000), true);
        expect_eq(sw.tryAcquire(1, 1000), false);
        expect_eq(sw.remaining(2500), u64(2)); // prev=5 * 0.5 = ceil(2.5) = 3 -> rem = 2

        let sg = SlidingGcraLimiter.init(100, 2, 1000);
        expect_eq(sg.checkAndAcquire(1000), u64(0));
        expect_eq(sg.checkAndAcquire(1000), u64(0));
        expect_eq(sg.checkAndAcquire(1000), u64(100));

        // SimpleTokenBucket & FixedWindowLimiter (limiter.cp)
        let stb = SimpleTokenBucket.init(4, 2, 0);
        expect_eq(stb.tryConsume(4, 0), true);
        expect_eq(stb.tryConsume(1, 0), false);
        expect_eq(stb.tryConsume(1, 500), true);

        let fw = FixedWindowLimiter.init(2, 1000, 0);
        expect_eq(fw.allow(0), true);
        expect_eq(fw.allow(100), true);
        expect_eq(fw.allow(200), false);
        expect_eq(fw.allow(1000), true);
    });
}
