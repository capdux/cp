// Origin: Pure-CP Stdlib Test Suite (AI Multi-Tenant Dual TPM/RPM Sliding-Window Quota Limiter) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/quota_limiter.cp
// Covers:
//   1. Multi-tenant isolation, independent RPM/TPM rejection & registration guards
//   2. Concurrency cap gating, release on reconciliation & rejection telemetry
//   3. Dual-window weighted sliding recovery & precise retry_after_ms calculation
//   4. reconcileTokens surplus refund & deficit surcharge accounting
//   5. abortRequest full quota rollback (concurrency + RPM + TPM) & backward clock-skew defense

import { test, expect_eq } from "cap:test";
import {
    QuotaRejectReason,
    TenantQuotaConfig,
    QuotaDecision,
    TenantQuotaStats,
    TenantQuotaSlot,
    LlmQuotaLimiter,
} from "../../lib/agent/index.cp";

fn testMultiTenantIsolationAndRpmTpmGates()! {
    let slots_buf: [2]TenantQuotaSlot = [TenantQuotaSlot{}; 2];
    let slots: []TenantQuotaSlot = slots_buf[0..];
    let limiter = LlmQuotaLimiter.init(slots);

    // Registration validation guards
    expect_eq(limiter.registerTenant(0, TenantQuotaConfig{}), false);
    expect_eq(limiter.registerTenant(1, TenantQuotaConfig{ window_ms: 0 }), false);

    // Tenant 1: tight RPM (2/min), generous TPM
    expect_eq(limiter.registerTenant(1, TenantQuotaConfig{
        rpm_limit: 2,
        tpm_limit: 10000,
        max_concurrent: 5,
        window_ms: 60000,
    }), true);
    // Duplicate rejected
    expect_eq(limiter.registerTenant(1, TenantQuotaConfig{}), false);

    // Tenant 2: generous RPM, tight TPM (500/min)
    expect_eq(limiter.registerTenant(2, TenantQuotaConfig{
        rpm_limit: 10,
        tpm_limit: 500,
        max_concurrent: 5,
        window_ms: 60000,
    }), true);
    // Full pool rejected
    expect_eq(limiter.registerTenant(3, TenantQuotaConfig{}), false);
    expect_eq(limiter.tenantCount(), 2);

    // Invalid & unknown tenant lookups
    expect_eq(limiter.tryAcquire(0, 10, 1000).reason == QuotaRejectReason.InvalidRequest, true);
    expect_eq(limiter.tryAcquire(99, 10, 1000).reason == QuotaRejectReason.TenantNotFound, true);

    // Tenant 1 hits RPM limit on 3rd request
    const d1 = limiter.tryAcquire(1, 100, 1000);
    expect_eq(d1.reason == QuotaRejectReason.Allowed, true);
    expect_eq(d1.remaining_rpm, 1);
    expect_eq(d1.remaining_tpm, 9900);

    const d2 = limiter.tryAcquire(1, 100, 1010);
    expect_eq(d2.reason == QuotaRejectReason.Allowed, true);
    expect_eq(d2.remaining_rpm, 0);

    const d3 = limiter.tryAcquire(1, 100, 1020);
    expect_eq(d3.reason == QuotaRejectReason.RpmExceeded, true);
    expect_eq(d3.retry_after_ms > 0, true);

    // Tenant 2 is completely isolated from Tenant 1 and enforces its own TPM limit
    const b1 = limiter.tryAcquire(2, 400, 1020);
    expect_eq(b1.reason == QuotaRejectReason.Allowed, true);
    expect_eq(b1.remaining_tpm, 100);

    const b2 = limiter.tryAcquire(2, 200, 1030);
    expect_eq(b2.reason == QuotaRejectReason.TpmExceeded, true);
    expect_eq(b2.retry_after_ms > 0, true);

    const st1 = limiter.getStats(1) ?? TenantQuotaStats{};
    const st2 = limiter.getStats(2) ?? TenantQuotaStats{};
    expect_eq(st1.total_allowed, 2);
    expect_eq(st1.total_rejected_rpm, 1);
    expect_eq(st2.total_allowed, 1);
    expect_eq(st2.total_rejected_tpm, 1);
}

