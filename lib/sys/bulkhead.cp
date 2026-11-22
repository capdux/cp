// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Tenant Bulkhead Isolation & AIMD Adaptive Concurrency Guard) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation bulkhead concurrency isolator & AIMD adaptive limiter:
//   - Fixed inline compartment table (`BULKHEAD_MAX_COMPARTMENTS = 32`), each with independent
//     concurrency limit, bounded FIFO ring waiter queue (`BULKHEAD_MAX_QUEUE_PER_COMP = 32`),
//     and wait timeout so one saturated tenant/dependency never starves others
//   - Full permit lifecycle: `tryAcquire` (granted / queued / rejected_full / unknown_compartment),
//     `release` (returns permit, runs optional AIMD latency/error controller, and FIFO-promotes
//     next non-expired waiter with `promoted_req_id`), and `reapExpiredWaiters`
//   - Zero-heap per-compartment snapshots & global aggregate telemetry

/// Maximum number of independent bulkhead compartments stored inline.
pub const BULKHEAD_MAX_COMPARTMENTS: usize = 32;

/// Maximum FIFO waiter queue capacity per compartment.
pub const BULKHEAD_MAX_QUEUE_PER_COMP: usize = 32;

// ============================================================================
// 1. Enums, Configuration & Snapshot Models (100% Zero-Heap)
// ============================================================================

/// Outcome classification returned by `BulkheadGuard.tryAcquire`.
pub enum BulkheadAcquireStatus: u8 {
    granted = 0,
    queued = 1,
    rejected_full = 2,
    unknown_compartment = 3,
    invalid_arg = 4,
}

/// Configuration for a single bulkhead compartment.
pub struct BulkheadConfig {
    /// Initial (or fixed) maximum concurrent in-flight permits.
    max_concurrent: u32 = 8;
    /// Maximum FIFO wait queue depth (`0..=BULKHEAD_MAX_QUEUE_PER_COMP`).
    max_queue: usize = 16;
    /// Maximum time a waiter may remain queued before expiring (`0` = no timeout).
    wait_timeout_ms: u64 = 5000;
    /// Enables AIMD adaptive concurrency adjustment on `release`.
    adaptive: bool = false;
    /// Lower concurrency floor when `adaptive == true`.
    min_concurrent: u32 = 2;
    /// Upper concurrency ceiling when `adaptive == true`.
    hard_max_concurrent: u32 = 32;
    /// Healthy latency threshold (ms); latency > target triggers multiplicative backoff.
    target_latency_ms: u64 = 200;
    /// Multiplicative backoff factor in permille (`1..=999`, e.g. `750` = 0.75x).
    backoff_permille: u32 = 750;
    /// Number of consecutive healthy completions required to increment `current_limit` by 1.
    healthy_streak_for_inc: u32 = 4;

    /// Creates a fixed-limit (non-adaptive) bulkhead configuration.
    static fn fixed(max_concurrent: u32, max_queue: usize, wait_timeout_ms: u64): BulkheadConfig {
        return BulkheadConfig{
            max_concurrent: max_concurrent,
            max_queue: max_queue,
            wait_timeout_ms: wait_timeout_ms,
            adaptive: false,
            min_concurrent: max_concurrent,
            hard_max_concurrent: max_concurrent,
        };
    }

    /// Creates an AIMD adaptive bulkhead configuration.
    static fn adaptiveDefaults(
        initial_max: u32,
        min_concurrent: u32,
        hard_max_concurrent: u32,
        target_latency_ms: u64,
    ): BulkheadConfig {
        return BulkheadConfig{
            max_concurrent: initial_max,
            max_queue: 16,
            wait_timeout_ms: 5000,
            adaptive: true,
            min_concurrent: min_concurrent,
            hard_max_concurrent: hard_max_concurrent,
            target_latency_ms: target_latency_ms,
            backoff_permille: 750,
            healthy_streak_for_inc: 4,
        };
    }

