// Type definitions for CP

export fn smwkRowMinInRange(matrix: &[]f64, cols: usize, row: usize, c_lo: usize, c_hi: usize): usize;
export fn rowMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize;
export fn isTotallyMonotone(matrix: &[]f64, rows: usize, cols: usize): bool;
export fn smwkDivideConquer(matrix: &[]f64, cols: usize, r_lo: usize, r_hi: usize, c_lo: usize, c_hi: usize, out_min_cols: []usize);
export fn mongeRowMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize;