fn testConcurrencyLimitAndRelease()! {
    let slots_buf: [2]TenantQuotaSlot = [TenantQuotaSlot{}; 2];
    let slots: []TenantQuotaSlot = slots_buf[0..];
    let limiter = LlmQuotaLimiter.init(slots);

    expect_eq(limiter.registerTenant(10, TenantQuotaConfig{
        rpm_limit: 100,
        tpm_limit: 100000,
        max_concurrent: 2,
        window_ms: 60000,
    }), true);

    const a1 = limiter.tryAcquire(10, 100, 500);
    expect_eq(a1.reason == QuotaRejectReason.Allowed, true);
    expect_eq(a1.active_concurrent, 1);

    const a2 = limiter.tryAcquire(10, 100, 510);
    expect_eq(a2.reason == QuotaRejectReason.Allowed, true);
    expect_eq(a2.active_concurrent, 2);

    // 3rd concurrent request blocked
    const a3 = limiter.tryAcquire(10, 100, 520);
    expect_eq(a3.reason == QuotaRejectReason.ConcurrencyExceeded, true);
    expect_eq(a3.active_concurrent, 2);
    expect_eq(a3.retry_after_ms > 0, true);

    // Reconcile releases 1 in-flight slot -> next request succeeds
    expect_eq(limiter.reconcileTokens(10, 100, 90, 600), true);
    const a4 = limiter.tryAcquire(10, 100, 610);
    expect_eq(a4.reason == QuotaRejectReason.Allowed, true);
    expect_eq(a4.active_concurrent, 2);

    const st = limiter.getStats(10) ?? TenantQuotaStats{};
    expect_eq(st.total_rejected_concurrency, 1);
    expect_eq(st.active_concurrent, 2);
}

fn testSlidingWindowSmoothRecovery()! {
    let slots_buf: [2]TenantQuotaSlot = [TenantQuotaSlot{}; 2];
    let slots: []TenantQuotaSlot = slots_buf[0..];
    let limiter = LlmQuotaLimiter.init(slots);

    expect_eq(limiter.registerTenant(7, TenantQuotaConfig{
        rpm_limit: 4,
        tpm_limit: 1000,
        max_concurrent: 0, // unlimited concurrency
        window_ms: 10000,
    }), true);

    // Fill all 4 RPM slots (800 tokens total) at t=0
    expect_eq(limiter.tryAcquire(7, 200, 0).reason == QuotaRejectReason.Allowed, true);
    expect_eq(limiter.tryAcquire(7, 200, 0).reason == QuotaRejectReason.Allowed, true);
    expect_eq(limiter.tryAcquire(7, 200, 0).reason == QuotaRejectReason.Allowed, true);
    expect_eq(limiter.tryAcquire(7, 200, 0).reason == QuotaRejectReason.Allowed, true);

    // At t=10000 (start of next window): prev_requests=4 carries 100% weight -> still blocked, retry_after_ms=2500
    const d_edge = limiter.tryAcquire(7, 100, 10000);
    expect_eq(d_edge.reason == QuotaRejectReason.RpmExceeded, true);
    expect_eq(d_edge.retry_after_ms, 2500);

    // At t=12500 (25% into new window): weighted prev_requests = floor(4 * 7500 / 10000) = 3 -> 1 slot recovered!
    const d_smooth = limiter.tryAcquire(7, 100, 12500);
    expect_eq(d_smooth.reason == QuotaRejectReason.Allowed, true);
    expect_eq(d_smooth.remaining_rpm, 0);

    // Immediate second request at t=12500 blocked again
    expect_eq(limiter.tryAcquire(7, 100, 12500).reason == QuotaRejectReason.RpmExceeded, true);

    // Jump past 2 full windows (t=35000): full quota reset
    const d_reset = limiter.tryAcquire(7, 500, 35000);
    expect_eq(d_reset.reason == QuotaRejectReason.Allowed, true);
    expect_eq(d_reset.remaining_rpm, 3);
    expect_eq(d_reset.remaining_tpm, 500);
}

