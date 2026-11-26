// @cap/bezier-curve — Zero-allocation quadratic & cubic Bezier curve and CSS easing library
// MIT License

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
        let diff: f64 = next - g;
        if (diff < 0.0) {
            diff = 0.0 - diff;
        }
        g = next;
        if (diff <= 0.000000000000001 * g) {
            break;
        }
        i = i + 1;
    }
    return g;
}

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return 0.0 - x;
    }
    return x;
}

fn lerpPt2(a: Pt2, b: Pt2, t: f64): Pt2 {
    return Pt2 {
        x: a.x + (b.x - a.x) * t,
        y: a.y + (b.y - a.y) * t,
    };
}

fn distPt2(a: Pt2, b: Pt2): f64 {
    let dx: f64 = b.x - a.x;
    let dy: f64 = b.y - a.y;
    return sqrtF64(dx * dx + dy * dy);
}

pub struct Pt2 {
    pub x: f64,
    pub y: f64,

    pub static fn new(x: f64, y: f64): Pt2 {
        return Pt2 { x: x, y: y };
    }
}

pub struct QuadBezier {
    pub p0: Pt2,
    pub p1: Pt2,
    pub p2: Pt2,

    pub static fn new(p0: Pt2, p1: Pt2, p2: Pt2): QuadBezier {
        return QuadBezier { p0: p0, p1: p1, p2: p2 };
    }

    pub fn eval(t: f64): Pt2 {
        let u: f64 = 1.0 - t;
        let uu: f64 = u * u;
        let ut2: f64 = 2.0 * u * t;
        let tt: f64 = t * t;
        return Pt2 {
            x: uu * _.p0.x + ut2 * _.p1.x + tt * _.p2.x,
            y: uu * _.p0.y + ut2 * _.p1.y + tt * _.p2.y,
        };
    }

    pub fn deriv(t: f64): Pt2 {
        let u: f64 = 1.0 - t;
        return Pt2 {
            x: 2.0 * u * (_.p1.x - _.p0.x) + 2.0 * t * (_.p2.x - _.p1.x),
            y: 2.0 * u * (_.p1.y - _.p0.y) + 2.0 * t * (_.p2.y - _.p1.y),
        };
    }

    pub fn arcLength(steps: u32): f64 {
        if (steps == 0) {
            return 0.0;
        }
        let n_f: f64 = steps as f64;
        let prev: Pt2 = _.eval(0.0);
        let total: f64 = 0.0;
        let i: u32 = 1;
        while (i <= steps) {
            let t: f64 = (i as f64) / n_f;
            let curr: Pt2 = _.eval(t);
            total = total + distPt2(prev, curr);
            prev = curr;
            i = i + 1;
        }
        return total;
    }
}

pub struct CubicBezier {
    pub p0: Pt2,
    pub p1: Pt2,
    pub p2: Pt2,
    pub p3: Pt2,

    pub static fn new(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2): CubicBezier {
        return CubicBezier { p0: p0, p1: p1, p2: p2, p3: p3 };
    }

    pub fn eval(t: f64): Pt2 {
        let u: f64 = 1.0 - t;
        let uu: f64 = u * u;
        let tt: f64 = t * t;
        let uuu: f64 = uu * u;
        let ttt: f64 = tt * t;
        let w1: f64 = 3.0 * uu * t;
        let w2: f64 = 3.0 * u * tt;
        return Pt2 {
            x: uuu * _.p0.x + w1 * _.p1.x + w2 * _.p2.x + ttt * _.p3.x,
            y: uuu * _.p0.y + w1 * _.p1.y + w2 * _.p2.y + ttt * _.p3.y,
        };
    }

    pub fn deriv(t: f64): Pt2 {
        let u: f64 = 1.0 - t;
        let w0: f64 = 3.0 * u * u;
        let w1: f64 = 6.0 * u * t;
        let w2: f64 = 3.0 * t * t;
        return Pt2 {
            x: w0 * (_.p1.x - _.p0.x) + w1 * (_.p2.x - _.p1.x) + w2 * (_.p3.x - _.p2.x),
            y: w0 * (_.p1.y - _.p0.y) + w1 * (_.p2.y - _.p1.y) + w2 * (_.p3.y - _.p2.y),
        };
    }

    pub fn arcLength(steps: u32): f64 {
        if (steps == 0) {
            return 0.0;
        }
        let n_f: f64 = steps as f64;
        let prev: Pt2 = _.eval(0.0);
        let total: f64 = 0.0;
        let i: u32 = 1;
        while (i <= steps) {
            let t: f64 = (i as f64) / n_f;
            let curr: Pt2 = _.eval(t);
            total = total + distPt2(prev, curr);
            prev = curr;
            i = i + 1;
        }
        return total;
    }
}

pub struct CubicSplit {
    pub left: CubicBezier,
    pub right: CubicBezier,
}

pub fn splitCubic(c: CubicBezier, t: f64): CubicSplit {
    let p01: Pt2 = lerpPt2(c.p0, c.p1, t);
    let p12: Pt2 = lerpPt2(c.p1, c.p2, t);
    let p23: Pt2 = lerpPt2(c.p2, c.p3, t);
    let p012: Pt2 = lerpPt2(p01, p12, t);
    let p123: Pt2 = lerpPt2(p12, p23, t);
    let p0123: Pt2 = lerpPt2(p012, p123, t);
    return CubicSplit {
        left: CubicBezier { p0: c.p0, p1: p01, p2: p012, p3: p0123 },
        right: CubicBezier { p0: p0123, p1: p123, p2: p23, p3: c.p3 },
    };
}

fn evalUnitCubic(p1: f64, p2: f64, t: f64): f64 {
    let c: f64 = 3.0 * p1;
    let b: f64 = 3.0 * (p2 - p1) - c;
    let a: f64 = 1.0 - c - b;
    return ((a * t + b) * t + c) * t;
}

fn derivUnitCubic(p1: f64, p2: f64, t: f64): f64 {
    let c: f64 = 3.0 * p1;
    let b: f64 = 3.0 * (p2 - p1) - c;
    let a: f64 = 1.0 - c - b;
    return (3.0 * a * t + 2.0 * b) * t + c;
}

pub fn cubicBezierEasing(x1: f64, y1: f64, x2: f64, y2: f64, x: f64): f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    if (x >= 1.0) {
        return 1.0;
    }
    if (x1 == y1 && x2 == y2) {
        return x;
    }

    let t: f64 = x;
    let i: usize = 0;
    while (i < 8) {
        let err: f64 = evalUnitCubic(x1, x2, t) - x;
        if (absF64(err) < 0.000000001) {
            return evalUnitCubic(y1, y2, t);
        }
        let d: f64 = derivUnitCubic(x1, x2, t);
        if (absF64(d) < 0.000001) {
            break;
        }
        let next: f64 = t - err / d;
        if (next < 0.0 || next > 1.0) {
            break;
        }
        t = next;
        i = i + 1;
    }

    if (absF64(evalUnitCubic(x1, x2, t) - x) < 0.000000001) {
        return evalUnitCubic(y1, y2, t);
    }

    let lo: f64 = 0.0;
    let hi: f64 = 1.0;
    let mid: f64 = x;
    let k: usize = 0;
    while (k < 64) {
        mid = 0.5 * (lo + hi);
        let cur: f64 = evalUnitCubic(x1, x2, mid);
        if (absF64(cur - x) < 0.000000000001 || (hi - lo) < 0.000000000001) {
            break;
        }
        if (cur < x) {
            lo = mid;
        } else {
            hi = mid;
        }
        k = k + 1;
    }
    return evalUnitCubic(y1, y2, mid);
}
