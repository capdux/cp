// Origin: Pure-CP Stdlib (Zero-Heap Online Welford/EWMA Streaming Anomaly Detector & Alert Cooldown Controller) | Created: 2026-11-22
// Pure CP zero-heap-allocation streaming anomaly detection & alert-storm suppression engine
// for microservice latency monitoring and AI Agent cost/error-rate spike detection:
//   1. `AnomalyConfig`, `AnomalySeverity` (`Normal | Warmup | Warning | AlertFired | SuppressedCooldown`),
//      `AnomalyEval`, `AnomalyDetectorStats`
//   2. `OnlineAnomalyDetector`:
//      - Numerically stable Welford online mean/variance + EWMA tracking
//      - Silent `warmup_samples` baseline learning with zero false positives
//      - Outlier-resistant baseline (anomalous spikes do not pollute Welford mean/variance)
//      - Zero-variance constant-series protection + consecutive-breach debouncing + `cooldown_ms` suppression

const std = @import("std");

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

/// Configuration for `OnlineAnomalyDetector`.
pub struct AnomalyConfig {
    /// Number of initial valid samples used to build the baseline before alerting (must be >= 2).
    warmup_samples: u64 = 30;
    /// Z-score threshold above which a sample counts as an anomaly breach (must be finite and > 0).
    z_threshold: f64 = 3.0;
    /// EWMA smoothing factor in `(0.0, 1.0]`.
    ewma_alpha: f64 = 0.2;
    /// Consecutive breaches required to escalate from `Warning` to `AlertFired` (must be >= 1).
    consecutive_breaches: u32 = 3;
    /// Minimum cooldown duration in milliseconds between consecutive `AlertFired` events (>= 0).
    cooldown_ms: i64 = 60000;
    /// Minimum standard deviation floor for constant/zero-variance series protection (> 0).
    min_stddev_eps: f64 = 0.000000001;

    static fn default(): AnomalyConfig {
        return AnomalyConfig{};
    }

    fn isValid(): bool {
        if (_.warmup_samples < 2 || _.consecutive_breaches == 0 || _.cooldown_ms < 0) return false;
        if (!isFiniteF64(_.z_threshold) || !(_.z_threshold > 0.0)) return false;
        if (!isFiniteF64(_.ewma_alpha) || !(_.ewma_alpha > 0.0) || _.ewma_alpha > 1.0) return false;
        if (!isFiniteF64(_.min_stddev_eps) || !(_.min_stddev_eps > 0.0)) return false;
        return true;
    }
}

/// Classification emitted for each observed metric sample.
pub enum AnomalySeverity {
    Normal,
    Warmup,
    Warning,
    AlertFired,
    SuppressedCooldown,
}

/// Per-sample evaluation result returned by `OnlineAnomalyDetector.observe`.
pub struct AnomalyEval {
    severity: AnomalySeverity = AnomalySeverity.Normal;
    z_score: f64 = 0.0;
    baseline_mean: f64 = 0.0;
    baseline_stddev: f64 = 0.0;
    ewma: f64 = 0.0;
    consecutive_count: u32 = 0;
    is_valid_sample: bool = true;
}

/// Telemetry snapshot for `OnlineAnomalyDetector`.
pub struct AnomalyDetectorStats {
    total_samples: u64 = 0;
    baseline_samples: u64 = 0;
    invalid_samples: u64 = 0;
    warning_count: u64 = 0;
    alerts_fired: u64 = 0;
    alerts_suppressed: u64 = 0;
    consecutive_breaches: u32 = 0;
    baseline_mean: f64 = 0.0;
    baseline_stddev: f64 = 0.0;
    ewma: f64 = 0.0;
}

/// Zero-heap-allocation streaming Welford/EWMA anomaly detector with breach debouncing & alert cooldown.
pub struct OnlineAnomalyDetector {
    config: AnomalyConfig;
    total_samples: u64 = 0;
    baseline_n: u64 = 0;
    invalid_samples: u64 = 0;
    mean: f64 = 0.0;
    m2: f64 = 0.0;
    ewma_val: f64 = 0.0;
    breach_streak: u32 = 0;
    warning_count: u64 = 0;
    alerts_fired: u64 = 0;
    alerts_suppressed: u64 = 0;
    has_fired_alert: bool = false;
    last_alert_ms: i64 = 0;

    /// Initializes a streaming anomaly detector. Returns `null` if `config` has invalid parameters.
    static fn init(config: AnomalyConfig): OnlineAnomalyDetector? {
        if (!config.isValid()) return null;
        return OnlineAnomalyDetector{
            config: config,
        };
    }

    /// Resets all accumulated baseline statistics, EWMA, breach streak, and cooldown state.
    fn reset(*_) {
        _.total_samples = 0;
        _.baseline_n = 0;
        _.invalid_samples = 0;
        _.mean = 0.0;
        _.m2 = 0.0;
        _.ewma_val = 0.0;
        _.breach_streak = 0;
        _.warning_count = 0;
        _.alerts_fired = 0;
        _.alerts_suppressed = 0;
        _.has_fired_alert = false;
        _.last_alert_ms = 0;
    }

    /// Returns the current Welford baseline mean.
    fn baselineMean(): f64 {
        return _.mean;
    }

    /// Returns the current Welford sample standard deviation (`0.0` when fewer than 2 baseline samples).
    fn baselineStddev(): f64 {
        if (_.baseline_n < 2) return 0.0;
        const variance: f64 = _.m2 / ((_.baseline_n - 1) as f64);
        if (!(variance > 0.0)) return 0.0;
        return @sqrt(variance);
    }

