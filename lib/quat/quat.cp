// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc 3D Unit Quaternion, Euler ZYX, Mat3 & SLERP/NLERP Library)

import std from "std";

const EPS: f64 = 1.0e-12;

fn absF64(v: f64): f64 {
    if (v < 0.0) {
        return 0.0 - v;
    }
    return v;
}

/// 3D Euclidean vector (`f64`).
pub struct Vec3 {
    x: f64,
    y: f64,
    z: f64,

    fn add(other: Vec3): Vec3 {
        return Vec3 {
            x: _.x + other.x,
            y: _.y + other.y,
            z: _.z + other.z,
        };
    }

    fn sub(other: Vec3): Vec3 {
        return Vec3 {
            x: _.x - other.x,
            y: _.y - other.y,
            z: _.z - other.z,
        };
    }

    fn scale(s: f64): Vec3 {
        return Vec3 {
            x: _.x * s,
            y: _.y * s,
            z: _.z * s,
        };
    }

    fn dot(other: Vec3): f64 {
        return _.x * other.x + _.y * other.y + _.z * other.z;
    }

    fn cross(other: Vec3): Vec3 {
        return Vec3 {
            x: _.y * other.z - _.z * other.y,
            y: _.z * other.x - _.x * other.z,
            z: _.x * other.y - _.y * other.x,
        };
    }

    fn len(): f64 {
        return std.math.sqrt(_.x * _.x + _.y * _.y + _.z * _.z);
    }

    fn normalize(): Vec3? {
        const l: f64 = _.len();
        if (l <= EPS) {
            return null;
        }
        return _.scale(1.0 / l);
    }
}

/// Intrinsic ZYX (Yaw-Pitch-Roll) Euler angles in radians.
pub struct EulerAngles {
    roll: f64,
    pitch: f64,
    yaw: f64,
}

/// Row-major 3x3 matrix (`[9]f64`).
pub struct Mat3 {
    m: [9]f64,

    fn mulVec3(v: Vec3): Vec3 {
        return Vec3 {
            x: _.m[0] * v.x + _.m[1] * v.y + _.m[2] * v.z,
            y: _.m[3] * v.x + _.m[4] * v.y + _.m[5] * v.z,
            z: _.m[6] * v.x + _.m[7] * v.y + _.m[8] * v.z,
        };
    }

    fn transpose(): Mat3 {
        return Mat3 {
            m: [
                _.m[0], _.m[3], _.m[6],
                _.m[1], _.m[4], _.m[7],
                _.m[2], _.m[5], _.m[8],
            ],
        };
    }

    fn det(): f64 {
        const a: f64 = _.m[0] * (_.m[4] * _.m[8] - _.m[5] * _.m[7]);
        const b: f64 = _.m[1] * (_.m[3] * _.m[8] - _.m[5] * _.m[6]);
        const c: f64 = _.m[2] * (_.m[3] * _.m[7] - _.m[4] * _.m[6]);
        return a - b + c;
    }
}

/// 3D Quaternion `(w, x, y, z)` using Hamilton multiplication conventions.
pub struct Quat {
    w: f64,
    x: f64,
    y: f64,
    z: f64,

    /// Identity rotation `(1, 0, 0, 0)`.
    static fn identity(): Quat {
        return Quat { w: 1.0, x: 0.0, y: 0.0, z: 0.0 };
    }

    /// Constructs a unit quaternion rotating by `angle_rad` around `axis`.
    /// Returns `null` if `axis` has zero length.
    static fn fromAxisAngle(axis: Vec3, angle_rad: f64): Quat? {
        if (u := axis.normalize()) {
            const half: f64 = angle_rad * 0.5;
            const s: f64 = std.math.sin(half);
            const c: f64 = std.math.cos(half);
            return Quat {
                w: c,
                x: u.x * s,
                y: u.y * s,
                z: u.z * s,
            };
        }
        return null;
    }

    /// Constructs a unit quaternion from intrinsic ZYX Euler angles (`roll` X, `pitch` Y, `yaw` Z, radians).
    static fn fromEulerZYX(roll: f64, pitch: f64, yaw: f64): Quat {
        const cr: f64 = std.math.cos(roll * 0.5);
        const sr: f64 = std.math.sin(roll * 0.5);
        const cp: f64 = std.math.cos(pitch * 0.5);
        const sp: f64 = std.math.sin(pitch * 0.5);
        const cy: f64 = std.math.cos(yaw * 0.5);
        const sy: f64 = std.math.sin(yaw * 0.5);

        return Quat {
            w: cr * cp * cy + sr * sp * sy,
            x: sr * cp * cy - cr * sp * sy,
            y: cr * sp * cy + sr * cp * sy,
            z: cr * cp * sy - sr * sp * cy,
        };
    }

