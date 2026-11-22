// Origin: Pure-CP Stdlib (Zero-Alloc FIPA Contract Net Protocol Multi-Agent Bidding & Adaptive Award Broker) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation FIPA Contract Net Protocol (CNP) task auction & adaptive award engine:
//   - Fixed inline auction table (`CNP_MAX_AUCTIONS = 32`), each accepting up to `CNP_MAX_BIDS_PER_CFP = 16`
//     distinct Worker Agent bids with capability bitmask, cost/latency ceilings, and bid/exec deadlines
//   - Strict bid admission filtering (capability mismatch, cost/latency overflow, late, duplicate, full)
//     and multi-objective pure-integer utility scoring (cost + latency + confidence + reputation)
//   - Automatic runner-up fallback (`fallbackToRunnerUp`) when an awarded worker fails or times out via
//     `reportExecutionOutcome` / `reapExpired`, without re-broadcasting the CFP

/// Maximum number of concurrent CFP auctions managed inline.
pub const CNP_MAX_AUCTIONS: usize = 32;

/// Maximum number of Worker Agent bids accepted per CFP auction.
pub const CNP_MAX_BIDS_PER_CFP: usize = 16;

/// Basis-point normalization scale (`100.00% = 10000`).
pub const CNP_BPS_SCALE: u32 = 10000;

const CNP_NO_BID: usize = 0xFFFF;

// ============================================================================
// 1. Enums, Weights & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Lifecycle state of a single CFP auction.
pub enum CnpAuctionState: u8 {
    open = 0,
    awarded = 1,
    completed = 2,
    failed_no_bids = 3,
    cancelled = 4,
}

/// Lifecycle status of a single Worker Agent bid inside a CFP.
pub enum CnpBidStatus: u8 {
    submitted = 0,
    awarded = 1,
    rejected = 2,
    succeeded = 3,
    failed = 4,
}

/// Admission verdict returned by `ContractNetBroker.submitBid`.
pub enum CnpBidSubmitStatus: u8 {
    accepted = 0,
    cfp_not_found = 1,
    cfp_not_open = 2,
    deadline_passed = 3,
    capability_mismatch = 4,
    cost_exceeded = 5,
    latency_exceeded = 6,
    duplicate_worker = 7,
    bids_full = 8,
    invalid_arg = 9,
}

/// Multi-objective utility weights for ranking valid bids.
pub struct CnpScoreWeights {
    cost_weight: u32 = 3000;
    latency_weight: u32 = 2500;
    confidence_weight: u32 = 2500;
    reputation_weight: u32 = 2000;

    /// Returns balanced default weights summing to `10000`.
    static fn defaults(): CnpScoreWeights {
        return CnpScoreWeights{};
    }

    /// Returns `true` if at least one scoring weight is positive.
    fn isValid(): bool {
        return (@as(u64, _.cost_weight)
            + @as(u64, _.latency_weight)
            + @as(u64, _.confidence_weight)
            + @as(u64, _.reputation_weight)) > 0;
    }
}

/// Single Worker Agent bid stored inline inside an auction.
pub struct CnpBid {
    occupied: bool = false;
    worker_id: u64 = 0;
    caps_mask: u64 = 0;
    proposed_cost: u32 = 0;
    proposed_latency_ms: u32 = 0;
    confidence_bps: u16 = 0;
    reputation_bps: u16 = 0;
    submitted_ms: i64 = 0;
    utility_score: u32 = 0;
    status: CnpBidStatus = CnpBidStatus.submitted;
}

/// Result returned by `evaluateAndAward` and `fallbackToRunnerUp`.
pub struct CnpAwardResult {
    awarded: bool = false;
    cfp_id: u64 = 0;
    winner_worker_id: u64 = 0;
    winner_utility_score: u32 = 0;
    rejected_count: usize = 0;
    is_fallback: bool = false;
    state: CnpAuctionState = CnpAuctionState.failed_no_bids;
}