    /// Returns the current exponentially weighted moving average (EWMA).
    fn ewma(): f64 {
        return _.ewma_val;
    }

    fn updateWelford(*_, value: f64) {
        _.baseline_n += 1;
        const delta: f64 = value - _.mean;
        _.mean += delta / (_.baseline_n as f64);
        const delta2: f64 = value - _.mean;
        _.m2 += delta * delta2;
    }

    fn updateEwma(*_, value: f64) {
        if (_.total_samples == 0) {
            _.ewma_val = value;
        } else {
            _.ewma_val = _.config.ewma_alpha * value + (1.0 - _.config.ewma_alpha) * _.ewma_val;
        }
    }

    /// Observes one metric sample at `now_ms` and returns its anomaly severity and diagnostic snapshot.
    /// - Rejects `NaN`/`Inf` safely (`is_valid_sample = false`) without polluting baseline statistics.
    /// - During `warmup_samples`, updates baseline & EWMA silently and returns `Warmup`.
    /// - After warmup, protects the Welford baseline from extreme outliers, debounces single-point
    ///   glitches via `consecutive_breaches`, and suppresses alert storms via `cooldown_ms`.
    fn observe(*_, value: f64, now_ms: i64): AnomalyEval {
        if (!isFiniteF64(value)) {
            _.invalid_samples += 1;
            return AnomalyEval{
                severity: AnomalySeverity.Normal,
                z_score: 0.0,
                baseline_mean: _.mean,
                baseline_stddev: _.baselineStddev(),
                ewma: _.ewma_val,
                consecutive_count: _.breach_streak,
                is_valid_sample: false,
            };
        }

        // 1. Warmup phase: build clean baseline silently
        if (_.total_samples < _.config.warmup_samples) {
            _.updateWelford(value);
            _.updateEwma(value);
            _.total_samples += 1;
            return AnomalyEval{
                severity: AnomalySeverity.Warmup,
                z_score: 0.0,
                baseline_mean: _.mean,
                baseline_stddev: _.baselineStddev(),
                ewma: _.ewma_val,
                consecutive_count: 0,
                is_valid_sample: true,
            };
        }

        // 2. Active detection phase
        const mean_before: f64 = _.mean;
        const std_before: f64 = _.baselineStddev();
        const diff: f64 = value - mean_before;

        let z_score: f64 = 0.0;
        if (std_before <= _.config.min_stddev_eps) {
            if (absF64(diff) <= _.config.min_stddev_eps) {
                z_score = 0.0;
            } else {
                const sentinel_mag: f64 = if (_.config.z_threshold * 2.0 > 1000.0) (_.config.z_threshold * 2.0) else 1000.0;
                z_score = if (diff > 0.0) sentinel_mag else -sentinel_mag;
            }
        } else {
            z_score = diff / std_before;
        }

        _.updateEwma(value);
        _.total_samples += 1;

        if (absF64(z_score) < _.config.z_threshold) {
            _.breach_streak = 0;
            _.updateWelford(value);
            return AnomalyEval{
                severity: AnomalySeverity.Normal,
                z_score: z_score,
                baseline_mean: _.mean,
                baseline_stddev: _.baselineStddev(),
                ewma: _.ewma_val,
                consecutive_count: 0,
                is_valid_sample: true,
            };
        }

        // Breach detected: do NOT pollute Welford baseline with outlier
        _.breach_streak += 1;
        if (_.breach_streak < _.config.consecutive_breaches) {
            _.warning_count += 1;
            return AnomalyEval{
                severity: AnomalySeverity.Warning,
                z_score: z_score,
                baseline_mean: mean_before,
                baseline_stddev: std_before,
                ewma: _.ewma_val,
                consecutive_count: _.breach_streak,
                is_valid_sample: true,
            };
        }

        // Debounce threshold met: apply cooldown storm suppression
        const in_cooldown: bool = _.has_fired_alert &&
            _.config.cooldown_ms > 0 &&
            (now_ms < _.last_alert_ms || (now_ms - _.last_alert_ms) < _.config.cooldown_ms);

        if (in_cooldown) {
            _.alerts_suppressed += 1;
            return AnomalyEval{
                severity: AnomalySeverity.SuppressedCooldown,
                z_score: z_score,
                baseline_mean: mean_before,
                baseline_stddev: std_before,
                ewma: _.ewma_val,
                consecutive_count: _.breach_streak,
                is_valid_sample: true,
            };
        }

        _.has_fired_alert = true;
        _.last_alert_ms = now_ms;
        _.alerts_fired += 1;
        return AnomalyEval{
            severity: AnomalySeverity.AlertFired,
            z_score: z_score,
            baseline_mean: mean_before,
            baseline_stddev: std_before,
            ewma: _.ewma_val,
            consecutive_count: _.breach_streak,
            is_valid_sample: true,
        };
    }

    /// Returns a point-in-time telemetry snapshot.
    fn stats(): AnomalyDetectorStats {
        return AnomalyDetectorStats{
            total_samples: _.total_samples,
            baseline_samples: _.baseline_n,
            invalid_samples: _.invalid_samples,
            warning_count: _.warning_count,
            alerts_fired: _.alerts_fired,
            alerts_suppressed: _.alerts_suppressed,
            consecutive_breaches: _.breach_streak,
            baseline_mean: _.mean,
            baseline_stddev: _.baselineStddev(),
            ewma: _.ewma_val,
        };
    }
}