    /// Converts a 3x3 rotation matrix to a unit quaternion using Shepperd's numerically stable method.
    static fn fromMat3(mat: Mat3): Quat {
        const m00: f64 = mat.m[0];
        const m01: f64 = mat.m[1];
        const m02: f64 = mat.m[2];
        const m10: f64 = mat.m[3];
        const m11: f64 = mat.m[4];
        const m12: f64 = mat.m[5];
        const m20: f64 = mat.m[6];
        const m21: f64 = mat.m[7];
        const m22: f64 = mat.m[8];

        const tr: f64 = m00 + m11 + m22;
        let q: Quat = Quat.identity();

        if (tr > 0.0) {
            const s: f64 = std.math.sqrt(tr + 1.0) * 2.0;
            q = Quat {
                w: 0.25 * s,
                x: (m21 - m12) / s,
                y: (m02 - m20) / s,
                z: (m10 - m01) / s,
            };
        } else if (m00 > m11 && m00 > m22) {
            const s: f64 = std.math.sqrt(1.0 + m00 - m11 - m22) * 2.0;
            q = Quat {
                w: (m21 - m12) / s,
                x: 0.25 * s,
                y: (m01 + m10) / s,
                z: (m02 + m20) / s,
            };
        } else if (m11 > m22) {
            const s: f64 = std.math.sqrt(1.0 + m11 - m00 - m22) * 2.0;
            q = Quat {
                w: (m02 - m20) / s,
                x: (m01 + m10) / s,
                y: 0.25 * s,
                z: (m12 + m21) / s,
            };
        } else {
            const s: f64 = std.math.sqrt(1.0 + m22 - m00 - m11) * 2.0;
            q = Quat {
                w: (m10 - m01) / s,
                x: (m02 + m20) / s,
                y: (m12 + m21) / s,
                z: 0.25 * s,
            };
        }

        if (unit := q.normalize()) {
            return unit;
        }
        return Quat.identity();
    }

    /// Computes the shortest-arc unit quaternion rotating direction `from_vec` onto `to_vec`.
    /// Handles parallel (`0 deg`) and anti-parallel (`180 deg`) singularities cleanly; returns `null` on zero vector.
    static fn fromToRotation(from_vec: Vec3, to_vec: Vec3): Quat? {
        if (u := from_vec.normalize()) {
            if (v := to_vec.normalize()) {
                const d: f64 = u.dot(v);
                if (d >= 1.0 - EPS) {
                    return Quat.identity();
                }
                if (d <= EPS - 1.0) {
                    // 180-degree antiparallel singularity: pick an orthogonal axis.
                    let ref_axis: Vec3 = Vec3 { x: 1.0, y: 0.0, z: 0.0 };
                    if (absF64(u.x) >= 0.9) {
                        ref_axis = Vec3 { x: 0.0, y: 1.0, z: 0.0 };
                    }
                    if (orth := u.cross(ref_axis).normalize()) {
                        return Quat { w: 0.0, x: orth.x, y: orth.y, z: orth.z };
                    }
                    return null;
                }
                const c: Vec3 = u.cross(v);
                const raw: Quat = Quat { w: 1.0 + d, x: c.x, y: c.y, z: c.z };
                return raw.normalize();
            }
        }
        return null;
    }

    fn normSq(): f64 {
        return _.w * _.w + _.x * _.x + _.y * _.y + _.z * _.z;
    }

    fn norm(): f64 {
        return std.math.sqrt(_.normSq());
    }

    fn normalize(): Quat? {
        const n: f64 = _.norm();
        if (n <= EPS) {
            return null;
        }
        const inv_n: f64 = 1.0 / n;
        return Quat {
            w: _.w * inv_n,
            x: _.x * inv_n,
            y: _.y * inv_n,
            z: _.z * inv_n,
        };
    }

    fn conjugate(): Quat {
        return Quat {
            w: _.w,
            x: 0.0 - _.x,
            y: 0.0 - _.y,
            z: 0.0 - _.z,
        };
    }

    fn inverse(): Quat? {
        const nsq: f64 = _.normSq();
        if (nsq <= EPS * EPS) {
            return null;
        }
        const inv_nsq: f64 = 1.0 / nsq;
        return Quat {
            w: _.w * inv_nsq,
            x: (0.0 - _.x) * inv_nsq,
            y: (0.0 - _.y) * inv_nsq,
            z: (0.0 - _.z) * inv_nsq,
        };
    }

    fn negate(): Quat {
        return Quat {
            w: 0.0 - _.w,
            x: 0.0 - _.x,
            y: 0.0 - _.y,
            z: 0.0 - _.z,
        };
    }

    fn dot(other: Quat): f64 {
        return _.w * other.w + _.x * other.x + _.y * other.y + _.z * other.z;
    }

    /// Hamilton product `this * other`.
    fn mul(other: Quat): Quat {
        return Quat {
            w: _.w * other.w - _.x * other.x - _.y * other.y - _.z * other.z,
            x: _.w * other.x + _.x * other.w + _.y * other.z - _.z * other.y,
            y: _.w * other.y - _.x * other.z + _.y * other.w + _.z * other.x,
            z: _.w * other.z + _.x * other.y - _.y * other.x + _.z * other.w,
        };
    }

    /// Rotates a 3D vector `v` by this unit quaternion (`v' = q * (0, v) * q*`).
    fn rotateVec3(v: Vec3): Vec3 {
        const u: Vec3 = Vec3 { x: _.x, y: _.y, z: _.z };
        const t: Vec3 = u.cross(v).scale(2.0);
        return v.add(t.scale(_.w)).add(u.cross(t));
    }