/// Read-only view of a single CFP auction.
pub struct CnpAuctionView {
    cfp_id: u64 = 0;
    required_caps: u64 = 0;
    max_cost: u32 = 0;
    max_latency_ms: u32 = 0;
    bid_deadline_ms: i64 = 0;
    exec_deadline_ms: i64 = 0;
    state: CnpAuctionState = CnpAuctionState.open;
    bid_count: usize = 0;
    winner_worker_id: u64 = 0;
    fallback_count: u32 = 0;
}

/// Summary report returned by `reapExpired`.
pub struct CnpReapReport {
    open_expired_no_bids: usize = 0;
    open_auto_awarded: usize = 0;
    exec_timed_out: usize = 0;
}

/// Cumulative zero-heap telemetry snapshot across all auctions.
pub struct CnpStats {
    capacity: usize = CNP_MAX_AUCTIONS;
    active_auctions: usize = 0;
    total_cfps: u64 = 0;
    total_awarded: u64 = 0;
    total_completed: u64 = 0;
    total_no_bids_or_failed: u64 = 0;
    total_fallback_awards: u64 = 0;
    total_fallback_successes: u64 = 0;
    total_bids_accepted: u64 = 0;
    total_invalid_bids_rejected: u64 = 0;
}

// ============================================================================
// 2. Internal Auction Storage & Utility Scoring
// ============================================================================

struct CnpAuctionSlot {
    occupied: bool = false;
    cfp_id: u64 = 0;
    required_caps: u64 = 0;
    max_cost: u32 = 0;
    max_latency_ms: u32 = 0;
    bid_deadline_ms: i64 = 0;
    exec_deadline_ms: i64 = 0;
    state: CnpAuctionState = CnpAuctionState.open;
    bids: [CNP_MAX_BIDS_PER_CFP]CnpBid = [CnpBid{}; CNP_MAX_BIDS_PER_CFP];
    bid_count: usize = 0;
    winner_idx: usize = CNP_NO_BID;
    fallback_count: u32 = 0;
}

fn computeBidUtility(
    bid: CnpBid,
    max_cost: u32,
    max_latency_ms: u32,
    weights: CnpScoreWeights,
): u32 {
    const cost_norm: u64 = if (max_cost > 0)
        (@as(u64, max_cost - bid.proposed_cost) * @as(u64, CNP_BPS_SCALE)) / @as(u64, max_cost)
    else
        @as(u64, CNP_BPS_SCALE);

    const lat_norm: u64 = if (max_latency_ms > 0)
        (@as(u64, max_latency_ms - bid.proposed_latency_ms) * @as(u64, CNP_BPS_SCALE)) / @as(u64, max_latency_ms)
    else
        @as(u64, CNP_BPS_SCALE);

    const w_sum: u64 = @as(u64, weights.cost_weight)
        + @as(u64, weights.latency_weight)
        + @as(u64, weights.confidence_weight)
        + @as(u64, weights.reputation_weight);
    if (w_sum == 0) {
        return 0;
    }

    const weighted: u64 = cost_norm * @as(u64, weights.cost_weight)
        + lat_norm * @as(u64, weights.latency_weight)
        + @as(u64, bid.confidence_bps) * @as(u64, weights.confidence_weight)
        + @as(u64, bid.reputation_bps) * @as(u64, weights.reputation_weight);

    return @as(u32, @intCast(weighted / w_sum));
}

fn bidBetter(a: CnpBid, b: CnpBid): bool {
    if (a.utility_score != b.utility_score) {
        return a.utility_score > b.utility_score;
    }
    if (a.proposed_cost != b.proposed_cost) {
        return a.proposed_cost < b.proposed_cost;
    }
    if (a.proposed_latency_ms != b.proposed_latency_ms) {
        return a.proposed_latency_ms < b.proposed_latency_ms;
    }
    return a.worker_id < b.worker_id;
}

// ============================================================================
// 3. ContractNetBroker Core Engine
// ============================================================================

