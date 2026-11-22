// Origin: Pure-CP Stdlib (Zero-Alloc Sarathi/vLLM Chunked-Prefill & Preemptive Continuous Batch Scheduler) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation LLM continuous batch scheduler with chunked prefill & priority preemption:
//   - Fixed inline request table (`BATCH_SCHED_MAX_REQS = 64`) tracking prompt/prefilled/decoded tokens,
//     max_output_tokens, KV blocks held, priority, arrival_ms, ttft_deadline_ms, and lifecycle state
//   - Joint token-budget & KV-block-pool step scheduling (`scheduleStep(*_, now_ms, *out_plan)`):
//       Phase 1: Protects running decode sequences (1 token/step); if KV pool is exhausted, preempts
//                the lowest-priority latest-arriving running sequence and reclaims its KV blocks
//       Phase 2: Advances in-flight chunked prefill sequences within remaining token & KV budgets
//       Phase 3: Resumes preempted sequences first, then admits waiting requests prioritized by
//                TTFT deadline urgency + priority + arrival order, slicing long prompts by `chunk_size`
//   - Step feedback (`commitStepResult`) auto-releases KV blocks on EOS/max_output_tokens and tracks TTFT

/// Maximum number of concurrent LLM inference requests stored inline.
pub const BATCH_SCHED_MAX_REQS: usize = 64;

/// Maximum number of scheduled execution items emitted per step.
pub const BATCH_SCHED_MAX_STEP_ITEMS: usize = 64;

// ============================================================================
// 1. Enums, Configuration & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Lifecycle state of an inference request inside `ChunkedBatchScheduler`.
pub enum BatchReqState: u8 {
    waiting = 0,
    running = 1,
    preempted = 2,
    finished = 3,
    aborted = 4,
}

/// Execution phase scheduled for a request during a single iteration step.
pub enum BatchExecPhase: u8 {
    prefill_chunk = 0,
    decode = 1,
}

/// Configuration for `ChunkedBatchScheduler`.
pub struct BatchSchedConfig {
    /// Maximum total tokens (prefill + decode) scheduled in a single forward step.
    max_batch_tokens: u32 = 512;
    /// Maximum prefill tokens sliced per request per step (Sarathi chunked prefill).
    chunk_size: u32 = 128;
    /// Total number of KV cache blocks managed by the scheduler.
    total_kv_blocks: u32 = 256;
    /// Number of tokens stored per KV cache block (typically 16).
    kv_block_size: u32 = 16;
    /// Maximum number of simultaneously running sequences (`1..=64`).
    max_running_seqs: usize = 32;
    /// Waiting requests with `ttft_deadline_ms - now_ms <= ttft_urgency_window_ms` get deadline boost.
    ttft_urgency_window_ms: i64 = 500;

    /// Returns production-balanced default scheduler configuration.
    static fn defaults(): BatchSchedConfig {
        return BatchSchedConfig{};
    }

    /// Validates all scheduler capacity and budget invariants.
    fn isValid(): bool {
        return _.max_batch_tokens > 0
            && _.chunk_size > 0
            && _.total_kv_blocks > 0
            && _.kv_block_size > 0
            && _.max_running_seqs > 0
            && _.max_running_seqs <= BATCH_SCHED_MAX_REQS
            && _.ttft_urgency_window_ms >= 0;
    }
}

/// Inline request descriptor stored inside `ChunkedBatchScheduler`.
pub struct BatchRequestSlot {
    occupied: bool = false;
    req_id: u64 = 0;
    prompt_tokens: u32 = 0;
    prefilled_tokens: u32 = 0;
    decoded_tokens: u32 = 0;
    max_output_tokens: u32 = 0;
    kv_blocks_held: u32 = 0;
    priority: u16 = 0;
    arrival_ms: i64 = 0;
    ttft_deadline_ms: i64 = 0;
    first_token_ms: i64 = 0;
    preempt_count: u32 = 0;
    state: BatchReqState = BatchReqState.waiting;

    /// Returns `true` if all prompt tokens have been prefilled.
    fn isPrefillComplete(): bool {
        return _.prefilled_tokens >= _.prompt_tokens;
    }

