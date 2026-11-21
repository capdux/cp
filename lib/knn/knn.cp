// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc K-Nearest Neighbors: Top-K Search, Classification, Regression & LOO-CV)

/// Supported vector distance metrics.
pub enum DistanceMetric {
    Euclidean,
    Manhattan,
    Chebyshev,
    Cosine,
}

/// Neighbor voting / regression weighting scheme.
pub enum WeightMode {
    Uniform,
    InverseDistance,
}

/// Single neighbor record returned by Top-K search.
pub struct Neighbor {
    index: usize,
    distance: f64,
}

fn isFiniteF64(v: f64): bool {
    const diff = v - v;
    return diff == 0.0;
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

/// Computes the distance between two equal-length feature vectors under `metric`.
/// Returns `-1.0` if vectors are empty, differ in length, or contain non-finite (`NaN`/`Inf`) values.
pub fn distanceMetric(a: []const f64, b: []const f64, metric: DistanceMetric): f64 {
    if (a.len == 0 || a.len != b.len) {
        return -1.0;
    }

    match (metric) {
        .Euclidean => {
            let sum_sq: f64 = 0.0;
            let i: usize = 0;
            while (i < a.len) {
                if (!isFiniteF64(a[i]) || !isFiniteF64(b[i])) {
                    return -1.0;
                }
                const d = a[i] - b[i];
                sum_sq += d * d;
                i += 1;
            }
            return @sqrt(sum_sq);
        },
        .Manhattan => {
            let sum: f64 = 0.0;
            let i: usize = 0;
            while (i < a.len) {
                if (!isFiniteF64(a[i]) || !isFiniteF64(b[i])) {
                    return -1.0;
                }
                sum += absF64(a[i] - b[i]);
                i += 1;
            }
            return sum;
        },
        .Chebyshev => {
            let max_d: f64 = 0.0;
            let i: usize = 0;
            while (i < a.len) {
                if (!isFiniteF64(a[i]) || !isFiniteF64(b[i])) {
                    return -1.0;
                }
                const d = absF64(a[i] - b[i]);
                if (d > max_d) {
                    max_d = d;
                }
                i += 1;
            }
            return max_d;
        },
        .Cosine => {
            let dot: f64 = 0.0;
            let norm_a: f64 = 0.0;
            let norm_b: f64 = 0.0;
            let i: usize = 0;
            while (i < a.len) {
                if (!isFiniteF64(a[i]) || !isFiniteF64(b[i])) {
                    return -1.0;
                }
                dot += a[i] * b[i];
                norm_a += a[i] * a[i];
                norm_b += b[i] * b[i];
                i += 1;
            }
            if (norm_a == 0.0 && norm_b == 0.0) {
                return 0.0;
            }
            if (norm_a == 0.0 || norm_b == 0.0) {
                return 1.0;
            }
            let cos_sim = dot / (@sqrt(norm_a) * @sqrt(norm_b));
            if (cos_sim > 1.0) {
                cos_sim = 1.0;
            } else if (cos_sim < -1.0) {
                cos_sim = -1.0;
            }
            return 1.0 - cos_sim;
        },
    }
}

/// Returns `true` if `a` is strictly worse (farther, or equal distance with larger index) than `b`.
fn neighborWorse(a: Neighbor, b: Neighbor): bool {
    if (a.distance != b.distance) {
        return a.distance > b.distance;
    }
    return a.index > b.index;
}

fn maxHeapSiftUp(heap: *[]Neighbor, start_idx: usize) {
    let idx = start_idx;
    while (idx > 0) {
        const parent = (idx - 1) >> 1;
        if (neighborWorse(heap.*[idx], heap.*[parent])) {
            const tmp = heap.*[idx];
            heap.*[idx] = heap.*[parent];
            heap.*[parent] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
}

fn maxHeapSiftDown(heap: *[]Neighbor, start_idx: usize, len: usize) {
    let idx = start_idx;
    while (true) {
        const left = (idx << 1) + 1;
        if (left >= len) {
            break;
        }
        let worst = left;
        const right = left + 1;
        if (right < len && neighborWorse(heap.*[right], heap.*[left])) {
            worst = right;
        }
        if (neighborWorse(heap.*[worst], heap.*[idx])) {
            const tmp = heap.*[idx];
            heap.*[idx] = heap.*[worst];
            heap.*[worst] = tmp;
            idx = worst;
        } else {
            break;
        }
    }
}

fn findKNearestExcluding(
    train_x: []const f64,
    n_samples: usize,
    n_features: usize,
    query: []const f64,
    k: usize,
    metric: DistanceMetric,
    exclude_idx: usize,
    out_neighbors: *[]Neighbor,
): usize? {
    if (n_samples == 0 || n_features == 0 || k == 0) {
        return null;
    }
    const available = if (exclude_idx < n_samples) n_samples - 1 else n_samples;
    if (k > available) {
        return null;
    }
    if (train_x.len < n_samples * n_features || query.len != n_features || out_neighbors.*.len < k) {
        return null;
    }

    let f: usize = 0;
    while (f < n_features) {
        if (!isFiniteF64(query[f])) {
            return null;
        }
        f += 1;
    }

    let heap_len: usize = 0;
    let i: usize = 0;
    while (i < n_samples) {
        if (i == exclude_idx) {
            i += 1;
            continue;
        }
        const row_start = i * n_features;
        const row = train_x[row_start .. row_start + n_features];
        const d = distanceMetric(row, query, metric);
        if (d < 0.0 || !isFiniteF64(d)) {
            return null;
        }

        const cand = Neighbor { index: i, distance: d };
        if (heap_len < k) {
            out_neighbors.*[heap_len] = cand;
            maxHeapSiftUp(out_neighbors, heap_len);
            heap_len += 1;
        } else if (neighborWorse(out_neighbors.*[0], cand)) {
            out_neighbors.*[0] = cand;
            maxHeapSiftDown(out_neighbors, 0, k);
        }

        i += 1;
    }

    // Heapsort in-place to ascending order by (distance, index)
    let end: usize = heap_len;
    while (end > 1) {
        end -= 1;
        const tmp = out_neighbors.*[0];
        out_neighbors.*[0] = out_neighbors.*[end];
        out_neighbors.*[end] = tmp;
        maxHeapSiftDown(out_neighbors, 0, end);
    }

    return heap_len;
}

/// Finds the `k` nearest neighbors of `query` in `train_x` (`n_samples x n_features` row-major),
/// sorted in ascending order of distance (ties broken deterministically by ascending sample index).
/// Returns `k` on success, or `null` on invalid dimensions, `k == 0`, `k > n_samples`, undersized buffer, or non-finite inputs.
pub fn findKNearest(
    train_x: []const f64,
    n_samples: usize,
    n_features: usize,
    query: []const f64,
    k: usize,
    metric: DistanceMetric,
    out_neighbors: *[]Neighbor,
): usize? {
    return findKNearestExcluding(train_x, n_samples, n_features, query, k, metric, n_samples, out_neighbors);
}

fn voteClassFromNeighbors(
    neighbors: []const Neighbor,
    train_labels: []const i32,
    weight_mode: WeightMode,
): i32 {
    // Exact zero-distance match immediately returns that neighbor's label
    if (neighbors[0].distance == 0.0) {
        return train_labels[neighbors[0].index];
    }

    let best_label: i32 = train_labels[neighbors[0].index];
    let best_score: f64 = -1.0;

    let i: usize = 0;
    while (i < neighbors.len) {
        const lbl = train_labels[neighbors[i].index];

        // Skip if already tallied by an earlier occurrence
        let seen = false;
        let prev: usize = 0;
        while (prev < i) {
            if (train_labels[neighbors[prev].index] == lbl) {
                seen = true;
                break;
            }
            prev += 1;
        }
        if (seen) {
            i += 1;
            continue;
        }

        let score: f64 = 0.0;
        let j: usize = i;
        while (j < neighbors.len) {
            if (train_labels[neighbors[j].index] == lbl) {
                match (weight_mode) {
                    .Uniform => {
                        score += 1.0;
                    },
                    .InverseDistance => {
                        score += 1.0 / neighbors[j].distance;
                    },
                }
            }
            j += 1;
        }

        if (score > best_score || (score == best_score && lbl < best_label)) {
            best_score = score;
            best_label = lbl;
        }
        i += 1;
    }

    return best_label;
}

/// Predicts the discrete `i32` class label of `query` using K-Nearest Neighbors voting.
/// Supports `Uniform` and `InverseDistance` weighting; an exact zero-distance match returns that sample's label immediately.
/// Returns `null` on invalid inputs or undersized buffers.
pub fn knnClassify(
    train_x: []const f64,
    train_labels: []const i32,
    n_samples: usize,
    n_features: usize,
    query: []const f64,
    k: usize,
    metric: DistanceMetric,
    weight_mode: WeightMode,
    neighbors_scratch: *[]Neighbor,
): i32? {
    if (train_labels.len < n_samples) {
        return null;
    }
    const found = findKNearestExcluding(
        train_x,
        n_samples,
        n_features,
        query,
        k,
        metric,
        n_samples,
        neighbors_scratch,
    ) ?? return null;
    return voteClassFromNeighbors(neighbors_scratch.*[0..found], train_labels, weight_mode);
}

/// Predicts the continuous `f64` target value of `query` using K-Nearest Neighbors regression.
/// Supports `Uniform` average and `InverseDistance` weighted average; exact zero-distance matches are averaged directly.
/// Returns `null` on invalid inputs, non-finite targets, or undersized buffers.
pub fn knnRegress(
    train_x: []const f64,
    train_y: []const f64,
    n_samples: usize,
    n_features: usize,
    query: []const f64,
    k: usize,
    metric: DistanceMetric,
    weight_mode: WeightMode,
    neighbors_scratch: *[]Neighbor,
): f64? {
    if (train_y.len < n_samples) {
        return null;
    }
    const found = findKNearestExcluding(
        train_x,
        n_samples,
        n_features,
        query,
        k,
        metric,
        n_samples,
        neighbors_scratch,
    ) ?? return null;

    // Validate selected targets are finite
    let i: usize = 0;
    while (i < found) {
        if (!isFiniteF64(train_y[neighbors_scratch.*[i].index])) {
            return null;
        }
        i += 1;
    }

    // Exact zero-distance hit(s): average all zero-distance neighbors
    if (neighbors_scratch.*[0].distance == 0.0) {
        let zero_sum: f64 = 0.0;
        let zero_cnt: usize = 0;
        while (zero_cnt < found && neighbors_scratch.*[zero_cnt].distance == 0.0) {
            zero_sum += train_y[neighbors_scratch.*[zero_cnt].index];
            zero_cnt += 1;
        }
        return zero_sum / (zero_cnt as f64);
    }

    match (weight_mode) {
        .Uniform => {
            let sum: f64 = 0.0;
            i = 0;
            while (i < found) {
                sum += train_y[neighbors_scratch.*[i].index];
                i += 1;
            }
            return sum / (found as f64);
        },
        .InverseDistance => {
            let w_sum: f64 = 0.0;
            let wy_sum: f64 = 0.0;
            i = 0;
            while (i < found) {
                const w = 1.0 / neighbors_scratch.*[i].distance;
                w_sum += w;
                wy_sum += w * train_y[neighbors_scratch.*[i].index];
                i += 1;
            }
            return wy_sum / w_sum;
        },
    }
}

/// Evaluates Leave-One-Out Cross-Validation (LOO-CV) classification accuracy in `[0.0, 1.0]`.
/// Requires `n_samples >= 2` and `1 <= k <= n_samples - 1`.
/// Returns `null` on invalid inputs or undersized buffers.
pub fn knnLeaveOneOutAccuracy(
    train_x: []const f64,
    train_labels: []const i32,
    n_samples: usize,
    n_features: usize,
    k: usize,
    metric: DistanceMetric,
    weight_mode: WeightMode,
    neighbors_scratch: *[]Neighbor,
): f64? {
    if (n_samples < 2 || n_features == 0 || k == 0 || k > n_samples - 1) {
        return null;
    }
    if (train_x.len < n_samples * n_features || train_labels.len < n_samples || neighbors_scratch.*.len < k) {
        return null;
    }

    let correct: usize = 0;
    let i: usize = 0;
    while (i < n_samples) {
        const row_start = i * n_features;
        const query = train_x[row_start .. row_start + n_features];
        const found = findKNearestExcluding(
            train_x,
            n_samples,
            n_features,
            query,
            k,
            metric,
            i,
            neighbors_scratch,
        ) ?? return null;
        const pred = voteClassFromNeighbors(neighbors_scratch.*[0..found], train_labels, weight_mode);
        if (pred == train_labels[i]) {
            correct += 1;
        }
        i += 1;
    }

    return (correct as f64) / (n_samples as f64);
}