/// Zero-heap FIPA Contract Net Protocol multi-agent task auction & runner-up fallback broker.
pub struct ContractNetBroker {
    weights: CnpScoreWeights = CnpScoreWeights.defaults();
    auctions: [CNP_MAX_AUCTIONS]CnpAuctionSlot = [CnpAuctionSlot{}; CNP_MAX_AUCTIONS];
    active_auctions: usize = 0;
    total_cfps: u64 = 0;
    total_awarded: u64 = 0;
    total_completed: u64 = 0;
    total_no_bids_or_failed: u64 = 0;
    total_fallback_awards: u64 = 0;
    total_fallback_successes: u64 = 0;
    total_bids_accepted: u64 = 0;
    total_invalid_bids_rejected: u64 = 0;

    /// Initializes an empty `ContractNetBroker` with default multi-objective weights.
    static fn init(): ContractNetBroker {
        return ContractNetBroker{};
    }

    /// Initializes an empty `ContractNetBroker` with custom weights, or `null` if weights invalid.
    static fn initWithWeights(weights: CnpScoreWeights): ContractNetBroker? {
        if (!weights.isValid()) {
            return null;
        }
        return ContractNetBroker{
            weights: weights,
        };
    }

    /// Clears all auctions and resets cumulative statistics.
    fn reset(*_) {
        let idx: usize = 0;
        while (idx < CNP_MAX_AUCTIONS) {
            _.auctions[idx] = CnpAuctionSlot{};
            idx += 1;
        }
        _.active_auctions = 0;
        _.total_cfps = 0;
        _.total_awarded = 0;
        _.total_completed = 0;
        _.total_no_bids_or_failed = 0;
        _.total_fallback_awards = 0;
        _.total_fallback_successes = 0;
        _.total_bids_accepted = 0;
        _.total_invalid_bids_rejected = 0;
    }

    fn findAuctionIdx(cfp_id: u64): usize? {
        if (cfp_id == 0) {
            return null;
        }
        let idx: usize = 0;
        while (idx < CNP_MAX_AUCTIONS) {
            if (_.auctions[idx].occupied && _.auctions[idx].cfp_id == cfp_id) {
                return idx;
            }
            idx += 1;
        }
        return null;
    }

    /// Opens a new CFP auction. Returns `false` on invalid arguments, duplicate `cfp_id`, or full table.
    fn openCfp(
        *_,
        cfp_id: u64,
        required_caps: u64,
        max_cost: u32,
        max_latency_ms: u32,
        bid_deadline_ms: i64,
        exec_deadline_ms: i64,
    ): bool {
        if (cfp_id == 0 || bid_deadline_ms <= 0 || exec_deadline_ms <= bid_deadline_ms) {
            return false;
        }
        if (_.active_auctions >= CNP_MAX_AUCTIONS) {
            return false;
        }
        if (_ := _.findAuctionIdx(cfp_id)) {
            return false;
        }

        let free_idx: usize = CNP_MAX_AUCTIONS;
        let idx: usize = 0;
        while (idx < CNP_MAX_AUCTIONS) {
            if (!_.auctions[idx].occupied) {
                free_idx = idx;
                break;
            }
            idx += 1;
        }
        if (free_idx >= CNP_MAX_AUCTIONS) {
            return false;
        }

        _.auctions[free_idx] = CnpAuctionSlot{
            occupied: true,
            cfp_id: cfp_id,
            required_caps: required_caps,
            max_cost: max_cost,
            max_latency_ms: max_latency_ms,
            bid_deadline_ms: bid_deadline_ms,
            exec_deadline_ms: exec_deadline_ms,
            state: CnpAuctionState.open,
        };
        _.active_auctions += 1;
        _.total_cfps += 1;
        return true;
    }

    /// Cancels an open or awarded CFP auction.
    fn cancelCfp(*_, cfp_id: u64): bool {
        const idx = _.findAuctionIdx(cfp_id) ?? return false;
        const st = _.auctions[idx].state;
        if (st == CnpAuctionState.completed || st == CnpAuctionState.failed_no_bids || st == CnpAuctionState.cancelled) {
            return false;
        }
        _.auctions[idx].state = CnpAuctionState.cancelled;
        _.auctions[idx].winner_idx = CNP_NO_BID;
        return true;
    }

