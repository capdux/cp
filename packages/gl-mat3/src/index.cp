// packages/@cap/mat3/src/index.cp
// Zero-allocation 3x3 row-major matrix & 2D affine transform library.
// MIT clean-room pure CP implementation.

/// 3x3 double-precision matrix stored in **row-major** order:
/// `[ m[0] m[1] m[2] ]`
/// `[ m[3] m[4] m[5] ]`
/// `[ m[6] m[7] m[8] ]`
pub struct Mat3 {
    pub m: [9]f64,
}

/// 2D point or direction vector for affine transformations.
pub struct Vec2D {
    pub x: f64,
    pub y: f64,
}

fn m3Abs(x: f64): f64 {
    if (x < (0.0 as f64)) {
        return -x;
    }
    return x;
}

fn m3WrapPi(rad: f64): f64 {
    const pi: f64 = 3.14159265358979323846;
    const twoPi: f64 = 6.28318530717958647692;
    let a: f64 = rad;
    if (a > pi || a < -pi) {
        const k: f64 = (((a + pi) / twoPi) as i64) as f64;
        a = a - k * twoPi;
        while (a > pi) { a = a - twoPi; }
        while (a < -pi) { a = a + twoPi; }
    }
    return a;
}

fn m3Sin(rad: f64): f64 {
    const pi: f64 = 3.14159265358979323846;
    const halfPi: f64 = 1.57079632679489661923;
    let x: f64 = m3WrapPi(rad);
    if (x > halfPi) {
        x = pi - x;
    } else if (x < -halfPi) {
        x = -pi - x;
    }
    const x2: f64 = x * x;
    let term: f64 = x;
    let sum: f64 = x;
    let n: usize = 1;
    while (n <= 10) {
        const d1: f64 = (2 * n) as f64;
        const d2: f64 = (2 * n + 1) as f64;
        term = -term * x2 / (d1 * d2);
        sum = sum + term;
        n += 1;
    }
    return sum;
}

fn m3Cos(rad: f64): f64 {
    const halfPi: f64 = 1.57079632679489661923;
    return m3Sin(rad + halfPi);
}

/// Construct the 3x3 all-zero matrix.
pub fn zero(): Mat3 {
    let arr: [9]f64 = undefined;
    let i: usize = 0;
    while (i < 9) {
        arr[i] = 0.0 as f64;
        i += 1;
    }
    return Mat3{ m: arr };
}

/// Construct the 3x3 identity matrix.
pub fn identity(): Mat3 {
    let mat = zero();
    mat.m[0] = 1.0 as f64;
    mat.m[4] = 1.0 as f64;
    mat.m[8] = 1.0 as f64;
    return mat;
}

/// Construct a 2D translation matrix by `(tx, ty)`.
pub fn fromTranslation(tx: f64, ty: f64): Mat3 {
    let mat = identity();
    mat.m[2] = tx;
    mat.m[5] = ty;
    return mat;
}

/// Construct a 2D non-uniform scaling matrix by `(sx, sy)`.
pub fn fromScale(sx: f64, sy: f64): Mat3 {
    let mat = zero();
    mat.m[0] = sx;
    mat.m[4] = sy;
    mat.m[8] = 1.0 as f64;
    return mat;
}

/// Construct a counter-clockwise 2D rotation matrix by `radians`.
pub fn fromRotation(radians: f64): Mat3 {
    const c: f64 = m3Cos(radians);
    const s: f64 = m3Sin(radians);
    let mat = zero();
    mat.m[0] = c;
    mat.m[1] = -s;
    mat.m[3] = s;
    mat.m[4] = c;
    mat.m[8] = 1.0 as f64;
    return mat;
}

/// Matrix multiplication `a * b` (row-major).
pub fn mul(a: Mat3, b: Mat3): Mat3 {
    let res: [9]f64 = undefined;
    let r: usize = 0;
    while (r < 3) {
        let c: usize = 0;
        while (c < 3) {
            const r3 = r * 3;
            res[r3 + c] = (a.m[r3 + 0] * b.m[0 + c]) + (a.m[r3 + 1] * b.m[3 + c]) + (a.m[r3 + 2] * b.m[6 + c]);
            c += 1;
        }
        r += 1;
    }
    return Mat3{ m: res };
}

