// cap/test/testTimerInternals.cp
// Production-grade deep tests for cap:event timerInternals.cp
// (Shared TimerObjectInternals state machine & 32-bit bitfield TimerObjectFlags absorbed from Bun)

import { test, assert, assertEq } from "cap:test";
import {
    TIMER_INTERNALS_MAX_EPOCH,
    TIMER_INTERNALS_MAX_INTERVAL_MS,
    TimerInternalsKind,
    TimerInternalsState,
    TimerObjectFlags,
    TimerObjectInternals,
} from "cap:event";

test("TimerObjectFlags: 32-bit bitfield pack/unpack round-trip", () => {
    const f = TimerObjectFlags{
        epoch: 123456,
        kind: TimerInternalsKind.SetInterval,
        has_cleared_timer: true,
        is_keeping_event_loop_alive: true,
        has_accessed_primitive: true,
        has_js_ref: false,
        in_callback: true,
    };
    const raw = f.toU32();
    const decoded = TimerObjectFlags.fromU32(raw);

    assertEq(decoded.epoch, 123456);
    assertEq(decoded.kind, TimerInternalsKind.SetInterval);
    assert(decoded.has_cleared_timer);
    assert(decoded.is_keeping_event_loop_alive);
    assert(decoded.has_accessed_primitive);
    assert(!decoded.has_js_ref);
    assert(decoded.in_callback);

    // Epoch overflow masks cleanly to 25 bits
    const f2 = TimerObjectFlags{
        epoch: TIMER_INTERNALS_MAX_EPOCH + 10,
        kind: TimerInternalsKind.SetImmediate,
    };
    const dec2 = TimerObjectFlags.fromU32(f2.toU32());
    assertEq(dec2.epoch, 9);
    assertEq(dec2.kind, TimerInternalsKind.SetImmediate);
});

test("TimerObjectInternals: init, asyncID, toPrimitive & ref/unref", () => {
    let t = TimerObjectInternals.init(42, TimerInternalsKind.SetTimeout, TIMER_INTERNALS_MAX_INTERVAL_MS + 99, 5, 1);
    assertEq(t.interval_ms, TIMER_INTERNALS_MAX_INTERVAL_MS);
    assertEq(t.state, TimerInternalsState.Active);
    assert(t.hasRef());
    assert(!t.flags.has_accessed_primitive);
    assertEq(t.toPrimitive(), 42);
    assert(t.flags.has_accessed_primitive);

    // Distinct asyncIDs across kinds for the same numeric id
    const t_int = TimerObjectInternals.init(42, TimerInternalsKind.SetInterval, 10, 0, 1);
    const t_imm = TimerObjectInternals.init(42, TimerInternalsKind.SetImmediate, 0, 0, 1);
    assert(t.asyncID() != t_int.asyncID());
    assert(t_int.asyncID() != t_imm.asyncID());

    t.doUnref();
    assert(!t.hasRef());
    t.doRef();
    assert(t.hasRef());
});

test("TimerObjectInternals: SetInterval re-arms on endFire unless cancelled in callback", () => {
    let t = TimerObjectInternals.init(10, TimerInternalsKind.SetInterval, 50, 1, 3);
    assert(t.beginFire(3, true));
    assert(t.flags.in_callback);
    assertEq(t.state, TimerInternalsState.Fired);
    assertEq(t.fire_count, 1);

    t.endFire();
    assert(!t.flags.in_callback);
    assertEq(t.state, TimerInternalsState.Active);
    assert(t.hasRef());

    // Cancel inside callback prevents re-arming
    assert(t.beginFire(3, true));
    t.cancel();
    t.endFire();
    assertEq(t.state, TimerInternalsState.Cancelled);
    assert(t.getDestroyed());
    assert(!t.shouldFire(3, true));
});

test("TimerObjectInternals: generation isolation & unref'd immediate liveness guard", () => {
    // Mismatched test_isolation_generation blocks firing
    let stale = TimerObjectInternals.init(11, TimerInternalsKind.SetTimeout, 10, 0, 2);
    assert(!stale.shouldFire(3, true));
    assert(!stale.beginFire(3, true));
    assert(!stale.flags.is_keeping_event_loop_alive);
    assert(!stale.flags.has_js_ref);

    // Unref'd SetImmediate does not fire when event loop has no other live handles
    let imm = TimerObjectInternals.init(12, TimerInternalsKind.SetImmediate, 0, 0, 5);
    imm.doUnref();
    assert(!imm.shouldFire(5, false));
    assert(imm.shouldFire(5, true));
    assert(imm.beginFire(5, true));
    imm.endFire();
    assertEq(imm.state, TimerInternalsState.Fired);
    assert(imm.getDestroyed());
});

test("TimerObjectInternals: reschedule & finalize terminal protection", () => {
    let t = TimerObjectInternals.init(20, TimerInternalsKind.SetTimeout, 100, 1, 1);
    assert(t.beginFire(1, true));
    t.endFire();
    assertEq(t.state, TimerInternalsState.Fired);

    // Reschedule re-arms fired timer with updated epoch
    assert(t.reschedule(99));
    assertEq(t.state, TimerInternalsState.Active);
    assertEq(t.flags.epoch, 99);
    assert(!t.flags.has_cleared_timer);

    // Finalize is terminal: reschedule returns false
    t.finalize();
    assertEq(t.state, TimerInternalsState.Destroyed);
    assert(t.getDestroyed());
    assert(!t.reschedule(100));
    assert(!t.beginFire(1, true));
});
