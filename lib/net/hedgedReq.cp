// Origin: Pure-CP Stdlib (Adaptive Tail-Latency Hedged Requests & Amplification Budget Scheduler) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation "The Tail at Scale" Hedged Requests scheduler:
//   - Adaptive tail-latency estimator (64-sample ring window computing P95/P99 with cold-start & clamp bounds)
//   - Amplification budget guard (basis-point token bucket capping extra hedge traffic e.g. 10% to prevent storms)
//   - Bounded 64-slot in-flight hedged request FSM (due-hedge polling, first-win cancellation list, fail-fast & timeout reap)

/// Maximum concurrent in-flight hedged requests tracked simultaneously.
pub const HEDGE_MAX_INFLIGHT: usize = 64;

/// Maximum replica attempts per request (1 primary + up to 3 backup replicas).
pub const HEDGE_MAX_REPLICAS: usize = 4;

/// Ring window sample capacity for the adaptive tail-latency estimator.
pub const HEDGE_LATENCY_WINDOW: usize = 64;

/// Basis-point denominator (`10000` = 100%).
pub const HEDGE_BPS_DENOM: u32 = 10000;

// ============================================================================
// Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Lifecycle state of an individual replica attempt.
pub enum HedgeAttemptState: u8 {
    pending = 0,
    in_flight = 1,
    succeeded = 2,
    failed = 3,
    cancelled = 4,
}

/// Overall state of a tracked hedged request.
pub enum HedgeReqState: u8 {
    in_flight = 0,
    completed_primary = 1,
    completed_hedged = 2,
    failed_all = 3,
    timed_out = 4,
}

/// Outcome returned by `HedgedRequestScheduler.completeAttempt`.
pub enum HedgeCompletionKind: u8 {
    winner_primary = 0,
    winner_hedged = 1,
    attempt_failed_continue = 2,
    all_attempts_failed = 3,
    ignored_stale = 4,
}

/// Configuration for `HedgedRequestScheduler`.
pub struct HedgeSchedulerConfig {
    percentile_bps: u32 = 9500;
    min_samples: usize = 8;
    default_hedge_delay_ms: u64 = 50;
    min_hedge_delay_ms: u64 = 5;
    max_hedge_delay_ms: u64 = 2000;
    max_hedge_ratio_bps: u32 = 1000;
    max_burst_hedges: u64 = 8;
    initial_hedge_tokens: u64 = 2;

    static fn defaults(): HedgeSchedulerConfig {
        return HedgeSchedulerConfig{};
    }

    fn isValid(): bool {
        if (_.percentile_bps == 0 || _.percentile_bps > HEDGE_BPS_DENOM) return false;
        if (_.min_samples == 0 || _.min_samples > HEDGE_LATENCY_WINDOW) return false;
        if (_.min_hedge_delay_ms == 0 || _.min_hedge_delay_ms > _.max_hedge_delay_ms) return false;
        if (_.default_hedge_delay_ms < _.min_hedge_delay_ms || _.default_hedge_delay_ms > _.max_hedge_delay_ms) return false;
        if (_.max_hedge_ratio_bps > HEDGE_BPS_DENOM) return false;
        if (_.max_burst_hedges == 0 || _.initial_hedge_tokens > _.max_burst_hedges) return false;
        return true;
    }
}

/// Adaptive P95/P99 tail-latency estimator backed by a fixed 64-sample ring buffer.
pub struct TailLatencyEstimator {
    samples: [HEDGE_LATENCY_WINDOW]u64 = [0; HEDGE_LATENCY_WINDOW];
    write_pos: usize = 0;
    count: usize = 0;
    percentile_bps: u32 = 9500;
    min_samples: usize = 8;
    default_delay_ms: u64 = 50;
    min_delay_ms: u64 = 5;
    max_delay_ms: u64 = 2000;

