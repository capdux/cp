// cap/test/testSpawnSyncLoop.cp
// Production-grade deep tests for cap:event spawnSyncLoop.cp
// (Synchronous sub-event-loop state machine, deadline timeout & parent handle isolation)

import { test, assert, assertEq } from "cap:test";
import {
    SpawnSyncTickState,
    SpawnSyncLoopError,
    SpawnSyncDeadline,
    SpawnSyncLoop,
} from "cap:event";

test("SpawnSyncDeadline: finite, infinite, clock-skew clamp & saturating overflow", () => {
    const inf = SpawnSyncDeadline.infinite(500);
    assert(inf.isInfinite());
    assert(!inf.isExpired(500));
    assert(!inf.isExpired(999_999_999));
    assertEq(inf.remainingMs(1000), 0);
    assertEq(inf.elapsedMs(1200), 700);
    assertEq(inf.elapsedMs(300), 0); // Clock skew clamped to 0

    const d = SpawnSyncDeadline.fromTimeout(1000, 250);
    assert(!d.isInfinite());
    assertEq(d.deadlineMs(), 1250);
    assert(!d.isExpired(1000));
    assert(!d.isExpired(1249));
    assertEq(d.remainingMs(1100), 150);
    assert(d.isExpired(1250));
    assert(d.isExpired(2000));
    assertEq(d.remainingMs(1250), 0);
    assertEq(d.remainingMs(1500), 0);

    // Saturating addition near u64 max must not wrap around
    const max_u64: u64 = 18446744073709551615;
    const sat = SpawnSyncDeadline.fromTimeout(max_u64 - 5, 100);
    assertEq(sat.deadlineMs(), max_u64);
    assert(!sat.isExpired(max_u64 - 1));
    assert(sat.isExpired(max_u64));
});

test("SpawnSyncLoop: normal completion lifecycle & parent handle swap/restore", () => {
    let active_handle: u64 = 100;
    let sub = SpawnSyncLoop.init(200);

    const saved = sub.prepare(&active_handle, 1000, 500) catch {
        assert(false);
        return;
    };
    assertEq(saved, 100);
    assertEq(active_handle, 200);
    assert(sub.prepared);
    assertEq(sub.state(), SpawnSyncTickState.Running);

    assertEq(sub.tick(1100), SpawnSyncTickState.Running);
    assertEq(sub.tick(1250), SpawnSyncTickState.Running);
    assertEq(sub.ticks, 2);
    assertEq(sub.remainingTimeoutMs(1250), 250);
    assertEq(sub.elapsedMs(1250), 250);

    sub.markCompleted();
    assertEq(sub.tick(1300), SpawnSyncTickState.Completed);
    assertEq(sub.state(), SpawnSyncTickState.Completed);
    assert(!sub.did_timeout);

    const restored = sub.cleanup(&active_handle) catch {
        assert(false);
        return;
    };
    assertEq(restored, 100);
    assertEq(active_handle, 100);
    assert(!sub.prepared);
});

test("SpawnSyncLoop: timeout expiration sets did_timeout and halts running", () => {
    let active_handle: u64 = 1;
    let sub = SpawnSyncLoop.init(77);

    * = sub.prepare(&active_handle, 2000, 300) catch {
        assert(false);
        return;
    };

    assertEq(sub.tick(2150), SpawnSyncTickState.Running);
    assertEq(sub.tick(2299), SpawnSyncTickState.Running);
    assertEq(sub.ticks, 2);

    assertEq(sub.tick(2300), SpawnSyncTickState.Timeout);
    assert(sub.did_timeout);
    assertEq(sub.state(), SpawnSyncTickState.Timeout);
    assertEq(sub.remainingTimeoutMs(2300), 0);

    * = sub.cleanup(&active_handle) catch {
        assert(false);
        return;
    };
    assertEq(active_handle, 1);
});

test("SpawnSyncLoop: explicit cancel takes precedence over running and timeout", () => {
    let active_handle: u64 = 5;
    let sub = SpawnSyncLoop.init(55);

    * = sub.prepare(&active_handle, 100, 50) catch {
        assert(false);
        return;
    };

    assertEq(sub.tick(120), SpawnSyncTickState.Running);
    sub.cancel();
    assertEq(sub.tick(200), SpawnSyncTickState.Cancelled);
    assertEq(sub.state(), SpawnSyncTickState.Cancelled);

    * = sub.cleanup(&active_handle) catch {
        assert(false);
        return;
    };
    assertEq(active_handle, 5);
});

test("SpawnSyncLoop: lifecycle error guards (InvalidSubHandle, AlreadyPrepared, NotPrepared)", () => {
    let active_handle: u64 = 9;
    let invalid = SpawnSyncLoop.init(0);
    let saw_invalid = false;
    * = invalid.prepare(&active_handle, 0, 100) catch e => {
        if (e == SpawnSyncLoopError.InvalidSubHandle) saw_invalid = true;
    };
    assert(saw_invalid);
    assertEq(active_handle, 9);

    let sub = SpawnSyncLoop.init(99);
    let saw_not_prepared = false;
    * = sub.cleanup(&active_handle) catch e => {
        if (e == SpawnSyncLoopError.NotPrepared) saw_not_prepared = true;
    };
    assert(saw_not_prepared);

    * = sub.prepare(&active_handle, 0, 100) catch {
        assert(false);
        return;
    };

    let saw_already = false;
    * = sub.prepare(&active_handle, 10, 100) catch e => {
        if (e == SpawnSyncLoopError.AlreadyPrepared) saw_already = true;
    };
    assert(saw_already);

    * = sub.cleanup(&active_handle) catch {
        assert(false);
        return;
    };
    assertEq(active_handle, 9);
});

test("SpawnSyncLoop: nested sub-loop isolation and instance reuse across cycles", () => {
    let active_handle: u64 = 1;
    let outer = SpawnSyncLoop.init(10);
    let inner = SpawnSyncLoop.init(20);

    * = outer.prepare(&active_handle, 1000, 1000) catch {
        assert(false);
        return;
    };
    assertEq(active_handle, 10);

    * = inner.prepare(&active_handle, 1050, 100) catch {
        assert(false);
        return;
    };
    assertEq(active_handle, 20);
    inner.markCompleted();
    assertEq(inner.tick(1060), SpawnSyncTickState.Completed);

    * = inner.cleanup(&active_handle) catch {
        assert(false);
        return;
    };
    assertEq(active_handle, 10);

    outer.markCompleted();
    assertEq(outer.tick(1080), SpawnSyncTickState.Completed);
    * = outer.cleanup(&active_handle) catch {
        assert(false);
        return;
    };
    assertEq(active_handle, 1);

    // Reuse `outer` for a second cycle and confirm stale flags are cleared
    * = outer.prepare(&active_handle, 5000, 0) catch {
        assert(false);
        return;
    };
    assertEq(outer.ticks, 0);
    assert(!outer.completed);
    assert(!outer.did_timeout);
    assertEq(outer.tick(999_999), SpawnSyncTickState.Running); // infinite timeout
    * = outer.cleanup(&active_handle) catch {
        assert(false);
        return;
    };
    assertEq(active_handle, 1);
});