    /// Submits a Worker Agent bid for `cfp_id`, strictly validating capabilities, budgets, deadline, and uniqueness.
    fn submitBid(
        *_,
        cfp_id: u64,
        worker_id: u64,
        caps_mask: u64,
        proposed_cost: u32,
        proposed_latency_ms: u32,
        confidence_bps: u16,
        reputation_bps: u16,
        now_ms: i64,
    ): CnpBidSubmitStatus {
        if (worker_id == 0
            || @as(u32, confidence_bps) > CNP_BPS_SCALE
            || @as(u32, reputation_bps) > CNP_BPS_SCALE)
        {
            _.total_invalid_bids_rejected += 1;
            return CnpBidSubmitStatus.invalid_arg;
        }
        const idx_opt = _.findAuctionIdx(cfp_id);
        if (idx_opt == null) {
            _.total_invalid_bids_rejected += 1;
            return CnpBidSubmitStatus.cfp_not_found;
        }
        const idx: usize = idx_opt ?? 0;
        if (_.auctions[idx].state != CnpAuctionState.open) {
            _.total_invalid_bids_rejected += 1;
            return CnpBidSubmitStatus.cfp_not_open;
        }
        if (now_ms > _.auctions[idx].bid_deadline_ms) {
            _.total_invalid_bids_rejected += 1;
            return CnpBidSubmitStatus.deadline_passed;
        }
        if ((caps_mask & _.auctions[idx].required_caps) != _.auctions[idx].required_caps) {
            _.total_invalid_bids_rejected += 1;
            return CnpBidSubmitStatus.capability_mismatch;
        }
        if (proposed_cost > _.auctions[idx].max_cost) {
            _.total_invalid_bids_rejected += 1;
            return CnpBidSubmitStatus.cost_exceeded;
        }
        if (proposed_latency_ms > _.auctions[idx].max_latency_ms) {
            _.total_invalid_bids_rejected += 1;
            return CnpBidSubmitStatus.latency_exceeded;
        }

        // Duplicate worker check.
        let b: usize = 0;
        while (b < _.auctions[idx].bid_count) {
            if (_.auctions[idx].bids[b].worker_id == worker_id) {
                _.total_invalid_bids_rejected += 1;
                return CnpBidSubmitStatus.duplicate_worker;
            }
            b += 1;
        }

        if (_.auctions[idx].bid_count >= CNP_MAX_BIDS_PER_CFP) {
            _.total_invalid_bids_rejected += 1;
            return CnpBidSubmitStatus.bids_full;
        }

        let bid = CnpBid{
            occupied: true,
            worker_id: worker_id,
            caps_mask: caps_mask,
            proposed_cost: proposed_cost,
            proposed_latency_ms: proposed_latency_ms,
            confidence_bps: confidence_bps,
            reputation_bps: reputation_bps,
            submitted_ms: now_ms,
            status: CnpBidStatus.submitted,
        };
        bid.utility_score = computeBidUtility(bid, _.auctions[idx].max_cost, _.auctions[idx].max_latency_ms, _.weights);

        const pos: usize = _.auctions[idx].bid_count;
        _.auctions[idx].bids[pos] = bid;
        _.auctions[idx].bid_count = pos + 1;
        _.total_bids_accepted += 1;
        return CnpBidSubmitStatus.accepted;
    }

