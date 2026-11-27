[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# matrixmultiply

Zero-heap general dense matrix multiplication (sgemm / dgemm / matmulF64) with arbitrary row and column strides

| Metadata | Value |
| --- | --- |
| Package | `matrixmultiply` |
| Version | `0.3.10` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add matrixmultiply
```

```cp
import { mmulMin, sgemm, dgemm, matmulF64 } from "matrixmultiply";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mmulMin` | `(a: usize, b: usize): usize` | Invokes `mmulMin` and returns `usize`. |
| `sgemm` | `(m: usize, k: usize, n: usize, alpha: f32, a: &[]f32, rsa: usize, csa: usize, b: &[]f32, rsb: usize, csb: usize, beta: f32, c: []f32, rsc: usize, csc: usize)` | Executes `sgemm` in-place. |
| `dgemm` | `(m: usize, k: usize, n: usize, alpha: f64, a: &[]f64, rsa: usize, csa: usize, b: &[]f64, rsb: usize, csb: usize, beta: f64, c: []f64, rsc: usize, csc: usize)` | Executes `dgemm` in-place. |
| `matmulF64` | `(m: usize, k: usize, n: usize, a: &[]f64, b: &[]f64, c: []f64)` | Executes `matmulF64` in-place. |

## Minimal Example

```cp
import { mmulMin } from "matrixmultiply";

pub fn main() {
    let result = mmulMin(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
