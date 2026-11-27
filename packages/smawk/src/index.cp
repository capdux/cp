// @cap/smawk — Zero-heap SMAWK O(m + n) row/column minima & total monotonicity algorithms
// Clean-room MIT port of Rust `smawk` crate (Aggarwal, Klawe, Moran, Shor, Wilber 1987).

const SMAWK_STACK_CAP: usize = 256;

fn smwkRowMinInRange(matrix: []const f64, cols: usize, row: usize, c_lo: usize, c_hi: usize): usize {
    const base: usize = row * cols;
    let best_c: usize = c_lo;
    let best_v: f64 = matrix[base + c_lo];
    let c: usize = c_lo + 1;
    while (c <= c_hi) : (c += 1) {
        const v: f64 = matrix[base + c];
        if (v < best_v) {
            best_v = v;
            best_c = c;
        }
    }
    return best_c;
}

/// Finds the leftmost minimum column index for every row (`0..rows`) via direct scan.
pub fn rowMinima(matrix: []const f64, rows: usize, cols: usize, out_min_cols: []usize): usize {
    if (rows == 0 || cols == 0 || matrix.len < rows * cols) {
        return 0;
    }
    const limit: usize = if (rows < out_min_cols.len) rows else out_min_cols.len;
    let r: usize = 0;
    while (r < limit) : (r += 1) {
        out_min_cols[r] = smwkRowMinInRange(matrix, cols, r, 0, cols - 1);
    }
    return limit;
}

/// Finds the topmost minimum row index for every column (`0..cols`) via direct scan.
pub fn colMinima(matrix: []const f64, rows: usize, cols: usize, out_min_rows: []usize): usize {
    if (rows == 0 || cols == 0 || matrix.len < rows * cols) {
        return 0;
    }
    const limit: usize = if (cols < out_min_rows.len) cols else out_min_rows.len;
    let c: usize = 0;
    while (c < limit) : (c += 1) {
        let best_r: usize = 0;
        let best_v: f64 = matrix[c];
        let r: usize = 1;
        while (r < rows) : (r += 1) {
            const v: f64 = matrix[r * cols + c];
            if (v < best_v) {
                best_v = v;
                best_r = r;
            }
        }
        out_min_rows[c] = best_r;
    }
    return limit;
}

/// Checks whether `matrix` satisfies the Monge property:
/// `M[i, j] + M[i+1, j+1] <= M[i, j+1] + M[i+1, j]` for all adjacent 2x2 submatrices.
/// Every Monge matrix is totally monotone. Runs in O(rows * cols).
pub fn isMonge(matrix: []const f64, rows: usize, cols: usize): bool {
    if (matrix.len < rows * cols) { return false; }
    if (rows < 2 || cols < 2) { return true; }
    let r: usize = 0;
    while (r + 1 < rows) : (r += 1) {
        let c: usize = 0;
        while (c + 1 < cols) : (c += 1) {
            const a = matrix[r * cols + c];
            const b = matrix[r * cols + c + 1];
            const d = matrix[(r + 1) * cols + c];
            const e = matrix[(r + 1) * cols + c + 1];
            if (a + e > b + d) {
                return false;
            }
        }
    }
    return true;
}

