// Origin: Pure-CP Stdlib Test Suite (lib/obs/trace_sampler.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc OpenTelemetry Tail-Based Trace Sampler:
//   1. Multi-span aggregation per TraceID, root_finished immediate flush & decision_wait_ms timeout flush
//   2. Error span 100% retention (child error latches trace) & policy precedence (error > slow > priority)
//   3. Slow latency threshold exact boundary retention (kept_slow_latency)
//   4. Priority tag retention (kept_priority_tag) on fast healthy traces
//   5. Deterministic SplitMix64 baseline probabilistic sampling & statistical convergence over 500 traces
//   6. Error storm max_kept_per_window budget circuit breaker & automatic next-window recovery
//   7. Full 128-slot oldest-first overflow eviction (evicted_overflow) & batched small-buffer flush
//   8. Invalid TailSamplerConfig rejection, zero 128-bit TraceID guard & empty output slice safety

import { test, expect_eq } from "cap:test";
import {
    TAIL_SAMPLER_MAX_TRACES,
    TailSampleReason,
    TailSamplerConfig,
    TailSampleDecision,
    TailSamplerStats,
    TailTraceSampler,
} from "cap:obs";

// ============================================================================
// Test 1: Multi-Span Aggregation, Root Flush & decision_wait_ms Timeout Flush
// ============================================================================
test("trace_sampler: multi-span aggregation, root flush & timeout flush", () => {
    expect_eq(TAIL_SAMPLER_MAX_TRACES, 128);

    let cfg = TailSamplerConfig.defaults();
    cfg.decision_wait_ms = 2_000;
    cfg.baseline_sample_bps = 10000; // keep all healthy traces for this test

    let sampler = TailTraceSampler.init(cfg) ?? return;

    // Trace A: 2 child spans arrive at t=100, t=200 (not ready yet)
    expect_eq(sampler.ingest_span(101, 1, 12_000, false, false, false, 100), true);
    expect_eq(sampler.ingest_span(101, 1, 35_000, false, false, false, 200), true);

    // Trace B: 1 child span arrives at t=150 (will never get root, must flush by timeout)
    expect_eq(sampler.ingest_span(202, 2, 18_000, false, false, false, 150), true);
    expect_eq(sampler.stats().active_traces, 2);

    let decs: [8]TailSampleDecision = [TailSampleDecision{}; 8];
    let dec_s: []TailSampleDecision = decs[0..];

    // At t=500, neither trace is ready
    expect_eq(sampler.flush_ready_decisions(500, dec_s), 0);
    expect_eq(sampler.stats().active_traces, 2);

    // Root span arrives for Trace A at t=600 -> Trace A immediately becomes ready
    expect_eq(sampler.ingest_span(101, 1, 20_000, false, false, true, 600), true);
    const n1 = sampler.flush_ready_decisions(600, dec_s);
    expect_eq(n1, 1);
    expect_eq(decs[0].trace_id_lo, 101);
    expect_eq(decs[0].span_count, 3);
    expect_eq(decs[0].max_duration_us, 35_000);
    expect_eq(decs[0].keep, true);
    expect_eq(decs[0].reason == TailSampleReason.kept_probabilistic, true);
    expect_eq(sampler.stats().active_traces, 1);

    // At t=2150 (elapsed 2150 - 150 == 2000 >= decision_wait_ms), Trace B flushes by timeout
    const n2 = sampler.flush_ready_decisions(2150, dec_s);
    expect_eq(n2, 1);
    expect_eq(decs[0].trace_id_lo, 202);
    expect_eq(decs[0].span_count, 1);
    expect_eq(decs[0].max_duration_us, 18_000);
    expect_eq(sampler.stats().active_traces, 0);
});

