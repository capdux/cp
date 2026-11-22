// Origin: Pure-CP Stdlib (Zero-Alloc Adaptive Structured Log Sampler & Storm Suppressor) | Created: 2026-11-23
// Pure CP zero-heap-allocation Uber Zap-style adaptive log sampler & storm deduplicator:
//   - Fixed 64-bucket fingerprint table keyed by (level, message_signature_hash) with LRU eviction
//   - Initial burst pass-through (`initial_burst`) + steady-state 1-in-N (`thereafter_every`) sampling
//   - Configurable severity bypass (`bypass_min_level`, e.g., critical/fatal never dropped)
//   - Window roll-over & LRU eviction suppression reporting (`dropped_in_prev_window` + `drainSummaries`)
//     preventing silent incident blackouts during log storms

const std = @import("std");

// ============================================================================
// 1. Constants & Fingerprint Hash Helper
// ============================================================================

/// Maximum distinct `(level, signature_hash)` log buckets tracked inline (zero heap).
pub const LOG_SAMPLER_MAX_BUCKETS: usize = 64;

/// Maximum pending suppression summaries retained in the ring queue.
pub const LOG_SAMPLER_MAX_SUMMARIES: usize = 64;

/// Computes deterministic 64-bit FNV-1a fingerprint for a log message template.
pub fn logSignatureHash(message: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < message.len) : (i += 1) {
        h ^= (message[i] as u64);
        h = h *% 1099511628211;
    }
    return h;
}

// ============================================================================
// 2. Public Enums & Data Models
// ============================================================================

/// Monotonic log severity levels for bucket partitioning and bypass policy.
pub enum LogSampleLevel : u8 {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    err = 4,
    critical = 5,
    fatal = 6,
}

/// Sampling verdict returned for each evaluated log event.
pub enum LogSampleVerdict {
    emit_burst,
    emit_sampled,
    emit_bypass,
    dropped,
}

/// Configuration for `AdaptiveLogSampler`.
pub struct LogSamplerConfig {
    window_ms: u64 = 1000;
    initial_burst: u32 = 100;
    thereafter_every: u32 = 100;
    bypass_min_level: LogSampleLevel = LogSampleLevel.critical;
    enable_bypass: bool = true;

    /// Returns production defaults (`window_ms=1000`, `initial_burst=100`, `thereafter_every=100`,
    /// `bypass_min_level=critical`, `enable_bypass=true`).
    static fn defaults(): LogSamplerConfig {
        return LogSamplerConfig{
            window_ms: 1000,
            initial_burst: 100,
            thereafter_every: 100,
            bypass_min_level: LogSampleLevel.critical,
            enable_bypass: true,
        };
    }

    /// Validates configuration invariants (`window_ms > 0`).
    fn isValid(): bool {
        return _.window_ms > 0;
    }
}

/// Per-event sampling decision with previous-window suppression telemetry.
pub struct LogSampleDecision {
    verdict: LogSampleVerdict = LogSampleVerdict.emit_burst;
    should_emit: bool = true;
    window_seen_count: u64 = 0;
    dropped_in_prev_window: u64 = 0;
    window_rolled: bool = false;
    evicted_lru: bool = false;
}

/// Suppression storm summary emitted when a window rolls over, flushes, or evicts a noisy bucket.
pub struct LogSuppressionSummary {
    level: LogSampleLevel = LogSampleLevel.info;
    signature_hash: u64 = 0;
    window_start_ms: u64 = 0;
    window_end_ms: u64 = 0;
    total_seen: u64 = 0;
    emitted_count: u64 = 0;
    dropped_count: u64 = 0;
}

/// Diagnostic telemetry snapshot returned by `AdaptiveLogSampler.stats()`.
pub struct LogSamplerStats {
    active_buckets: usize = 0;
    capacity: usize = LOG_SAMPLER_MAX_BUCKETS;
    total_evaluated: u64 = 0;
    total_emitted: u64 = 0;
    total_dropped: u64 = 0;
    total_bypassed: u64 = 0;
    total_evictions: u64 = 0;
    pending_summaries: usize = 0;
}

// ============================================================================
// 3. Internal Bucket & Zero-Alloc Adaptive Log Sampler Engine
// ============================================================================

struct LogSampleBucket {
    level: LogSampleLevel = LogSampleLevel.info;
    signature_hash: u64 = 0;
    window_start_ms: u64 = 0;
    seen_in_window: u64 = 0;
    emitted_in_window: u64 = 0;
    dropped_in_window: u64 = 0;
    last_touch_tick: u64 = 0;
    occupied: bool = false;
}

/// Zero-heap-allocation Uber Zap-style adaptive log sampler & storm suppressor.
pub struct AdaptiveLogSampler {
    cfg: LogSamplerConfig = LogSamplerConfig{},
    buckets: [64]LogSampleBucket = [LogSampleBucket{}; 64],
    active_buckets: usize = 0,
    summaries: [64]LogSuppressionSummary = [LogSuppressionSummary{}; 64],
    sum_head: usize = 0,
    sum_tail: usize = 0,
    sum_count: usize = 0,
    lru_tick: u64 = 0,
    total_evaluated: u64 = 0,
    total_emitted: u64 = 0,
    total_dropped: u64 = 0,
    total_bypassed: u64 = 0,
    total_evictions: u64 = 0;

