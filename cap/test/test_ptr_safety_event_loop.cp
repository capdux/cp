// ============================================================================
// cap/test/test_ptr_safety_event_loop.cp — cap:ptr, cap:safety, cap:event_loop
// 第四阶段三大核心运行时与内存安全标准库端到端全量回归测试套件
// ============================================================================

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Shared,
    AtomicShared,
    Cow,
    CowSlice,
    Owned,
    TaggedPointer,
    LowTagPointer,
    RawRefCount,
    ThreadSafety,
    DecrementResult,
    RefCount,
    RefCountOptions,
    RefPtr,
} from "cap:ptr";
import {
    invalid,
    getCurrent,
    isCurrent,
    isValid,
    ThreadLock,
    CriticalSection,
    CheckedAllocator,
    assertAllocatorEq,
    poison,
    unpoison,
    isPoisoned,
    assertPoisoned,
    assertUnpoisoned,
    safety_assert,
} from "cap:safety";
import {
    TimeSpec,
    TimerState,
    Timer,
    TimerHeap,
    ConcurrentTask,
    ConcurrentTaskQueue,
    DeferredTaskQueue,
    AnyTask,
    ManagedTask,
    AutoFlusher,
    EventLoop,
} from "cap:event_loop";

type PtrI32 = *i32;
type PtrU64 = *u64;
type PtrI64 = *i64;

struct IntrusiveItem {
    val: i32 = 0,
    ref_count: RefCount(IntrusiveItem, "ref_count", IntrusiveItem.destroy, RefCountOptions{}) = RefCount(IntrusiveItem, "ref_count", IntrusiveItem.destroy, RefCountOptions{}).init(),

    pub static fn destroy(item: *IntrusiveItem) {
        std.heap.page_allocator.destroy(item);
    }
}

struct LoopCounter {
    concurrent_hits: i32 = 0,
    deferred_hits: i32 = 0,
    timer_hits: i32 = 0,
}

fn onConcurrentRun(ctx: (*anyopaque)?) {
    if (c := ctx) {
        let lc: *LoopCounter = @alignCast(@ptrCast(c));
        lc.concurrent_hits += 1;
    }
}

fn onDeferredFlush(ctx: *anyopaque): bool {
    let lc: *LoopCounter = @alignCast(@ptrCast(ctx));
    lc.deferred_hits += 1;
    return false;
}

fn onTimerFired(t: *Timer) {
    if (c := t.ctx) {
        let lc: *LoopCounter = @alignCast(@ptrCast(c));
        lc.timer_hits += 1;
    }
}

fn testPtrSuite(): !void {
    // 1. Shared<*i32> & Weak<*i32>
    let s1 = Shared(PtrI32).new(100);
    let s2 = s1.clone();
    expect_eq(s1.strongCount() as i64, 2);
    s2.get().* = 250;
    expect_eq(s1.get().*, 250);

    let w = s1.cloneWeak();
    if (up_opt := w.upgrade()) {
        let up = up_opt;
        expect_eq(up.get().*, 250);
        expect_eq(up.strongCount() as i64, 3);
        up.deinit();
    }
    s1.deinit();
    s2.deinit();
    expect_eq(w.upgrade() == null, true);
    w.deinit();

    // 2. AtomicShared<*u64>
    let a1 = AtomicShared(PtrU64).new(9001);
    let a2 = a1.clone();
    expect_eq(a1.strongCount() as i64, 2);
    a1.deinit();
    expect_eq(a2.isUnique(), true);
    a2.deinit();

    // 3. Cow<i32> & CowSlice<u8>
    let stack_v: i32 = 55;
    let cow = Cow(i32).borrow(&stack_v);
    expect_eq(cow.isOwned(), false);
    let mut_ptr = cow.toOwned(std.heap.page_allocator);
    mut_ptr.* = 88;
    expect_eq(cow.isOwned(), true);
    expect_eq(cow.inner().*, 88);
    expect_eq(stack_v, 55);
    cow.deinit(std.heap.page_allocator);

    let cs = CowSlice(u8).initStatic("cp_runtime");
    let sub = cs.borrowSubslice(0, 2);
    expect_eq(sub.isOwned(), false);
    expect_eq(sub.slice(), "cp");
    let sm = sub.sliceMut(std.heap.page_allocator) catch unreachable;
    sm[0] = 'C';
    sm[1] = 'P';
    expect_eq(sub.isOwned(), true);
    expect_eq(sub.slice(), "CP");
    expect_eq(cs.slice(), "cp_runtime");
    sub.deinit(std.heap.page_allocator);

    // 4. Owned<*i64>, TaggedPointer, LowTagPointer, RefPtr & RawRefCount
    let owned = Owned(PtrI64).new(4096);
    expect_eq(owned.get().*, 4096);
    owned.deinit();

    let target: u64 = 0xDEADBEEF;
    let tp = TaggedPointer.fromPtr(&target, 0xCAFE);
    expect_eq(tp.tag() as i64, 0xCAFE);
    expect_eq(tp.get(u64).*, 0xDEADBEEF);

    let lp = LowTagPointer.fromPtr(&target, 0x3);
    expect_eq(lp.tag() as i64, 3);
    expect_eq(lp.get(u64).*, 0xDEADBEEF);

    let item_ptr = RefPtr(IntrusiveItem).new(IntrusiveItem{
        val: 77,
        ref_count: RefCount(IntrusiveItem, "ref_count", IntrusiveItem.destroy, RefCountOptions{}).init(),
    });
    let item_dup = item_ptr.dupeRef();
    expect_eq(item_ptr.get().ref_count.get() as i64, 2);
    item_ptr.deref();
    expect_eq(item_dup.get().ref_count.hasOneRef(), true);
    item_dup.deref();

    let raw_rc = RawRefCount(u32, ThreadSafety.single_threaded).init(1);
    raw_rc.increment();
    expect_eq(raw_rc.decrement() == DecrementResult.keep_alive, true);
    expect_eq(raw_rc.decrement() == DecrementResult.should_destroy, true);
}

