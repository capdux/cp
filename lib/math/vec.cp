// Source: cp/lib/math/vec.cp (2D/3D/4D Vector & Mat4 Linear Algebra)
// Pure CP 2D/3D/4D f32 vectors and column-major 4x4 matrix linear algebra.

const std = @import("std");

const EPSILON_F32: f32 = 0.000001;

/// 2D single-precision floating-point vector (value type).
pub struct Vec2 {
    x: f32 = 0.0;
    y: f32 = 0.0;

    /// Constructs a Vec2 from (x, y).
    static fn init(x: f32, y: f32): Vec2 {
        return Vec2 { x: x, y: y };
    }

    /// Returns the zero vector (0, 0).
    static fn zero(): Vec2 {
        return Vec2 { x: 0.0, y: 0.0 };
    }

    /// Broadcasts a scalar across both components.
    static fn splat(v: f32): Vec2 {
        return Vec2 { x: v, y: v };
    }

    /// Component-wise addition.
    fn add(other: Vec2): Vec2 {
        return Vec2 { x: _.x + other.x, y: _.y + other.y };
    }

    /// Component-wise subtraction.
    fn sub(other: Vec2): Vec2 {
        return Vec2 { x: _.x - other.x, y: _.y - other.y };
    }

    /// Multiplies both components by scalar `s`.
    fn scale(s: f32): Vec2 {
        return Vec2 { x: _.x * s, y: _.y * s };
    }

    /// Returns the Euclidean dot product.
    fn dot(other: Vec2): f32 {
        return _.x * other.x + _.y * other.y;
    }

    /// Returns the squared Euclidean length.
    fn lengthSq(): f32 {
        return _.x * _.x + _.y * _.y;
    }

    /// Returns the Euclidean length.
    fn length(): f32 {
        return @sqrt(_.x * _.x + _.y * _.y);
    }

    /// Returns a unit-length vector in the same direction, or zero if length is near zero.
    fn normalize(): Vec2 {
        const len = _.length();
        if (len <= 0.0) return Vec2.zero();
        const inv = 1.0 / len;
        return Vec2 { x: _.x * inv, y: _.y * inv };
    }

    /// Linearly interpolates between `_` and `other` by parameter `t`.
    fn lerp(other: Vec2, t: f32): Vec2 {
        return Vec2 {
            x: _.x + (other.x - _.x) * t,
            y: _.y + (other.y - _.y) * t,
        };
    }

    /// Returns the Euclidean distance between `_` and `other`.
    fn distance(other: Vec2): f32 {
        return _.sub(other).length();
    }

    /// Approximate component-wise equality within tolerance `eps`.
    fn approxEql(other: Vec2, eps: f32 = EPSILON_F32): bool {
        return @abs(_.x - other.x) <= eps && @abs(_.y - other.y) <= eps;
    }
}

/// 3D single-precision floating-point vector (value type).
pub struct Vec3 {
    x: f32 = 0.0;
    y: f32 = 0.0;
    z: f32 = 0.0;

    /// Constructs a Vec3 from (x, y, z).
    static fn init(x: f32, y: f32, z: f32): Vec3 {
        return Vec3 { x: x, y: y, z: z };
    }

    /// Returns the zero vector (0, 0, 0).
    static fn zero(): Vec3 {
        return Vec3 { x: 0.0, y: 0.0, z: 0.0 };
    }

    /// Broadcasts a scalar across all three components.
    static fn splat(v: f32): Vec3 {
        return Vec3 { x: v, y: v, z: v };
    }

    /// Component-wise addition.
    fn add(other: Vec3): Vec3 {
        return Vec3 { x: _.x + other.x, y: _.y + other.y, z: _.z + other.z };
    }

    /// Component-wise subtraction.
    fn sub(other: Vec3): Vec3 {
        return Vec3 { x: _.x - other.x, y: _.y - other.y, z: _.z - other.z };
    }

    /// Multiplies all components by scalar `s`.
    fn scale(s: f32): Vec3 {
        return Vec3 { x: _.x * s, y: _.y * s, z: _.z * s };
    }

    /// Returns the Euclidean dot product.
    fn dot(other: Vec3): f32 {
        return _.x * other.x + _.y * other.y + _.z * other.z;
    }

