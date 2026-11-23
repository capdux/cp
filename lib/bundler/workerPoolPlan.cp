// Source: Absorbed from /data/dev/bun/src/bundler/ThreadPool.cp | Synced: 2026-11-22T04:25:56Z
// ============================================================================
// lib/bundler/workerPoolPlan.cp — Work-Stealing Build Worker Pool & Load Planner
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven parallel bundler worker pool engine
// for `cap build`:
//   1. Adaptive worker count & I/O pool planner (`planWorkerCount`) balancing
//      host CPU cores, module count, memory ceiling, and platform I/O contention
//      (mirroring Bun's macOS/Windows bounded IOThreadPool heuristic);
//   2. External-slice Chase-Lev style work-stealing deque (`BuildWorkerDeque`)
//      with local LIFO `pushLocal`/`popLocal` for L1/L2 cache locality and
//      cross-core FIFO `stealOne`/`stealHalf` load balancing;
//   3. Multi-core work-stealing scheduler (`BuildWorkPool`) with high-priority
//      fast-lane queue, batch submission, backpressure admission control,
//      graceful `shutdownDrain`, emergency `abortAll`, conservation accounting
//      (`submitted == completed + cancelled`), and P95 tail-latency telemetry;
//   4. Human-readable telemetry & per-worker utilization reporter
//      (`formatBuildWorkPoolReport`).

import { Simd } from "cap:simd";

/// Minimum recommended memory budget per compiler worker in megabytes.
pub const BUILD_WORKER_MIN_MEM_PER_WORKER_MB: u64 = 64;

/// Default stack size per worker thread in kilobytes (matches Bun's 512 KiB IO thread stack).
pub const BUILD_WORKER_DEFAULT_STACK_KB: u32 = 512;

/// Maximum I/O pool threads on platforms with file-open kernel lock contention (macOS/Windows).
pub const BUILD_WORKER_MAX_IO_THREADS: u32 = 4;

/// Sentinel worker ID indicating no worker or no steal victim.
pub const BUILD_WORKER_INVALID_ID: u32 = 0xFFFFFFFF;

/// Error set for worker pool planning, deque operations, and scheduling.
pub err BuildWorkerPoolError {
    InvalidWorkerId,
    PoolClosed,
    QueueFull,
    BackpressureRejected,
    SliceCapacityExceeded,
    BufferTooSmall,
    InvalidConfig,
}

// ============================================================================
// Internal Zero-Alloc Formatting Helpers
// ============================================================================

fn wpAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn wpAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn wpAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        wpAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        wpAppendByte(out, pos, tmp[n])?;
    }
}

// ============================================================================
// Section 1: Adaptive Worker Count & I/O Pool Planner
// ============================================================================

/// Host operating system family affecting I/O thread pool heuristics.
pub enum WorkerPlatformKind: u8 {
    Linux = 0,
    MacOS = 1,
    Windows = 2,
}

/// Hardware and workload constraints provided to `planWorkerCount`.
pub struct WorkerPlanInput {
    cpu_cores: u32 = 4,
    module_count: usize = 1,
    memory_limit_mb: u64 = 0,
    mem_per_worker_mb: u64 = BUILD_WORKER_MIN_MEM_PER_WORKER_MB,
    platform: WorkerPlatformKind = WorkerPlatformKind.Linux,
    force_io_pool: bool = false,
    disable_io_pool: bool = false,
}

/// Recommended worker pool topology produced by `planWorkerCount`.
pub struct WorkerPlanResult {
    worker_count: u32 = 1,
    use_io_pool: bool = false,
    io_thread_count: u32 = 0,
    warm_worker_count: u32 = 1,
    recommended_queue_capacity: usize = 16,
}

