[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/matrixmultiply

Zero-heap general dense matrix multiplication (`sgemm` / `dgemm` / `matmulF64`) in pure CP, supporting arbitrary row/column strides with cache-blocked accumulation.

## API

- `sgemm(m, k, n, alpha, a, rsa, csa, b, rsb, csb, beta, c, rsc, csc)` — computes `C = alpha * A * B + beta * C` (`f32`)
- `dgemm(m, k, n, alpha, a, rsa, csa, b, rsb, csb, beta, c, rsc, csc)` — computes `C = alpha * A * B + beta * C` (`f64`)
- `matmulF64(m, k, n, a, b, c)` — contiguous row-major `C = A * B` (`f64`)

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`matrixmultiply@0.3.10`)
- **Upstream Repository**: https://github.com/bluss/matrixmultiply
- **License**: MIT (clean-room CP implementation)
