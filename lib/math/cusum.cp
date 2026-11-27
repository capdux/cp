// Origin: Pure-CP Stdlib (Zero-Alloc Online Change-Point & Streaming Anomaly Detection: CUSUM, Page-Hinkley, EWMA & Welford Z-Score) | Created: 2026-11-22

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

/// Direction of a detected mean shift, drift, or anomaly.
pub enum ShiftDirection {
    None,
    Positive,
    Negative,
}

/// Detected change point emitted by `scanCusum`.
pub struct ChangePoint {
    index: usize,
    direction: ShiftDirection,
    stat: f64,
}

/// Two-sided Page CUSUM change-point detector (zero-alloc streaming state machine).
pub struct CusumDetector {
    target_mean: f64,
    drift_k: f64,
    threshold_h: f64,
    g_pos: f64,
    g_neg: f64,
    count: usize,
    alarm_count: usize,
    reset_on_alarm: bool,

    /// Initializes a two-sided CUSUM detector.
    /// Requires finite parameters with `drift_k >= 0` and `threshold_h > 0`.
    static fn init(target_mean: f64, drift_k: f64, threshold_h: f64): CusumDetector? {
        if (
            !isFiniteF64(target_mean) ||
            !isFiniteF64(drift_k) ||
            drift_k < 0.0 ||
            !isFiniteF64(threshold_h) ||
            threshold_h <= 0.0
        ) {
            return null;
        }
        return CusumDetector {
            target_mean: target_mean,
            drift_k: drift_k,
            threshold_h: threshold_h,
            g_pos: 0.0,
            g_neg: 0.0,
            count: 0,
            alarm_count: 0,
            reset_on_alarm: true,
        };
    }

    /// Resets accumulated statistics and counters while preserving configuration.
    fn reset(*_) {
        _.g_pos = 0.0;
        _.g_neg = 0.0;
        _.count = 0;
        _.alarm_count = 0;
    }

    /// Feeds one observation `x` and returns the detected shift direction.
    fn update(*_, x: f64): ShiftDirection {
        if (!isFiniteF64(x)) {
            return ShiftDirection.None;
        }
        _.count += 1;
        const pos_next = _.g_pos + (x - _.target_mean) - _.drift_k;
        const neg_next = _.g_neg + (_.target_mean - x) - _.drift_k;
        _.g_pos = if (pos_next > 0.0) pos_next else 0.0;
        _.g_neg = if (neg_next > 0.0) neg_next else 0.0;

        if (_.g_pos > _.threshold_h && _.g_pos >= _.g_neg) {
            _.alarm_count += 1;
            if (_.reset_on_alarm) {
                _.g_pos = 0.0;
                _.g_neg = 0.0;
            }
            return ShiftDirection.Positive;
        }
        if (_.g_neg > _.threshold_h) {
            _.alarm_count += 1;
            if (_.reset_on_alarm) {
                _.g_pos = 0.0;
                _.g_neg = 0.0;
            }
            return ShiftDirection.Negative;
        }
        return ShiftDirection.None;
    }

    /// Returns current upper cumulative sum statistic `g_pos`.
    fn positiveStat(): f64 {
        return _.g_pos;
    }

    /// Returns current lower cumulative sum statistic `g_neg`.
    fn negativeStat(): f64 {
        return _.g_neg;
    }
}

/// Two-sided Page-Hinkley concept-drift detector (no prior target mean required).
pub struct PageHinkleyDetector {
    delta: f64,
    lambda: f64,
    min_samples: usize,
    mean: f64,
    sum_pos: f64,
    min_sum_pos: f64,
    sum_neg: f64,
    max_sum_neg: f64,
    count: usize,

    /// Initializes a Page-Hinkley drift detector (`delta >= 0`, `lambda > 0`).
    static fn init(delta: f64, lambda: f64, min_samples: usize): PageHinkleyDetector? {
        if (!isFiniteF64(delta) || delta < 0.0 || !isFiniteF64(lambda) || lambda <= 0.0) {
            return null;
        }
        return PageHinkleyDetector {
            delta: delta,
            lambda: lambda,
            min_samples: min_samples,
            mean: 0.0,
            sum_pos: 0.0,
            min_sum_pos: 0.0,
            sum_neg: 0.0,
            max_sum_neg: 0.0,
            count: 0,
        };
    }

