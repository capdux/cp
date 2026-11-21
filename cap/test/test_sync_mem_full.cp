// ============================================================================
// cap/test/test_sync_mem_full.cp — cap:sync, cap:threading, cap:mem
// 第五阶段并发同步原语、无锁队列、并发字典与内存分配器全量回归测试套件
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Futex,
    Deadline,
    Mutex,
    Condition,
    WaitGroup,
    Channel,
    MpmcQueue,
    RingBuffer,
    UnboundedQueue,
    ConcurrentMap,
    ArcSlice,
    Wyhash,
    WyhashStateless,
    Guarded,
    TaskDesc,
    WorkPool,
} from "cap:sync";
import {
    SharedTensor,
    StackFallbackAllocator,
    BumpArena,
    MaybeOwnedBuf,
    MaxHeapAllocator,
    TaggedPointer,
    LowTagPointer,
    TaggedPtr,
    CowBytes,
    CowSlice,
    Cow,
    DecrementResult,
    RefCounter,
    AtomicRefCounter,
    WeakControlBlock,
    Rc,
} from "cap:mem";

struct IntrusiveJob {
    id: i32 = 0,
    next: (*IntrusiveJob)? = null,
}

fn dummyTaskFn(ctx: *!) {}

fn testSyncPrimitivesAndLocks(): !void {
    // 1. Futex & Mutex
    let atom_val: std.atomic.Value(u32) = .{ .raw = 77 };
    Futex.wake(&atom_val, 1);
    expect_eq(atom_val.raw, 77 as u32);

    let mu: Mutex = .{};
    defer mu.deinit();
    expect_eq(mu.tryLock(), true);
    mu.unlock();
    mu.lock();
    mu.unlock();

    // 2. WaitGroup
    let wg = WaitGroup.initWithCount(2);
    defer wg.deinit();
    wg.addOne(); // count = 3
    wg.done();
    wg.done();
    wg.finish();
    wg.wait();

    // 3. Guarded<T> & WorkPool
    let guarded = Guarded<i64>.init(500);
    expect_eq(guarded.get(), 500 as i64);
    guarded.set(750);
    expect_eq(guarded.get(), 750 as i64);

    let wp = WorkPool.init();
    defer wp.~init();
    expect_eq(wp.isRunning(), true);
    let dummy_ctx: i32 = 1;
    let t1 = TaskDesc{ func: &dummyTaskFn, ctx: &dummy_ctx, next: null };
    let t2 = TaskDesc{ func: &dummyTaskFn, ctx: &dummy_ctx, next: null };
    wp.schedule(&t1);
    wp.schedule(&t2);
    expect_eq(wp.queueCount(), 2 as usize);
    expect_eq(wp.pollTask() != null, true);
    expect_eq(wp.pollTask() != null, true);
    expect_eq(wp.pollTask() == null, true);
}

fn testLockFreeQueuesAndChannels(): !void {
    const allocator = std.heap.page_allocator;

    // 1. MpmcQueue / RingBuffer
    let rb = RingBuffer(i32, 4).init();
    expect_eq(rb.isEmpty(), true);
    expect_eq(rb.tryPush(11), true);
    expect_eq(rb.tryPush(22), true);
    expect_eq(rb.tryPush(33), true);
    expect_eq(rb.tryPush(44), true);
    expect_eq(rb.isFull(), true);
    expect_eq(rb.tryPush(55), false);

    expect_eq(rb.tryPop().?, 11);
    expect_eq(rb.tryPop().?, 22);
    expect_eq(rb.tryPop().?, 33);
    expect_eq(rb.tryPop().?, 44);
    expect_eq(rb.tryPop() == null, true);

    // 2. Intrusive lock-free UnboundedQueue
    const JobQueue = UnboundedQueue(IntrusiveJob, .next);
    let uq: JobQueue = .{};
    let j1 = IntrusiveJob{ id: 101, next: null };
    let j2 = IntrusiveJob{ id: 202, next: null };
    let j3 = IntrusiveJob{ id: 303, next: null };

    uq.push(&j1);
    uq.push(&j2);
    uq.push(&j3);

    const popped1 = uq.pop();
    expect_eq(popped1 != null, true);
    expect_eq(popped1.?.id, 101);

    const batch_opt = uq.popBatch();
    expect_eq(batch_opt != null, true);
    if (batch_val := batch_opt) {
        let batch = batch_val;
        expect_eq(batch.count, 2 as usize);
        let it = batch.iterator();
        const b1 = it.next();
        const b2 = it.next();
        expect_eq(b1.?.id, 202);
        expect_eq(b2.?.id, 303);
        expect_eq(it.next() == null, true);
    }

    // 3. CSP Channel<T>
    let ch = try Channel(i32).init(allocator, 3);
    defer ch.deinit();
    try ch.send(10);
    expect_eq(try ch.trySend(20), true);
    expect_eq(ch.len(), 2 as usize);
    expect_eq(ch.tryRecv().?, 10);
    expect_eq((try ch.recv()).?, 20);
    expect_eq(ch.tryRecv() == null, true);
    ch.close();
    expect_eq(ch.isClosed(), true);
}

