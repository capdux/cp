// Origin: Pure-CP Stdlib (vLLM/Sarathi-Style Chunked-Prefill + Decode Continuous Batching Iteration Scheduler) | Created: 2026-11-22
// Pure CP zero-heap-allocation LLM inference iteration-level continuous batching scheduler:
//   1. `IterSchedulerConfig`, `SeqRunState`, `SeqSchedEntry`, `ScheduledStepItem`, `IterStepPlan`, `IterSchedulerStats`
//   2. `ContinuousIterScheduler`:
//      - Decode-first scheduling to protect inter-token latency (TPOT / ITL)
//      - Priority + recency-aware KV-block preemption (`Preempted`) to prevent memory deadlocks
//      - Chunked Prefill using remaining per-step token budget (Sarathi-Serve style) for high throughput & low TTFT
//      - Automatic KV-block accounting & release on EOS, max_new_tokens completion, or cancellation

/// Configuration for `ContinuousIterScheduler`.
pub struct IterSchedulerConfig {
    max_batch_seqs: usize = 32;
    max_token_budget_per_step: usize = 512;
    max_kv_blocks: usize = 256;
    block_size: usize = 16;

    /// Returns a sensible default scheduler configuration.
    static fn default(): IterSchedulerConfig {
        return IterSchedulerConfig{};
    }

    /// Returns `true` if all capacity and block parameters are strictly positive.
    fn isValid(): bool {
        return _.max_batch_seqs > 0 &&
            _.max_token_budget_per_step > 0 &&
            _.max_kv_blocks > 0 &&
            _.block_size > 0;
    }
}

/// Lifecycle state of a sequence managed by `ContinuousIterScheduler`.
pub enum SeqRunState {
    Waiting,
    Prefilling,
    Decoding,
    Preempted,
    Finished,
}

/// Per-sequence scheduling descriptor stored in caller-provided zero-heap buffer.
pub struct SeqSchedEntry {
    active: bool = false;
    req_id: u64 = 0;
    state: SeqRunState = SeqRunState.Waiting;
    priority: i32 = 0;
    enqueue_order: u64 = 0;
    prompt_tokens: usize = 0;
    prefilled_tokens: usize = 0;
    max_new_tokens: usize = 0;
    decoded_tokens: usize = 0;
    kv_blocks_held: usize = 0;
    in_flight_tokens: usize = 0;
    in_flight_is_prefill: bool = false;
    preempt_count: u32 = 0;
    last_preempt_step: u64 = 0;
    finish_reason_eos: bool = false;

    /// Total KV tokens required before this sequence can enter or resume `Decoding`.
    fn prefillTarget(): usize {
        return _.prompt_tokens + _.decoded_tokens;
    }

    /// Remaining prefill tokens still to be processed.
    fn remainingPrefillTokens(): usize {
        const target: usize = _.prompt_tokens + _.decoded_tokens;
        if (_.prefilled_tokens >= target) return 0;
        return target - _.prefilled_tokens;
    }
}

/// Single scheduled work item emitted by `ContinuousIterScheduler.scheduleStep`.
pub struct ScheduledStepItem {
    req_id: u64 = 0;
    is_prefill: bool = false;
    start_pos: usize = 0;
    num_tokens: usize = 0;
    new_kv_blocks: usize = 0;
    priority: i32 = 0;
}

/// Summary of a single scheduled forward iteration.
pub struct IterStepPlan {
    step_id: u64 = 0;
    num_items: usize = 0;
    num_decode_seqs: usize = 0;
    num_prefill_seqs: usize = 0;
    total_scheduled_tokens: usize = 0;
    preempted_count: usize = 0;
    kv_blocks_used: usize = 0;
    kv_blocks_free: usize = 0;
}

/// Point-in-time and cumulative telemetry snapshot for `ContinuousIterScheduler`.
pub struct IterSchedulerStats {
    step_count: u64 = 0;
    active_seqs: usize = 0;
    waiting_seqs: usize = 0;
    prefilling_seqs: usize = 0;
    decoding_seqs: usize = 0;
    preempted_seqs: usize = 0;
    finished_seqs: usize = 0;
    kv_blocks_used: usize = 0;
    kv_blocks_free: usize = 0;
    total_enqueued: u64 = 0;
    total_finished: u64 = 0;
    total_preemptions: u64 = 0;
    total_prefill_tokens: u64 = 0;
    total_decode_tokens: u64 = 0;
}

