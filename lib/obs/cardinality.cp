// Origin: Pure-CP Stdlib (Prometheus / OpenTelemetry High-Cardinality Guard & __overflow__ Rollup Controller) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation metrics cardinality protection engine:
//   - Per-metric quota registry (max_series, max_label_value_len, overflow_policy)
//   - Fixed-capacity open-addressing FNV-1a 64-bit active series hash table with tombstone reuse
//   - Per-metric 64-bucket HyperLogLog register estimator using native hardware @clz
//   - Existing-series fast path (never throttled) + automatic __overflow__ bucket rollup / drop / warn_only
//   - TTL cold-series eviction (evictStaleSeries) & window reset + full telemetry stats

const std = @import("std");

// ============================================================================
// Constants
// ============================================================================

/// Maximum number of distinct metrics tracked by a single `CardinalityGuard`.
pub const CARD_MAX_METRICS: usize = 32;

/// Fixed power-of-two capacity of the open-addressing active series hash table.
pub const CARD_MAX_SERIES: usize = 1024;

/// Number of HyperLogLog registers per metric (`m = 64`, `p = 6`, 64 bytes per metric).
pub const CARD_HLL_REGISTERS: usize = 64;

/// Canonical Prometheus / OpenTelemetry overflow label value for rolled-up high-cardinality series.
pub const CARD_OVERFLOW_LABEL: []const u8 = "__overflow__";

const CARD_SERIES_MASK: usize = CARD_MAX_SERIES - 1;
const FNV1A_OFFSET_BASIS: u64 = 0xcbf29ce484222325;
const FNV1A_PRIME: u64 = 0x100000001b3;
const HLL_ALPHA_64: f64 = 0.709;

// ============================================================================
// Internal Hash & HyperLogLog Helpers
// ============================================================================

fn mix64(x_in: u64): u64 {
    let z: u64 = x_in +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

fn fnv1aUpdateU64(h_in: u64, v: u64): u64 {
    let h: u64 = h_in;
    let shift: usize = 0;
    while (shift < 64) : (shift += 8) {
        const s: u6 = @intCast(shift);
        const byte: u64 = (v >> s) & 0xff;
        h = (h ^ byte) *% FNV1A_PRIME;
    }
    return h;
}

/// Computes FNV-1a 64-bit hash over an arbitrary byte slice.
pub fn fnv1aHash64(bytes: []const u8): u64 {
    let h: u64 = FNV1A_OFFSET_BASIS;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h = (h ^ (bytes[i] as u64)) *% FNV1A_PRIME;
    }
    return h;
}

/// Computes a deterministic 64-bit FNV-1a series fingerprint combining `metric_id` and label bytes.
pub fn computeSeriesFingerprint(metric_id: u32, label_kv_bytes: []const u8): u64 {
    let h: u64 = fnv1aUpdateU64(FNV1A_OFFSET_BASIS, metric_id as u64);
    let i: usize = 0;
    while (i < label_kv_bytes.len) : (i += 1) {
        h = (h ^ (label_kv_bytes[i] as u64)) *% FNV1A_PRIME;
    }
    return h;
}

/// Returns `value` unchanged if `value.len <= max_label_value_len`, otherwise rewrites to `"__overflow__"`.
pub fn labelValueOrOverflow(value: []const u8, max_label_value_len: u32): []const u8 {
    if (value.len > (max_label_value_len as usize)) {
        return CARD_OVERFLOW_LABEL;
    }
    return value;
}

fn hllObserve(regs: *[CARD_HLL_REGISTERS]u8, fingerprint: u64) {
    const h: u64 = mix64(fingerprint);
    const bucket: usize = (h >> 58) as usize;
    const w: u64 = h << 6;
    let rho: u8 = 59;
    if (w != 0) {
        rho = (@clz(w) as u8) + 1;
    }
    if (rho > regs[bucket]) {
        regs[bucket] = rho;
    }
}

