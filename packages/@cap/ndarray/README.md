# `@cap/ndarray`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ndarray` ([https://github.com/rust-ndarray/ndarray](https://github.com/rust-ndarray/ndarray))

Zero-heap fixed-capacity 2D/N-D numerical array with stride indexing, matrix multiplication, transpose, reshape, dot product, and reductions — clean-room CP equivalent of crates.io `ndarray`.

## Features
- `Array2D`: `zeros`, `filled`, `eye`, `fromSlice`, `get`, `set`, `transpose`, `scale`, `reshape`
- Reductions: `sum`, `mean`, `minVal`, `maxVal`
- Linear ops: `addArrays`, `matmul`, `dotProduct`
