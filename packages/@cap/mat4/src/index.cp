// @cap/mat4 — Zero-allocation 4x4 3D transformation matrix library (row-major)
// MIT License
//
// Row-major layout (m[row * 4 + col]):
//   [ m[0]   m[1]   m[2]   m[3]  ]
//   [ m[4]   m[5]   m[6]   m[7]  ]
//   [ m[8]   m[9]   m[10]  m[11] ]
//   [ m[12]  m[13]  m[14]  m[15] ]

pub const PI: f64 = 3.14159265358979323846;
const TWO_PI: f64 = 6.28318530717958647692;

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return 0.0 - x;
    }
    return x;
}

pub fn sqrtF64(x: f64): f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g: f64 = x;
    if (g < 1.0) {
        g = 1.0;
    }
    let i: usize = 0;
    while (i < 64) {
        let next: f64 = 0.5 * (g + x / g);
        if (absF64(next - g) <= 0.000000000000001 * next) {
            return next;
        }
        g = next;
        i = i + 1;
    }
    return g;
}

fn reduceAngle(rad: f64): f64 {
    let x: f64 = rad;
    while (x > PI) {
        x = x - TWO_PI;
    }
    while (x < 0.0 - PI) {
        x = x + TWO_PI;
    }
    return x;
}

pub fn sinF64(rad: f64): f64 {
    let x: f64 = reduceAngle(rad);
    let xx: f64 = x * x;
    let term: f64 = x;
    let sum: f64 = x;
    let k: usize = 1;
    while (k <= 12) {
        let d1: f64 = (k * 2) as f64;
        let d2: f64 = (k * 2 + 1) as f64;
        term = (0.0 - term) * xx / (d1 * d2);
        sum = sum + term;
        k = k + 1;
    }
    return sum;
}

pub fn cosF64(rad: f64): f64 {
    let x: f64 = reduceAngle(rad);
    let xx: f64 = x * x;
    let term: f64 = 1.0;
    let sum: f64 = 1.0;
    let k: usize = 1;
    while (k <= 12) {
        let d1: f64 = (k * 2 - 1) as f64;
        let d2: f64 = (k * 2) as f64;
        term = (0.0 - term) * xx / (d1 * d2);
        sum = sum + term;
        k = k + 1;
    }
    return sum;
}

pub fn tanF64(rad: f64): f64 {
    let c: f64 = cosF64(rad);
    if (absF64(c) < 0.000000000001) {
        return 0.0;
    }
    return sinF64(rad) / c;
}

pub struct Vec3D {
    pub x: f64,
    pub y: f64,
    pub z: f64,

    pub static fn new(x: f64, y: f64, z: f64): Vec3D {
        return Vec3D { x: x, y: y, z: z };
    }

    pub fn approxEq(other: Vec3D, eps: f64): bool {
        return absF64(_.x - other.x) <= eps &&
            absF64(_.y - other.y) <= eps &&
            absF64(_.z - other.z) <= eps;
    }
}

