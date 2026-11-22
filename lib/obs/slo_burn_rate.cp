// Origin: Pure-CP Stdlib (Zero-Alloc Google SRE Multi-Window Multi-Burn-Rate SLO Error Budget Monitor) | Created: 2026-11-22
// Pure CP zero-heap-allocation Google SRE Workbook Ch. 5 Multi-Window Multi-Burn-Rate SLO Alerting Engine:
//   - Fixed 360-minute (6-hour) inline ring buffer (`SLO_RING_MINUTES = 360`), 0 dynamic heap allocation
//   - Automatic bucket expiry on clock advance + out-of-order support within window + stale event rejection
//   - Dual-window alert gating eliminating long-tail flapping:
//       * `page_critical_burn`: 1h AND 5m burn >= critical_burn_rate (default 14.4x -> 2% 30d budget in 1h)
//       * `page_high_burn`:     6h AND 30m burn >= high_burn_rate    (default 6.0x  -> 5% 30d budget in 6h)
//       * `ticket_slow_burn`:   6h AND 30m burn >= slow_burn_rate    (default 1.0x  -> 100% 30d budget in 30d)
//   - Cold-start low-traffic guard (`min_samples_short_win`), 30-day budget consumption ratio & hours to exhaustion

const std = @import("std");

/// Fixed 6-hour (360 one-minute buckets) inline ring buffer capacity (zero dynamic heap).
pub const SLO_RING_MINUTES: usize = 360;

const SLO_30D_HOURS: f64 = 720.0;
const NO_EXHAUSTION_HOURS: f64 = 1_000_000.0;
const BURN_EPS: f64 = 1.0e-12;

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return a + b;
}

// ============================================================================
// 1. Public Enums, Configuration & Evaluation Models
// ============================================================================

/// Multi-window multi-burn-rate SLO alert severity.
pub enum SloAlertSeverity {
    ok,
    ticket_slow_burn,
    page_high_burn,
    page_critical_burn,
}

/// Configuration for `SloBurnMonitor` following Google SRE Workbook Chapter 5.
pub struct SloBurnConfig {
    /// Target availability ratio strictly in `(0.0, 1.0)` (e.g., `0.999` = 99.9% SLO, 0.1% error budget).
    target_availability_f64: f64 = 0.999;
    /// 1h + 5m critical paging burn rate threshold (default `14.4`).
    critical_burn_rate: f64 = 14.4;
    /// 6h + 30m high paging burn rate threshold (default `6.0`).
    high_burn_rate: f64 = 6.0;
    /// 6h + 30m slow ticket burn rate threshold (default `1.0`).
    slow_burn_rate: f64 = 1.0;
    /// Minimum total events required in the short window (`5m` / `30m`) before firing alerts.
    min_samples_short_win: u64 = 20;

    /// Returns Google SRE 99.9% ("three nines") default thresholds:
    /// `target = 0.999`, `critical = 14.4`, `high = 6.0`, `slow = 1.0`, `min_samples_short_win = 20`.
    static fn defaultsThreeNines(): SloBurnConfig {
        return SloBurnConfig{};
    }

    /// Validates that `target_availability_f64 in (0.0, 1.0)` and
    /// `0.0 < slow_burn_rate <= high_burn_rate <= critical_burn_rate` (all finite).
    fn isValid(): bool {
        if (!isFiniteF64(_.target_availability_f64)) return false;
        if (_.target_availability_f64 <= 0.0 || _.target_availability_f64 >= 1.0) return false;
        if (!isFiniteF64(_.slow_burn_rate) || !isFiniteF64(_.high_burn_rate) || !isFiniteF64(_.critical_burn_rate)) return false;
        if (_.slow_burn_rate <= 0.0) return false;
        if (_.high_burn_rate < _.slow_burn_rate || _.critical_burn_rate < _.high_burn_rate) return false;
        return true;
    }
}

/// Aggregated event counts, error rate, and SLO burn rate over a specific sliding window.
pub struct SloWindowMetrics {
    good_events: u64 = 0;
    bad_events: u64 = 0;
    total_events: u64 = 0;
    error_rate_f64: f64 = 0.0;
    burn_rate_f64: f64 = 0.0;
}

