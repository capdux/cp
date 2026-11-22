// Origin: Pure-CP Stdlib Test Suite (lib/test_runner/fakeTimers.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Bun-Compatible Fake Timers & Mock Clock:
//   1. Inactive guard (NotActive), useFakeTimers/useRealTimers lifecycle & setSystemTime decoupling
//   2. setTimeout chronological ordering, same-ms FIFO stability & HTML/Bun 0ms -> 1ms immediate clamp
//   3. setInterval repeating schedule, multi-tick advanceTimersByTime, clearTimer & clearAllTimers
//   4. advanceTimersToNextTimer single-stepping & runOnlyPendingTimers upper-bound cutoff
//   5. runAllTimers exhaustion & infinite-interval RunLimitExceeded protection
//   6. Boundary guards: AdvanceOutOfRange (<0 / >u32::MAX), CapacityExceeded (256 slots) & small event buffer truncation

import { test, expect_eq, expect_true } from "cap:test";
import {
    FAKE_TIMERS_MAX_SLOTS,
    FAKE_TIMERS_MAX_ADVANCE_MS,
    FakeTimerError,
    FakeTimerKind,
    FakeTimerFired,
    normalizeFakeDelayMs,
    FakeTimerClock,
} from "cap:test_runner";

test("FakeTimerClock: inactive guard, lifecycle & setSystemTime decoupling", fn() {
    let clock = FakeTimerClock.init();
    expect_true(!clock.isFakeTimers());

    // Calling timer APIs before useFakeTimers() must fail with NotActive
    const c_res = clock.getTimerCount();
    let got_not_active = false;
    c_res catch |e| {
        if (e == FakeTimerError.NotActive) got_not_active = true;
    };
    expect_true(got_not_active);

    // Anchor at 2020-01-01T00:00:00.000Z (1577836800000)
    const epoch_2020: i64 = 1577836800000;
    clock.useFakeTimers(epoch_2020);
    expect_true(clock.isFakeTimers());
    expect_eq(clock.now(), epoch_2020);
    expect_eq(clock.monotonicNow(), 0);

    const id = clock.setTimeout(500, 42) catch 0;
    expect_true(id > 0);
    expect_eq(clock.getTimerCount() catch 999, 1);

    // Override system time to 1999-01-01T00:00:00.000Z (915148800000) without shifting monotonic due_ms
    const epoch_1999: i64 = 915148800000;
    clock.setSystemTime(epoch_1999);
    expect_eq(clock.now(), epoch_1999);
    expect_eq(clock.monotonicNow(), 0);

    let events: [4]FakeTimerFired = [FakeTimerFired{}; 4];
    const adv = clock.advanceTimersByTime(500, events[0..]) catch unreachable;
    expect_eq(adv.fired_count, 1);
    expect_eq(adv.recorded_count, 1);
    expect_eq(events[0].tag, 42);
    expect_eq(events[0].fired_at_ms, 500);
    expect_eq(events[0].system_time_ms, epoch_1999 + 500);
    expect_eq(clock.now(), epoch_1999 + 500);

    // Reset to real timers clears active state & resets virtual counters
    clock.useRealTimers();
    expect_true(!clock.isFakeTimers());
    expect_eq(clock.monotonicNow(), 0);
});