    static fn init(
        percentile_bps: u32,
        min_samples: usize,
        default_delay_ms: u64,
        min_delay_ms: u64,
        max_delay_ms: u64,
    ): TailLatencyEstimator {
        return TailLatencyEstimator{
            samples: [0; HEDGE_LATENCY_WINDOW],
            write_pos: 0,
            count: 0,
            percentile_bps: percentile_bps,
            min_samples: min_samples,
            default_delay_ms: default_delay_ms,
            min_delay_ms: min_delay_ms,
            max_delay_ms: max_delay_ms,
        };
    }

    /// Records a new latency sample in O(1).
    fn recordSample(*_, latency_ms: u64) {
        _.samples[_.write_pos & (HEDGE_LATENCY_WINDOW - 1)] = latency_ms;
        _.write_pos += 1;
        if (_.count < HEDGE_LATENCY_WINDOW) {
            _.count += 1;
        }
    }

    /// Computes the clamped P95/P99 hedge delay threshold in milliseconds.
    /// Returns `default_delay_ms` while `count < min_samples`.
    fn estimateDelayMs(): u64 {
        if (_.count < _.min_samples) {
            return _.default_delay_ms;
        }
        // Copy active samples to stack and insertion-sort (N <= 64)
        let sorted: [HEDGE_LATENCY_WINDOW]u64 = [0; HEDGE_LATENCY_WINDOW];
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            sorted[i] = _.samples[i];
        }
        let a: usize = 1;
        while (a < _.count) : (a += 1) {
            const key: u64 = sorted[a];
            let b: usize = a;
            while (b > 0 && sorted[b - 1] > key) : (b -= 1) {
                sorted[b] = sorted[b - 1];
            }
            sorted[b] = key;
        }

        // Nearest-rank percentile index in [0, count - 1]
        const rank_num: u64 = ((_.count as u64) * (_.percentile_bps as u64)) + ((HEDGE_BPS_DENOM as u64) - 1);
        let rank: usize = (rank_num / (HEDGE_BPS_DENOM as u64)) as usize;
        if (rank == 0) rank = 1 as usize;
        if (rank > _.count) rank = _.count;
        const raw_ms: u64 = sorted[(rank - 1) as usize];
        if (raw_ms < _.min_delay_ms) return _.min_delay_ms;
        if (raw_ms > _.max_delay_ms) return _.max_delay_ms;
        return raw_ms;
    }
}

/// Basis-point token-bucket amplification guard limiting extra hedge traffic ratio.
pub struct HedgeAmplificationBudget {
    tokens_bps: u64 = 0;
    max_tokens_bps: u64 = 0;
    ratio_bps: u32 = 1000;

    static fn init(ratio_bps: u32, max_burst_hedges: u64, initial_tokens: u64): HedgeAmplificationBudget {
        const denom: u64 = HEDGE_BPS_DENOM as u64;
        const max_bps: u64 = max_burst_hedges * denom;
        const init_bps: u64 = if (initial_tokens <= max_burst_hedges) (initial_tokens * denom) else max_bps;
        return HedgeAmplificationBudget{
            tokens_bps: init_bps,
            max_tokens_bps: max_bps,
            ratio_bps: ratio_bps,
        };
    }

    /// Credits `ratio_bps` toward the hedge token bucket for each primary request.
    fn recordPrimaryRequest(*_) {
        const add: u64 = _.ratio_bps as u64;
        if (_.tokens_bps + add >= _.max_tokens_bps) {
            _.tokens_bps = _.max_tokens_bps;
        } else {
            _.tokens_bps += add;
        }
    }

    /// Attempts to consume 1 full hedge token (`10000` bps). Returns `false` if budget exhausted.
    fn tryConsumeHedge(*_): bool {
        const cost: u64 = HEDGE_BPS_DENOM as u64;
        if (_.tokens_bps < cost) {
            return false;
        }
        _.tokens_bps -= cost;
        return true;
    }

    /// Returns whole hedge tokens currently available.
    fn availableTokens(): u64 {
        return _.tokens_bps / (HEDGE_BPS_DENOM as u64);
    }
}

