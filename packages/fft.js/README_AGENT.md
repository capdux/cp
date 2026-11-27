# fft.js@1.0.0 (Agent Quickref)

> Zero-allocation in-place iterative Cooley-Tukey Radix-2 FFT, IFFT, and magnitude spectrum for CP | License: MIT
> Import: `import { ... } from "fft.js";` | Full types: `src/index.d.cp`

## Functions
- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn reduceAngle(rad: f64): f64;`
- `export fn sinF64(rad: f64): f64;`
- `export fn cosF64(rad: f64): f64;`
- `export fn isPowerOfTwo(n: usize): bool;`
- `export fn fftInPlace(re: []f64, im: []f64): bool;`
- `export fn ifftInPlace(re: []f64, im: []f64): bool;`
- `export fn magnitudeSpectrum(re: &[]f64, im: &[]f64, out_mag: []f64): usize;`
- `export fn dominantBin(re: &[]f64, im: &[]f64): usize;`
