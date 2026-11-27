// Origin: Pure-CP Stdlib (AI Inference Continuous Batching & Prefix-Cache Affinity Scheduler) | Created: 2026-11-22

const PREFIX_AFFINITY_BONUS: i64 = 10;

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) {
        return 0xFFFFFFFF;
    }
    return a + b;
}

fn blocksNeeded(tokens: u32, tokens_per_block: u32): u32 {
    if (tokens == 0 || tokens_per_block == 0) {
        return 0;
    }
    return ((tokens - 1) / tokens_per_block) + 1;
}

fn effectivePriority(req: InferRequest, now_ms: i64, aging_ms: i64): i64 {
    let bonus: i64 = 0;
    if (aging_ms > 0 && now_ms > req.arrival_ms) {
        bonus = (now_ms - req.arrival_ms) / aging_ms;
    }
    return (req.priority as i64) + bonus;
}

fn idInSlice(ids: []const u64, len: usize, target: u64): bool {
    let i: usize = 0;
    while (i < len) {
        if (ids[i] == target) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Execution state of an inference request inside `BatchScheduler`.
pub enum ReqState {
    Queued,
    Prefill,
    Decode,
    Finished,
    Preempted,
    Cancelled,
}

/// Inference request descriptor tracked in a caller-provided fixed slot array.
pub struct InferRequest {
    id: u64,
    prefix_hash: u64,
    prompt_tokens: u32,
    generated_tokens: u32,
    max_new_tokens: u32,
    priority: i32,
    arrival_ms: i64,
    kv_blocks_held: u32,
    state: ReqState,
}

/// Hard resource limits and anti-starvation aging configuration for `BatchScheduler`.
pub struct BatchLimits {
    max_batch_reqs: usize,
    max_batch_tokens: u32,
    total_kv_blocks: u32,
    tokens_per_kv_block: u32,
    aging_ms_per_priority: i64,
}

/// Scheduling outcome returned by `BatchScheduler.stepSchedule`.
pub struct ScheduleStep {
    prefill_count: usize,
    decode_count: usize,
    preempted_count: usize,
    scheduled_tokens: u32,
    kv_blocks_used: u32,
}

/// Zero-allocation LLM Continuous Batching & Prefix-Cache Affinity Scheduler (vLLM / SGLang style).
pub struct BatchScheduler {
    requests: []InferRequest,
    count: usize,
    limits: BatchLimits,
    kv_blocks_used: u32,

    /// Initializes `BatchScheduler` over caller-provided `requests` buffer.
    /// Returns `null` if any hard capacity limit is zero.
    static fn init(requests: *[]InferRequest, limits: BatchLimits): BatchScheduler? {
        if (
            limits.max_batch_reqs == 0 ||
            limits.max_batch_tokens == 0 ||
            limits.total_kv_blocks == 0 ||
            limits.tokens_per_kv_block == 0
        ) {
            return null;
        }
        return BatchScheduler {
            requests: requests.*,
            count: 0,
            limits: limits,
            kv_blocks_used: 0,
        };
    }

    /// Returns remaining free KV-cache blocks.
    fn freeKvBlocks(): u32 {
        if (_.kv_blocks_used >= _.limits.total_kv_blocks) {
            return 0;
        }
        return _.limits.total_kv_blocks - _.kv_blocks_used;
    }

    /// Returns a copy of request `id`, or `null` if not found.
    fn requestAt(id: u64): InferRequest? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.requests[i].id == id) {
                return _.requests[i];
            }
            i += 1;
        }
        return null;
    }

    /// Enqueues a new inference request in `Queued` state.
    /// Rejects zero `prompt_tokens`, duplicate `id`, or full request pool.
    fn enqueue(
        *_,
        id: u64,
        prefix_hash: u64,
        prompt_tokens: u32,
        max_new_tokens: u32,
        priority: i32,
        arrival_ms: i64,
    ): bool {
        if (prompt_tokens == 0 || _.count >= _.requests.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.count) {
            if (_.requests[i].id == id) {
                return false;
            }
            i += 1;
        }

        _.requests[_.count] = InferRequest {
            id: id,
            prefix_hash: prefix_hash,
            prompt_tokens: prompt_tokens,
            generated_tokens: 0,
            max_new_tokens: max_new_tokens,
            priority: priority,
            arrival_ms: arrival_ms,
            kv_blocks_held: 0,
            state: ReqState.Queued,
        };
        _.count += 1;
        return true;
    }

    fn preemptLowestDecode(*_, now_ms: i64, max_victim_eff: i64, strict_lt: bool, exclude_id: u64): bool {
        let found: bool = false;
        let victim_idx: usize = 0;
        let victim_eff: i64 = 0;

        let i: usize = 0;
        while (i < _.count) {
            const req = _.requests[i];
            if (req.state == ReqState.Decode && req.id != exclude_id && req.kv_blocks_held > 0) {
                const eff = effectivePriority(req, now_ms, _.limits.aging_ms_per_priority);
                const eligible = if (strict_lt) (eff < max_victim_eff) else (eff <= max_victim_eff);
                if (eligible) {
                    if (
                        !found ||
                        eff < victim_eff ||
                        (eff == victim_eff && req.arrival_ms > _.requests[victim_idx].arrival_ms) ||
                        (eff == victim_eff && req.arrival_ms == _.requests[victim_idx].arrival_ms && req.id > _.requests[victim_idx].id)
                    ) {
                        found = true;
                        victim_idx = i;
                        victim_eff = eff;
                    }
                }
            }
            i += 1;
        }

        if (!found) {
            return false;
        }

        const freed = _.requests[victim_idx].kv_blocks_held;
        if (freed >= _.kv_blocks_used) {
            _.kv_blocks_used = 0;
        } else {
            _.kv_blocks_used -= freed;
        }
        _.requests[victim_idx].kv_blocks_held = 0;
        _.requests[victim_idx].state = ReqState.Preempted;
        return true;
    }

    /// Executes one continuous-batching scheduling iteration:
    /// 1. Preempts strictly lower-priority `Decode` requests if a higher-priority waiting request needs KV blocks.
    /// 2. Schedules active `Decode` requests in effective-priority order (preempting lower-priority victims on KV growth).
    /// 3. Admits waiting `Queued`/`Preempted` requests into `Prefill` using effective priority + prefix-cache affinity.
    fn stepSchedule(
        *_,
        now_ms: i64,
        active_prefix_hash: u64,
        prefill_out: *[]u64,
        decode_out: *[]u64,
    ): ScheduleStep {
        let prefill_count: usize = 0;
        let decode_count: usize = 0;
        let preempted_count: usize = 0;
        let scheduled_reqs: usize = 0;
        let scheduled_tokens: u32 = 0;

        // Phase 0: If a strictly higher-priority waiting request cannot fit in free KV blocks,
        // preempt lower-priority Decode requests to make room.
        while (true) {
            let found_high: bool = false;
            let high_eff: i64 = 0;
            let high_blocks: u32 = 0;

            let i: usize = 0;
            while (i < _.count) {
                const req = _.requests[i];
                if (req.state == ReqState.Queued || req.state == ReqState.Preempted) {
                    const ptoks = satAddU32(req.prompt_tokens, req.generated_tokens);
                    const need = blocksNeeded(ptoks, _.limits.tokens_per_kv_block);
                    if (need <= _.limits.total_kv_blocks && need > _.freeKvBlocks() && ptoks <= _.limits.max_batch_tokens) {
                        const eff = effectivePriority(req, now_ms, _.limits.aging_ms_per_priority);
                        if (!found_high || eff > high_eff) {
                            found_high = true;
                            high_eff = eff;
                            high_blocks = need;
                        }
                    }
                }
                i += 1;
            }

            if (!found_high) {
                break;
            }
            if (_.freeKvBlocks() >= high_blocks) {
                break;
            }
            if (!_.preemptLowestDecode(now_ms, high_eff, true, 0)) {
                break;
            }
            preempted_count += 1;
        }

        // Phase 1: Prioritize active Decode requests in descending effective priority.
        while (
            scheduled_reqs < _.limits.max_batch_reqs &&
            decode_count < decode_out.*.len &&
            scheduled_tokens < _.limits.max_batch_tokens
        ) {
            let found: bool = false;
            let best_idx: usize = 0;
            let best_eff: i64 = 0;

            let i: usize = 0;
            while (i < _.count) {
                const req = _.requests[i];
                if (req.state == ReqState.Decode && !idInSlice(decode_out.*[0..decode_count], decode_count, req.id)) {
                    const eff = effectivePriority(req, now_ms, _.limits.aging_ms_per_priority);
                    if (
                        !found ||
                        eff > best_eff ||
                        (eff == best_eff && req.arrival_ms < _.requests[best_idx].arrival_ms) ||
                        (eff == best_eff && req.arrival_ms == _.requests[best_idx].arrival_ms && req.id < _.requests[best_idx].id)
                    ) {
                        found = true;
                        best_idx = i;
                        best_eff = eff;
                    }
                }
                i += 1;
            }

            if (!found) {
                break;
            }

            const total_after = satAddU32(satAddU32(_.requests[best_idx].prompt_tokens, _.requests[best_idx].generated_tokens), 1);
            const needed = blocksNeeded(total_after, _.limits.tokens_per_kv_block);
            const extra: u32 = if (needed > _.requests[best_idx].kv_blocks_held)
                (needed - _.requests[best_idx].kv_blocks_held)
            else
                0;

            while (extra > _.freeKvBlocks()) {
                if (!_.preemptLowestDecode(now_ms, best_eff, false, _.requests[best_idx].id)) {
                    break;
                }
                preempted_count += 1;
            }

            if (extra > _.freeKvBlocks()) {
                // Even after preempting all lower-priority victims, KV pool is exhausted; preempt self.
                const freed = _.requests[best_idx].kv_blocks_held;
                if (freed >= _.kv_blocks_used) {
                    _.kv_blocks_used = 0;
                } else {
                    _.kv_blocks_used -= freed;
                }
                _.requests[best_idx].kv_blocks_held = 0;
                _.requests[best_idx].state = ReqState.Preempted;
                preempted_count += 1;
                continue;
            }

            _.requests[best_idx].kv_blocks_held += extra;
            _.kv_blocks_used += extra;
            decode_out.*[decode_count] = _.requests[best_idx].id;
            decode_count += 1;
            scheduled_reqs += 1;
            scheduled_tokens += 1;
        }

        // Phase 2: Admit waiting Queued / Preempted requests into Prefill with prefix-cache affinity + aging.
        while (
            scheduled_reqs < _.limits.max_batch_reqs &&
            prefill_count < prefill_out.*.len &&
            scheduled_tokens < _.limits.max_batch_tokens
        ) {
            const rem_tokens = _.limits.max_batch_tokens - scheduled_tokens;
            const free_blks = _.freeKvBlocks();

            let found: bool = false;
            let best_idx: usize = 0;
            let best_score: i64 = 0;
            let best_toks: u32 = 0;
            let best_blks: u32 = 0;

            let i: usize = 0;
            while (i < _.count) {
                const req = _.requests[i];
                if (req.state == ReqState.Queued || req.state == ReqState.Preempted) {
                    const ptoks = satAddU32(req.prompt_tokens, req.generated_tokens);
                    const need_blks = blocksNeeded(ptoks, _.limits.tokens_per_kv_block);
                    if (ptoks <= rem_tokens && need_blks <= free_blks) {
                        const affinity: i64 = if (active_prefix_hash != 0 && req.prefix_hash == active_prefix_hash)
                            PREFIX_AFFINITY_BONUS
                        else
                            0;
                        const score = effectivePriority(req, now_ms, _.limits.aging_ms_per_priority) + affinity;
                        if (
                            !found ||
                            score > best_score ||
                            (score == best_score && req.arrival_ms < _.requests[best_idx].arrival_ms) ||
                            (score == best_score && req.arrival_ms == _.requests[best_idx].arrival_ms && req.id < _.requests[best_idx].id)
                        ) {
                            found = true;
                            best_idx = i;
                            best_score = score;
                            best_toks = ptoks;
                            best_blks = need_blks;
                        }
                    }
                }
                i += 1;
            }

            if (!found) {
                break;
            }

            _.requests[best_idx].state = ReqState.Prefill;
            _.requests[best_idx].kv_blocks_held = best_blks;
            _.kv_blocks_used += best_blks;
            prefill_out.*[prefill_count] = _.requests[best_idx].id;
            prefill_count += 1;
            scheduled_reqs += 1;
            scheduled_tokens += best_toks;
        }

        return ScheduleStep {
            prefill_count: prefill_count,
            decode_count: decode_count,
            preempted_count: preempted_count,
            scheduled_tokens: scheduled_tokens,
            kv_blocks_used: _.kv_blocks_used,
        };
    }

    /// Advances a `Prefill` or `Decode` request by `new_tokens`.
    /// Transitions `Prefill -> Decode`, and automatically transitions to `Finished` releasing KV blocks
    /// once `generated_tokens >= max_new_tokens`.
    fn advanceDecode(*_, id: u64, new_tokens: u32): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (_.requests[i].id == id) {
                if (_.requests[i].state != ReqState.Prefill && _.requests[i].state != ReqState.Decode) {
                    return false;
                }
                _.requests[i].state = ReqState.Decode;
                _.requests[i].generated_tokens = satAddU32(_.requests[i].generated_tokens, new_tokens);
                if (_.requests[i].generated_tokens >= _.requests[i].max_new_tokens) {
                    _.requests[i].state = ReqState.Finished;
                    const freed = _.requests[i].kv_blocks_held;
                    if (freed >= _.kv_blocks_used) {
                        _.kv_blocks_used = 0;
                    } else {
                        _.kv_blocks_used -= freed;
                    }
                    _.requests[i].kv_blocks_held = 0;
                }
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Cancels a non-terminal request and immediately releases any held KV blocks.
    fn cancel(*_, id: u64): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (_.requests[i].id == id) {
                if (_.requests[i].state == ReqState.Finished || _.requests[i].state == ReqState.Cancelled) {
                    return false;
                }
                const freed = _.requests[i].kv_blocks_held;
                if (freed >= _.kv_blocks_used) {
                    _.kv_blocks_used = 0;
                } else {
                    _.kv_blocks_used -= freed;
                }
                _.requests[i].kv_blocks_held = 0;
                _.requests[i].state = ReqState.Cancelled;
                return true;
            }
            i += 1;
        }
        return false;
    }
}
