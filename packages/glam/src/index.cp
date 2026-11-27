// @cap/glam — Zero-heap 3D linear algebra (Vec3, Quat, column-major Mat4)
// MIT License (clean-room CP implementation, zero external dependencies)

const PI: f64 = 3.14159265358979323846;
const TWO_PI: f64 = 6.28318530717958647692;
const HALF_PI: f64 = 1.57079632679489661923;

fn absF64(x: f64) -> f64 {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

fn sqrtF64(x: f64) -> f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g: f64 = x;
    if (g < 1.0) {
        g = 1.0;
    }
    let i: usize = 0;
    while (i < 48) {
        const next: f64 = 0.5 * (g + x / g);
        if (absF64(next - g) <= 0.0000000000000005 * g) {
            return next;
        }
        g = next;
        i += 1;
    }
    return g;
}

fn sinF64(angle: f64) -> f64 {
    // Reduce angle into [-PI, PI]
    const k: f64 = ((angle + PI) / TWO_PI);
    let ki: i64 = k as i64;
    if (k < 0.0 && (ki as f64) > k) {
        ki -= 1;
    }
    let r: f64 = angle - (ki as f64) * TWO_PI;
    // Fold into [-HALF_PI, HALF_PI]
    if (r > HALF_PI) {
        r = PI - r;
    } else if (r < -HALF_PI) {
        r = -PI - r;
    }
    // Taylor series: r - r^3/3! + r^5/5! - ... (10 terms => full f64 precision)
    const r_sq: f64 = r * r;
    let term: f64 = r;
    let sum: f64 = r;
    let n: usize = 1;
    while (n < 10) {
        const d1: f64 = (2 * n) as f64;
        const d2: f64 = (2 * n + 1) as f64;
        term = -term * r_sq / (d1 * d2);
        sum = sum + term;
        n += 1;
    }
    return sum;
}

fn cosF64(angle: f64) -> f64 {
    return sinF64(angle + HALF_PI);
}

pub struct Vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,

    pub static fn init(x: f64, y: f64, z: f64) -> Vec3 {
        return Vec3 { x: x, y: y, z: z };
    }

    pub fn add(other: Vec3) -> Vec3 {
        return Vec3 { x: _.x + other.x, y: _.y + other.y, z: _.z + other.z };
    }

    pub fn sub(other: Vec3) -> Vec3 {
        return Vec3 { x: _.x - other.x, y: _.y - other.y, z: _.z - other.z };
    }

    pub fn scale(s: f64) -> Vec3 {
        return Vec3 { x: _.x * s, y: _.y * s, z: _.z * s };
    }

    pub fn dot(other: Vec3) -> f64 {
        return _.x * other.x + _.y * other.y + _.z * other.z;
    }

    pub fn cross(other: Vec3) -> Vec3 {
        return Vec3 {
            x: _.y * other.z - _.z * other.y,
            y: _.z * other.x - _.x * other.z,
            z: _.x * other.y - _.y * other.x,
        };
    }

    pub fn lengthSq() -> f64 {
        return _.x * _.x + _.y * _.y + _.z * _.z;
    }

    pub fn length() -> f64 {
        return sqrtF64(_.lengthSq());
    }

    pub fn normalize() -> Vec3 {
        const len: f64 = _.length();
        if (len <= 0.0) {
            return Vec3 { x: 0.0, y: 0.0, z: 0.0 };
        }
        const inv: f64 = 1.0 / len;
        return Vec3 { x: _.x * inv, y: _.y * inv, z: _.z * inv };
    }

    pub fn lerp(other: Vec3, t: f64) -> Vec3 {
        return Vec3 {
            x: _.x + (other.x - _.x) * t,
            y: _.y + (other.y - _.y) * t,
            z: _.z + (other.z - _.z) * t,
        };
    }

    pub fn distance(other: Vec3) -> f64 {
        const dx: f64 = _.x - other.x;
        const dy: f64 = _.y - other.y;
        const dz: f64 = _.z - other.z;
        return sqrtF64(dx * dx + dy * dy + dz * dz);
    }
}

pub struct Quat {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,

    pub static fn identity() -> Quat {
        return Quat { x: 0.0, y: 0.0, z: 0.0, w: 1.0 };
    }

    pub static fn fromAxisAngle(axis: Vec3, angle_rad: f64) -> Quat {
        const n: Vec3 = axis.normalize();
        if (n.lengthSq() <= 0.0) {
            return Quat.identity();
        }
        const half: f64 = angle_rad * 0.5;
        const s: f64 = sinF64(half);
        const c: f64 = cosF64(half);
        return Quat { x: n.x * s, y: n.y * s, z: n.z * s, w: c };
    }

    pub fn normalize() -> Quat {
        const len_sq: f64 = _.x * _.x + _.y * _.y + _.z * _.z + _.w * _.w;
        if (len_sq <= 0.0) {
            return Quat.identity();
        }
        const inv: f64 = 1.0 / sqrtF64(len_sq);
        return Quat { x: _.x * inv, y: _.y * inv, z: _.z * inv, w: _.w * inv };
    }

