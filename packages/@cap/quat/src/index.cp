// @cap/quat — Zero-heap 3D unit quaternion rotation, SLERP/NLERP & Euler conversion in pure CP
// MIT License — Clean-room implementation with zero external math dependencies

pub const PI: f64 = 3.14159265358979323846;
pub const TWO_PI: f64 = 6.28318530717958647692;
pub const HALF_PI: f64 = 1.57079632679489661923;
const SIXTH_PI: f64 = 0.52359877559829887308;
const SQRT3: f64 = 1.73205080756887729353;
const TAN_PI_12: f64 = 0.26794919243112270647;
const EPS_ZERO: f64 = 0.000000000001;

pub struct Quat {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}

pub struct Point3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

/// Absolute value of an f64.
pub fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

/// Square root via Newton-Raphson iteration (returns 0.0 for x <= 0.0).
pub fn sqrtF64(x: f64): f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g: f64 = if (x > 1.0) x * 0.5 else 1.0;
    let i: u32 = 0;
    while (i < 48) {
        const next: f64 = 0.5 * (g + x / g);
        if (absF64(next - g) <= 0.000000000000001 * next) {
            return next;
        }
        g = next;
        i += 1;
    }
    return g;
}

/// Reduce angle in radians into [-PI, PI].
fn wrapAngle(x: f64): f64 {
    let r: f64 = x;
    if (r > PI || r < -PI) {
        const turns: i64 = ((r + PI) / TWO_PI) as i64;
        r = r - (turns as f64) * TWO_PI;
        while (r > PI) {
            r = r - TWO_PI;
        }
        while (r < -PI) {
            r = r + TWO_PI;
        }
    }
    return r;
}

/// Sine via range reduction to [-PI/2, PI/2] and 11-term Taylor series.
pub fn sinF64(x: f64): f64 {
    let r: f64 = wrapAngle(x);
    if (r > HALF_PI) {
        r = PI - r;
    } else if (r < -HALF_PI) {
        r = -PI - r;
    }
    const r2: f64 = r * r;
    let term: f64 = r;
    let sum: f64 = r;
    let k: u32 = 1;
    while (k <= 10) {
        const d1: f64 = (2 * k) as f64;
        const d2: f64 = (2 * k + 1) as f64;
        term = -term * r2 / (d1 * d2);
        sum = sum + term;
        k += 1;
    }
    return sum;
}

/// Cosine via sin(x + PI/2).
pub fn cosF64(x: f64): f64 {
    return sinF64(x + HALF_PI);
}

/// Arctangent via argument reduction into [-tan(PI/12), tan(PI/12)] and Taylor series.
pub fn atanF64(x: f64): f64 {
    if (x < 0.0) {
        return -atanF64(-x);
    }
    if (x > 1.0) {
        return HALF_PI - atanF64(1.0 / x);
    }
    let z: f64 = x;
    let offset: f64 = 0.0;
    if (z > TAN_PI_12) {
        z = (z * SQRT3 - 1.0) / (SQRT3 + z);
        offset = SIXTH_PI;
    }
    const z2: f64 = z * z;
    let pwr: f64 = z;
    let sum: f64 = z;
    let k: u32 = 1;
    while (k <= 12) {
        pwr = -pwr * z2;
        sum = sum + pwr / ((2 * k + 1) as f64);
        k += 1;
    }
    return offset + sum;
}

/// Arccosine on [-1.0, 1.0] via 2 * atan(sqrt(1 - x) / sqrt(1 + x)).
pub fn acosF64(x: f64): f64 {
    if (x >= 1.0) {
        return 0.0;
    }
    if (x <= -1.0) {
        return PI;
    }
    const num: f64 = sqrtF64(1.0 - x);
    const den: f64 = sqrtF64(1.0 + x);
    return 2.0 * atanF64(num / den);
}

/// Construct a quaternion from (x, y, z, w) components.
pub fn new(x: f64, y: f64, z: f64, w: f64): Quat {
    return Quat { x: x, y: y, z: z, w: w };
}

/// Construct the identity quaternion (0, 0, 0, 1).
pub fn identity(): Quat {
    return Quat { x: 0.0, y: 0.0, z: 0.0, w: 1.0 };
}

/// Construct a unit quaternion from an axis (ax, ay, az) and angle in radians.
/// Returns identity() if the axis has zero length.
pub fn fromAxisAngle(ax: f64, ay: f64, az: f64, radians: f64): Quat {
    const axis_len: f64 = sqrtF64(ax * ax + ay * ay + az * az);
    if (axis_len <= EPS_ZERO) {
        return identity();
    }
    const half: f64 = radians * 0.5;
    const s: f64 = sinF64(half) / axis_len;
    const c: f64 = cosF64(half);
    return Quat {
        x: ax * s,
        y: ay * s,
        z: az * s,
        w: c,
    };
}