test("FakeTimerClock: chronological ordering, same-ms FIFO & 0ms HTML/Bun 1ms clamp", fn() {
    expect_eq(normalizeFakeDelayMs(-50), 1);
    expect_eq(normalizeFakeDelayMs(0), 1);
    expect_eq(normalizeFakeDelayMs(15), 15);

    let clock = FakeTimerClock.init();
    clock.useFakeTimers(1000);

    // Schedule out of chronological order + two timers at same 10ms + one immediate 0ms timer
    const id_late = clock.setTimeout(50, 300) catch 0;
    const id_mid_a = clock.setTimeout(10, 201) catch 0;
    const id_mid_b = clock.setTimeout(10, 202) catch 0;
    const id_imm = clock.setTimeout(0, 100) catch 0;
    expect_true(id_late > 0 && id_mid_a > 0 && id_mid_b > 0 && id_imm > 0);
    expect_eq(clock.getTimerCount() catch 0, 4);

    // advanceTimersByTime(0) must advance by 1ms and fire setTimeout(0)
    let events: [8]FakeTimerFired = [FakeTimerFired{}; 8];
    const r0 = clock.advanceTimersByTime(0, events[0..]) catch unreachable;
    expect_eq(r0.fired_count, 1);
    expect_eq(events[0].tag, 100);
    expect_eq(events[0].fired_at_ms, 1);
    expect_eq(clock.monotonicNow(), 1);
    expect_eq(clock.getTimerCount() catch 0, 3);

    // Advance 9ms -> monotonicNow == 10, must fire id_mid_a then id_mid_b in strict FIFO order
    const r1 = clock.advanceTimersByTime(9, events[0..]) catch unreachable;
    expect_eq(r1.fired_count, 2);
    expect_eq(events[0].tag, 201);
    expect_eq(events[1].tag, 202);
    expect_eq(clock.getTimerCount() catch 0, 1);

    // Advance 40ms -> monotonicNow == 50, fires id_late
    const r2 = clock.advanceTimersByTime(40, events[0..]) catch unreachable;
    expect_eq(r2.fired_count, 1);
    expect_eq(events[0].tag, 300);
    expect_eq(clock.getTimerCount() catch 99, 0);
});

test("FakeTimerClock: setInterval multi-tick firing, clearTimer & clearAllTimers", fn() {
    let clock = FakeTimerClock.init();
    clock.useFakeTimers(10000);

    const iv_id = clock.setInterval(25, 7) catch 0;
    const to_id = clock.setTimeout(60, 9) catch 0;
    expect_true(iv_id > 0 && to_id > 0);

    let events: [8]FakeTimerFired = [FakeTimerFired{}; 8];
    // Advance 80ms: interval fires at 25, 50, 75; timeout fires at 60
    const r = clock.advanceTimersByTime(80, events[0..]) catch unreachable;
    expect_eq(r.fired_count, 4);
    expect_eq(events[0].tag, 7);
    expect_eq(events[0].fired_at_ms, 25);
    expect_eq(events[1].tag, 7);
    expect_eq(events[1].fired_at_ms, 50);
    expect_eq(events[2].tag, 9);
    expect_eq(events[2].fired_at_ms, 60);
    expect_eq(events[2].kind, FakeTimerKind.Timeout);
    expect_eq(events[3].tag, 7);
    expect_eq(events[3].fired_at_ms, 75);
    expect_eq(events[3].kind, FakeTimerKind.Interval);
    expect_eq(clock.monotonicNow(), 80);
    expect_eq(clock.getTimerCount() catch 0, 1);

    // Cancel interval by ID; cancelling again returns false
    expect_true(clock.clearTimer(iv_id) catch false);
    expect_true(!(clock.clearTimer(iv_id) catch true));
    expect_eq(clock.getTimerCount() catch 99, 0);

    // Schedule 3 timers and wipe with clearAllTimers
    * = clock.setTimeout(10, 1) catch 0;
    * = clock.setTimeout(20, 2) catch 0;
    * = clock.setInterval(30, 3) catch 0;
    const cleared = clock.clearAllTimers() catch 0;
    expect_eq(cleared, 3);
    expect_eq(clock.getTimerCount() catch 99, 0);
});

test("FakeTimerClock: advanceTimersToNextTimer & runOnlyPendingTimers cutoff", fn() {
    let clock = FakeTimerClock.init();
    clock.useFakeTimers(5000);

    * = clock.setInterval(30, 11) catch 0;
    * = clock.setTimeout(100, 22) catch 0;

    let events: [16]FakeTimerFired = [FakeTimerFired{}; 16];

    // Step 1: advanceTimersToNextTimer fires interval at t=30
    const step1 = clock.advanceTimersToNextTimer(events[0..]) catch unreachable;
    expect_eq(step1.fired_count, 1);
    expect_eq(events[0].tag, 11);
    expect_eq(events[0].fired_at_ms, 30);
    expect_eq(clock.monotonicNow(), 30);

    // peekNextDueMs == 60, peekLastDueMs == 100
    if (next_due := clock.peekNextDueMs()) {
        expect_eq(next_due, 60);
    } else {
        expect_true(false);
    }
    if (last_due := clock.peekLastDueMs()) {
        expect_eq(last_due, 100);
    } else {
        expect_true(false);
    }

    // runOnlyPendingTimers locks until=100: fires interval@60, interval@90, timeout@100 (then stops before interval@120)
    const pending = clock.runOnlyPendingTimers(events[0..]) catch unreachable;
    expect_eq(pending.fired_count, 3);
    expect_eq(events[0].fired_at_ms, 60);
    expect_eq(events[1].fired_at_ms, 90);
    expect_eq(events[2].fired_at_ms, 100);
    expect_eq(events[2].tag, 22);
    expect_eq(clock.monotonicNow(), 100);
    expect_eq(clock.getTimerCount() catch 0, 1);
});

