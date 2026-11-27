// Origin: Pure-CP Stdlib (AI Agent Multi-Model Gateway: RPM/TPM Limiter, Cooldown & Failover) | Created: 2026-11-22

const WINDOW_DURATION_MS: i64 = 60000;
const BASE_BACKOFF_MS: i64 = 1000;
const MAX_BACKOFF_MS: i64 = 60000;
const EWMA_ALPHA: f64 = 0.3;

fn isNaN(x: f64): bool {
    return x != x;
}

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) {
        return 0xFFFFFFFF;
    }
    return a + b;
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return a + b;
}

fn satAddI64(a: i64, b: i64): i64 {
    const max_i64: i64 = 9223372036854775807;
    if (b > 0 && a > max_i64 - b) {
        return max_i64;
    }
    return a + b;
}

/// Model selection policy for `ModelRouter.selectEndpoint`.
pub enum ModelRoutePolicy {
    PriorityOrder,
    LowestCost,
    LowestLatency,
}

/// Descriptor and live rate-limit/cooldown/latency state for a single LLM model endpoint.
pub struct ModelEndpoint {
    id: u32,
    name: string,
    priority: i32,
    cost_per_1m_micro_usd: u64,
    rpm_limit: u32,
    tpm_limit: u64,
    window_start_ms: i64,
    window_reqs: u32,
    window_tokens: u64,
    cooldown_until_ms: i64,
    consecutive_errors: u32,
    ewma_latency_ms: f64,
    enabled: bool,
}

fn windowExpired(ep: ModelEndpoint, now_ms: i64): bool {
    if (ep.window_reqs == 0 && ep.window_tokens == 0) {
        return true;
    }
    if (now_ms < ep.window_start_ms) {
        return true;
    }
    return (now_ms - ep.window_start_ms) >= WINDOW_DURATION_MS;
}

fn endpointFitsBudget(ep: ModelEndpoint, est_tokens: u64, now_ms: i64): bool {
    if (!ep.enabled) {
        return false;
    }
    if (ep.cooldown_until_ms > now_ms) {
        return false;
    }

    const expired = windowExpired(ep, now_ms);
    const reqs: u32 = if (expired) 0 else ep.window_reqs;
    const toks: u64 = if (expired) 0 else ep.window_tokens;

    if (ep.rpm_limit > 0 && reqs >= ep.rpm_limit) {
        return false;
    }
    if (ep.tpm_limit > 0) {
        if (toks > ep.tpm_limit || est_tokens > (ep.tpm_limit - toks)) {
            return false;
        }
    }
    return true;
}

fn isBetterEndpoint(cand: ModelEndpoint, best: ModelEndpoint, policy: ModelRoutePolicy): bool {
    match (policy) {
        ModelRoutePolicy.PriorityOrder => {
            if (cand.priority != best.priority) {
                return cand.priority > best.priority;
            }
            return cand.id < best.id;
        }
        ModelRoutePolicy.LowestCost => {
            if (cand.cost_per_1m_micro_usd != best.cost_per_1m_micro_usd) {
                return cand.cost_per_1m_micro_usd < best.cost_per_1m_micro_usd;
            }
            if (cand.priority != best.priority) {
                return cand.priority > best.priority;
            }
            return cand.id < best.id;
        }
        ModelRoutePolicy.LowestLatency => {
            if (cand.ewma_latency_ms != best.ewma_latency_ms) {
                return cand.ewma_latency_ms < best.ewma_latency_ms;
            }
            if (cand.priority != best.priority) {
                return cand.priority > best.priority;
            }
            return cand.id < best.id;
        }
    }
}

/// Zero-allocation fixed-pool multi-model LLM gateway router with RPM/TPM rate limiting,
/// EWMA latency tracking, and 429/5xx cooldown failover.
pub struct ModelRouter {
    endpoints: []ModelEndpoint,
    count: usize,

    /// Initializes an empty `ModelRouter` backed by caller-provided `endpoints`.
    static fn init(endpoints: *[]ModelEndpoint): ModelRouter {
        return ModelRouter {
            endpoints: endpoints.*,
            count: 0,
        };
    }

    /// Registers a new model endpoint. Rejects empty `name`, duplicate `id`, or full pool.
    /// Returns the assigned slot index on success, or `null`.
    fn addEndpoint(
        *_,
        id: u32,
        name: string,
        priority: i32,
        cost_per_1m_micro_usd: u64,
        rpm_limit: u32,
        tpm_limit: u64,
    ): usize? {
        if (name.len == 0 || _.count >= _.endpoints.len) {
            return null;
        }
        let i: usize = 0;
        while (i < _.count) {
            if (_.endpoints[i].id == id) {
                return null;
            }
            i += 1;
        }

        const slot = _.count;
        _.endpoints[slot] = ModelEndpoint {
            id: id,
            name: name,
            priority: priority,
            cost_per_1m_micro_usd: cost_per_1m_micro_usd,
            rpm_limit: rpm_limit,
            tpm_limit: tpm_limit,
            window_start_ms: 0,
            window_reqs: 0,
            window_tokens: 0,
            cooldown_until_ms: 0,
            consecutive_errors: 0,
            ewma_latency_ms: 0.0,
            enabled: true,
        };
        _.count += 1;
        return slot;
    }