    /// Converts this unit quaternion to a row-major 3x3 rotation matrix.
    fn toMat3(): Mat3 {
        const xx: f64 = _.x * _.x;
        const yy: f64 = _.y * _.y;
        const zz: f64 = _.z * _.z;
        const xy: f64 = _.x * _.y;
        const xz: f64 = _.x * _.z;
        const yz: f64 = _.y * _.z;
        const wx: f64 = _.w * _.x;
        const wy: f64 = _.w * _.y;
        const wz: f64 = _.w * _.z;

        return Mat3 {
            m: [
                1.0 - 2.0 * (yy + zz), 2.0 * (xy - wz),       2.0 * (xz + wy),
                2.0 * (xy + wz),       1.0 - 2.0 * (xx + zz), 2.0 * (yz - wx),
                2.0 * (xz - wy),       2.0 * (yz + wx),       1.0 - 2.0 * (xx + yy),
            ],
        };
    }

    /// Extracts intrinsic ZYX Euler angles (`roll`, `pitch`, `yaw` in radians),
    /// handling `pitch = +-pi/2` gimbal lock safely.
    fn toEulerZYX(): EulerAngles {
        const sinp: f64 = 2.0 * (_.w * _.y - _.z * _.x);
        const half_pi: f64 = std.math.pi * 0.5;

        if (sinp >= 1.0 - EPS) {
            return EulerAngles {
                roll: 0.0,
                pitch: half_pi,
                yaw: 2.0 * std.math.atan2(_.z, _.w),
            };
        }
        if (sinp <= EPS - 1.0) {
            return EulerAngles {
                roll: 0.0,
                pitch: 0.0 - half_pi,
                yaw: 2.0 * std.math.atan2(_.z, _.w),
            };
        }

        const sinr_cosp: f64 = 2.0 * (_.w * _.x + _.y * _.z);
        const cosr_cosp: f64 = 1.0 - 2.0 * (_.x * _.x + _.y * _.y);
        const siny_cosp: f64 = 2.0 * (_.w * _.z + _.x * _.y);
        const cosy_cosp: f64 = 1.0 - 2.0 * (_.y * _.y + _.z * _.z);

        return EulerAngles {
            roll: std.math.atan2(sinr_cosp, cosr_cosp),
            pitch: std.math.asin(sinp),
            yaw: std.math.atan2(siny_cosp, cosy_cosp),
        };
    }

    /// Shortest angular distance in radians `[0, pi]` between two rotations (accounting for `q == -q`).
    fn angleTo(other: Quat): f64 {
        let qa: Quat = Quat { w: _.w, x: _.x, y: _.y, z: _.z };
        let qb: Quat = other;
        if (na := qa.normalize()) {
            qa = na;
        }
        if (nb := qb.normalize()) {
            qb = nb;
        }
        let d: f64 = absF64(qa.dot(qb));
        if (d > 1.0) {
            d = 1.0;
        }
        return 2.0 * std.math.acos(d);
    }

    /// Shortest-arc Normalized Linear Interpolation (NLERP).
    static fn nlerp(a: Quat, b: Quat, t: f64): Quat {
        let target: Quat = b;
        if (a.dot(b) < 0.0) {
            target = b.negate();
        }
        const one_minus_t: f64 = 1.0 - t;
        const blended: Quat = Quat {
            w: one_minus_t * a.w + t * target.w,
            x: one_minus_t * a.x + t * target.x,
            y: one_minus_t * a.y + t * target.y,
            z: one_minus_t * a.z + t * target.z,
        };
        if (unit := blended.normalize()) {
            return unit;
        }
        return Quat.identity();
    }

    /// Shortest-arc Spherical Linear Interpolation (SLERP) with smooth NLERP fallback near `theta -> 0`.
    static fn slerp(a: Quat, b: Quat, t: f64): Quat {
        let qa: Quat = a;
        let qb: Quat = b;
        if (na := a.normalize()) {
            qa = na;
        }
        if (nb := b.normalize()) {
            qb = nb;
        }

        let d: f64 = qa.dot(qb);
        if (d < 0.0) {
            qb = qb.negate();
            d = 0.0 - d;
        }
        if (d > 0.999995) {
            return Quat.nlerp(qa, qb, t);
        }
        if (d > 1.0) {
            d = 1.0;
        }

        const theta: f64 = std.math.acos(d);
        const sin_theta: f64 = std.math.sin(theta);
        const w0: f64 = std.math.sin((1.0 - t) * theta) / sin_theta;
        const w1: f64 = std.math.sin(t * theta) / sin_theta;

        const res: Quat = Quat {
            w: w0 * qa.w + w1 * qb.w,
            x: w0 * qa.x + w1 * qb.x,
            y: w0 * qa.y + w1 * qb.y,
            z: w0 * qa.z + w1 * qb.z,
        };
        if (unit := res.normalize()) {
            return unit;
        }
        return Quat.identity();
    }
}