fn hllEstimate(regs: [CARD_HLL_REGISTERS]u8): f64 {
    let sum: f64 = 0.0;
    let zeros: usize = 0;
    let i: usize = 0;
    while (i < CARD_HLL_REGISTERS) : (i += 1) {
        const r: u8 = regs[i];
        if (r == 0) {
            zeros += 1;
            sum += 1.0;
        } else {
            // Compute 2^(-r) without powf overhead via bit shift when r < 53.
            if (r < 53) {
                const shift: u6 = @intCast(r);
                const denom: f64 = ((1 as u64) << shift) as f64;
                sum += 1.0 / denom;
            } else {
                sum += std.math.pow(f64, 2.0, -(r as f64));
            }
        }
    }

    const m_f: f64 = CARD_HLL_REGISTERS as f64;
    const raw_est: f64 = HLL_ALPHA_64 * m_f * m_f / sum;
    if (raw_est <= 2.5 * m_f && zeros > 0) {
        return m_f * std.math.log(f64, std.math.e, m_f / (zeros as f64));
    }
    return raw_est;
}

// ============================================================================
// 1. Enums, Quota Config & Telemetry Structs
// ============================================================================

/// Action taken when a metric exceeds its `max_series` cardinality quota.
pub enum CardinalityOverflowPolicy: u8 {
    /// Drop new series beyond `max_series`.
    drop = 0,
    /// Rewrite new series beyond `max_series` into a single `__overflow__` rollup bucket.
    rollup_overflow_label = 1,
    /// Record a warning counter and still admit the new series (subject to global table capacity).
    warn_only = 2,
}

/// Admission decision returned by `CardinalityGuard.admitSeries(...)`.
pub enum CardinalityDecision: u8 {
    /// Series already exists in the active table; allowed without consuming new quota.
    allow_existing = 0,
    /// New series admitted and recorded in the active table.
    allow_new = 1,
    /// Quota exceeded under `rollup_overflow_label`; caller should rewrite labels to `__overflow__`.
    rewrite_to_overflow = 2,
    /// Quota exceeded under `drop` policy, or global active series table is full.
    drop_over_limit = 3,
    /// `metric_id` has not been registered.
    unknown_metric = 4,
}

/// Per-metric cardinality quota configuration.
pub struct MetricQuotaConfig {
    metric_id: u32 = 0;
    max_series: u32 = 0;
    max_label_value_len: u32 = 128;
    overflow_policy: CardinalityOverflowPolicy = CardinalityOverflowPolicy.rollup_overflow_label;

    /// Convenience constructor for `rollup_overflow_label` policy with default 128-byte label limit.
    static fn defaultRollup(metric_id: u32, max_series: u32): MetricQuotaConfig {
        return MetricQuotaConfig{
            metric_id: metric_id,
            max_series: max_series,
            max_label_value_len: 128,
            overflow_policy: CardinalityOverflowPolicy.rollup_overflow_label,
        };
    }

    /// Convenience constructor for strict `drop` policy with default 128-byte label limit.
    static fn defaultDrop(metric_id: u32, max_series: u32): MetricQuotaConfig {
        return MetricQuotaConfig{
            metric_id: metric_id,
            max_series: max_series,
            max_label_value_len: 128,
            overflow_policy: CardinalityOverflowPolicy.drop,
        };
    }

    /// Returns `true` iff `max_series > 0` and `max_label_value_len > 0`.
    fn isValid(): bool {
        return _.max_series > 0 && _.max_label_value_len > 0;
    }
}

/// Per-metric real-time cardinality snapshot including HyperLogLog true-cardinality estimate.
pub struct MetricCardinalitySnapshot {
    metric_id: u32 = 0;
    max_series: u32 = 0;
    max_label_value_len: u32 = 0;
    overflow_policy: CardinalityOverflowPolicy = CardinalityOverflowPolicy.rollup_overflow_label;
    active_series: u32 = 0;
    overflow_active: bool = false;
    overflow_hits: u64 = 0;
    dropped_hits: u64 = 0;
    warn_hits: u64 = 0;
    total_admits: u64 = 0;
    estimated_true_cardinality: f64 = 0.0;
}

/// Global telemetry statistics across all registered metrics in `CardinalityGuard`.
pub struct CardinalityGuardStats {
    registered_metrics: usize = 0;
    active_series_total: usize = 0;
    tombstone_slots: usize = 0;
    total_admits: u64 = 0;
    allow_existing_count: u64 = 0;
    allow_new_count: u64 = 0;
    overflow_rollup_count: u64 = 0;
    dropped_count: u64 = 0;
    warn_only_count: u64 = 0;
    unknown_metric_count: u64 = 0;
    evicted_series_total: u64 = 0;
}

