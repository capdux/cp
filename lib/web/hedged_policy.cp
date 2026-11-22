// Origin: Pure-CP Stdlib (Google "The Tail at Scale" Hedged Requests & Retry Budget Storm-Prevention Controller) | Created: 2026-11-22
// Pure CP zero-heap-allocation tail-latency hedging & retry-storm prevention engine for microservices & LLM API gateways:
//   1. `RetryBudgetConfig`, `RetryBudgetStats`, `RetryBudget`:
//      - Fixed-point basis-point (10,000 bps = 1 credit) retry budget strictly capping retry + hedge volume
//        at a configured percentage of normal traffic (e.g. 10%) plus a minimum per-second floor allowance
//   2. `HedgedRequestConfig`, `HedgedAction`, `HedgedAttemptState`, `HedgedWinResult`, `HedgedRequestTracker`:
//      - Tracks primary + up to `max_hedges` in-flight hedge replicas with zero heap allocation
//      - Automatically disables hedging for non-idempotent requests (`idempotent_only`)
//      - Cooperates with `RetryBudget.tryConsumeRetry` (degrades to single-request wait when budget exhausted)
//      - Cancels losing in-flight replicas immediately upon first winner (`onReplicaSucceeded`)

/// Maximum total attempts tracked per request (1 primary + up to 7 hedges).
pub const HEDGED_MAX_ATTEMPTS: usize = 8;

const BPS_PER_RETRY: u64 = 10000;

// ============================================================================
// 1. Retry Budget Storm-Prevention Controller (`RetryBudget`)
// ============================================================================

/// Configuration for `RetryBudget`.
pub struct RetryBudgetConfig {
    /// Allowed retry + hedge ratio in basis points (1000 = 10%, max 10000 = 100%).
    retry_ratio_bps: u32 = 1000;
    /// Minimum guaranteed retries per window even at low traffic (0 = pure ratio mode).
    min_retries_per_sec: u32 = 10;
    /// Time window in milliseconds for `min_retries_per_sec` replenishment (default 1000ms).
    window_ms: u64 = 1000;
    /// Maximum accumulated retry burst credits.
    max_burst_retries: u32 = 100;

    static fn default(): RetryBudgetConfig {
        return RetryBudgetConfig{};
    }

    fn isValid(): bool {
        return _.retry_ratio_bps > 0 &&
            _.retry_ratio_bps <= 10000 &&
            _.window_ms > 0 &&
            _.max_burst_retries > 0;
    }
}

/// Telemetry snapshot for `RetryBudget`.
pub struct RetryBudgetStats {
    total_requests: u64 = 0;
    retries_allowed: u64 = 0;
    retries_rejected: u64 = 0;
    available_retries: u64 = 0;
}

/// Zero-heap-allocation fixed-point Retry Budget capping cluster-wide retry + hedge traffic
/// to prevent retry storms during downstream degradation.
pub struct RetryBudget {
    config: RetryBudgetConfig;
    credits_bps: u64 = 0;
    max_credits_bps: u64 = 0;
    last_refill_ms: u64 = 0;
    refill_rem_ms: u64 = 0;
    total_requests: u64 = 0;
    retries_allowed: u64 = 0;
    retries_rejected: u64 = 0;

    /// Initializes a retry budget at `now_ms`. Returns `null` if `config` is invalid.
    static fn init(config: RetryBudgetConfig, now_ms: u64): RetryBudget? {
        if (!config.isValid()) return null;
        const max_bps: u64 = (config.max_burst_retries as u64) * BPS_PER_RETRY;
        let init_bps: u64 = (config.min_retries_per_sec as u64) * BPS_PER_RETRY;
        if (init_bps > max_bps) init_bps = max_bps;

        return RetryBudget{
            config: config,
            credits_bps: init_bps,
            max_credits_bps: max_bps,
            last_refill_ms: now_ms,
            refill_rem_ms: 0,
            total_requests: 0,
            retries_allowed: 0,
            retries_rejected: 0,
        };
    }

    fn refillFloor(*_, now_ms: u64) {
        if (now_ms <= _.last_refill_ms) return;
        const elapsed: u64 = now_ms - _.last_refill_ms;
        _.last_refill_ms = now_ms;

        if (_.config.min_retries_per_sec == 0) return;
        const total_ms: u64 = elapsed + _.refill_rem_ms;
        const rate_bps: u64 = (_.config.min_retries_per_sec as u64) * BPS_PER_RETRY;
        const whole_windows: u64 = total_ms / _.config.window_ms;
        const rem_ms: u64 = total_ms % _.config.window_ms;
        _.refill_rem_ms = rem_ms;

        if (whole_windows > 0) {
            const room: u64 = _.max_credits_bps - _.credits_bps;
            if (whole_windows >= room / rate_bps + 1) {
                _.credits_bps = _.max_credits_bps;
            } else {
                const add_bps: u64 = whole_windows * rate_bps;
                if (add_bps >= room) {
                    _.credits_bps = _.max_credits_bps;
                } else {
                    _.credits_bps += add_bps;
                }
            }
        }
    }

