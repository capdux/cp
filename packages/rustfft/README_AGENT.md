# rustfft@1.0.0 (Agent Quickref)

> Zero-allocation in-place radix-2 Cooley-Tukey FFT, IFFT, magnitude spectrum, and Hann/Hamming windowing (Rust rustfft / microfft parity) for CP | License: MIT
> Import: `import { ... } from "rustfft";` | Full types: `src/index.d.cp`

## Functions
- `export fn isPowerOfTwo(n: usize): bool;`
- `export fn magnitude(c: Complex): f64;`
- `export fn fftInPlace(buf: []Complex): bool;`
- `export fn ifftInPlace(buf: []Complex): bool;`
- `export fn applyHannWindow(samples: []f64);`
- `export fn applyHammingWindow(samples: []f64);`
