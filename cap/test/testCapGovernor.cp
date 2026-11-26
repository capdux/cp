// cap/test/testCapGovernor.cp
// Comprehensive unit tests for @cap/governor

import { test, expect_eq } from "cap:test";
import {
    RateDecision,
    GcraLimiter,
    TokenBucket,
} from "../../packages/@cap/governor/src/index.cp";

fn main() {
    test("1. @cap/governor — GCRA burst allowance, exhaustion & exact retry_after_ms", () => {
        // 10 req/sec => emission_interval = 100ms, burst = 3
        let lim = GcraLimiter.init(10, 3);
        expect_eq(lim.emission_interval_ms, 100 as u64);

        const d1 = lim.checkAt(1000);
        expect_eq(d1.allowed, true);
        expect_eq(d1.remaining, 2 as u64);
        expect_eq(d1.retry_after_ms, 0 as u64);

        const d2 = lim.checkAt(1000);
        expect_eq(d2.allowed, true);
        expect_eq(d2.remaining, 1 as u64);

        const d3 = lim.checkAt(1000);
        expect_eq(d3.allowed, true);
        expect_eq(d3.remaining, 0 as u64);
        expect_eq(d3.reset_after_ms, 300 as u64);

        // 4th request at same ms is denied with exact retry_after_ms = 100
        const d4 = lim.checkAt(1000);
        expect_eq(d4.allowed, false);
        expect_eq(d4.remaining, 0 as u64);
        expect_eq(d4.retry_after_ms, 100 as u64);
    });

    test("2. @cap/governor — GCRA automatic recovery as monotonic time advances", () => {
        let lim = GcraLimiter.init(10, 2); // T = 100ms, burst = 2
        expect_eq(lim.checkAt(500).allowed, true);
        expect_eq(lim.checkAt(500).allowed, true);
        expect_eq(lim.checkAt(500).allowed, false);

        // 99ms later: still 1ms short
        const early = lim.checkAt(599);
        expect_eq(early.allowed, false);
        expect_eq(early.retry_after_ms, 1 as u64);

        // 100ms later: exactly 1 slot replenished
        const ok = lim.checkAt(600);
        expect_eq(ok.allowed, true);
        expect_eq(ok.remaining, 0 as u64);

        // Long idle period (5 seconds later): full burst restored
        const idle = lim.checkAt(5600);
        expect_eq(idle.allowed, true);
        expect_eq(idle.remaining, 1 as u64);
    });

    test("3. @cap/governor — GCRA weighted checkNAt & perPeriod constructor", () => {
        // 5 tokens per 1000ms (T=200ms), burst=5
        let lim = GcraLimiter.perPeriod(5, 1000, 5);
        const d1 = lim.checkNAt(3, 1000);
        expect_eq(d1.allowed, true);
        expect_eq(d1.remaining, 2 as u64);

        // Asking for 3 more exceeds remaining 2 -> denied with retry = 200ms
        const d2 = lim.checkNAt(3, 1000);
        expect_eq(d2.allowed, false);
        expect_eq(d2.retry_after_ms, 200 as u64);

        // Asking for 2 succeeds immediately
        const d3 = lim.checkNAt(2, 1000);
        expect_eq(d3.allowed, true);
        expect_eq(d3.remaining, 0 as u64);
    });

    test("4. @cap/governor — TokenBucket acquisition, fractional refill & cap clamping", () => {
        // Capacity 5.0 tokens, refilling at 2.0 tokens/sec (1 token every 500ms)
        let tb = TokenBucket.init(5.0, 2.0, 0);
        const d1 = tb.tryAcquire(3.0, 0);
        expect_eq(d1.allowed, true);
        expect_eq(d1.remaining, 2 as u64);

        // Request 3.0 -> deficit 1.0 -> retry_after_ms = 500
        const d2 = tb.tryAcquire(3.0, 0);
        expect_eq(d2.allowed, false);
        expect_eq(d2.retry_after_ms, 500 as u64);

        // After 500ms, 1.0 token added => 3.0 available => succeeds
        const d3 = tb.tryAcquire(3.0, 500);
        expect_eq(d3.allowed, true);
        expect_eq(d3.remaining, 0 as u64);

        // After 10 seconds idle, balance is clamped to capacity 5.0
        const avail = tb.available(10500);
        expect_eq(avail >= 4.999 && avail <= 5.001, true);
    });

    test("5. @cap/governor — TokenBucket reset & zero-cost peek", () => {
        let tb = TokenBucket.init(4.0, 1.0, 100);
        expect_eq(tb.tryAcquire(4.0, 100).allowed, true);
        expect_eq(tb.tryAcquire(1.0, 100).allowed, false);

        tb.reset(100);
        const peek = tb.tryAcquire(0.0, 100);
        expect_eq(peek.allowed, true);
        expect_eq(peek.remaining, 4 as u64);
        expect_eq(peek.reset_after_ms, 0 as u64);
    });

    test("6. @cap/governor — GCRA reset & strict burst=1 spacing", () => {
        let strict = GcraLimiter.init(4, 1); // T=250ms, burst=1 (no burst)
        expect_eq(strict.checkAt(0).allowed, true);
        const d2 = strict.checkAt(0);
        expect_eq(d2.allowed, false);
        expect_eq(d2.retry_after_ms, 250 as u64);

        strict.reset();
        expect_eq(strict.checkAt(0).allowed, true);
    });
}
