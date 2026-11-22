// Origin: Pure-CP Stdlib (AI Multi-Tenant Dual TPM/RPM Sliding-Window Quota Limiter) | Created: 2026-11-22
// Zero-heap-allocation multi-tenant LLM dual-dimension (RPM + TPM) sliding-window quota limiter
// with concurrency gating, post-response token reconciliation, and failed-request rollback.

/// Admission or rejection outcome returned by `LlmQuotaLimiter.tryAcquire`.
pub enum QuotaRejectReason {
    Allowed,
    RpmExceeded,
    TpmExceeded,
    ConcurrencyExceeded,
    TenantNotFound,
    InvalidRequest,
}

/// Per-tenant rate and concurrency limits. A value of `0` for `rpm_limit`, `tpm_limit`,
/// or `max_concurrent` disables that specific dimension.
pub struct TenantQuotaConfig {
    rpm_limit: u32 = 60;
    tpm_limit: u64 = 100000;
    max_concurrent: u32 = 10;
    window_ms: u64 = 60000;
}

/// Decision snapshot returned by `LlmQuotaLimiter.tryAcquire`.
pub struct QuotaDecision {
    reason: QuotaRejectReason = QuotaRejectReason.Allowed;
    retry_after_ms: u64 = 0;
    remaining_rpm: u32 = 0;
    remaining_tpm: u64 = 0;
    active_concurrent: u32 = 0;
}

/// Cumulative per-tenant admission and reconciliation telemetry.
pub struct TenantQuotaStats {
    tenant_id: u64 = 0;
    active_concurrent: u32 = 0;
    total_allowed: u64 = 0;
    total_rejected_rpm: u64 = 0;
    total_rejected_tpm: u64 = 0;
    total_rejected_concurrency: u64 = 0;
    total_reconciled: u64 = 0;
    total_aborted: u64 = 0;
    total_actual_tokens: u64 = 0;
}

/// Fixed-slot dual-window sliding counter state for a single tenant.
pub struct TenantQuotaSlot {
    occupied: bool = false;
    tenant_id: u64 = 0;
    config: TenantQuotaConfig = TenantQuotaConfig{};
    window_start_ms: u64 = 0;
    window_initialized: bool = false;
    curr_requests: u64 = 0;
    prev_requests: u64 = 0;
    curr_tokens: u64 = 0;
    prev_tokens: u64 = 0;
    in_flight: u32 = 0;
    stats: TenantQuotaStats = TenantQuotaStats{};
}

fn clampNow(now_ms: i64): u64 {
    return if (now_ms < 0) 0 else u64(now_ms);
}

fn weightedPrev(prev_val: u64, remaining_weight_ms: u64, window_ms: u64): u64 {
    if (prev_val == 0 || remaining_weight_ms == 0 || window_ms == 0) return 0;
    const prod: u128 = u128(prev_val) * u128(remaining_weight_ms);
    return u64(prod / u128(window_ms));
}

fn advanceTenantWindow(slot: *TenantQuotaSlot, safe_now: u64): u64 {
    const w = slot.config.window_ms;
    if (!slot.window_initialized) {
        slot.window_start_ms = safe_now;
        slot.window_initialized = true;
        slot.curr_requests = 0;
        slot.prev_requests = 0;
        slot.curr_tokens = 0;
        slot.prev_tokens = 0;
        return 0;
    }
    if (safe_now <= slot.window_start_ms) {
        return 0;
    }
    const elapsed = safe_now - slot.window_start_ms;
    if (elapsed >= w * 2) {
        slot.prev_requests = 0;
        slot.prev_tokens = 0;
        slot.curr_requests = 0;
        slot.curr_tokens = 0;
        slot.window_start_ms = safe_now;
        return 0;
    }
    if (elapsed >= w) {
        slot.prev_requests = slot.curr_requests;
        slot.prev_tokens = slot.curr_tokens;
        slot.curr_requests = 0;
        slot.curr_tokens = 0;
        slot.window_start_ms += w;
        return safe_now - slot.window_start_ms;
    }
    return elapsed;
}

/// Computes minimum milliseconds to wait until `curr + need + prev * (W - elapsed') / W <= limit`.
fn computeSlidingRetryAfter(
    curr: u64,
    prev: u64,
    need: u64,
    limit: u64,
    elapsed: u64,
    window_ms: u64,
): u64 {
    const rem_window = if (elapsed < window_ms) window_ms - elapsed else 1;
    if (need > limit) {
        return window_ms;
    }
    if (curr + need > limit) {
        return if (rem_window > 0) rem_window else 1;
    }
    const allowed_prev = limit - (curr + need);
    if (prev == 0) return 1;
    // Need: prev * rem_target / W <= allowed_prev  =>  rem_target = floor(allowed_prev * W / prev)
    const rem_target = u64((u128(allowed_prev) * u128(window_ms)) / u128(prev));
    if (rem_window <= rem_target) return 1;
    const wait_ms = rem_window - rem_target;
    return if (wait_ms > 0) wait_ms else 1;
}