    /// Validates all concurrency, queue, and AIMD bounds.
    fn isValid(): bool {
        if (_.max_concurrent == 0 || _.max_queue > BULKHEAD_MAX_QUEUE_PER_COMP) {
            return false;
        }
        if (_.adaptive) {
            if (_.min_concurrent == 0
                || _.min_concurrent > _.max_concurrent
                || _.max_concurrent > _.hard_max_concurrent
                || _.backoff_permille == 0
                || _.backoff_permille >= 1000
                || _.healthy_streak_for_inc == 0)
            {
                return false;
            }
        }
        return true;
    }
}

/// Result returned by `BulkheadGuard.tryAcquire`.
pub struct BulkheadAcquireResult {
    status: BulkheadAcquireStatus = BulkheadAcquireStatus.unknown_compartment;
    /// 1-based queue position when `status == queued`, otherwise `0`.
    queue_position: usize = 0;
    in_flight: u32 = 0;
    current_limit: u32 = 0;
}

/// Result returned by `BulkheadGuard.release`.
pub struct BulkheadReleaseResult {
    released: bool = false;
    /// Non-zero `req_id` of the FIFO waiter automatically promoted to in-flight, or `0` if none.
    promoted_req_id: u64 = 0;
    /// Number of expired waiters skipped and reaped during promotion.
    expired_skipped: usize = 0;
    in_flight: u32 = 0;
    current_limit: u32 = 0;
}

/// Per-compartment zero-heap telemetry snapshot.
pub struct BulkheadCompartmentSnapshot {
    compartment_id: u32 = 0;
    in_flight: u32 = 0;
    queued: usize = 0;
    current_limit: u32 = 0;
    granted: u64 = 0;
    rejected: u64 = 0;
    timed_out: u64 = 0;
    promoted: u64 = 0;
    aimd_increases: u64 = 0;
    aimd_decreases: u64 = 0;
}

/// Global aggregate zero-heap statistics across all active compartments.
pub struct BulkheadGlobalStats {
    capacity: usize = BULKHEAD_MAX_COMPARTMENTS;
    active_compartments: usize = 0;
    total_in_flight: u64 = 0;
    total_queued: u64 = 0;
    total_granted: u64 = 0;
    total_rejected: u64 = 0;
    total_timed_out: u64 = 0;
    total_promoted: u64 = 0;
}

// ============================================================================
// 2. Internal Compartment Storage
// ============================================================================

struct BulkheadWaiter {
    req_id: u64 = 0;
    enqueued_ms: i64 = 0;
}

struct BulkheadCompartment {
    occupied: bool = false;
    compartment_id: u32 = 0;
    cfg: BulkheadConfig = BulkheadConfig{},
    current_limit: u32 = 0;
    in_flight: u32 = 0;
    healthy_streak: u32 = 0;

    // Fixed FIFO ring buffer for queued waiters.
    queue: [BULKHEAD_MAX_QUEUE_PER_COMP]BulkheadWaiter = [BulkheadWaiter{}; BULKHEAD_MAX_QUEUE_PER_COMP];
    q_head: usize = 0;
    q_len: usize = 0;

    // Telemetry counters.
    granted: u64 = 0;
    rejected: u64 = 0;
    timed_out: u64 = 0;
    promoted: u64 = 0;
    aimd_increases: u64 = 0;
    aimd_decreases: u64 = 0;
}

fn isWaiterExpired(enqueued_ms: i64, wait_timeout_ms: u64, now_ms: i64): bool {
    if (wait_timeout_ms == 0 || now_ms <= enqueued_ms) {
        return false;
    }
    const elapsed: u64 = @as(u64, @intCast(now_ms - enqueued_ms));
    return elapsed >= wait_timeout_ms;
}

// ============================================================================
// 3. BulkheadGuard Core Engine
// ============================================================================

