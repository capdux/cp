// Type definitions for CP

export fn absF64(x: f64): f64;
export fn sqrtF64(x: f64): f64;
export fn reduceAngle(rad: f64): f64;
export fn sinF64(rad: f64): f64;
export fn cosF64(rad: f64): f64;
export fn isPowerOfTwo(n: usize): bool;
export fn fftInPlace(re: []f64, im: []f64): bool;
export fn ifftInPlace(re: []f64, im: []f64): bool;
export fn magnitudeSpectrum(re: &[]f64, im: &[]f64, out_mag: []f64): usize;
export fn dominantBin(re: &[]f64, im: &[]f64): usize;