/// Zero-heap-allocation multi-tenant LLM quota limiter backed by a caller-owned slot slice.
pub struct LlmQuotaLimiter {
    slots: []TenantQuotaSlot;
    count: usize = 0;

    /// Initializes a quota limiter backed by caller-owned `slots`.
    static fn init(slots: *[]TenantQuotaSlot): LlmQuotaLimiter {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = TenantQuotaSlot{};
        }
        return LlmQuotaLimiter{
            slots: slots,
            count: 0,
        };
    }

    /// Returns the number of registered tenants.
    fn tenantCount(): usize {
        return _.count;
    }

    fn findTenantIndex(tenant_id: u64): usize? {
        if (tenant_id == 0) return null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].tenant_id == tenant_id) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new tenant with `config`. Rejects `tenant_id == 0`, `window_ms == 0`,
    /// duplicate `tenant_id`, or full slot pool.
    fn registerTenant(*_, tenant_id: u64, config: TenantQuotaConfig): bool {
        if (tenant_id == 0 || config.window_ms == 0) return false;
        if (_.findTenantIndex(tenant_id) != null) return false;

        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) {
                _.slots[i] = TenantQuotaSlot{
                    occupied: true,
                    tenant_id: tenant_id,
                    config: config,
                    window_start_ms: 0,
                    window_initialized: false,
                    curr_requests: 0,
                    prev_requests: 0,
                    curr_tokens: 0,
                    prev_tokens: 0,
                    in_flight: 0,
                    stats: TenantQuotaStats{ tenant_id: tenant_id },
                };
                _.count += 1;
                return true;
            }
        }
        return false;
    }

    /// Updates quota configuration for an existing tenant. Returns `false` if absent or invalid.
    fn updateTenantConfig(*_, tenant_id: u64, config: TenantQuotaConfig): bool {
        if (config.window_ms == 0) return false;
        if (idx := _.findTenantIndex(tenant_id)) {
            _.slots[idx].config = config;
            return true;
        }
        return false;
    }

    /// Attempts to acquire concurrency, RPM, and estimated TPM quota for `tenant_id` at `now_ms`.
    fn tryAcquire(*_, tenant_id: u64, estimated_tokens: u64, now_ms: i64): QuotaDecision {
        if (tenant_id == 0) {
            return QuotaDecision{ reason: QuotaRejectReason.InvalidRequest };
        }
        const idx_opt = _.findTenantIndex(tenant_id);
        if (idx_opt == null) {
            return QuotaDecision{ reason: QuotaRejectReason.TenantNotFound };
        }
        const idx = idx_opt ?? 0;
        const safe_now = clampNow(now_ms);
        const elapsed = advanceTenantWindow(*_.slots[idx], safe_now);

        const cfg = _.slots[idx].config;
        const w = cfg.window_ms;
        const rem_weight = if (elapsed < w) w - elapsed else 0;
        const eff_req = _.slots[idx].curr_requests + weightedPrev(_.slots[idx].prev_requests, rem_weight, w);
        const eff_tok = _.slots[idx].curr_tokens + weightedPrev(_.slots[idx].prev_tokens, rem_weight, w);

        const cur_rem_rpm: u32 = if (cfg.rpm_limit == 0)
            0
        else if (eff_req >= u64(cfg.rpm_limit))
            0
        else
            u32(u64(cfg.rpm_limit) - eff_req);

        const cur_rem_tpm: u64 = if (cfg.tpm_limit == 0)
            0
        else if (eff_tok >= cfg.tpm_limit)
            0
        else
            cfg.tpm_limit - eff_tok;

        // 1. Concurrency gate
        if (cfg.max_concurrent > 0 && _.slots[idx].in_flight >= cfg.max_concurrent) {
            _.slots[idx].stats.total_rejected_concurrency += 1;
            return QuotaDecision{
                reason: QuotaRejectReason.ConcurrencyExceeded,
                retry_after_ms: 100,
                remaining_rpm: cur_rem_rpm,
                remaining_tpm: cur_rem_tpm,
                active_concurrent: _.slots[idx].in_flight,
            };
        }

        // 2. RPM sliding-window gate
        if (cfg.rpm_limit > 0 && eff_req + 1 > u64(cfg.rpm_limit)) {
            _.slots[idx].stats.total_rejected_rpm += 1;
            const retry_ms = computeSlidingRetryAfter(
                _.slots[idx].curr_requests,
                _.slots[idx].prev_requests,
                1,
                u64(cfg.rpm_limit),
                elapsed,
                w,
            );
            return QuotaDecision{
                reason: QuotaRejectReason.RpmExceeded,
                retry_after_ms: retry_ms,
                remaining_rpm: 0,
                remaining_tpm: cur_rem_tpm,
                active_concurrent: _.slots[idx].in_flight,
            };
        }

        // 3. TPM sliding-window gate
        if (cfg.tpm_limit > 0 && eff_tok + estimated_tokens > cfg.tpm_limit) {
            _.slots[idx].stats.total_rejected_tpm += 1;
            const retry_ms = computeSlidingRetryAfter(
                _.slots[idx].curr_tokens,
                _.slots[idx].prev_tokens,
                estimated_tokens,
                cfg.tpm_limit,
                elapsed,
                w,
            );
            return QuotaDecision{
                reason: QuotaRejectReason.TpmExceeded,
                retry_after_ms: retry_ms,
                remaining_rpm: cur_rem_rpm,
                remaining_tpm: cur_rem_tpm,
                active_concurrent: _.slots[idx].in_flight,
            };
        }

        // Reserve quota & concurrency
        _.slots[idx].curr_requests += 1;
        _.slots[idx].curr_tokens += estimated_tokens;
        _.slots[idx].in_flight += 1;
        _.slots[idx].stats.total_allowed += 1;
        _.slots[idx].stats.active_concurrent = _.slots[idx].in_flight;

        const new_eff_req = eff_req + 1;
        const new_eff_tok = eff_tok + estimated_tokens;
        const post_rem_rpm: u32 = if (cfg.rpm_limit == 0)
            0
        else if (new_eff_req >= u64(cfg.rpm_limit))
            0
        else
            u32(u64(cfg.rpm_limit) - new_eff_req);
        const post_rem_tpm: u64 = if (cfg.tpm_limit == 0)
            0
        else if (new_eff_tok >= cfg.tpm_limit)
            0
        else
            cfg.tpm_limit - new_eff_tok;

        return QuotaDecision{
            reason: QuotaRejectReason.Allowed,
            retry_after_ms: 0,
            remaining_rpm: post_rem_rpm,
            remaining_tpm: post_rem_tpm,
            active_concurrent: _.slots[idx].in_flight,
        };
    }

    /// Reconciles actual LLM token usage against the earlier reservation (refunding surplus
    /// or charging deficit) and releases one in-flight concurrency slot.
    fn reconcileTokens(
        *_,
        tenant_id: u64,
        estimated_tokens: u64,
        actual_tokens: u64,
        now_ms: i64,
    ): bool {
        if (idx := _.findTenantIndex(tenant_id)) {
            const safe_now = clampNow(now_ms);
            * = advanceTenantWindow(*_.slots[idx], safe_now);

            if (_.slots[idx].in_flight > 0) {
                _.slots[idx].in_flight -= 1;
            }
            if (actual_tokens > estimated_tokens) {
                _.slots[idx].curr_tokens += actual_tokens - estimated_tokens;
            } else if (actual_tokens < estimated_tokens) {
                let refund = estimated_tokens - actual_tokens;
                if (_.slots[idx].curr_tokens >= refund) {
                    _.slots[idx].curr_tokens -= refund;
                } else {
                    refund -= _.slots[idx].curr_tokens;
                    _.slots[idx].curr_tokens = 0;
                    if (_.slots[idx].prev_tokens >= refund) {
                        _.slots[idx].prev_tokens -= refund;
                    } else {
                        _.slots[idx].prev_tokens = 0;
                    }
                }
            }
            _.slots[idx].stats.total_reconciled += 1;
            _.slots[idx].stats.total_actual_tokens += actual_tokens;
            _.slots[idx].stats.active_concurrent = _.slots[idx].in_flight;
            return true;
        }
        return false;
    }

    /// Rolls back a failed/cancelled request reservation (concurrency, 1 RPM, and `estimated_tokens`).
    fn abortRequest(*_, tenant_id: u64, estimated_tokens: u64, now_ms: i64): bool {
        if (idx := _.findTenantIndex(tenant_id)) {
            const safe_now = clampNow(now_ms);
            * = advanceTenantWindow(*_.slots[idx], safe_now);

            if (_.slots[idx].in_flight > 0) {
                _.slots[idx].in_flight -= 1;
            }
            if (_.slots[idx].curr_requests > 0) {
                _.slots[idx].curr_requests -= 1;
            } else if (_.slots[idx].prev_requests > 0) {
                _.slots[idx].prev_requests -= 1;
            }

            let refund = estimated_tokens;
            if (_.slots[idx].curr_tokens >= refund) {
                _.slots[idx].curr_tokens -= refund;
            } else {
                refund -= _.slots[idx].curr_tokens;
                _.slots[idx].curr_tokens = 0;
                if (_.slots[idx].prev_tokens >= refund) {
                    _.slots[idx].prev_tokens -= refund;
                } else {
                    _.slots[idx].prev_tokens = 0;
                }
            }

            _.slots[idx].stats.total_aborted += 1;
            _.slots[idx].stats.active_concurrent = _.slots[idx].in_flight;
            return true;
        }
        return false;
    }

    /// Returns a snapshot of cumulative statistics for `tenant_id`, or `null` if not registered.
    fn getStats(tenant_id: u64): TenantQuotaStats? {
        if (idx := _.findTenantIndex(tenant_id)) {
            return _.slots[idx].stats;
        }
        return null;
    }
}