fn testSafetySuite(): !void {
    const tid = getCurrent();
    expect_eq(isValid(tid), true);
    expect_eq(isValid(invalid), false);
    expect_eq(isCurrent(tid), true);

    let tlock = ThreadLock.initUnlocked();
    expect_eq(tlock.isLocked(), false);
    tlock.lock();
    expect_eq(tlock.isLocked(), true);
    expect_eq(tlock.isHeldByCurrentThread(), true);
    tlock.assertLocked();
    tlock.lockOrAssert();
    tlock.unlock();
    tlock.assertNotLocked();

    let cs = CriticalSection.init();
    cs.begin();
    cs.begin(); // Reentrant exclusive write lock
    expect_eq(cs.isLocked(), true);
    expect_eq(cs.isExclusiveHeldByCurrentThread(), true);
    cs.end();
    expect_eq(cs.isLocked(), true);
    cs.end();
    expect_eq(cs.isLocked(), false);

    cs.beginReadOnly();
    expect_eq(cs.isLocked(), true);
    expect_eq(cs.isExclusiveHeldByCurrentThread(), false);
    cs.end();
    expect_eq(cs.isLocked(), false);

    const alloc = std.heap.page_allocator;
    assertAllocatorEq(alloc, alloc);
    let checked = CheckedAllocator.init(alloc);
    expect_eq(checked.isSet(), true);
    checked.set(alloc);
    checked.assertEq(alloc);
    checked.reset();
    expect_eq(checked.isSet(), false);

    let sample: u64 = 123;
    const raw_p: *const anyopaque = @ptrCast(&sample);
    poison(raw_p, 8);
    expect_eq(isPoisoned(raw_p), false);
    unpoison(raw_p, 8);
    assertPoisoned(raw_p);
    assertUnpoisoned(raw_p);
    safety_assert(sample == 123, "sample intact");
}

fn testEventLoopSuite(): !void {
    let counts = LoopCounter{
        concurrent_hits: 0,
        deferred_hits: 0,
        timer_hits: 0,
    };

    // 1. TimerHeap pairing min-heap ordering & cancellation
    let heap = TimerHeap.init();
    let t_late = Timer{ expires_at_ms: 500, callback: onTimerFired, ctx: @ptrCast(&counts) };
    let t_early = Timer{ expires_at_ms: 100, callback: onTimerFired, ctx: @ptrCast(&counts) };
    let t_mid = Timer{ expires_at_ms: 250, callback: onTimerFired, ctx: @ptrCast(&counts) };

    heap.insert(&t_late);
    heap.insert(&t_early);
    heap.insert(&t_mid);
    expect_eq(heap.count(), 3 as usize);
    expect_eq(heap.peek().?.expires_at_ms, 100 as i64);

    expect_eq(heap.remove(&t_late), true);
    expect_eq(t_late.isCancelled(), true);
    expect_eq(heap.fireExpired(300), 2 as usize);
    expect_eq(counts.timer_hits, 2);
    expect_eq(heap.isEmpty(), true);

    // 2. ManagedTask cancellation
    let mtask = ManagedTask.init(@ptrCast(&counts), onConcurrentRun);
    mtask.cancel();
    expect_eq(mtask.isCancelled(), true);
    mtask.run();
    expect_eq(counts.concurrent_hits, 0);

    // 3. Full EventLoop tick with ConcurrentTask, AutoFlusher/DeferredTaskQueue & Timer
    let loop = EventLoop.init();
    let flusher = AutoFlusher.init();

    expect_eq(loop.postTask(onConcurrentRun, @ptrCast(&counts)), true);
    expect_eq(flusher.registerDeferred(&loop.deferred_tasks, @ptrCast(&counts), onDeferredFlush), true);
    // Duplicate registration while already registered returns false
    expect_eq(flusher.registerDeferred(&loop.deferred_tasks, @ptrCast(&counts), onDeferredFlush), false);

    const timer_slot = loop.setTimeout(0, onTimerFired, @ptrCast(&counts));
    expect_eq(timer_slot != null, true);

    const processed = loop.tick();
    expect_eq(processed, 3 as usize);
    expect_eq(counts.concurrent_hits, 1);
    expect_eq(counts.deferred_hits, 1);
    expect_eq(counts.timer_hits, 3);
}

fn main() {
    test("1. cap:ptr — Shared/Weak, AtomicShared, Cow/CowSlice, Owned, TaggedPointer & RefPtr", () => {
        testPtrSuite()?;
    });

    test("2. cap:safety — ThreadLock, CriticalSection, CheckedAllocator & ASAN primitives", () => {
        testSafetySuite()?;
    });

    test("3. cap:event_loop — TimerHeap, ConcurrentTaskQueue, AutoFlusher, ManagedTask & EventLoop", () => {
        testEventLoopSuite()?;
    });
}