    /// Returns the 3D right-handed cross product `_ x other`.
    fn cross(other: Vec3): Vec3 {
        return Vec3 {
            x: _.y * other.z - _.z * other.y,
            y: _.z * other.x - _.x * other.z,
            z: _.x * other.y - _.y * other.x,
        };
    }

    /// Returns the squared Euclidean length.
    fn lengthSq(): f32 {
        return _.x * _.x + _.y * _.y + _.z * _.z;
    }

    /// Returns the Euclidean length.
    fn length(): f32 {
        return @sqrt(_.x * _.x + _.y * _.y + _.z * _.z);
    }

    /// Returns a unit-length vector in the same direction, or zero if length is zero.
    fn normalize(): Vec3 {
        const len = _.length();
        if (len <= 0.0) return Vec3.zero();
        const inv = 1.0 / len;
        return Vec3 { x: _.x * inv, y: _.y * inv, z: _.z * inv };
    }

    /// Linearly interpolates between `_` and `other` by parameter `t`.
    fn lerp(other: Vec3, t: f32): Vec3 {
        return Vec3 {
            x: _.x + (other.x - _.x) * t,
            y: _.y + (other.y - _.y) * t,
            z: _.z + (other.z - _.z) * t,
        };
    }

    /// Returns the Euclidean distance between `_` and `other`.
    fn distance(other: Vec3): f32 {
        return _.sub(other).length();
    }

    /// Approximate component-wise equality within tolerance `eps`.
    fn approxEql(other: Vec3, eps: f32 = EPSILON_F32): bool {
        return @abs(_.x - other.x) <= eps
            && @abs(_.y - other.y) <= eps
            && @abs(_.z - other.z) <= eps;
    }
}

/// 4D single-precision floating-point vector (value type).
pub struct Vec4 {
    x: f32 = 0.0;
    y: f32 = 0.0;
    z: f32 = 0.0;
    w: f32 = 0.0;

    /// Constructs a Vec4 from (x, y, z, w).
    static fn init(x: f32, y: f32, z: f32, w: f32): Vec4 {
        return Vec4 { x: x, y: y, z: z, w: w };
    }

    /// Returns the zero vector (0, 0, 0, 0).
    static fn zero(): Vec4 {
        return Vec4 { x: 0.0, y: 0.0, z: 0.0, w: 0.0 };
    }

    /// Broadcasts a scalar across all four components.
    static fn splat(v: f32): Vec4 {
        return Vec4 { x: v, y: v, z: v, w: v };
    }

    /// Component-wise addition.
    fn add(other: Vec4): Vec4 {
        return Vec4 {
            x: _.x + other.x,
            y: _.y + other.y,
            z: _.z + other.z,
            w: _.w + other.w,
        };
    }

    /// Component-wise subtraction.
    fn sub(other: Vec4): Vec4 {
        return Vec4 {
            x: _.x - other.x,
            y: _.y - other.y,
            z: _.z - other.z,
            w: _.w - other.w,
        };
    }

    /// Multiplies all components by scalar `s`.
    fn scale(s: f32): Vec4 {
        return Vec4 {
            x: _.x * s,
            y: _.y * s,
            z: _.z * s,
            w: _.w * s,
        };
    }

    /// Returns the Euclidean dot product.
    fn dot(other: Vec4): f32 {
        return _.x * other.x + _.y * other.y + _.z * other.z + _.w * other.w;
    }

    /// Returns the squared Euclidean length.
    fn lengthSq(): f32 {
        return _.x * _.x + _.y * _.y + _.z * _.z + _.w * _.w;
    }

    /// Returns the Euclidean length.
    fn length(): f32 {
        return @sqrt(_.x * _.x + _.y * _.y + _.z * _.z + _.w * _.w);
    }

    /// Returns a unit-length vector in the same direction, or zero if length is zero.
    fn normalize(): Vec4 {
        const len = _.length();
        if (len <= 0.0) return Vec4.zero();
        const inv = 1.0 / len;
        return Vec4 {
            x: _.x * inv,
            y: _.y * inv,
            z: _.z * inv,
            w: _.w * inv,
        };
    }

    /// Linearly interpolates between `_` and `other` by parameter `t`.
    fn lerp(other: Vec4, t: f32): Vec4 {
        return Vec4 {
            x: _.x + (other.x - _.x) * t,
            y: _.y + (other.y - _.y) * t,
            z: _.z + (other.z - _.z) * t,
            w: _.w + (other.w - _.w) * t,
        };
    }

