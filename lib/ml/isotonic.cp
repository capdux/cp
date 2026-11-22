// Origin: Pure-CP Stdlib (Zero-Alloc Isotonic Regression PAVA & Probability Calibration Metrics) | Created: 2026-11-22

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn clamp01(v: f64): f64 {
    if (v < 0.0) {
        return 0.0;
    }
    if (v > 1.0) {
        return 1.0;
    }
    return v;
}

/// Caller-owned zero-alloc workspace for Pool Adjacent Violators Algorithm (PAVA).
/// All 4 slices must have length `>= n`.
pub struct PavaWorkspace {
    block_sum_wy: []f64,
    block_sum_w: []f64,
    block_count: []usize,
    block_end: []usize,
}

/// Fitted piecewise-linear isotonic regression model backed by caller-owned knot slices.
pub struct IsotonicModel {
    x_knots: []f64,
    y_knots: []f64,
    knot_count: usize,
    increasing: bool,
    y_min: f64,
    y_max: f64,
}

fn pavaCore(
    y: []const f64,
    weights: []const f64,
    increasing: bool,
    y_out: *[]f64,
    ws: *PavaWorkspace,
): bool {
    const n = y.len;
    if (n == 0) {
        return false;
    }
    if (weights.len != 0 && weights.len != n) {
        return false;
    }
    if (
        y_out.*.len < n ||
        ws.block_sum_wy.len < n ||
        ws.block_sum_w.len < n ||
        ws.block_count.len < n ||
        ws.block_end.len < n
    ) {
        return false;
    }

    let i: usize = 0;
    while (i < n) {
        if (!isFiniteF64(y[i])) {
            return false;
        }
        if (weights.len == n) {
            if (!isFiniteF64(weights[i]) || weights[i] <= 0.0) {
                return false;
            }
        }
        i += 1;
    }

    let top: usize = 0;
    let idx: usize = 0;
    while (idx < n) {
        const w_i = if (weights.len == 0) 1.0 else weights[idx];
        ws.block_sum_wy[top] = w_i * y[idx];
        ws.block_sum_w[top] = w_i;
        ws.block_count[top] = 1 as usize;
        ws.block_end[top] = idx;
        top += 1;

        while (top >= 2) {
            const prev_mean = ws.block_sum_wy[top - 2] / ws.block_sum_w[top - 2];
            const curr_mean = ws.block_sum_wy[top - 1] / ws.block_sum_w[top - 1];
            const violated = if (increasing) (prev_mean > curr_mean) else (prev_mean < curr_mean);
            if (!violated) {
                break;
            }
            ws.block_sum_wy[top - 2] += ws.block_sum_wy[top - 1];
            ws.block_sum_w[top - 2] += ws.block_sum_w[top - 1];
            ws.block_count[top - 2] += ws.block_count[top - 1];
            ws.block_end[top - 2] = ws.block_end[top - 1];
            top -= 1;
        }
        idx += 1;
    }

    let start_idx: usize = 0;
    let b: usize = 0;
    while (b < top) {
        const mean_val = ws.block_sum_wy[b] / ws.block_sum_w[b];
        const end_idx = ws.block_end[b];
        let k: usize = start_idx;
        while (k <= end_idx) {
            y_out.*[k] = mean_val;
            k += 1;
        }
        start_idx = end_idx + 1;
        b += 1;
    }
    return true;
}

/// O(n) weighted Pool Adjacent Violators Algorithm (non-decreasing: `f[0] <= f[1] <= ... <= f[n-1]`).
/// Pass an empty slice for `weights` to use uniform unit weights.
pub fn pavaIncreasing(
    y: []const f64,
    weights: []const f64,
    y_out: *[]f64,
    ws: *PavaWorkspace,
): bool {
    return pavaCore(y, weights, true, y_out, ws);
}

/// O(n) weighted Pool Adjacent Violators Algorithm (non-increasing: `f[0] >= f[1] >= ... >= f[n-1]`).
/// Pass an empty slice for `weights` to use uniform unit weights.
pub fn pavaDecreasing(
    y: []const f64,
    weights: []const f64,
    y_out: *[]f64,
    ws: *PavaWorkspace,
): bool {
    return pavaCore(y, weights, false, y_out, ws);
}

