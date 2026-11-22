// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Multi-Model LLM Provider RPM/TPM & 429 Failover Pool) | Created: 2026-11-22
// Pure CP zero-heap-allocation Multi-Provider LLM Gateway Pool with Dual RPM/TPM Quotas & 429 Failover:
//   - Fixed inline 16-endpoint pool (`PROVIDER_POOL_MAX_ENDPOINTS`) with 0 dynamic heap allocation
//   - Deterministic selection by `(priority asc, cost_per_1k_micro_usd asc, slot_idx asc)`
//   - Dual-dimension RPM + TPM window quota enforcement with automatic window refresh
//   - Automatic failover (`used_failover = true`) on primary RPM/TPM exhaustion or 429 Retry-After cooldown
//   - Post-response token reconciliation (`reconcile_actual_tokens`) with underflow/overflow guards
//   - Exact `earliest_retry_ms` calculation when all enabled providers are temporarily unavailable

// ============================================================================
// 1. Constants, Enums & Public Models
// ============================================================================

/// Maximum registered LLM provider endpoints in `LlmProviderPool` (zero dynamic heap).
pub const PROVIDER_POOL_MAX_ENDPOINTS: usize = 16;

/// Maximum byte length of a provider endpoint identifier.
pub const PROVIDER_ID_MAX_LEN: usize = 48;

/// Effective runtime availability status of a registered provider endpoint.
pub enum ProviderStatus {
    available,
    rate_limited,
    cooldown_429,
    disabled,
}

/// Static specification for registering an LLM provider endpoint.
pub struct ProviderEndpointSpec {
    id_buf: [PROVIDER_ID_MAX_LEN]u8 = [0; PROVIDER_ID_MAX_LEN];
    id_len: usize = 0;
    priority: u16 = 0;
    rpm_limit: u32 = 60;
    tpm_limit: u64 = 100_000;
    window_ms: u64 = 60_000;
    cost_per_1k_micro_usd: u32 = 1_000;

    /// Constructs a validated `ProviderEndpointSpec` from a byte slice identifier.
    /// Returns `null` if `id` is empty, exceeds `PROVIDER_ID_MAX_LEN`, or any limit/window is 0.
    static fn create(
        id: []const u8,
        priority: u16,
        rpm_limit: u32,
        tpm_limit: u64,
        window_ms: u64,
        cost_per_1k_micro_usd: u32,
    ): ProviderEndpointSpec? {
        if (id.len == 0 || id.len > PROVIDER_ID_MAX_LEN) return null;
        if (rpm_limit == 0 || tpm_limit == 0 || window_ms == 0) return null;

        let spec = ProviderEndpointSpec{
            id_buf: [0; PROVIDER_ID_MAX_LEN],
            id_len: id.len,
            priority: priority,
            rpm_limit: rpm_limit,
            tpm_limit: tpm_limit,
            window_ms: window_ms,
            cost_per_1k_micro_usd: cost_per_1k_micro_usd,
        };
        let i: usize = 0;
        while (i < id.len) : (i += 1) {
            spec.id_buf[i] = id[i];
        }
        return spec;
    }

    /// Returns the endpoint identifier as a read-only byte slice.
    fn idSlice(): []const u8 {
        return _.id_buf[0.._.id_len];
    }

    /// Validates all identifier and quota/window invariants.
    fn isValid(): bool {
        return _.id_len > 0 &&
            _.id_len <= PROVIDER_ID_MAX_LEN &&
            _.rpm_limit > 0 &&
            _.tpm_limit > 0 &&
            _.window_ms > 0;
    }
}

/// Outcome returned by `LlmProviderPool.acquire`.
pub struct ProviderLeaseResult {
    acquired: bool = false;
    slot_idx: usize = 0;
    id_buf: [PROVIDER_ID_MAX_LEN]u8 = [0; PROVIDER_ID_MAX_LEN];
    id_len: usize = 0;
    used_failover: bool = false;
    earliest_retry_ms: u64 = 0;

    /// Returns the leased endpoint identifier slice (empty when `acquired == false`).
    fn idSlice(): []const u8 {
        return _.id_buf[0.._.id_len];
    }
}

