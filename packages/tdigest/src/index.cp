// @cap/tdigest — Zero-allocation Merging T-Digest streaming quantile estimator for CP
// Clean-room MIT implementation inspired by Ted Dunning's T-Digest paper and Rust `tdigest`.

pub const MAX_CENTROIDS: usize = 128;
const TARGET_AFTER_COMPRESS: usize = 96;

pub struct Centroid {
    pub mean: f64,
    pub weight: f64,
}

fn emptyCentroid() -> Centroid {
    return Centroid{ mean: 0.0, weight: 0.0 };
}

pub struct TDigest {
    pub centroids: [128]Centroid,
    pub centroid_len: usize,
    pub compression: f64,
    pub total_weight: f64,
    pub min_val: f64,
    pub max_val: f64,
    pub dirty: bool,

    /// Initializes an empty `TDigest` with compression factor `comp` (defaults to `50.0` if `<= 1.0`).
    pub static fn init(comp: f64) -> TDigest {
        let centroids: [128]Centroid = undefined;
        let i: usize = 0;
        while (i < MAX_CENTROIDS) {
            centroids[i] = emptyCentroid();
            i += 1;
        }
        let eff_comp = comp;
        if (eff_comp <= 1.0) {
            eff_comp = 50.0;
        }
        return TDigest{
            centroids: centroids,
            centroid_len: 0,
            compression: eff_comp,
            total_weight: 0.0,
            min_val: 0.0,
            max_val: 0.0,
            dirty: false,
        };
    }

    pub fn count() -> f64 {
        return _.total_weight;
    }

    pub fn min() -> f64 {
        return _.min_val;
    }

    pub fn max() -> f64 {
        return _.max_val;
    }

    pub fn centroidCount() -> usize {
        return _.centroid_len;
    }

    pub fn isEmpty() -> bool {
        return _.total_weight <= 0.0 || _.centroid_len == 0;
    }

    pub fn reset(*_) {
        _.centroid_len = 0;
        _.total_weight = 0.0;
        _.min_val = 0.0;
        _.max_val = 0.0;
        _.dirty = false;
    }

    fn sortCentroids(*_) {
        // In-place insertion sort by `mean` (fast on mostly-sorted <= 128 elements)
        let i: usize = 1;
        while (i < _.centroid_len) {
            const key = _.centroids[i];
            let j = i;
            while (j > 0 && _.centroids[j - 1].mean > key.mean) {
                _.centroids[j] = _.centroids[j - 1];
                j -= 1;
            }
            _.centroids[j] = key;
            i += 1;
        }
    }

    fn compressPass(*_, eff_comp: f64) {
        if (_.centroid_len <= 1 || _.total_weight <= 0.0) {
            return;
        }
        const n = _.total_weight;
        let out_len: usize = 0;
        let cur_mean = _.centroids[0].mean;
        let cur_weight = _.centroids[0].weight;
        let weight_so_far: f64 = 0.0;

        let i: usize = 1;
        while (i < _.centroid_len) {
            const nxt = _.centroids[i];
            const proposed = cur_weight + nxt.weight;
            const q = (weight_so_far + proposed * 0.5) / n;
            const max_w = 4.0 * n * q * (1.0 - q) / eff_comp;

            if (proposed <= max_w || nxt.mean == cur_mean) {
                cur_mean = (cur_mean * cur_weight + nxt.mean * nxt.weight) / proposed;
                cur_weight = proposed;
            } else {
                _.centroids[out_len] = Centroid{ mean: cur_mean, weight: cur_weight };
                out_len += 1;
                weight_so_far += cur_weight;
                cur_mean = nxt.mean;
                cur_weight = nxt.weight;
            }
            i += 1;
        }
        _.centroids[out_len] = Centroid{ mean: cur_mean, weight: cur_weight };
        out_len += 1;
        _.centroid_len = out_len;
    }

    /// Sorts centroids by mean and merges adjacent centroids under the T-Digest `4*N*q*(1-q)/compression` bound.
    pub fn compress(*_) {
        if (_.centroid_len <= 1) {
            _.dirty = false;
            return;
        }
        _.sortCentroids();
        let eff_comp = _.compression;
        _.compressPass(eff_comp);

        // Guarantee headroom in the fixed pool so subsequent `add` calls never drop samples
        let rounds: usize = 0;
        while (_.centroid_len > TARGET_AFTER_COMPRESS && rounds < 6) {
            eff_comp *= 0.6;
            if (eff_comp < 2.0) {
                eff_comp = 2.0;
            }
            _.compressPass(eff_comp);
            rounds += 1;
        }
        _.dirty = false;
    }

