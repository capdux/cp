// cap/test/testTestRunnerDoneCallback.cp
// Production-grade deep tests for cap:test_runner doneCallback.cp (Bun DoneCallback lifecycle)

import { test, assert, assertEq } from "cap:test";
import {
    DONE_CALLBACK_DEFAULT_TIMEOUT_MS,
    DONE_CALLBACK_MAX_ERR_MSG,
    DoneCallbackError,
    DoneCallbackState,
    DoneCallback,
} from "cap:test_runner";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("DoneCallback: createUnbound -> bind -> complete -> finalize happy path", () => {
    assertEq(DONE_CALLBACK_DEFAULT_TIMEOUT_MS, 5000);
    assertEq(DONE_CALLBACK_MAX_ERR_MSG, 128);

    let cb = DoneCallback.createUnbound();
    assert(cb.state == DoneCallbackState.Unbound);
    assert(!cb.isPending());
    assert(!cb.isSettled());

    // Passing timeout_ms == 0 uses default 5000ms
    * = cb.bind(42, 1000, 0) catch unreachable;
    assert(cb.isPending());
    assertEq(cb.test_id, 42);
    assertEq(cb.deadlineMs(), 6000);

    const ok = cb.complete(1350) catch unreachable;
    assert(ok);
    assert(cb.didPass());
    assert(cb.isSettled());
    assert(cb.called);
    assertEq(cb.call_count, 1);
    assertEq(cb.elapsedMs(), 350);

    // Double complete while attached returns AlreadySettled
    let dup_err = false;
    * = cb.complete(1400) catch |e| {
        dup_err = (e == DoneCallbackError.AlreadySettled);
    };
    assert(dup_err);
    assertEq(cb.call_count, 2);

    // Finalizing an already-settled callback returns true and detaches
    assert(cb.finalize());
    assert(cb.state == DoneCallbackState.Detached);

    // Late callback after finalize is a safe no-op returning false
    const late = cb.complete(2000) catch unreachable;
    assert(!late);
});

test("DoneCallback: fail() records error code and truncates long message safely", () => {
    let cb = DoneCallback.createUnbound();
    * = cb.bind(7, 500, 2000) catch unreachable;
    assertEq(cb.deadlineMs(), 2500);

    const ok = cb.fail(500, "connection refused", 900) catch unreachable;
    assert(ok);
    assert(cb.didFail());
    assert(!cb.didPass());
    assertEq(cb.error_code, 500);
    assert(eqStr(cb.errorMessage(), "connection refused"));
    assertEq(cb.elapsedMs(), 400);

    // Subsequent complete() after fail() triggers AlreadySettled
    let settled_err = false;
    * = cb.complete(950) catch |e| {
        settled_err = (e == DoneCallbackError.AlreadySettled);
    };
    assert(settled_err);
});

test("DoneCallback: watchdog onTimeout and late callback no-op", () => {
    let cb = DoneCallback.createUnbound();
    * = cb.bind(99, 10000, 1000) catch unreachable;

    assert(cb.onTimeout(11000));
    assert(cb.didTimeout());
    assert(cb.isSettled());
    assertEq(cb.elapsedMs(), 1000);

    // Repeated onTimeout is a no-op
    assert(!cb.onTimeout(12000));

    // Late complete/fail after timeout returns false (safe no-op, not error)
    const late_ok = cb.complete(11050) catch unreachable;
    assert(!late_ok);
    const late_fail = cb.fail(1, "too late", 11060) catch unreachable;
    assert(!late_fail);
});

test("DoneCallback: implicit expiration when complete() arrives at or after deadline", () => {
    let cb = DoneCallback.createUnbound();
    * = cb.bind(11, 2000, 500) catch unreachable;

    // Exactly at deadline (2500 - 2000 == 500 >= 500) -> auto TimedOut
    const ok = cb.complete(2500) catch unreachable;
    assert(!ok);
    assert(cb.didTimeout());
    assert(!cb.didPass());
});

test("DoneCallback: finalize detects dangling uncalled callback", () => {
    let cb = DoneCallback.createUnbound();
    * = cb.bind(55, 0, 1000) catch unreachable;
    assert(cb.isPending());

    // Finalizing while still Pending returns false (dangling callback)
    assert(!cb.finalize());
    assert(cb.state == DoneCallbackState.Detached);
});

test("DoneCallback: unbound and double-bind error guards", () => {
    let cb = DoneCallback.createUnbound();

    let not_bound_complete = false;
    * = cb.complete(100) catch |e| {
        not_bound_complete = (e == DoneCallbackError.NotBound);
    };
    assert(not_bound_complete);

    let not_bound_fail = false;
    * = cb.fail(1, "err", 100) catch |e| {
        not_bound_fail = (e == DoneCallbackError.NotBound);
    };
    assert(not_bound_fail);

    * = cb.bind(1, 0, 1000) catch unreachable;
    let already_bound = false;
    * = cb.bind(2, 0, 1000) catch |e| {
        already_bound = (e == DoneCallbackError.AlreadyBound);
    };
    assert(already_bound);
});
