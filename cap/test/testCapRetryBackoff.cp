import { test, expect_eq } from "cap:test";
import {
    RetryPolicy, BackoffState, computeBackoffMs, applyJitterMs
} from "../../packages/@cap/retry-backoff/src/index.cp";

fn main() {
    test("1. @cap/retry-backoff — Default exponential backoff sequence (100, 200, 400, 800, 1600)", () => {
        let p = RetryPolicy.exponential();
        expect_eq(computeBackoffMs(p, 0), 100);
        expect_eq(computeBackoffMs(p, 1), 200);
        expect_eq(computeBackoffMs(p, 2), 400);
        expect_eq(computeBackoffMs(p, 3), 800);
        expect_eq(computeBackoffMs(p, 4), 1600);
    });

    test("2. @cap/retry-backoff — Max delay cap prevents unbounded growth", () => {
        let p = RetryPolicy.exponential();
        p.max_delay_ms = 500;
        expect_eq(computeBackoffMs(p, 0), 100);
        expect_eq(computeBackoffMs(p, 2), 400);
        expect_eq(computeBackoffMs(p, 3), 500);
        expect_eq(computeBackoffMs(p, 20), 500);
    });

    test("3. @cap/retry-backoff — Constant retry policy & max_retries exhaustion", () => {
        let p = RetryPolicy.constant(3, 250);
        let st = BackoffState.new(p, 1);
        expect_eq(st.nextDelayMs() ?? 0, 250);
        expect_eq(st.nextDelayMs() ?? 0, 250);
        expect_eq(st.nextDelayMs() ?? 0, 250);
        expect_eq(st.nextDelayMs() == null, true);
        expect_eq(st.attempts(), 3);
        expect_eq(st.totalWaitedMs(), 750);
    });

    test("4. @cap/retry-backoff — Deterministic jitter stays within +/- jitter_bps bounds", () => {
        let d1 = applyJitterMs(1000, 2000, 12345); // +/- 20% -> [800, 1200]
        let d2 = applyJitterMs(1000, 2000, 12345);
        expect_eq(d1, d2); // deterministic for same seed
        expect_eq(d1 >= 800 && d1 <= 1200, true);
        expect_eq(applyJitterMs(1000, 0, 99), 1000);
    });

    test("5. @cap/retry-backoff — max_elapsed_ms budget stops retries early", () => {
        let p = RetryPolicy.exponential();
        p.max_retries = 10;
        p.max_elapsed_ms = 350; // allows 100 + 200 = 300, rejects 400
        let st = BackoffState.new(p, 42);
        expect_eq(st.nextDelayMs() ?? 0, 100);
        expect_eq(st.nextDelayMs() ?? 0, 200);
        expect_eq(st.nextDelayMs() == null, true);
        expect_eq(st.totalWaitedMs(), 300);
    });

    test("6. @cap/retry-backoff — reset restores attempt count & totalWaitedMs", () => {
        let p = RetryPolicy.exponential();
        let st = BackoffState.new(p, 7);
        st.nextDelayMs();
        st.nextDelayMs();
        st.reset();
        expect_eq(st.attempts(), 0);
        expect_eq(st.totalWaitedMs(), 0);
        expect_eq(st.nextDelayMs() ?? 0, 100);
    });
}