/// Due hedge launch instruction emitted by `pollDueHedges`.
pub struct HedgeLaunchEvent {
    request_id: u64 = 0;
    attempt_idx: usize = 0;
    node_id: u64 = 0;
    hedge_delay_ms: u64 = 0;
}

/// Outcome of `completeAttempt`, including slow in-flight replicas that should be cancelled.
pub struct HedgeCompletionResult {
    kind: HedgeCompletionKind = HedgeCompletionKind.ignored_stale;
    winner_node_id: u64 = 0;
    winner_attempt_idx: usize = 0;
    latency_ms: u64 = 0;
    cancel_count: usize = 0;
    cancel_node_ids: [HEDGE_MAX_REPLICAS]u64 = [0; HEDGE_MAX_REPLICAS];
    cancel_attempt_idxs: [HEDGE_MAX_REPLICAS]usize = [0; HEDGE_MAX_REPLICAS];

    fn cancelNodesSlice(): []const u64 {
        const n: usize = if (_.cancel_count <= HEDGE_MAX_REPLICAS) _.cancel_count else HEDGE_MAX_REPLICAS;
        return _.cancel_node_ids[0..n];
    }
}

/// Telemetry snapshot for `HedgedRequestScheduler`.
pub struct HedgeSchedulerStats {
    inflight_requests: usize = 0;
    current_hedge_delay_ms: u64 = 0;
    available_budget_tokens: u64 = 0;
    total_primary_requests: u64 = 0;
    total_hedges_launched: u64 = 0;
    total_hedges_suppressed: u64 = 0;
    total_primary_wins: u64 = 0;
    total_hedge_wins: u64 = 0;
    total_all_failed: u64 = 0;
    total_timed_out: u64 = 0;
    estimated_tail_saved_ms: u64 = 0;
}

struct HedgeReplicaSlot {
    node_id: u64 = 0;
    state: HedgeAttemptState = HedgeAttemptState.pending;
    started_ms: u64 = 0;
}

struct HedgeRequestSlot {
    request_id: u64 = 0;
    state: HedgeReqState = HedgeReqState.in_flight;
    started_ms: u64 = 0;
    deadline_ms: u64 = 0;
    next_hedge_at_ms: u64 = 0;
    replica_count: usize = 0;
    replicas: [HEDGE_MAX_REPLICAS]HedgeReplicaSlot = [HedgeReplicaSlot{}; HEDGE_MAX_REPLICAS];
    occupied: bool = false;

    static fn empty(): HedgeRequestSlot {
        return HedgeRequestSlot{};
    }
}

// ============================================================================
// Core Hedged Request Scheduler Engine
// ============================================================================

/// Zero-heap-allocation adaptive hedged-request scheduler with amplification budget control.
pub struct HedgedRequestScheduler {
    config: HedgeSchedulerConfig = HedgeSchedulerConfig.defaults();
    estimator: TailLatencyEstimator = TailLatencyEstimator.init(9500, 8, 50, 5, 2000);
    budget: HedgeAmplificationBudget = HedgeAmplificationBudget.init(1000, 8, 2);
    slots: [HEDGE_MAX_INFLIGHT]HedgeRequestSlot = [HedgeRequestSlot.empty(); HEDGE_MAX_INFLIGHT];

    total_primary_requests: u64 = 0;
    total_hedges_launched: u64 = 0;
    total_hedges_suppressed: u64 = 0;
    total_primary_wins: u64 = 0;
    total_hedge_wins: u64 = 0;
    total_all_failed: u64 = 0;
    total_timed_out: u64 = 0;
    estimated_tail_saved_ms: u64 = 0;