/// Complete multi-window SLO burn evaluation returned by `SloBurnMonitor.evaluate`.
pub struct SloBurnEvaluation {
    severity: SloAlertSeverity = SloAlertSeverity.ok;
    win_5m: SloWindowMetrics = SloWindowMetrics{},
    win_30m: SloWindowMetrics = SloWindowMetrics{},
    win_1h: SloWindowMetrics = SloWindowMetrics{},
    win_6h: SloWindowMetrics = SloWindowMetrics{},
    /// Fraction of the 30-day error budget consumed over the trailing 6-hour window (`win_6h.burn_rate * 6 / 720`).
    error_budget_consumed_ratio: f64 = 0.0;
    /// Estimated hours until 30-day error budget exhaustion at the active burn rate (`1_000_000.0` when burn == 0).
    hours_to_budget_exhaustion: f64 = NO_EXHAUSTION_HOURS;
}

struct SloMinuteBucket {
    minute_epoch: u64 = 0;
    good: u64 = 0;
    bad: u64 = 0;
    valid: bool = false;
}

// ============================================================================
// 2. Zero-Alloc 360-Minute Ring Buffer SLO Burn Monitor
// ============================================================================

/// Fixed-memory (360 one-minute buckets) Google SRE multi-window SLO burn rate monitor.
pub struct SloBurnMonitor {
    cfg: SloBurnConfig = SloBurnConfig{},
    buckets: [SLO_RING_MINUTES]SloMinuteBucket = [SloMinuteBucket{}; SLO_RING_MINUTES],
    latest_minute: u64 = 0;
    has_data: bool = false;

    /// Initializes a validated `SloBurnMonitor`, returning `null` if `cfg` is invalid.
    static fn init(cfg: SloBurnConfig): SloBurnMonitor? {
        if (!cfg.isValid()) return null;
        return SloBurnMonitor{ cfg: cfg };
    }

    /// Clears all 360 minute buckets and resets clock state.
    fn reset(*_) {
        let i: usize = 0;
        while (i < SLO_RING_MINUTES) : (i += 1) {
            _.buckets[i] = SloMinuteBucket{};
        }
        _.latest_minute = 0;
        _.has_data = false;
    }

    /// Records `good_count` and `bad_count` events into the bucket for `minute_epoch`.
    /// Automatically advances and zeroes expired buckets on forward time jumps.
    /// Safely rejects stale events older than `SLO_RING_MINUTES` behind `latest_minute` (returns `false`).
    fn record_events(*_, minute_epoch: u64, good_count: u64, bad_count: u64): bool {
        const ring_u64: u64 = SLO_RING_MINUTES as u64;
        if (!_.has_data) {
            _.has_data = true;
            _.latest_minute = minute_epoch;
        } else if (minute_epoch > _.latest_minute) {
            const advance: u64 = minute_epoch - _.latest_minute;
            if (advance >= ring_u64) {
                let i: usize = 0;
                while (i < SLO_RING_MINUTES) : (i += 1) {
                    _.buckets[i] = SloMinuteBucket{};
                }
            } else {
                let step: u64 = 1;
                while (step <= advance) : (step += 1) {
                    const m: u64 = _.latest_minute + step;
                    const idx: usize = (m % ring_u64) as usize;
                    _.buckets[idx] = SloMinuteBucket{};
                }
            }
            _.latest_minute = minute_epoch;
        } else {
            const age: u64 = _.latest_minute - minute_epoch;
            if (age >= ring_u64) {
                return false;
            }
        }

        const slot: usize = (minute_epoch % ring_u64) as usize;
        if (!_.buckets[slot].valid || _.buckets[slot].minute_epoch != minute_epoch) {
            _.buckets[slot] = SloMinuteBucket{
                minute_epoch: minute_epoch,
                good: good_count,
                bad: bad_count,
                valid: true,
            };
        } else {
            _.buckets[slot].good = satAddU64(_.buckets[slot].good, good_count);
            _.buckets[slot].bad = satAddU64(_.buckets[slot].bad, bad_count);
        }
        return true;
    }

