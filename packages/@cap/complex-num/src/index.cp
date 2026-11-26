// @cap/complex-num — Zero-allocation complex number arithmetic library
// MIT License

pub const PI: f64 = 3.14159265358979323846;
const HALF_PI: f64 = 1.57079632679489661923;
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

pub fn expF64(x: f64): f64 {
    if (x == 0.0) {
        return 1.0;
    }
    let neg_x: bool = x < 0.0;
    let a: f64 = absF64(x);
    let halvings: usize = 0;
    while (a > 0.5 && halvings < 60) {
        a = a * 0.5;
        halvings = halvings + 1;
    }
    let term: f64 = 1.0;
    let sum: f64 = 1.0;
    let k: usize = 1;
    while (k <= 18) {
        term = term * a / (k as f64);
        sum = sum + term;
        k = k + 1;
    }
    let i: usize = 0;
    while (i < halvings) {
        sum = sum * sum;
        i = i + 1;
    }
    if (neg_x) {
        return 1.0 / sum;
    }
    return sum;
}

fn atanSmall(x: f64): f64 {
    let xx: f64 = x * x;
    let term: f64 = x;
    let sum: f64 = x;
    let k: usize = 1;
    while (k <= 12) {
        term = (0.0 - term) * xx;
        sum = sum + term / ((k * 2 + 1) as f64);
        k = k + 1;
    }
    return sum;
}

pub fn atanF64(x: f64): f64 {
    if (x < 0.0) {
        return 0.0 - atanF64(0.0 - x);
    }
    if (x > 1.0) {
        return HALF_PI - atanF64(1.0 / x);
    }
    // Two half-angle reductions bring [0, 1] into [0, ~0.199]
    let r1: f64 = x / (1.0 + sqrtF64(1.0 + x * x));
    let r2: f64 = r1 / (1.0 + sqrtF64(1.0 + r1 * r1));
    return 4.0 * atanSmall(r2);
}

pub fn atan2F64(y: f64, x: f64): f64 {
    if (x > 0.0) {
        return atanF64(y / x);
    }
    if (x < 0.0) {
        if (y >= 0.0) {
            return atanF64(y / x) + PI;
        }
        return atanF64(y / x) - PI;
    }
    if (y > 0.0) {
        return HALF_PI;
    }
    if (y < 0.0) {
        return 0.0 - HALF_PI;
    }
    return 0.0;
}

pub struct Complex {
    pub re: f64,
    pub im: f64,

    pub static fn new(re: f64, im: f64): Complex {
        return Complex { re: re, im: im };
    }

    pub static fn zero(): Complex {
        return Complex { re: 0.0, im: 0.0 };
    }

    pub static fn one(): Complex {
        return Complex { re: 1.0, im: 0.0 };
    }

    pub static fn iUnit(): Complex {
        return Complex { re: 0.0, im: 1.0 };
    }

    pub static fn fromPolar(r: f64, theta: f64): Complex {
        return Complex {
            re: r * cosF64(theta),
            im: r * sinF64(theta),
        };
    }

    pub fn add(other: Complex): Complex {
        return Complex {
            re: _.re + other.re,
            im: _.im + other.im,
        };
    }

    pub fn sub(other: Complex): Complex {
        return Complex {
            re: _.re - other.re,
            im: _.im - other.im,
        };
    }

    pub fn mul(other: Complex): Complex {
        return Complex {
            re: _.re * other.re - _.im * other.im,
            im: _.re * other.im + _.im * other.re,
        };
    }

    pub fn div(other: Complex): Complex {
        let denom: f64 = other.re * other.re + other.im * other.im;
        if (denom <= 0.0) {
            return Complex.zero();
        }
        return Complex {
            re: (_.re * other.re + _.im * other.im) / denom,
            im: (_.im * other.re - _.re * other.im) / denom,
        };
    }

    pub fn conj(): Complex {
        return Complex {
            re: _.re,
            im: 0.0 - _.im,
        };
    }

    pub fn neg(): Complex {
        return Complex {
            re: 0.0 - _.re,
            im: 0.0 - _.im,
        };
    }

    pub fn scale(s: f64): Complex {
        return Complex {
            re: _.re * s,
            im: _.im * s,
        };
    }

    pub fn normSq(): f64 {
        return _.re * _.re + _.im * _.im;
    }

    pub fn norm(): f64 {
        return sqrtF64(_.normSq());
    }

    pub fn arg(): f64 {
        return atan2F64(_.im, _.re);
    }

    pub fn exp(): Complex {
        let er: f64 = expF64(_.re);
        return Complex {
            re: er * cosF64(_.im),
            im: er * sinF64(_.im),
        };
    }

    pub fn sqrt(): Complex {
        let r: f64 = _.norm();
        if (r <= 0.0) {
            return Complex.zero();
        }
        let u: f64 = sqrtF64(0.5 * (r + _.re));
        let v: f64 = sqrtF64(0.5 * (r - _.re));
        if (_.im < 0.0) {
            v = 0.0 - v;
        }
        return Complex { re: u, im: v };
    }

    pub fn approxEq(other: Complex, eps: f64): bool {
        return absF64(_.re - other.re) <= eps && absF64(_.im - other.im) <= eps;
    }
}

pub fn new(re: f64, im: f64): Complex {
    return Complex.new(re, im);
}

pub fn zero(): Complex {
    return Complex.zero();
}

pub fn one(): Complex {
    return Complex.one();
}

pub fn iUnit(): Complex {
    return Complex.iUnit();
}

pub fn fromPolar(r: f64, theta: f64): Complex {
    return Complex.fromPolar(r, theta);
}

pub fn add(a: Complex, b: Complex): Complex {
    return a.add(b);
}

pub fn sub(a: Complex, b: Complex): Complex {
    return a.sub(b);
}

pub fn mul(a: Complex, b: Complex): Complex {
    return a.mul(b);
}

pub fn div(a: Complex, b: Complex): Complex {
    return a.div(b);
}

pub fn conj(z: Complex): Complex {
    return z.conj();
}

pub fn neg(z: Complex): Complex {
    return z.neg();
}

pub fn scale(z: Complex, s: f64): Complex {
    return z.scale(s);
}

pub fn normSq(z: Complex): f64 {
    return z.normSq();
}

pub fn norm(z: Complex): f64 {
    return z.norm();
}

pub fn arg(z: Complex): f64 {
    return z.arg();
}

pub fn exp(z: Complex): Complex {
    return z.exp();
}

pub fn powInt(z: Complex, n: i32): Complex {
    if (n == 0) {
        return Complex.one();
    }
    let nn: i64 = n as i64;
    let base: Complex = z;
    let exp_u: u64 = 0;
    if (nn < 0) {
        base = Complex.one().div(z);
        exp_u = (0 - nn) as u64;
    } else {
        exp_u = nn as u64;
    }
    let acc: Complex = Complex.one();
    while (exp_u > 0) {
        if ((exp_u & 1) == 1) {
            acc = acc.mul(base);
        }
        base = base.mul(base);
        exp_u = exp_u >> 1;
    }
    return acc;
}

pub fn sqrt(z: Complex): Complex {
    return z.sqrt();
}

pub fn approxEq(a: Complex, b: Complex, eps: f64): bool {
    return a.approxEq(b, eps);
}