/// Zero-heap multi-tenant bulkhead concurrency isolator with optional AIMD adaptive concurrency control.
pub struct BulkheadGuard {
    compartments: [BULKHEAD_MAX_COMPARTMENTS]BulkheadCompartment = [BulkheadCompartment{}; BULKHEAD_MAX_COMPARTMENTS];
    active_count: usize = 0;

    /// Initializes an empty `BulkheadGuard`.
    static fn init(): BulkheadGuard {
        return BulkheadGuard{};
    }

    /// Removes all compartments and resets all telemetry counters.
    fn reset(*_) {
        let idx: usize = 0;
        while (idx < BULKHEAD_MAX_COMPARTMENTS) {
            _.compartments[idx] = BulkheadCompartment{};
            idx += 1;
        }
        _.active_count = 0;
    }

    fn findCompIdx(comp_id: u32): usize? {
        if (comp_id == 0) {
            return null;
        }
        let idx: usize = 0;
        while (idx < BULKHEAD_MAX_COMPARTMENTS) {
            if (_.compartments[idx].occupied && _.compartments[idx].compartment_id == comp_id) {
                return idx;
            }
            idx += 1;
        }
        return null;
    }

    /// Registers a new isolated bulkhead compartment. Returns `false` if invalid, duplicate, or table full.
    fn registerCompartment(*_, comp_id: u32, cfg: BulkheadConfig): bool {
        if (comp_id == 0 || !cfg.isValid() || _.active_count >= BULKHEAD_MAX_COMPARTMENTS) {
            return false;
        }
        if (_ := _.findCompIdx(comp_id)) {
            return false;
        }

        let free_idx: usize = BULKHEAD_MAX_COMPARTMENTS;
        let idx: usize = 0;
        while (idx < BULKHEAD_MAX_COMPARTMENTS) {
            if (!_.compartments[idx].occupied) {
                free_idx = idx;
                break;
            }
            idx += 1;
        }
        if (free_idx >= BULKHEAD_MAX_COMPARTMENTS) {
            return false;
        }

        _.compartments[free_idx] = BulkheadCompartment{
            occupied: true,
            compartment_id: comp_id,
            cfg: cfg,
            current_limit: cfg.max_concurrent,
        };
        _.active_count += 1;
        return true;
    }

    /// Removes a registered compartment by `comp_id`.
    fn removeCompartment(*_, comp_id: u32): bool {
        const idx = _.findCompIdx(comp_id) ?? return false;
        _.compartments[idx] = BulkheadCompartment{};
        if (_.active_count > 0) {
            _.active_count -= 1;
        }
        return true;
    }

    fn reapExpiredAtSlot(*_, slot_idx: usize, now_ms: i64): usize {
        if (_.compartments[slot_idx].q_len == 0 || _.compartments[slot_idx].cfg.wait_timeout_ms == 0) {
            return 0;
        }
        const timeout_ms: u64 = _.compartments[slot_idx].cfg.wait_timeout_ms;
        let kept: [BULKHEAD_MAX_QUEUE_PER_COMP]BulkheadWaiter = [BulkheadWaiter{}; BULKHEAD_MAX_QUEUE_PER_COMP];
        let kept_n: usize = 0;
        let reaped: usize = 0;

        let step: usize = 0;
        while (step < _.compartments[slot_idx].q_len) {
            const pos: usize = (_.compartments[slot_idx].q_head + step) % BULKHEAD_MAX_QUEUE_PER_COMP;
            const w = _.compartments[slot_idx].queue[pos];
            if (isWaiterExpired(w.enqueued_ms, timeout_ms, now_ms)) {
                reaped += 1;
            } else {
                kept[kept_n] = w;
                kept_n += 1;
            }
            step += 1;
        }

        if (reaped > 0) {
            let k: usize = 0;
            while (k < kept_n) {
                _.compartments[slot_idx].queue[k] = kept[k];
                k += 1;
            }
            _.compartments[slot_idx].q_head = 0;
            _.compartments[slot_idx].q_len = kept_n;
            _.compartments[slot_idx].timed_out += @as(u64, reaped);
        }
        return reaped;
    }

