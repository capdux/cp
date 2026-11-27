# rustfft — AI Agent Quick Ref

Zero-allocation in-place radix-2 Cooley-Tukey FFT, IFFT, magnitude spectrum, and Hann/Hamming windowing (Rust rustfft / microfft parity) for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class Complex { re: f64, im: f64 }
```

## API Signatures

```cp
fn sqrtF64(x: f64): f64;
fn sinF64(x_in: f64): f64;
fn cosF64(x: f64): f64;
fn isPowerOfTwo(n: usize): bool;
fn magnitude(c: Complex): f64;
fn fftInPlace(buf: []Complex): bool;
fn ifftInPlace(buf: []Complex): bool;
fn applyHannWindow(samples: []f64);
fn applyHammingWindow(samples: []f64);
```

## Minimal Usage

```cp
import { isPowerOfTwo } from "rustfft";

pub fn main() {
    let result = isPowerOfTwo(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rustfft"`; install via `cap add rustfft`.
