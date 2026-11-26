// cap/test/testCapBackoff.cp
// Comprehensive unit tests for @cap/backoff

import { test, expect_eq } from "cap:test";
import {
    JitterKind,
    BackoffPolicy,
    BackoffState,
    expDelayMs,
    linearDelayMs,
} from "../../packages/@cap/backoff/src/index.cp";

fn main() {
    test("1. @cap/backoff — Exact exponential doubling, capping & max_retries exhaustion (JitterKind.None)", () => {
        const policy = BackoffPolicy{
            initial_ms: 100 as u64,
            max_ms: 500 as u64,
            multiplier_num: 2 as u64,
            multiplier_den: 1 as u64,
            max_retries: 5 as u32,
            jitter: JitterKind.None,
        };
        let st = BackoffState.init(policy, 42 as u64);
        expect_eq(st.attempts(), 0 as u32);
        expect_eq(st.nextDelayMs() ?? (9999 as u64), 100 as u64);
        expect_eq(st.nextDelayMs() ?? (9999 as u64), 200 as u64);
        expect_eq(st.nextDelayMs() ?? (9999 as u64), 400 as u64);
        expect_eq(st.nextDelayMs() ?? (9999 as u64), 500 as u64);
        expect_eq(st.nextDelayMs() ?? (9999 as u64), 500 as u64);
        expect_eq(st.attempts(), 5 as u32);
        expect_eq(st.nextDelayMs() == null, true);

        st.reset();
        expect_eq(st.attempts(), 0 as u32);
        expect_eq(st.nextDelayMs() ?? (9999 as u64), 100 as u64);
    });

    test("2. @cap/backoff — Stateless expDelayMs overflow safety at attempt=100", () => {
        expect_eq(expDelayMs(0 as u32, 100 as u64, 30000 as u64), 100 as u64);
        expect_eq(expDelayMs(1 as u32, 100 as u64, 30000 as u64), 200 as u64);
        expect_eq(expDelayMs(3 as u32, 100 as u64, 30000 as u64), 800 as u64);
        expect_eq(expDelayMs(100 as u32, 100 as u64, 30000 as u64), 30000 as u64);
        expect_eq(expDelayMs(100 as u32, 0 as u64, 30000 as u64), 0 as u64);
    });

    test("3. @cap/backoff — Stateless linearDelayMs & capping", () => {
        expect_eq(linearDelayMs(0 as u32, 250 as u64, 1000 as u64), 250 as u64);
        expect_eq(linearDelayMs(1 as u32, 250 as u64, 1000 as u64), 500 as u64);
        expect_eq(linearDelayMs(3 as u32, 250 as u64, 1000 as u64), 1000 as u64);
        expect_eq(linearDelayMs(50 as u32, 250 as u64, 1000 as u64), 1000 as u64);
    });

    test("4. @cap/backoff — JitterKind.Full range [0, cap] & determinism", () => {
        const policy = BackoffPolicy.defaultPolicy();
        let s1 = BackoffState.init(policy, 7777 as u64);
        let s2 = BackoffState.init(policy, 7777 as u64);

        let i: u32 = 0;
        while (i < 5) {
            const cap = expDelayMs(i, 100 as u64, 30000 as u64);
            const d1 = s1.nextDelayMs() ?? (999999 as u64);
            const d2 = s2.nextDelayMs() ?? (999999 as u64);
            expect_eq(d1, d2);
            expect_eq(d1 <= cap, true);
            i += 1 as u32;
        }
        expect_eq(s1.nextDelayMs() == null, true);
    });

    test("5. @cap/backoff — JitterKind.Equal range [cap/2, cap]", () => {
        const policy = BackoffPolicy{
            initial_ms: 200 as u64,
            max_ms: 5000 as u64,
            multiplier_num: 2 as u64,
            multiplier_den: 1 as u64,
            max_retries: 6 as u32,
            jitter: JitterKind.Equal,
        };
        let st = BackoffState.init(policy, 999 as u64);
        let i: u32 = 0;
        while (i < 6) {
            const cap = expDelayMs(i, 200 as u64, 5000 as u64);
            const d = st.nextDelayMs() ?? (0 as u64);
            expect_eq(d >= (cap / (2 as u64)), true);
            expect_eq(d <= cap, true);
            i += 1 as u32;
        }
    });

    test("6. @cap/backoff — AWS JitterKind.Decorrelated bounds [base, min(max_ms, prev*3)]", () => {
        const policy = BackoffPolicy{
            initial_ms: 100 as u64,
            max_ms: 2000 as u64,
            multiplier_num: 2 as u64,
            multiplier_den: 1 as u64,
            max_retries: 8 as u32,
            jitter: JitterKind.Decorrelated,
        };
        let st = BackoffState.init(policy, 123456 as u64);
        let prev: u64 = 100;
        let i: u32 = 0;
        while (i < 8) {
            const upperRaw = prev * (3 as u64);
            let upper: u64 = upperRaw;
            if (upper > (2000 as u64)) {
                upper = 2000 as u64;
            }
            const d = st.nextDelayMs() ?? (0 as u64);
            expect_eq(d >= (100 as u64), true);
            expect_eq(d <= upper, true);
            prev = d;
            i += 1 as u32;
        }
        expect_eq(st.nextDelayMs() == null, true);
    });
}
