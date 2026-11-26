// Type definitions for CP

export fn sqrtF64(x: f64): f64;
export fn absF64(x: f64): f64;
export fn minF64(a: f64, b: f64): f64;
export fn maxF64(a: f64, b: f64): f64;
export class Vec3 {
    x: f64,
    y: f64,
    z: f64,
    fn new(x: f64, y: f64, z: f64): Vec3;
    fn zero(): Vec3;
    fn splat(v: f64): Vec3;
    fn unitX(): Vec3;
    fn unitY(): Vec3;
    fn unitZ(): Vec3;
    fn add(other: Vec3): Vec3;
    fn sub(other: Vec3): Vec3;
    fn mul(other: Vec3): Vec3;
    fn scale(s: f64): Vec3;
    fn neg(): Vec3;
    fn dot(other: Vec3): f64;
    fn cross(other: Vec3): Vec3;
    fn lengthSq(): f64;
    fn length(): f64;
    fn distance(other: Vec3): f64;
    fn normalize(): Vec3;
    fn lerp(other: Vec3, t: f64): Vec3;
    fn reflect(normal: Vec3): Vec3;
    fn project(onto: Vec3): Vec3;
    fn minVec(other: Vec3): Vec3;
    fn maxVec(other: Vec3): Vec3;
    fn clampLength(max_len: f64): Vec3;
    fn approxEq(other: Vec3, eps: f64): bool;
}

export fn new(x: f64, y: f64, z: f64): Vec3;
export fn zero(): Vec3;
export fn splat(v: f64): Vec3;
export fn unitX(): Vec3;
export fn unitY(): Vec3;
export fn unitZ(): Vec3;
export fn add(a: Vec3, b: Vec3): Vec3;
export fn sub(a: Vec3, b: Vec3): Vec3;
export fn mul(a: Vec3, b: Vec3): Vec3;
export fn scale(v: Vec3, s: f64): Vec3;
export fn neg(v: Vec3): Vec3;
export fn dot(a: Vec3, b: Vec3): f64;
export fn cross(a: Vec3, b: Vec3): Vec3;
export fn lengthSq(v: Vec3): f64;
export fn length(v: Vec3): f64;
export fn distance(a: Vec3, b: Vec3): f64;
export fn normalize(v: Vec3): Vec3;
export fn lerp(a: Vec3, b: Vec3, t: f64): Vec3;
export fn reflect(v: Vec3, normal: Vec3): Vec3;
export fn project(v: Vec3, onto: Vec3): Vec3;
export fn minVec(a: Vec3, b: Vec3): Vec3;
export fn maxVec(a: Vec3, b: Vec3): Vec3;
export fn clampLength(v: Vec3, max_len: f64): Vec3;
export fn approxEq(a: Vec3, b: Vec3, eps: f64): bool;