// ============================================================================
// Test 2: Error 100% Retention & Policy Precedence (Error > Slow > Priority)
// ============================================================================
test("trace_sampler: error retention & policy priority order", () => {
    let cfg = TailSamplerConfig.defaults();
    cfg.slow_threshold_us = 100_000;
    cfg.baseline_sample_bps = 0; // drop all baseline healthy traces

    let sampler = TailTraceSampler.init(cfg) ?? return;

    // Trace 1: child span errors, root span is fast & healthy -> kept_error
    expect_eq(sampler.ingest_span(1, 0, 5_000, true, false, false, 100), true);
    expect_eq(sampler.ingest_span(1, 0, 10_000, false, false, true, 110), true);

    // Trace 2: has error AND slow AND priority -> error takes highest precedence (kept_error)
    expect_eq(sampler.ingest_span(2, 0, 999_000, true, true, true, 120), true);

    // Trace 3: healthy, fast, non-priority -> dropped
    expect_eq(sampler.ingest_span(3, 0, 10_000, false, false, true, 130), true);

    let decs: [8]TailSampleDecision = [TailSampleDecision{}; 8];
    let dec_s: []TailSampleDecision = decs[0..];
    const n = sampler.flush_ready_decisions(200, dec_s);
    expect_eq(n, 3);

    expect_eq(decs[0].keep, true);
    expect_eq(decs[0].reason == TailSampleReason.kept_error, true);
    expect_eq(decs[1].keep, true);
    expect_eq(decs[1].reason == TailSampleReason.kept_error, true);
    expect_eq(decs[2].keep, false);
    expect_eq(decs[2].reason == TailSampleReason.dropped, true);

    const st: TailSamplerStats = sampler.stats();
    expect_eq(st.kept_error, 2);
    expect_eq(st.dropped_baseline, 1);
    expect_eq(st.total_evaluated, 3);
});

// ============================================================================
// Test 3: Slow Latency Threshold Exact Boundary Retention
// ============================================================================
test("trace_sampler: slow latency threshold exact boundary retention", () => {
    let cfg = TailSamplerConfig.defaults();
    cfg.slow_threshold_us = 250_000;
    cfg.baseline_sample_bps = 0;

    let sampler = TailTraceSampler.init(cfg) ?? return;

    // Exact boundary (250_000 us) -> kept_slow_latency (even if priority is also set, slow > priority)
    expect_eq(sampler.ingest_span(10, 0, 250_000, false, true, true, 100), true);
    // Just below boundary (249_999 us) -> dropped
    expect_eq(sampler.ingest_span(11, 0, 249_999, false, false, true, 100), true);

    let decs: [4]TailSampleDecision = [TailSampleDecision{}; 4];
    let dec_s: []TailSampleDecision = decs[0..];
    expect_eq(sampler.flush_ready_decisions(150, dec_s), 2);

    expect_eq(decs[0].keep, true);
    expect_eq(decs[0].reason == TailSampleReason.kept_slow_latency, true);
    expect_eq(decs[1].keep, false);
    expect_eq(decs[1].reason == TailSampleReason.dropped, true);

    expect_eq(sampler.stats().kept_slow, 1);
    expect_eq(sampler.stats().dropped_baseline, 1);
});

// ============================================================================
// Test 4: Priority Tag Retention on Fast Healthy Traces
// ============================================================================
test("trace_sampler: priority tag keeps fast healthy traces", () => {
    let cfg = TailSamplerConfig.defaults();
    cfg.slow_threshold_us = 500_000;
    cfg.baseline_sample_bps = 0;

    let sampler = TailTraceSampler.init(cfg) ?? return;

    // Child span carries priority tag (e.g. VIP tenant or canary debug header)
    expect_eq(sampler.ingest_span(55, 99, 2_000, false, true, false, 100), true);
    expect_eq(sampler.ingest_span(55, 99, 3_000, false, false, true, 105), true);

    let decs: [4]TailSampleDecision = [TailSampleDecision{}; 4];
    let dec_s: []TailSampleDecision = decs[0..];
    expect_eq(sampler.flush_ready_decisions(110, dec_s), 1);

    expect_eq(decs[0].keep, true);
    expect_eq(decs[0].reason == TailSampleReason.kept_priority_tag, true);
    expect_eq(sampler.stats().kept_priority, 1);
});