    fn awardBestAtSlot(*_, idx: usize, want_status: CnpBidStatus, is_fallback: bool): CnpAwardResult {
        let found: bool = false;
        let best_b: usize = 0;

        let b: usize = 0;
        while (b < _.auctions[idx].bid_count) {
            if (_.auctions[idx].bids[b].occupied && _.auctions[idx].bids[b].status == want_status) {
                if (!found || bidBetter(_.auctions[idx].bids[b], _.auctions[idx].bids[best_b])) {
                    found = true;
                    best_b = b;
                }
            }
            b += 1;
        }

        if (!found) {
            _.auctions[idx].state = CnpAuctionState.failed_no_bids;
            _.auctions[idx].winner_idx = CNP_NO_BID;
            _.total_no_bids_or_failed += 1;
            return CnpAwardResult{
                awarded: false,
                cfp_id: _.auctions[idx].cfp_id,
                is_fallback: is_fallback,
                state: CnpAuctionState.failed_no_bids,
            };
        }

        let rejected_cnt: usize = 0;
        let k: usize = 0;
        while (k < _.auctions[idx].bid_count) {
            if (k == best_b) {
                _.auctions[idx].bids[k].status = CnpBidStatus.awarded;
            } else if (_.auctions[idx].bids[k].status == CnpBidStatus.submitted) {
                _.auctions[idx].bids[k].status = CnpBidStatus.rejected;
                rejected_cnt += 1;
            } else if (_.auctions[idx].bids[k].status == CnpBidStatus.rejected) {
                rejected_cnt += 1;
            }
            k += 1;
        }

        _.auctions[idx].state = CnpAuctionState.awarded;
        _.auctions[idx].winner_idx = best_b;
        if (is_fallback) {
            _.auctions[idx].fallback_count += 1;
            _.total_fallback_awards += 1;
        } else {
            _.total_awarded += 1;
        }

        return CnpAwardResult{
            awarded: true,
            cfp_id: _.auctions[idx].cfp_id,
            winner_worker_id: _.auctions[idx].bids[best_b].worker_id,
            winner_utility_score: _.auctions[idx].bids[best_b].utility_score,
            rejected_count: rejected_cnt,
            is_fallback: is_fallback,
            state: CnpAuctionState.awarded,
        };
    }

    /// Evaluates all submitted bids for `cfp_id`, awards the highest-utility winner, and marks others `rejected`.
    fn evaluateAndAward(*_, cfp_id: u64, now_ms: i64): CnpAwardResult {
        * = now_ms;
        const idx = _.findAuctionIdx(cfp_id) ?? return CnpAwardResult{
            awarded: false,
            cfp_id: cfp_id,
            state: CnpAuctionState.failed_no_bids,
        };
        if (_.auctions[idx].state != CnpAuctionState.open) {
            return CnpAwardResult{
                awarded: false,
                cfp_id: cfp_id,
                state: _.auctions[idx].state,
            };
        }
        return _.awardBestAtSlot(idx, CnpBidStatus.submitted, false);
    }

    /// Reports execution outcome from the currently awarded worker.
    /// On success, completes the CFP; on failure, marks the winner `failed` ready for `fallbackToRunnerUp`.
    fn reportExecutionOutcome(*_, cfp_id: u64, worker_id: u64, succeeded: bool, now_ms: i64): bool {
        * = now_ms;
        const idx = _.findAuctionIdx(cfp_id) ?? return false;
        if (_.auctions[idx].state != CnpAuctionState.awarded) {
            return false;
        }
        const win_idx: usize = _.auctions[idx].winner_idx;
        if (win_idx >= _.auctions[idx].bid_count) {
            return false;
        }
        if (_.auctions[idx].bids[win_idx].worker_id != worker_id) {
            return false;
        }

        if (succeeded) {
            _.auctions[idx].bids[win_idx].status = CnpBidStatus.succeeded;
            _.auctions[idx].state = CnpAuctionState.completed;
            _.total_completed += 1;
            if (_.auctions[idx].fallback_count > 0) {
                _.total_fallback_successes += 1;
            }
            return true;
        }

        _.auctions[idx].bids[win_idx].status = CnpBidStatus.failed;
        _.auctions[idx].winner_idx = CNP_NO_BID;
        return true;
    }

    /// Automatically promotes the highest-utility remaining `rejected` runner-up after a winner fails or times out.
    fn fallbackToRunnerUp(*_, cfp_id: u64, new_exec_deadline_ms: i64, now_ms: i64): CnpAwardResult {
        const idx = _.findAuctionIdx(cfp_id) ?? return CnpAwardResult{
            awarded: false,
            cfp_id: cfp_id,
            is_fallback: true,
            state: CnpAuctionState.failed_no_bids,
        };
        if (_.auctions[idx].state != CnpAuctionState.awarded) {
            return CnpAwardResult{
                awarded: false,
                cfp_id: cfp_id,
                is_fallback: true,
                state: _.auctions[idx].state,
            };
        }
        // If a current winner is still active, mark it failed before promoting runner-up.
        const cur_win: usize = _.auctions[idx].winner_idx;
        if (cur_win < _.auctions[idx].bid_count && _.auctions[idx].bids[cur_win].status == CnpBidStatus.awarded) {
            _.auctions[idx].bids[cur_win].status = CnpBidStatus.failed;
            _.auctions[idx].winner_idx = CNP_NO_BID;
        }
        if (new_exec_deadline_ms > now_ms) {
            _.auctions[idx].exec_deadline_ms = new_exec_deadline_ms;
        }
        return _.awardBestAtSlot(idx, CnpBidStatus.rejected, true);
    }

