// Type definitions for CP

export fn fabs(x: f64): f64;
export class Complex64 {
    re: f64,
    im: f64,
    fn new(re: f64, im: f64): Complex64;
    fn zero(): Complex64;
    fn one(): Complex64;
    fn i(): Complex64;
    fn fromPolar(r: f64, theta: f64): Complex64;
    fn add(other: Complex64): Complex64;
    fn sub(other: Complex64): Complex64;
    fn mul(other: Complex64): Complex64;
    fn div(other: Complex64): Complex64;
    fn neg(): Complex64;
    fn conj(): Complex64;
    fn scale(s: f64): Complex64;
    fn inv(): Complex64;
    fn powi(exponent: i32): Complex64;
    fn normSqr(): f64;
    fn norm(): f64;
    fn arg(): f64;
    fn exp(): Complex64;
    fn sqrt(): Complex64;
    fn approxEq(other: Complex64, eps: f64): bool;
}

export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushU64(out: []u8, pos: usize, val: u64): usize;
export fn pushAbsFloat(out: []u8, pos: usize, v: f64): usize;
export fn formatComplex(z: Complex64, out: []u8): string;
