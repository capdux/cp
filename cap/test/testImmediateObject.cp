// cap/test/testImmediateObject.cp
// Production-grade deep tests for cap:event immediateObject.cp
// (Node/Bun setImmediate/clearImmediate ImmediateObject lifecycle & turn-bounded FIFO ImmediateQueue)

import { test, assert, assertEq } from "cap:test";
import {
    IMMEDIATE_QUEUE_MAX,
    ImmediateState,
    ImmediateObjectError,
    ImmediateObject,
    ImmediateQueue,
} from "cap:event";

test("ImmediateObject: init, toPrimitive, doRef/doUnref/hasRef & ref/deref", () => {
    let imm = ImmediateObject.init(7, 0);
    assertEq(imm.toPrimitive(), 7);
    assertEq(imm.state, ImmediateState.Active);
    assert(imm.isActive());
    assert(!imm.getDestroyed());
    assert(imm.hasRef());
    assertEq(imm.ref_count, 1);

    imm.doUnref();
    assert(!imm.hasRef());
    imm.doRef();
    assert(imm.hasRef());

    imm.ref();
    assertEq(imm.ref_count, 2);
    assert(!imm.deref());
    assertEq(imm.ref_count, 1);
    assert(imm.deref());
    assertEq(imm.ref_count, 0);
    assert(imm.deref()); // saturates at 0
});

test("ImmediateObject: runImmediateTask one-shot, dispose & finalize", () => {
    let i1 = ImmediateObject.init(1, 0);
    assert(i1.runImmediateTask());
    assertEq(i1.state, ImmediateState.Fired);
    assert(!i1.hasRef());
    assert(i1.getDestroyed());
    // Second run is a no-op
    assert(!i1.runImmediateTask());
    // doRef on fired immediate does not re-arm hasRef
    i1.doRef();
    assert(!i1.hasRef());

    // dispose cancels active immediate
    let i2 = ImmediateObject.init(2, 0);
    i2.dispose();
    assertEq(i2.state, ImmediateState.Cancelled);
    assert(!i2.hasRef());
    assert(i2.getDestroyed());
    assert(!i2.runImmediateTask());

    // finalize destroys immediate
    let i3 = ImmediateObject.init(3, 0);
    i3.finalize();
    assertEq(i3.state, ImmediateState.Destroyed);
    assert(!i3.hasRef());
    assert(i3.getDestroyed());
    assert(!i3.runImmediateTask());
});

test("ImmediateQueue: schedule, clear, setRef & activeRefCount tracking", () => {
    let q = ImmediateQueue.init();
    const id1 = q.schedule() catch {
        assert(false);
        return;
    };
    const id2 = q.schedule() catch {
        assert(false);
        return;
    };
    const id3 = q.schedule() catch {
        assert(false);
        return;
    };
    assertEq(id1, 1);
    assertEq(id2, 2);
    assertEq(id3, 3);
    assertEq(q.activeCount(), 3);
    assertEq(q.activeRefCount(), 3);

    const has_ref2 = q.setRef(id2, false) catch {
        assert(false);
        return;
    };
    assert(!has_ref2);
    assertEq(q.activeCount(), 3);
    assertEq(q.activeRefCount(), 2);

    assert(q.clear(id1));
    assert(!q.clear(id1)); // idempotent clear
    assertEq(q.activeCount(), 2);
    assertEq(q.activeRefCount(), 1);

    const fetched = q.get(id3) catch {
        assert(false);
        return;
    };
    assertEq(fetched.id, id3);
    assert(fetched.hasRef());
});

test("ImmediateQueue: drainTurn isolates current turn from newly scheduled turns", () => {
    let q = ImmediateQueue.init();
    const id1 = q.schedule() catch {
        assert(false);
        return;
    };
    const id2 = q.schedule() catch {
        assert(false);
        return;
    };

    let out: [8]i32 = [0; 8];
    const fired1 = q.drainTurn(out[0..]);
    assertEq(fired1, 2);
    assertEq(out[0], id1);
    assertEq(out[1], id2);
    assertEq(q.current_turn, 1);
    assertEq(q.total_fired, 2);
    assertEq(q.activeCount(), 0);

    // Schedule in next turn & verify slot reuse
    const id3 = q.schedule() catch {
        assert(false);
        return;
    };
    const obj3 = q.get(id3) catch {
        assert(false);
        return;
    };
    assertEq(obj3.turn, 1);

    const fired2 = q.drainTurn(out[0..]);
    assertEq(fired2, 1);
    assertEq(out[0], id3);
    assertEq(q.total_fired, 3);
});

test("ImmediateQueue: boundary guards (QueueFull, NotFound, small out_ids buffer)", () => {
    let q = ImmediateQueue.init();
    let i: usize = 0;
    while (i < IMMEDIATE_QUEUE_MAX) {
        * = q.schedule() catch {
            assert(false);
            return;
        };
        i += 1;
    }
    assertEq(q.activeCount(), IMMEDIATE_QUEUE_MAX);

    let got_full = false;
    * = q.schedule() catch |e| {
        assertEq(e, ImmediateObjectError.QueueFull);
        got_full = true;
        0
    };
    assert(got_full);

    let got_not_found = false;
    * = q.get(9999) catch |e| {
        assertEq(e, ImmediateObjectError.NotFound);
        got_not_found = true;
        ImmediateObject{}
    };
    assert(got_not_found);

    let got_ref_nf = false;
    * = q.setRef(9999, true) catch |e| {
        assertEq(e, ImmediateObjectError.NotFound);
        got_ref_nf = true;
        false
    };
    assert(got_ref_nf);

    // Drain with undersized out_ids still fires all 64 and frees all slots safely
    let tiny: [2]i32 = [0; 2];
    const fired = q.drainTurn(tiny[0..]);
    assertEq(fired, IMMEDIATE_QUEUE_MAX);
    assertEq(tiny[0], 1);
    assertEq(tiny[1], 2);
    assertEq(q.activeCount(), 0);
});
