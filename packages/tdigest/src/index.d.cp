// Type definitions for CP

export class Centroid {
    mean: f64,
    weight: f64,
}

export fn emptyCentroid(): Centroid;
export class TDigest {
    centroids: [128]Centroid,
    centroid_len: usize,
    compression: f64,
    total_weight: f64,
    min_val: f64,
    max_val: f64,
    dirty: bool,
    fn init(comp: f64): TDigest;
    fn count(): f64;
    fn min(): f64;
    fn max(): f64;
    fn centroidCount(): usize;
    fn isEmpty(): bool;
    fn reset(_: auto);
    fn sortCentroids(_: auto);
    fn compressPass(_: auto, eff_comp: f64);
    fn compress(_: auto);
    fn addWeighted(_: auto, value: f64, weight: f64);
    fn add(_: auto, value: f64);
    fn quantile(_: auto, q: f64): f64;
    fn cdf(_: auto, x: f64): f64;
    fn merge(_: auto, other: &TDigest);
}

