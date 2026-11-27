// @cap/online-stats — Zero-allocation Welford streaming statistics, linear regression & percentiles for CP
// Clean-room MIT implementation inspired by Rust `average` / `criterion` statistical kernels.

const MAX_PERCENTILE_SAMPLES: usize = 256;

fn sqrtF64(x: f64) -> f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g = if (x >= 1.0) { x } else { 1.0 };
    // Fast initial magnitude reduction for very large/small inputs
    let i: usize = 0;
    while (i < 64) {
        const next = 0.5 * (g + (x / g));
        const diff = if (next > g) { next - g } else { g - next };
        g = next;
        if (diff <= 0.000000000000001 * g) {
            break;
        }
        i += 1;
    }
    return g;
}

pub struct RunningStats {
    pub n: u64,
    pub mean_val: f64,
    pub m2: f64,
    pub min_val: f64,
    pub max_val: f64,
    pub sum_val: f64,

    pub static fn init() -> RunningStats {
        return RunningStats{
            n: 0 as u64,
            mean_val: 0.0,
            m2: 0.0,
            min_val: 0.0,
            max_val: 0.0,
            sum_val: 0.0,
        };
    }

    pub fn push(*_, x: f64) {
        if (_.n == (0 as u64)) {
            _.min_val = x;
            _.max_val = x;
        } else {
            if (x < _.min_val) {
                _.min_val = x;
            }
            if (x > _.max_val) {
                _.max_val = x;
            }
        }
        _.n += 1 as u64;
        _.sum_val += x;
        const delta = x - _.mean_val;
        _.mean_val += delta / (_.n as f64);
        const delta2 = x - _.mean_val;
        _.m2 += delta * delta2;
    }

    pub fn merge(*_, other: RunningStats) {
        if (other.n == (0 as u64)) {
            return;
        }
        if (_.n == (0 as u64)) {
            _.n = other.n;
            _.mean_val = other.mean_val;
            _.m2 = other.m2;
            _.min_val = other.min_val;
            _.max_val = other.max_val;
            _.sum_val = other.sum_val;
            return;
        }
        const nA = _.n as f64;
        const nB = other.n as f64;
        const combinedN = _.n + other.n;
        const nTotal = combinedN as f64;
        const delta = other.mean_val - _.mean_val;

        _.mean_val = _.mean_val + (delta * (nB / nTotal));
        _.m2 = _.m2 + other.m2 + (delta * delta * ((nA * nB) / nTotal));
        if (other.min_val < _.min_val) {
            _.min_val = other.min_val;
        }
        if (other.max_val > _.max_val) {
            _.max_val = other.max_val;
        }
        _.sum_val += other.sum_val;
        _.n = combinedN;
    }

    pub fn count() -> u64 {
        return _.n;
    }

    pub fn mean() -> f64 {
        return _.mean_val;
    }

    pub fn varianceSample() -> f64 {
        if (_.n < (2 as u64)) {
            return 0.0;
        }
        return _.m2 / ((_.n - (1 as u64)) as f64);
    }

    pub fn variancePop() -> f64 {
        if (_.n == (0 as u64)) {
            return 0.0;
        }
        return _.m2 / (_.n as f64);
    }

    pub fn stddevSample() -> f64 {
        return sqrtF64(_.varianceSample());
    }

    pub fn stddevPop() -> f64 {
        return sqrtF64(_.variancePop());
    }

    pub fn min() -> f64 {
        return _.min_val;
    }

    pub fn max() -> f64 {
        return _.max_val;
    }

    pub fn sum() -> f64 {
        return _.sum_val;
    }
}

pub struct LinearFit {
    pub valid: bool,
    pub slope: f64,
    pub intercept: f64,
    pub r_squared: f64,
    pub pearson_r: f64,
}

pub fn linearRegression(xs: []const f64, ys: []const f64) -> LinearFit {
    if (xs.len != ys.len || xs.len < 2) {
        return LinearFit{
            valid: false,
            slope: 0.0,
            intercept: 0.0,
            r_squared: 0.0,
            pearson_r: 0.0,
        };
    }

    const n = xs.len as f64;
    let sumX: f64 = 0.0;
    let sumY: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        sumX += xs[i];
        sumY += ys[i];
        i += 1;
    }
    const meanX = sumX / n;
    const meanY = sumY / n;

    let sxx: f64 = 0.0;
    let syy: f64 = 0.0;
    let sxy: f64 = 0.0;
    let j: usize = 0;
    while (j < xs.len) {
        const dx = xs[j] - meanX;
        const dy = ys[j] - meanY;
        sxx += dx * dx;
        syy += dy * dy;
        sxy += dx * dy;
        j += 1;
    }

    if (sxx == 0.0) {
        return LinearFit{
            valid: false,
            slope: 0.0,
            intercept: 0.0,
            r_squared: 0.0,
            pearson_r: 0.0,
        };
    }

    const slope = sxy / sxx;
    const intercept = meanY - (slope * meanX);
    const r = if (syy > 0.0) { sxy / sqrtF64(sxx * syy) } else { 0.0 };
    return LinearFit{
        valid: true,
        slope: slope,
        intercept: intercept,
        r_squared: r * r,
        pearson_r: r,
    };
}

fn copyAndSort(values: []const f64, buf: *[MAX_PERCENTILE_SAMPLES]f64) -> usize {
    const n = values.len;
    let i: usize = 0;
    while (i < n) {
        buf[i] = values[i];
        i += 1;
    }
    // Insertion sort (zero heap allocation, O(n^2) fast for n <= 256)
    let k: usize = 1;
    while (k < n) {
        const key = buf[k];
        let j = k;
        while (j > 0 && buf[j - 1] > key) {
            buf[j] = buf[j - 1];
            j -= 1;
        }
        buf[j] = key;
        k += 1;
    }
    return n;
}

fn percentileFromSorted(sorted: *[MAX_PERCENTILE_SAMPLES]f64, n: usize, p: f64) -> f64 {
    if (n == 1 || p <= 0.0) {
        return sorted[0];
    }
    if (p >= 100.0) {
        return sorted[n - 1];
    }
    const pos = (p / 100.0) * ((n - 1) as f64);
    const lo = pos as usize;
    const hi = if (lo + 1 < n) { lo + 1 } else { lo };
    const frac = pos - (lo as f64);
    return sorted[lo] + (frac * (sorted[hi] - sorted[lo]));
}

pub fn percentile(values: []const f64, p: f64) -> f64? {
    if (values.len == 0 || values.len > MAX_PERCENTILE_SAMPLES || p < 0.0 || p > 100.0) {
        return null;
    }
    let buf: [MAX_PERCENTILE_SAMPLES]f64 = undefined;
    const n = copyAndSort(values, &buf);
    return percentileFromSorted(&buf, n, p);
}

pub fn median(values: []const f64) -> f64? {
    return percentile(values, 50.0);
}

pub fn iqr(values: []const f64) -> f64? {
    if (values.len == 0 || values.len > MAX_PERCENTILE_SAMPLES) {
        return null;
    }
    let buf: [MAX_PERCENTILE_SAMPLES]f64 = undefined;
    const n = copyAndSort(values, &buf);
    const q1 = percentileFromSorted(&buf, n, 25.0);
    const q3 = percentileFromSorted(&buf, n, 75.0);
    return q3 - q1;
}

export {
    RunningStats,
    LinearFit,
    linearRegression,
    percentile,
    median,
    iqr,
};
