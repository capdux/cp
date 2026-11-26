// @cap/mat2 — Zero-allocation 2x2 matrix and 2D linear transformation library
// MIT License

pub const PI: f64 = 3.14159265358979323846;
const TWO_PI: f64 = 6.28318530717958647692;
const SINGULAR_EPS: f64 = 0.000000000001;

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return 0.0 - x;
    }
    return x;
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

pub struct V2 {
    pub x: f64,
    pub y: f64,

    pub static fn new(x: f64, y: f64): V2 {
        return V2 { x: x, y: y };
    }

    pub fn approxEq(other: V2, eps: f64): bool {
        return absF64(_.x - other.x) <= eps && absF64(_.y - other.y) <= eps;
    }
}

pub struct Mat2 {
    pub m00: f64,
    pub m01: f64,
    pub m10: f64,
    pub m11: f64,

    pub static fn new(m00: f64, m01: f64, m10: f64, m11: f64): Mat2 {
        return Mat2 { m00: m00, m01: m01, m10: m10, m11: m11 };
    }

    pub static fn identity(): Mat2 {
        return Mat2 { m00: 1.0, m01: 0.0, m10: 0.0, m11: 1.0 };
    }

    pub static fn zero(): Mat2 {
        return Mat2 { m00: 0.0, m01: 0.0, m10: 0.0, m11: 0.0 };
    }

    pub static fn fromScale(sx: f64, sy: f64): Mat2 {
        return Mat2 { m00: sx, m01: 0.0, m10: 0.0, m11: sy };
    }

    pub static fn fromRotation(radians: f64): Mat2 {
        let c: f64 = cosF64(radians);
        let s: f64 = sinF64(radians);
        return Mat2 { m00: c, m01: 0.0 - s, m10: s, m11: c };
    }

    pub static fn fromShear(shx: f64, shy: f64): Mat2 {
        return Mat2 { m00: 1.0, m01: shx, m10: shy, m11: 1.0 };
    }

    pub fn add(other: Mat2): Mat2 {
        return Mat2 {
            m00: _.m00 + other.m00,
            m01: _.m01 + other.m01,
            m10: _.m10 + other.m10,
            m11: _.m11 + other.m11,
        };
    }

    pub fn sub(other: Mat2): Mat2 {
        return Mat2 {
            m00: _.m00 - other.m00,
            m01: _.m01 - other.m01,
            m10: _.m10 - other.m10,
            m11: _.m11 - other.m11,
        };
    }

    pub fn scale(s: f64): Mat2 {
        return Mat2 {
            m00: _.m00 * s,
            m01: _.m01 * s,
            m10: _.m10 * s,
            m11: _.m11 * s,
        };
    }

    pub fn mul(other: Mat2): Mat2 {
        return Mat2 {
            m00: _.m00 * other.m00 + _.m01 * other.m10,
            m01: _.m00 * other.m01 + _.m01 * other.m11,
            m10: _.m10 * other.m00 + _.m11 * other.m10,
            m11: _.m10 * other.m01 + _.m11 * other.m11,
        };
    }

    pub fn mulVec(v: V2): V2 {
        return V2 {
            x: _.m00 * v.x + _.m01 * v.y,
            y: _.m10 * v.x + _.m11 * v.y,
        };
    }

    pub fn transpose(): Mat2 {
        return Mat2 {
            m00: _.m00,
            m01: _.m10,
            m10: _.m01,
            m11: _.m11,
        };
    }

    pub fn trace(): f64 {
        return _.m00 + _.m11;
    }

    pub fn determinant(): f64 {
        return _.m00 * _.m11 - _.m01 * _.m10;
    }

    pub fn isInvertible(): bool {
        return absF64(_.determinant()) > SINGULAR_EPS;
    }

    pub fn inverse(): Mat2 {
        let det: f64 = _.determinant();
        if (absF64(det) <= SINGULAR_EPS) {
            return Mat2.zero();
        }
        let inv_det: f64 = 1.0 / det;
        return Mat2 {
            m00: _.m11 * inv_det,
            m01: (0.0 - _.m01) * inv_det,
            m10: (0.0 - _.m10) * inv_det,
            m11: _.m00 * inv_det,
        };
    }

    pub fn frobeniusNormSq(): f64 {
        return _.m00 * _.m00 + _.m01 * _.m01 + _.m10 * _.m10 + _.m11 * _.m11;
    }

    pub fn approxEq(other: Mat2, eps: f64): bool {
        return absF64(_.m00 - other.m00) <= eps &&
            absF64(_.m01 - other.m01) <= eps &&
            absF64(_.m10 - other.m10) <= eps &&
            absF64(_.m11 - other.m11) <= eps;
    }
}

pub fn identity(): Mat2 {
    return Mat2.identity();
}

pub fn zero(): Mat2 {
    return Mat2.zero();
}

pub fn fromScale(sx: f64, sy: f64): Mat2 {
    return Mat2.fromScale(sx, sy);
}

pub fn fromRotation(radians: f64): Mat2 {
    return Mat2.fromRotation(radians);
}

pub fn fromShear(shx: f64, shy: f64): Mat2 {
    return Mat2.fromShear(shx, shy);
}

pub fn add(a: Mat2, b: Mat2): Mat2 {
    return a.add(b);
}

pub fn sub(a: Mat2, b: Mat2): Mat2 {
    return a.sub(b);
}

pub fn scale(m: Mat2, s: f64): Mat2 {
    return m.scale(s);
}

pub fn mul(a: Mat2, b: Mat2): Mat2 {
    return a.mul(b);
}

pub fn mulVec(m: Mat2, v: V2): V2 {
    return m.mulVec(v);
}

pub fn transpose(m: Mat2): Mat2 {
    return m.transpose();
}

pub fn trace(m: Mat2): f64 {
    return m.trace();
}

pub fn determinant(m: Mat2): f64 {
    return m.determinant();
}

pub fn isInvertible(m: Mat2): bool {
    return m.isInvertible();
}

pub fn inverse(m: Mat2): Mat2 {
    return m.inverse();
}

pub fn frobeniusNormSq(m: Mat2): f64 {
    return m.frobeniusNormSq();
}

pub fn approxEq(a: Mat2, b: Mat2, eps: f64): bool {
    return a.approxEq(b, eps);
}