/// Computes optimal bundler worker count, warm-up count, per-worker deque capacity,
/// and dedicated I/O pool sizing based on host CPU cores, module count, memory ceiling,
/// and OS file-open contention characteristics.
pub fn planWorkerCount(input: WorkerPlanInput): WorkerPlanResult {
    const cores: u32 = if (input.cpu_cores == 0) 1 else input.cpu_cores;
    let workers: u32 = cores;

    // Cap workers by memory ceiling if specified.
    if (input.memory_limit_mb > 0) {
        const per_w = if (input.mem_per_worker_mb == 0) BUILD_WORKER_MIN_MEM_PER_WORKER_MB else input.mem_per_worker_mb;
        const mem_cap_u64 = input.memory_limit_mb / per_w;
        const mem_cap: u32 = if (mem_cap_u64 == 0) 1 else if (mem_cap_u64 > 0xFFFFFFFF) 0xFFFFFFFF else @as(u32, @intCast(mem_cap_u64));
        if (mem_cap < workers) {
            workers = mem_cap;
        }
    }

    // Do not spawn more workers than modules when module_count is small.
    if (input.module_count == 0) {
        workers = 1;
    } else if (input.module_count < @as(usize, workers)) {
        workers = @as(u32, @intCast(input.module_count));
    }
    if (workers == 0) workers = 1;

    // Dedicated I/O pool heuristic (mirrors Bun ThreadPool.usesIOPool):
    // macOS & Windows suffer kernel IORWLock contention beyond 4 file-open threads.
    let use_io = false;
    if (input.force_io_pool) {
        use_io = true;
    } else if (input.disable_io_pool) {
        use_io = false;
    } else if ((input.platform == WorkerPlatformKind.MacOS || input.platform == WorkerPlatformKind.Windows) && cores > 3) {
        use_io = true;
    }

    let io_threads: u32 = 0;
    if (use_io) {
        const capped = if (cores > BUILD_WORKER_MAX_IO_THREADS) BUILD_WORKER_MAX_IO_THREADS else cores;
        io_threads = if (capped < 2) 2 else capped;
    }

    // Warm up at most 8 workers eagerly (mirrors Bun ThreadPool.start(): warm(8)).
    const warm: u32 = if (workers > 8) @as(u32, 8) else workers;

    // Recommend power-of-two friendly deque capacity per worker based on workload density.
    const per_worker_mods = if (input.module_count == 0) 1 else (input.module_count + @as(usize, workers) - 1) / @as(usize, workers);
    let q_cap: usize = 16;
    while (q_cap < per_worker_mods * 2 && q_cap < 4096) {
        q_cap = q_cap * 2;
    }

    return WorkerPlanResult{
        worker_count: workers,
        use_io_pool: use_io,
        io_thread_count: io_threads,
        warm_worker_count: warm,
        recommended_queue_capacity: q_cap,
    };
}

// ============================================================================
// Section 2: External-Slice Work-Stealing Deque (`BuildWorkerDeque`)
// ============================================================================

/// Scheduling priority tier for a build work item.
pub enum BuildWorkPriority: u8 {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3,
}

/// Unit of compilation/transpilation/linking work scheduled in `BuildWorkPool`.
pub struct BuildWorkItem {
    task_id: u32 = 0,
    module_index: u32 = 0,
    priority: BuildWorkPriority = BuildWorkPriority.Normal,
    estimated_cost_us: u32 = 0,
    submitted_at_us: u64 = 0,

    static fn empty(): BuildWorkItem {
        return BuildWorkItem{};
    }
}

/// Caller-slice-backed circular work-stealing deque supporting:
///   - Local worker LIFO `pushLocal` / `popLocal` (cache-hot recent tasks);
///   - Peer worker FIFO `stealOne` / `stealHalf` (oldest coarse-grained tasks).
pub struct BuildWorkerDeque {
    slots: []BuildWorkItem,
    head: usize = 0,
    tail: usize = 0,
    count: usize = 0,

    static fn init(slots_buf: []BuildWorkItem): BuildWorkerDeque {
        return BuildWorkerDeque{
            slots: slots_buf,
            head: 0,
            tail: 0,
            count: 0,
        };
    }

    /// Returns the number of queued items.
    fn len(): usize {
        return _.count;
    }

    /// Returns the backing slice capacity.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Returns true if no items are queued.
    fn isEmpty(): bool {
        return _.count == 0;
    }