// ============================================================================
// Test 5: Deterministic SplitMix64 Baseline Sampling & Statistical Convergence
// ============================================================================
test("trace_sampler: deterministic SplitMix64 baseline sampling convergence", () => {
    let cfg = TailSamplerConfig.defaults();
    cfg.slow_threshold_us = 1_000_000;
    cfg.baseline_sample_bps = 2500; // 25.00%
    cfg.max_kept_per_window = 10_000;

    let s1 = TailTraceSampler.init(cfg) ?? return;
    let s2 = TailTraceSampler.init(cfg) ?? return;

    let dec1: [1]TailSampleDecision = [TailSampleDecision{}; 1];
    let dec2: [1]TailSampleDecision = [TailSampleDecision{}; 1];
    let d1_s: []TailSampleDecision = dec1[0..];
    let d2_s: []TailSampleDecision = dec2[0..];

    let kept_count: usize = 0;
    let i: u64 = 1;
    while (i <= 500) : (i += 1) {
        expect_eq(s1.ingest_span(i * 17 + 3, i ^ 0xCAFE, 1_000, false, false, true, 1000), true);
        expect_eq(s2.ingest_span(i * 17 + 3, i ^ 0xCAFE, 1_000, false, false, true, 1000), true);
        expect_eq(s1.flush_ready_decisions(1000, d1_s), 1);
        expect_eq(s2.flush_ready_decisions(1000, d2_s), 1);

        // 100% deterministic across independent instances
        expect_eq(dec1[0].keep, dec2[0].keep);
        if (dec1[0].keep) {
            expect_eq(dec1[0].reason == TailSampleReason.kept_probabilistic, true);
            kept_count += 1;
        }
    }

    // Expected ~125 out of 500 (25%); verify tight convergence [80, 170]
    expect_eq(kept_count >= 80 && kept_count <= 170, true);
    expect_eq(s1.stats().kept_probabilistic, kept_count as u64);
    expect_eq(s1.stats().dropped_baseline, (500 - kept_count) as u64);
});

// ============================================================================
// Test 6: Error Storm Budget Cap & Next-Window Automatic Recovery
// ============================================================================
test("trace_sampler: error storm budget circuit breaker & next-window recovery", () => {
    let cfg = TailSamplerConfig.defaults();
    cfg.max_kept_per_window = 3;
    cfg.rate_window_ms = 1_000;
    cfg.baseline_sample_bps = 0;

    let sampler = TailTraceSampler.init(cfg) ?? return;

    // Window 1 [t=1000..1999]: 5 error traces arrive -> first 3 kept, 2 dropped_budget_cap
    let i: u64 = 1;
    while (i <= 5) : (i += 1) {
        expect_eq(sampler.ingest_span(i, 1, 5_000, true, false, true, 1000), true);
    }
    let decs: [8]TailSampleDecision = [TailSampleDecision{}; 8];
    let dec_s: []TailSampleDecision = decs[0..];
    expect_eq(sampler.flush_ready_decisions(1000, dec_s), 5);

    expect_eq(decs[0].keep, true);
    expect_eq(decs[1].keep, true);
    expect_eq(decs[2].keep, true);
    expect_eq(decs[3].keep, false);
    expect_eq(decs[3].reason == TailSampleReason.dropped_budget_cap, true);
    expect_eq(decs[4].keep, false);
    expect_eq(decs[4].reason == TailSampleReason.dropped_budget_cap, true);

    // Still inside Window 1 at t=1500 -> another error trace is also budget-capped
    expect_eq(sampler.ingest_span(6, 1, 5_000, true, false, true, 1500), true);
    expect_eq(sampler.flush_ready_decisions(1500, dec_s), 1);
    expect_eq(decs[0].keep, false);
    expect_eq(decs[0].reason == TailSampleReason.dropped_budget_cap, true);

    // Window 2 at t=2000 (2000 - 1000 >= 1000): budget resets automatically -> 3 new errors kept!
    let j: u64 = 10;
    while (j < 13) : (j += 1) {
        expect_eq(sampler.ingest_span(j, 2, 5_000, true, false, true, 2000), true);
    }
    expect_eq(sampler.flush_ready_decisions(2000, dec_s), 3);
    expect_eq(decs[0].keep, true);
    expect_eq(decs[1].keep, true);
    expect_eq(decs[2].keep, true);

    const st = sampler.stats();
    expect_eq(st.kept_error, 6);
    expect_eq(st.dropped_budget_cap, 3);
    expect_eq(st.total_evaluated, 9);
});