/// Matrix transpose `a^T`.
pub fn transpose(a: Mat3): Mat3 {
    let res: [9]f64 = undefined;
    res[0] = a.m[0]; res[1] = a.m[3]; res[2] = a.m[6];
    res[3] = a.m[1]; res[4] = a.m[4]; res[5] = a.m[7];
    res[6] = a.m[2]; res[7] = a.m[5]; res[8] = a.m[8];
    return Mat3{ m: res };
}

/// Compute the determinant `det(a)`.
pub fn determinant(a: Mat3): f64 {
    const c0: f64 = (a.m[4] * a.m[8]) - (a.m[5] * a.m[7]);
    const c1: f64 = (a.m[3] * a.m[8]) - (a.m[5] * a.m[6]);
    const c2: f64 = (a.m[3] * a.m[7]) - (a.m[4] * a.m[6]);
    return (a.m[0] * c0) - (a.m[1] * c1) + (a.m[2] * c2);
}

/// Returns true if `a` is non-singular (`|det(a)| > 1e-12`).
pub fn isInvertible(a: Mat3): bool {
    return m3Abs(determinant(a)) > (0.000000000001 as f64);
}

/// Compute the matrix inverse `a^-1`. Returns `zero()` safely when `a` is singular.
pub fn inverse(a: Mat3): Mat3 {
    const det: f64 = determinant(a);
    if (m3Abs(det) <= (0.000000000001 as f64)) {
        return zero();
    }
    const invDet: f64 = (1.0 as f64) / det;
    let res: [9]f64 = undefined;

    res[0] = ((a.m[4] * a.m[8]) - (a.m[5] * a.m[7])) * invDet;
    res[1] = ((a.m[2] * a.m[7]) - (a.m[1] * a.m[8])) * invDet;
    res[2] = ((a.m[1] * a.m[5]) - (a.m[2] * a.m[4])) * invDet;

    res[3] = ((a.m[5] * a.m[6]) - (a.m[3] * a.m[8])) * invDet;
    res[4] = ((a.m[0] * a.m[8]) - (a.m[2] * a.m[6])) * invDet;
    res[5] = ((a.m[2] * a.m[3]) - (a.m[0] * a.m[5])) * invDet;

    res[6] = ((a.m[3] * a.m[7]) - (a.m[4] * a.m[6])) * invDet;
    res[7] = ((a.m[1] * a.m[6]) - (a.m[0] * a.m[7])) * invDet;
    res[8] = ((a.m[0] * a.m[4]) - (a.m[1] * a.m[3])) * invDet;

    return Mat3{ m: res };
}

/// Transform a 2D point `p` using homogeneous coordinates `(p.x, p.y, 1)`.
pub fn transformPoint(mat: Mat3, p: Vec2D): Vec2D {
    const x: f64 = (mat.m[0] * p.x) + (mat.m[1] * p.y) + mat.m[2];
    const y: f64 = (mat.m[3] * p.x) + (mat.m[4] * p.y) + mat.m[5];
    const w: f64 = (mat.m[6] * p.x) + (mat.m[7] * p.y) + mat.m[8];
    if (m3Abs(w - (1.0 as f64)) > (0.000000000001 as f64) && m3Abs(w) > (0.000000000001 as f64)) {
        return Vec2D{ x: x / w, y: y / w };
    }
    return Vec2D{ x: x, y: y };
}

/// Transform a 2D direction vector `v` using homogeneous coordinates `(v.x, v.y, 0)` (translation ignored).
pub fn transformDir(mat: Mat3, v: Vec2D): Vec2D {
    return Vec2D{
        x: (mat.m[0] * v.x) + (mat.m[1] * v.y),
        y: (mat.m[3] * v.x) + (mat.m[4] * v.y),
    };
}

/// Element-wise approximate equality between two 3x3 matrices within `eps`.
pub fn approxEq(a: Mat3, b: Mat3, eps: f64): bool {
    let i: usize = 0;
    while (i < 9) {
        if (m3Abs(a.m[i] - b.m[i]) > eps) {
            return false;
        }
        i += 1;
    }
    return true;
}

export {
    Mat3,
    Vec2D,
    identity,
    zero,
    fromTranslation,
    fromScale,
    fromRotation,
    mul,
    transpose,
    determinant,
    isInvertible,
    inverse,
    transformPoint,
    transformDir,
    approxEq,
};