fn testReconcileTokensRefundAndCharge()! {
    let slots_buf: [2]TenantQuotaSlot = [TenantQuotaSlot{}; 2];
    let slots: []TenantQuotaSlot = slots_buf[0..];
    let limiter = LlmQuotaLimiter.init(slots);

    expect_eq(limiter.registerTenant(20, TenantQuotaConfig{
        rpm_limit: 10,
        tpm_limit: 1000,
        max_concurrent: 4,
        window_ms: 60000,
    }), true);

    // 1. Over-estimated (800) vs actual (300) -> refunds 500 tokens
    expect_eq(limiter.tryAcquire(20, 800, 1000).reason == QuotaRejectReason.Allowed, true);
    // Before reconciliation, 600 would exceed 1000
    expect_eq(limiter.tryAcquire(20, 600, 1010).reason == QuotaRejectReason.TpmExceeded, true);

    expect_eq(limiter.reconcileTokens(20, 800, 300, 1020), true);
    // After refund, used=300 so 600 succeeds (remaining=100)
    const d_after_refund = limiter.tryAcquire(20, 600, 1030);
    expect_eq(d_after_refund.reason == QuotaRejectReason.Allowed, true);
    expect_eq(d_after_refund.remaining_tpm, 100);

    // 2. Under-estimated (600) vs actual (750) -> surcharges 150 (total used = 300 + 750 = 1050 > 1000)
    expect_eq(limiter.reconcileTokens(20, 600, 750, 1040), true);
    expect_eq(limiter.tryAcquire(20, 10, 1050).reason == QuotaRejectReason.TpmExceeded, true);

    const st = limiter.getStats(20) ?? TenantQuotaStats{};
    expect_eq(st.total_reconciled, 2);
    expect_eq(st.total_actual_tokens, 1050);
    expect_eq(st.active_concurrent, 0);
}

fn testAbortRequestRollbackAndClockSkew()! {
    let slots_buf: [2]TenantQuotaSlot = [TenantQuotaSlot{}; 2];
    let slots: []TenantQuotaSlot = slots_buf[0..];
    let limiter = LlmQuotaLimiter.init(slots);

    expect_eq(limiter.registerTenant(30, TenantQuotaConfig{
        rpm_limit: 1,
        tpm_limit: 500,
        max_concurrent: 1,
        window_ms: 60000,
    }), true);

    // Saturate concurrency, RPM, and TPM in a single call
    expect_eq(limiter.tryAcquire(30, 500, 5000).reason == QuotaRejectReason.Allowed, true);
    expect_eq(limiter.tryAcquire(30, 10, 5010).reason == QuotaRejectReason.ConcurrencyExceeded, true);

    // Backward clock skew never resets the active window
    expect_eq(limiter.tryAcquire(30, 10, 1000).reason == QuotaRejectReason.ConcurrencyExceeded, true);

    // Abort rolls back concurrency, RPM, and TPM completely
    expect_eq(limiter.abortRequest(30, 500, 5020), true);
    expect_eq(limiter.abortRequest(999, 500, 5020), false);

    // Immediately re-acquiring full 500 tokens succeeds
    const retry_ok = limiter.tryAcquire(30, 500, 5030);
    expect_eq(retry_ok.reason == QuotaRejectReason.Allowed, true);
    expect_eq(retry_ok.active_concurrent, 1);
    expect_eq(retry_ok.remaining_rpm, 0);
    expect_eq(retry_ok.remaining_tpm, 0);

    const st = limiter.getStats(30) ?? TenantQuotaStats{};
    expect_eq(st.total_aborted, 1);
    expect_eq(st.total_allowed, 2);
}

fn main() {
    test("1. agent quota_limiter: multi-tenant isolation & independent RPM/TPM gates", () => {
        testMultiTenantIsolationAndRpmTpmGates()?;
    });
    test("2. agent quota_limiter: concurrency cap gating & slot release", () => {
        testConcurrencyLimitAndRelease()?;
    });
    test("3. agent quota_limiter: dual-window sliding smooth recovery & retry_after_ms", () => {
        testSlidingWindowSmoothRecovery()?;
    });
    test("4. agent quota_limiter: reconcileTokens surplus refund & deficit surcharge", () => {
        testReconcileTokensRefundAndCharge()?;
    });
    test("5. agent quota_limiter: abortRequest full rollback & backward clock-skew defense", () => {
        testAbortRequestRollbackAndClockSkew()?;
    });
}
