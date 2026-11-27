# fft.js — AI Agent Quick Ref

Zero-allocation in-place iterative Cooley-Tukey Radix-2 FFT, IFFT, and magnitude spectrum for CP (`v1.0.0`, `MIT`)

## Import

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

## API Signatures

```cp
fn absF64(x: f64): f64;
fn sqrtF64(x: f64): f64;
fn reduceAngle(rad: f64): f64;
fn sinF64(rad: f64): f64;
fn cosF64(rad: f64): f64;
fn isPowerOfTwo(n: usize): bool;
fn fftInPlace(re: []f64, im: []f64): bool;
fn ifftInPlace(re: []f64, im: []f64): bool;
fn magnitudeSpectrum(re: &[]f64, im: &[]f64, out_mag: []f64): usize;
fn dominantBin(re: &[]f64, im: &[]f64): usize;
```

## Minimal Usage

```cp
import { isPowerOfTwo } from "fft.js";

pub fn main() {
    let result = isPowerOfTwo(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fft.js"`; install via `cap add fft.js`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
