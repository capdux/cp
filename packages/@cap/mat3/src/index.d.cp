// Type definitions for CP

export class Mat3 {
    m: [9]f64,
}

export class Vec2D {
    x: f64,
    y: f64,
}

export fn m3Abs(x: f64): f64;
export fn m3WrapPi(rad: f64): f64;
export fn m3Sin(rad: f64): f64;
export fn m3Cos(rad: f64): f64;
export fn zero(): Mat3;
export fn identity(): Mat3;
export fn fromTranslation(tx: f64, ty: f64): Mat3;
export fn fromScale(sx: f64, sy: f64): Mat3;
export fn fromRotation(radians: f64): Mat3;
export fn mul(a: Mat3, b: Mat3): Mat3;
export fn transpose(a: Mat3): Mat3;
export fn determinant(a: Mat3): f64;
export fn isInvertible(a: Mat3): bool;
export fn inverse(a: Mat3): Mat3;
export fn transformPoint(mat: Mat3, p: Vec2D): Vec2D;
export fn transformDir(mat: Mat3, v: Vec2D): Vec2D;
export fn approxEq(a: Mat3, b: Mat3, eps: f64): bool;