fn v3Sub(a: Vec3D, b: Vec3D): Vec3D {
    return Vec3D { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
}

fn v3Dot(a: Vec3D, b: Vec3D): f64 {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

fn v3Cross(a: Vec3D, b: Vec3D): Vec3D {
    return Vec3D {
        x: a.y * b.z - a.z * b.y,
        y: a.z * b.x - a.x * b.z,
        z: a.x * b.y - a.y * b.x,
    };
}

fn v3Normalize(v: Vec3D): Vec3D {
    let len: f64 = sqrtF64(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len <= 0.000000000001) {
        return Vec3D { x: 0.0, y: 0.0, z: 0.0 };
    }
    let inv: f64 = 1.0 / len;
    return Vec3D { x: v.x * inv, y: v.y * inv, z: v.z * inv };
}

pub struct Mat4 {
    pub m: [16]f64,

    pub static fn zero(): Mat4 {
        return Mat4 {
            m: [
                0.0, 0.0, 0.0, 0.0,
                0.0, 0.0, 0.0, 0.0,
                0.0, 0.0, 0.0, 0.0,
                0.0, 0.0, 0.0, 0.0,
            ],
        };
    }

    pub static fn identity(): Mat4 {
        return Mat4 {
            m: [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
            ],
        };
    }

    pub static fn fromTranslation(tx: f64, ty: f64, tz: f64): Mat4 {
        return Mat4 {
            m: [
                1.0, 0.0, 0.0, tx,
                0.0, 1.0, 0.0, ty,
                0.0, 0.0, 1.0, tz,
                0.0, 0.0, 0.0, 1.0,
            ],
        };
    }

    pub static fn fromScale(sx: f64, sy: f64, sz: f64): Mat4 {
        return Mat4 {
            m: [
                sx,  0.0, 0.0, 0.0,
                0.0, sy,  0.0, 0.0,
                0.0, 0.0, sz,  0.0,
                0.0, 0.0, 0.0, 1.0,
            ],
        };
    }

    pub static fn fromRotationX(rad: f64): Mat4 {
        let c: f64 = cosF64(rad);
        let s: f64 = sinF64(rad);
        return Mat4 {
            m: [
                1.0, 0.0,     0.0, 0.0,
                0.0, c,       0.0 - s, 0.0,
                0.0, s,       c,   0.0,
                0.0, 0.0,     0.0, 1.0,
            ],
        };
    }

    pub static fn fromRotationY(rad: f64): Mat4 {
        let c: f64 = cosF64(rad);
        let s: f64 = sinF64(rad);
        return Mat4 {
            m: [
                c,       0.0, s,   0.0,
                0.0,     1.0, 0.0, 0.0,
                0.0 - s, 0.0, c,   0.0,
                0.0,     0.0, 0.0, 1.0,
            ],
        };
    }

    pub static fn fromRotationZ(rad: f64): Mat4 {
        let c: f64 = cosF64(rad);
        let s: f64 = sinF64(rad);
        return Mat4 {
            m: [
                c,   0.0 - s, 0.0, 0.0,
                s,   c,       0.0, 0.0,
                0.0, 0.0,     1.0, 0.0,
                0.0, 0.0,     0.0, 1.0,
            ],
        };
    }

    pub static fn orthographic(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64): Mat4 {
        let rl: f64 = right - left;
        let tb: f64 = top - bottom;
        let fn_d: f64 = far - near;
        if (absF64(rl) < 0.000000000001 || absF64(tb) < 0.000000000001 || absF64(fn_d) < 0.000000000001) {
            return Mat4.zero();
        }
        return Mat4 {
            m: [
                2.0 / rl, 0.0,      0.0,          (0.0 - (right + left)) / rl,
                0.0,      2.0 / tb, 0.0,          (0.0 - (top + bottom)) / tb,
                0.0,      0.0,      -2.0 / fn_d,  (0.0 - (far + near)) / fn_d,
                0.0,      0.0,      0.0,          1.0,
            ],
        };
    }

    pub static fn perspective(fov_y_rad: f64, aspect: f64, near: f64, far: f64): Mat4 {
        let t: f64 = tanF64(fov_y_rad * 0.5);
        let nf: f64 = near - far;
        if (absF64(t) < 0.000000000001 || absF64(aspect) < 0.000000000001 || absF64(nf) < 0.000000000001) {
            return Mat4.zero();
        }
        let f: f64 = 1.0 / t;
        return Mat4 {
            m: [
                f / aspect, 0.0, 0.0,                 0.0,
                0.0,        f,   0.0,                 0.0,
                0.0,        0.0, (far + near) / nf,   (2.0 * far * near) / nf,
                0.0,        0.0, -1.0,                0.0,
            ],
        };
    }

    pub static fn lookAt(eye: Vec3D, target: Vec3D, up: Vec3D): Mat4 {
        let fwd: Vec3D = v3Normalize(v3Sub(target, eye));
        let side: Vec3D = v3Normalize(v3Cross(fwd, up));
        let cam_up: Vec3D = v3Cross(side, fwd);
        return Mat4 {
            m: [
                side.x,         side.y,         side.z,         0.0 - v3Dot(side, eye),
                cam_up.x,       cam_up.y,       cam_up.z,       0.0 - v3Dot(cam_up, eye),
                0.0 - fwd.x,    0.0 - fwd.y,    0.0 - fwd.z,    v3Dot(fwd, eye),
                0.0,            0.0,            0.0,            1.0,
            ],
        };
    }
}

pub fn identity(): Mat4 {
    return Mat4.identity();
}

pub fn zero(): Mat4 {
    return Mat4.zero();
}

pub fn fromTranslation(tx: f64, ty: f64, tz: f64): Mat4 {
    return Mat4.fromTranslation(tx, ty, tz);
}

pub fn fromScale(sx: f64, sy: f64, sz: f64): Mat4 {
    return Mat4.fromScale(sx, sy, sz);
}

pub fn fromRotationX(rad: f64): Mat4 {
    return Mat4.fromRotationX(rad);
}

pub fn fromRotationY(rad: f64): Mat4 {
    return Mat4.fromRotationY(rad);
}

pub fn fromRotationZ(rad: f64): Mat4 {
    return Mat4.fromRotationZ(rad);
}

pub fn orthographic(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64): Mat4 {
    return Mat4.orthographic(left, right, bottom, top, near, far);
}

pub fn perspective(fov_y_rad: f64, aspect: f64, near: f64, far: f64): Mat4 {
    return Mat4.perspective(fov_y_rad, aspect, near, far);
}

pub fn lookAt(eye: Vec3D, target: Vec3D, up: Vec3D): Mat4 {
    return Mat4.lookAt(eye, target, up);
}

pub fn mul(a: Mat4, b: Mat4): Mat4 {
    let out: Mat4 = Mat4.zero();
    let r: usize = 0;
    while (r < 4) {
        let c: usize = 0;
        while (c < 4) {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < 4) {
                sum = sum + a.m[r * 4 + k] * b.m[k * 4 + c];
                k = k + 1;
            }
            out.m[r * 4 + c] = sum;
            c = c + 1;
        }
        r = r + 1;
    }
    return out;
}

pub fn transpose(a: Mat4): Mat4 {
    let out: Mat4 = Mat4.zero();
    let r: usize = 0;
    while (r < 4) {
        let c: usize = 0;
        while (c < 4) {
            out.m[r * 4 + c] = a.m[c * 4 + r];
            c = c + 1;
        }
        r = r + 1;
    }
    return out;
}

pub fn transformPoint(mat: Mat4, p: Vec3D): Vec3D {
    let rx: f64 = mat.m[0] * p.x + mat.m[1] * p.y + mat.m[2] * p.z + mat.m[3];
    let ry: f64 = mat.m[4] * p.x + mat.m[5] * p.y + mat.m[6] * p.z + mat.m[7];
    let rz: f64 = mat.m[8] * p.x + mat.m[9] * p.y + mat.m[10] * p.z + mat.m[11];
    let rw: f64 = mat.m[12] * p.x + mat.m[13] * p.y + mat.m[14] * p.z + mat.m[15];
    if (absF64(rw) > 0.000000000001 && rw != 1.0) {
        let inv_w: f64 = 1.0 / rw;
        return Vec3D { x: rx * inv_w, y: ry * inv_w, z: rz * inv_w };
    }
    return Vec3D { x: rx, y: ry, z: rz };
}

pub fn transformDir(mat: Mat4, v: Vec3D): Vec3D {
    return Vec3D {
        x: mat.m[0] * v.x + mat.m[1] * v.y + mat.m[2] * v.z,
        y: mat.m[4] * v.x + mat.m[5] * v.y + mat.m[6] * v.z,
        z: mat.m[8] * v.x + mat.m[9] * v.y + mat.m[10] * v.z,
    };
}

pub fn approxEq(a: Mat4, b: Mat4, eps: f64): bool {
    let i: usize = 0;
    while (i < 16) {
        if (absF64(a.m[i] - b.m[i]) > eps) {
            return false;
        }
        i = i + 1;
    }
    return true;
}
