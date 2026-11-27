// Type definitions for CP

export fn sqrtF64(x: f64): f64;
export class RunningStats {
    n: u64,
    mean_val: f64,
    m2: f64,
    min_val: f64,
    max_val: f64,
    sum_val: f64,
    fn init(): RunningStats;
    fn push(_: auto, x: f64);
    fn merge(_: auto, other: RunningStats);
    fn count(): u64;
    fn mean(): f64;
    fn varianceSample(): f64;
    fn variancePop(): f64;
    fn stddevSample(): f64;
    fn stddevPop(): f64;
    fn min(): f64;
    fn max(): f64;
    fn sum(): f64;
}

export class LinearFit {
    valid: bool,
    slope: f64,
    intercept: f64,
    r_squared: f64,
    pearson_r: f64,
}

export fn linearRegression(xs: &[]f64, ys: &[]f64): LinearFit;
export fn copyAndSort(values: &[]f64, buf: &mut [MAX_PERCENTILE_SAMPLES]f64): usize;
export fn percentileFromSorted(sorted: &mut [MAX_PERCENTILE_SAMPLES]f64, n: usize, p: f64): f64;
export fn percentile(values: &[]f64, p: f64): f64?;
export fn median(values: &[]f64): f64?;
export fn iqr(values: &[]f64): f64?;
