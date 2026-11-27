# @cap/smawk

Zero-heap matrix row-minima algorithms (SMAWK / divide-and-conquer monotone row minima and total monotonicity validation) in pure CP.

## API

- `rowMinima(matrix, rows, cols, out_min_cols): usize` — finds leftmost minimum column index per row
- `isTotallyMonotone(matrix, rows, cols): bool` — validates whether every 2×2 submatrix satisfies total monotonicity
- `mongeRowMinima(matrix, rows, cols, out_min_cols): usize` — divide-and-conquer row minima leveraging monotonicity, matching `rowMinima` on all monotone/Monge matrices

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`smawk@0.3.2`)
- **Upstream Repository**: https://github.com/mgeisler/smawk
- **License**: MIT (clean-room CP implementation)
