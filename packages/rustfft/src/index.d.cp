// Type definitions for CP

export class Complex {
    re: f64,
    im: f64,
}

export fn sqrtF64(x: f64): f64;
export fn sinF64(x_in: f64): f64;
export fn cosF64(x: f64): f64;
export fn isPowerOfTwo(n: usize): bool;
export fn magnitude(c: Complex): f64;
export fn fftInPlace(buf: []Complex): bool;
export fn ifftInPlace(buf: []Complex): bool;
export fn applyHannWindow(samples: []f64);
export fn applyHammingWindow(samples: []f64);