    /// Sweeps expired waiters from compartment `comp_id` at `now_ms`. Returns number of timed-out waiters removed.
    fn reapExpiredWaiters(*_, comp_id: u32, now_ms: i64): usize {
        const idx = _.findCompIdx(comp_id) ?? return 0;
        return _.reapExpiredAtSlot(idx, now_ms);
    }

    /// Attempts to acquire an execution permit in compartment `comp_id` for `req_id` at `now_ms`.
    fn tryAcquire(*_, comp_id: u32, req_id: u64, now_ms: i64): BulkheadAcquireResult {
        if (req_id == 0) {
            return BulkheadAcquireResult{
                status: BulkheadAcquireStatus.invalid_arg,
            };
        }
        const idx = _.findCompIdx(comp_id) ?? return BulkheadAcquireResult{
            status: BulkheadAcquireStatus.unknown_compartment,
        };

        // Reap stale waiters before checking queue capacity so expired requests never block fresh traffic.
        * = _.reapExpiredAtSlot(idx, now_ms);

        if (_.compartments[idx].in_flight < _.compartments[idx].current_limit) {
            _.compartments[idx].in_flight += 1;
            _.compartments[idx].granted += 1;
            return BulkheadAcquireResult{
                status: BulkheadAcquireStatus.granted,
                queue_position: 0,
                in_flight: _.compartments[idx].in_flight,
                current_limit: _.compartments[idx].current_limit,
            };
        }

        if (_.compartments[idx].q_len < _.compartments[idx].cfg.max_queue) {
            const tail: usize = (_.compartments[idx].q_head + _.compartments[idx].q_len) % BULKHEAD_MAX_QUEUE_PER_COMP;
            _.compartments[idx].queue[tail] = BulkheadWaiter{
                req_id: req_id,
                enqueued_ms: now_ms,
            };
            _.compartments[idx].q_len += 1;
            return BulkheadAcquireResult{
                status: BulkheadAcquireStatus.queued,
                queue_position: _.compartments[idx].q_len,
                in_flight: _.compartments[idx].in_flight,
                current_limit: _.compartments[idx].current_limit,
            };
        }

        _.compartments[idx].rejected += 1;
        return BulkheadAcquireResult{
            status: BulkheadAcquireStatus.rejected_full,
            queue_position: 0,
            in_flight: _.compartments[idx].in_flight,
            current_limit: _.compartments[idx].current_limit,
        };
    }

    fn applyAimd(*_, slot_idx: usize, latency_ms: u64, failed: bool) {
        if (!_.compartments[slot_idx].cfg.adaptive) {
            return;
        }
        const cfg = _.compartments[slot_idx].cfg;
        const overloaded: bool = failed || (cfg.target_latency_ms > 0 && latency_ms > cfg.target_latency_ms);

        if (overloaded) {
            _.compartments[slot_idx].healthy_streak = 0;
            const cur: u32 = _.compartments[slot_idx].current_limit;
            if (cur > cfg.min_concurrent) {
                let next_lim: u32 = @as(u32, @intCast((@as(u64, cur) * @as(u64, cfg.backoff_permille)) / 1000));
                if (next_lim >= cur) {
                    next_lim = cur - 1;
                }
                if (next_lim < cfg.min_concurrent) {
                    next_lim = cfg.min_concurrent;
                }
                if (next_lim < cur) {
                    _.compartments[slot_idx].current_limit = next_lim;
                    _.compartments[slot_idx].aimd_decreases += 1;
                }
            }
            return;
        }

        _.compartments[slot_idx].healthy_streak += 1;
        if (_.compartments[slot_idx].healthy_streak >= cfg.healthy_streak_for_inc) {
            _.compartments[slot_idx].healthy_streak = 0;
            if (_.compartments[slot_idx].current_limit < cfg.hard_max_concurrent) {
                _.compartments[slot_idx].current_limit += 1;
                _.compartments[slot_idx].aimd_increases += 1;
            }
        }
    }