    /// Resets running mean and cumulative drift statistics.
    fn reset(*_) {
        _.mean = 0.0;
        _.sum_pos = 0.0;
        _.min_sum_pos = 0.0;
        _.sum_neg = 0.0;
        _.max_sum_neg = 0.0;
        _.count = 0;
    }

    /// Feeds one observation `x` and returns `Positive` / `Negative` when drift exceeds `lambda`.
    fn update(*_, x: f64): ShiftDirection {
        if (!isFiniteF64(x)) {
            return ShiftDirection.None;
        }
        _.count += 1;
        _.mean += (x - _.mean) / (_.count as f64);

        _.sum_pos += (x - _.mean - _.delta);
        if (_.sum_pos < _.min_sum_pos) {
            _.min_sum_pos = _.sum_pos;
        }

        _.sum_neg += (x - _.mean + _.delta);
        if (_.sum_neg > _.max_sum_neg) {
            _.max_sum_neg = _.sum_neg;
        }

        if (_.count >= _.min_samples) {
            const ph_pos = _.sum_pos - _.min_sum_pos;
            const ph_neg = _.max_sum_neg - _.sum_neg;
            if (ph_pos > _.lambda && ph_pos >= ph_neg) {
                _.reset();
                return ShiftDirection.Positive;
            }
            if (ph_neg > _.lambda) {
                _.reset();
                return ShiftDirection.Negative;
            }
        }
        return ShiftDirection.None;
    }
}

/// Exponentially Weighted Moving Average (EWMA) control chart with exact Roberts time-varying limits.
pub struct EwmaControlChart {
    lambda: f64,
    target_mean: f64,
    sigma: f64,
    l_factor: f64,
    ewma: f64,
    step: usize,

    /// Initializes an EWMA control chart (`sigma > 0`, `0 < lambda <= 1`, `l_factor > 0`).
    static fn init(target_mean: f64, sigma: f64, lambda: f64, l_factor: f64): EwmaControlChart? {
        if (
            !isFiniteF64(target_mean) ||
            !isFiniteF64(sigma) ||
            sigma <= 0.0 ||
            !isFiniteF64(lambda) ||
            lambda <= 0.0 ||
            lambda > 1.0 ||
            !isFiniteF64(l_factor) ||
            l_factor <= 0.0
        ) {
            return null;
        }
        return EwmaControlChart {
            lambda: lambda,
            target_mean: target_mean,
            sigma: sigma,
            l_factor: l_factor,
            ewma: target_mean,
            step: 0,
        };
    }

    /// Resets the EWMA chart back to `target_mean` and `step = 0`.
    fn reset(*_) {
        _.ewma = _.target_mean;
        _.step = 0;
    }

    fn halfWidth(): f64 {
        if (_.step == 0) {
            return 0.0;
        }
        const asymp = _.lambda / (2.0 - _.lambda);
        let factor: f64 = 1.0;
        if (_.lambda < 1.0) {
            const one_minus = 1.0 - _.lambda;
            const pow_term = @exp((2.0 * (_.step as f64)) * @log(one_minus));
            factor = 1.0 - pow_term;
            if (factor < 0.0) {
                factor = 0.0;
            }
        }
        return _.l_factor * _.sigma * @sqrt(asymp * factor);
    }

    /// Feeds one observation `x` and checks whether the updated EWMA breaches UCL or LCL.
    fn update(*_, x: f64): ShiftDirection {
        if (!isFiniteF64(x)) {
            return ShiftDirection.None;
        }
        _.step += 1;
        _.ewma = _.lambda * x + (1.0 - _.lambda) * _.ewma;
        const hw = _.halfWidth();
        if (_.ewma > _.target_mean + hw) {
            return ShiftDirection.Positive;
        }
        if (_.ewma < _.target_mean - hw) {
            return ShiftDirection.Negative;
        }
        return ShiftDirection.None;
    }

