// @cap/smawk — Zero-heap matrix row minima & total monotonicity checker

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