/// Construct a unit quaternion from Euler angles (rx, ry, rz) in XYZ order (radians).
pub fn fromEulerXYZ(rx: f64, ry: f64, rz: f64): Quat {
    const sx: f64 = sinF64(rx * 0.5);
    const cx: f64 = cosF64(rx * 0.5);
    const sy: f64 = sinF64(ry * 0.5);
    const cy: f64 = cosF64(ry * 0.5);
    const sz: f64 = sinF64(rz * 0.5);
    const cz: f64 = cosF64(rz * 0.5);
    return Quat {
        x: sx * cy * cz + cx * sy * sz,
        y: cx * sy * cz - sx * cy * sz,
        z: cx * cy * sz + sx * sy * cz,
        w: cx * cy * cz - sx * sy * sz,
    };
}

/// Hamilton quaternion product a * b.
pub fn mul(a: Quat, b: Quat): Quat {
    return Quat {
        x: a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        y: a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        z: a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        w: a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

/// Quaternion conjugate (-x, -y, -z, w).
pub fn conjugate(q: Quat): Quat {
    return Quat { x: -q.x, y: -q.y, z: -q.z, w: q.w };
}

/// Dot product of two quaternions.
pub fn dot(a: Quat, b: Quat): f64 {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/// Euclidean norm (length) of a quaternion.
pub fn length(q: Quat): f64 {
    return sqrtF64(dot(q, q));
}

/// Multiplicative inverse of a quaternion; returns identity() for zero-length input.
pub fn inverse(q: Quat): Quat {
    const len_sq: f64 = dot(q, q);
    if (len_sq <= EPS_ZERO) {
        return identity();
    }
    const inv: f64 = 1.0 / len_sq;
    return Quat {
        x: -q.x * inv,
        y: -q.y * inv,
        z: -q.z * inv,
        w: q.w * inv,
    };
}

/// Normalize a quaternion to unit length; returns identity() for zero-length input.
pub fn normalize(q: Quat): Quat {
    const len: f64 = length(q);
    if (len <= EPS_ZERO) {
        return identity();
    }
    const inv: f64 = 1.0 / len;
    return Quat {
        x: q.x * inv,
        y: q.y * inv,
        z: q.z * inv,
        w: q.w * inv,
    };
}

/// Normalized linear interpolation (NLERP) along the shortest arc.
pub fn nlerp(a: Quat, b: Quat, t: f64): Quat {
    const d: f64 = dot(a, b);
    const sign: f64 = if (d < 0.0) -1.0 else 1.0;
    const omt: f64 = 1.0 - t;
    const blended = Quat {
        x: a.x * omt + (b.x * sign) * t,
        y: a.y * omt + (b.y * sign) * t,
        z: a.z * omt + (b.z * sign) * t,
        w: a.w * omt + (b.w * sign) * t,
    };
    return normalize(blended);
}

/// Spherical linear interpolation (SLERP) along the shortest arc.
pub fn slerp(a: Quat, b: Quat, t: f64): Quat {
    const qa: Quat = normalize(a);
    let qb: Quat = normalize(b);
    let d: f64 = dot(qa, qb);
    if (d < 0.0) {
        qb = Quat { x: -qb.x, y: -qb.y, z: -qb.z, w: -qb.w };
        d = -d;
    }
    if (d > 0.999995) {
        return nlerp(qa, qb, t);
    }
    const theta: f64 = acosF64(d);
    const sin_theta: f64 = sinF64(theta);
    if (absF64(sin_theta) <= EPS_ZERO) {
        return nlerp(qa, qb, t);
    }
    const w0: f64 = sinF64((1.0 - t) * theta) / sin_theta;
    const w1: f64 = sinF64(t * theta) / sin_theta;
    return Quat {
        x: w0 * qa.x + w1 * qb.x,
        y: w0 * qa.y + w1 * qb.y,
        z: w0 * qa.z + w1 * qb.z,
        w: w0 * qa.w + w1 * qb.w,
    };
}

/// Rotate a 3D point by quaternion q (automatically normalizes q).
pub fn rotatePoint(q: Quat, p: Point3): Point3 {
    const uq: Quat = normalize(q);
    // t = 2 * cross(uq.xyz, p)
    const tx: f64 = 2.0 * (uq.y * p.z - uq.z * p.y);
    const ty: f64 = 2.0 * (uq.z * p.x - uq.x * p.z);
    const tz: f64 = 2.0 * (uq.x * p.y - uq.y * p.x);
    // p' = p + uq.w * t + cross(uq.xyz, t)
    return Point3 {
        x: p.x + uq.w * tx + (uq.y * tz - uq.z * ty),
        y: p.y + uq.w * ty + (uq.z * tx - uq.x * tz),
        z: p.z + uq.w * tz + (uq.x * ty - uq.y * tx),
    };
}

/// Component-wise approximate equality within tolerance `eps`.
pub fn approxEq(a: Quat, b: Quat, eps: f64): bool {
    return absF64(a.x - b.x) <= eps &&
        absF64(a.y - b.y) <= eps &&
        absF64(a.z - b.z) <= eps &&
        absF64(a.w - b.w) <= eps;
}
