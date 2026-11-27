# smawk (AI Agent Quick Ref)

Zero-heap $O(m+n)$ SMAWK row/column minima for totally monotone/Monge matrices + checkers & online DP.

## Import
```typescript
import {
    rowMinima, colMinima,
    isMonge, isTotallyMonotone,
    reduce, interpolate,
    mongeRowMinima, smawkRowMinima, smawkColMinima,
    onlineColumnMinima,
} from "smawk";
```

## API Signatures
- `smawkRowMinima` / `mongeRowMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize`
- `smawkColMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_rows: []usize): usize`
- `rowMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize` ($O(mn)$, general)
- `colMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_rows: []usize): usize` ($O(mn)$, general)
- `onlineColumnMinima(matrix: &[]f64, n: usize, out_min_rows: []usize): usize` (strictly upper-triangular `i < j`)
- `isMonge(matrix: &[]f64, rows: usize, cols: usize): bool`
- `isTotallyMonotone(matrix: &[]f64, rows: usize, cols: usize): bool`
- `reduce(matrix: &[]f64, cols: usize, rows_idx: &[]usize, cols_idx: &[]usize, out_cols: []usize): usize`
- `interpolate(matrix: &[]f64, cols: usize, rows_idx: &[]usize, cols_idx: &[]usize, minima: []usize)`

## Minimal Example
```typescript
import { isMonge, smawkRowMinima } from "smawk";

fn solve(mat: &[]f64, rows: usize, cols: usize, out: []usize): usize {
    if (isMonge(mat, rows, cols)) {
        return smawkRowMinima(mat, rows, cols, out);
    }
    return 0;
}
```

## Pitfalls & Borrowing Rules
- Matrix layout is flat row-major `&[]f64` where element `(r, c)` is at `matrix[r * cols + c]`; `matrix.len` must be `>= rows * cols` (otherwise returns `0`).
- `smawkRowMinima` / `mongeRowMinima` / `smawkColMinima` require a totally monotone (or Monge) matrix; for non-monotone matrices use `rowMinima` / `colMinima`.
- Output slices must be pre-sized mutable `[]usize`: `out_min_cols.len >= rows` for row minima, `out_min_rows.len >= cols` (or `n`) for column minima.
- `onlineColumnMinima` only reads strictly upper-triangular entries `M[i, j]` with `i < j` (`out_min_rows[0]` is `0`).
- Ties are broken in favor of the leftmost column (or topmost row for column minima).