    /// Returns total committed sequence length (`prefilled_tokens + decoded_tokens`).
    fn committedTokens(): u32 {
        return _.prefilled_tokens + _.decoded_tokens;
    }
}

/// Single scheduled work item in a `BatchStepPlan`.
pub struct BatchStepEntry {
    req_id: u64 = 0;
    phase: BatchExecPhase = BatchExecPhase.decode;
    tokens_scheduled: u32 = 0;
    kv_blocks_allocated: u32 = 0;
    is_resume_from_preempt: bool = false;
}

/// Complete zero-heap scheduling plan produced by a single `scheduleStep` call.
pub struct BatchStepPlan {
    items: [BATCH_SCHED_MAX_STEP_ITEMS]BatchStepEntry = [BatchStepEntry{}; BATCH_SCHED_MAX_STEP_ITEMS];
    item_count: usize = 0;
    decode_count: usize = 0;
    prefill_count: usize = 0;
    total_tokens_scheduled: u32 = 0;
    kv_blocks_allocated_step: u32 = 0;
    preempted_count: usize = 0;
    preempted_ids: [BATCH_SCHED_MAX_REQS]u64 = [0; BATCH_SCHED_MAX_REQS];
    free_kv_blocks_after: u32 = 0;

    /// Resets all plan counters and buffers to empty state.
    fn reset(*_) {
        _.item_count = 0;
        _.decode_count = 0;
        _.prefill_count = 0;
        _.total_tokens_scheduled = 0;
        _.kv_blocks_allocated_step = 0;
        _.preempted_count = 0;
        _.free_kv_blocks_after = 0;
    }
}

/// Cumulative zero-heap telemetry snapshot of `ChunkedBatchScheduler`.
pub struct BatchSchedStats {
    capacity: usize = BATCH_SCHED_MAX_REQS;
    active_slots: usize = 0;
    waiting_count: usize = 0;
    running_count: usize = 0;
    preempted_count: usize = 0;
    finished_count: usize = 0;
    aborted_count: usize = 0;
    used_kv_blocks: u32 = 0;
    free_kv_blocks: u32 = 0;
    total_steps: u64 = 0;
    total_prefill_tokens: u64 = 0;
    total_decode_tokens: u64 = 0;
    total_preemptions: u64 = 0;
    total_completed: u64 = 0;
    total_ttft_ms: u64 = 0;
    ttft_samples: u64 = 0;
    avg_ttft_ms: u64 = 0;
}

// ============================================================================
// 2. Internal Helpers
// ============================================================================

fn blocksForTokens(tokens: u32, block_size: u32): u32 {
    if (tokens == 0 || block_size == 0) {
        return 0;
    }
    return (tokens + block_size - 1) / block_size;
}

fn runningBetter(a: BatchRequestSlot, b: BatchRequestSlot): bool {
    if (a.priority != b.priority) {
        return a.priority > b.priority;
    }
    if (a.arrival_ms != b.arrival_ms) {
        return a.arrival_ms < b.arrival_ms;
    }
    return a.req_id < b.req_id;
}

fn isTtftUrgent(slot: BatchRequestSlot, now_ms: i64, urgency_window_ms: i64): bool {
    if (slot.ttft_deadline_ms <= 0) {
        return false;
    }
    return (slot.ttft_deadline_ms - now_ms) <= urgency_window_ms;
}

fn waitingBetter(a: BatchRequestSlot, b: BatchRequestSlot, now_ms: i64, urgency_window_ms: i64): bool {
    const a_urg: bool = isTtftUrgent(a, now_ms, urgency_window_ms);
    const b_urg: bool = isTtftUrgent(b, now_ms, urgency_window_ms);
    if (a_urg != b_urg) {
        return a_urg;
    }
    if (a_urg && b_urg && a.ttft_deadline_ms != b.ttft_deadline_ms) {
        return a.ttft_deadline_ms < b.ttft_deadline_ms;
    }
    if (a.priority != b.priority) {
        return a.priority > b.priority;
    }
    if (a.arrival_ms != b.arrival_ms) {
        return a.arrival_ms < b.arrival_ms;
    }
    return a.req_id < b.req_id;
}

