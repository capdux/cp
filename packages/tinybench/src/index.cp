// @cap/tinybench — Zero-heap micro-benchmark statistics engine in pure CP
// Clean-room MIT implementation of npm `tinybench` (#294, 172.3M/wk).

pub const BENCH_MAX_SAMPLES: usize = 512;

pub struct BenchStats {
    pub count: usize,
    pub total_ns: f64,
    pub mean_ns: f64,
    pub min_ns: f64,
    pub max_ns: f64,
    pub variance: f64,
    pub stddev_ns: f64,
    pub p50_ns: f64,
    pub p75_ns: f64,
    pub p95_ns: f64,
    pub p99_ns: f64,
    pub ops_per_sec: f64,
    pub rme_percent: f64,
}

fn tbnSqrt(x: f64): f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g = x;
    if (g < 1.0) {
        g = 1.0;
    }
    let i: usize = 0;
    while (i < 32) {
        g = 0.5 * (g + x / g);
        i += 1;
    }
    return g;
}

fn tbnSortAsc(arr: []f64) {
    // Insertion sort (fast and zero-alloc for <= 512 samples)
    let i: usize = 1;
    while (i < arr.len) {
        const key = arr[i];
        let j = i;
        while (j > 0 && arr[j - 1] > key) {
            arr[j] = arr[j - 1];
            j -= 1;
        }
        arr[j] = key;
        i += 1;
    }
}

fn tbnPercentile(sorted: []const f64, p: f64): f64 {
    if (sorted.len == 0) { return 0.0; }
    if (sorted.len == 1) { return sorted[0]; }
    const rank = p * ((sorted.len - 1) as f64);
    const lo = rank as usize;
    let hi = lo + 1;
    if (hi >= sorted.len) { hi = sorted.len - 1; }
    const frac = rank - (lo as f64);
    return sorted[lo] * (1.0 - frac) + sorted[hi] * frac;
}

/// Computes complete benchmark statistics from a slice of nanosecond sample timings.
pub fn computeStats(samples_ns: []const f64): BenchStats {
    if (samples_ns.len == 0) {
        return BenchStats {
            count: 0, total_ns: 0.0, mean_ns: 0.0, min_ns: 0.0, max_ns: 0.0,
            variance: 0.0, stddev_ns: 0.0, p50_ns: 0.0, p75_ns: 0.0, p95_ns: 0.0, p99_ns: 0.0,
            ops_per_sec: 0.0, rme_percent: 0.0,
        };
    }

    let n = samples_ns.len;
    if (n > BENCH_MAX_SAMPLES) {
        n = BENCH_MAX_SAMPLES;
    }
    let buf: [512]f64 = [0.0; 512];
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        buf[i] = samples_ns[i];
        total += samples_ns[i];
        i += 1;
    }

    tbnSortAsc(buf[0..n]);
    const mean = total / (n as f64);

    let sum_sq: f64 = 0.0;
    let k: usize = 0;
    while (k < n) {
        const diff = buf[k] - mean;
        sum_sq += diff * diff;
        k += 1;
    }
    let variance: f64 = 0.0;
    if (n > 1) {
        variance = sum_sq / ((n - 1) as f64);
    }
    const sd = tbnSqrt(variance);
    const sem = sd / tbnSqrt(n as f64);
    const moe = sem * 1.96; // 95% confidence z-score
    let rme: f64 = 0.0;
    if (mean > 0.0) {
        rme = (moe / mean) * 100.0;
    }
    let ops: f64 = 0.0;
    if (mean > 0.0) {
        ops = 1000000000.0 / mean;
    }

    return BenchStats {
        count: n,
        total_ns: total,
        mean_ns: mean,
        min_ns: buf[0],
        max_ns: buf[n - 1],
        variance: variance,
        stddev_ns: sd,
        p50_ns: tbnPercentile(buf[0..n], 0.50),
        p75_ns: tbnPercentile(buf[0..n], 0.75),
        p95_ns: tbnPercentile(buf[0..n], 0.95),
        p99_ns: tbnPercentile(buf[0..n], 0.99),
        ops_per_sec: ops,
        rme_percent: rme,
    };
}

/// Computes speedup ratio of `candidate` relative to `baseline` (>1.0 means candidate is faster).
pub fn speedupRatio(baseline: BenchStats, candidate: BenchStats): f64 {
    if (candidate.mean_ns <= 0.0 || baseline.mean_ns <= 0.0) {
        return 0.0;
    }
    return baseline.mean_ns / candidate.mean_ns;
}

export {
    BENCH_MAX_SAMPLES,
    BenchStats,
    computeStats,
    speedupRatio,
};
