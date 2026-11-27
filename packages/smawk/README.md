[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# smawk

Zero-heap SMAWK $O(m + n)$ row-minima and column-minima algorithm for totally monotone and Monge matrices, plus Monge/total-monotonicity checkers, `reduce`/`interpolate` primitives, and online column-minima DP solver in pure CP.

- **Package**: `smawk` (`v0.3.2`)
- **Upstream Alignment**: Clean-room CP port of `crates.io/smawk` (`v0.3.2`, `MIT`, `mgeisler/smawk`), ideal for Knuth-Plass line wrapping, optimal text layout, and 1D dynamic programming speedups.

## Installation & Import

```bash
cap add smawk
```

```typescript
import {
    rowMinima, colMinima,
    isMonge, isTotallyMonotone,
    reduce, interpolate,
    mongeRowMinima, smawkRowMinima, smawkColMinima,
    onlineColumnMinima,
} from "smawk";
```

## Core API

### High-Level Matrix Minima

| Function | Signature | Description |
| --- | --- | --- |
| `smawkRowMinima` / `mongeRowMinima` | `(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize` | $O(m + n)$ SMAWK row minima for totally monotone / Monge matrices; writes column index of leftmost minimum per row into `out_min_cols`, returns `rows` written. |
| `smawkColMinima` | `(matrix: &[]f64, rows: usize, cols: usize, out_min_rows: []usize): usize` | $O(m + n)$ SMAWK column minima (transposed view); writes row index per column into `out_min_rows`. |
| `rowMinima` | `(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize` | General $O(mn)$ exact row minima for arbitrary matrices (no monotonicity required). |
| `colMinima` | `(matrix: &[]f64, rows: usize, cols: usize, out_min_rows: []usize): usize` | General $O(mn)$ exact column minima for arbitrary matrices. |
| `onlineColumnMinima` | `(matrix: &[]f64, n: usize, out_min_rows: []usize): usize` | Strictly upper-triangular online SMAWK (`i < j` only; never reads `M[i, j]` where `i >= j`), for 1D DP recurrences. |

### Property Verification & Low-Level SMAWK Primitives

| Function | Signature | Description |
| --- | --- | --- |
| `isMonge` | `(matrix: &[]f64, rows: usize, cols: usize): bool` | Checks the Monge property: $M[i,j] + M[i+1,j+1] \le M[i,j+1] + M[i+1,j]$. |
| `isTotallyMonotone` | `(matrix: &[]f64, rows: usize, cols: usize): bool` | Checks total monotonicity across all $2 \times 2$ submatrices. |
| `reduce` | `(matrix: &[]f64, cols: usize, rows_idx: &[]usize, cols_idx: &[]usize, out_cols: []usize): usize` | SMAWK REDUCE step: prunes candidate columns down to at most `rows_idx.len`, writing surviving column indices into `out_cols`. |
| `interpolate` | `(matrix: &[]f64, cols: usize, rows_idx: &[]usize, cols_idx: &[]usize, minima: []usize)` | SMAWK INTERPOLATE step: fills odd-row minima between already-solved even rows in `minima`. |

## Examples

### 1. SMAWK Row Minima on a Monge Matrix

```typescript
import { isMonge, smawkRowMinima } from "smawk";

fn main() {
    // 3x4 Monge matrix stored in row-major order
    let mat: [12]f64 = [
        10.0, 17.0, 13.0, 28.0,
        17.0, 22.0, 16.0, 29.0,
        24.0, 28.0, 22.0, 34.0,
    ];
    let ok = isMonge(mat[0..], 3, 4); // true
    let mins: [3]usize = undefined;
    let count = smawkRowMinima(mat[0..], 3, 4, mins[0..]);
    // count == 3, mins == [0, 2, 2]
}
```

### 2. Column Minima & Total Monotonicity Check

```typescript
import { isTotallyMonotone, smawkColMinima } from "smawk";

fn main() {
    let mat: [9]f64 = [
        1.0, 2.0, 4.0,
        2.0, 1.0, 2.0,
        4.0, 2.0, 1.0,
    ];
    if (isTotallyMonotone(mat[0..], 3, 3)) {
        let col_mins: [3]usize = undefined;
        let n = smawkColMinima(mat[0..], 3, 3, col_mins[0..]); // [0, 1, 2]
    }
}
```

### 3. Online Upper-Triangular Column Minima (1D DP)

```typescript
import { onlineColumnMinima } from "smawk";

fn main() {
    // 4x4 strictly upper-triangular cost matrix (entries i >= j are never read)
    let cost: [16]f64 = [
        0.0, 5.0, 9.0, 14.0,
        0.0, 0.0, 3.0,  7.0,
        0.0, 0.0, 0.0,  2.0,
        0.0, 0.0, 0.0,  0.0,
    ];
    let best_row: [4]usize = undefined;
    let n = onlineColumnMinima(cost[0..], 4, best_row[0..]);
}
```

## Differences from Upstream & Highlights

- **Flat Row-Major Zero-Heap Representation**: Accepts contiguous `&[]f64` slices (`matrix[r * cols + c]`) and writes directly into caller-provided `[]usize` buffers, eliminating Rust `Vec<Vec<T>>` / `Matrix` trait heap overhead.
- **Exposed `reduce` & `interpolate`**: Provides both high-level `smawkRowMinima` / `smawkColMinima` / `onlineColumnMinima` and low-level building blocks for custom DP pipelines.
- **Defensive Validation**: Returns `0` gracefully when `rows == 0`, `cols == 0`, `matrix.len < rows * cols`, or output buffer is too small.