// ============================================================================
// Test 7: Full 128-Slot Overflow Eviction & Batched Small-Buffer Flush
// ============================================================================
test("trace_sampler: 128-slot oldest overflow eviction & batched flush", () => {
    let cfg = TailSamplerConfig.defaults();
    cfg.decision_wait_ms = 5_000;
    cfg.max_kept_per_window = 500;
    cfg.baseline_sample_bps = 10000;

    let sampler = TailTraceSampler.init(cfg) ?? return;

    // Fill all 128 slots with non-root traces at t = 100 .. 227
    let i: u64 = 0;
    while (i < TAIL_SAMPLER_MAX_TRACES as u64) : (i += 1) {
        expect_eq(sampler.ingest_span(i + 1, 0, 1_000, false, false, false, 100 + i), true);
    }
    expect_eq(sampler.stats().active_traces, 128);
    expect_eq(sampler.stats().evicted_overflow, 0);

    // Insert 2 more traces -> evicts oldest 2 slots (trace_id_lo 1 & 2, t=100 & 101)
    expect_eq(sampler.ingest_span(9001, 0, 1_000, false, false, false, 300), true);
    expect_eq(sampler.ingest_span(9002, 0, 1_000, false, false, false, 301), true);
    expect_eq(sampler.stats().active_traces, 128);
    expect_eq(sampler.stats().evicted_overflow, 2);

    // Drain all 128 traces after timeout using a small 32-item batch buffer (4 flushes)
    let batch: [32]TailSampleDecision = [TailSampleDecision{}; 32];
    let batch_s: []TailSampleDecision = batch[0..];
    let total_flushed: usize = 0;
    let rounds: usize = 0;
    while (rounds < 10) : (rounds += 1) {
        const n = sampler.flush_ready_decisions(10_000, batch_s);
        if (n == 0) break;
        total_flushed += n;
    }

    expect_eq(total_flushed, 128);
    expect_eq(sampler.stats().active_traces, 0);
    expect_eq(sampler.stats().total_evaluated, 128);
});

// ============================================================================
// Test 8: Invalid Config Rejection, Zero TraceID & Empty Output Slice Guards
// ============================================================================
test("trace_sampler: invalid config, zero trace ID & empty output slice guards", () => {
    let bad_bps = TailSamplerConfig.defaults();
    bad_bps.baseline_sample_bps = 10001;
    expect_eq(bad_bps.isValid(), false);
    expect_eq(TailTraceSampler.init(bad_bps) == null, true);

    let bad_wait = TailSamplerConfig.defaults();
    bad_wait.decision_wait_ms = 0;
    expect_eq(TailTraceSampler.init(bad_wait) == null, true);

    let bad_win = TailSamplerConfig.defaults();
    bad_win.rate_window_ms = 0;
    expect_eq(TailTraceSampler.init(bad_win) == null, true);

    let bad_cap = TailSamplerConfig.defaults();
    bad_cap.max_kept_per_window = 0;
    expect_eq(TailTraceSampler.init(bad_cap) == null, true);

    let sampler = TailTraceSampler.init(TailSamplerConfig.defaults()) ?? return;

    // All-zero 128-bit TraceID rejected
    expect_eq(sampler.ingest_span(0, 0, 1_000, true, true, true, 100), false);
    expect_eq(sampler.stats().active_traces, 0);

    // Empty output slice preserves ready traces for subsequent flush
    expect_eq(sampler.ingest_span(42, 0, 1_000, true, false, true, 100), true);
    let empty_buf: [0]TailSampleDecision = [];
    let empty_s: []TailSampleDecision = empty_buf[0..];
    expect_eq(sampler.flush_ready_decisions(100, empty_s), 0);
    expect_eq(sampler.stats().active_traces, 1);

    let one_buf: [1]TailSampleDecision = [TailSampleDecision{}; 1];
    let one_s: []TailSampleDecision = one_buf[0..];
    expect_eq(sampler.flush_ready_decisions(100, one_s), 1);
    expect_eq(one_buf[0].trace_id_lo, 42);
    expect_eq(one_buf[0].keep, true);
    expect_eq(sampler.stats().active_traces, 0);
});