/// Telemetry snapshot returned by `LlmProviderPool.stats(now_ms)`.
pub struct ProviderPoolStats {
    total_endpoints: usize = 0;
    available_endpoints: usize = 0;
    cooldown_endpoints: usize = 0;
    total_leases: u64 = 0;
    total_failovers: u64 = 0;
    total_rejections: u64 = 0;
}

/// Internal runtime state per registered provider slot.
struct ProviderSlot {
    spec: ProviderEndpointSpec = ProviderEndpointSpec{};
    enabled: bool = true;
    window_start_ms: u64 = 0;
    window_initialized: bool = false;
    window_requests: u32 = 0;
    window_tokens: u64 = 0;
    cooldown_until_ms: u64 = 0;
    occupied: bool = false;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// 2. Zero-Alloc Multi-Model LLM Provider Pool
// ============================================================================

/// Zero-heap-allocation multi-provider LLM gateway pool with dual RPM/TPM quotas,
/// 429 Retry-After cooldown isolation, and deterministic priority/cost failover.
pub struct LlmProviderPool {
    slots: [PROVIDER_POOL_MAX_ENDPOINTS]ProviderSlot = [ProviderSlot{}; PROVIDER_POOL_MAX_ENDPOINTS];
    count: usize = 0;
    total_leases: u64 = 0;
    total_failovers: u64 = 0;
    total_rejections: u64 = 0;

    /// Initializes an empty `LlmProviderPool`.
    static fn init(): LlmProviderPool {
        return LlmProviderPool{
            slots: [ProviderSlot{}; PROVIDER_POOL_MAX_ENDPOINTS],
            count: 0,
            total_leases: 0,
            total_failovers: 0,
            total_rejections: 0,
        };
    }

    /// Registers a new provider endpoint.
    /// Rejects invalid specs, duplicate IDs, or pool overflow beyond 16 endpoints.
    fn register_endpoint(*_, spec: ProviderEndpointSpec): bool {
        if (!spec.isValid()) return false;
        if (_.count >= PROVIDER_POOL_MAX_ENDPOINTS) return false;

        const new_id: []const u8 = spec.idSlice();
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.slots[i].occupied && bytesEqual(_.slots[i].spec.idSlice(), new_id)) {
                return false;
            }
        }