    /// Returns true if the deque cannot accept additional items.
    fn isFull(): bool {
        return _.count >= _.slots.len;
    }

    /// Pushes `item` onto the local (bottom/tail) end for LIFO execution.
    fn pushLocal(*_, item: BuildWorkItem)! {
        if (_.slots.len == 0 || _.count >= _.slots.len) {
            return err.QueueFull;
        }
        _.slots[_.tail] = item;
        _.tail = (_.tail + 1) % _.slots.len;
        _.count += 1;
    }

    /// Pops the most recently pushed item from the local (bottom/tail) end (LIFO).
    fn popLocal(*_): BuildWorkItem? {
        if (_.count == 0 || _.slots.len == 0) return null;
        _.tail = if (_.tail == 0) _.slots.len - 1 else _.tail - 1;
        const item = _.slots[_.tail];
        _.count -= 1;
        return item;
    }

    /// Steals a single oldest item from the remote (top/head) end (FIFO).
    fn stealOne(*_): BuildWorkItem? {
        if (_.count == 0 || _.slots.len == 0) return null;
        const item = _.slots[_.head];
        _.head = (_.head + 1) % _.slots.len;
        _.count -= 1;
        return item;
    }

    /// Batch-steals up to `min(ceil(count / 2), out_buf.len)` oldest items from
    /// the remote (top/head) end in FIFO order to amortize cross-core contention.
    fn stealHalf(*_, out_buf: []BuildWorkItem): usize {
        if (_.count == 0 || out_buf.len == 0 || _.slots.len == 0) return 0;
        let to_steal = (_.count + 1) / 2;
        if (to_steal > out_buf.len) to_steal = out_buf.len;

        let i: usize = 0;
        while (i < to_steal) {
            out_buf[i] = _.slots[_.head];
            _.head = (_.head + 1) % _.slots.len;
            _.count -= 1;
            i += 1;
        }
        return to_steal;
    }

    /// Empties all queued items and returns how many were discarded.
    fn clear(*_): usize {
        const dropped = _.count;
        _.head = 0;
        _.tail = 0;
        _.count = 0;
        return dropped;
    }
}

// ============================================================================
// Section 3: Multi-Core Work-Stealing Pool, Backpressure & Telemetry
// ============================================================================

/// Lifecycle state of `BuildWorkPool`.
pub enum BuildWorkPoolState: u8 {
    Running = 0,
    Draining = 1,
    Aborted = 2,
}

/// Per-worker deque and telemetry counters inside `BuildWorkPool`.
pub struct BuildWorkerSlot {
    worker_id: u32 = 0,
    deque: BuildWorkerDeque,
    tasks_completed: u64 = 0,
    tasks_stolen_in: u64 = 0,
    tasks_stolen_out: u64 = 0,
    local_pops: u64 = 0,
    busy_time_us: u64 = 0,
    idle_polls: u64 = 0,

    static fn init(worker_id: u32, deque_slots: []BuildWorkItem): BuildWorkerSlot {
        return BuildWorkerSlot{
            worker_id: worker_id,
            deque: BuildWorkerDeque.init(deque_slots),
        };
    }
}

/// Origin of a task returned by `BuildWorkPool.pollNext`.
pub enum BuildWorkPollSource: u8 {
    LocalDeque = 0,
    PriorityQueue = 1,
    StolenFromPeer = 2,
}

/// Result of a worker polling `BuildWorkPool.pollNext`.
pub struct BuildWorkPollResult {
    item: BuildWorkItem,
    source: BuildWorkPollSource = BuildWorkPollSource.LocalDeque,
    victim_worker_id: u32 = BUILD_WORKER_INVALID_ID,
}

/// Aggregate pool telemetry including conservation counters, steal rate, and P95 latency.
pub struct BuildWorkPoolStats {
    worker_count: usize = 0,
    state: BuildWorkPoolState = BuildWorkPoolState.Running,
    total_submitted: u64 = 0,
    total_completed: u64 = 0,
    total_cancelled: u64 = 0,
    total_rejected_backpressure: u64 = 0,
    total_local_pops: u64 = 0,
    total_priority_pops: u64 = 0,
    total_steals: u64 = 0,
    steal_rate_permille: u32 = 0,
    avg_utilization_permille: u32 = 0,
    p95_latency_us: u64 = 0,
    pending_count: usize = 0,
    in_flight_count: u64 = 0,
}

