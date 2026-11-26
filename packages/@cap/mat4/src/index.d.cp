// Type definitions for CP

export fn absF64(x: f64): f64;
export fn sqrtF64(x: f64): f64;
export fn reduceAngle(rad: f64): f64;
export fn sinF64(rad: f64): f64;
export fn cosF64(rad: f64): f64;
export fn tanF64(rad: f64): f64;
export class Vec3D {
    x: f64,
    y: f64,
    z: f64,
    fn new(x: f64, y: f64, z: f64): Vec3D;
    fn approxEq(other: Vec3D, eps: f64): bool;
}

export fn v3Sub(a: Vec3D, b: Vec3D): Vec3D;
export fn v3Dot(a: Vec3D, b: Vec3D): f64;
export fn v3Cross(a: Vec3D, b: Vec3D): Vec3D;
export fn v3Normalize(v: Vec3D): Vec3D;
export class Mat4 {
    m: [16]f64,
    fn zero(): Mat4;
    fn identity(): Mat4;
    fn fromTranslation(tx: f64, ty: f64, tz: f64): Mat4;
    fn fromScale(sx: f64, sy: f64, sz: f64): Mat4;
    fn fromRotationX(rad: f64): Mat4;
    fn fromRotationY(rad: f64): Mat4;
    fn fromRotationZ(rad: f64): Mat4;
    fn orthographic(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64): Mat4;
    fn perspective(fov_y_rad: f64, aspect: f64, near: f64, far: f64): Mat4;
    fn lookAt(eye: Vec3D, target: Vec3D, up: Vec3D): Mat4;
}

export fn identity(): Mat4;
export fn zero(): Mat4;
export fn fromTranslation(tx: f64, ty: f64, tz: f64): Mat4;
export fn fromScale(sx: f64, sy: f64, sz: f64): Mat4;
export fn fromRotationX(rad: f64): Mat4;
export fn fromRotationY(rad: f64): Mat4;
export fn fromRotationZ(rad: f64): Mat4;
export fn orthographic(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64): Mat4;
export fn perspective(fov_y_rad: f64, aspect: f64, near: f64, far: f64): Mat4;
export fn lookAt(eye: Vec3D, target: Vec3D, up: Vec3D): Mat4;
export fn mul(a: Mat4, b: Mat4): Mat4;
export fn transpose(a: Mat4): Mat4;
export fn transformPoint(mat: Mat4, p: Vec3D): Vec3D;
export fn transformDir(mat: Mat4, v: Vec3D): Vec3D;
export fn approxEq(a: Mat4, b: Mat4, eps: f64): bool;