    /// Adds a weighted sample `(value, weight)`. Ignores `weight <= 0.0`.
    pub fn addWeighted(*_, value: f64, weight: f64) {
        if (weight <= 0.0) {
            return;
        }
        if (_.centroid_len >= MAX_CENTROIDS) {
            _.compress();
        }
        if (_.total_weight <= 0.0) {
            _.min_val = value;
            _.max_val = value;
        } else {
            if (value < _.min_val) {
                _.min_val = value;
            }
            if (value > _.max_val) {
                _.max_val = value;
            }
        }

        if (_.centroid_len < MAX_CENTROIDS) {
            _.centroids[_.centroid_len] = Centroid{ mean: value, weight: weight };
            _.centroid_len += 1;
        } else {
            // Extreme fallback: merge into last centroid
            const last = MAX_CENTROIDS - 1;
            const prev_w = _.centroids[last].weight;
            const nw = prev_w + weight;
            _.centroids[last].mean = (_.centroids[last].mean * prev_w + value * weight) / nw;
            _.centroids[last].weight = nw;
        }
        _.total_weight += weight;
        _.dirty = true;
    }

    /// Adds a single unweighted sample (weight `1.0`).
    pub fn add(*_, value: f64) {
        _.addWeighted(value, 1.0);
    }

    /// Estimates the `q`-th quantile (`q` clamped to `[0.0, 1.0]`). Returns `0.0` if empty.
    pub fn quantile(*_, q: f64) -> f64 {
        if (_.centroid_len == 0 || _.total_weight <= 0.0) {
            return 0.0;
        }
        if (_.dirty) {
            _.compress();
        }
        if (q <= 0.0) {
            return _.min_val;
        }
        if (q >= 1.0) {
            return _.max_val;
        }
        if (_.centroid_len == 1) {
            return _.centroids[0].mean;
        }

        const n = _.total_weight;
        const idx = q * n;
        const first = _.centroids[0];
        if (idx < first.weight * 0.5) {
            const denom = first.weight * 0.5;
            if (denom <= 0.0) return first.mean;
            return _.min_val + (first.mean - _.min_val) * (idx / denom);
        }

        const last = _.centroids[_.centroid_len - 1];
        const last_mid = n - last.weight * 0.5;
        if (idx >= last_mid) {
            const denom = last.weight * 0.5;
            if (denom <= 0.0) return last.mean;
            return last.mean + (_.max_val - last.mean) * ((idx - last_mid) / denom);
        }

        let mid = first.weight * 0.5;
        let i: usize = 0;
        while (i + 1 < _.centroid_len) {
            const c0 = _.centroids[i];
            const c1 = _.centroids[i + 1];
            const delta_w = (c0.weight + c1.weight) * 0.5;
            const next_mid = mid + delta_w;
            if (idx <= next_mid) {
                if (delta_w <= 0.0) {
                    return (c0.mean + c1.mean) * 0.5;
                }
                const frac = (idx - mid) / delta_w;
                return c0.mean + (c1.mean - c0.mean) * frac;
            }
            mid = next_mid;
            i += 1;
        }

        return last.mean;
    }

    /// Estimates the cumulative distribution function `P(X <= x)` in `[0.0, 1.0]`.
    pub fn cdf(*_, x: f64) -> f64 {
        if (_.centroid_len == 0 || _.total_weight <= 0.0) {
            return 0.0;
        }
        if (_.dirty) {
            _.compress();
        }
        if (x < _.min_val) {
            return 0.0;
        }
        if (x >= _.max_val) {
            return 1.0;
        }
        if (_.centroid_len == 1) {
            return if (x < _.centroids[0].mean) 0.0 else 1.0;
        }

        const n = _.total_weight;
        const first = _.centroids[0];
        if (x < first.mean) {
            const span = first.mean - _.min_val;
            if (span <= 0.0) return (first.weight * 0.5) / n;
            return ((first.weight * 0.5) * ((x - _.min_val) / span)) / n;
        }

        const last = _.centroids[_.centroid_len - 1];
        if (x >= last.mean) {
            const span = _.max_val - last.mean;
            const base = n - last.weight * 0.5;
            if (span <= 0.0) return 1.0;
            return (base + (last.weight * 0.5) * ((x - last.mean) / span)) / n;
        }

        let mid = first.weight * 0.5;
        let i: usize = 0;
        while (i + 1 < _.centroid_len) {
            const c0 = _.centroids[i];
            const c1 = _.centroids[i + 1];
            const delta_w = (c0.weight + c1.weight) * 0.5;
            if (x <= c1.mean) {
                const span = c1.mean - c0.mean;
                if (span <= 0.0) {
                    return (mid + delta_w) / n;
                }
                const frac = (x - c0.mean) / span;
                return (mid + delta_w * frac) / n;
            }
            mid += delta_w;
            i += 1;
        }

        return 1.0;
    }

    /// Merges all centroids and exact min/max bounds from another `TDigest` into `self`.
    pub fn merge(*_, other: *const TDigest) {
        if (other.total_weight <= 0.0 || other.centroid_len == 0) {
            return;
        }
        const was_empty = (_.total_weight <= 0.0);
        let i: usize = 0;
        while (i < other.centroid_len) {
            const c = other.centroids[i];
            _.addWeighted(c.mean, c.weight);
            i += 1;
        }
        if (was_empty) {
            _.min_val = other.min_val;
            _.max_val = other.max_val;
        } else {
            if (other.min_val < _.min_val) _.min_val = other.min_val;
            if (other.max_val > _.max_val) _.max_val = other.max_val;
        }
        _.compress();
    }
}
