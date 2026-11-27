[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rustfft

Zero-allocation in-place radix-2 Cooley-Tukey FFT, IFFT, magnitude spectrum, and Hann/Hamming windowing (Rust rustfft / microfft parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustfft` ([https://github.com/ejmahler/RustFFT](https://github.com/ejmahler/RustFFT))（原库名 `rustfft` → CP 包名 `rustfft`）

| Metadata | Value |
| --- | --- |
| Package | `rustfft` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rustfft
```

```cp
import {
    Complex,
    sqrtF64,
    sinF64,
    cosF64,
    isPowerOfTwo,
    magnitude,
    fftInPlace,
    ifftInPlace,
} from "rustfft";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Complex` | `class` | Fields: `re: f64, im: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `sinF64` | `(x_in: f64): f64` | Invokes `sinF64` and returns `f64`. |
| `cosF64` | `(x: f64): f64` | Invokes `cosF64` and returns `f64`. |
| `isPowerOfTwo` | `(n: usize): bool` | Checks predicate `isPowerOfTwo` and returns `bool`. |
| `magnitude` | `(c: Complex): f64` | Invokes `magnitude` and returns `f64`. |
| `fftInPlace` | `(buf: []Complex): bool` | Invokes `fftInPlace` and returns `bool`. |
| `ifftInPlace` | `(buf: []Complex): bool` | Invokes `ifftInPlace` and returns `bool`. |
| `applyHannWindow` | `(samples: []f64)` | Executes `applyHannWindow` in-place. |
| `applyHammingWindow` | `(samples: []f64)` | Executes `applyHammingWindow` in-place. |

## Minimal Example

```cp
import { isPowerOfTwo } from "rustfft";

pub fn main() {
    let result = isPowerOfTwo(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
