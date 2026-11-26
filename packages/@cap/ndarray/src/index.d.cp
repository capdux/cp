// Type definitions for CP

export class Array2D {
    rows: usize,
    cols: usize,
    data: [64]f64,
    fn zeros(r: usize, c: usize): Array2D;
    fn filled(r: usize, c: usize, val: f64): Array2D;
    fn eye(n: usize): Array2D;
    fn fromSlice(r: usize, c: usize, vals: []f64): Array2D?;
    fn elemCount(): usize;
    fn get(r: usize, c: usize): f64?;
    fn set(_: auto, r: usize, c: usize, val: f64): bool;
    fn sum(): f64;
    fn mean(): f64?;
    fn minVal(): f64?;
    fn maxVal(): f64?;
    fn transpose(): Array2D;
    fn scale(factor: f64): Array2D;
    fn reshape(new_r: usize, new_c: usize): Array2D?;
}

export fn addArrays(a: Array2D, b: Array2D): Array2D?;
export fn matmul(a: Array2D, b: Array2D): Array2D?;
export fn dotProduct(a: []f64, b: []f64): f64?;