    /// Enables or disables an endpoint slot. Returns `false` if `slot` is out of bounds.
    fn setEnabled(*_, slot: usize, enabled: bool): bool {
        if (slot >= _.count) {
            return false;
        }
        _.endpoints[slot].enabled = enabled;
        return true;
    }

    /// Returns a snapshot copy of endpoint at `slot`, or `null` if out of bounds.
    fn endpointAt(slot: usize): ModelEndpoint? {
        if (slot >= _.count) {
            return null;
        }
        return _.endpoints[slot];
    }

    /// Selects the optimal eligible endpoint according to `policy` at `now_ms`, automatically
    /// resetting expired 60s windows and atomically pre-deducting 1 request and `est_tokens`.
    /// Returns `null` if no endpoint is eligible.
    fn selectEndpoint(*_, policy: ModelRoutePolicy, est_tokens: u64, now_ms: i64): usize? {
        let found: bool = false;
        let best_slot: usize = 0;

        let i: usize = 0;
        while (i < _.count) {
            if (windowExpired(_.endpoints[i], now_ms)) {
                _.endpoints[i].window_start_ms = now_ms;
                _.endpoints[i].window_reqs = 0;
                _.endpoints[i].window_tokens = 0;
            }
            if (endpointFitsBudget(_.endpoints[i], est_tokens, now_ms)) {
                if (!found || isBetterEndpoint(_.endpoints[i], _.endpoints[best_slot], policy)) {
                    found = true;
                    best_slot = i;
                }
            }
            i += 1;
        }

        if (!found) {
            return null;
        }

        if (_.endpoints[best_slot].window_reqs == 0 && _.endpoints[best_slot].window_tokens == 0) {
            _.endpoints[best_slot].window_start_ms = now_ms;
        }
        _.endpoints[best_slot].window_reqs = satAddU32(_.endpoints[best_slot].window_reqs, 1);
        _.endpoints[best_slot].window_tokens = satAddU64(_.endpoints[best_slot].window_tokens, est_tokens);
        return best_slot;
    }

    /// Records a successful completion on `slot`: resets `consecutive_errors`, updates `ewma_latency_ms`,
    /// and reconciles `window_tokens` by `actual_tokens_delta` (`actual - est_tokens`).
    /// Returns `false` if `slot` is out of bounds or `latency_ms` is `NaN` or negative.
    fn recordSuccess(*_, slot: usize, latency_ms: f64, actual_tokens_delta: i64): bool {
        if (slot >= _.count || isNaN(latency_ms) || latency_ms < 0.0) {
            return false;
        }

        _.endpoints[slot].consecutive_errors = 0;
        if (_.endpoints[slot].ewma_latency_ms <= 0.0) {
            _.endpoints[slot].ewma_latency_ms = latency_ms;
        } else {
            _.endpoints[slot].ewma_latency_ms =
                EWMA_ALPHA * latency_ms + (1.0 - EWMA_ALPHA) * _.endpoints[slot].ewma_latency_ms;
        }

        if (actual_tokens_delta > 0) {
            _.endpoints[slot].window_tokens =
                satAddU64(_.endpoints[slot].window_tokens, actual_tokens_delta as u64);
        } else if (actual_tokens_delta < 0) {
            const min_i64: i64 = -9223372036854775807 - 1;
            const mag: u64 = if (actual_tokens_delta == min_i64)
                9223372036854775808
            else
                ((-actual_tokens_delta) as u64);
            if (mag >= _.endpoints[slot].window_tokens) {
                _.endpoints[slot].window_tokens = 0;
            } else {
                _.endpoints[slot].window_tokens -= mag;
            }
        }
        return true;
    }

    /// Records a 429 rate-limit or 5xx upstream failure on `slot`.
    /// Increments `consecutive_errors` and sets `cooldown_until_ms`:
    /// - If `retry_after_ms > 0`, strictly honors `retry_after_ms`.
    /// - Otherwise applies exponential backoff `1000 * 2^(errors - 1)` capped at `60000` ms.
    /// Returns the new `cooldown_until_ms`, or `null` if `slot` is out of bounds.
    fn recordRateLimitOrFailure(*_, slot: usize, now_ms: i64, retry_after_ms: i64): i64? {
        if (slot >= _.count) {
            return null;
        }

        const errs = satAddU32(_.endpoints[slot].consecutive_errors, 1);
        _.endpoints[slot].consecutive_errors = errs;

        let wait_ms: i64 = 0;
        if (retry_after_ms > 0) {
            wait_ms = retry_after_ms;
        } else {
            const exp_pow: u32 = if (errs > 7) 6 else (errs - 1);
            let backoff: i64 = BASE_BACKOFF_MS;
            let s: u32 = 0;
            while (s < exp_pow) {
                backoff *= 2;
                if (backoff >= MAX_BACKOFF_MS) {
                    backoff = MAX_BACKOFF_MS;
                    break;
                }
                s += 1;
            }
            wait_ms = backoff;
        }

        const until = satAddI64(now_ms, wait_ms);
        _.endpoints[slot].cooldown_until_ms = until;
        return until;
    }

    /// Counts how many endpoints are currently eligible to serve a request of `est_tokens` at `now_ms`.
    fn availableCount(est_tokens: u64, now_ms: i64): usize {
        let avail: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (endpointFitsBudget(_.endpoints[i], est_tokens, now_ms)) {
                avail += 1;
            }
            i += 1;
        }
        return avail;
    }
}