/// Zero-heap, caller-slice-backed multi-core work-stealing pool with:
///   - High/Critical priority fast-lane queue;
///   - Per-worker LIFO local deques + cross-worker FIFO `stealHalf` balancing;
///   - Global backpressure admission cap;
///   - Graceful `shutdownDrain` and immediate `abortAll` with task conservation;
///   - Ring-buffered P95 tail-latency & per-worker utilization telemetry.
pub struct BuildWorkPool {
    workers: []BuildWorkerSlot,
    priority_queue: BuildWorkerDeque,
    latency_samples: []u64,
    latency_write_idx: usize = 0,
    latency_sample_count: usize = 0,
    max_pending_backpressure: usize = 0,
    state: BuildWorkPoolState = BuildWorkPoolState.Running,
    total_submitted: u64 = 0,
    total_completed: u64 = 0,
    total_cancelled: u64 = 0,
    total_rejected_backpressure: u64 = 0,
    total_priority_pops: u64 = 0,
    in_flight_count: u64 = 0,
    rr_cursor: usize = 0,

    static fn init(
        workers_buf: []BuildWorkerSlot,
        priority_buf: []BuildWorkItem,
        latency_buf: []u64,
        max_pending_backpressure: usize,
    ): BuildWorkPool {
        return BuildWorkPool{
            workers: workers_buf,
            priority_queue: BuildWorkerDeque.init(priority_buf),
            latency_samples: latency_buf,
            max_pending_backpressure: max_pending_backpressure,
        };
    }

    /// Total queued items across the priority fast-lane and all worker deques.
    fn pendingCount(): usize {
        let total = _.priority_queue.len();
        let i: usize = 0;
        while (i < _.workers.len) {
            total += _.workers[i].deque.len();
            i += 1;
        }
        return total;
    }

    /// Finds the worker index with the fewest queued items (and available capacity).
    fn findLightestWorker(): usize? {
        let best_idx: usize = 0;
        let found = false;
        let i: usize = 0;
        while (i < _.workers.len) {
            const idx = (_.rr_cursor + i) % _.workers.len;
            if (!_.workers[idx].deque.isFull()) {
                if (!found || _.workers[idx].deque.len() < _.workers[best_idx].deque.len()) {
                    best_idx = idx;
                    found = true;
                }
            }
            i += 1;
        }
        if (found) return best_idx;
        return null;
    }

    /// Submits a single work item with backpressure and priority routing.
    /// `High`/`Critical` tasks enter the global priority fast-lane when space permits;
    /// `Normal`/`Low` tasks target `preferred_worker` (or the lightest non-full worker).
    fn submit(*_, item: BuildWorkItem, preferred_worker: u32)! {
        if (_.state != BuildWorkPoolState.Running) return err.PoolClosed;
        if (_.workers.len == 0) return err.InvalidConfig;

        const current_pending = _.pendingCount();
        if (_.max_pending_backpressure > 0 && current_pending >= _.max_pending_backpressure) {
            _.total_rejected_backpressure += 1;
            return err.BackpressureRejected;
        }

        const is_high = item.priority == BuildWorkPriority.High || item.priority == BuildWorkPriority.Critical;
        if (is_high && !_.priority_queue.isFull()) {
            _.priority_queue.pushLocal(item)?;
            _.total_submitted += 1;
            return;
        }

        // Try preferred worker first if valid and not full
        if (preferred_worker != BUILD_WORKER_INVALID_ID) {
            const pref_idx: usize = @as(usize, preferred_worker);
            if (pref_idx < _.workers.len && !_.workers[pref_idx].deque.isFull()) {
                _.workers[pref_idx].deque.pushLocal(item)?;
                _.total_submitted += 1;
                return;
            }
        }

        // Fallback to lightest non-full worker
        if (light := _.findLightestWorker()) {
            _.workers[light].deque.pushLocal(item)?;
            _.rr_cursor = (light + 1) % _.workers.len;
            _.total_submitted += 1;
            return;
        }

        return err.QueueFull;
    }