/// Computes `ceil(tokens / block_size)` safely without overflow.
fn blocksNeededForTokens(tokens: usize, block_size: usize): usize {
    if (tokens == 0 || block_size == 0) return 0;
    return ((tokens - 1) / block_size) + 1;
}

/// Ranking helper for Phase 2 prefill admission within identical priority:
/// Prefilling (2) > Preempted (1) > Waiting (0).
fn prefillStateRank(state: SeqRunState): u8 {
    if (state == SeqRunState.Prefilling) return 2;
    if (state == SeqRunState.Preempted) return 1;
    return 0;
}

/// Returns `true` if sequence `a` has strictly higher scheduling priority than `b`
/// for Decode scheduling (higher `priority`, tie-break older `enqueue_order`).
fn isBetterDecodeCandidate(a: SeqSchedEntry, b: SeqSchedEntry): bool {
    if (a.priority != b.priority) return a.priority > b.priority;
    return a.enqueue_order < b.enqueue_order;
}

/// Returns `true` if sequence `a` has strictly higher scheduling priority than `b`
/// for Chunked-Prefill scheduling (higher `priority`, tie-break in-progress state rank, then older `enqueue_order`).
fn isBetterPrefillCandidate(a: SeqSchedEntry, b: SeqSchedEntry): bool {
    if (a.priority != b.priority) return a.priority > b.priority;
    const ra: u8 = prefillStateRank(a.state);
    const rb: u8 = prefillStateRank(b.state);
    if (ra != rb) return ra > rb;
    return a.enqueue_order < b.enqueue_order;
}

/// Returns `true` if `a` is a better preemption victim than `b`
/// (lower `priority` first; tie-break newest `enqueue_order` first).
fn isBetterPreemptVictim(a: SeqSchedEntry, b: SeqSchedEntry): bool {
    if (a.priority != b.priority) return a.priority < b.priority;
    return a.enqueue_order > b.enqueue_order;
}

/// Returns `true` if `victim` is eligible to be preempted on behalf of `requester`
/// (never preempts a strictly higher-priority or older same-priority request).
fn canPreemptFor(victim: SeqSchedEntry, requester: SeqSchedEntry): bool {
    if (victim.priority < requester.priority) return true;
    if (victim.priority == requester.priority && victim.enqueue_order > requester.enqueue_order) {
        return true;
    }
    return false;
}

/// Zero-heap-allocation vLLM/Sarathi-style Chunked-Prefill + Decode Continuous Batching Scheduler.
pub struct ContinuousIterScheduler {
    entries: []SeqSchedEntry;
    config: IterSchedulerConfig;
    kv_blocks_used: usize = 0;
    step_count: u64 = 0;
    next_enqueue_order: u64 = 1;
    total_enqueued: u64 = 0;
    total_finished: u64 = 0;
    total_preemptions: u64 = 0;
    total_prefill_tokens: u64 = 0;
    total_decode_tokens: u64 = 0;

    /// Initializes the scheduler over caller-provided `entries_buf`.
    /// Returns `null` if `entries_buf.len == 0` or `config` has any zero capacity/block parameter.
    static fn init(entries_buf: *[]SeqSchedEntry, config: IterSchedulerConfig): ContinuousIterScheduler? {
        if (entries_buf.len == 0 || !config.isValid()) return null;
        let i: usize = 0;
        while (i < entries_buf.len) : (i += 1) {
            entries_buf[i] = SeqSchedEntry{};
        }
        return ContinuousIterScheduler{
            entries: entries_buf,
            config: config,
            kv_blocks_used: 0,
            step_count: 0,
            next_enqueue_order: 1,
            total_enqueued: 0,
            total_finished: 0,
            total_preemptions: 0,
            total_prefill_tokens: 0,
            total_decode_tokens: 0,
        };
    }

