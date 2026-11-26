// Type definitions for CP

export fn isNanF64(x: f64): bool;
export fn isInfF64(x: f64): bool;
export fn isFiniteF64(x: f64): bool;
export fn orderedCmpF64(a: f64, b: f64): i32;
export fn orderedEqF64(a: f64, b: f64): bool;
export fn orderedMinF64(a: f64, b: f64): f64;
export fn orderedMaxF64(a: f64, b: f64): f64;
export fn clampF64(x: f64, lo: f64, hi: f64): f64;
export class NotNanF64 {
    inner: f64,
    fn new(v: f64): NotNanF64?;
    fn value(): f64;
    fn cmp(other: NotNanF64): i32;
}

export fn siftDownF64(slice: []f64, start: usize, end: usize);
export fn sortF64(slice: []f64);
export fn isSortedF64(slice: &[]f64): bool;
export fn binarySearchF64(slice: &[]f64, target: f64): usize?;