    /// Submits a batch of items, returning the number accepted before any backpressure
    /// or full-queue limit is encountered. Returns `err.PoolClosed` if pool is not running.
    fn submitBatch(*_, items: []const BuildWorkItem, start_worker: u32): BuildWorkerPoolError!usize {
        if (_.state != BuildWorkPoolState.Running) return err.PoolClosed;
        if (_.workers.len == 0) return err.InvalidConfig;

        let accepted: usize = 0;
        let i: usize = 0;
        while (i < items.len) {
            const pref: u32 = if (start_worker == BUILD_WORKER_INVALID_ID)
                @as(u32, @intCast((_.rr_cursor + i) % _.workers.len))
            else
                @as(u32, @intCast((@as(usize, start_worker) + i) % _.workers.len));

            let ok = true;
            _.submit(items[i], pref) catch {
                ok = false;
            };
            if (!ok) break;
            accepted += 1;
            i += 1;
        }
        return accepted;
    }

    /// Polls the next task for `worker_id`:
    ///   1. Global high-priority fast-lane (FIFO);
    ///   2. Local worker deque (`popLocal`, LIFO for cache locality);
    ///   3. Work-stealing from the most loaded peer (`stealHalf`, FIFO), keeping one
    ///      to run immediately and pushing stolen siblings onto the local deque.
    fn pollNext(*_, worker_id: u32): BuildWorkerPoolError!BuildWorkPollResult? {
        const widx: usize = @as(usize, worker_id);
        if (widx >= _.workers.len) return err.InvalidWorkerId;
        if (_.state == BuildWorkPoolState.Aborted) return null;

        // 1. High-priority fast lane
        if (prio_item := _.priority_queue.stealOne()) {
            _.total_priority_pops += 1;
            _.in_flight_count += 1;
            return BuildWorkPollResult{
                item: prio_item,
                source: BuildWorkPollSource.PriorityQueue,
                victim_worker_id: BUILD_WORKER_INVALID_ID,
            };
        }

        // 2. Local deque (LIFO)
        if (local_item := _.workers[widx].deque.popLocal()) {
            _.workers[widx].local_pops += 1;
            _.in_flight_count += 1;
            return BuildWorkPollResult{
                item: local_item,
                source: BuildWorkPollSource.LocalDeque,
                victim_worker_id: BUILD_WORKER_INVALID_ID,
            };
        }

        // 3. Work-stealing from the most loaded peer worker
        let victim_idx: usize = widx;
        let max_peer_len: usize = 0;
        let p: usize = 0;
        while (p < _.workers.len) {
            if (p != widx) {
                const plen = _.workers[p].deque.len();
                if (plen > max_peer_len) {
                    max_peer_len = plen;
                    victim_idx = p;
                }
            }
            p += 1;
        }

        if (max_peer_len > 0) {
            // Steal half (bounded by local deque free space + 1 for the returned item).
            let steal_buf: [32]BuildWorkItem = [BuildWorkItem.empty(); 32];
            const local_free = _.workers[widx].deque.capacity() - _.workers[widx].deque.len();
            let max_take = local_free + 1;
            if (max_take > steal_buf.len) max_take = steal_buf.len;

            const stolen = _.workers[victim_idx].deque.stealHalf(steal_buf[0..max_take]);
            if (stolen > 0) {
                _.workers[widx].tasks_stolen_in += @as(u64, stolen);
                _.workers[victim_idx].tasks_stolen_out += @as(u64, stolen);

                // Push remaining stolen items into local deque
                let k: usize = 1;
                while (k < stolen) {
                    _.workers[widx].deque.pushLocal(steal_buf[k]) catch break;
                    k += 1;
                }

                _.in_flight_count += 1;
                return BuildWorkPollResult{
                    item: steal_buf[0],
                    source: BuildWorkPollSource.StolenFromPeer,
                    victim_worker_id: _.workers[victim_idx].worker_id,
                };
            }
        }

        _.workers[widx].idle_polls += 1;
        return null;
    }