/// Checks whether `matrix` is totally monotone (all 2x2 submatrices are monotone).
pub fn isTotallyMonotone(matrix: []const f64, rows: usize, cols: usize): bool {
    if (matrix.len < rows * cols) {
        return false;
    }
    if (rows < 2 || cols < 2) {
        return true;
    }
    let r1: usize = 0;
    while (r1 < rows) : (r1 += 1) {
        let r2: usize = r1 + 1;
        while (r2 < rows) : (r2 += 1) {
            let c1: usize = 0;
            while (c1 < cols) : (c1 += 1) {
                let c2: usize = c1 + 1;
                while (c2 < cols) : (c2 += 1) {
                    const a: f64 = matrix[r1 * cols + c1];
                    const b: f64 = matrix[r1 * cols + c2];
                    const c: f64 = matrix[r2 * cols + c1];
                    const d: f64 = matrix[r2 * cols + c2];
                    if (a > b && c <= d) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

/// SMAWK `REDUCE` step: eliminates columns from `cols_idx` that cannot contain any row minimum
/// for the submatrix defined by `rows_idx` and `cols_idx`.
/// Writes at most `rows_idx.len` surviving column indices into `out_cols` and returns the count.
pub fn reduce(
    matrix: []const f64,
    cols: usize,
    rows_idx: []const usize,
    cols_idx: []const usize,
    out_cols: []usize,
): usize {
    if (rows_idx.len == 0 || cols_idx.len == 0 || out_cols.len == 0) {
        return 0;
    }
    let k: usize = 0;
    let j: usize = 0;
    while (j < cols_idx.len) : (j += 1) {
        const c = cols_idx[j];
        while (k > 0) {
            const row = rows_idx[k - 1];
            const prev_c = out_cols[k - 1];
            if (matrix[row * cols + prev_c] > matrix[row * cols + c]) {
                k -= 1;
            } else {
                break;
            }
        }
        if (k < rows_idx.len && k < out_cols.len) {
            out_cols[k] = c;
            k += 1;
        }
    }
    return k;
}

/// SMAWK `INTERPOLATE` step: given row minima already computed at odd indices (`1, 3, 5, ...`)
/// of `rows_idx` and stored in `minima[rows_idx[odd]]`, computes the even-index row minima
/// (`0, 2, 4, ...`) by scanning only the monotonic sub-range of `cols_idx`.
pub fn interpolate(
    matrix: []const f64,
    cols: usize,
    rows_idx: []const usize,
    cols_idx: []const usize,
    minima: []usize,
) {
    if (rows_idx.len == 0 || cols_idx.len == 0) { return; }
    let c_pos: usize = 0;
    let r_i: usize = 0;
    while (r_i < rows_idx.len) : (r_i += 2) {
        const row = rows_idx[r_i];
        // Upper bound column index for this even row is the minimum of the next odd row (if any)
        const end_col: usize = if (r_i + 1 < rows_idx.len) {
            minima[rows_idx[r_i + 1]]
        } else {
            cols_idx[cols_idx.len - 1]
        };

        let best_c = cols_idx[c_pos];
        let best_v = matrix[row * cols + best_c];
        while (c_pos < cols_idx.len) {
            const c = cols_idx[c_pos];
            const v = matrix[row * cols + c];
            if (v < best_v) {
                best_v = v;
                best_c = c;
            }
            if (c >= end_col) { break; }
            c_pos += 1;
        }
        minima[row] = best_c;
    }
}

fn smawkRec(
    matrix: []const f64,
    cols: usize,
    rows_idx: []const usize,
    cols_idx: []const usize,
    minima: []usize,
) {
    if (rows_idx.len == 0 || cols_idx.len == 0) { return; }

    // 1. Reduce columns to at most rows_idx.len
    let red_buf: [256]usize = undefined;
    let active_cols_len: usize = cols_idx.len;
    let use_red = false;
    if (cols_idx.len > rows_idx.len && rows_idx.len <= SMAWK_STACK_CAP) {
        active_cols_len = reduce(matrix, cols, rows_idx, cols_idx, red_buf[0..SMAWK_STACK_CAP]);
        use_red = true;
    }
    const active_cols: []const usize = if (use_red) red_buf[0..active_cols_len] else cols_idx;

    if (rows_idx.len == 1) {
        const row = rows_idx[0];
        let best_c = active_cols[0];
        let best_v = matrix[row * cols + best_c];
        let j: usize = 1;
        while (j < active_cols.len) : (j += 1) {
            const c = active_cols[j];
            const v = matrix[row * cols + c];
            if (v < best_v) {
                best_v = v;
                best_c = c;
            }
        }
        minima[row] = best_c;
        return;
    }

    // 2. Recursively solve odd-indexed rows (1, 3, 5, ...)
    let odd_rows: [256]usize = undefined;
    let odd_len: usize = 0;
    let i: usize = 1;
    while (i < rows_idx.len && odd_len < SMAWK_STACK_CAP) : (i += 2) {
        odd_rows[odd_len] = rows_idx[i];
        odd_len += 1;
    }
    smawkRec(matrix, cols, odd_rows[0..odd_len], active_cols, minima);

    // 3. Interpolate even-indexed rows (0, 2, 4, ...)
    interpolate(matrix, cols, rows_idx, active_cols, minima);
}

fn smwkDivideConquer(
    matrix: []const f64,
    cols: usize,
    r_lo: usize,
    r_hi: usize,
    c_lo: usize,
    c_hi: usize,
    out_min_cols: []usize,
) {
    if (r_lo >= r_hi) {
        return;
    }
    const mid: usize = r_lo + (r_hi - r_lo) / 2;
    const best_c: usize = smwkRowMinInRange(matrix, cols, mid, c_lo, c_hi);
    out_min_cols[mid] = best_c;
    if (mid > r_lo) {
        smwkDivideConquer(matrix, cols, r_lo, mid, c_lo, best_c, out_min_cols);
    }
    if (mid + 1 < r_hi) {
        smwkDivideConquer(matrix, cols, mid + 1, r_hi, best_c, c_hi, out_min_cols);
    }
}

/// Computes row minima of a totally monotone matrix in O(rows * log(cols)) via divide-and-conquer.
pub fn mongeRowMinima(matrix: []const f64, rows: usize, cols: usize, out_min_cols: []usize): usize {
    if (rows == 0 || cols == 0 || matrix.len < rows * cols) {
        return 0;
    }
    const limit: usize = if (rows < out_min_cols.len) rows else out_min_cols.len;
    if (limit == 0) {
        return 0;
    }
    smwkDivideConquer(matrix, cols, 0, limit, 0, cols - 1, out_min_cols);
    return limit;
}

/// Full O(rows + cols) SMAWK row minima for totally monotone matrices using `reduce` + `interpolate`.
/// Falls back to `mongeRowMinima` if dimensions exceed the zero-heap 256-element stack workspace.
pub fn smawkRowMinima(matrix: []const f64, rows: usize, cols: usize, out_min_cols: []usize): usize {
    if (rows == 0 || cols == 0 || matrix.len < rows * cols) { return 0; }
    const limit: usize = if (rows < out_min_cols.len) rows else out_min_cols.len;
    if (limit == 0) { return 0; }
    if (limit > SMAWK_STACK_CAP || cols > SMAWK_STACK_CAP) {
        return mongeRowMinima(matrix, limit, cols, out_min_cols);
    }
    let r_idx: [256]usize = undefined;
    let c_idx: [256]usize = undefined;
    let i: usize = 0;
    while (i < limit) : (i += 1) { r_idx[i] = i; }
    let j: usize = 0;
    while (j < cols) : (j += 1) { c_idx[j] = j; }
    smawkRec(matrix, cols, r_idx[0..limit], c_idx[0..cols], out_min_cols);
    return limit;
}

fn colDivideConquer(
    matrix: []const f64,
    cols: usize,
    c_lo: usize,
    c_hi: usize,
    r_lo: usize,
    r_hi: usize,
    out_min_rows: []usize,
) {
    if (c_lo >= c_hi) { return; }
    const mid: usize = c_lo + (c_hi - c_lo) / 2;
    let best_r: usize = r_lo;
    let best_v: f64 = matrix[r_lo * cols + mid];
    let r: usize = r_lo + 1;
    while (r <= r_hi) : (r += 1) {
        const v = matrix[r * cols + mid];
        if (v < best_v) {
            best_v = v;
            best_r = r;
        }
    }
    out_min_rows[mid] = best_r;
    if (mid > c_lo) {
        colDivideConquer(matrix, cols, c_lo, mid, r_lo, best_r, out_min_rows);
    }
    if (mid + 1 < c_hi) {
        colDivideConquer(matrix, cols, mid + 1, c_hi, best_r, r_hi, out_min_rows);
    }
}

/// Fast column minima for totally monotone matrices (monotonicity across columns).
pub fn smawkColMinima(matrix: []const f64, rows: usize, cols: usize, out_min_rows: []usize): usize {
    if (rows == 0 || cols == 0 || matrix.len < rows * cols) { return 0; }
    const limit: usize = if (cols < out_min_rows.len) cols else out_min_rows.len;
    if (limit == 0) { return 0; }
    colDivideConquer(matrix, cols, 0, limit, 0, rows - 1, out_min_rows);
    return limit;
}

fn onlineUpdateBlock(
    matrix: []const f64,
    n: usize,
    c_lo: usize,
    c_hi: usize,
    r_lo: usize,
    r_hi: usize,
    out_min_rows: []usize,
) {
    if (c_lo >= c_hi || r_lo > r_hi) { return; }
    const mid: usize = c_lo + (c_hi - c_lo) / 2;
    let best_r: usize = r_lo;
    let best_v: f64 = matrix[r_lo * n + mid];
    let r: usize = r_lo + 1;
    while (r <= r_hi) : (r += 1) {
        const v = matrix[r * n + mid];
        if (v < best_v) {
            best_v = v;
            best_r = r;
        }
    }
    const cur_r = out_min_rows[mid];
    const cur_v = matrix[cur_r * n + mid];
    if (best_v < cur_v || (best_v == cur_v && best_r < cur_r)) {
        out_min_rows[mid] = best_r;
    }
    if (mid > c_lo) {
        onlineUpdateBlock(matrix, n, c_lo, mid, r_lo, best_r, out_min_rows);
    }
    if (mid + 1 < c_hi) {
        onlineUpdateBlock(matrix, n, mid + 1, c_hi, best_r, r_hi, out_min_rows);
    }
}

fn onlineSolveRec(matrix: []const f64, n: usize, lo: usize, hi: usize, out_min_rows: []usize) {
    if (lo + 1 >= hi) { return; }
    const mid: usize = lo + (hi - lo) / 2;
    onlineSolveRec(matrix, n, lo, mid, out_min_rows);
    // Rows [lo..mid-1] are now finalized and strictly < all columns [mid..hi-1]
    onlineUpdateBlock(matrix, n, mid, hi, lo, mid - 1, out_min_rows);
    onlineSolveRec(matrix, n, mid, hi, out_min_rows);
}

/// Online column minima for an `n x n` totally monotone upper-triangular DP matrix
/// (matching Rust `smawk::online_column_minima`):
/// Sets `out_min_rows[0] = 0`, and for each `j` in `1..n` computes `argmin_{0 <= i < j} M[i, j]`
/// without ever reading entries `M[i, j]` where `i >= j`.
pub fn onlineColumnMinima(matrix: []const f64, n: usize, out_min_rows: []usize): usize {
    if (n == 0 || matrix.len < n * n || out_min_rows.len < n) { return 0; }
    let j: usize = 0;
    while (j < n) : (j += 1) {
        out_min_rows[j] = 0;
    }
    if (n > 1) {
        onlineSolveRec(matrix, n, 0, n, out_min_rows);
    }
    return n;
}
