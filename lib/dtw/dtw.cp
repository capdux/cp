// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Dynamic Time Warping: 1D/Multi-D DTW, Sakoe-Chiba Band, Path Alignment, LB_Keogh & Z-Normalization)

/// Sentinel infinity cost for unreachable DP cells outside the Sakoe-Chiba band.
pub const DTW_INF: f64 = 1.0e300;

/// Single alignment pair `(i, j)` mapping `a[i]` to `b[j]`.
pub struct WarpPair {
    i: usize,
    j: usize,
}

/// Optimal DTW distance and warping path length returned by `dtwAlign1D`.
pub struct DtwPathResult {
    distance: f64,
    path_len: usize,
}

fn isFiniteF64(v: f64): bool {
    const diff = v - v;
    return diff == 0.0;
}

fn allFinite(s: []const f64, len: usize): bool {
    let i: usize = 0;
    while (i < len) {
        if (!isFiniteF64(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn effectiveWindow(n: usize, m: usize, window: usize): usize {
    const max_len = if (n > m) n else m;
    if (window == 0) {
        return max_len;
    }
    const diff = if (n > m) n - m else m - n;
    return if (window < diff) diff else window;
}

/// Computes the 1D L2 Dynamic Time Warping distance using O(m) rolling two-row buffers.
/// - `window == 0` disables band constraints; if `0 < window < |a.len - b.len|`, automatically widens to `|a.len - b.len|` so `(0,0) -> (n-1,m-1)` remains reachable.
/// - Requires `prev_row_s.len >= b.len + 1` and `curr_row_s.len >= b.len + 1`.
/// - Returns `null` on empty sequences, undersized buffers, or non-finite inputs.
pub fn dtwDistance1D(
    a: []const f64,
    b: []const f64,
    window: usize,
    prev_row_s: *[]f64,
    curr_row_s: *[]f64,
): f64? {
    const n = a.len;
    const m = b.len;
    if (n == 0 || m == 0 || prev_row_s.*.len < m + 1 || curr_row_s.*.len < m + 1) {
        return null;
    }
    if (!allFinite(a, n) || !allFinite(b, m)) {
        return null;
    }

    const eff_w = effectiveWindow(n, m, window);
    let j: usize = 0;
    while (j <= m) {
        prev_row_s.*[j] = DTW_INF;
        j += 1;
    }
    prev_row_s.*[0] = 0.0;

    let i: usize = 1;
    while (i <= n) {
        let jc: usize = 0;
        while (jc <= m) {
            curr_row_s.*[jc] = DTW_INF;
            jc += 1;
        }

        const j_start: usize = if (i > eff_w) i - eff_w else 1;
        const j_end_raw = i + eff_w;
        const j_end: usize = if (j_end_raw < m) j_end_raw else m;
        const ai = a[i - 1];

        let col: usize = j_start;
        while (col <= j_end) {
            const diff = ai - b[col - 1];
            const cost = diff * diff;
            let best = prev_row_s.*[col - 1];
            if (prev_row_s.*[col] < best) {
                best = prev_row_s.*[col];
            }
            if (curr_row_s.*[col - 1] < best) {
                best = curr_row_s.*[col - 1];
            }
            curr_row_s.*[col] = cost + best;
            col += 1;
        }

        // Copy curr_row -> prev_row
        let cp: usize = 0;
        while (cp <= m) {
            prev_row_s.*[cp] = curr_row_s.*[cp];
            cp += 1;
        }
        i += 1;
    }

    return @sqrt(prev_row_s.*[m]);
}

/// Computes the 1D L2 DTW distance and backtracks the optimal warping path from `(0, 0)` to `(n-1, m-1)`.
/// - Requires `dp_mat_s.len >= (a.len + 1) * (b.len + 1)` and `path_out.len >= path_len` (at most `a.len + b.len - 1`).
/// - Returns `null` on empty sequences, undersized buffers, or non-finite inputs.
pub fn dtwAlign1D(
    a: []const f64,
    b: []const f64,
    window: usize,
    dp_mat_s: *[]f64,
    path_out: *[]WarpPair,
): DtwPathResult? {
    const n = a.len;
    const m = b.len;
    const cols = m + 1;
    if (n == 0 || m == 0 || dp_mat_s.*.len < (n + 1) * cols || path_out.*.len == 0) {
        return null;
    }
    if (!allFinite(a, n) || !allFinite(b, m)) {
        return null;
    }

    const eff_w = effectiveWindow(n, m, window);
    const total_cells = (n + 1) * cols;
    let idx: usize = 0;
    while (idx < total_cells) {
        dp_mat_s.*[idx] = DTW_INF;
        idx += 1;
    }
    dp_mat_s.*[0] = 0.0;

    let i: usize = 1;
    while (i <= n) {
        const j_start: usize = if (i > eff_w) i - eff_w else 1;
        const j_end_raw = i + eff_w;
        const j_end: usize = if (j_end_raw < m) j_end_raw else m;
        const ai = a[i - 1];

        let col: usize = j_start;
        while (col <= j_end) {
            const diff = ai - b[col - 1];
            const cost = diff * diff;
            let best = dp_mat_s.*[(i - 1) * cols + (col - 1)];
            const up = dp_mat_s.*[(i - 1) * cols + col];
            const left = dp_mat_s.*[i * cols + (col - 1)];
            if (up < best) {
                best = up;
            }
            if (left < best) {
                best = left;
            }
            dp_mat_s.*[i * cols + col] = cost + best;
            col += 1;
        }
        i += 1;
    }

    const total_dist = @sqrt(dp_mat_s.*[n * cols + m]);

    // Backtrack from (n, m) to (1, 1)
    let cur_i = n;
    let cur_j = m;
    let plen: usize = 0;

    while (true) {
        if (plen >= path_out.*.len) {
            return null;
        }
        path_out.*[plen] = WarpPair { i: cur_i - 1, j: cur_j - 1 };
        plen += 1;

        if (cur_i == 1 && cur_j == 1) {
            break;
        } else if (cur_i == 1) {
            cur_j -= 1;
        } else if (cur_j == 1) {
            cur_i -= 1;
        } else {
            const diag = dp_mat_s.*[(cur_i - 1) * cols + (cur_j - 1)];
            const up = dp_mat_s.*[(cur_i - 1) * cols + cur_j];
            const left = dp_mat_s.*[cur_i * cols + (cur_j - 1)];
            if (diag <= up && diag <= left) {
                cur_i -= 1;
                cur_j -= 1;
            } else if (up <= left) {
                cur_i -= 1;
            } else {
                cur_j -= 1;
            }
        }
    }

    // Reverse path in-place so it starts at (0, 0) and ends at (n-1, m-1)
    let lo: usize = 0;
    let hi: usize = plen - 1;
    while (lo < hi) {
        const tmp = path_out.*[lo];
        path_out.*[lo] = path_out.*[hi];
        path_out.*[hi] = tmp;
        lo += 1;
        hi -= 1;
    }

    return DtwPathResult {
        distance: total_dist,
        path_len: plen,
    };
}

/// Computes Multi-Dimensional DTW distance between `a` (`n x dims`) and `b` (`m x dims`)
/// using squared Euclidean frame distances and O(m) rolling buffers.
pub fn dtwDistanceMulti(
    a: []const f64,
    n: usize,
    b: []const f64,
    m: usize,
    dims: usize,
    window: usize,
    prev_row_s: *[]f64,
    curr_row_s: *[]f64,
): f64? {
    if (
        n == 0 || m == 0 || dims == 0 ||
        a.len < n * dims || b.len < m * dims ||
        prev_row_s.*.len < m + 1 || curr_row_s.*.len < m + 1
    ) {
        return null;
    }
    if (!allFinite(a, n * dims) || !allFinite(b, m * dims)) {
        return null;
    }

    const eff_w = effectiveWindow(n, m, window);
    let j: usize = 0;
    while (j <= m) {
        prev_row_s.*[j] = DTW_INF;
        j += 1;
    }
    prev_row_s.*[0] = 0.0;

    let i: usize = 1;
    while (i <= n) {
        let jc: usize = 0;
        while (jc <= m) {
            curr_row_s.*[jc] = DTW_INF;
            jc += 1;
        }

        const j_start: usize = if (i > eff_w) i - eff_w else 1;
        const j_end_raw = i + eff_w;
        const j_end: usize = if (j_end_raw < m) j_end_raw else m;
        const a_off = (i - 1) * dims;

        let col: usize = j_start;
        while (col <= j_end) {
            const b_off = (col - 1) * dims;
            let cost: f64 = 0.0;
            let d: usize = 0;
            while (d < dims) {
                const diff = a[a_off + d] - b[b_off + d];
                cost += diff * diff;
                d += 1;
            }
            let best = prev_row_s.*[col - 1];
            if (prev_row_s.*[col] < best) {
                best = prev_row_s.*[col];
            }
            if (curr_row_s.*[col - 1] < best) {
                best = curr_row_s.*[col - 1];
            }
            curr_row_s.*[col] = cost + best;
            col += 1;
        }

        let cp: usize = 0;
        while (cp <= m) {
            prev_row_s.*[cp] = curr_row_s.*[cp];
            cp += 1;
        }
        i += 1;
    }

    return @sqrt(prev_row_s.*[m]);
}

/// Computes the Keogh upper and lower envelopes over `seq` with Sakoe-Chiba radius `window`.
/// (`window == 0` uses the global min/max envelope over the entire sequence).
pub fn computeKeoghEnvelope(
    seq: []const f64,
    window: usize,
    upper_out: *[]f64,
    lower_out: *[]f64,
): bool {
    const n = seq.len;
    if (n == 0 || upper_out.*.len < n || lower_out.*.len < n) {
        return false;
    }
    if (!allFinite(seq, n)) {
        return false;
    }

    const eff_w = if (window == 0) n else window;
    let i: usize = 0;
    while (i < n) {
        const lo: usize = if (i > eff_w) i - eff_w else 0;
        const hi_raw = i + eff_w;
        const hi: usize = if (hi_raw < n - 1) hi_raw else n - 1;

        let mn = seq[lo];
        let mx = seq[lo];
        let k: usize = lo + 1;
        while (k <= hi) {
            if (seq[k] < mn) {
                mn = seq[k];
            }
            if (seq[k] > mx) {
                mx = seq[k];
            }
            k += 1;
        }
        lower_out.*[i] = mn;
        upper_out.*[i] = mx;
        i += 1;
    }

    return true;
}

/// Computes the LB_Keogh lower bound between `query` and candidate envelopes `(upper, lower)`.
/// Strictly satisfies `lbKeogh(query, env(candidate, w)) <= dtwDistance1D(query, candidate, w)`.
pub fn lbKeogh(
    query: []const f64,
    upper: []const f64,
    lower: []const f64,
): f64? {
    const n = query.len;
    if (n == 0 || upper.len < n || lower.len < n) {
        return null;
    }
    if (!allFinite(query, n) || !allFinite(upper, n) || !allFinite(lower, n)) {
        return null;
    }

    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        if (lower[i] > upper[i]) {
            return null;
        }
        const q = query[i];
        if (q > upper[i]) {
            const d = q - upper[i];
            sum_sq += d * d;
        } else if (q < lower[i]) {
            const d = lower[i] - q;
            sum_sq += d * d;
        }
        i += 1;
    }

    return @sqrt(sum_sq);
}

/// Z-normalizes a 1D time series into `out[0..seq.len]` (zero mean, unit standard deviation).
/// Constant sequences (std == 0) are mapped cleanly to all zeros.
pub fn zNormalize(seq: []const f64, out: *[]f64): bool {
    const n = seq.len;
    if (n == 0 || out.*.len < n || !allFinite(seq, n)) {
        return false;
    }

    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        sum += seq[i];
        i += 1;
    }
    const mean = sum / (n as f64);

    let var_sum: f64 = 0.0;
    i = 0;
    while (i < n) {
        const d = seq[i] - mean;
        var_sum += d * d;
        i += 1;
    }
    const std_dev = @sqrt(var_sum / (n as f64));

    if (std_dev == 0.0) {
        i = 0;
        while (i < n) {
            out.*[i] = 0.0;
            i += 1;
        }
        return true;
    }

    const inv_std = 1.0 / std_dev;
    i = 0;
    while (i < n) {
        out.*[i] = (seq[i] - mean) * inv_std;
        i += 1;
    }
    return true;
}
