// Type definitions for CP

export fn rowMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize;
export fn colMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_rows: []usize): usize;
export fn isMonge(matrix: &[]f64, rows: usize, cols: usize): bool;
export fn isTotallyMonotone(matrix: &[]f64, rows: usize, cols: usize): bool;
export fn reduce(matrix: &[]f64, cols: usize, rows_idx: &[]usize, cols_idx: &[]usize, out_cols: []usize): usize;
export fn interpolate(matrix: &[]f64, cols: usize, rows_idx: &[]usize, cols_idx: &[]usize, minima: []usize);
export fn mongeRowMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize;
export fn smawkRowMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize;
export fn smawkColMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_rows: []usize): usize;
export fn onlineColumnMinima(matrix: &[]f64, n: usize, out_min_rows: []usize): usize;