// ============================================================================
// 2. Internal Table Slots
// ============================================================================

enum SeriesSlotState: u8 {
    empty = 0,
    occupied = 1,
    tombstone = 2,
}

struct MetricQuotaSlot {
    active: bool = false;
    config: MetricQuotaConfig = MetricQuotaConfig{};
    active_series: u32 = 0;
    overflow_active: bool = false;
    overflow_hits: u64 = 0;
    dropped_hits: u64 = 0;
    warn_hits: u64 = 0;
    total_admits: u64 = 0;
    hll_regs: [CARD_HLL_REGISTERS]u8 = [0; CARD_HLL_REGISTERS];
}

struct SeriesEntry {
    state: SeriesSlotState = SeriesSlotState.empty;
    metric_slot: u16 = 0;
    series_hash: u64 = 0;
    label_fingerprint: u64 = 0;
    last_seen_ms: u64 = 0;
}

// ============================================================================
// 3. CardinalityGuard (Zero-Alloc Metrics High-Cardinality Guard Engine)
// ============================================================================

/// Zero-heap-allocation Prometheus / OpenTelemetry high-cardinality guard & overflow rollup engine.
pub struct CardinalityGuard {
    metrics: [CARD_MAX_METRICS]MetricQuotaSlot = [MetricQuotaSlot{}; CARD_MAX_METRICS];
    metric_count: usize = 0;
    series_table: [CARD_MAX_SERIES]SeriesEntry = [SeriesEntry{}; CARD_MAX_SERIES];
    active_series_total: usize = 0;
    tombstone_slots: usize = 0;
    total_admits: u64 = 0;
    allow_existing_count: u64 = 0;
    allow_new_count: u64 = 0;
    overflow_rollup_count: u64 = 0;
    dropped_count: u64 = 0;
    warn_only_count: u64 = 0;
    unknown_metric_count: u64 = 0;
    evicted_series_total: u64 = 0;

    /// Creates an empty `CardinalityGuard`.
    static fn init(): CardinalityGuard {
        return CardinalityGuard{};
    }