fn orderLess(x: []const f64, a: usize, b: usize): bool {
    if (x[a] < x[b]) {
        return true;
    }
    if (x[a] > x[b]) {
        return false;
    }
    return a < b;
}

fn siftDown(x: []const f64, order: *[]usize, start: usize, end: usize) {
    let root = start;
    while (root * 2 + 1 <= end) {
        const child = root * 2 + 1;
        let swap_idx = root;
        if (orderLess(x, order.*[swap_idx], order.*[child])) {
            swap_idx = child;
        }
        if (child + 1 <= end && orderLess(x, order.*[swap_idx], order.*[child + 1])) {
            swap_idx = child + 1;
        }
        if (swap_idx == root) {
            return;
        }
        const tmp = order.*[root];
        order.*[root] = order.*[swap_idx];
        order.*[swap_idx] = tmp;
        root = swap_idx;
    }
}

fn heapSortIndices(x: []const f64, order: *[]usize, n: usize) {
    if (n < 2) {
        return;
    }
    let start: usize = (n - 2) / 2;
    while (true) {
        siftDown(x, order, start, n - 1);
        if (start == 0) {
            break;
        }
        start -= 1;
    }
    let end: usize = n - 1;
    while (end > 0) {
        const tmp = order.*[end];
        order.*[end] = order.*[0];
        order.*[0] = tmp;
        end -= 1;
        siftDown(x, order, 0, end);
    }
}

/// Fits a piecewise-linear isotonic regression model on arbitrarily ordered `(x, y)` samples.
/// Sorts samples in-place via `order_s`, aggregates duplicate `x` values by weighted mean,
/// and applies O(m) PAVA on the unique sorted knots.
pub fn fitIsotonic(
    x: []const f64,
    y: []const f64,
    weights: []const f64,
    increasing: bool,
    x_knots_out: *[]f64,
    y_knots_out: *[]f64,
    order_s: *[]usize,
    y_sorted_s: *[]f64,
    w_sorted_s: *[]f64,
    pava_ws: *PavaWorkspace,
): IsotonicModel? {
    const n = x.len;
    if (n == 0 || y.len != n) {
        return null;
    }
    if (weights.len != 0 && weights.len != n) {
        return null;
    }
    if (
        x_knots_out.*.len < n ||
        y_knots_out.*.len < n ||
        order_s.*.len < n ||
        y_sorted_s.*.len < n ||
        w_sorted_s.*.len < n ||
        pava_ws.block_sum_wy.len < n ||
        pava_ws.block_sum_w.len < n ||
        pava_ws.block_count.len < n ||
        pava_ws.block_end.len < n
    ) {
        return null;
    }

    let i: usize = 0;
    while (i < n) {
        if (!isFiniteF64(x[i]) || !isFiniteF64(y[i])) {
            return null;
        }
        if (weights.len == n) {
            if (!isFiniteF64(weights[i]) || weights[i] <= 0.0) {
                return null;
            }
        }
        order_s.*[i] = i;
        i += 1;
    }

    heapSortIndices(x, order_s, n);

    // Merge duplicate x coordinates by weighted average.
    let m: usize = 0;
    i = 0;
    while (i < n) {
        const idx0 = order_s.*[i];
        const cur_x = x[idx0];
        let sum_w: f64 = 0.0;
        let sum_wy: f64 = 0.0;
        while (i < n && x[order_s.*[i]] == cur_x) {
            const idx = order_s.*[i];
            const w_i = if (weights.len == 0) 1.0 else weights[idx];
            sum_w += w_i;
            sum_wy += w_i * y[idx];
            i += 1;
        }
        x_knots_out.*[m] = cur_x;
        w_sorted_s.*[m] = sum_w;
        y_sorted_s.*[m] = sum_wy / sum_w;
        m += 1;
    }

    let y_out_slice: []f64 = y_knots_out.*[0..m];
    const ok = pavaCore(y_sorted_s.*[0..m], w_sorted_s.*[0..m], increasing, &y_out_slice, pava_ws);
    if (!ok) {
        return null;
    }

    const y_first = y_knots_out.*[0];
    const y_last = y_knots_out.*[m - 1];
    const y_min = if (y_first < y_last) y_first else y_last;
    const y_max = if (y_first > y_last) y_first else y_last;

    return IsotonicModel {
        x_knots: x_knots_out.*,
        y_knots: y_knots_out.*,
        knot_count: m,
        increasing: increasing,
        y_min: y_min,
        y_max: y_max,
    };
}

