# @cap/openblas@0.3.29 (Agent Quickref)

> OpenBLAS (v0.3.29) high-performance BLAS matrix computation (SGEMM/DGEMM/SDOT/DDOT) for CP via prebuilt dynamic library | License: BSD-3-Clause
> Import: `import { ... } from "@cap/openblas";` | Full types: `src/index.d.cp`

## Functions
- `export fn cblas_sgemm(order: i32, trans_a: i32, trans_b: i32, m: i32, n: i32, k: i32, alpha: f32, a: [*]&f32!, lda: i32, b: [*]&f32!, ldb: i32, beta: f32, c: [*]f32!, ldc: i32);`
- `export fn cblas_dgemm(order: i32, trans_a: i32, trans_b: i32, m: i32, n: i32, k: i32, alpha: f64, a: [*]&f64!, lda: i32, b: [*]&f64!, ldb: i32, beta: f64, c: [*]f64!, ldc: i32);`
- `export fn cblas_sdot(n: i32, x: [*]&f32!, inc_x: i32, y: [*]&f32!, inc_y: i32): f32;`
- `export fn cblas_ddot(n: i32, x: [*]&f64!, inc_x: i32, y: [*]&f64!, inc_y: i32): f64;`
- `export fn openblas_get_config(): [*:0]&u8!;`
- `export fn openblasGetConfig(): string;`
- `export fn sgemm(m: usize, n: usize, k: usize, alpha: f32, a: &[]f32, b: &[]f32, beta: f32, c: []f32): !void;`
- `export fn dgemm(m: usize, n: usize, k: usize, alpha: f64, a: &[]f64, b: &[]f64, beta: f64, c: []f64): !void;`
- `export fn sdot(x: &[]f32, y: &[]f32): !f32;`
- `export fn ddot(x: &[]f64, y: &[]f64): !f64;`