    /// Records task completion, updates worker busy time, and appends `latency_us`
    /// into the caller-provided ring buffer for P95 calculation.
    fn recordCompleted(*_, worker_id: u32, duration_us: u64, latency_us: u64)! {
        const widx: usize = @as(usize, worker_id);
        if (widx >= _.workers.len) return err.InvalidWorkerId;

        _.workers[widx].tasks_completed += 1;
        _.workers[widx].busy_time_us += duration_us;
        _.total_completed += 1;
        if (_.in_flight_count > 0) {
            _.in_flight_count -= 1;
        }

        if (_.latency_samples.len > 0) {
            _.latency_samples[_.latency_write_idx] = latency_us;
            _.latency_write_idx = (_.latency_write_idx + 1) % _.latency_samples.len;
            if (_.latency_sample_count < _.latency_samples.len) {
                _.latency_sample_count += 1;
            }
        }
    }

    /// Initiates graceful shutdown: stops accepting new submissions while allowing
    /// workers to drain all already-queued tasks.
    fn shutdownDrain(*_) {
        if (_.state == BuildWorkPoolState.Running) {
            _.state = BuildWorkPoolState.Draining;
        }
    }

    /// Immediately aborts the pool and cancels all queued tasks (plus any in-flight
    /// tasks not yet completed) so `total_submitted == total_completed + total_cancelled`.
    fn abortAll(*_): u64 {
        _.state = BuildWorkPoolState.Aborted;
        let cancelled: u64 = @as(u64, _.priority_queue.clear());
        let i: usize = 0;
        while (i < _.workers.len) {
            cancelled += @as(u64, _.workers[i].deque.clear());
            i += 1;
        }
        cancelled += _.in_flight_count;
        _.in_flight_count = 0;
        _.total_cancelled += cancelled;
        return cancelled;
    }

    /// Computes P95 tail latency in microseconds from recorded samples using
    /// caller-provided `scratch` slice (zero heap allocation).
    fn computeP95LatencyUs(scratch: []u64): u64 {
        const n = if (_.latency_sample_count <= scratch.len) _.latency_sample_count else scratch.len;
        if (n == 0) return 0;

        let i: usize = 0;
        while (i < n) {
            scratch[i] = _.latency_samples[i];
            i += 1;
        }

        // Insertion sort ascending
        let s: usize = 1;
        while (s < n) {
            const key = scratch[s];
            let j = s;
            while (j > 0 && scratch[j - 1] > key) {
                scratch[j] = scratch[j - 1];
                j -= 1;
            }
            scratch[j] = key;
            s += 1;
        }

        const p95_idx = ((n - 1) * 95) / 100;
        return scratch[p95_idx];
    }

    /// Computes aggregate pool statistics, steal rate, utilization, and P95 latency.
    fn stats(latency_scratch: []u64): BuildWorkPoolStats {
        let local_pops: u64 = 0;
        let steals: u64 = 0;
        let util_sum: u64 = 0;

        // Use SIMD reduction over per-worker completed counts when workers <= 32.
        let completed_i32: [32]i32 = [0; 32];
        const simd_n = if (_.workers.len <= 32) _.workers.len else 32;

        let i: usize = 0;
        while (i < _.workers.len) {
            const w = _.workers[i];
            local_pops += w.local_pops;
            steals += w.tasks_stolen_in;
            if (i < simd_n) {
                completed_i32[i] = @as(i32, @intCast(if (w.tasks_completed > 0x7FFFFFFF) 0x7FFFFFFF else w.tasks_completed));
            }
            const polls = w.tasks_completed + w.idle_polls;
            if (polls > 0) {
                util_sum += (w.tasks_completed * 1000) / polls;
            }
            i += 1;
        }

        * = Simd.reduceAddI32(completed_i32[0..simd_n]);

        const total_pops = local_pops + _.total_priority_pops + steals;
        const steal_rate: u32 = if (total_pops > 0)
            @as(u32, @intCast((steals * 1000) / total_pops))
        else
            0;

        const avg_util: u32 = if (_.workers.len > 0)
            @as(u32, @intCast(util_sum / @as(u64, _.workers.len)))
        else
            0;

        return BuildWorkPoolStats{
            worker_count: _.workers.len,
            state: _.state,
            total_submitted: _.total_submitted,
            total_completed: _.total_completed,
            total_cancelled: _.total_cancelled,
            total_rejected_backpressure: _.total_rejected_backpressure,
            total_local_pops: local_pops,
            total_priority_pops: _.total_priority_pops,
            total_steals: steals,
            steal_rate_permille: steal_rate,
            avg_utilization_permille: avg_util,
            p95_latency_us: _.computeP95LatencyUs(latency_scratch),
            pending_count: _.pendingCount(),
            in_flight_count: _.in_flight_count,
        };
    }
}