/// Evaluates fitted `IsotonicModel` at `x_query` via piecewise-linear interpolation,
/// clamping to endpoint knot values outside `[x_knots[0], x_knots[knot_count - 1]]`.
pub fn isotonicPredict(model: IsotonicModel, x_query: f64): f64 {
    if (model.knot_count == 0) {
        return 0.0;
    }
    if (!isFiniteF64(x_query)) {
        return x_query;
    }
    if (model.knot_count == 1 || x_query <= model.x_knots[0]) {
        return model.y_knots[0];
    }
    const last = model.knot_count - 1;
    if (x_query >= model.x_knots[last]) {
        return model.y_knots[last];
    }

    // Binary search for lo such that x_knots[lo] <= x_query <= x_knots[lo + 1].
    let lo: usize = 0;
    let hi: usize = last;
    while (hi - lo > 1) {
        const mid = lo + (hi - lo) / 2;
        if (model.x_knots[mid] <= x_query) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    const x0 = model.x_knots[lo];
    const x1 = model.x_knots[lo + 1];
    const dx = x1 - x0;
    if (dx <= 0.0) {
        return model.y_knots[lo];
    }
    const t = (x_query - x0) / dx;
    return model.y_knots[lo] + t * (model.y_knots[lo + 1] - model.y_knots[lo]);
}

/// Computes Brier score `mean((probs[i] - y_binary[i])^2)` for probabilistic binary classification.
pub fn brierScore(probs: []const f64, y_binary: []const f64): f64? {
    const n = probs.len;
    if (n == 0 || y_binary.len != n) {
        return null;
    }
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        if (!isFiniteF64(probs[i]) || !isFiniteF64(y_binary[i])) {
            return null;
        }
        const diff = probs[i] - y_binary[i];
        sum += diff * diff;
        i += 1;
    }
    return sum / (n as f64);
}

/// Computes equal-width binned Expected Calibration Error (ECE) over `[0, 1]`.
/// All 3 bin scratch slices must have length `>= num_bins`.
pub fn expectedCalibrationError(
    probs: []const f64,
    y_binary: []const f64,
    num_bins: usize,
    bin_cnt_s: *[]usize,
    bin_prob_s: *[]f64,
    bin_acc_s: *[]f64,
): f64? {
    const n = probs.len;
    if (
        n == 0 ||
        y_binary.len != n ||
        num_bins == 0 ||
        bin_cnt_s.*.len < num_bins ||
        bin_prob_s.*.len < num_bins ||
        bin_acc_s.*.len < num_bins
    ) {
        return null;
    }

    let b: usize = 0;
    while (b < num_bins) {
        bin_cnt_s.*[b] = 0;
        bin_prob_s.*[b] = 0.0;
        bin_acc_s.*[b] = 0.0;
        b += 1;
    }

    let i: usize = 0;
    while (i < n) {
        if (!isFiniteF64(probs[i]) || !isFiniteF64(y_binary[i])) {
            return null;
        }
        const p = clamp01(probs[i]);
        let bin_idx = (@floor(p * (num_bins as f64))) as usize;
        if (bin_idx >= num_bins) {
            bin_idx = num_bins - 1;
        }
        bin_cnt_s.*[bin_idx] += 1;
        bin_prob_s.*[bin_idx] += p;
        bin_acc_s.*[bin_idx] += y_binary[i];
        i += 1;
    }

    let ece: f64 = 0.0;
    b = 0;
    while (b < num_bins) {
        if (bin_cnt_s.*[b] > 0) {
            ece += absF64(bin_acc_s.*[b] - bin_prob_s.*[b]);
        }
        b += 1;
    }
    return ece / (n as f64);
}
