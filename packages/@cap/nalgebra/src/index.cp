import {
    Vector2, Vector3, Vector4, RowMatrix2, RowMatrix3, RowMatrix4
} from "../../mint/src/index.cp";

fn naSqrt(x: f64): f64 {
    if (x <= 0.0) { return 0.0; }
    let g: f64 = if (x >= 1.0) { x } else { 1.0 };
    let i: usize = 0;
    while (i < 48) {
        let next_g = 0.5 * (g + x / g);
        let diff = if (next_g > g) { next_g - g } else { g - next_g };
        g = next_g;
        if (diff <= 1.0e-15 * g) { break; }
        i = i + 1;
    }
    return g;
}

pub fn vec2Add(a: Vector2, b: Vector2): Vector2 {
    return Vector2.new(a.x + b.x, a.y + b.y);
}

pub fn vec2Sub(a: Vector2, b: Vector2): Vector2 {
    return Vector2.new(a.x - b.x, a.y - b.y);
}

pub fn vec2Scale(v: Vector2, s: f64): Vector2 {
    return Vector2.new(v.x * s, v.y * s);
}

pub fn vec2Norm(v: Vector2): f64 {
    return naSqrt(v.lenSq());
}

pub fn vec3Add(a: Vector3, b: Vector3): Vector3 {
    return Vector3.new(a.x + b.x, a.y + b.y, a.z + b.z);
}

pub fn vec3Sub(a: Vector3, b: Vector3): Vector3 {
    return Vector3.new(a.x - b.x, a.y - b.y, a.z - b.z);
}

pub fn vec3Scale(v: Vector3, s: f64): Vector3 {
    return Vector3.new(v.x * s, v.y * s, v.z * s);
}

pub fn vec3Norm(v: Vector3): f64 {
    return naSqrt(v.lenSq());
}

pub fn vec3Normalize(v: Vector3): Vector3? {
    let n = vec3Norm(v);
    if (n <= 1.0e-15) { return null; }
    return Vector3.new(v.x / n, v.y / n, v.z / n);
}

pub fn vec3Lerp(a: Vector3, b: Vector3, t: f64): Vector3 {
    return Vector3.new(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

pub fn vec3Distance(a: Vector3, b: Vector3): f64 {
    return vec3Norm(vec3Sub(a, b));
}

pub fn mat2Mul(a: RowMatrix2, b: RowMatrix2): RowMatrix2 {
    let bt = b.transpose();
    return RowMatrix2 {
        x: Vector2.new(a.x.dot(bt.x), a.x.dot(bt.y)),
        y: Vector2.new(a.y.dot(bt.x), a.y.dot(bt.y)),
    };
}

pub fn mat2Inverse(m: RowMatrix2): RowMatrix2? {
    let d = m.det();
    let ad = if (d < 0.0) { -d } else { d };
    if (ad <= 1.0e-15) { return null; }
    let inv = 1.0 / d;
    return RowMatrix2 {
        x: Vector2.new(m.y.y * inv, -m.x.y * inv),
        y: Vector2.new(-m.y.x * inv, m.x.x * inv),
    };
}

pub fn mat3Det(m: RowMatrix3): f64 {
    let c0 = m.y.y * m.z.z - m.y.z * m.z.y;
    let c1 = m.y.x * m.z.z - m.y.z * m.z.x;
    let c2 = m.y.x * m.z.y - m.y.y * m.z.x;
    return m.x.x * c0 - m.x.y * c1 + m.x.z * c2;
}

pub fn mat3Mul(a: RowMatrix3, b: RowMatrix3): RowMatrix3 {
    let bt = b.transpose();
    return RowMatrix3 {
        x: Vector3.new(a.x.dot(bt.x), a.x.dot(bt.y), a.x.dot(bt.z)),
        y: Vector3.new(a.y.dot(bt.x), a.y.dot(bt.y), a.y.dot(bt.z)),
        z: Vector3.new(a.z.dot(bt.x), a.z.dot(bt.y), a.z.dot(bt.z)),
    };
}

pub fn mat3Inverse(m: RowMatrix3): RowMatrix3? {
    let d = mat3Det(m);
    let ad = if (d < 0.0) { -d } else { d };
    if (ad <= 1.0e-15) { return null; }
    let inv = 1.0 / d;
    return RowMatrix3 {
        x: Vector3.new(
            (m.y.y * m.z.z - m.y.z * m.z.y) * inv,
            (m.x.z * m.z.y - m.x.y * m.z.z) * inv,
            (m.x.y * m.y.z - m.x.z * m.y.y) * inv
        ),
        y: Vector3.new(
            (m.y.z * m.z.x - m.y.x * m.z.z) * inv,
            (m.x.x * m.z.z - m.x.z * m.z.x) * inv,
            (m.x.z * m.y.x - m.x.x * m.y.z) * inv
        ),
        z: Vector3.new(
            (m.y.x * m.z.y - m.y.y * m.z.x) * inv,
            (m.x.y * m.z.x - m.x.x * m.z.y) * inv,
            (m.x.x * m.y.y - m.x.y * m.y.x) * inv
        ),
    };
}

pub fn mat4Mul(a: RowMatrix4, b: RowMatrix4): RowMatrix4 {
    let bt = b.transpose();
    return RowMatrix4 {
        x: Vector4.new(a.x.dot(bt.x), a.x.dot(bt.y), a.x.dot(bt.z), a.x.dot(bt.w)),
        y: Vector4.new(a.y.dot(bt.x), a.y.dot(bt.y), a.y.dot(bt.z), a.y.dot(bt.w)),
        z: Vector4.new(a.z.dot(bt.x), a.z.dot(bt.y), a.z.dot(bt.z), a.z.dot(bt.w)),
        w: Vector4.new(a.w.dot(bt.x), a.w.dot(bt.y), a.w.dot(bt.z), a.w.dot(bt.w)),
    };
}

pub fn mat4Translation(tx: f64, ty: f64, tz: f64): RowMatrix4 {
    return RowMatrix4 {
        x: Vector4.new(1.0, 0.0, 0.0, tx),
        y: Vector4.new(0.0, 1.0, 0.0, ty),
        z: Vector4.new(0.0, 0.0, 1.0, tz),
        w: Vector4.new(0.0, 0.0, 0.0, 1.0),
    };
}

pub fn mat4Scaling(sx: f64, sy: f64, sz: f64): RowMatrix4 {
    return RowMatrix4 {
        x: Vector4.new(sx, 0.0, 0.0, 0.0),
        y: Vector4.new(0.0, sy, 0.0, 0.0),
        z: Vector4.new(0.0, 0.0, sz, 0.0),
        w: Vector4.new(0.0, 0.0, 0.0, 1.0),
    };
}

pub fn mat4TransformPoint(m: RowMatrix4, p: Vector3): Vector3 {
    let r = m.mulVec4(Vector4.new(p.x, p.y, p.z, 1.0));
    return Vector3.new(r.x, r.y, r.z);
}
