// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc OpenTelemetry Tail-Based Trace Sampler) | Created: 2026-11-22
// Pure CP zero-heap-allocation OpenTelemetry Collector / Jaeger style Tail-Based Trace Sampling Engine:
//   - Fixed inline 128-slot in-flight trace table (`TAIL_SAMPLER_MAX_TRACES`) with 0 dynamic heap allocation
//   - Multi-span aggregation per 128-bit TraceID (`span_count`, `max_duration_us`, `has_error`, `has_priority_tag`, `root_finished`)
//   - Oldest-slot overflow eviction (`evicted_overflow`) under extreme trace cardinality spikes
//   - Priority tail decision pipeline: error -> slow latency -> priority tag -> SplitMix64 deterministic bps sampling
//   - Sliding-window `max_kept_per_window` budget cap protecting downstream trace storage from error storms

// ============================================================================
// 1. Constants, Enums & Public Models
// ============================================================================

/// Maximum concurrent in-flight traces tracked in `TailTraceSampler` (zero dynamic heap).
pub const TAIL_SAMPLER_MAX_TRACES: usize = 128;

/// Final tail-sampling disposition reason for an evaluated trace.
pub enum TailSampleReason {
    dropped,
    kept_error,
    kept_slow_latency,
    kept_priority_tag,
    kept_probabilistic,
    dropped_budget_cap,
}

/// Configuration for `TailTraceSampler`.
pub struct TailSamplerConfig {
    slow_threshold_us: u64 = 500_000;
    baseline_sample_bps: u16 = 500;
    decision_wait_ms: u64 = 5_000;
    max_kept_per_window: u32 = 1_000;
    rate_window_ms: u64 = 1_000;

    /// Returns production default config (500ms slow threshold, 5% baseline, 5s wait, 1000 kept/1s).
    static fn defaults(): TailSamplerConfig {
        return TailSamplerConfig{
            slow_threshold_us: 500_000,
            baseline_sample_bps: 500,
            decision_wait_ms: 5_000,
            max_kept_per_window: 1_000,
            rate_window_ms: 1_000,
        };
    }

    /// Validates basis-point bounds (`<= 10000`) and strictly positive wait/window/budget parameters.
    fn isValid(): bool {
        return _.baseline_sample_bps <= 10000 &&
            _.decision_wait_ms > 0 &&
            _.rate_window_ms > 0 &&
            _.max_kept_per_window > 0;
    }
}

/// In-flight trace aggregation slot stored inline in `TailTraceSampler`.
pub struct ActiveTraceSlot {
    trace_id_lo: u64 = 0;
    trace_id_hi: u64 = 0;
    first_seen_ms: u64 = 0;
    last_seen_ms: u64 = 0;
    span_count: u32 = 0;
    max_duration_us: u64 = 0;
    has_error: bool = false;
    has_priority_tag: bool = false;
    root_finished: bool = false;
    occupied: bool = false;
}

/// Tail-sampling decision emitted when a trace finishes or reaches `decision_wait_ms`.
pub struct TailSampleDecision {
    trace_id_lo: u64 = 0;
    trace_id_hi: u64 = 0;
    keep: bool = false;
    reason: TailSampleReason = TailSampleReason.dropped;
    span_count: u32 = 0;
    max_duration_us: u64 = 0;
}

/// Cumulative telemetry snapshot for `TailTraceSampler`.
pub struct TailSamplerStats {
    active_traces: usize = 0;
    total_evaluated: u64 = 0;
    kept_error: u64 = 0;
    kept_slow: u64 = 0;
    kept_priority: u64 = 0;
    kept_probabilistic: u64 = 0;
    dropped_baseline: u64 = 0;
    dropped_budget_cap: u64 = 0;
    evicted_overflow: u64 = 0;
}

// ============================================================================
// 2. Internal Deterministic SplitMix64 Hashing Helper
// ============================================================================