    /// Records a normal primary request, depositing `retry_ratio_bps` into the retry budget.
    fn recordRequest(*_, now_ms: u64) {
        _.refillFloor(now_ms);
        _.total_requests += 1;
        const deposit: u64 = _.config.retry_ratio_bps as u64;
        if (deposit >= _.max_credits_bps - _.credits_bps) {
            _.credits_bps = _.max_credits_bps;
        } else {
            _.credits_bps += deposit;
        }
    }

    /// Attempts to consume 1 retry/hedge credit (`10,000` bps).
    /// Returns `true` if allowed, `false` if the retry budget is exhausted.
    fn tryConsumeRetry(*_, now_ms: u64): bool {
        _.refillFloor(now_ms);
        if (_.credits_bps >= BPS_PER_RETRY) {
            _.credits_bps -= BPS_PER_RETRY;
            _.retries_allowed += 1;
            return true;
        }
        _.retries_rejected += 1;
        return false;
    }

    /// Returns the whole number of retries currently available in the budget.
    fn availableRetries(): u64 {
        return _.credits_bps / BPS_PER_RETRY;
    }

    /// Returns a point-in-time telemetry snapshot.
    fn stats(): RetryBudgetStats {
        return RetryBudgetStats{
            total_requests: _.total_requests,
            retries_allowed: _.retries_allowed,
            retries_rejected: _.retries_rejected,
            available_retries: _.credits_bps / BPS_PER_RETRY,
        };
    }
}

// ============================================================================
// 2. Hedged Request State Machine (`HedgedRequestTracker`)
// ============================================================================

/// Policy configuration for a single hedged request.
pub struct HedgedRequestConfig {
    hedge_delay_ms: u64 = 50;
    max_hedges: u8 = 2;
    total_timeout_ms: u64 = 500;
    idempotent_only: bool = true;

    static fn default(): HedgedRequestConfig {
        return HedgedRequestConfig{};
    }

    fn isValid(): bool {
        return _.hedge_delay_ms > 0 &&
            _.total_timeout_ms > 0 &&
            (_.max_hedges as usize) < HEDGED_MAX_ATTEMPTS;
    }
}

/// Next action recommended by `HedgedRequestTracker.evaluate`.
pub enum HedgedAction {
    Wait,
    LaunchHedge,
    GiveUpTimeout,
    Complete,
}

/// Per-replica attempt lifecycle state.
pub enum HedgedAttemptState {
    Unused,
    InFlight,
    Succeeded,
    Failed,
    Cancelled,
}

/// Outcome returned when a replica completes successfully (`onReplicaSucceeded`).
pub struct HedgedWinResult {
    accepted: bool = false;
    winner_attempt: u8 = 0;
    cancelled_inflight: u8 = 0;
}

/// Zero-heap-allocation per-request hedged execution tracker coordinating primary + hedge copies
/// with `RetryBudget` storm protection and first-winner cancellation.
pub struct HedgedRequestTracker {
    config: HedgedRequestConfig;
    is_idempotent: bool = true;
    start_ms: u64 = 0;
    last_launch_ms: u64 = 0;
    launched_count: u8 = 1;
    inflight_count: u8 = 1;
    attempts: [8]HedgedAttemptState;
    attempt_start_ms: [8]u64;
    completed: bool = false;
    timed_out: bool = false;
    winner_attempt: u8? = null;
    hedges_launched: u8 = 0;
    hedges_blocked_by_budget: u8 = 0;

    /// Initializes a tracker with primary attempt `0` in-flight at `start_ms`.
    /// Returns `null` if `config` is invalid.
    static fn init(
        config: HedgedRequestConfig,
        is_idempotent: bool,
        start_ms: u64,
    ): HedgedRequestTracker? {
        if (!config.isValid()) return null;
        let att: [8]HedgedAttemptState = [HedgedAttemptState.Unused; 8];
        let ts: [8]u64 = [0; 8];
        att[0] = HedgedAttemptState.InFlight;
        ts[0] = start_ms;

        return HedgedRequestTracker{
            config: config,
            is_idempotent: is_idempotent,
            start_ms: start_ms,
            last_launch_ms: start_ms,
            launched_count: 1,
            inflight_count: 1,
            attempts: att,
            attempt_start_ms: ts,
            completed: false,
            timed_out: false,
            winner_attempt: null,
            hedges_launched: 0,
            hedges_blocked_by_budget: 0,
        };
    }