        _.slots[_.count] = ProviderSlot{
            spec: spec,
            enabled: true,
            window_start_ms: 0,
            window_initialized: false,
            window_requests: 0,
            window_tokens: 0,
            cooldown_until_ms: 0,
            occupied: true,
        };
        _.count += 1;
        return true;
    }

    /// Internal helper: advances expired rate window and expired 429 cooldown for slot `idx`.
    fn advanceSlot(*_, idx: usize, now_ms: u64) {
        if (!_.slots[idx].window_initialized) {
            _.slots[idx].window_start_ms = now_ms;
            _.slots[idx].window_initialized = true;
            _.slots[idx].window_requests = 0;
            _.slots[idx].window_tokens = 0;
        } else if (now_ms >= _.slots[idx].window_start_ms &&
            (now_ms - _.slots[idx].window_start_ms) >= _.slots[idx].spec.window_ms)
        {
            _.slots[idx].window_start_ms = now_ms;
            _.slots[idx].window_requests = 0;
            _.slots[idx].window_tokens = 0;
        }

        if (_.slots[idx].cooldown_until_ms > 0 && now_ms >= _.slots[idx].cooldown_until_ms) {
            _.slots[idx].cooldown_until_ms = 0;
        }
    }

    /// Internal helper: strict ranking `(priority asc, cost_per_1k_micro_usd asc, slot_idx asc)`.
    fn slotBetter(a_idx: usize, b_idx: usize): bool {
        const a_prio: u16 = _.slots[a_idx].spec.priority;
        const b_prio: u16 = _.slots[b_idx].spec.priority;
        if (a_prio != b_prio) return a_prio < b_prio;

        const a_cost: u32 = _.slots[a_idx].spec.cost_per_1k_micro_usd;
        const b_cost: u32 = _.slots[b_idx].spec.cost_per_1k_micro_usd;
        if (a_cost != b_cost) return a_cost < b_cost;

        return a_idx < b_idx;
    }

    /// Acquires a lease from the highest-ranked available provider endpoint that satisfies
    /// both RPM (`window_requests + 1 <= rpm_limit`) and TPM (`window_tokens + estimated_tokens <= tpm_limit`)
    /// quotas and is not in 429 cooldown.
    /// - Automatically refreshes expired rate windows and expired 429 cooldowns.
    /// - Sets `used_failover = true` when the top-ranked enabled endpoint was bypassed.
    /// - When all enabled endpoints are unavailable, returns `acquired = false` with the exact
    ///   `earliest_retry_ms` until any capable endpoint becomes ready.
    fn acquire(*_, estimated_tokens: u64, now_ms: u64): ProviderLeaseResult {
        // 1. Advance windows and cooldowns across all registered endpoints
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.slots[i].occupied) {
                _.advanceSlot(i, now_ms);
            }
        }

        // 2. Identify the primary (best-ranked enabled) and best eligible endpoints
        let primary_idx: usize = PROVIDER_POOL_MAX_ENDPOINTS;
        let best_idx: usize = PROVIDER_POOL_MAX_ENDPOINTS;

        let j: usize = 0;
        while (j < _.count) : (j += 1) {
            if (!_.slots[j].occupied || !_.slots[j].enabled) continue;

            if (primary_idx == PROVIDER_POOL_MAX_ENDPOINTS || _.slotBetter(j, primary_idx)) {
                primary_idx = j;
            }

            const in_cooldown: bool = _.slots[j].cooldown_until_ms > now_ms;
            const rpm_ok: bool = _.slots[j].window_requests < _.slots[j].spec.rpm_limit;
            const tpm_ok: bool = (estimated_tokens <= _.slots[j].spec.tpm_limit) &&
                (_.slots[j].window_tokens <= (_.slots[j].spec.tpm_limit - estimated_tokens));

            if (!in_cooldown && rpm_ok && tpm_ok) {
                if (best_idx == PROVIDER_POOL_MAX_ENDPOINTS || _.slotBetter(j, best_idx)) {
                    best_idx = j;
                }
            }
        }

        // 3. Lease acquired
        if (best_idx < PROVIDER_POOL_MAX_ENDPOINTS) {
            _.slots[best_idx].window_requests += 1;
            _.slots[best_idx].window_tokens += estimated_tokens;

            const is_failover: bool = (best_idx != primary_idx);
            _.total_leases += 1;
            if (is_failover) {
                _.total_failovers += 1;
            }

            let res = ProviderLeaseResult{
                acquired: true,
                slot_idx: best_idx,
                id_buf: [0; PROVIDER_ID_MAX_LEN],
                id_len: _.slots[best_idx].spec.id_len,
                used_failover: is_failover,
                earliest_retry_ms: 0,
            };
            let k: usize = 0;
            while (k < res.id_len) : (k += 1) {
                res.id_buf[k] = _.slots[best_idx].spec.id_buf[k];
            }
            return res;
        }

        // 4. All enabled endpoints unavailable: compute exact earliest_retry_ms
        _.total_rejections += 1;
        let found_retry: bool = false;
        let best_wait_ms: u64 = 0;

        let m: usize = 0;
        while (m < _.count) : (m += 1) {
            if (!_.slots[m].occupied || !_.slots[m].enabled) continue;
            // Skip endpoints whose total TPM limit can never fit estimated_tokens
            if (estimated_tokens > _.slots[m].spec.tpm_limit) continue;

            let ready_at: u64 = now_ms;
            if (_.slots[m].cooldown_until_ms > ready_at) {
                ready_at = _.slots[m].cooldown_until_ms;
            }

            const rpm_exhausted: bool = _.slots[m].window_requests >= _.slots[m].spec.rpm_limit;
            const tpm_exhausted: bool = _.slots[m].window_tokens > (_.slots[m].spec.tpm_limit - estimated_tokens);
            if (rpm_exhausted || tpm_exhausted) {
                const win_end: u64 = _.slots[m].window_start_ms + _.slots[m].spec.window_ms;
                if (win_end > ready_at) {
                    ready_at = win_end;
                }
            }

            const wait_ms: u64 = if (ready_at > now_ms) (ready_at - now_ms) else 0;
            if (!found_retry || wait_ms < best_wait_ms) {
                found_retry = true;
                best_wait_ms = wait_ms;
            }
        }

        return ProviderLeaseResult{
            acquired: false,
            slot_idx: 0,
            id_buf: [0; PROVIDER_ID_MAX_LEN],
            id_len: 0,
            used_failover: false,
            earliest_retry_ms: best_wait_ms,
        };
    }

    /// Reconciles pre-deducted `estimated_tokens` against `actual_tokens` after an LLM response completes.
    /// Refunds excess reservation (clamped at 0 against underflow) or charges additional tokens.
    fn reconcile_actual_tokens(
        *_,
        slot_idx: usize,
        estimated_tokens: u64,
        actual_tokens: u64,
    ): bool {
        if (slot_idx >= _.count || !_.slots[slot_idx].occupied) return false;

        if (actual_tokens >= estimated_tokens) {
            const extra: u64 = actual_tokens - estimated_tokens;
            if (_.slots[slot_idx].window_tokens <= 0xFFFFFFFFFFFFFFFF - extra) {
                _.slots[slot_idx].window_tokens += extra;
            } else {
                _.slots[slot_idx].window_tokens = 0xFFFFFFFFFFFFFFFF;
            }
        } else {
            const refund: u64 = estimated_tokens - actual_tokens;
            if (_.slots[slot_idx].window_tokens >= refund) {
                _.slots[slot_idx].window_tokens -= refund;
            } else {
                _.slots[slot_idx].window_tokens = 0;
            }
        }
        return true;
    }

    /// Places `slot_idx` into 429 cooldown until `now_ms + retry_after_ms`.
    fn report_rate_limit_429(
        *_,
        slot_idx: usize,
        retry_after_ms: u64,
        now_ms: u64,
    ): bool {
        if (slot_idx >= _.count || !_.slots[slot_idx].occupied) return false;

        const until_ms: u64 = if (now_ms <= 0xFFFFFFFFFFFFFFFF - retry_after_ms)
            (now_ms + retry_after_ms)
        else
            0xFFFFFFFFFFFFFFFF;

        if (until_ms > _.slots[slot_idx].cooldown_until_ms) {
            _.slots[slot_idx].cooldown_until_ms = until_ms;
        }
        return true;
    }

    /// Enables or disables (drains) a registered endpoint slot.
    fn set_enabled(*_, slot_idx: usize, enabled: bool): bool {
        if (slot_idx >= _.count || !_.slots[slot_idx].occupied) return false;
        _.slots[slot_idx].enabled = enabled;
        return true;
    }

    /// Queries the effective `ProviderStatus` of `slot_idx` at `now_ms`.
    fn endpoint_status(slot_idx: usize, now_ms: u64): ProviderStatus {
        if (slot_idx >= _.count || !_.slots[slot_idx].occupied || !_.slots[slot_idx].enabled) {
            return ProviderStatus.disabled;
        }
        if (_.slots[slot_idx].cooldown_until_ms > now_ms) {
            return ProviderStatus.cooldown_429;
        }
        const window_expired: bool = _.slots[slot_idx].window_initialized &&
            now_ms >= _.slots[slot_idx].window_start_ms &&
            (now_ms - _.slots[slot_idx].window_start_ms) >= _.slots[slot_idx].spec.window_ms;
        if (!window_expired && _.slots[slot_idx].window_initialized) {
            if (_.slots[slot_idx].window_requests >= _.slots[slot_idx].spec.rpm_limit ||
                _.slots[slot_idx].window_tokens >= _.slots[slot_idx].spec.tpm_limit)
            {
                return ProviderStatus.rate_limited;
            }
        }
        return ProviderStatus.available;
    }

    /// Returns a pool-wide availability & telemetry snapshot at `now_ms`.
    fn stats(now_ms: u64): ProviderPoolStats {
        let avail: usize = 0;
        let cool: usize = 0;

        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const st = _.endpoint_status(i, now_ms);
            if (st == ProviderStatus.available) {
                avail += 1;
            } else if (st == ProviderStatus.cooldown_429) {
                cool += 1;
            }
        }

        return ProviderPoolStats{
            total_endpoints: _.count,
            available_endpoints: avail,
            cooldown_endpoints: cool,
            total_leases: _.total_leases,
            total_failovers: _.total_failovers,
            total_rejections: _.total_rejections,
        };
    }
}