    /// Initializes an `AdaptiveLogSampler` from `cfg`. Returns `null` if `cfg` is invalid.
    static fn init(cfg: LogSamplerConfig): AdaptiveLogSampler? {
        if (!cfg.isValid()) return null;
        return AdaptiveLogSampler{
            cfg: cfg,
        };
    }

    /// Resets all buckets, pending summaries, and telemetry counters while keeping `cfg`.
    fn reset(*_) {
        let i: usize = 0;
        while (i < LOG_SAMPLER_MAX_BUCKETS) : (i += 1) {
            _.buckets[i] = LogSampleBucket{};
        }
        _.active_buckets = 0;
        _.sum_head = 0;
        _.sum_tail = 0;
        _.sum_count = 0;
        _.lru_tick = 0;
        _.total_evaluated = 0;
        _.total_emitted = 0;
        _.total_dropped = 0;
        _.total_bypassed = 0;
        _.total_evictions = 0;
    }

    fn pushSummary(*_, s: LogSuppressionSummary) {
        if (_.sum_count == LOG_SAMPLER_MAX_SUMMARIES) {
            // Overwrite oldest unread summary when ring is full (zero heap).
            _.sum_head = (_.sum_head + 1) % LOG_SAMPLER_MAX_SUMMARIES;
            _.sum_count -= 1;
        }
        _.summaries[_.sum_tail] = s;
        _.sum_tail = (_.sum_tail + 1) % LOG_SAMPLER_MAX_SUMMARIES;
        _.sum_count += 1;
    }

    fn findBucket(level: LogSampleLevel, sig_hash: u64): usize? {
        let i: usize = 0;
        while (i < LOG_SAMPLER_MAX_BUCKETS) : (i += 1) {
            if (_.buckets[i].occupied &&
                _.buckets[i].level == level &&
                _.buckets[i].signature_hash == sig_hash)
            {
                return i;
            }
        }
        return null;
    }

    /// Evaluates whether a log event `(level, signature_hash)` at `now_ms` should be emitted.
    fn shouldLog(
        *_,
        level: LogSampleLevel,
        signature_hash: u64,
        now_ms: u64,
    ): LogSampleDecision {
        _.total_evaluated += 1;
        _.lru_tick += 1;

        // 1. Severity bypass policy (e.g., critical/fatal always emitted verbatim).
        if (_.cfg.enable_bypass && (level as u8) >= (_.cfg.bypass_min_level as u8)) {
            _.total_emitted += 1;
            _.total_bypassed += 1;
            return LogSampleDecision{
                verdict: LogSampleVerdict.emit_bypass,
                should_emit: true,
                window_seen_count: 1,
                dropped_in_prev_window: 0,
                window_rolled: false,
                evicted_lru: false,
            };
        }

        let slot_idx: usize = 0;
        let window_rolled: bool = false;
        let prev_dropped: u64 = 0;
        let evicted_lru: bool = false;

        if (existing := _.findBucket(level, signature_hash)) {
            slot_idx = existing;
            // Roll window forward when elapsed >= window_ms (monotonic clock guard).
            if (now_ms >= _.buckets[slot_idx].window_start_ms &&
                (now_ms - _.buckets[slot_idx].window_start_ms) >= _.cfg.window_ms)
            {
                window_rolled = true;
                prev_dropped = _.buckets[slot_idx].dropped_in_window;
                if (prev_dropped > 0) {
                    _.pushSummary(LogSuppressionSummary{
                        level: level,
                        signature_hash: signature_hash,
                        window_start_ms: _.buckets[slot_idx].window_start_ms,
                        window_end_ms: now_ms,
                        total_seen: _.buckets[slot_idx].seen_in_window,
                        emitted_count: _.buckets[slot_idx].emitted_in_window,
                        dropped_count: prev_dropped,
                    });
                }
                _.buckets[slot_idx].window_start_ms = now_ms;
                _.buckets[slot_idx].seen_in_window = 0;
                _.buckets[slot_idx].emitted_in_window = 0;
                _.buckets[slot_idx].dropped_in_window = 0;
            }
        } else {
            // Find free bucket or evict least-recently-used bucket.
            let found_free: bool = false;
            let i: usize = 0;
            while (i < LOG_SAMPLER_MAX_BUCKETS) : (i += 1) {
                if (!_.buckets[i].occupied) {
                    slot_idx = i;
                    found_free = true;
                    break;
                }
            }

            if (found_free) {
                _.active_buckets += 1;
            } else {
                let victim: usize = 0;
                let oldest_tick: u64 = _.buckets[0].last_touch_tick;
                let j: usize = 1;
                while (j < LOG_SAMPLER_MAX_BUCKETS) : (j += 1) {
                    if (_.buckets[j].last_touch_tick < oldest_tick) {
                        oldest_tick = _.buckets[j].last_touch_tick;
                        victim = j;
                    }
                }
                if (_.buckets[victim].dropped_in_window > 0) {
                    _.pushSummary(LogSuppressionSummary{
                        level: _.buckets[victim].level,
                        signature_hash: _.buckets[victim].signature_hash,
                        window_start_ms: _.buckets[victim].window_start_ms,
                        window_end_ms: now_ms,
                        total_seen: _.buckets[victim].seen_in_window,
                        emitted_count: _.buckets[victim].emitted_in_window,
                        dropped_count: _.buckets[victim].dropped_in_window,
                    });
                }
                slot_idx = victim;
                evicted_lru = true;
                _.total_evictions += 1;
            }

            _.buckets[slot_idx] = LogSampleBucket{
                level: level,
                signature_hash: signature_hash,
                window_start_ms: now_ms,
                seen_in_window: 0,
                emitted_in_window: 0,
                dropped_in_window: 0,
                last_touch_tick: _.lru_tick,
                occupied: true,
            };
        }

        // 2. Apply Uber Zap sampling rule: first `initial_burst` pass, then 1-in-`thereafter_every`.
        _.buckets[slot_idx].seen_in_window += 1;
        _.buckets[slot_idx].last_touch_tick = _.lru_tick;
        const seen: u64 = _.buckets[slot_idx].seen_in_window;
        const burst_lim: u64 = _.cfg.initial_burst as u64;
        const every: u64 = _.cfg.thereafter_every as u64;

        let verdict: LogSampleVerdict = LogSampleVerdict.dropped;
        let emit: bool = false;

        if (seen <= burst_lim) {
            verdict = LogSampleVerdict.emit_burst;
            emit = true;
        } else if (every > 0 && ((seen - burst_lim) % every == 0)) {
            verdict = LogSampleVerdict.emit_sampled;
            emit = true;
        } else {
            verdict = LogSampleVerdict.dropped;
            emit = false;
        }

        if (emit) {
            _.buckets[slot_idx].emitted_in_window += 1;
            _.total_emitted += 1;
        } else {
            _.buckets[slot_idx].dropped_in_window += 1;
            _.total_dropped += 1;
        }

        return LogSampleDecision{
            verdict: verdict,
            should_emit: emit,
            window_seen_count: seen,
            dropped_in_prev_window: prev_dropped,
            window_rolled: window_rolled,
            evicted_lru: evicted_lru,
        };
    }

