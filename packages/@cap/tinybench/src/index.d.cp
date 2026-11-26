// Type definitions for CP

export class BenchStats {
    count: usize,
    total_ns: f64,
    mean_ns: f64,
    min_ns: f64,
    max_ns: f64,
    variance: f64,
    stddev_ns: f64,
    p50_ns: f64,
    p75_ns: f64,
    p95_ns: f64,
    p99_ns: f64,
    ops_per_sec: f64,
    rme_percent: f64,
}

export fn tbnSqrt(x: f64): f64;
export fn tbnSortAsc(arr: []f64);
export fn tbnPercentile(sorted: &[]f64, p: f64): f64;
export fn computeStats(samples_ns: &[]f64): BenchStats;
export fn speedupRatio(baseline: BenchStats, candidate: BenchStats): f64;
