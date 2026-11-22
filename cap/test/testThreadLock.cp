// cap/test/testThreadLock.cp
// Production-grade deep tests for cap:sync threadLock.cp
// (Single-thread ownership assertion lock ThreadLock absorbed from Bun safety/ThreadLock.cp)

import { test, assert, assertEq } from "cap:test";
import {
    THREAD_LOCK_INVALID_ID,
    ThreadLockError,
    ThreadLock,
} from "../../lib/sync/threadLock.cp";

test("ThreadLock: initUnlocked, initLocked, lock, assertLocked & unlock", () => {
    assertEq(THREAD_LOCK_INVALID_ID, 0);

    let lk = ThreadLock.initUnlocked();
    assert(!lk.isLocked());
    assertEq(lk.owningThread(), THREAD_LOCK_INVALID_ID);
    assert(!lk.isOwnedBy(101));

    assert(lk.lock(101) catch false);
    assert(lk.isLocked());
    assertEq(lk.owningThread(), 101);
    assert(lk.isOwnedBy(101));
    assert(!lk.isOwnedBy(202));
    assertEq(lk.acquire_count, 1);

    assert(lk.assertLocked(101) catch false);
    assert(lk.unlock(101) catch false);
    assert(!lk.isLocked());
    assertEq(lk.owningThread(), THREAD_LOCK_INVALID_ID);

    // initLocked acquires immediately
    let lk2 = ThreadLock.initLocked(555) catch {
        assert(false);
        return;
    };
    assert(lk2.isOwnedBy(555));
    assertEq(lk2.acquire_count, 1);
});

test("ThreadLock: lockOrAssert lazy acquire & idempotent same-thread check", () => {
    let lk = ThreadLock.initUnlocked();
    assert(lk.lockOrAssert(42) catch false);
    assertEq(lk.owningThread(), 42);
    assertEq(lk.acquire_count, 1);

    // Calling lockOrAssert again on same thread succeeds without re-acquiring
    assert(lk.lockOrAssert(42) catch false);
    assertEq(lk.acquire_count, 1);

    // Calling lockOrAssert from another thread returns WrongThread
    let got_wrong = false;
    * = lk.lockOrAssert(99) catch |e| {
        assertEq(e, ThreadLockError.WrongThread);
        got_wrong = true;
        false
    };
    assert(got_wrong);
    assertEq(lk.violation_count, 1);
});

test("ThreadLock: ownership transfer between threads", () => {
    let lk = ThreadLock.initLocked(10) catch {
        assert(false);
        return;
    };
    assert(lk.transfer(10, 20) catch false);
    assertEq(lk.owningThread(), 20);
    assert(!lk.isOwnedBy(10));
    assert(lk.isOwnedBy(20));
    assertEq(lk.acquire_count, 2);

    // Wrong source thread rejected
    let got_wrong = false;
    * = lk.transfer(10, 30) catch |e| {
        assertEq(e, ThreadLockError.WrongThread);
        got_wrong = true;
        false
    };
    assert(got_wrong);
    assertEq(lk.owningThread(), 20);
});

test("ThreadLock: disabled fast path is a zero-violation no-op", () => {
    let lk = ThreadLock.initDisabled();
    assert(lk.isOwnedBy(999));
    assert(lk.lock(0) catch false);
    assert(lk.assertLocked(0) catch false);
    assert(lk.unlock(0) catch false);
    assert(lk.lockOrAssert(123) catch false);
    assert(lk.transfer(1, 2) catch false);
    assertEq(lk.violation_count, 0);
});

test("ThreadLock: error guards (AlreadyLocked, NotLocked, WrongThread, InvalidThreadId)", () => {
    let lk = ThreadLock.initUnlocked();

    // InvalidThreadId on lock
    let got_invalid = false;
    * = lk.lock(THREAD_LOCK_INVALID_ID) catch |e| {
        assertEq(e, ThreadLockError.InvalidThreadId);
        got_invalid = true;
        false
    };
    assert(got_invalid);

    // NotLocked on assertLocked & unlock
    let got_not_locked = false;
    * = lk.assertLocked(7) catch |e| {
        assertEq(e, ThreadLockError.NotLocked);
        got_not_locked = true;
        false
    };
    assert(got_not_locked);

    let got_unlock_not_locked = false;
    * = lk.unlock(7) catch |e| {
        assertEq(e, ThreadLockError.NotLocked);
        got_unlock_not_locked = true;
        false
    };
    assert(got_unlock_not_locked);

    // AlreadyLocked on double lock (even same thread, matching Bun ThreadLock non-reentrant semantics)
    * = lk.lock(7) catch {
        assert(false);
        return;
    };
    let got_already = false;
    * = lk.lock(7) catch |e| {
        assertEq(e, ThreadLockError.AlreadyLocked);
        got_already = true;
        false
    };
    assert(got_already);

    // WrongThread on unlock
    let got_wrong_unlock = false;
    * = lk.unlock(8) catch |e| {
        assertEq(e, ThreadLockError.WrongThread);
        got_wrong_unlock = true;
        false
    };
    assert(got_wrong_unlock);
    assertEq(lk.violation_count, 5);
});