    /// Computes aggregated `SloWindowMetrics` across the trailing `span_minutes` ending at `now_minute` (inclusive).
    fn window_metrics(now_minute: u64, span_minutes: usize): SloWindowMetrics {
        if (!_.has_data || span_minutes == 0) {
            return SloWindowMetrics{};
        }
        const span: usize = if (span_minutes > SLO_RING_MINUTES) SLO_RING_MINUTES else span_minutes;
        const ring_u64: u64 = SLO_RING_MINUTES as u64;

        let good_sum: u64 = 0;
        let bad_sum: u64 = 0;
        let off: usize = 0;
        while (off < span) : (off += 1) {
            const off_u64: u64 = off as u64;
            if (off_u64 > now_minute) break;
            const target_m: u64 = now_minute - off_u64;
            const idx: usize = (target_m % ring_u64) as usize;
            const b = _.buckets[idx];
            if (b.valid && b.minute_epoch == target_m) {
                good_sum = satAddU64(good_sum, b.good);
                bad_sum = satAddU64(bad_sum, b.bad);
            }
        }

        const total: u64 = satAddU64(good_sum, bad_sum);
        if (total == 0) {
            return SloWindowMetrics{};
        }
        const err_rate: f64 = (bad_sum as f64) / (total as f64);
        const err_budget: f64 = 1.0 - _.cfg.target_availability_f64;
        const burn: f64 = if (err_budget > BURN_EPS) (err_rate / err_budget) else 0.0;

        return SloWindowMetrics{
            good_events: good_sum,
            bad_events: bad_sum,
            total_events: total,
            error_rate_f64: err_rate,
            burn_rate_f64: burn,
        };
    }

    /// Evaluates all 4 SRE windows (`5m`, `30m`, `1h`, `6h`) at `now_minute` and returns
    /// the deterministic dual-window `SloAlertSeverity`, 30-day budget consumption ratio,
    /// and estimated hours to budget exhaustion.
    fn evaluate(now_minute: u64): SloBurnEvaluation {
        const w5 = _.window_metrics(now_minute, 5);
        const w30 = _.window_metrics(now_minute, 30);
        const w60 = _.window_metrics(now_minute, 60);
        const w360 = _.window_metrics(now_minute, 360);

        let sev: SloAlertSeverity = SloAlertSeverity.ok;
        if (
            w5.total_events >= _.cfg.min_samples_short_win &&
            w60.burn_rate_f64 >= _.cfg.critical_burn_rate &&
            w5.burn_rate_f64 >= _.cfg.critical_burn_rate
        ) {
            sev = SloAlertSeverity.page_critical_burn;
        } else if (
            w30.total_events >= _.cfg.min_samples_short_win &&
            w360.burn_rate_f64 >= _.cfg.high_burn_rate &&
            w30.burn_rate_f64 >= _.cfg.high_burn_rate
        ) {
            sev = SloAlertSeverity.page_high_burn;
        } else if (
            w30.total_events >= _.cfg.min_samples_short_win &&
            w360.burn_rate_f64 >= _.cfg.slow_burn_rate &&
            w30.burn_rate_f64 >= _.cfg.slow_burn_rate
        ) {
            sev = SloAlertSeverity.ticket_slow_burn;
        }

        // 6-hour trailing consumption fraction of 30-day (720h) error budget
        const consumed_ratio: f64 = w360.burn_rate_f64 * (6.0 / SLO_30D_HOURS);
        const active_burn: f64 = if (w60.burn_rate_f64 > BURN_EPS) w60.burn_rate_f64 else w360.burn_rate_f64;
        const hours_left: f64 = if (active_burn > BURN_EPS) (SLO_30D_HOURS / active_burn) else NO_EXHAUSTION_HOURS;

        return SloBurnEvaluation{
            severity: sev,
            win_5m: w5,
            win_30m: w30,
            win_1h: w60,
            win_6h: w360,
            error_budget_consumed_ratio: consumed_ratio,
            hours_to_budget_exhaustion: hours_left,
        };
    }
}