    /// Releases an in-flight permit, updates optional AIMD concurrency limit, and FIFO-promotes the next valid waiter.
    fn release(
        *_,
        comp_id: u32,
        req_id: u64,
        latency_ms: u64,
        failed: bool,
        now_ms: i64,
    ): BulkheadReleaseResult {
        * = req_id;
        const idx = _.findCompIdx(comp_id) ?? return BulkheadReleaseResult{};
        if (_.compartments[idx].in_flight == 0) {
            return BulkheadReleaseResult{
                released: false,
                in_flight: 0,
                current_limit: _.compartments[idx].current_limit,
            };
        }

        _.compartments[idx].in_flight -= 1;
        _.applyAimd(idx, latency_ms, failed);

        let promoted_id: u64 = 0;
        let expired_skipped: usize = 0;
        const timeout_ms: u64 = _.compartments[idx].cfg.wait_timeout_ms;

        // Promote the earliest non-expired waiter if capacity is now available.
        while (_.compartments[idx].q_len > 0 && _.compartments[idx].in_flight < _.compartments[idx].current_limit) {
            const head_pos: usize = _.compartments[idx].q_head;
            const waiter = _.compartments[idx].queue[head_pos];
            _.compartments[idx].q_head = (head_pos + 1) % BULKHEAD_MAX_QUEUE_PER_COMP;
            _.compartments[idx].q_len -= 1;

            if (isWaiterExpired(waiter.enqueued_ms, timeout_ms, now_ms)) {
                _.compartments[idx].timed_out += 1;
                expired_skipped += 1;
                continue;
            }

            _.compartments[idx].in_flight += 1;
            _.compartments[idx].granted += 1;
            _.compartments[idx].promoted += 1;
            promoted_id = waiter.req_id;
            break;
        }

        return BulkheadReleaseResult{
            released: true,
            promoted_req_id: promoted_id,
            expired_skipped: expired_skipped,
            in_flight: _.compartments[idx].in_flight,
            current_limit: _.compartments[idx].current_limit,
        };
    }

    /// Returns a zero-heap snapshot for compartment `comp_id`, or `null` if not registered.
    fn compartmentSnapshot(comp_id: u32): BulkheadCompartmentSnapshot? {
        const idx = _.findCompIdx(comp_id) ?? return null;
        const c = _.compartments[idx];
        return BulkheadCompartmentSnapshot{
            compartment_id: c.compartment_id,
            in_flight: c.in_flight,
            queued: c.q_len,
            current_limit: c.current_limit,
            granted: c.granted,
            rejected: c.rejected,
            timed_out: c.timed_out,
            promoted: c.promoted,
            aimd_increases: c.aimd_increases,
            aimd_decreases: c.aimd_decreases,
        };
    }

    /// Returns a zero-heap global aggregate statistics snapshot across all compartments.
    fn stats(): BulkheadGlobalStats {
        let st = BulkheadGlobalStats{
            capacity: BULKHEAD_MAX_COMPARTMENTS,
            active_compartments: _.active_count,
        };
        let idx: usize = 0;
        while (idx < BULKHEAD_MAX_COMPARTMENTS) {
            if (_.compartments[idx].occupied) {
                const c = _.compartments[idx];
                st.total_in_flight += @as(u64, c.in_flight);
                st.total_queued += @as(u64, c.q_len);
                st.total_granted += c.granted;
                st.total_rejected += c.rejected;
                st.total_timed_out += c.timed_out;
                st.total_promoted += c.promoted;
            }
            idx += 1;
        }
        return st;
    }
}