    /// Enqueues a new inference request into the scheduler in `Waiting` state.
    /// Reuses an empty slot first, or recycles the oldest `Finished` slot if all slots were used.
    /// Returns `false` on `prompt_tokens == 0`, duplicate active `req_id`, or full active table.
    fn enqueue(*_, req_id: u64, prompt_tokens: usize, max_new_tokens: usize, priority: i32): bool {
        if (prompt_tokens == 0) return false;

        let empty_idx: usize? = null;
        let recycle_idx: usize? = null;
        let recycle_order: u64 = 0;

        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            const e: SeqSchedEntry = _.entries[i];
            if (!e.active) {
                if (empty_idx == null) empty_idx = i;
                continue;
            }
            if (e.state != SeqRunState.Finished) {
                if (e.req_id == req_id) return false;
            } else {
                if (recycle_idx == null || e.enqueue_order < recycle_order) {
                    recycle_idx = i;
                    recycle_order = e.enqueue_order;
                }
            }
        }

        const slot: usize = empty_idx ?? (recycle_idx ?? return false);
        const order: u64 = _.next_enqueue_order;
        _.next_enqueue_order += 1;
        _.total_enqueued += 1;

        _.entries[slot] = SeqSchedEntry{
            active: true,
            req_id: req_id,
            state: SeqRunState.Waiting,
            priority: priority,
            enqueue_order: order,
            prompt_tokens: prompt_tokens,
            prefilled_tokens: 0,
            max_new_tokens: max_new_tokens,
            decoded_tokens: 0,
            kv_blocks_held: 0,
            in_flight_tokens: 0,
            in_flight_is_prefill: false,
            preempt_count: 0,
            last_preempt_step: 0,
            finish_reason_eos: false,
        };
        return true;
    }

    /// Plans one continuous-batching forward iteration into `out_items`:
    /// 1. Prioritizes in-flight `Decoding` sequences (1 token each) to protect TPOT.
    ///    If KV blocks are insufficient, preempts the lowest-priority / newest-enqueued
    ///    victim sequence (`Preempted`), releasing its KV blocks to prevent deadlocks.
    /// 2. Allocates remaining `max_token_budget_per_step` to `Prefilling`, `Preempted`,
    ///    and `Waiting` sequences via Chunked Prefill (shrinking chunk size if KV blocks are tight).
    fn scheduleStep(*_, out_items: *[]ScheduledStepItem): IterStepPlan {
        _.step_count += 1;
        const cur_step: u64 = _.step_count;

        let plan = IterStepPlan{
            step_id: cur_step,
            num_items: 0,
            num_decode_seqs: 0,
            num_prefill_seqs: 0,
            total_scheduled_tokens: 0,
            preempted_count: 0,
            kv_blocks_used: _.kv_blocks_used,
            kv_blocks_free: _.config.max_kv_blocks - _.kv_blocks_used,
        };

        const max_items: usize = if (out_items.len < _.config.max_batch_seqs) out_items.len else _.config.max_batch_seqs;
        let rem_budget: usize = _.config.max_token_budget_per_step;
        if (max_items == 0 || rem_budget == 0) {
            return plan;
        }

        // --------------------------------------------------------------------
        // Phase 1: Decode-first scheduling with priority/recency preemption
        // --------------------------------------------------------------------
        let decode_cursor_prio: i32 = 0;
        let decode_cursor_order: u64 = 0;
        let has_decode_cursor: bool = false;

        while (plan.num_items < max_items && rem_budget > 0) {
            // Find the best eligible Decoding sequence strictly after the previous cursor
            let best_idx: usize? = null;
            let i: usize = 0;
            while (i < _.entries.len) : (i += 1) {
                const e: SeqSchedEntry = _.entries[i];
                if (!e.active || e.state != SeqRunState.Decoding || e.in_flight_tokens > 0) {
                    continue;
                }
                if (e.decoded_tokens >= e.max_new_tokens) {
                    continue;
                }
                if (has_decode_cursor) {
                    const after_cursor: bool = (e.priority < decode_cursor_prio) ||
                        (e.priority == decode_cursor_prio && e.enqueue_order > decode_cursor_order);
                    if (!after_cursor) continue;
                }
                if (bi := best_idx) {
                    if (isBetterDecodeCandidate(e, _.entries[bi])) {
                        best_idx = i;
                    }
                } else {
                    best_idx = i;
                }
            }

            const idx: usize = best_idx ?? break;
            decode_cursor_prio = _.entries[idx].priority;
            decode_cursor_order = _.entries[idx].enqueue_order;
            has_decode_cursor = true;

            const cur_tokens: usize = _.entries[idx].prompt_tokens + _.entries[idx].decoded_tokens;
            const needed_total: usize = blocksNeededForTokens(cur_tokens + 1, _.config.block_size);
            const delta_blocks: usize = if (needed_total > _.entries[idx].kv_blocks_held) {
                needed_total - _.entries[idx].kv_blocks_held
            } else {
                0
            };

            // Preempt lower-priority / newest-enqueued sequences if KV blocks are insufficient
            while (delta_blocks > (_.config.max_kv_blocks - _.kv_blocks_used)) {
                let victim_idx: usize? = null;
                let j: usize = 0;
                while (j < _.entries.len) : (j += 1) {
                    if (j == idx) continue;
                    const v: SeqSchedEntry = _.entries[j];
                    if (!v.active || v.kv_blocks_held == 0 || v.in_flight_tokens > 0) continue;
                    if (v.state != SeqRunState.Decoding && v.state != SeqRunState.Prefilling) continue;
                    if (!canPreemptFor(v, _.entries[idx])) continue;

                    if (vi := victim_idx) {
                        if (isBetterPreemptVictim(v, _.entries[vi])) {
                            victim_idx = j;
                        }
                    } else {
                        victim_idx = j;
                    }
                }

                const v_idx: usize = victim_idx ?? break;
                const freed: usize = _.entries[v_idx].kv_blocks_held;
                _.kv_blocks_used -= freed;
                _.entries[v_idx].kv_blocks_held = 0;
                _.entries[v_idx].prefilled_tokens = 0;
                _.entries[v_idx].state = SeqRunState.Preempted;
                _.entries[v_idx].preempt_count += 1;
                _.entries[v_idx].last_preempt_step = cur_step;
                plan.preempted_count += 1;
                _.total_preemptions += 1;
            }

            // If still not enough KV blocks even after exhausting eligible victims, skip this sequence
            if (delta_blocks > (_.config.max_kv_blocks - _.kv_blocks_used)) {
                continue;
            }

            _.kv_blocks_used += delta_blocks;
            _.entries[idx].kv_blocks_held += delta_blocks;
            _.entries[idx].in_flight_tokens = 1;
            _.entries[idx].in_flight_is_prefill = false;

            out_items[plan.num_items] = ScheduledStepItem{
                req_id: _.entries[idx].req_id,
                is_prefill: false,
                start_pos: cur_tokens,
                num_tokens: 1,
                new_kv_blocks: delta_blocks,
                priority: _.entries[idx].priority,
            };
            plan.num_items += 1;
            plan.num_decode_seqs += 1;
            plan.total_scheduled_tokens += 1;
            rem_budget -= 1;
        }

        // --------------------------------------------------------------------
        // Phase 2: Chunked-Prefill scheduling using remaining token budget
        // --------------------------------------------------------------------
        let prefill_cursor_prio: i32 = 0;
        let prefill_cursor_rank: u8 = 0;
        let prefill_cursor_order: u64 = 0;
        let has_prefill_cursor: bool = false;

        while (plan.num_items < max_items && rem_budget > 0) {
            let best_idx: usize? = null;
            let i: usize = 0;
            while (i < _.entries.len) : (i += 1) {
                const e: SeqSchedEntry = _.entries[i];
                if (!e.active || e.in_flight_tokens > 0) continue;
                if (e.state != SeqRunState.Prefilling &&
                    e.state != SeqRunState.Preempted &&
                    e.state != SeqRunState.Waiting) {
                    continue;
                }
                // Do not immediately re-admit a sequence preempted in this exact step
                if (e.state == SeqRunState.Preempted && e.last_preempt_step == cur_step) {
                    continue;
                }
                if (e.remainingPrefillTokens() == 0) continue;

                const r: u8 = prefillStateRank(e.state);
                if (has_prefill_cursor) {
                    const after_cursor: bool = (e.priority < prefill_cursor_prio) ||
                        (e.priority == prefill_cursor_prio && r < prefill_cursor_rank) ||
                        (e.priority == prefill_cursor_prio && r == prefill_cursor_rank && e.enqueue_order > prefill_cursor_order);
                    if (!after_cursor) continue;
                }

                if (bi := best_idx) {
                    if (isBetterPrefillCandidate(e, _.entries[bi])) {
                        best_idx = i;
                    }
                } else {
                    best_idx = i;
                }
            }

            const idx: usize = best_idx ?? break;
            prefill_cursor_prio = _.entries[idx].priority;
            prefill_cursor_rank = prefillStateRank(_.entries[idx].state);
            prefill_cursor_order = _.entries[idx].enqueue_order;
            has_prefill_cursor = true;

            const rem_prefill: usize = _.entries[idx].remainingPrefillTokens();
            let chunk: usize = if (rem_prefill < rem_budget) rem_prefill else rem_budget;

            const free_blocks: usize = _.config.max_kv_blocks - _.kv_blocks_used;
            const max_blocks_now: usize = _.entries[idx].kv_blocks_held + free_blocks;
            const max_tokens_now: usize = max_blocks_now * _.config.block_size;

            if (max_tokens_now <= _.entries[idx].prefilled_tokens) {
                // Zero KV capacity left for this sequence right now; try next candidate
                continue;
            }
            const fittable: usize = max_tokens_now - _.entries[idx].prefilled_tokens;
            if (chunk > fittable) {
                chunk = fittable;
            }
            if (chunk == 0) continue;

            const target_after: usize = _.entries[idx].prefilled_tokens + chunk;
            const needed_total: usize = blocksNeededForTokens(target_after, _.config.block_size);
            const delta_blocks: usize = if (needed_total > _.entries[idx].kv_blocks_held) {
                needed_total - _.entries[idx].kv_blocks_held
            } else {
                0
            };

            _.kv_blocks_used += delta_blocks;
            _.entries[idx].kv_blocks_held += delta_blocks;
            _.entries[idx].state = SeqRunState.Prefilling;
            _.entries[idx].in_flight_tokens = chunk;
            _.entries[idx].in_flight_is_prefill = true;

            out_items[plan.num_items] = ScheduledStepItem{
                req_id: _.entries[idx].req_id,
                is_prefill: true,
                start_pos: _.entries[idx].prefilled_tokens,
                num_tokens: chunk,
                new_kv_blocks: delta_blocks,
                priority: _.entries[idx].priority,
            };
            plan.num_items += 1;
            plan.num_prefill_seqs += 1;
            plan.total_scheduled_tokens += chunk;
            rem_budget -= chunk;
        }

        plan.kv_blocks_used = _.kv_blocks_used;
        plan.kv_blocks_free = _.config.max_kv_blocks - _.kv_blocks_used;
        return plan;
    }

    /// Commits the execution result for a scheduled request in the current iteration.
    /// - Advances `prefilled_tokens` (for prefill chunks) or `decoded_tokens` (for decode steps).
    /// - Transitions `Prefilling -> Decoding` once all prompt/recompute tokens are prefilled.
    /// - Automatically transitions to `Finished` and releases all held KV blocks upon `hit_eos`
    ///   or when `decoded_tokens >= max_new_tokens`.
    /// Returns `false` if `req_id` is not found or was not scheduled in-flight.
    fn commitStep(*_, req_id: u64, generated_tokens: usize, hit_eos: bool): bool {
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (!_.entries[i].active || _.entries[i].state == SeqRunState.Finished) continue;
            if (_.entries[i].req_id != req_id) continue;
            if (_.entries[i].in_flight_tokens == 0) return false;

            const scheduled: usize = _.entries[i].in_flight_tokens;
            const was_prefill: bool = _.entries[i].in_flight_is_prefill;
            _.entries[i].in_flight_tokens = 0;
            _.entries[i].in_flight_is_prefill = false;

            if (was_prefill) {
                _.entries[i].prefilled_tokens += scheduled;
                _.total_prefill_tokens += scheduled as u64;

                const target: usize = _.entries[i].prefillTarget();
                if (_.entries[i].prefilled_tokens >= target) {
                    _.entries[i].prefilled_tokens = target;
                    if (generated_tokens > 0) {
                        _.entries[i].decoded_tokens += generated_tokens;
                        _.total_decode_tokens += generated_tokens as u64;
                    }
                    if (hit_eos || _.entries[i].decoded_tokens >= _.entries[i].max_new_tokens) {
                        _.kv_blocks_used -= _.entries[i].kv_blocks_held;
                        _.entries[i].kv_blocks_held = 0;
                        _.entries[i].state = SeqRunState.Finished;
                        _.entries[i].finish_reason_eos = hit_eos;
                        _.total_finished += 1;
                    } else {
                        _.entries[i].state = SeqRunState.Decoding;
                    }
                } else if (hit_eos) {
                    _.kv_blocks_used -= _.entries[i].kv_blocks_held;
                    _.entries[i].kv_blocks_held = 0;
                    _.entries[i].state = SeqRunState.Finished;
                    _.entries[i].finish_reason_eos = true;
                    _.total_finished += 1;
                }
            } else {
                if (generated_tokens > 0) {
                    _.entries[i].decoded_tokens += generated_tokens;
                    _.total_decode_tokens += generated_tokens as u64;
                }
                if (hit_eos || _.entries[i].decoded_tokens >= _.entries[i].max_new_tokens) {
                    _.kv_blocks_used -= _.entries[i].kv_blocks_held;
                    _.entries[i].kv_blocks_held = 0;
                    _.entries[i].state = SeqRunState.Finished;
                    _.entries[i].finish_reason_eos = hit_eos;
                    _.total_finished += 1;
                }
            }
            return true;
        }
        return false;
    }

    /// Cancels an active non-finished request and immediately releases its held KV blocks.
    fn cancelRequest(*_, req_id: u64): bool {
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (!_.entries[i].active || _.entries[i].state == SeqRunState.Finished) continue;
            if (_.entries[i].req_id != req_id) continue;

            _.kv_blocks_used -= _.entries[i].kv_blocks_held;
            _.entries[i].kv_blocks_held = 0;
            _.entries[i].in_flight_tokens = 0;
            _.entries[i].in_flight_is_prefill = false;
            _.entries[i].state = SeqRunState.Finished;
            _.total_finished += 1;
            return true;
        }
        return false;
    }

    /// Looks up a sequence entry by `req_id` (prefers non-finished match if present).
    fn getEntry(req_id: u64): SeqSchedEntry? {
        let finished_match: SeqSchedEntry? = null;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            const e: SeqSchedEntry = _.entries[i];
            if (!e.active || e.req_id != req_id) continue;
            if (e.state != SeqRunState.Finished) return e;
            finished_match = e;
        }
        return finished_match;
    }

    /// Returns the number of currently free KV blocks.
    fn kvBlocksFree(): usize {
        return _.config.max_kv_blocks - _.kv_blocks_used;
    }

    /// Returns the number of currently allocated KV blocks.
    fn kvBlocksUsed(): usize {
        return _.kv_blocks_used;
    }

    /// Returns the number of active non-finished sequences.
    fn activeCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].active && _.entries[i].state != SeqRunState.Finished) {
                n += 1;
            }
        }
        return n;
    }

    /// Returns a full point-in-time and cumulative telemetry snapshot.
    fn stats(): IterSchedulerStats {
        let st = IterSchedulerStats{
            step_count: _.step_count,
            active_seqs: 0,
            waiting_seqs: 0,
            prefilling_seqs: 0,
            decoding_seqs: 0,
            preempted_seqs: 0,
            finished_seqs: 0,
            kv_blocks_used: _.kv_blocks_used,
            kv_blocks_free: _.config.max_kv_blocks - _.kv_blocks_used,
            total_enqueued: _.total_enqueued,
            total_finished: _.total_finished,
            total_preemptions: _.total_preemptions,
            total_prefill_tokens: _.total_prefill_tokens,
            total_decode_tokens: _.total_decode_tokens,
        };

        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            const e: SeqSchedEntry = _.entries[i];
            if (!e.active) continue;
            if (e.state == SeqRunState.Waiting) {
                st.waiting_seqs += 1;
                st.active_seqs += 1;
            } else if (e.state == SeqRunState.Prefilling) {
                st.prefilling_seqs += 1;
                st.active_seqs += 1;
            } else if (e.state == SeqRunState.Decoding) {
                st.decoding_seqs += 1;
                st.active_seqs += 1;
            } else if (e.state == SeqRunState.Preempted) {
                st.preempted_seqs += 1;
                st.active_seqs += 1;
            } else {
                st.finished_seqs += 1;
            }
        }
        return st;
    }
}