    /// Initializes a `HedgedRequestScheduler`; returns `null` if `cfg` is invalid.
    static fn init(cfg: HedgeSchedulerConfig): HedgedRequestScheduler? {
        if (!cfg.isValid()) return null;
        return HedgedRequestScheduler{
            config: cfg,
            estimator: TailLatencyEstimator.init(
                cfg.percentile_bps,
                cfg.min_samples,
                cfg.default_hedge_delay_ms,
                cfg.min_hedge_delay_ms,
                cfg.max_hedge_delay_ms,
            ),
            budget: HedgeAmplificationBudget.init(
                cfg.max_hedge_ratio_bps,
                cfg.max_burst_hedges,
                cfg.initial_hedge_tokens,
            ),
            slots: [HedgeRequestSlot.empty(); HEDGE_MAX_INFLIGHT],
            total_primary_requests: 0,
            total_hedges_launched: 0,
            total_hedges_suppressed: 0,
            total_primary_wins: 0,
            total_hedge_wins: 0,
            total_all_failed: 0,
            total_timed_out: 0,
            estimated_tail_saved_ms: 0,
        };
    }

    /// Returns the current adaptive hedge delay threshold in milliseconds.
    fn currentHedgeDelayMs(): u64 {
        return _.estimator.estimateDelayMs();
    }

    /// Manually feeds an observed latency sample into the adaptive tail-latency estimator.
    fn recordLatencySample(*_, latency_ms: u64) {
        _.estimator.recordSample(latency_ms);
    }

