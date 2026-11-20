// Source: cp/lib/math/stats.cp (Descriptive Statistics & Linear Regression)
// Pure CP descriptive statistics, quantiles, Pearson correlation, and OLS linear regression.

const std = @import("std");

/// Error set for statistical operations.
pub err StatsErr {
    EmptyInput,
    LengthMismatch,
    ZeroVariance,
}

/// Summary of descriptive statistics for a 1D floating-point sample.
pub struct StatsSummary {
    count: usize = 0;
    min: f64 = 0.0;
    max: f64 = 0.0;
    sum: f64 = 0.0;
    mean: f64 = 0.0;
    variance: f64 = 0.0;
    stddev: f64 = 0.0;
    median: f64 = 0.0;
    p95: f64 = 0.0;
    p99: f64 = 0.0;
}

/// Result of ordinary least squares simple linear regression (`y = slope * x + intercept`).
pub struct LinearRegressionResult {
    slope: f64 = 0.0;
    intercept: f64 = 0.0;
    r_squared: f64 = 0.0;

    /// Evaluates the fitted line at `x`.
    fn predict(x: f64): f64 {
        return _.slope * x + _.intercept;
    }
}

fn quantileSorted(sorted: []const f64, q: f64): f64 {
    if (sorted.len == 0) return 0.0;
    if (sorted.len == 1) return sorted[0];
    const pos = q * ((sorted.len - 1) as f64);
    const lo = pos as usize;
    const hi = if (lo + 1 < sorted.len) lo + 1 else lo;
    const frac = pos - (lo as f64);
    return sorted[lo] + (sorted[hi] - sorted[lo]) * frac;
}

/// Descriptive statistics and regression namespace.
pub static struct Stats {
    /// Computes count, min, max, sum, mean, population variance/stddev, median, p95, and p99.
    static fn summarize(alloc: std.mem.Allocator, data: []const f64): StatsSummary! {
        if (data.len == 0) return StatsErr.EmptyInput;

        let sorted = alloc.alloc(f64, data.len)?;
        defer alloc.free(sorted);
        @memcpy(sorted, data);
        zig {
            std.sort.pdq(f64, sorted, {}, std.sort.asc(f64));
        }

        // Welford single-pass mean & M2 for numerical stability
        let sum: f64 = 0.0;
        let mean: f64 = 0.0;
        let m2: f64 = 0.0;
        for (i of 0..data.len) {
            const x = data[i];
            sum += x;
            const k = (i + 1) as f64;
            const delta = x - mean;
            mean += delta / k;
            const delta2 = x - mean;
            m2 += delta * delta2;
        }

        const n_f = data.len as f64;
        const variance = m2 / n_f;
        const stddev = @sqrt(if (variance > 0.0) variance else 0.0);

        return StatsSummary {
            count: data.len,
            min: sorted[0],
            max: sorted[sorted.len - 1],
            sum: sum,
            mean: mean,
            variance: variance,
            stddev: stddev,
            median: quantileSorted(sorted, 0.50),
            p95: quantileSorted(sorted, 0.95),
            p99: quantileSorted(sorted, 0.99),
        };
    }

    /// Computes the Pearson product-moment correlation coefficient `r` in `[-1, 1]`.
    static fn pearsonCorrelation(x: []const f64, y: []const f64): f64! {
        if (x.len != y.len) return StatsErr.LengthMismatch;
        if (x.len == 0) return StatsErr.EmptyInput;

        let mean_x: f64 = 0.0;
        let mean_y: f64 = 0.0;
        const n_f = x.len as f64;
        for (i of 0..x.len) {
            mean_x += x[i];
            mean_y += y[i];
        }
        mean_x /= n_f;
        mean_y /= n_f;

        let s_xy: f64 = 0.0;
        let s_xx: f64 = 0.0;
        let s_yy: f64 = 0.0;
        for (i of 0..x.len) {
            const dx = x[i] - mean_x;
            const dy = y[i] - mean_y;
            s_xy += dx * dy;
            s_xx += dx * dx;
            s_yy += dy * dy;
        }

        const denom = @sqrt(s_xx * s_yy);
        if (denom <= 0.0) return 0.0;
        let r = s_xy / denom;
        if (r > 1.0) r = 1.0;
        if (r < -1.0) r = -1.0;
        return r;
    }

    /// Fits a simple ordinary least squares linear regression `y = slope * x + intercept`.
    static fn linearRegression(x: []const f64, y: []const f64): LinearRegressionResult! {
        if (x.len != y.len) return StatsErr.LengthMismatch;
        if (x.len == 0) return StatsErr.EmptyInput;

        let mean_x: f64 = 0.0;
        let mean_y: f64 = 0.0;
        const n_f = x.len as f64;
        for (i of 0..x.len) {
            mean_x += x[i];
            mean_y += y[i];
        }
        mean_x /= n_f;
        mean_y /= n_f;

        let s_xy: f64 = 0.0;
        let s_xx: f64 = 0.0;
        let s_yy: f64 = 0.0;
        for (i of 0..x.len) {
            const dx = x[i] - mean_x;
            const dy = y[i] - mean_y;
            s_xy += dx * dy;
            s_xx += dx * dx;
            s_yy += dy * dy;
        }

        if (s_xx <= 0.0) return StatsErr.ZeroVariance;
        const slope = s_xy / s_xx;
        const intercept = mean_y - slope * mean_x;
        let r_squared: f64 = 1.0;
        if (s_yy > 0.0) {
            r_squared = (s_xy * s_xy) / (s_xx * s_yy);
            if (r_squared > 1.0) r_squared = 1.0;
            if (r_squared < 0.0) r_squared = 0.0;
        }

        return LinearRegressionResult {
            slope: slope,
            intercept: intercept,
            r_squared: r_squared,
        };
    }
}

/// Computes the Pearson product-moment correlation coefficient `r` in `[-1, 1]`.
pub fn pearsonCorrelation(x: []const f64, y: []const f64): f64! {
    return Stats.pearsonCorrelation(x, y);
}

/// Fits a simple ordinary least squares linear regression `y = slope * x + intercept`.
pub fn linearRegression(x: []const f64, y: []const f64): LinearRegressionResult! {
    return Stats.linearRegression(x, y);
}
