[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# fft.js

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fft.js` ([https://github.com/indutny/fft.js](https://github.com/indutny/fft.js)) / `crates.io:microfft` ([https://gitlab.com/ra_kete/microfft-rs](https://gitlab.com/ra_kete/microfft-rs))（原库名 `fft.js` → CP 包名 `fft.js`）

Zero-heap-allocation in-place iterative Cooley-Tukey Radix-2 Fast Fourier Transform (FFT / IFFT) and magnitude spectrum library in pure CP with zero external dependencies.

## Features

- `isPowerOfTwo(n: usize): bool`
- `fftInPlace(re: []f64, im: []f64): bool`: bit-reversal permutation + iterative radix-2 butterflies
- `ifftInPlace(re: []f64, im: []f64): bool`: conjugate -> FFT -> conjugate & scale by `1/N`
- `magnitudeSpectrum(re: []const f64, im: []const f64, out_mag: []f64): usize`
- `dominantBin(re: []const f64, im: []const f64): usize`
- Built-in Taylor-series `sinF64` / `cosF64` and Newton-Raphson `sqrtF64`.

## License

MIT
