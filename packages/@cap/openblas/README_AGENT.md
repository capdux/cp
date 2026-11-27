# @cap/openblas — AI Agent Quick Ref

OpenBLAS (v0.3.29) high-performance BLAS matrix computation (SGEMM/DGEMM/SDOT/DDOT) for CP via prebuilt dynamic library (`v0.3.29`, `BSD-3-Clause`)

## Import

```cp
import {
    BlasError,
    cblas_sgemm,
    cblas_dgemm,
    cblas_sdot,
    cblas_ddot,
    openblas_get_config,
    openblasGetConfig,
    sgemm,
} from "@cap/openblas";
```

## Core Types & Constants

```cp
error BlasError { InvalidDimensions, BufferTooSmall }
```

## API Signatures

```cp
fn cblas_sgemm(order: i32, trans_a: i32, trans_b: i32, m: i32, n: i32, k: i32, alpha: f32, a: [*]&f32!, lda: i32, b: [*]&f32!, ldb: i32, beta: f32, c: [*]f32!, ldc: i32);
fn cblas_dgemm(order: i32, trans_a: i32, trans_b: i32, m: i32, n: i32, k: i32, alpha: f64, a: [*]&f64!, lda: i32, b: [*]&f64!, ldb: i32, beta: f64, c: [*]f64!, ldc: i32);
fn cblas_sdot(n: i32, x: [*]&f32!, inc_x: i32, y: [*]&f32!, inc_y: i32): f32;
fn cblas_ddot(n: i32, x: [*]&f64!, inc_x: i32, y: [*]&f64!, inc_y: i32): f64;
fn openblas_get_config(): [*:0]&u8!;
fn openblasGetConfig(): string;
fn sgemm(m: usize, n: usize, k: usize, alpha: f32, a: &[]f32, b: &[]f32, beta: f32, c: []f32): !void;
fn dgemm(m: usize, n: usize, k: usize, alpha: f64, a: &[]f64, b: &[]f64, beta: f64, c: []f64): !void;
fn sdot(x: &[]f32, y: &[]f32): !f32;
fn ddot(x: &[]f64, y: &[]f64): !f64;
```

## Minimal Usage

```cp
import { openblas_get_config } from "@cap/openblas";

pub fn main() {
    let result = openblas_get_config();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/openblas"`; install via `cap add @cap/openblas`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
