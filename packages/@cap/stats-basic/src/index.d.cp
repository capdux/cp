// Type definitions for CP

export fn absF64(x: f64): f64;
export fn sqrtF64(x: f64): f64;
export class OnlineStats {
    count: u64,
    mean: f64,
    m2: f64,
    min_val: f64,
    max_val: f64,
    fn new(): OnlineStats;
    fn push(_: auto, x: f64);
    fn merge(_: auto, other: OnlineStats);
    fn variance(): f64;
    fn sampleVariance(): f64;
    fn stddev(): f64;
    fn sampleStddev(): f64;
}

export fn sum(xs: &[]f64): f64;
export fn mean(xs: &[]f64): f64;
export fn variance(xs: &[]f64): f64;
export fn stddev(xs: &[]f64): f64;
export fn minSlice(xs: &[]f64): f64;
export fn maxSlice(xs: &[]f64): f64;
export fn covariance(xs: &[]f64, ys: &[]f64): f64;
export fn pearsonR(xs: &[]f64, ys: &[]f64): f64;
export fn percentileSorted(sorted: &[]f64, p: f64): f64;
export fn medianSorted(sorted: &[]f64): f64;
