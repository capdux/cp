import { test, assert, assertEq } from "cap:test";
import {
    BUILD_WORKER_INVALID_ID,
    BuildWorkerPoolError,
    WorkerPlatformKind,
    WorkerPlanInput,
    planWorkerCount,
    BuildWorkPriority,
    BuildWorkItem,
    BuildWorkerDeque,
    BuildWorkPoolState,
    BuildWorkerSlot,
    BuildWorkPollSource,
    BuildWorkPool,
    formatBuildWorkPoolReport,
    IoPoolLifecycle,
    evaluateWorkerLoadBalance,
} from "cap:bundler";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strContains(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("planWorkerCount adapts to CPU cores, module count, memory ceiling, and platform I/O heuristics", () => {
    // macOS 12-core with 1000 modules -> 12 workers, I/O pool enabled with 4 threads, warm=8
    const mac = planWorkerCount(WorkerPlanInput{
        cpu_cores: 12,
        module_count: 1000,
        memory_limit_mb: 0,
        platform: WorkerPlatformKind.MacOS,
    });
    assertEq(mac.worker_count, @as(u32, 12));
    assert(mac.use_io_pool);
    assertEq(mac.io_thread_count, @as(u32, 4));
    assertEq(mac.warm_worker_count, @as(u32, 8));
    assert(mac.recommended_queue_capacity >= 128);

    // Linux 16-core capped by 256MB memory (64MB/worker -> 4 workers), I/O pool disabled
    const lin = planWorkerCount(WorkerPlanInput{
        cpu_cores: 16,
        module_count: 500,
        memory_limit_mb: 256,
        platform: WorkerPlatformKind.Linux,
    });
    assertEq(lin.worker_count, @as(u32, 4));
    assert(!lin.use_io_pool);
    assertEq(lin.io_thread_count, @as(u32, 0));

    // Small project (2 modules on 32 cores) -> only 2 workers
    const small = planWorkerCount(WorkerPlanInput{
        cpu_cores: 32,
        module_count: 2,
        platform: WorkerPlatformKind.Linux,
    });
    assertEq(small.worker_count, @as(u32, 2));
});

test("BuildWorkerDeque supports local LIFO push/pop and cross-core FIFO stealOne/stealHalf", () => {
    let slots: [8]BuildWorkItem = [BuildWorkItem.empty(); 8];
    let dq = BuildWorkerDeque.init(slots[0..]);
    assert(dq.isEmpty());

    let i: u32 = 1;
    while (i <= 6) {
        dq.pushLocal(BuildWorkItem{ task_id: i, module_index: i }) catch unreachable;
        i += 1;
    }
    assertEq(dq.len(), @as(usize, 6));

    // Local pop is LIFO -> returns task 6
    const local = dq.popLocal() orelse unreachable;
    assertEq(local.task_id, @as(u32, 6));
    assertEq(dq.len(), @as(usize, 5));

    // StealOne is FIFO -> returns oldest task 1
    const stolen1 = dq.stealOne() orelse unreachable;
    assertEq(stolen1.task_id, @as(u32, 1));
    assertEq(dq.len(), @as(usize, 4));

    // StealHalf steals ceil(4 / 2) = 2 oldest tasks: 2 and 3
    let out: [4]BuildWorkItem = [BuildWorkItem.empty(); 4];
    const half = dq.stealHalf(out[0..]);
    assertEq(half, @as(usize, 2));
    assertEq(out[0].task_id, @as(u32, 2));
    assertEq(out[1].task_id, @as(u32, 3));
    assertEq(dq.len(), @as(usize, 2));

    // Remaining are 4 and 5; popLocal yields 5 then 4
    assertEq((dq.popLocal() orelse unreachable).task_id, @as(u32, 5));
    assertEq((dq.popLocal() orelse unreachable).task_id, @as(u32, 4));
    assert(dq.isEmpty());
});

test("BuildWorkPool steals work across cores and prioritizes High/Critical fast-lane items", () => {
    let q0: [16]BuildWorkItem = [BuildWorkItem.empty(); 16];
    let q1: [16]BuildWorkItem = [BuildWorkItem.empty(); 16];
    let prio: [8]BuildWorkItem = [BuildWorkItem.empty(); 8];
    let lat: [32]u64 = [0; 32];
    let workers: [2]BuildWorkerSlot = [
        BuildWorkerSlot.init(0, q0[0..]),
        BuildWorkerSlot.init(1, q1[0..]),
    ];

    let pool = BuildWorkPool.init(workers[0..], prio[0..], lat[0..], 0);

    // Load 6 normal tasks onto worker 0, leaving worker 1 empty
    let id: u32 = 1;
    while (id <= 6) {
        pool.submit(BuildWorkItem{ task_id: id, priority: BuildWorkPriority.Normal }, 0) catch unreachable;
        id += 1;
    }
    // Submit 1 Critical task -> enters priority fast lane
    pool.submit(BuildWorkItem{ task_id: 99, priority: BuildWorkPriority.Critical }, 0) catch unreachable;

    // Worker 1 polls -> gets Critical task 99 first from PriorityQueue
    const r0 = (pool.pollNext(1) catch unreachable) orelse unreachable;
    assertEq(r0.item.task_id, @as(u32, 99));
    assert(r0.source == BuildWorkPollSource.PriorityQueue);
    pool.recordCompleted(1, 50, 80) catch unreachable;

    // Worker 1 polls again -> local deque empty, steals half (3 items) from worker 0!
    const r1 = (pool.pollNext(1) catch unreachable) orelse unreachable;
    assert(r1.source == BuildWorkPollSource.StolenFromPeer);
    assertEq(r1.victim_worker_id, @as(u32, 0));
    assertEq(r1.item.task_id, @as(u32, 1));
    assertEq(workers[1].deque.len(), @as(usize, 2)); // 2 stolen siblings queued locally
    assertEq(workers[0].deque.len(), @as(usize, 3));
    pool.recordCompleted(1, 100, 120) catch unreachable;

    // Drain remaining tasks
    let drained: u32 = 2;
    while (pool.pendingCount() > 0) {
        if (p0 := (pool.pollNext(0) catch unreachable)) {
            * = p0;
            pool.recordCompleted(0, 80, 100 + @as(u64, drained) * 20) catch unreachable;
            drained += 1;
        }
        if (p1 := (pool.pollNext(1) catch unreachable)) {
            * = p1;
            pool.recordCompleted(1, 90, 110 + @as(u64, drained) * 20) catch unreachable;
            drained += 1;
        }
    }

    let scratch: [32]u64 = [0; 32];
    const st = pool.stats(scratch[0..]);
    assertEq(st.total_submitted, @as(u64, 7));
    assertEq(st.total_completed, @as(u64, 7));
    assertEq(st.total_cancelled, @as(u64, 0));
    assert(st.total_steals >= 3);
    assert(st.p95_latency_us >= 180);

    const bal = evaluateWorkerLoadBalance(workers[0..]);
    assertEq(bal.total_completed, @as(i32, 7));
    assert(bal.min_completed >= 3 && bal.max_completed <= 4);

    let rep: [1024]u8 = [0; 1024];
    const rlen = formatBuildWorkPoolReport(st, workers[0..], rep[0..]) catch unreachable;
    assert(strContains(rep[0..rlen], "BuildWorkPool [Running] workers=2"));
    assert(strContains(rep[0..rlen], "completed=7 cancelled=0"));
});

test("BuildWorkPool enforces backpressure, graceful shutdownDrain, and emergency abortAll conservation", () => {
    let q0: [8]BuildWorkItem = [BuildWorkItem.empty(); 8];
    let q1: [8]BuildWorkItem = [BuildWorkItem.empty(); 8];
    let prio: [4]BuildWorkItem = [BuildWorkItem.empty(); 4];
    let lat: [16]u64 = [0; 16];
    let workers: [2]BuildWorkerSlot = [
        BuildWorkerSlot.init(0, q0[0..]),
        BuildWorkerSlot.init(1, q1[0..]),
    ];

    // Max pending backpressure = 4
    let pool = BuildWorkPool.init(workers[0..], prio[0..], lat[0..], 4);
    const batch_items: [6]BuildWorkItem = [
        BuildWorkItem{ task_id: 1 },
        BuildWorkItem{ task_id: 2 },
        BuildWorkItem{ task_id: 3 },
        BuildWorkItem{ task_id: 4 },
        BuildWorkItem{ task_id: 5 },
        BuildWorkItem{ task_id: 6 },
    ];
    const accepted = pool.submitBatch(batch_items[0..], 0) catch unreachable;
    assertEq(accepted, @as(usize, 4));

    // Additional submit must be rejected by backpressure
    let bp_err = false;
    pool.submit(BuildWorkItem{ task_id: 7 }, 0) catch {
        bp_err = true;
    };
    assert(bp_err);

    // Graceful shutdownDrain rejects new submits but still lets workers drain
    pool.shutdownDrain();
    let closed_err = false;
    pool.submit(BuildWorkItem{ task_id: 8 }, 0) catch {
        closed_err = true;
    };
    assert(closed_err);

    const popped = (pool.pollNext(0) catch unreachable) orelse unreachable;
    * = popped;
    pool.recordCompleted(0, 50, 50) catch unreachable;

    // Emergency abortAll cancels remaining 3 queued items -> conservation holds!
    const cancelled = pool.abortAll();
    assertEq(cancelled, @as(u64, 3));
    assert(pool.state == BuildWorkPoolState.Aborted);
    assert((pool.pollNext(0) catch unreachable) == null);

    let scratch: [16]u64 = [0; 16];
    const st = pool.stats(scratch[0..]);
    assertEq(st.total_submitted, st.total_completed + st.total_cancelled);
    assertEq(st.total_completed, @as(u64, 1));
    assertEq(st.total_cancelled, @as(u64, 3));
    assert(st.total_rejected_backpressure >= 2);
});

test("IoPoolLifecycle enforces ref-counted acquire, release, and idle-only shutdown", () => {
    let io = IoPoolLifecycle.init(16);
    assertEq(io.max_threads, @as(u32, 4));
    assert(!io.isRunning());

    // First acquire initializes pool (ref_count = 2)
    assert(io.acquire());
    assert(io.isRunning());
    assert(!io.tryShutdown()); // active holder exists

    // Second acquire shares pool (ref_count = 3)
    assert(!io.acquire());
    io.release() catch unreachable; // ref_count = 2
    assert(!io.tryShutdown());

    io.release() catch unreachable; // ref_count = 1 (idle)
    assert(io.tryShutdown()); // succeeds!
    assert(!io.isRunning());
    assertEq(io.total_shutdowns, @as(u64, 1));
});

test("BuildWorkPool rejects invalid worker IDs cleanly", () => {
    let q0: [4]BuildWorkItem = [BuildWorkItem.empty(); 4];
    let prio: [2]BuildWorkItem = [BuildWorkItem.empty(); 2];
    let lat: [4]u64 = [0; 4];
    let workers: [1]BuildWorkerSlot = [BuildWorkerSlot.init(0, q0[0..])];
    let pool = BuildWorkPool.init(workers[0..], prio[0..], lat[0..], 0);

    let bad_poll = false;
    * = pool.pollNext(5) catch {
        bad_poll = true;
    };
    assert(bad_poll);

    let bad_rec = false;
    pool.recordCompleted(9, 10, 10) catch {
        bad_rec = true;
    };
    assert(bad_rec);
});

test("deterministic 500-iteration multi-core work-stealing & backpressure fuzz preserves exact task conservation", () => {
    let seed: u64 = 0x2545f4914f6cdd1d;
    let q0: [16]BuildWorkItem = [BuildWorkItem.empty(); 16];
    let q1: [16]BuildWorkItem = [BuildWorkItem.empty(); 16];
    let q2: [16]BuildWorkItem = [BuildWorkItem.empty(); 16];
    let q3: [16]BuildWorkItem = [BuildWorkItem.empty(); 16];
    let prio: [8]BuildWorkItem = [BuildWorkItem.empty(); 8];
    let lat: [64]u64 = [0; 64];
    let scratch: [64]u64 = [0; 64];
    let executed_flags: [64]u8 = [0; 64];

    let iter: u32 = 0;
    let total_tasks_processed: u64 = 0;
    while (iter < 500) {
        seed = seed ^ (seed >> 12);
        seed = seed ^ (seed << 25);
        seed = seed ^ (seed >> 27);
        let r = seed *% 2685821657736338717;

        let workers: [4]BuildWorkerSlot = [
            BuildWorkerSlot.init(0, q0[0..]),
            BuildWorkerSlot.init(1, q1[0..]),
            BuildWorkerSlot.init(2, q2[0..]),
            BuildWorkerSlot.init(3, q3[0..]),
        ];
        const bp_limit: usize = @as(usize, @intCast((r % 20) + 8));
        let pool = BuildWorkPool.init(workers[0..], prio[0..], lat[0..], bp_limit);

        let f: usize = 0;
        while (f < 64) {
            executed_flags[f] = 0;
            f += 1;
        }

        let next_id: u32 = 0;
        let step: u32 = 0;
        const do_abort = (r & 7) == 0;

        while (step < 40) {
            r = r *% 6364136223846793005 +% 1442695040888963407;
            if ((r & 1) == 0 && next_id < 60 && pool.state == BuildWorkPoolState.Running) {
                const pr = if ((r & 6) == 0) BuildWorkPriority.High else BuildWorkPriority.Normal;
                const pref = @as(u32, @intCast((r >> 16) % 4));
                pool.submit(BuildWorkItem{ task_id: next_id, module_index: next_id, priority: pr }, pref) catch {};
                next_id += 1;
            } else {
                const wid = @as(u32, @intCast((r >> 8) % 4));
                if (polled := (pool.pollNext(wid) catch unreachable)) {
                    const tid: usize = @as(usize, polled.item.task_id);
                    // Zero duplicate execution invariant!
                    assertEq(executed_flags[tid], @as(u8, 0));
                    executed_flags[tid] = 1;
                    pool.recordCompleted(wid, 20, 40 + @as(u64, step)) catch unreachable;
                }
            }
            step += 1;
        }

        if (do_abort) {
            * = pool.abortAll();
        } else {
            pool.shutdownDrain();
            let guard: u32 = 0;
            while (pool.pendingCount() > 0 && guard < 200) {
                let wid: u32 = 0;
                while (wid < 4) {
                    if (polled := (pool.pollNext(wid) catch unreachable)) {
                        const tid: usize = @as(usize, polled.item.task_id);
                        assertEq(executed_flags[tid], @as(u8, 0));
                        executed_flags[tid] = 1;
                        pool.recordCompleted(wid, 15, 30) catch unreachable;
                    }
                    wid += 1;
                }
                guard += 1;
            }
        }

        const st = pool.stats(scratch[0..]);
        // Exact task conservation invariant: submitted == completed + cancelled
        assertEq(st.total_submitted, st.total_completed + st.total_cancelled);
        assertEq(st.pending_count, @as(usize, 0));
        assertEq(st.in_flight_count, @as(u64, 0));
        total_tasks_processed += st.total_submitted;
        iter += 1;
    }

    assert(total_tasks_processed > 5000);
});
