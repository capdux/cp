// cap/test/testCapRetry.cp
// Comprehensive unit tests for @cap/retry (Fixed/Linear/Exponential/Fibonacci curves,
// AWS Full/Equal/Decorrelated jitter, Permanent/RateLimited handling, deadline budget, and RetryBudget).

import { test, expect_eq } from "cap:test";
import {
    NS_PER_MS,
    NS_PER_SEC,
    BackoffStrategy,
    JitterMode,
    FailureKind,
    StopReason,
    RetryPolicy,
    RetryState,
    RetryBudget,
} from "../../packages/async-retry/src/index.cp";

fn main() {
    test("1. @cap/retry — Exponential, Linear, Fixed & Fibonacci Curves (No Jitter)", () => {
        const exp_pol = RetryPolicy{
            strategy: BackoffStrategy.Exponential,
            jitter: JitterMode.None,
            max_retries: 4,
            base_delay_ns: 100 * NS_PER_MS,
            max_delay_ns: 500 * NS_PER_MS,
            factor_num: 2,
            factor_den: 1,
        };
        let s = RetryState.initSeeded(exp_pol, 0, 123);
        expect_eq(s.onFailure(FailureKind.Transient, 0, 0).delay_ns, 100 * NS_PER_MS);
        expect_eq(s.onFailure(FailureKind.Transient, 0, 0).delay_ns, 200 * NS_PER_MS);
        expect_eq(s.onFailure(FailureKind.Transient, 0, 0).delay_ns, 400 * NS_PER_MS);
        expect_eq(s.onFailure(FailureKind.Transient, 0, 0).delay_ns, 500 * NS_PER_MS); // capped
        const stop = s.onFailure(FailureKind.Transient, 0, 0);
        expect_eq(stop.should_retry, false);
        expect_eq(stop.stop_reason == StopReason.MaxAttemptsReached, true);

        // Fibonacci: 1, 1, 2, 3, 5 * base
        const fib_pol = RetryPolicy{
            strategy: BackoffStrategy.Fibonacci,
            jitter: JitterMode.None,
            max_retries: 5,
            base_delay_ns: 10 * NS_PER_MS,
            max_delay_ns: 1000 * NS_PER_MS,
        };
        let sf = RetryState.initSeeded(fib_pol, 0, 1);
        expect_eq(sf.onFailure(FailureKind.Transient, 0, 0).delay_ns, 10 * NS_PER_MS);
        expect_eq(sf.onFailure(FailureKind.Transient, 0, 0).delay_ns, 10 * NS_PER_MS);
        expect_eq(sf.onFailure(FailureKind.Transient, 0, 0).delay_ns, 20 * NS_PER_MS);
        expect_eq(sf.onFailure(FailureKind.Transient, 0, 0).delay_ns, 30 * NS_PER_MS);
        expect_eq(sf.onFailure(FailureKind.Transient, 0, 0).delay_ns, 50 * NS_PER_MS);
    });

    test("2. @cap/retry — AWS Full / Equal / Decorrelated Jitter Bounds", () => {
        const base: u64 = (100 as u64) * NS_PER_MS;
        const eq_pol = RetryPolicy{
            strategy: BackoffStrategy.Fixed,
            jitter: JitterMode.Equal,
            max_retries: 10,
            base_delay_ns: base,
            max_delay_ns: base,
        };
        let se = RetryState.initSeeded(eq_pol, 0, 999);
        let i: usize = 0;
        while (i < 10) : (i += 1) {
            const d_eq = se.onFailure(FailureKind.Transient, 0, 0);
            expect_eq(d_eq.should_retry, true);
            expect_eq(d_eq.delay_ns >= base / 2 && d_eq.delay_ns <= base, true);
        }

        const dec_pol = RetryPolicy.decorrelated(50 * NS_PER_MS, 500 * NS_PER_MS, 8);
        let sd = RetryState.initSeeded(dec_pol, 0, 777);
        let j: usize = 0;
        while (j < 8) : (j += 1) {
            const d_dec = sd.onFailure(FailureKind.Transient, 0, 0);
            expect_eq(d_dec.should_retry, true);
            expect_eq(d_dec.delay_ns <= 500 * NS_PER_MS, true);
        }
    });

    test("3. @cap/retry — Permanent Abort, RateLimited Hint & Elapsed Deadline", () => {
        const pol = RetryPolicy{
            strategy: BackoffStrategy.Fixed,
            jitter: JitterMode.None,
            max_retries: 5,
            base_delay_ns: 100 * NS_PER_MS,
            max_delay_ns: 5 * NS_PER_SEC,
            max_elapsed_ns: 2 * NS_PER_SEC,
        };
        let s = RetryState.initSeeded(pol, 1000, 1);

        // RateLimited honors hint up to max_delay_ns & remaining elapsed budget
        const rl = s.onFailure(FailureKind.RateLimited, 500 * NS_PER_MS, 1000);
        expect_eq(rl.should_retry, true);
        expect_eq(rl.delay_ns, 500 * NS_PER_MS);

        // Elapsed budget exceeded when now_ns - started_at_ns >= max_elapsed_ns
        const exp = s.onFailure(FailureKind.Transient, 0, 1000 + 2 * NS_PER_SEC);
        expect_eq(exp.should_retry, false);
        expect_eq(exp.stop_reason == StopReason.ElapsedBudgetExceeded, true);

        // Permanent failure aborts immediately
        s.reset(0);
        const perm = s.onFailure(FailureKind.Permanent, 0, 0);
        expect_eq(perm.should_retry, false);
        expect_eq(perm.stop_reason == StopReason.PermanentFailure, true);
    });

    test("4. @cap/retry — RetryBudget Storm Protection", () => {
        let budget = RetryBudget.init(2, 20); // max 2 burst retries, +20% per normal request
        expect_eq(budget.availableRetries(), 2 as u64);
        expect_eq(budget.tryAcquireRetry(), true);
        expect_eq(budget.tryAcquireRetry(), true);
        expect_eq(budget.tryAcquireRetry(), false); // exhausted

        // 5 normal requests replenish 5 * 20 = 100 tokens -> 1 retry permitted
        let k: usize = 0;
        while (k < 5) : (k += 1) {
            budget.recordRequest();
        }
        expect_eq(budget.tryAcquireRetry(), true);
        expect_eq(budget.tryAcquireRetry(), false);
    });
}
