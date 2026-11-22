// cap/test/testRunLoopTimer.cp
// Production-grade deep tests for cap:event runLoopTimer.cp
// (High-precision f64-second RunLoopTimer absorbed from Bun WTFTimer.cp)

import { test, assert, assertEq } from "cap:test";
import {
    RUN_LOOP_TIMER_MAX_SECONDS,
    RUN_LOOP_TIMER_NS_PER_SEC,
    RunLoopTimerState,
    RunLoopTimeSpec,
    RunLoopTimer,
} from "cap:event";

test("RunLoopTimeSpec: carry normalization, addSeconds & compare", () => {
    assertEq(RUN_LOOP_TIMER_NS_PER_SEC, 1_000_000_000);

    const ts = RunLoopTimeSpec.init(10, 1_500_000_000);
    assertEq(ts.sec, 11);
    assertEq(ts.nsec, 500_000_000);

    const neg = RunLoopTimeSpec.init(10, -250_000_000);
    assertEq(neg.sec, 9);
    assertEq(neg.nsec, 750_000_000);

    // Fractional carry across 1s boundary
    const base = RunLoopTimeSpec.init(100, 750_000_000);
    const next = base.addSeconds(0.5);
    assertEq(next.sec, 101);
    assertEq(next.nsec, 250_000_000);

    assertEq(base.compare(next), -1);
    assertEq(next.compare(base), 1);
    assertEq(next.compare(RunLoopTimeSpec.init(101, 250_000_000)), 0);
    assert(next.isReached(RunLoopTimeSpec.init(101, 250_000_000)));
    assert(!next.isReached(RunLoopTimeSpec.init(101, 249_999_999)));
});

test("RunLoopTimer: imminent zero/negative delay fast path & runIfImminent", () => {
    let t = RunLoopTimer.init(42);
    assertEq(t.state, RunLoopTimerState.Cancelled);
    assert(!t.isImminent());
    assert(!t.runIfImminent());

    const now = RunLoopTimeSpec.init(50, 0);
    t.update(now, 0.0, false);
    assertEq(t.state, RunLoopTimerState.Imminent);
    assert(t.isImminent());
    assert(t.isDue(now));

    assert(t.runIfImminent());
    assertEq(t.state, RunLoopTimerState.Fired);
    assert(!t.isImminent());
    assertEq(t.fire_count, 1);

    // Second runIfImminent is a no-op
    assert(!t.runIfImminent());

    // Negative seconds also enters imminent path
    t.update(now, -3.5, false);
    assert(t.isImminent());
    assert(t.fire(now));
    assertEq(t.fire_count, 2);
});

test("RunLoopTimer: one-shot & repeating active scheduling", () => {
    let one = RunLoopTimer.init(1);
    const t0 = RunLoopTimeSpec.init(10, 0);
    one.update(t0, 1.25, false);
    assertEq(one.state, RunLoopTimerState.Active);
    assert(!one.isDue(RunLoopTimeSpec.init(11, 249_999_999)));
    assert(one.isDue(RunLoopTimeSpec.init(11, 250_000_000)));
    assert(one.fire(RunLoopTimeSpec.init(11, 250_000_000)));
    assertEq(one.state, RunLoopTimerState.Fired);
    assertEq(one.fire_count, 1);
    assert(!one.fire(RunLoopTimeSpec.init(20, 0)));

    // Repeating timer reschedules automatically
    let rep = RunLoopTimer.init(2);
    rep.update(t0, 0.5, true);
    assert(rep.fire(RunLoopTimeSpec.init(10, 500_000_000)));
    assertEq(rep.state, RunLoopTimerState.Active);
    assertEq(rep.fire_count, 1);
    assertEq(rep.next.sec, 11);
    assertEq(rep.next.nsec, 0);
    assert(!rep.fire(RunLoopTimeSpec.init(10, 999_999_999)));
    assert(rep.fire(RunLoopTimeSpec.init(11, 0)));
    assertEq(rep.fire_count, 2);
});

test("RunLoopTimer: i32-second clamping & rescheduling out of imminent", () => {
    let t = RunLoopTimer.init(7);
    const t0 = RunLoopTimeSpec.init(100, 0);

    // Enter imminent first, then reschedule with positive duration
    t.update(t0, 0.0, false);
    assert(t.isImminent());

    t.update(t0, 9_999_999_999.0, false);
    assert(!t.isImminent());
    assertEq(t.state, RunLoopTimerState.Active);
    assertEq(t.interval_sec, RUN_LOOP_TIMER_MAX_SECONDS);
    assertEq(t.next.sec, 100 + 2_147_483_647);
    assertEq(t.next.nsec, 0);
});

test("RunLoopTimer: cancel respects script execution context validity", () => {
    // Valid context cancels both Active and Imminent
    let valid = RunLoopTimer.init(99);
    const now = RunLoopTimeSpec.init(1, 0);
    valid.update(now, 0.0, false);
    assert(valid.isImminent());
    valid.cancel();
    assertEq(valid.state, RunLoopTimerState.Cancelled);
    assert(!valid.isImminent());
    assert(!valid.fire(now));

    // Invalid context (0) makes cancel a safe no-op (aligned with WTFTimer.cancel)
    let invalid = RunLoopTimer.init(0);
    assert(!invalid.hasValidContext());
    invalid.update(now, 1.0, false);
    invalid.cancel();
    assertEq(invalid.state, RunLoopTimerState.Active);
});
