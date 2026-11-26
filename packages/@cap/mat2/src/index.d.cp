// Type definitions for CP

export fn absF64(x: f64): f64;
export fn reduceAngle(rad: f64): f64;
export fn sinF64(rad: f64): f64;
export fn cosF64(rad: f64): f64;
export class V2 {
    x: f64,
    y: f64,
    fn new(x: f64, y: f64): V2;
    fn approxEq(other: V2, eps: f64): bool;
}

export class Mat2 {
    m00: f64,
    m01: f64,
    m10: f64,
    m11: f64,
    fn new(m00: f64, m01: f64, m10: f64, m11: f64): Mat2;
    fn identity(): Mat2;
    fn zero(): Mat2;
    fn fromScale(sx: f64, sy: f64): Mat2;
    fn fromRotation(radians: f64): Mat2;
    fn fromShear(shx: f64, shy: f64): Mat2;
    fn add(other: Mat2): Mat2;
    fn sub(other: Mat2): Mat2;
    fn scale(s: f64): Mat2;
    fn mul(other: Mat2): Mat2;
    fn mulVec(v: V2): V2;
    fn transpose(): Mat2;
    fn trace(): f64;
    fn determinant(): f64;
    fn isInvertible(): bool;
    fn inverse(): Mat2;
    fn frobeniusNormSq(): f64;
    fn approxEq(other: Mat2, eps: f64): bool;
}

export fn identity(): Mat2;
export fn zero(): Mat2;
export fn fromScale(sx: f64, sy: f64): Mat2;
export fn fromRotation(radians: f64): Mat2;
export fn fromShear(shx: f64, shy: f64): Mat2;
export fn add(a: Mat2, b: Mat2): Mat2;
export fn sub(a: Mat2, b: Mat2): Mat2;
export fn scale(m: Mat2, s: f64): Mat2;
export fn mul(a: Mat2, b: Mat2): Mat2;
export fn mulVec(m: Mat2, v: V2): V2;
export fn transpose(m: Mat2): Mat2;
export fn trace(m: Mat2): f64;
export fn determinant(m: Mat2): f64;
export fn isInvertible(m: Mat2): bool;
export fn inverse(m: Mat2): Mat2;
export fn frobeniusNormSq(m: Mat2): f64;
export fn approxEq(a: Mat2, b: Mat2, eps: f64): bool;
