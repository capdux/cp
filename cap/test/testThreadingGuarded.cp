// Production deep regression tests for lib/threading/guarded.cp (cap:threading)

import { test, expect_eq } from "cap:test";
import { Guarded, GuardedBy, SpinLock } from "cap:threading";

struct CounterPair {
    a: i64 = 0,
    b: i64 = 0,
}

struct MockLock {
    held: bool = false,
    lock_count: u32 = 0,
    unlock_count: u32 = 0,

    fn lock(*_) {
        _.held = true;
        _.lock_count += 1;
    }

    fn tryLock(*_): bool {
        if (_.held) return false;
        _.held = true;
        _.lock_count += 1;
        return true;
    }

    fn unlock(*_) {
        _.held = false;
        _.unlock_count += 1;
    }
}

test("guarded: basic lock, mutate, tryLock contention & unlock", () => {
    let g = Guarded<i64>.init(10);
    expect_eq(g.isLocked(), false);

    // Lock and mutate in-place
    const ptr = g.lock();
    expect_eq(g.isLocked(), true);
    ptr.* += 32;

    // While locked, tryLock must fail cleanly without blocking
    expect_eq(g.tryLock() == null, true);

    g.unlock();
    expect_eq(g.isLocked(), false);
    expect_eq(g.getCopy(), 42);
});

test("guarded: tryLock success path & automatic unlock helpers", () => {
    let g = Guarded<u32>.init(7);

    if (v := g.tryLock()) {
        v.* = 99;
        g.unlock();
    }
    expect_eq(g.isLocked(), false);
    expect_eq(g.getCopy(), 99);

    // set & swap automatically acquire and release the lock
    g.set(123);
    expect_eq(g.isLocked(), false);
    const old = g.swap(456);
    expect_eq(old, 123);
    expect_eq(g.getCopy(), 456);
    expect_eq(g.isLocked(), false);
});

test("guarded: struct payload mutation & intoUnprotected extraction", () => {
    let g = Guarded<CounterPair>.init(CounterPair{ a: 1, b: 2 });

    const p = g.lock();
    p.a += 10;
    p.b += 20;
    g.unlock();

    const snap = g.getCopy();
    expect_eq(snap.a, 11);
    expect_eq(snap.b, 22);

    const raw = g.intoUnprotected();
    expect_eq(raw.a, 11);
    expect_eq(raw.b, 22);
});

test("guardedBy: custom lock protocol invocation & contention tracking", () => {
    let gb = GuardedBy<i32, MockLock>.initWithLock(5, MockLock{});

    const v = gb.lock();
    v.* = 15;
    expect_eq(gb.tryLock() == null, true);
    gb.unlock();

    expect_eq(gb.mutex.lock_count, 1);
    expect_eq(gb.mutex.unlock_count, 1);
    expect_eq(gb.mutex.held, false);

    const prev = gb.swap(25);
    expect_eq(prev, 15);
    expect_eq(gb.getCopy(), 25);
    expect_eq(gb.mutex.lock_count, 3);
    expect_eq(gb.mutex.unlock_count, 3);
});

test("guardedBy: SpinLock explicit composition", () => {
    let gb = GuardedBy<u64, SpinLock>.initWithLock(100, SpinLock.init());
    gb.set(200);
    expect_eq(gb.swap(300), 200);
    expect_eq(gb.intoUnprotected(), 300);
});