    fn cancelAllInFlight(*_): u8 {
        let cancelled: u8 = 0;
        let i: usize = 0;
        while (i < (_.launched_count as usize)) : (i += 1) {
            if (_.attempts[i] == HedgedAttemptState.InFlight) {
                _.attempts[i] = HedgedAttemptState.Cancelled;
                cancelled += 1;
            }
        }
        _.inflight_count = 0;
        return cancelled;
    }

    /// Evaluates the next action at `now_ms`, checking total timeout, idempotency, hedge delay,
    /// and `budget.tryConsumeRetry` before authorizing a new hedge replica.
    fn evaluate(*_, now_ms: u64, budget: *RetryBudget): HedgedAction {
        if (_.completed) return HedgedAction.Complete;
        if (_.timed_out) return HedgedAction.GiveUpTimeout;

        // 1. Check overall request deadline
        if (now_ms >= _.start_ms && (now_ms - _.start_ms) >= _.config.total_timeout_ms) {
            * = _.cancelAllInFlight();
            _.timed_out = true;
            return HedgedAction.GiveUpTimeout;
        }

        const hedging_allowed: bool = (!_.config.idempotent_only) || _.is_idempotent;
        const can_launch_more: bool = hedging_allowed && (_.hedges_launched < _.config.max_hedges);

        // 2. All launched attempts failed and no further hedge is permitted
        if (_.inflight_count == 0 && !can_launch_more) {
            _.timed_out = true;
            return HedgedAction.GiveUpTimeout;
        }

        // 3. Launch a hedge if previous attempts failed or hedge_delay_ms has elapsed
        const delay_elapsed: bool = (now_ms >= _.last_launch_ms) &&
            ((now_ms - _.last_launch_ms) >= _.config.hedge_delay_ms);

        if (can_launch_more && (_.inflight_count == 0 || delay_elapsed)) {
            if (budget.tryConsumeRetry(now_ms)) {
                const idx: usize = _.launched_count as usize;
                _.attempts[idx] = HedgedAttemptState.InFlight;
                _.attempt_start_ms[idx] = now_ms;
                _.launched_count += 1;
                _.inflight_count += 1;
                _.hedges_launched += 1;
                _.last_launch_ms = now_ms;
                return HedgedAction.LaunchHedge;
            }
            _.hedges_blocked_by_budget += 1;
            if (_.inflight_count == 0) {
                _.timed_out = true;
                return HedgedAction.GiveUpTimeout;
            }
            return HedgedAction.Wait;
        }

        return HedgedAction.Wait;
    }

    /// Records the first winning replica, marks `winner_attempt`, and cancels all other
    /// in-flight replicas, returning how many sibling attempts should be aborted.
    fn onReplicaSucceeded(*_, attempt_idx: u8): HedgedWinResult {
        if (_.completed || _.timed_out) return HedgedWinResult{};
        const idx: usize = attempt_idx as usize;
        if (idx >= (_.launched_count as usize)) return HedgedWinResult{};
        if (_.attempts[idx] != HedgedAttemptState.InFlight) return HedgedWinResult{};

        _.attempts[idx] = HedgedAttemptState.Succeeded;
        let cancelled: u8 = 0;
        let i: usize = 0;
        while (i < (_.launched_count as usize)) : (i += 1) {
            if (i != idx && _.attempts[i] == HedgedAttemptState.InFlight) {
                _.attempts[i] = HedgedAttemptState.Cancelled;
                cancelled += 1;
            }
        }
        _.inflight_count = 0;
        _.completed = true;
        _.winner_attempt = attempt_idx;
        return HedgedWinResult{
            accepted: true,
            winner_attempt: attempt_idx,
            cancelled_inflight: cancelled,
        };
    }

    /// Marks an in-flight replica attempt as failed.
    fn onReplicaFailed(*_, attempt_idx: u8): bool {
        if (_.completed || _.timed_out) return false;
        const idx: usize = attempt_idx as usize;
        if (idx >= (_.launched_count as usize)) return false;
        if (_.attempts[idx] != HedgedAttemptState.InFlight) return false;

        _.attempts[idx] = HedgedAttemptState.Failed;
        if (_.inflight_count > 0) _.inflight_count -= 1;
        return true;
    }

    /// Returns the lifecycle state of `attempt_idx`.
    fn attemptState(attempt_idx: u8): HedgedAttemptState {
        const idx: usize = attempt_idx as usize;
        if (idx >= HEDGED_MAX_ATTEMPTS) return HedgedAttemptState.Unused;
        return _.attempts[idx];
    }

    /// Returns the index of the most recently launched attempt (`0` = primary, `1..` = hedges).
    fn latestAttemptIdx(): u8 {
        return _.launched_count - 1;
    }
}