    fn findMetricSlot(metric_id: u32): usize? {
        let i: usize = 0;
        while (i < _.metric_count) : (i += 1) {
            if (_.metrics[i].active && _.metrics[i].config.metric_id == metric_id) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new metric quota configuration.
    /// Returns `false` if `cfg` is invalid (`max_series == 0` or `max_label_value_len == 0`),
    /// `metric_id` is already registered, or the metric table (`CARD_MAX_METRICS`) is full.
    fn registerMetric(*_, cfg: MetricQuotaConfig): bool {
        if (!cfg.isValid()) return false;
        if (_.findMetricSlot(cfg.metric_id) != null) return false;
        if (_.metric_count >= CARD_MAX_METRICS) return false;

        const idx: usize = _.metric_count;
        _.metrics[idx] = MetricQuotaSlot{
            active: true,
            config: cfg,
            active_series: 0,
            overflow_active: false,
            overflow_hits: 0,
            dropped_hits: 0,
            warn_hits: 0,
            total_admits: 0,
            hll_regs: [0; CARD_HLL_REGISTERS],
        };
        _.metric_count += 1;
        return true;
    }

    /// Admits or guards a metric time series identified by `(metric_id, label_fingerprint)` at `now_ms`.
    ///
    /// - Always updates the per-metric 64-bucket HyperLogLog estimator so true cardinality remains
    ///   observable even after overflow protection engages.
    /// - Existing active series are never throttled (`allow_existing`) and have `last_seen_ms` refreshed.
    /// - New series beyond `max_series` follow the metric's `overflow_policy`:
    ///   `rollup_overflow_label` -> `rewrite_to_overflow`, `drop` -> `drop_over_limit`,
    ///   `warn_only` -> `allow_new` (while incrementing `warn_hits`, or `drop_over_limit` if table is full).
    fn admitSeries(*_, metric_id: u32, label_fingerprint: u64, now_ms: u64): CardinalityDecision {
        _.total_admits += 1;
        const m_idx_opt = _.findMetricSlot(metric_id);
        if (m_idx_opt == null) {
            _.unknown_metric_count += 1;
            return CardinalityDecision.unknown_metric;
        }
        const m_idx: usize = m_idx_opt ?? 0;

        _.metrics[m_idx].total_admits += 1;
        hllObserve(&_.metrics[m_idx].hll_regs, label_fingerprint);

        // Compute open-addressing probe hash combining metric_id and label_fingerprint.
        const h1: u64 = fnv1aUpdateU64(FNV1A_OFFSET_BASIS, metric_id as u64);
        const series_hash: u64 = fnv1aUpdateU64(h1, label_fingerprint);
        const start_pos: usize = (series_hash as usize) & CARD_SERIES_MASK;

        let first_tombstone: usize = CARD_MAX_SERIES;
        let insert_pos: usize = CARD_MAX_SERIES;

        let step: usize = 0;
        while (step < CARD_MAX_SERIES) : (step += 1) {
            const pos: usize = (start_pos + step) & CARD_SERIES_MASK;
            const st: SeriesSlotState = _.series_table[pos].state;
            if (st == SeriesSlotState.empty) {
                insert_pos = if (first_tombstone < CARD_MAX_SERIES) first_tombstone else pos;
                break;
            }
            if (st == SeriesSlotState.tombstone) {
                if (first_tombstone == CARD_MAX_SERIES) {
                    first_tombstone = pos;
                }
            } else {
                // Occupied slot: check exact match.
                if (_.series_table[pos].metric_slot == (m_idx as u16) &&
                    _.series_table[pos].series_hash == series_hash &&
                    _.series_table[pos].label_fingerprint == label_fingerprint)
                {
                    if (now_ms > _.series_table[pos].last_seen_ms) {
                        _.series_table[pos].last_seen_ms = now_ms;
                    }
                    _.allow_existing_count += 1;
                    return CardinalityDecision.allow_existing;
                }
            }
        }

        if (insert_pos == CARD_MAX_SERIES && first_tombstone < CARD_MAX_SERIES) {
            insert_pos = first_tombstone;
        }

        // New series: check per-metric quota.
        const quota: MetricQuotaConfig = _.metrics[m_idx].config;
        const over_quota: bool = _.metrics[m_idx].active_series >= quota.max_series;

        if (over_quota) {
            if (quota.overflow_policy == CardinalityOverflowPolicy.rollup_overflow_label) {
                _.metrics[m_idx].overflow_active = true;
                _.metrics[m_idx].overflow_hits += 1;
                _.overflow_rollup_count += 1;
                return CardinalityDecision.rewrite_to_overflow;
            }
            if (quota.overflow_policy == CardinalityOverflowPolicy.drop) {
                _.metrics[m_idx].dropped_hits += 1;
                _.dropped_count += 1;
                return CardinalityDecision.drop_over_limit;
            }
            // warn_only: record warning and attempt admission into the series table.
            _.metrics[m_idx].warn_hits += 1;
            _.warn_only_count += 1;
        }

        // Guard against global series table exhaustion.
        if (insert_pos >= CARD_MAX_SERIES) {
            _.metrics[m_idx].dropped_hits += 1;
            _.dropped_count += 1;
            return CardinalityDecision.drop_over_limit;
        }

        if (_.series_table[insert_pos].state == SeriesSlotState.tombstone) {
            _.tombstone_slots -= 1;
        }
        _.series_table[insert_pos] = SeriesEntry{
            state: SeriesSlotState.occupied,
            metric_slot: m_idx as u16,
            series_hash: series_hash,
            label_fingerprint: label_fingerprint,
            last_seen_ms: now_ms,
        };
        _.metrics[m_idx].active_series += 1;
        _.active_series_total += 1;
        _.allow_new_count += 1;
        return CardinalityDecision.allow_new;
    }

    /// Evicts all active series whose `last_seen_ms < cutoff_ms` (TTL cold-series expiration).
    /// Returns the number of series evicted and reclaimed for future admission.
    fn evictStaleSeries(*_, cutoff_ms: u64): usize {
        let evicted: usize = 0;
        let i: usize = 0;
        while (i < CARD_MAX_SERIES) : (i += 1) {
            if (_.series_table[i].state == SeriesSlotState.occupied &&
                _.series_table[i].last_seen_ms < cutoff_ms)
            {
                const m_idx: usize = _.series_table[i].metric_slot as usize;
                _.series_table[i].state = SeriesSlotState.tombstone;
                _.series_table[i].series_hash = 0;
                _.series_table[i].label_fingerprint = 0;
                _.series_table[i].last_seen_ms = 0;

                if (m_idx < _.metric_count && _.metrics[m_idx].active_series > 0) {
                    _.metrics[m_idx].active_series -= 1;
                }
                if (_.active_series_total > 0) {
                    _.active_series_total -= 1;
                }
                _.tombstone_slots += 1;
                evicted += 1;
            }
        }
        _.evicted_series_total += evicted as u64;
        return evicted;
    }

    /// Rotates/resets the active series window and HyperLogLog registers for a single `metric_id`.
    /// Returns `false` if `metric_id` is not registered.
    fn resetMetricWindow(*_, metric_id: u32): bool {
        const m_idx_opt = _.findMetricSlot(metric_id);
        if (m_idx_opt == null) return false;
        const m_idx: usize = m_idx_opt ?? 0;

        let evicted: usize = 0;
        let i: usize = 0;
        while (i < CARD_MAX_SERIES) : (i += 1) {
            if (_.series_table[i].state == SeriesSlotState.occupied &&
                _.series_table[i].metric_slot == (m_idx as u16))
            {
                _.series_table[i].state = SeriesSlotState.tombstone;
                _.series_table[i].series_hash = 0;
                _.series_table[i].label_fingerprint = 0;
                _.series_table[i].last_seen_ms = 0;
                if (_.active_series_total > 0) {
                    _.active_series_total -= 1;
                }
                _.tombstone_slots += 1;
                evicted += 1;
            }
        }

        _.metrics[m_idx].active_series = 0;
        _.metrics[m_idx].overflow_active = false;
        let r: usize = 0;
        while (r < CARD_HLL_REGISTERS) : (r += 1) {
            _.metrics[m_idx].hll_regs[r] = 0;
        }
        _.evicted_series_total += evicted as u64;
        return true;
    }

    /// Returns the HyperLogLog estimated true cardinality for `metric_id`, or `null` if unknown.
    fn estimateMetricCardinality(metric_id: u32): f64? {
        const m_idx_opt = _.findMetricSlot(metric_id);
        if (m_idx_opt == null) return null;
        const m_idx: usize = m_idx_opt ?? 0;
        return hllEstimate(_.metrics[m_idx].hll_regs);
    }

    /// Returns a point-in-time snapshot for `metric_id`, or `null` if unregistered.
    fn getMetricSnapshot(metric_id: u32): MetricCardinalitySnapshot? {
        const m_idx_opt = _.findMetricSlot(metric_id);
        if (m_idx_opt == null) return null;
        const m_idx: usize = m_idx_opt ?? 0;
        const m = _.metrics[m_idx];
        return MetricCardinalitySnapshot{
            metric_id: m.config.metric_id,
            max_series: m.config.max_series,
            max_label_value_len: m.config.max_label_value_len,
            overflow_policy: m.config.overflow_policy,
            active_series: m.active_series,
            overflow_active: m.overflow_active,
            overflow_hits: m.overflow_hits,
            dropped_hits: m.dropped_hits,
            warn_hits: m.warn_hits,
            total_admits: m.total_admits,
            estimated_true_cardinality: hllEstimate(m.hll_regs),
        };
    }

    /// Checks whether a label value complies with `metric_id`'s `max_label_value_len`,
    /// returning `CARD_OVERFLOW_LABEL` (`"__overflow__"`) if oversized or `null` if unknown metric.
    fn guardLabelValue(metric_id: u32, value: []const u8): ([]const u8)? {
        const m_idx_opt = _.findMetricSlot(metric_id);
        if (m_idx_opt == null) return null;
        const m_idx: usize = m_idx_opt ?? 0;
        return labelValueOrOverflow(value, _.metrics[m_idx].config.max_label_value_len);
    }

    /// Returns global telemetry statistics across all registered metrics.
    fn stats(): CardinalityGuardStats {
        return CardinalityGuardStats{
            registered_metrics: _.metric_count,
            active_series_total: _.active_series_total,
            tombstone_slots: _.tombstone_slots,
            total_admits: _.total_admits,
            allow_existing_count: _.allow_existing_count,
            allow_new_count: _.allow_new_count,
            overflow_rollup_count: _.overflow_rollup_count,
            dropped_count: _.dropped_count,
            warn_only_count: _.warn_only_count,
            unknown_metric_count: _.unknown_metric_count,
            evicted_series_total: _.evicted_series_total,
        };
    }
}