fn splitMix64(seed: u64): u64 {
    let z: u64 = seed +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

/// Deterministically maps a 128-bit TraceID `(lo, hi)` into a uniform basis-point bucket `[0, 10000)`.
fn traceSampleBucketBps(trace_id_lo: u64, trace_id_hi: u64): u16 {
    const hi_mix: u64 = splitMix64(trace_id_hi +% 0x517cc1b727220a95);
    const mixed: u64 = splitMix64(trace_id_lo ^ hi_mix);
    return @intCast(mixed % 10000);
}

// ============================================================================
// 3. Zero-Alloc Tail-Based Trace Sampler Engine
// ============================================================================

/// Zero-heap-allocation OpenTelemetry Tail-Based Trace Sampler with 128 inline trace slots
/// and per-window keep-budget circuit breaker.
pub struct TailTraceSampler {
    cfg: TailSamplerConfig = TailSamplerConfig{};
    slots: [TAIL_SAMPLER_MAX_TRACES]ActiveTraceSlot = [ActiveTraceSlot{}; TAIL_SAMPLER_MAX_TRACES];
    active_count: usize = 0;
    window_start_ms: u64 = 0;
    window_initialized: bool = false;
    kept_in_window: u32 = 0;

    total_evaluated: u64 = 0;
    kept_error: u64 = 0;
    kept_slow: u64 = 0;
    kept_priority: u64 = 0;
    kept_probabilistic: u64 = 0;
    dropped_baseline: u64 = 0;
    dropped_budget_cap: u64 = 0;
    evicted_overflow: u64 = 0;

    /// Initializes a `TailTraceSampler` after validating `cfg`. Returns `null` if `cfg` is invalid.
    static fn init(cfg: TailSamplerConfig): TailTraceSampler? {
        if (!cfg.isValid()) return null;
        return TailTraceSampler{
            cfg: cfg,
            slots: [ActiveTraceSlot{}; TAIL_SAMPLER_MAX_TRACES],
            active_count: 0,
            window_start_ms: 0,
            window_initialized: false,
            kept_in_window: 0,
            total_evaluated: 0,
            kept_error: 0,
            kept_slow: 0,
            kept_priority: 0,
            kept_probabilistic: 0,
            dropped_baseline: 0,
            dropped_budget_cap: 0,
            evicted_overflow: 0,
        };
    }

    /// Ingests a span belonging to `(trace_id_lo, trace_id_hi)` at `now_ms`.
    /// - Rejects all-zero 128-bit TraceID (`trace_id_lo == 0 && trace_id_hi == 0`), returning `false`.
    /// - Aggregates multiple spans of the same trace (latching `has_error`, `has_priority_tag`,
    ///   `root_finished`, tracking `max_duration_us` and `span_count`).
    /// - When all 128 inline slots are occupied, evicts the oldest in-flight slot (lowest `first_seen_ms`)
    ///   and increments `evicted_overflow`.
    fn ingest_span(
        *_,
        trace_id_lo: u64,
        trace_id_hi: u64,
        duration_us: u64,
        is_error: bool,
        is_priority: bool,
        is_root: bool,
        now_ms: u64,
    ): bool {
        if (trace_id_lo == 0 && trace_id_hi == 0) return false;

        // 1. Search existing active slot for this TraceID
        let free_idx: usize = TAIL_SAMPLER_MAX_TRACES;
        let i: usize = 0;
        while (i < TAIL_SAMPLER_MAX_TRACES) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].trace_id_lo == trace_id_lo && _.slots[i].trace_id_hi == trace_id_hi) {
                    if (_.slots[i].span_count < 0xFFFFFFFF) {
                        _.slots[i].span_count += 1;
                    }
                    if (duration_us > _.slots[i].max_duration_us) {
                        _.slots[i].max_duration_us = duration_us;
                    }
                    if (is_error) _.slots[i].has_error = true;
                    if (is_priority) _.slots[i].has_priority_tag = true;
                    if (is_root) _.slots[i].root_finished = true;
                    if (now_ms > _.slots[i].last_seen_ms) {
                        _.slots[i].last_seen_ms = now_ms;
                    }
                    return true;
                }
            } else if (free_idx == TAIL_SAMPLER_MAX_TRACES) {
                free_idx = i;
            }
        }

        // 2. Allocate free slot, or evict oldest in-flight slot on overflow
        let target_idx: usize = free_idx;
        if (target_idx == TAIL_SAMPLER_MAX_TRACES) {
            let oldest_idx: usize = 0;
            let oldest_ms: u64 = _.slots[0].first_seen_ms;
            let j: usize = 1;
            while (j < TAIL_SAMPLER_MAX_TRACES) : (j += 1) {
                if (_.slots[j].first_seen_ms < oldest_ms) {
                    oldest_ms = _.slots[j].first_seen_ms;
                    oldest_idx = j;
                }
            }
            target_idx = oldest_idx;
            _.evicted_overflow += 1;
        } else {
            _.active_count += 1;
        }

        _.slots[target_idx] = ActiveTraceSlot{
            trace_id_lo: trace_id_lo,
            trace_id_hi: trace_id_hi,
            first_seen_ms: now_ms,
            last_seen_ms: now_ms,
            span_count: 1,
            max_duration_us: duration_us,
            has_error: is_error,
            has_priority_tag: is_priority,
            root_finished: is_root,
            occupied: true,
        };
        return true;
    }

    /// Evaluates and flushes ready traces (`root_finished == true` or
    /// `now_ms - first_seen_ms >= cfg.decision_wait_ms`) into `out_decisions`, reclaiming their slots.
    /// Decision priority:
    ///   1. `has_error` -> `kept_error`
    ///   2. `slow_threshold_us > 0 && max_duration_us >= slow_threshold_us` -> `kept_slow_latency`
    ///   3. `has_priority_tag` -> `kept_priority_tag`
    ///   4. Deterministic SplitMix64 bucket `< baseline_sample_bps` -> `kept_probabilistic` (else `dropped`)
    /// All `keep` outcomes are gated by `max_kept_per_window` per `rate_window_ms`; excess keeps
    /// are downgraded to `keep = false, reason = dropped_budget_cap`.
    /// Stops flushing when `out_decisions` is full so unread ready traces are preserved for the next flush.
    fn flush_ready_decisions(
        *_,
        now_ms: u64,
        out_decisions: *[]TailSampleDecision,
    ): usize {
        if (out_decisions.len == 0) return 0;

        // Maintain rate-limit budget window
        if (!_.window_initialized) {
            _.window_start_ms = now_ms;
            _.window_initialized = true;
            _.kept_in_window = 0;
        } else if (now_ms >= _.window_start_ms && (now_ms - _.window_start_ms) >= _.cfg.rate_window_ms) {
            _.window_start_ms = now_ms;
            _.kept_in_window = 0;
        }

        let written: usize = 0;
        let i: usize = 0;
        while (i < TAIL_SAMPLER_MAX_TRACES && written < out_decisions.len) : (i += 1) {
            if (!_.slots[i].occupied) continue;

            const elapsed_ms: u64 = if (now_ms >= _.slots[i].first_seen_ms)
                (now_ms - _.slots[i].first_seen_ms)
            else
                0;
            const is_ready: bool = _.slots[i].root_finished || (elapsed_ms >= _.cfg.decision_wait_ms);
            if (!is_ready) continue;

            // Priority policy evaluation
            let want_keep: bool = false;
            let base_reason: TailSampleReason = TailSampleReason.dropped;

            if (_.slots[i].has_error) {
                want_keep = true;
                base_reason = TailSampleReason.kept_error;
            } else if (_.cfg.slow_threshold_us > 0 && _.slots[i].max_duration_us >= _.cfg.slow_threshold_us) {
                want_keep = true;
                base_reason = TailSampleReason.kept_slow_latency;
            } else if (_.slots[i].has_priority_tag) {
                want_keep = true;
                base_reason = TailSampleReason.kept_priority_tag;
            } else if (_.cfg.baseline_sample_bps > 0) {
                const bucket_bps: u16 = traceSampleBucketBps(_.slots[i].trace_id_lo, _.slots[i].trace_id_hi);
                if (_.cfg.baseline_sample_bps == 10000 || bucket_bps < _.cfg.baseline_sample_bps) {
                    want_keep = true;
                    base_reason = TailSampleReason.kept_probabilistic;
                }
            }

            // Apply per-window keep budget cap
            let final_keep: bool = false;
            let final_reason: TailSampleReason = TailSampleReason.dropped;

            if (want_keep) {
                if (_.kept_in_window < _.cfg.max_kept_per_window) {
                    _.kept_in_window += 1;
                    final_keep = true;
                    final_reason = base_reason;

                    if (base_reason == TailSampleReason.kept_error) {
                        _.kept_error += 1;
                    } else if (base_reason == TailSampleReason.kept_slow_latency) {
                        _.kept_slow += 1;
                    } else if (base_reason == TailSampleReason.kept_priority_tag) {
                        _.kept_priority += 1;
                    } else {
                        _.kept_probabilistic += 1;
                    }
                } else {
                    final_keep = false;
                    final_reason = TailSampleReason.dropped_budget_cap;
                    _.dropped_budget_cap += 1;
                }
            } else {
                final_keep = false;
                final_reason = TailSampleReason.dropped;
                _.dropped_baseline += 1;
            }

            _.total_evaluated += 1;
            out_decisions[written] = TailSampleDecision{
                trace_id_lo: _.slots[i].trace_id_lo,
                trace_id_hi: _.slots[i].trace_id_hi,
                keep: final_keep,
                reason: final_reason,
                span_count: _.slots[i].span_count,
                max_duration_us: _.slots[i].max_duration_us,
            };
            written += 1;

            // Reclaim slot
            _.slots[i] = ActiveTraceSlot{};
            _.active_count -= 1;
        }

        return written;
    }

    /// Returns a cumulative telemetry & active-slot snapshot.
    fn stats(): TailSamplerStats {
        return TailSamplerStats{
            active_traces: _.active_count,
            total_evaluated: _.total_evaluated,
            kept_error: _.kept_error,
            kept_slow: _.kept_slow,
            kept_priority: _.kept_priority,
            kept_probabilistic: _.kept_probabilistic,
            dropped_baseline: _.dropped_baseline,
            dropped_budget_cap: _.dropped_budget_cap,
            evicted_overflow: _.evicted_overflow,
        };
    }
}