    /// Returns the Euclidean distance between `_` and `other`.
    fn distance(other: Vec4): f32 {
        return _.sub(other).length();
    }

    /// Approximate component-wise equality within tolerance `eps`.
    fn approxEql(other: Vec4, eps: f32 = EPSILON_F32): bool {
        return @abs(_.x - other.x) <= eps
            && @abs(_.y - other.y) <= eps
            && @abs(_.z - other.z) <= eps
            && @abs(_.w - other.w) <= eps;
    }
}

/// Column-major 4x4 single-precision matrix (`data[col * 4 + row]`).
pub struct Mat4 {
    data: [16]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];

    /// Returns the 4x4 identity matrix.
    static fn identity(): Mat4 {
        return Mat4 {
            data: [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
            ],
        };
    }

    /// Constructs a Mat4 from four column vectors.
    static fn fromCols(c0: Vec4, c1: Vec4, c2: Vec4, c3: Vec4): Mat4 {
        return Mat4 {
            data: [
                c0.x, c0.y, c0.z, c0.w,
                c1.x, c1.y, c1.z, c1.w,
                c2.x, c2.y, c2.z, c2.w,
                c3.x, c3.y, c3.z, c3.w,
            ],
        };
    }

    /// Constructs a 3D translation matrix for `(tx, ty, tz)`.
    static fn translation(tx: f32, ty: f32, tz: f32): Mat4 {
        return Mat4 {
            data: [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                tx,  ty,  tz,  1.0,
            ],
        };
    }

    /// Constructs a 3D non-uniform scaling matrix for `(sx, sy, sz)`.
    static fn scaling(sx: f32, sy: f32, sz: f32): Mat4 {
        return Mat4 {
            data: [
                sx,  0.0, 0.0, 0.0,
                0.0, sy,  0.0, 0.0,
                0.0, 0.0, sz,  0.0,
                0.0, 0.0, 0.0, 1.0,
            ],
        };
    }

    /// Constructs a counter-clockwise rotation matrix around the Z axis by `radians`.
    static fn rotationZ(radians: f32): Mat4 {
        const c = @cos(radians);
        const s = @sin(radians);
        return Mat4 {
            data: [
                c,   s,   0.0, 0.0,
                -s,  c,   0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
            ],
        };
    }

    /// Reads element at `(row, col)` (0-indexed).
    fn get(row: usize, col: usize): f32 {
        return _.data[col * 4 + row];
    }

    /// Matrix multiplication `_ * other` (column-major).
    fn mul(other: Mat4): Mat4 {
        let out: [16]f32 = [0.0; 16];
        for (col of 0..4) {
            for (row of 0..4) {
                let sum: f32 = 0.0;
                for (k of 0..4) {
                    sum += _.data[k * 4 + row] * other.data[col * 4 + k];
                }
                out[col * 4 + row] = sum;
            }
        }
        return Mat4 { data: out };
    }

    /// Multiplies column vector `v` by `_` (`_ * v`).
    fn mulVec4(v: Vec4): Vec4 {
        const d = _.data;
        return Vec4 {
            x: d[0] * v.x + d[4] * v.y + d[8] * v.z + d[12] * v.w,
            y: d[1] * v.x + d[5] * v.y + d[9] * v.z + d[13] * v.w,
            z: d[2] * v.x + d[6] * v.y + d[10] * v.z + d[14] * v.w,
            w: d[3] * v.x + d[7] * v.y + d[11] * v.z + d[15] * v.w,
        };
    }

    /// Returns the transposed 4x4 matrix.
    fn transpose(): Mat4 {
        let out: [16]f32 = [0.0; 16];
        for (col of 0..4) {
            for (row of 0..4) {
                out[col * 4 + row] = _.data[row * 4 + col];
            }
        }
        return Mat4 { data: out };
    }

    /// Approximate element-wise equality within tolerance `eps`.
    fn approxEql(other: Mat4, eps: f32 = EPSILON_F32): bool {
        for (i of 0..16) {
            if (@abs(_.data[i] - other.data[i]) > eps) return false;
        }
        return true;
    }
}