// ============================================================================
// Section 4: Telemetry & Per-Worker Utilization Reporter
// ============================================================================

fn wpStateName(state: BuildWorkPoolState): []const u8 {
    if (state == BuildWorkPoolState.Running) return "Running";
    if (state == BuildWorkPoolState.Draining) return "Draining";
    return "Aborted";
}

/// Formats a structured summary report of pool throughput, work-stealing rate,
/// P95 tail latency, and per-worker counters into `out_buf`.
pub fn formatBuildWorkPoolReport(
    stats: BuildWorkPoolStats,
    workers: []const BuildWorkerSlot,
    out_buf: []u8,
): BuildWorkerPoolError!usize {
    let pos: usize = 0;
    wpAppendBytes(out_buf, &pos, "BuildWorkPool [")?;
    wpAppendBytes(out_buf, &pos, wpStateName(stats.state))?;
    wpAppendBytes(out_buf, &pos, "] workers=")?;
    wpAppendU64(out_buf, &pos, @as(u64, stats.worker_count))?;
    wpAppendByte(out_buf, &pos, '\n')?;

    wpAppendBytes(out_buf, &pos, "  tasks: submitted=")?;
    wpAppendU64(out_buf, &pos, stats.total_submitted)?;
    wpAppendBytes(out_buf, &pos, " completed=")?;
    wpAppendU64(out_buf, &pos, stats.total_completed)?;
    wpAppendBytes(out_buf, &pos, " cancelled=")?;
    wpAppendU64(out_buf, &pos, stats.total_cancelled)?;
    wpAppendBytes(out_buf, &pos, " rejected=")?;
    wpAppendU64(out_buf, &pos, stats.total_rejected_backpressure)?;
    wpAppendByte(out_buf, &pos, '\n')?;

    wpAppendBytes(out_buf, &pos, "  steal_rate=")?;
    wpAppendU64(out_buf, &pos, @as(u64, stats.steal_rate_permille))?;
    wpAppendBytes(out_buf, &pos, "pm avg_util=")?;
    wpAppendU64(out_buf, &pos, @as(u64, stats.avg_utilization_permille))?;
    wpAppendBytes(out_buf, &pos, "pm p95_latency=")?;
    wpAppendU64(out_buf, &pos, stats.p95_latency_us)?;
    wpAppendBytes(out_buf, &pos, "us\n")?;

    let i: usize = 0;
    while (i < workers.len) {
        const w = workers[i];
        wpAppendBytes(out_buf, &pos, "  worker[")?;
        wpAppendU64(out_buf, &pos, @as(u64, w.worker_id))?;
        wpAppendBytes(out_buf, &pos, "]: done=")?;
        wpAppendU64(out_buf, &pos, w.tasks_completed)?;
        wpAppendBytes(out_buf, &pos, " local=")?;
        wpAppendU64(out_buf, &pos, w.local_pops)?;
        wpAppendBytes(out_buf, &pos, " stolen_in=")?;
        wpAppendU64(out_buf, &pos, w.tasks_stolen_in)?;
        wpAppendBytes(out_buf, &pos, " stolen_out=")?;
        wpAppendU64(out_buf, &pos, w.tasks_stolen_out)?;
        wpAppendByte(out_buf, &pos, '\n')?;
        i += 1;
    }

    return pos;
}