    /// Sweeps expired open CFPs (auto-awarding if bids exist, else failing no-bids) and timed-out executions.
    fn reapExpired(*_, now_ms: i64): CnpReapReport {
        let rep = CnpReapReport{};
        let idx: usize = 0;
        while (idx < CNP_MAX_AUCTIONS) {
            if (_.auctions[idx].occupied) {
                if (_.auctions[idx].state == CnpAuctionState.open && now_ms > _.auctions[idx].bid_deadline_ms) {
                    const res = _.awardBestAtSlot(idx, CnpBidStatus.submitted, false);
                    if (res.awarded) {
                        rep.open_auto_awarded += 1;
                    } else {
                        rep.open_expired_no_bids += 1;
                    }
                } else if (_.auctions[idx].state == CnpAuctionState.awarded
                    && _.auctions[idx].winner_idx < _.auctions[idx].bid_count
                    && now_ms > _.auctions[idx].exec_deadline_ms)
                {
                    const w_idx: usize = _.auctions[idx].winner_idx;
                    _.auctions[idx].bids[w_idx].status = CnpBidStatus.failed;
                    _.auctions[idx].winner_idx = CNP_NO_BID;
                    rep.exec_timed_out += 1;
                }
            }
            idx += 1;
        }
        return rep;
    }

    /// Removes terminal auctions (`completed` / `failed_no_bids` / `cancelled`) to free slots for reuse.
    fn purgeTerminal(*_): usize {
        let purged: usize = 0;
        let idx: usize = 0;
        while (idx < CNP_MAX_AUCTIONS) {
            if (_.auctions[idx].occupied) {
                const st = _.auctions[idx].state;
                if (st == CnpAuctionState.completed
                    || st == CnpAuctionState.failed_no_bids
                    || st == CnpAuctionState.cancelled)
                {
                    _.auctions[idx] = CnpAuctionSlot{};
                    if (_.active_auctions > 0) {
                        _.active_auctions -= 1;
                    }
                    purged += 1;
                }
            }
            idx += 1;
        }
        return purged;
    }

    /// Returns a read-only view of `cfp_id`, or `null` if not found.
    fn getAuction(cfp_id: u64): CnpAuctionView? {
        const idx = _.findAuctionIdx(cfp_id) ?? return null;
        const a = _.auctions[idx];
        const win_id: u64 = if (a.winner_idx < a.bid_count) a.bids[a.winner_idx].worker_id else 0;
        return CnpAuctionView{
            cfp_id: a.cfp_id,
            required_caps: a.required_caps,
            max_cost: a.max_cost,
            max_latency_ms: a.max_latency_ms,
            bid_deadline_ms: a.bid_deadline_ms,
            exec_deadline_ms: a.exec_deadline_ms,
            state: a.state,
            bid_count: a.bid_count,
            winner_worker_id: win_id,
            fallback_count: a.fallback_count,
        };
    }

    /// Returns a zero-heap cumulative telemetry snapshot.
    fn stats(): CnpStats {
        return CnpStats{
            capacity: CNP_MAX_AUCTIONS,
            active_auctions: _.active_auctions,
            total_cfps: _.total_cfps,
            total_awarded: _.total_awarded,
            total_completed: _.total_completed,
            total_no_bids_or_failed: _.total_no_bids_or_failed,
            total_fallback_awards: _.total_fallback_awards,
            total_fallback_successes: _.total_fallback_successes,
            total_bids_accepted: _.total_bids_accepted,
            total_invalid_bids_rejected: _.total_invalid_bids_rejected,
        };
    }
}
