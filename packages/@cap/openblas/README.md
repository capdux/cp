[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/openblas

OpenBLAS (v0.3.29) high-performance BLAS matrix computation (SGEMM/DGEMM/SDOT/DDOT) for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`OpenBLAS`](https://github.com/OpenMathLib/OpenBLAS)（`BSD-3-Clause`）（包名 `@cap/openblas`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/openblas` |
| Version | `0.3.29` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/openblas
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `BlasError` | `InvalidDimensions, BufferTooSmall` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cblas_sgemm` | `(order: i32, trans_a: i32, trans_b: i32, m: i32, n: i32, k: i32, alpha: f32, a: [*]&f32!, lda: i32, b: [*]&f32!, ldb: i32, beta: f32, c: [*]f32!, ldc: i32)` | Executes `cblas_sgemm` in-place. |
| `cblas_dgemm` | `(order: i32, trans_a: i32, trans_b: i32, m: i32, n: i32, k: i32, alpha: f64, a: [*]&f64!, lda: i32, b: [*]&f64!, ldb: i32, beta: f64, c: [*]f64!, ldc: i32)` | Executes `cblas_dgemm` in-place. |
| `cblas_sdot` | `(n: i32, x: [*]&f32!, inc_x: i32, y: [*]&f32!, inc_y: i32): f32` | Invokes `cblas_sdot` and returns `f32`. |
| `cblas_ddot` | `(n: i32, x: [*]&f64!, inc_x: i32, y: [*]&f64!, inc_y: i32): f64` | Invokes `cblas_ddot` and returns `f64`. |
| `openblas_get_config` | `(): [*:0]&u8!` | Invokes `openblas_get_config` and returns `[*:0]&u8!`. |
| `openblasGetConfig` | `(): string` | Invokes `openblasGetConfig` and returns `string`. |
| `sgemm` | `(m: usize, n: usize, k: usize, alpha: f32, a: &[]f32, b: &[]f32, beta: f32, c: []f32): !void` | Invokes `sgemm` and returns `!void`. |
| `dgemm` | `(m: usize, n: usize, k: usize, alpha: f64, a: &[]f64, b: &[]f64, beta: f64, c: []f64): !void` | Invokes `dgemm` and returns `!void`. |
| `sdot` | `(x: &[]f32, y: &[]f32): !f32` | Invokes `sdot` and returns `!f32`. |
| `ddot` | `(x: &[]f64, y: &[]f64): !f64` | Invokes `ddot` and returns `!f64`. |

## Minimal Example

```cp
import { openblas_get_config } from "@cap/openblas";

pub fn main() {
    let result = openblas_get_config();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
