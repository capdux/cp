[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fft.js

Zero-allocation in-place iterative Cooley-Tukey Radix-2 FFT, IFFT, and magnitude spectrum for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fft.js` ([https://github.com/indutny/fft.js](https://github.com/indutny/fft.js)) / `crates.io:microfft` ([https://gitlab.com/ra_kete/microfft-rs](https://gitlab.com/ra_kete/microfft-rs))（原库名 `fft.js` → CP 包名 `fft.js`）

| Metadata | Value |
| --- | --- |
| Package | `fft.js` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fft.js
```

```cp
import {
    absF64,
    sqrtF64,
    reduceAngle,
    sinF64,
    cosF64,
    isPowerOfTwo,
    fftInPlace,
    ifftInPlace,
} from "fft.js";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `reduceAngle` | `(rad: f64): f64` | Invokes `reduceAngle` and returns `f64`. |
| `sinF64` | `(rad: f64): f64` | Invokes `sinF64` and returns `f64`. |
| `cosF64` | `(rad: f64): f64` | Invokes `cosF64` and returns `f64`. |
| `isPowerOfTwo` | `(n: usize): bool` | Checks predicate `isPowerOfTwo` and returns `bool`. |
| `fftInPlace` | `(re: []f64, im: []f64): bool` | Invokes `fftInPlace` and returns `bool`. |
| `ifftInPlace` | `(re: []f64, im: []f64): bool` | Invokes `ifftInPlace` and returns `bool`. |
| `magnitudeSpectrum` | `(re: &[]f64, im: &[]f64, out_mag: []f64): usize` | Invokes `magnitudeSpectrum` and returns `usize`. |
| `dominantBin` | `(re: &[]f64, im: &[]f64): usize` | Invokes `dominantBin` and returns `usize`. |

## Minimal Example

```cp
import { isPowerOfTwo } from "fft.js";

pub fn main() {
    let result = isPowerOfTwo(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
