import { test, expect_eq } from "cap:test";
import {
    PT_PENDING,
    PT_RESOLVED,
    PT_REJECTED,
    PT_TIMED_OUT,
    PT_CANCELED,
    TimeoutOptions,
    TimeoutGuard,
} from "../../packages/@cap/p-timeout-core/src/index.cp";

fn main() {
    test("1. @cap/p-timeout-core — computes deadline, remainingMs, expiry, and saturates on overflow", () => {
        let opts: TimeoutOptions = TimeoutOptions.withMs(500);
        let g: TimeoutGuard = TimeoutGuard.new(1000, opts);

        expect_eq(g.deadlineMs(), 1500);
        expect_eq(g.remainingMs(1200), 300);
        expect_eq(g.isExpired(1499), false);
        expect_eq(g.isExpired(1500), true);
        expect_eq(g.remainingMs(1600), 0);
        expect_eq(g.isSettled(), false);

        let lo: u64 = 4294967295;
        let max_u64: u64 = (lo << 32) | lo;
        let g_ovf: TimeoutGuard = TimeoutGuard.new(max_u64 - 10, TimeoutOptions.withMs(100));
        expect_eq(g_ovf.deadlineMs(), max_u64);
    });

    test("2. @cap/p-timeout-core — resolves before deadline and locks settled state", () => {
        let g: TimeoutGuard = TimeoutGuard.new(100, TimeoutOptions.withMs(200));
        expect_eq(g.poll(250), PT_PENDING);
        expect_eq(g.resolve(260, 42), true);
        expect_eq(g.state, PT_RESOLVED);
        expect_eq(g.result_code, 42);
        expect_eq(g.used_fallback, false);
        expect_eq(g.isSettled(), true);

        // Further transitions are rejected even after deadline
        expect_eq(g.poll(500), PT_RESOLVED);
        expect_eq(g.reject(270, -1), false);
        expect_eq(g.cancel(), false);
    });

    test("3. @cap/p-timeout-core — rejects before deadline and prevents later resolve", () => {
        let g: TimeoutGuard = TimeoutGuard.new(0, TimeoutOptions.withMs(1000));
        expect_eq(g.reject(400, -5), true);
        expect_eq(g.state, PT_REJECTED);
        expect_eq(g.result_code, -5);
        expect_eq(g.isSettled(), true);

        expect_eq(g.resolve(450, 99), false);
        expect_eq(g.poll(2000), PT_REJECTED);
    });

    test("4. @cap/p-timeout-core — times out on deadline without fallback and blocks late resolve/reject", () => {
        let g1: TimeoutGuard = TimeoutGuard.new(1000, TimeoutOptions.withMs(250));
        expect_eq(g1.poll(1250), PT_TIMED_OUT);
        expect_eq(g1.isSettled(), true);
        expect_eq(g1.resolve(1250, 1), false);

        let g2: TimeoutGuard = TimeoutGuard.new(1000, TimeoutOptions.withMs(250));
        expect_eq(g2.resolve(1300, 7), false);
        expect_eq(g2.state, PT_TIMED_OUT);
        expect_eq(g2.reject(1300, -2), false);
    });

    test("5. @cap/p-timeout-core — resolves with fallback_code on timeout when has_fallback is true", () => {
        let opts: TimeoutOptions = TimeoutOptions {
            milliseconds: 300,
            has_fallback: true,
            fallback_code: 777,
            error_message: "fallback used",
        };
        let g: TimeoutGuard = TimeoutGuard.new(500, opts);
        expect_eq(g.poll(799), PT_PENDING);
        expect_eq(g.poll(800), PT_RESOLVED);
        expect_eq(g.result_code, 777);
        expect_eq(g.used_fallback, true);
        expect_eq(g.isSettled(), true);
    });

    test("6. @cap/p-timeout-core — cancel transitions to PT_CANCELED and prevents timeout override", () => {
        let g: TimeoutGuard = TimeoutGuard.new(10, TimeoutOptions.withMs(50));
        expect_eq(g.cancel(), true);
        expect_eq(g.state, PT_CANCELED);
        expect_eq(g.isSettled(), true);
        expect_eq(g.cancel(), false);
        expect_eq(g.poll(1000), PT_CANCELED);
        expect_eq(g.resolve(20, 1), false);
    });
}