test("FakeTimerClock: runAllTimers exhaustion & infinite-interval RunLimitExceeded guard", fn() {
    let clock = FakeTimerClock.init();
    clock.useFakeTimers(0);

    * = clock.setTimeout(100, 3) catch 0;
    * = clock.setTimeout(5, 1) catch 0;
    * = clock.setTimeout(40, 2) catch 0;

    let events: [8]FakeTimerFired = [FakeTimerFired{}; 8];
    const all_res = clock.runAllTimers(events[0..]) catch unreachable;
    expect_eq(all_res.fired_count, 3);
    expect_eq(events[0].tag, 1);
    expect_eq(events[1].tag, 2);
    expect_eq(events[2].tag, 3);
    expect_eq(clock.monotonicNow(), 100);
    expect_eq(clock.getTimerCount() catch 99, 0);

    // Infinite repeating interval must trigger RunLimitExceeded instead of hanging
    clock.setRunLimit(25);
    * = clock.setInterval(10, 99) catch 0;
    let hit_limit = false;
    const inf_res = clock.runAllTimers(events[0..]);
    inf_res catch |e| {
        if (e == FakeTimerError.RunLimitExceeded) hit_limit = true;
    };
    expect_true(hit_limit);
});

test("FakeTimerClock: boundary guards (AdvanceOutOfRange, CapacityExceeded, small event buffer)", fn() {
    let clock = FakeTimerClock.init();
    clock.useFakeTimers(0);

    let small_buf: [2]FakeTimerFired = [FakeTimerFired{}; 2];

    // Negative delta_ms -> AdvanceOutOfRange
    let neg_err = false;
    clock.advanceTimersByTime(-1, small_buf[0..]) catch |e| {
        if (e == FakeTimerError.AdvanceOutOfRange) neg_err = true;
    };
    expect_true(neg_err);

    // > u32::MAX delta_ms -> AdvanceOutOfRange
    let big_err = false;
    clock.advanceTimersByTime(FAKE_TIMERS_MAX_ADVANCE_MS + 1, small_buf[0..]) catch |e| {
        if (e == FakeTimerError.AdvanceOutOfRange) big_err = true;
    };
    expect_true(big_err);

    // Fill all 256 slots; 257th must return CapacityExceeded
    let i: usize = 0;
    while (i < FAKE_TIMERS_MAX_SLOTS) {
        const id = clock.setTimeout(10, @as(u64, i)) catch 0;
        expect_true(id > 0);
        i += 1;
    }
    expect_eq(clock.getTimerCount() catch 0, FAKE_TIMERS_MAX_SLOTS);

    let cap_err = false;
    clock.setTimeout(10, 9999) catch |e| {
        if (e == FakeTimerError.CapacityExceeded) cap_err = true;
    };
    expect_true(cap_err);

    // Fire all 256 timers with a 2-element event buffer: fired_count == 256, recorded_count == 2
    const res = clock.advanceTimersByTime(10, small_buf[0..]) catch unreachable;
    expect_eq(res.fired_count, FAKE_TIMERS_MAX_SLOTS);
    expect_eq(res.recorded_count, 2);
    expect_eq(small_buf[0].tag, 0);
    expect_eq(small_buf[1].tag, 1);
    expect_eq(clock.getTimerCount() catch 99, 0);

    // Slots are immediately reusable after firing
    const reused_id = clock.setTimeout(5, 777) catch 0;
    expect_true(reused_id > 0);
});
