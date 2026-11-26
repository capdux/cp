// Type definitions for CP

export fn absF64(x: f64): f64;
export fn sqrtF64(x: f64): f64;
export fn reduceAngle(rad: f64): f64;
export fn sinF64(rad: f64): f64;
export fn cosF64(rad: f64): f64;
export fn expF64(x: f64): f64;
export fn atanSmall(x: f64): f64;
export fn atanF64(x: f64): f64;
export fn atan2F64(y: f64, x: f64): f64;
export class Complex {
    re: f64,
    im: f64,
    fn new(re: f64, im: f64): Complex;
    fn zero(): Complex;
    fn one(): Complex;
    fn iUnit(): Complex;
    fn fromPolar(r: f64, theta: f64): Complex;
    fn add(other: Complex): Complex;
    fn sub(other: Complex): Complex;
    fn mul(other: Complex): Complex;
    fn div(other: Complex): Complex;
    fn conj(): Complex;
    fn neg(): Complex;
    fn scale(s: f64): Complex;
    fn normSq(): f64;
    fn norm(): f64;
    fn arg(): f64;
    fn exp(): Complex;
    fn sqrt(): Complex;
    fn approxEq(other: Complex, eps: f64): bool;
}

export fn new(re: f64, im: f64): Complex;
export fn zero(): Complex;
export fn one(): Complex;
export fn iUnit(): Complex;
export fn fromPolar(r: f64, theta: f64): Complex;
export fn add(a: Complex, b: Complex): Complex;
export fn sub(a: Complex, b: Complex): Complex;
export fn mul(a: Complex, b: Complex): Complex;
export fn div(a: Complex, b: Complex): Complex;
export fn conj(z: Complex): Complex;
export fn neg(z: Complex): Complex;
export fn scale(z: Complex, s: f64): Complex;
export fn normSq(z: Complex): f64;
export fn norm(z: Complex): f64;
export fn arg(z: Complex): f64;
export fn exp(z: Complex): Complex;
export fn powInt(z: Complex, n: i32): Complex;
export fn sqrt(z: Complex): Complex;
export fn approxEq(a: Complex, b: Complex, eps: f64): bool;
