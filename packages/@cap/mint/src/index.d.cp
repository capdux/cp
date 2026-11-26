// Type definitions for CP

export class Point2 {
    x: f64,
    y: f64,
    fn new(x: f64, y: f64): Point2;
    fn fromSlice(s: []f64): Point2;
    fn toArray(): [2]f64;
}

export class Point3 {
    x: f64,
    y: f64,
    z: f64,
    fn new(x: f64, y: f64, z: f64): Point3;
    fn toArray(): [3]f64;
}

export class Vector2 {
    x: f64,
    y: f64,
    fn new(x: f64, y: f64): Vector2;
    fn dot(other: Vector2): f64;
    fn lenSq(): f64;
    fn toArray(): [2]f64;
}

export class Vector3 {
    x: f64,
    y: f64,
    z: f64,
    fn new(x: f64, y: f64, z: f64): Vector3;
    fn dot(other: Vector3): f64;
    fn cross(other: Vector3): Vector3;
    fn lenSq(): f64;
    fn toArray(): [3]f64;
}

export class Vector4 {
    x: f64,
    y: f64,
    z: f64,
    w: f64,
    fn new(x: f64, y: f64, z: f64, w: f64): Vector4;
    fn dot(other: Vector4): f64;
    fn toArray(): [4]f64;
}

export class Quaternion {
    v: Vector3,
    s: f64,
    fn identity(): Quaternion;
    fn new(x: f64, y: f64, z: f64, s: f64): Quaternion;
    fn conjugate(): Quaternion;
    fn normSq(): f64;
    fn mul(rhs: Quaternion): Quaternion;
}

export class EulerAngles {
    a: f64,
    b: f64,
    c: f64,
    fn new(a: f64, b: f64, c: f64): EulerAngles;
}

export class RowMatrix2 {
    x: Vector2,
    y: Vector2,
    fn identity(): RowMatrix2;
    fn det(): f64;
    fn transpose(): RowMatrix2;
}

export class RowMatrix3 {
    x: Vector3,
    y: Vector3,
    z: Vector3,
    fn identity(): RowMatrix3;
    fn transpose(): RowMatrix3;
    fn mulVec3(v: Vector3): Vector3;
}

export class RowMatrix4 {
    x: Vector4,
    y: Vector4,
    z: Vector4,
    w: Vector4,
    fn identity(): RowMatrix4;
    fn transpose(): RowMatrix4;
    fn mulVec4(v: Vector4): Vector4;
    fn toColumnMajor(): ColumnMatrix4;
}

export class ColumnMatrix4 {
    x: Vector4,
    y: Vector4,
    z: Vector4,
    w: Vector4,
    fn identity(): ColumnMatrix4;
    fn toRowMajor(): RowMatrix4;
}

export fn point3ToVector3(p: Point3): Vector3;
export fn vector3ToPoint3(v: Vector3): Point3;
