// Type definitions for CP

export fn absF64(x: f64): f64;
export fn sqrtF64(x: f64): f64;
export fn sinF64(angle: f64): f64;
export fn cosF64(angle: f64): f64;
export class Vec3 {
    x: f64,
    y: f64,
    z: f64,
    fn init(x: f64, y: f64, z: f64): Vec3;
    fn add(other: Vec3): Vec3;
    fn sub(other: Vec3): Vec3;
    fn scale(s: f64): Vec3;
    fn dot(other: Vec3): f64;
    fn cross(other: Vec3): Vec3;
    fn lengthSq(): f64;
    fn length(): f64;
    fn normalize(): Vec3;
    fn lerp(other: Vec3, t: f64): Vec3;
    fn distance(other: Vec3): f64;
}

export class Quat {
    x: f64,
    y: f64,
    z: f64,
    w: f64,
    fn identity(): Quat;
    fn fromAxisAngle(axis: Vec3, angle_rad: f64): Quat;
    fn normalize(): Quat;
    fn conjugate(): Quat;
    fn mul(other: Quat): Quat;
    fn rotateVec3(v: Vec3): Vec3;
}

export class Mat4 {
    m: [16]f64,
    fn identity(): Mat4;
    fn translation(t: Vec3): Mat4;
    fn scaling(s: Vec3): Mat4;
    fn rotationZ(angle_rad: f64): Mat4;
    fn mul(other: Mat4): Mat4;
    fn transformPoint(p: Vec3): Vec3;
    fn determinant(): f64;
}