// ============================================================================
// Section 5: Bounded I/O Pool Refcount Lifecycle & SIMD Load-Skew Analyzer
// ============================================================================

/// State machine mirroring Bun's `IOThreadPool` ref-counted shared lifecycle
/// (`0 = stopped`, `1 = idle/shutdownable`, `>= 2 = active handles`).
pub struct IoPoolLifecycle {
    ref_count: u32 = 0,
    max_threads: u32 = 2,
    stack_size_kb: u32 = BUILD_WORKER_DEFAULT_STACK_KB,
    total_acquires: u64 = 0,
    total_shutdowns: u64 = 0,

    static fn init(cpu_cores: u32): IoPoolLifecycle {
        const capped = if (cpu_cores > BUILD_WORKER_MAX_IO_THREADS) BUILD_WORKER_MAX_IO_THREADS else cpu_cores;
        const threads = if (capped < 2) @as(u32, 2) else capped;
        return IoPoolLifecycle{
            ref_count: 0,
            max_threads: threads,
            stack_size_kb: BUILD_WORKER_DEFAULT_STACK_KB,
        };
    }

    /// Acquires a reference to the shared I/O pool, initializing it (`ref_count = 2`)
    /// on first acquire. Returns true if this call initialized the pool.
    fn acquire(*_): bool {
        _.total_acquires += 1;
        if (_.ref_count == 0) {
            _.ref_count = 2;
            return true;
        }
        _.ref_count += 1;
        return false;
    }

    /// Releases a previously acquired reference. Fails if no active holder remains.
    fn release(*_)! {
        if (_.ref_count <= 1) return err.InvalidConfig;
        _.ref_count -= 1;
    }

    /// Shuts down the I/O pool if and only if no active `BuildWorkPool` holds a
    /// reference (`ref_count == 1`). Returns true when shutdown succeeded.
    fn tryShutdown(*_): bool {
        if (_.ref_count != 1) return false;
        _.ref_count = 0;
        _.total_shutdowns += 1;
        return true;
    }

    /// Returns true if the I/O pool is currently initialized.
    fn isRunning(): bool {
        return _.ref_count > 0;
    }
}

/// Multi-core load balance summary computed with `cap:simd` reductions.
pub struct WorkerLoadBalanceSummary {
    worker_count: usize = 0,
    total_completed: i32 = 0,
    min_completed: i32 = 0,
    max_completed: i32 = 0,
    skew_permille: u32 = 0,
}

/// Evaluates cross-worker load distribution using `Simd.reduceAddI32`,
/// `Simd.reduceMinI32`, and `Simd.reduceMaxI32`.
/// `skew_permille` is `(max - min) * 1000 / max` (0 = perfectly balanced, 1000 = extreme skew).
pub fn evaluateWorkerLoadBalance(workers: []const BuildWorkerSlot): WorkerLoadBalanceSummary {
    if (workers.len == 0) return WorkerLoadBalanceSummary{};

    let counts: [64]i32 = [0; 64];
    const n = if (workers.len <= 64) workers.len else 64;
    let i: usize = 0;
    while (i < n) {
        const c = workers[i].tasks_completed;
        counts[i] = @as(i32, @intCast(if (c > 0x7FFFFFFF) 0x7FFFFFFF else c));
        i += 1;
    }

    const total = Simd.reduceAddI32(counts[0..n]);
    const min_c = Simd.reduceMinI32(counts[0..n]);
    const max_c = Simd.reduceMaxI32(counts[0..n]);
    const skew: u32 = if (max_c > 0)
        @as(u32, @intCast((@as(u64, @intCast(max_c - min_c)) * 1000) / @as(u64, @intCast(max_c))))
    else
        0;

    return WorkerLoadBalanceSummary{
        worker_count: n,
        total_completed: total,
        min_completed: min_c,
        max_completed: max_c,
        skew_permille: skew,
    };
}