fn victimWorse(cand: BatchRequestSlot, best: BatchRequestSlot): bool {
    // Lowest priority first, then latest arrival_ms, then largest req_id.
    if (cand.priority != best.priority) {
        return cand.priority < best.priority;
    }
    if (cand.arrival_ms != best.arrival_ms) {
        return cand.arrival_ms > best.arrival_ms;
    }
    return cand.req_id > best.req_id;
}

// ============================================================================
// 3. ChunkedBatchScheduler Core Engine
// ============================================================================

/// Zero-heap Sarathi/vLLM-style chunked-prefill & priority-preemptive LLM continuous batch scheduler.
pub struct ChunkedBatchScheduler {
    cfg: BatchSchedConfig = BatchSchedConfig.defaults();
    slots: [BATCH_SCHED_MAX_REQS]BatchRequestSlot = [BatchRequestSlot{}; BATCH_SCHED_MAX_REQS];
    used_kv_blocks: u32 = 0;
    total_steps: u64 = 0;
    total_prefill_tokens: u64 = 0;
    total_decode_tokens: u64 = 0;
    total_preemptions: u64 = 0;
    total_completed: u64 = 0;
    total_ttft_ms: u64 = 0;
    ttft_samples: u64 = 0;

    /// Initializes a `ChunkedBatchScheduler` with validated `cfg`, or `null` if invalid.
    static fn init(cfg: BatchSchedConfig): ChunkedBatchScheduler? {
        if (!cfg.isValid()) {
            return null;
        }
        return ChunkedBatchScheduler{
            cfg: cfg,
        };
    }

    /// Resets all request slots, KV pool usage, and cumulative telemetry counters.
    fn reset(*_) {
        let i: usize = 0;
        while (i < BATCH_SCHED_MAX_REQS) {
            _.slots[i] = BatchRequestSlot{};
            i += 1;
        }
        _.used_kv_blocks = 0;
        _.total_steps = 0;
        _.total_prefill_tokens = 0;
        _.total_decode_tokens = 0;
        _.total_preemptions = 0;
        _.total_completed = 0;
        _.total_ttft_ms = 0;
        _.ttft_samples = 0;
    }

    fn freeKvBlocks(): u32 {
        if (_.used_kv_blocks >= _.cfg.total_kv_blocks) {
            return 0;
        }
        return _.cfg.total_kv_blocks - _.used_kv_blocks;
    }