    /// Returns the current EWMA statistic.
    fn value(): f64 {
        return _.ewma;
    }

    /// Returns the exact upper control limit (UCL) at the current step.
    fn upperControlLimit(): f64 {
        return _.target_mean + _.halfWidth();
    }

    /// Returns the exact lower control limit (LCL) at the current step.
    fn lowerControlLimit(): f64 {
        return _.target_mean - _.halfWidth();
    }
}

/// Welford streaming Z-Score anomaly detector (numerically stable against catastrophic cancellation).
pub struct StreamingZScore {
    min_samples: usize,
    z_threshold: f64,
    count: usize,
    mean: f64,
    m2: f64,

    /// Initializes a streaming Z-Score detector (`min_samples >= 2`, `z_threshold > 0`).
    static fn init(min_samples: usize, z_threshold: f64): StreamingZScore? {
        if (min_samples < 2 || !isFiniteF64(z_threshold) || z_threshold <= 0.0) {
            return null;
        }
        return StreamingZScore {
            min_samples: min_samples,
            z_threshold: z_threshold,
            count: 0,
            mean: 0.0,
            m2: 0.0,
        };
    }

    /// Resets running Welford statistics.
    fn reset(*_) {
        _.count = 0;
        _.mean = 0.0;
        _.m2 = 0.0;
    }

    /// Returns current running sample mean.
    fn currentMean(): f64 {
        return _.mean;
    }

    /// Returns current unbiased running sample standard deviation (`0.0` if `count < 2`).
    fn currentStd(): f64 {
        if (_.count < 2) {
            return 0.0;
        }
        const variance = _.m2 / ((_.count - 1) as f64);
        return if (variance > 0.0) @sqrt(variance) else 0.0;
    }

    /// Evaluates `x` against the current baseline distribution and updates Welford statistics.
    fn update(*_, x: f64): ShiftDirection {
        if (!isFiniteF64(x)) {
            return ShiftDirection.None;
        }

        let dir = ShiftDirection.None;
        if (_.count >= _.min_samples) {
            const sd = _.currentStd();
            const diff = x - _.mean;
            if (sd > 1.0e-15) {
                const z = diff / sd;
                if (z > _.z_threshold) {
                    dir = ShiftDirection.Positive;
                } else if (z < -_.z_threshold) {
                    dir = ShiftDirection.Negative;
                }
            } else {
                if (diff > 1.0e-15) {
                    dir = ShiftDirection.Positive;
                } else if (diff < -1.0e-15) {
                    dir = ShiftDirection.Negative;
                }
            }
        }

        _.count += 1;
        const delta = x - _.mean;
        _.mean += delta / (_.count as f64);
        const delta2 = x - _.mean;
        _.m2 += delta * delta2;
        if (_.m2 < 0.0) {
            _.m2 = 0.0;
        }

        return dir;
    }
}

/// Scans `series` with a two-sided CUSUM detector and writes detected change points into `out_points`.
/// Returns the number of change points written, or `null` on invalid parameters or non-finite values.
pub fn scanCusum(
    series: []const f64,
    target_mean: f64,
    drift_k: f64,
    threshold_h: f64,
    out_points: *[]ChangePoint,
): usize? {
    let det = CusumDetector.init(target_mean, drift_k, threshold_h) ?? return null;
    det.reset_on_alarm = false;

    let written: usize = 0;
    let i: usize = 0;
    while (i < series.len) {
        const x = series[i];
        if (!isFiniteF64(x)) {
            return null;
        }
        const dir = det.update(x);
        if (dir != ShiftDirection.None) {
            const stat_val = if (dir == ShiftDirection.Positive) det.positiveStat() else det.negativeStat();
            if (written < out_points.*.len) {
                out_points.*[written] = ChangePoint {
                    index: i,
                    direction: dir,
                    stat: stat_val,
                };
                written += 1;
            }
            det.g_pos = 0.0;
            det.g_neg = 0.0;
        }
        i += 1;
    }
    return written;
}