    pub fn conjugate() -> Quat {
        return Quat { x: -_.x, y: -_.y, z: -_.z, w: _.w };
    }

    pub fn mul(other: Quat) -> Quat {
        return Quat {
            x: _.w * other.x + _.x * other.w + _.y * other.z - _.z * other.y,
            y: _.w * other.y - _.x * other.z + _.y * other.w + _.z * other.x,
            z: _.w * other.z + _.x * other.y - _.y * other.x + _.z * other.w,
            w: _.w * other.w - _.x * other.x - _.y * other.y - _.z * other.z,
        };
    }

    pub fn rotateVec3(v: Vec3) -> Vec3 {
        const qv = Vec3 { x: _.x, y: _.y, z: _.z };
        const t: Vec3 = qv.cross(v).scale(2.0);
        const c: Vec3 = qv.cross(t);
        return Vec3 {
            x: v.x + _.w * t.x + c.x,
            y: v.y + _.w * t.y + c.y,
            z: v.z + _.w * t.z + c.z,
        };
    }
}

pub struct Mat4 {
    pub m: [16]f64,

    pub static fn identity() -> Mat4 {
        let res = Mat4 { m: undefined };
        let i: usize = 0;
        while (i < 16) {
            res.m[i] = 0.0;
            i += 1;
        }
        res.m[0] = 1.0;
        res.m[5] = 1.0;
        res.m[10] = 1.0;
        res.m[15] = 1.0;
        return res;
    }

    pub static fn translation(t: Vec3) -> Mat4 {
        let res: Mat4 = Mat4.identity();
        res.m[12] = t.x;
        res.m[13] = t.y;
        res.m[14] = t.z;
        return res;
    }

    pub static fn scaling(s: Vec3) -> Mat4 {
        let res: Mat4 = Mat4.identity();
        res.m[0] = s.x;
        res.m[5] = s.y;
        res.m[10] = s.z;
        return res;
    }

    pub static fn rotationZ(angle_rad: f64) -> Mat4 {
        let res: Mat4 = Mat4.identity();
        const c: f64 = cosF64(angle_rad);
        const s: f64 = sinF64(angle_rad);
        res.m[0] = c;
        res.m[1] = s;
        res.m[4] = -s;
        res.m[5] = c;
        return res;
    }

    pub fn mul(other: Mat4) -> Mat4 {
        let res = Mat4 { m: undefined };
        let col: usize = 0;
        while (col < 4) {
            let row: usize = 0;
            while (row < 4) {
                let sum: f64 = 0.0;
                let k: usize = 0;
                while (k < 4) {
                    sum = sum + _.m[k * 4 + row] * other.m[col * 4 + k];
                    k += 1;
                }
                res.m[col * 4 + row] = sum;
                row += 1;
            }
            col += 1;
        }
        return res;
    }

    pub fn transformPoint(p: Vec3) -> Vec3 {
        const x: f64 = _.m[0] * p.x + _.m[4] * p.y + _.m[8] * p.z + _.m[12];
        const y: f64 = _.m[1] * p.x + _.m[5] * p.y + _.m[9] * p.z + _.m[13];
        const z: f64 = _.m[2] * p.x + _.m[6] * p.y + _.m[10] * p.z + _.m[14];
        const w: f64 = _.m[3] * p.x + _.m[7] * p.y + _.m[11] * p.z + _.m[15];
        if (w != 0.0 && w != 1.0) {
            const inv_w: f64 = 1.0 / w;
            return Vec3 { x: x * inv_w, y: y * inv_w, z: z * inv_w };
        }
        return Vec3 { x: x, y: y, z: z };
    }

    pub fn determinant() -> f64 {
        // Column-major 4x4 determinant via 2x2 sub-determinants
        const s0: f64 = _.m[0] * _.m[5] - _.m[4] * _.m[1];
        const s1: f64 = _.m[0] * _.m[9] - _.m[8] * _.m[1];
        const s2: f64 = _.m[0] * _.m[13] - _.m[12] * _.m[1];
        const s3: f64 = _.m[4] * _.m[9] - _.m[8] * _.m[5];
        const s4: f64 = _.m[4] * _.m[13] - _.m[12] * _.m[5];
        const s5: f64 = _.m[8] * _.m[13] - _.m[12] * _.m[9];

        const c5: f64 = _.m[10] * _.m[15] - _.m[14] * _.m[11];
        const c4: f64 = _.m[6] * _.m[15] - _.m[14] * _.m[7];
        const c3: f64 = _.m[6] * _.m[11] - _.m[10] * _.m[7];
        const c2: f64 = _.m[2] * _.m[15] - _.m[14] * _.m[3];
        const c1: f64 = _.m[2] * _.m[11] - _.m[10] * _.m[3];
        const c0: f64 = _.m[2] * _.m[7] - _.m[6] * _.m[3];

        return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    }
}
