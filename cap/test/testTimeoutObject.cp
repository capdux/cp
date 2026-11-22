// cap/test/testTimeoutObject.cp
// Production-grade deep tests for cap:event timeoutObject.cp
// (Node/Bun setTimeout & setInterval TimeoutObject lifecycle, ref/unref/refresh & TimeoutRegistry)

import { test, assert, assertEq } from "cap:test";
import {
    TIMEOUT_MAX_INTERVAL_MS,
    TIMEOUT_REGISTRY_MAX,
    TimeoutKind,
    TimeoutState,
    TimeoutObjectError,
    clampTimeoutIntervalMs,
    TimeoutObject,
    TimeoutRegistry,
} from "cap:event";

test("clampTimeoutIntervalMs: Node/Bun u31 overflow and non-positive clamping", () => {
    assertEq(clampTimeoutIntervalMs(0), 1);
    assertEq(clampTimeoutIntervalMs(-500), 1);
    assertEq(clampTimeoutIntervalMs(1), 1);
    assertEq(clampTimeoutIntervalMs(250), 250);
    assertEq(clampTimeoutIntervalMs(@as(i64, TIMEOUT_MAX_INTERVAL_MS)), TIMEOUT_MAX_INTERVAL_MS);
    assertEq(clampTimeoutIntervalMs(@as(i64, TIMEOUT_MAX_INTERVAL_MS) + 1), 1);
    assertEq(clampTimeoutIntervalMs(9_000_000_000), 1);
});

test("TimeoutObject: one-shot SetTimeout firing, toPrimitive & doRefresh re-arming", () => {
    let t = TimeoutObject.init(42, TimeoutKind.SetTimeout, 1000, 100);
    assertEq(t.toPrimitive(), 42);
    assertEq(t.next_fire_ms, 1100);
    assert(t.hasRef());
    assert(!t.isDue(1099));
    assert(!t.fire(1099));

    assert(t.isDue(1100));
    assert(t.fire(1100));
    assertEq(t.fire_count, 1);
    assertEq(t.state, TimeoutState.Fired);
    assert(!t.hasRef());
    assert(!t.fire(2000)); // Already fired one-shot

    // doRefresh re-arms the fired one-shot timeout
    const next_ms = t.doRefresh(2000) catch {
        assert(false);
        return;
    };
    assertEq(next_ms, 2100);
    assertEq(t.state, TimeoutState.Active);
    assert(t.fire(2100));
    assertEq(t.fire_count, 2);
});

test("TimeoutObject: repeating SetInterval rescheduling & cancel/finalize lifecycle", () => {
    let t = TimeoutObject.init(7, TimeoutKind.SetInterval, 500, 50);
    assert(t.fire(550));
    assertEq(t.fire_count, 1);
    assertEq(t.state, TimeoutState.Active);
    assertEq(t.next_fire_ms, 600);
    assert(t.hasRef());

    assert(!t.fire(599));
    assert(t.fire(610));
    assertEq(t.fire_count, 2);
    assertEq(t.next_fire_ms, 660);

    t.cancel();
    assertEq(t.state, TimeoutState.Cancelled);
    assert(!t.hasRef());
    assert(!t.fire(9999));

    t.finalize();
    assertEq(t.state, TimeoutState.Destroyed);
    let saw_destroyed = false;
    * = t.doRefresh(1000) catch e => {
        if (e == TimeoutObjectError.AlreadyDestroyed) saw_destroyed = true;
    };
    assert(saw_destroyed);
});

test("TimeoutObject: doRef/doUnref and internal ref/deref counters", () => {
    let t = TimeoutObject.init(9, TimeoutKind.SetTimeout, 0, 10);
    assert(t.hasRef());
    t.doUnref();
    assert(!t.hasRef());
    t.doRef();
    assert(t.hasRef());

    assertEq(t.ref_count, 1);
    t.ref();
    assertEq(t.ref_count, 2);
    assert(!t.deref());
    assertEq(t.ref_count, 1);
    assert(t.deref());
    assertEq(t.ref_count, 0);
});

test("TimeoutRegistry: schedule, tickDue, refresh, setRef & clear", () => {
    let reg = TimeoutRegistry.init();
    const id1 = reg.scheduleTimeout(1000, 100) catch {
        assert(false);
        return;
    };
    const id2 = reg.scheduleInterval(1000, 40) catch {
        assert(false);
        return;
    };
    assertEq(reg.activeRefCount(), 2);

    assert(reg.setRef(id1, false));
    assertEq(reg.activeRefCount(), 1);

    // At 1050ms: only interval id2 (due at 1040) fires
    assertEq(reg.tickDue(1050), 1);
    if (t2 := reg.get(id2)) {
        assertEq(t2.fire_count, 1);
        assertEq(t2.next_fire_ms, 1090);
    } else {
        assert(false);
    }

    // Refresh id1 at 1080 -> due at 1180
    const refreshed = reg.refresh(id1, 1080) catch {
        assert(false);
        return;
    };
    assertEq(refreshed, 1180);

    // At 1100: only id2 fires again (due at 1090), id1 not due yet
    assertEq(reg.tickDue(1100), 1);

    // At 1180: both id1 and id2 fire
    assertEq(reg.tickDue(1180), 2);
    assert(reg.clear(id2));
    assert(!reg.clear(id2));
    assertEq(reg.activeRefCount(), 0);
});

test("TimeoutRegistry: capacity guard (64 slots) & NotFound on unknown refresh", () => {
    let reg = TimeoutRegistry.init();
    let saw_not_found = false;
    * = reg.refresh(999, 100) catch e => {
        if (e == TimeoutObjectError.NotFound) saw_not_found = true;
    };
    assert(saw_not_found);

    let i: usize = 0;
    while (i < TIMEOUT_REGISTRY_MAX) {
        * = reg.scheduleTimeout(0, 10) catch {
            assert(false);
            return;
        };
        i += 1;
    }

    let saw_full = false;
    * = reg.scheduleTimeout(0, 10) catch e => {
        if (e == TimeoutObjectError.RegistryFull) saw_full = true;
    };
    assert(saw_full);
});