    fn findSlotById(req_id: u64): usize? {
        if (req_id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < BATCH_SCHED_MAX_REQS) {
            if (_.slots[i].occupied && _.slots[i].req_id == req_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn countRunning(): usize {
        let c: usize = 0;
        let i: usize = 0;
        while (i < BATCH_SCHED_MAX_REQS) {
            if (_.slots[i].occupied && _.slots[i].state == BatchReqState.running) {
                c += 1;
            }
            i += 1;
        }
        return c;
    }

    /// Returns a value copy of the request slot with `req_id`, or `null` if not found.
    fn getRequest(req_id: u64): BatchRequestSlot? {
        const idx = _.findSlotById(req_id) ?? return null;
        return _.slots[idx];
    }

    /// Submits a new LLM inference request into the `waiting` queue.
    /// Returns `false` if parameters are invalid, `req_id` duplicates an existing slot, or table is full.
    fn submitRequest(
        *_,
        req_id: u64,
        prompt_tokens: u32,
        max_output_tokens: u32,
        priority: u16,
        arrival_ms: i64,
        ttft_deadline_ms: i64,
    ): bool {
        if (req_id == 0 || prompt_tokens == 0 || max_output_tokens == 0) {
            return false;
        }
        if (_ := _.findSlotById(req_id)) {
            return false;
        }

        let free_idx: usize = BATCH_SCHED_MAX_REQS;
        let i: usize = 0;
        while (i < BATCH_SCHED_MAX_REQS) {
            if (!_.slots[i].occupied) {
                free_idx = i;
                break;
            }
            i += 1;
        }
        if (free_idx >= BATCH_SCHED_MAX_REQS) {
            return false;
        }

        _.slots[free_idx] = BatchRequestSlot{
            occupied: true,
            req_id: req_id,
            prompt_tokens: prompt_tokens,
            prefilled_tokens: 0,
            decoded_tokens: 0,
            max_output_tokens: max_output_tokens,
            kv_blocks_held: 0,
            priority: priority,
            arrival_ms: arrival_ms,
            ttft_deadline_ms: ttft_deadline_ms,
            first_token_ms: 0,
            preempt_count: 0,
            state: BatchReqState.waiting,
        };
        return true;
    }

    /// Aborts a non-terminal request and immediately returns all its held KV blocks to the pool.
    fn abortRequest(*_, req_id: u64): bool {
        const idx = _.findSlotById(req_id) ?? return false;
        const st = _.slots[idx].state;
        if (st == BatchReqState.finished || st == BatchReqState.aborted) {
            return false;
        }
        const held = _.slots[idx].kv_blocks_held;
        if (_.used_kv_blocks >= held) {
            _.used_kv_blocks -= held;
        } else {
            _.used_kv_blocks = 0;
        }
        _.slots[idx].kv_blocks_held = 0;
        _.slots[idx].state = BatchReqState.aborted;
        return true;
    }

    /// Frees all `finished` and `aborted` slots for reuse. Returns the number of slots reaped.
    fn reapTerminal(*_): usize {
        let reaped: usize = 0;
        let i: usize = 0;
        while (i < BATCH_SCHED_MAX_REQS) {
            if (_.slots[i].occupied
                && (_.slots[i].state == BatchReqState.finished || _.slots[i].state == BatchReqState.aborted))
            {
                _.slots[i] = BatchRequestSlot{};
                reaped += 1;
            }
            i += 1;
        }
        return reaped;
    }

    /// Picks the lowest-priority, latest-arriving running victim (excluding `exclude_idx` and already scheduled slots).
    fn findPreemptionVictim(exclude_idx: usize, scheduled_mask: [BATCH_SCHED_MAX_REQS]bool): usize? {
        let found: bool = false;
        let best_idx: usize = 0;

        let i: usize = 0;
        while (i < BATCH_SCHED_MAX_REQS) {
            if (i != exclude_idx
                && !scheduled_mask[i]
                && _.slots[i].occupied
                && _.slots[i].state == BatchReqState.running
                && _.slots[i].kv_blocks_held > 0)
            {
                if (!found || victimWorse(_.slots[i], _.slots[best_idx])) {
                    found = true;
                    best_idx = i;
                }
            }
            i += 1;
        }

        if (!found) {
            return null;
        }
        return best_idx;
    }

    fn preemptSlot(*_, victim_idx: usize, out_plan: *BatchStepPlan) {
        const held = _.slots[victim_idx].kv_blocks_held;
        if (_.used_kv_blocks >= held) {
            _.used_kv_blocks -= held;
        } else {
            _.used_kv_blocks = 0;
        }
        // Recomputation preemption: release KV blocks and reset prefilled_tokens so it re-prefills on resume.
        _.slots[victim_idx].kv_blocks_held = 0;
        _.slots[victim_idx].prefilled_tokens = 0;
        _.slots[victim_idx].state = BatchReqState.preempted;
        _.slots[victim_idx].preempt_count += 1;
        _.total_preemptions += 1;

        if (out_plan.preempted_count < BATCH_SCHED_MAX_REQS) {
            out_plan.preempted_ids[out_plan.preempted_count] = _.slots[victim_idx].req_id;
            out_plan.preempted_count += 1;
        }
    }

    /// Schedules one forward iteration step under `max_batch_tokens`, `chunk_size`, `max_running_seqs`, and KV pool limits.
    fn scheduleStep(*_, now_ms: i64, out_plan: *BatchStepPlan) {
        out_plan.reset();
        let rem_tokens: u32 = _.cfg.max_batch_tokens;
        let scheduled_mask: [BATCH_SCHED_MAX_REQS]bool = [false; BATCH_SCHED_MAX_REQS];

        // --------------------------------------------------------------------
        // Phase 1: Protect running Decode requests (1 token/step) with KV preemption if needed
        // --------------------------------------------------------------------
        let decode_idxs: [BATCH_SCHED_MAX_REQS]usize = [0; BATCH_SCHED_MAX_REQS];
        let decode_n: usize = 0;
        let i: usize = 0;
        while (i < BATCH_SCHED_MAX_REQS) {
            if (_.slots[i].occupied && _.slots[i].state == BatchReqState.running && _.slots[i].isPrefillComplete()) {
                decode_idxs[decode_n] = i;
                decode_n += 1;
            }
            i += 1;
        }

        // Deterministic insertion sort by (priority desc, arrival_ms asc, req_id asc).
        let s1: usize = 1;
        while (s1 < decode_n) {
            const key_i: usize = decode_idxs[s1];
            let j: usize = s1;
            while (j > 0 && runningBetter(_.slots[key_i], _.slots[decode_idxs[j - 1]])) {
                decode_idxs[j] = decode_idxs[j - 1];
                j -= 1;
            }
            decode_idxs[j] = key_i;
            s1 += 1;
        }

        let di: usize = 0;
        while (di < decode_n && rem_tokens > 0 && out_plan.item_count < BATCH_SCHED_MAX_STEP_ITEMS) {
            const idx: usize = decode_idxs[di];
            di += 1;
            // May have been preempted earlier in this same loop.
            if (_.slots[idx].state != BatchReqState.running) {
                continue;
            }

            const next_total: u32 = _.slots[idx].committedTokens() + 1;
            const need_blocks: u32 = blocksForTokens(next_total, _.cfg.kv_block_size);
            const delta_blocks: u32 = if (need_blocks > _.slots[idx].kv_blocks_held)
                need_blocks - _.slots[idx].kv_blocks_held
            else
                0;

            while (delta_blocks > _.freeKvBlocks()) {
                const vic = _.findPreemptionVictim(idx, scheduled_mask) ?? break;
                _.preemptSlot(vic, out_plan);
            }

            if (delta_blocks <= _.freeKvBlocks()) {
                _.used_kv_blocks += delta_blocks;
                _.slots[idx].kv_blocks_held += delta_blocks;
                scheduled_mask[idx] = true;
                rem_tokens -= 1;

                out_plan.items[out_plan.item_count] = BatchStepEntry{
                    req_id: _.slots[idx].req_id,
                    phase: BatchExecPhase.decode,
                    tokens_scheduled: 1,
                    kv_blocks_allocated: delta_blocks,
                    is_resume_from_preempt: false,
                };
                out_plan.item_count += 1;
                out_plan.decode_count += 1;
                out_plan.total_tokens_scheduled += 1;
                out_plan.kv_blocks_allocated_step += delta_blocks;
            }
        }

        // --------------------------------------------------------------------
        // Phase 2: Continue in-flight running Chunked-Prefill requests
        // --------------------------------------------------------------------
        let inflight_pf: [BATCH_SCHED_MAX_REQS]usize = [0; BATCH_SCHED_MAX_REQS];
        let inflight_n: usize = 0;
        let k2: usize = 0;
        while (k2 < BATCH_SCHED_MAX_REQS) {
            if (_.slots[k2].occupied
                && _.slots[k2].state == BatchReqState.running
                && !_.slots[k2].isPrefillComplete()
                && !scheduled_mask[k2])
            {
                inflight_pf[inflight_n] = k2;
                inflight_n += 1;
            }
            k2 += 1;
        }

        let s2: usize = 1;
        while (s2 < inflight_n) {
            const key_i: usize = inflight_pf[s2];
            let j: usize = s2;
            while (j > 0 && runningBetter(_.slots[key_i], _.slots[inflight_pf[j - 1]])) {
                inflight_pf[j] = inflight_pf[j - 1];
                j -= 1;
            }
            inflight_pf[j] = key_i;
            s2 += 1;
        }

        let pi: usize = 0;
        while (pi < inflight_n && rem_tokens > 0 && out_plan.item_count < BATCH_SCHED_MAX_STEP_ITEMS) {
            const idx: usize = inflight_pf[pi];
            pi += 1;
            if (_.slots[idx].state != BatchReqState.running) {
                continue;
            }
            const remaining_prompt: u32 = _.slots[idx].prompt_tokens - _.slots[idx].prefilled_tokens;
            let chunk: u32 = if (remaining_prompt < _.cfg.chunk_size) remaining_prompt else _.cfg.chunk_size;
            if (chunk > rem_tokens) {
                chunk = rem_tokens;
            }
            if (chunk == 0) {
                continue;
            }

            // Trim chunk to fit available KV blocks if needed.
            const max_total_tokens_allowed: u32 = (_.slots[idx].kv_blocks_held + _.freeKvBlocks()) * _.cfg.kv_block_size;
            const cur_committed: u32 = _.slots[idx].committedTokens();
            if (max_total_tokens_allowed <= cur_committed) {
                continue;
            }
            const max_addable: u32 = max_total_tokens_allowed - cur_committed;
            if (chunk > max_addable) {
                chunk = max_addable;
            }
            if (chunk == 0) {
                continue;
            }

            const need_blocks: u32 = blocksForTokens(cur_committed + chunk, _.cfg.kv_block_size);
            const delta_blocks: u32 = if (need_blocks > _.slots[idx].kv_blocks_held)
                need_blocks - _.slots[idx].kv_blocks_held
            else
                0;
            if (delta_blocks > _.freeKvBlocks()) {
                continue;
            }

            _.used_kv_blocks += delta_blocks;
            _.slots[idx].kv_blocks_held += delta_blocks;
            scheduled_mask[idx] = true;
            rem_tokens -= chunk;

            out_plan.items[out_plan.item_count] = BatchStepEntry{
                req_id: _.slots[idx].req_id,
                phase: BatchExecPhase.prefill_chunk,
                tokens_scheduled: chunk,
                kv_blocks_allocated: delta_blocks,
                is_resume_from_preempt: false,
            };
            out_plan.item_count += 1;
            out_plan.prefill_count += 1;
            out_plan.total_tokens_scheduled += chunk;
            out_plan.kv_blocks_allocated_step += delta_blocks;
        }

        // --------------------------------------------------------------------
        // Phase 3A: Resume preempted requests first, then Phase 3B: Admit waiting requests
        // --------------------------------------------------------------------
        let running_now: usize = _.countRunning();

        // 3A: Preempted queue
        let pre_idxs: [BATCH_SCHED_MAX_REQS]usize = [0; BATCH_SCHED_MAX_REQS];
        let pre_n: usize = 0;
        let k3: usize = 0;
        while (k3 < BATCH_SCHED_MAX_REQS) {
            if (_.slots[k3].occupied && _.slots[k3].state == BatchReqState.preempted) {
                pre_idxs[pre_n] = k3;
                pre_n += 1;
            }
            k3 += 1;
        }
        let s3a: usize = 1;
        while (s3a < pre_n) {
            const key_i: usize = pre_idxs[s3a];
            let j: usize = s3a;
            while (j > 0 && runningBetter(_.slots[key_i], _.slots[pre_idxs[j - 1]])) {
                pre_idxs[j] = pre_idxs[j - 1];
                j -= 1;
            }
            pre_idxs[j] = key_i;
            s3a += 1;
        }

        let qi: usize = 0;
        while (qi < pre_n
            && rem_tokens > 0
            && running_now < _.cfg.max_running_seqs
            && out_plan.item_count < BATCH_SCHED_MAX_STEP_ITEMS)
        {
            const idx: usize = pre_idxs[qi];
            qi += 1;
            // Skip requests that were just preempted in this exact step to avoid thrashing.
            let just_preempted: bool = false;
            let pk: usize = 0;
            while (pk < out_plan.preempted_count) {
                if (out_plan.preempted_ids[pk] == _.slots[idx].req_id) {
                    just_preempted = true;
                    break;
                }
                pk += 1;
            }
            if (just_preempted) {
                continue;
            }

            const remaining_prompt: u32 = _.slots[idx].prompt_tokens - _.slots[idx].prefilled_tokens;
            let chunk: u32 = if (remaining_prompt < _.cfg.chunk_size) remaining_prompt else _.cfg.chunk_size;
            if (chunk > rem_tokens) {
                chunk = rem_tokens;
            }
            const max_kv_toks: u32 = _.freeKvBlocks() * _.cfg.kv_block_size;
            const target_after: u32 = _.slots[idx].decoded_tokens + chunk;
            const need_blocks: u32 = blocksForTokens(target_after, _.cfg.kv_block_size);
            if (chunk == 0 || max_kv_toks == 0 || need_blocks > _.freeKvBlocks()) {
                continue;
            }

            _.used_kv_blocks += need_blocks;
            _.slots[idx].kv_blocks_held = need_blocks;
            _.slots[idx].state = BatchReqState.running;
            running_now += 1;
            rem_tokens -= chunk;

            out_plan.items[out_plan.item_count] = BatchStepEntry{
                req_id: _.slots[idx].req_id,
                phase: BatchExecPhase.prefill_chunk,
                tokens_scheduled: chunk,
                kv_blocks_allocated: need_blocks,
                is_resume_from_preempt: true,
            };
            out_plan.item_count += 1;
            out_plan.prefill_count += 1;
            out_plan.total_tokens_scheduled += chunk;
            out_plan.kv_blocks_allocated_step += need_blocks;
        }

        // 3B: Waiting queue (TTFT deadline urgency + priority + arrival order)
        let wait_idxs: [BATCH_SCHED_MAX_REQS]usize = [0; BATCH_SCHED_MAX_REQS];
        let wait_n: usize = 0;
        let k4: usize = 0;
        while (k4 < BATCH_SCHED_MAX_REQS) {
            if (_.slots[k4].occupied && _.slots[k4].state == BatchReqState.waiting) {
                wait_idxs[wait_n] = k4;
                wait_n += 1;
            }
            k4 += 1;
        }
        let s3b: usize = 1;
        while (s3b < wait_n) {
            const key_i: usize = wait_idxs[s3b];
            let j: usize = s3b;
            while (j > 0 && waitingBetter(_.slots[key_i], _.slots[wait_idxs[j - 1]], now_ms, _.cfg.ttft_urgency_window_ms)) {
                wait_idxs[j] = wait_idxs[j - 1];
                j -= 1;
            }
            wait_idxs[j] = key_i;
            s3b += 1;
        }

        let wi: usize = 0;
        while (wi < wait_n
            && rem_tokens > 0
            && running_now < _.cfg.max_running_seqs
            && out_plan.item_count < BATCH_SCHED_MAX_STEP_ITEMS)
        {
            const idx: usize = wait_idxs[wi];
            wi += 1;

            const remaining_prompt: u32 = _.slots[idx].prompt_tokens;
            let chunk: u32 = if (remaining_prompt < _.cfg.chunk_size) remaining_prompt else _.cfg.chunk_size;
            if (chunk > rem_tokens) {
                chunk = rem_tokens;
            }
            const max_kv_toks: u32 = _.freeKvBlocks() * _.cfg.kv_block_size;
            if (chunk > max_kv_toks) {
                chunk = max_kv_toks;
            }
            if (chunk == 0) {
                continue;
            }

            const need_blocks: u32 = blocksForTokens(chunk, _.cfg.kv_block_size);
            if (need_blocks > _.freeKvBlocks()) {
                continue;
            }

            _.used_kv_blocks += need_blocks;
            _.slots[idx].kv_blocks_held = need_blocks;
            _.slots[idx].state = BatchReqState.running;
            running_now += 1;
            rem_tokens -= chunk;

            out_plan.items[out_plan.item_count] = BatchStepEntry{
                req_id: _.slots[idx].req_id,
                phase: BatchExecPhase.prefill_chunk,
                tokens_scheduled: chunk,
                kv_blocks_allocated: need_blocks,
                is_resume_from_preempt: false,
            };
            out_plan.item_count += 1;
            out_plan.prefill_count += 1;
            out_plan.total_tokens_scheduled += chunk;
            out_plan.kv_blocks_allocated_step += need_blocks;
        }

        out_plan.free_kv_blocks_after = _.freeKvBlocks();
        _.total_steps += 1;
    }

    /// Commits forward-pass execution results for a running request, auto-releasing KV blocks on completion.
    fn commitStepResult(
        *_,
        req_id: u64,
        tokens_processed: u32,
        reached_eos: bool,
        now_ms: i64,
    ): bool {
        const idx = _.findSlotById(req_id) ?? return false;
        if (_.slots[idx].state != BatchReqState.running) {
            return false;
        }

        if (!_.slots[idx].isPrefillComplete()) {
            const rem: u32 = _.slots[idx].prompt_tokens - _.slots[idx].prefilled_tokens;
            const applied: u32 = if (tokens_processed <= rem) tokens_processed else rem;
            _.slots[idx].prefilled_tokens += applied;
            _.total_prefill_tokens += @as(u64, applied);

            if (_.slots[idx].isPrefillComplete() && _.slots[idx].first_token_ms == 0) {
                _.slots[idx].first_token_ms = now_ms;
                const ttft: u64 = if (now_ms >= _.slots[idx].arrival_ms)
                    @as(u64, @intCast(now_ms - _.slots[idx].arrival_ms))
                else
                    0;
                _.total_ttft_ms += ttft;
                _.ttft_samples += 1;
            }

            if (reached_eos) {
                const held = _.slots[idx].kv_blocks_held;
                if (_.used_kv_blocks >= held) {
                    _.used_kv_blocks -= held;
                } else {
                    _.used_kv_blocks = 0;
                }
                _.slots[idx].kv_blocks_held = 0;
                _.slots[idx].state = BatchReqState.finished;
                _.total_completed += 1;
            }
            return true;
        }

        // Decode phase commit.
        _.slots[idx].decoded_tokens += tokens_processed;
        _.total_decode_tokens += @as(u64, tokens_processed);
        if (_.slots[idx].first_token_ms == 0) {
            _.slots[idx].first_token_ms = now_ms;
            const ttft: u64 = if (now_ms >= _.slots[idx].arrival_ms)
                @as(u64, @intCast(now_ms - _.slots[idx].arrival_ms))
            else
                0;
            _.total_ttft_ms += ttft;
            _.ttft_samples += 1;
        }

        if (reached_eos || _.slots[idx].decoded_tokens >= _.slots[idx].max_output_tokens) {
            const held = _.slots[idx].kv_blocks_held;
            if (_.used_kv_blocks >= held) {
                _.used_kv_blocks -= held;
            } else {
                _.used_kv_blocks = 0;
            }
            _.slots[idx].kv_blocks_held = 0;
            _.slots[idx].state = BatchReqState.finished;
            _.total_completed += 1;
        }
        return true;
    }

    /// Returns a zero-heap telemetry snapshot of the scheduler and KV block pool.
    fn stats(): BatchSchedStats {
        let st = BatchSchedStats{
            capacity: BATCH_SCHED_MAX_REQS,
            used_kv_blocks: _.used_kv_blocks,
            free_kv_blocks: _.freeKvBlocks(),
            total_steps: _.total_steps,
            total_prefill_tokens: _.total_prefill_tokens,
            total_decode_tokens: _.total_decode_tokens,
            total_preemptions: _.total_preemptions,
            total_completed: _.total_completed,
            total_ttft_ms: _.total_ttft_ms,
            ttft_samples: _.ttft_samples,
            avg_ttft_ms: if (_.ttft_samples > 0) _.total_ttft_ms / _.ttft_samples else 0,
        };

        let i: usize = 0;
        while (i < BATCH_SCHED_MAX_REQS) {
            if (_.slots[i].occupied) {
                st.active_slots += 1;
                const rs = _.slots[i].state;
                if (rs == BatchReqState.waiting) {
                    st.waiting_count += 1;
                } else if (rs == BatchReqState.running) {
                    st.running_count += 1;
                } else if (rs == BatchReqState.preempted) {
                    st.preempted_count += 1;
                } else if (rs == BatchReqState.finished) {
                    st.finished_count += 1;
                } else if (rs == BatchReqState.aborted) {
                    st.aborted_count += 1;
                }
            }
            i += 1;
        }
        return st;
    }
}