    /// Convenience wrapper hashing `message` with `logSignatureHash` before calling `shouldLog`.
    fn shouldLogMessage(
        *_,
        level: LogSampleLevel,
        message: []const u8,
        now_ms: u64,
    ): LogSampleDecision {
        return _.shouldLog(level, logSignatureHash(message), now_ms);
    }

    /// Proactively flushes all expired buckets at `now_ms`, enqueueing suppression summaries
    /// for any bucket that dropped events and resetting their counters. Returns summaries enqueued.
    fn flushExpiredWindows(*_, now_ms: u64): usize {
        let flushed: usize = 0;
        let i: usize = 0;
        while (i < LOG_SAMPLER_MAX_BUCKETS) : (i += 1) {
            if (!_.buckets[i].occupied) continue;
            if (now_ms >= _.buckets[i].window_start_ms &&
                (now_ms - _.buckets[i].window_start_ms) >= _.cfg.window_ms)
            {
                if (_.buckets[i].dropped_in_window > 0) {
                    _.pushSummary(LogSuppressionSummary{
                        level: _.buckets[i].level,
                        signature_hash: _.buckets[i].signature_hash,
                        window_start_ms: _.buckets[i].window_start_ms,
                        window_end_ms: now_ms,
                        total_seen: _.buckets[i].seen_in_window,
                        emitted_count: _.buckets[i].emitted_in_window,
                        dropped_count: _.buckets[i].dropped_in_window,
                    });
                    flushed += 1;
                }
                _.buckets[i].window_start_ms = now_ms;
                _.buckets[i].seen_in_window = 0;
                _.buckets[i].emitted_in_window = 0;
                _.buckets[i].dropped_in_window = 0;
            }
        }
        return flushed;
    }

    /// Drains up to `out_summaries.len` pending suppression summaries in FIFO order.
    fn drainSummaries(*_, out_summaries: *[]LogSuppressionSummary): usize {
        if (out_summaries.len == 0 || _.sum_count == 0) return 0;
        let written: usize = 0;
        while (written < out_summaries.len && _.sum_count > 0) : (written += 1) {
            out_summaries[written] = _.summaries[_.sum_head];
            _.sum_head = (_.sum_head + 1) % LOG_SAMPLER_MAX_SUMMARIES;
            _.sum_count -= 1;
        }
        return written;
    }

    /// Returns a diagnostic telemetry snapshot.
    fn stats(): LogSamplerStats {
        return LogSamplerStats{
            active_buckets: _.active_buckets,
            capacity: LOG_SAMPLER_MAX_BUCKETS,
            total_evaluated: _.total_evaluated,
            total_emitted: _.total_emitted,
            total_dropped: _.total_dropped,
            total_bypassed: _.total_bypassed,
            total_evictions: _.total_evictions,
            pending_summaries: _.sum_count,
        };
    }
}