    /// Starts a new primary request targeting `candidate_nodes[0]` with backup replicas `candidate_nodes[1..]`.
    /// Rejects `request_id == 0`, empty or >4 candidate list, duplicate/zero node IDs, duplicate active request_id, or full table.
    fn startRequest(
        *_,
        request_id: u64,
        candidate_nodes: []const u64,
        timeout_ms: u64,
        now_ms: u64,
    ): bool {
        if (request_id == 0 || candidate_nodes.len == 0 || candidate_nodes.len > HEDGE_MAX_REPLICAS) {
            return false;
        }
        // Validate non-zero and distinct candidate node IDs
        let a: usize = 0;
        while (a < candidate_nodes.len) : (a += 1) {
            if (candidate_nodes[a] == 0) return false;
            let b: usize = a + 1;
            while (b < candidate_nodes.len) : (b += 1) {
                if (candidate_nodes[a] == candidate_nodes[b]) return false;
            }
        }

        let free_idx: usize = HEDGE_MAX_INFLIGHT;
        let i: usize = 0;
        while (i < HEDGE_MAX_INFLIGHT) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].request_id == request_id) return false;
            } else if (free_idx == HEDGE_MAX_INFLIGHT) {
                free_idx = i;
            }
        }
        if (free_idx == HEDGE_MAX_INFLIGHT) return false;

        const delay_ms: u64 = _.estimator.estimateDelayMs();
        let req = HedgeRequestSlot.empty();
        req.request_id = request_id;
        req.state = HedgeReqState.in_flight;
        req.started_ms = now_ms;
        req.deadline_ms = if (timeout_ms > 0) (now_ms + timeout_ms) else 0;
        req.next_hedge_at_ms = now_ms + delay_ms;
        req.replica_count = candidate_nodes.len;
        req.occupied = true;

        let r: usize = 0;
        while (r < candidate_nodes.len) : (r += 1) {
            req.replicas[r] = HedgeReplicaSlot{
                node_id: candidate_nodes[r],
                state: if (r == 0) HedgeAttemptState.in_flight else HedgeAttemptState.pending,
                started_ms: if (r == 0) now_ms else 0,
            };
        }

        _.slots[free_idx] = req;
        _.budget.recordPrimaryRequest();
        _.total_primary_requests += 1;
        return true;
    }

    /// Polls all in-flight requests whose hedge timer has elapsed at `now_ms`, launching
    /// backup replicas up to `out_launches.len` while respecting the amplification budget guard.
    fn pollDueHedges(*_, now_ms: u64, out_launches: *[]HedgeLaunchEvent): usize {
        if (out_launches.len == 0) return 0;
        const delay_ms: u64 = _.estimator.estimateDelayMs();
        let emitted: usize = 0;

        let i: usize = 0;
        while (i < HEDGE_MAX_INFLIGHT && emitted < out_launches.len) : (i += 1) {
            if (!_.slots[i].occupied || _.slots[i].state != HedgeReqState.in_flight) continue;
            if (_.slots[i].deadline_ms > 0 && now_ms >= _.slots[i].deadline_ms) continue;
            if (now_ms < _.slots[i].next_hedge_at_ms) continue;

            // Find next pending backup replica
            let next_r: usize = _.slots[i].replica_count;
            let r: usize = 1;
            while (r < _.slots[i].replica_count) : (r += 1) {
                if (_.slots[i].replicas[r].state == HedgeAttemptState.pending) {
                    next_r = r;
                    break;
                }
            }
            if (next_r >= _.slots[i].replica_count) continue;

            // Check amplification budget guard
            if (!_.budget.tryConsumeHedge()) {
                _.total_hedges_suppressed += 1;
                _.slots[i].next_hedge_at_ms = now_ms + delay_ms;
                continue;
            }

            _.slots[i].replicas[next_r].state = HedgeAttemptState.in_flight;
            _.slots[i].replicas[next_r].started_ms = now_ms;
            _.slots[i].next_hedge_at_ms = now_ms + delay_ms;
            _.total_hedges_launched += 1;

            out_launches[emitted] = HedgeLaunchEvent{
                request_id: _.slots[i].request_id,
                attempt_idx: next_r,
                node_id: _.slots[i].replicas[next_r].node_id,
                hedge_delay_ms: delay_ms,
            };
            emitted += 1;
        }

        return emitted;
    }

    /// Records the completion (success or failure) of `attempt_idx` for `request_id`.
    /// - On first success: completes the request, updates tail-latency estimator, and returns
    ///   all other in-flight replicas to cancel in `cancel_node_ids`.
    /// - On failure: triggers immediate backup dispatch if pending replicas remain, or reports
    ///   `all_attempts_failed` when every replica has failed.
    fn completeAttempt(
        *_,
        request_id: u64,
        attempt_idx: usize,
        succeeded: bool,
        now_ms: u64,
    ): HedgeCompletionResult {
        if (request_id == 0) return HedgeCompletionResult{};

        let i: usize = 0;
        while (i < HEDGE_MAX_INFLIGHT) : (i += 1) {
            if (!_.slots[i].occupied || _.slots[i].request_id != request_id) continue;
            if (_.slots[i].state != HedgeReqState.in_flight || attempt_idx >= _.slots[i].replica_count) {
                return HedgeCompletionResult{};
            }
            if (_.slots[i].replicas[attempt_idx].state != HedgeAttemptState.in_flight) {
                return HedgeCompletionResult{};
            }

            const att_start: u64 = _.slots[i].replicas[attempt_idx].started_ms;
            const lat_ms: u64 = if (now_ms > att_start) (now_ms - att_start) else 0;

            if (succeeded) {
                _.slots[i].replicas[attempt_idx].state = HedgeAttemptState.succeeded;
                _.estimator.recordSample(lat_ms);

                let res = HedgeCompletionResult{
                    kind: if (attempt_idx == 0) HedgeCompletionKind.winner_primary else HedgeCompletionKind.winner_hedged,
                    winner_node_id: _.slots[i].replicas[attempt_idx].node_id,
                    winner_attempt_idx: attempt_idx,
                    latency_ms: lat_ms,
                    cancel_count: 0,
                    cancel_node_ids: [0; HEDGE_MAX_REPLICAS],
                    cancel_attempt_idxs: [0; HEDGE_MAX_REPLICAS],
                };

                // Collect all other in-flight replicas for cancellation
                let r: usize = 0;
                while (r < _.slots[i].replica_count) : (r += 1) {
                    if (r != attempt_idx && _.slots[i].replicas[r].state == HedgeAttemptState.in_flight) {
                        _.slots[i].replicas[r].state = HedgeAttemptState.cancelled;
                        res.cancel_node_ids[res.cancel_count] = _.slots[i].replicas[r].node_id;
                        res.cancel_attempt_idxs[res.cancel_count] = r;
                        res.cancel_count += 1;
                    }
                }

                if (attempt_idx == 0) {
                    _.total_primary_wins += 1;
                } else {
                    _.total_hedge_wins += 1;
                    if (att_start > _.slots[i].started_ms) {
                        _.estimated_tail_saved_ms += (att_start - _.slots[i].started_ms);
                    }
                }
                _.slots[i] = HedgeRequestSlot.empty();
                return res;
            }

            // Attempt failed
            _.slots[i].replicas[attempt_idx].state = HedgeAttemptState.failed;

            let any_inflight: bool = false;
            let any_pending: bool = false;
            let r: usize = 0;
            while (r < _.slots[i].replica_count) : (r += 1) {
                if (_.slots[i].replicas[r].state == HedgeAttemptState.in_flight) any_inflight = true;
                else if (_.slots[i].replicas[r].state == HedgeAttemptState.pending) any_pending = true;
            }

            if (any_pending) {
                // Fail-fast: make next backup immediately eligible on next pollDueHedges
                _.slots[i].next_hedge_at_ms = now_ms;
            }

            if (any_inflight || any_pending) {
                return HedgeCompletionResult{
                    kind: HedgeCompletionKind.attempt_failed_continue,
                    winner_node_id: 0,
                    winner_attempt_idx: attempt_idx,
                    latency_ms: lat_ms,
                    cancel_count: 0,
                    cancel_node_ids: [0; HEDGE_MAX_REPLICAS],
                    cancel_attempt_idxs: [0; HEDGE_MAX_REPLICAS],
                };
            }

            // All replicas failed
            _.total_all_failed += 1;
            _.slots[i] = HedgeRequestSlot.empty();
            return HedgeCompletionResult{
                kind: HedgeCompletionKind.all_attempts_failed,
                winner_node_id: 0,
                winner_attempt_idx: attempt_idx,
                latency_ms: lat_ms,
                cancel_count: 0,
                cancel_node_ids: [0; HEDGE_MAX_REPLICAS],
                cancel_attempt_idxs: [0; HEDGE_MAX_REPLICAS],
            };
        }

        return HedgeCompletionResult{};
    }

    /// Reaps in-flight requests whose deadline has elapsed at `now_ms`, writing timed-out IDs into `out_ids`.
    fn reapTimedOut(*_, now_ms: u64, out_ids: *[]u64): usize {
        let reaped: usize = 0;
        let i: usize = 0;
        while (i < HEDGE_MAX_INFLIGHT && reaped < out_ids.len) : (i += 1) {
            if (!_.slots[i].occupied || _.slots[i].state != HedgeReqState.in_flight) continue;
            if (_.slots[i].deadline_ms > 0 && now_ms >= _.slots[i].deadline_ms) {
                out_ids[reaped] = _.slots[i].request_id;
                reaped += 1;
                _.total_timed_out += 1;
                _.slots[i] = HedgeRequestSlot.empty();
            }
        }
        return reaped;
    }

    /// Returns a comprehensive telemetry snapshot.
    fn stats(): HedgeSchedulerStats {
        let inflight: usize = 0;
        let i: usize = 0;
        while (i < HEDGE_MAX_INFLIGHT) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == HedgeReqState.in_flight) {
                inflight += 1;
            }
        }
        return HedgeSchedulerStats{
            inflight_requests: inflight,
            current_hedge_delay_ms: _.estimator.estimateDelayMs(),
            available_budget_tokens: _.budget.availableTokens(),
            total_primary_requests: _.total_primary_requests,
            total_hedges_launched: _.total_hedges_launched,
            total_hedges_suppressed: _.total_hedges_suppressed,
            total_primary_wins: _.total_primary_wins,
            total_hedge_wins: _.total_hedge_wins,
            total_all_failed: _.total_all_failed,
            total_timed_out: _.total_timed_out,
            estimated_tail_saved_ms: _.estimated_tail_saved_ms,
        };
    }
}
