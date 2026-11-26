// @cap/spline — Natural Cubic Spline, Catmull-Rom, Bezier & Interpolation
// MIT License (clean-room CP implementation, zero-heap)

pub const MAX_KNOTS: usize = 64;

pub struct Vec2 {
    pub x: f64,
    pub y: f64,
}

pub struct CubicSpline {
    pub xs: [64]f64,
    pub a: [64]f64,
    pub b: [64]f64,
    pub c: [64]f64,
    pub d: [64]f64,
    pub count: usize,
    pub valid: bool,

    pub static fn build(xs: []const f64, ys: []const f64) -> CubicSpline {
        let res = CubicSpline {
            xs: undefined,
            a: undefined,
            b: undefined,
            c: undefined,
            d: undefined,
            count: 0,
            valid: false,
        };

        if (xs.len != ys.len || xs.len < 2 || xs.len > MAX_KNOTS) {
            return res;
        }

        const n: usize = xs.len;
        let i: usize = 0;
        while (i < n) {
            if (i > 0 && xs[i] <= xs[i - 1]) {
                return res;
            }
            res.xs[i] = xs[i];
            res.a[i] = ys[i];
            res.b[i] = 0.0;
            res.c[i] = 0.0;
            res.d[i] = 0.0;
            i += 1;
        }

        res.count = n;
        res.valid = true;

        if (n == 2) {
            const h0: f64 = res.xs[1] - res.xs[0];
            res.b[0] = (res.a[1] - res.a[0]) / h0;
            res.c[0] = 0.0;
            res.d[0] = 0.0;
            return res;
        }

        // Compute segment widths h[i] = xs[i+1] - xs[i]
        let h: [64]f64 = undefined;
        let k: usize = 0;
        while (k < n - 1) {
            h[k] = res.xs[k + 1] - res.xs[k];
            k += 1;
        }

        // Compute right-hand side alpha[1..n-1]
        let alpha: [64]f64 = undefined;
        alpha[0] = 0.0;
        let idx: usize = 1;
        while (idx < n - 1) {
            alpha[idx] = (3.0 / h[idx]) * (res.a[idx + 1] - res.a[idx])
                - (3.0 / h[idx - 1]) * (res.a[idx] - res.a[idx - 1]);
            idx += 1;
        }

        // Thomas algorithm forward sweep (Natural boundary: c[0] = 0, c[n-1] = 0)
        let l: [64]f64 = undefined;
        let mu: [64]f64 = undefined;
        let z: [64]f64 = undefined;

        l[0] = 1.0;
        mu[0] = 0.0;
        z[0] = 0.0;

        idx = 1;
        while (idx < n - 1) {
            l[idx] = 2.0 * (res.xs[idx + 1] - res.xs[idx - 1]) - h[idx - 1] * mu[idx - 1];
            mu[idx] = h[idx] / l[idx];
            z[idx] = (alpha[idx] - h[idx - 1] * z[idx - 1]) / l[idx];
            idx += 1;
        }

        l[n - 1] = 1.0;
        z[n - 1] = 0.0;
        res.c[n - 1] = 0.0;

        // Back substitution
        let j: usize = n - 1;
        while (j > 0) {
            j -= 1;
            res.c[j] = z[j] - mu[j] * res.c[j + 1];
            res.b[j] = (res.a[j + 1] - res.a[j]) / h[j]
                - h[j] * (res.c[j + 1] + 2.0 * res.c[j]) / 3.0;
            res.d[j] = (res.c[j + 1] - res.c[j]) / (3.0 * h[j]);
        }

        return res;
    }

    pub fn isValid() -> bool {
        return _.valid;
    }

    pub fn knotCount() -> usize {
        return _.count;
    }

    fn findSegment(x: f64) -> usize {
        if (x <= _.xs[0]) {
            return 0;
        }
        const last_seg: usize = _.count - 2;
        if (x >= _.xs[_.count - 1]) {
            return last_seg;
        }
        let lo: usize = 0;
        let hi: usize = last_seg;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo + 1) / 2;
            if (_.xs[mid] <= x) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }
        return lo;
    }

    pub fn eval(x: f64) -> f64 {
        if (!_.valid) {
            return 0.0;
        }
        const seg: usize = _.findSegment(x);
        const dx: f64 = x - _.xs[seg];
        return ((_.d[seg] * dx + _.c[seg]) * dx + _.b[seg]) * dx + _.a[seg];
    }

    pub fn derivative(x: f64) -> f64 {
        if (!_.valid) {
            return 0.0;
        }
        const seg: usize = _.findSegment(x);
        const dx: f64 = x - _.xs[seg];
        return (3.0 * _.d[seg] * dx + 2.0 * _.c[seg]) * dx + _.b[seg];
    }
}

pub fn lerp(a: f64, b: f64, t: f64) -> f64 {
    return a + (b - a) * t;
}

pub fn smoothstep(edge0: f64, edge1: f64, x: f64) -> f64 {
    if (edge1 == edge0) {
        if (x < edge0) {
            return 0.0;
        }
        return 1.0;
    }
    let t: f64 = (x - edge0) / (edge1 - edge0);
    if (t < 0.0) {
        t = 0.0;
    }
    if (t > 1.0) {
        t = 1.0;
    }
    return t * t * (3.0 - 2.0 * t);
}

pub fn bezierQuad(p0: Vec2, p1: Vec2, p2: Vec2, t: f64) -> Vec2 {
    const u: f64 = 1.0 - t;
    const u_sq: f64 = u * u;
    const t_sq: f64 = t * t;
    const two_ut: f64 = 2.0 * u * t;
    return Vec2 {
        x: u_sq * p0.x + two_ut * p1.x + t_sq * p2.x,
        y: u_sq * p0.y + two_ut * p1.y + t_sq * p2.y,
    };
}

pub fn bezierCubic(p0: Vec2, p1: Vec2, p2: Vec2, p3: Vec2, t: f64) -> Vec2 {
    const u: f64 = 1.0 - t;
    const u_sq: f64 = u * u;
    const u_cb: f64 = u_sq * u;
    const t_sq: f64 = t * t;
    const t_cb: f64 = t_sq * t;
    const c1: f64 = 3.0 * u_sq * t;
    const c2: f64 = 3.0 * u * t_sq;
    return Vec2 {
        x: u_cb * p0.x + c1 * p1.x + c2 * p2.x + t_cb * p3.x,
        y: u_cb * p0.y + c1 * p1.y + c2 * p2.y + t_cb * p3.y,
    };
}

pub fn catmullRom(p0: f64, p1: f64, p2: f64, p3: f64, t: f64) -> f64 {
    const t2: f64 = t * t;
    const t3: f64 = t2 * t;
    const a: f64 = 2.0 * p1;
    const b: f64 = (-p0 + p2) * t;
    const c: f64 = (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2;
    const d: f64 = (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3;
    return 0.5 * (a + b + c + d);
}