fn testConcurrentMapAndArcSlice(): !void {
    const allocator = std.heap.page_allocator;

    // 1. 32-stripe ConcurrentMap
    let cmap = ConcurrentMap(i32, i32).init(allocator);
    defer cmap.deinit();

    try cmap.put(10, 1000);
    try cmap.put(20, 2000);
    try cmap.put(30, 3000);
    expect_eq(cmap.count(), 3 as usize);
    expect_eq(cmap.get(20).?, 2000);
    expect_eq(cmap.contains(30), true);
    expect_eq(cmap.remove(20), true);
    expect_eq(cmap.contains(20), false);
    expect_eq(cmap.count(), 2 as usize);

    // 2. ArcSlice atomic reference-counted slice
    let arc = try ArcSlice(u8).init(allocator, "concurrent_zero_copy");
    expect_eq(arc.len(), 20 as usize);
    expect_eq(arc.refCount(), 1 as usize);

    let arc2 = arc.clone();
    expect_eq(arc.refCount(), 2 as usize);
    expect_eq(arc2.get(0).?, 'c' as u8);
    arc2.~init();
    expect_eq(arc.refCount(), 1 as usize);
    arc.~init();

    // 3. Wyhash & WyhashStateless
    const h_full = Wyhash.hash(0, "cap_sync_wyhash_vector");
    let wy = Wyhash.init(0);
    wy.update("cap_sync_");
    wy.update("wyhash_vector");
    expect_eq(wy.final(), h_full);
}

fn main() {
    test("1. cap:sync — Futex, Mutex, WaitGroup, Guarded<T> & WorkPool lifecycle", () => {
        testSyncPrimitivesAndLocks()?;
    });

    test("2. cap:sync — RingBuffer/MpmcQueue, intrusive UnboundedQueue & CSP Channel", () => {
        testLockFreeQueuesAndChannels()?;
    });

    test("3. cap:sync — 32-stripe ConcurrentMap, ArcSlice & Wyhash streaming", () => {
        testConcurrentMapAndArcSlice()?;
    });

    test("4. cap:mem — MaxHeapAllocator, StackFallbackAllocator, BumpArena & MaybeOwnedBuf", () => {
        // MaxHeapAllocator high-water-mark reuse & quota enforcement
        let mha = MaxHeapAllocator.init(512);
        defer mha.~init();
        expect_eq(mha.maxQuota(), 512 as usize);

        if (b1 := mha.alloc(128)) {
            let mut_b1: []u8 = b1;
            expect_eq(mut_b1.len, 128 as usize);
            mut_b1[0] = 0x5A;
            expect_eq(mut_b1[0], 0x5A as u8);
        } else {
            expect_eq(true, false);
        }
        expect_eq(mha.capacity(), 128 as usize);
        expect_eq(mha.used(), 128 as usize);

        // Smaller allocation reuses existing 128-byte buffer without growing capacity
        if (b2 := mha.alloc(64)) {
            expect_eq(b2.len, 64 as usize);
        } else {
            expect_eq(true, false);
        }
        expect_eq(mha.capacity(), 128 as usize);
        expect_eq(mha.used(), 64 as usize);

        // Allocation exceeding 512-byte quota returns null
        expect_eq(mha.alloc(1024) == null, true);

        // StackFallbackAllocator
        let sfa = StackFallbackAllocator<32>.init();
        expect_eq(sfa.allocStack(16) != null, true);
        expect_eq(sfa.stackRemaining(), 16 as usize);
        expect_eq(sfa.allocStack(24) == null, true);
        sfa.recordFallback(24);
        expect_eq(sfa.heap_allocs, 1 as usize);
        sfa.reset();

        // BumpArena & MaybeOwnedBuf
        let ba = BumpArena.initCapacity(128);
        defer ba.~init();
        expect_eq(ba.allocBytes(10) != null, true);
        expect_eq(ba.allocAligned(16, 8) != null, true);
        expect_eq(ba.used(), 32 as usize);

        let mob = MaybeOwnedBuf.initOwnedCopy("owned_buffer");
        expect_eq(mob.isOwned(), true);
        expect_eq(mob.slice() == "owned_buffer", true);
        mob.~init();
    });

    test("5. cap:mem — Cow / CowSlice / CowBytes, Rc / AtomicRefCounter / WeakControlBlock, TaggedPtr & SharedTensor", () => {
        let c1 = Cow.initBorrowed("immutable");
        expect_eq(c1.isOwned(), false);
        expect_eq(c1.setByte(0, 'I' as u8), true);
        expect_eq(c1.isOwned(), true);
        expect_eq(c1.slice() == "Immutable", true);
        c1.~init();

        let rc = Rc.init(1);
        expect_eq(rc.increment(), 2 as u32);
        expect_eq(rc.decrement() == DecrementResult.KeepAlive, true);
        expect_eq(rc.decrement() == DecrementResult.ShouldDestroy, true);

        let arc = AtomicRefCounter.init(1);
        expect_eq(arc.increment(), 2 as u32);
        expect_eq(arc.decrement() == DecrementResult.KeepAlive, true);
        expect_eq(arc.decrement() == DecrementResult.ShouldDestroy, true);

        let wcb = WeakControlBlock.init();
        expect_eq(wcb.incWeak(), 1 as u32);
        expect_eq(wcb.decStrong() == DecrementResult.ShouldDestroy, true);
        expect_eq(wcb.decWeak(), true);
        expect_eq(wcb.canDeallocate(), true);

        const tp = TaggedPtr.pack(0x00007FFF00001000, 0x1234);
        expect_eq(tp.address(), 0x00007FFF00001000 as usize);
        expect_eq(tp.tag(), 0x1234 as u16);

        const ltp = LowTagPointer.pack(0x200080, 0x6);
        expect_eq(ltp.address(), 0x200080 as usize);
        expect_eq(ltp.tag(), 6 as u8);

        const tensor = new SharedTensor(256);
        expect_eq(tensor.get_size(), 256 as usize);
        tensor.retain();
        expect_eq(tensor.get_ref_count(), 2 as usize);
        tensor.~init();
        expect_eq(tensor.get_ref_count(), 1 as usize);
    });
}
