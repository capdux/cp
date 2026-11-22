// Origin: Pure-CP Stdlib (Zero-Alloc 2D Quadratic & Cubic Bezier Computational Geometry) | Created: 2026-11-22

import std from "std";

const GL5_NODES: [5]f64 = [
    -0.9061798459386640,
    -0.5384693101056831,
    0.0,
    0.5384693101056831,
    0.9061798459386640,
];

const GL5_WEIGHTS: [5]f64 = [
    0.2369268850561891,
    0.4786286704993665,
    0.5688888888888889,
    0.4786286704993665,
    0.2369268850561891,
];

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

/// 2D floating-point vector.
pub struct Vec2 {
    x: f64,
    y: f64,

    fn add(o: Vec2): Vec2 {
        return Vec2 { x: _.x + o.x, y: _.y + o.y };
    }

    fn sub(o: Vec2): Vec2 {
        return Vec2 { x: _.x - o.x, y: _.y - o.y };
    }

    fn scale(s: f64): Vec2 {
        return Vec2 { x: _.x * s, y: _.y * s };
    }

    fn neg(): Vec2 {
        return Vec2 { x: 0.0 - _.x, y: 0.0 - _.y };
    }

    fn dot(o: Vec2): f64 {
        return _.x * o.x + _.y * o.y;
    }

    fn cross(o: Vec2): f64 {
        return _.x * o.y - _.y * o.x;
    }

    fn lenSq(): f64 {
        return _.x * _.x + _.y * _.y;
    }

    fn len(): f64 {
        return std.math.sqrt(_.x * _.x + _.y * _.y);
    }

    /// 90-degree counter-clockwise perpendicular vector `(-y, x)`.
    fn perp(): Vec2 {
        return Vec2 { x: 0.0 - _.y, y: _.x };
    }

    fn dist(o: Vec2): f64 {
        return _.sub(o).len();
    }

    fn lerp(o: Vec2, t: f64): Vec2 {
        return Vec2 {
            x: _.x + (o.x - _.x) * t,
            y: _.y + (o.y - _.y) * t,
        };
    }

    fn normalize(): Vec2 {
        const l = _.len();
        if (l <= 1.0e-15) {
            return Vec2 { x: 0.0, y: 0.0 };
        }
        return Vec2 { x: _.x / l, y: _.y / l };
    }
}

/// Axis-aligned 2D bounding box (AABB).
pub struct BBox2 {
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,

    fn width(): f64 {
        return _.max_x - _.min_x;
    }

    fn height(): f64 {
        return _.max_y - _.min_y;
    }

    fn contains(p: Vec2): bool {
        return p.x >= _.min_x && p.x <= _.max_x && p.y >= _.min_y && p.y <= _.max_y;
    }

    fn expand(*_, p: Vec2) {
        if (p.x < _.min_x) { _.min_x = p.x; }
        if (p.y < _.min_y) { _.min_y = p.y; }
        if (p.x > _.max_x) { _.max_x = p.x; }
        if (p.y > _.max_y) { _.max_y = p.y; }
    }
}

/// Pair of quadratic Bezier sub-curves produced by De Casteljau subdivision.
pub struct QuadPair {
    left: QuadBezier,
    right: QuadBezier,
}

/// Pair of cubic Bezier sub-curves produced by De Casteljau subdivision.
pub struct CubicPair {
    left: CubicBezier,
    right: CubicBezier,
}

/// 2D Quadratic Bezier curve defined by control points `(p0, p1, p2)`.
pub struct QuadBezier {
    p0: Vec2,
    p1: Vec2,
    p2: Vec2,

    /// Evaluates point `B(t)` at parameter `t`.
    fn eval(t: f64): Vec2 {
        const u = 1.0 - t;
        const uu = u * u;
        const two_ut = 2.0 * u * t;
        const tt = t * t;
        return Vec2 {
            x: uu * _.p0.x + two_ut * _.p1.x + tt * _.p2.x,
            y: uu * _.p0.y + two_ut * _.p1.y + tt * _.p2.y,
        };
    }

    /// Evaluates first derivative `B'(t)`.
    fn derivative(t: f64): Vec2 {
        const u = 1.0 - t;
        return Vec2 {
            x: 2.0 * u * (_.p1.x - _.p0.x) + 2.0 * t * (_.p2.x - _.p1.x),
            y: 2.0 * u * (_.p1.y - _.p0.y) + 2.0 * t * (_.p2.y - _.p1.y),
        };
    }

    /// Returns unit tangent vector at parameter `t`.
    fn tangent(t: f64): Vec2 {
        return _.derivative(t).normalize();
    }

    /// Returns unit left-hand normal vector `(-ty, tx)` at parameter `t`.
    fn normal(t: f64): Vec2 {
        const tan = _.tangent(t);
        return Vec2 { x: -tan.y, y: tan.x };
    }

    /// Subdivides the curve at parameter `t` via De Casteljau's algorithm.
    fn split(t: f64): QuadPair {
        const q0 = _.p0.lerp(_.p1, t);
        const q1 = _.p1.lerp(_.p2, t);
        const mid = q0.lerp(q1, t);
        return QuadPair {
            left: QuadBezier { p0: _.p0, p1: q0, p2: mid },
            right: QuadBezier { p0: mid, p1: q1, p2: _.p2 },
        };
    }

    /// Exact degree elevation from quadratic to cubic Bezier.
    fn elevateToCubic(): CubicBezier {
        const two_thirds: f64 = 2.0 / 3.0;
        const one_third: f64 = 1.0 / 3.0;
        return CubicBezier {
            p0: _.p0,
            p1: Vec2 {
                x: one_third * _.p0.x + two_thirds * _.p1.x,
                y: one_third * _.p0.y + two_thirds * _.p1.y,
            },
            p2: Vec2 {
                x: two_thirds * _.p1.x + one_third * _.p2.x,
                y: two_thirds * _.p1.y + one_third * _.p2.y,
            },
            p3: _.p2,
        };
    }

    /// Computes exact tight axis-aligned bounding box analytically from extrema.
    fn bbox(): BBox2 {
        let box = BBox2 {
            min_x: _.p0.x,
            min_y: _.p0.y,
            max_x: _.p0.x,
            max_y: _.p0.y,
        };
        box.expand(_.p2);

        const denom_x = _.p0.x - 2.0 * _.p1.x + _.p2.x;
        if (absF64(denom_x) > 1.0e-12) {
            const tx = (_.p0.x - _.p1.x) / denom_x;
            if (tx > 0.0 && tx < 1.0) {
                box.expand(_.eval(tx));
            }
        }

        const denom_y = _.p0.y - 2.0 * _.p1.y + _.p2.y;
        if (absF64(denom_y) > 1.0e-12) {
            const ty = (_.p0.y - _.p1.y) / denom_y;
            if (ty > 0.0 && ty < 1.0) {
                box.expand(_.eval(ty));
            }
        }

        return box;
    }

    /// Computes arc length over `[0, 1]` via composite 5-point Gauss-Legendre quadrature.
    fn arcLength(): f64 {
        const segs: usize = 4;
        const step: f64 = 1.0 / (segs as f64);
        let total: f64 = 0.0;
        let s: usize = 0;
        while (s < segs) {
            const a = (s as f64) * step;
            const b = a + step;
            const half = 0.5 * (b - a);
            const mid = 0.5 * (a + b);
            let acc: f64 = 0.0;
            let k: usize = 0;
            while (k < 5) {
                const t = mid + half * GL5_NODES[k];
                acc += GL5_WEIGHTS[k] * _.derivative(t).len();
                k += 1;
            }
            total += half * acc;
            s += 1;
        }
        return total;
    }
}

/// 2D Cubic Bezier curve defined by control points `(p0, p1, p2, p3)`.
pub struct CubicBezier {
    p0: Vec2,
    p1: Vec2,
    p2: Vec2,
    p3: Vec2,

    /// Evaluates point `B(t)` at parameter `t`.
    fn eval(t: f64): Vec2 {
        const u = 1.0 - t;
        const uu = u * u;
        const tt = t * t;
        const c0 = uu * u;
        const c1 = 3.0 * uu * t;
        const c2 = 3.0 * u * tt;
        const c3 = tt * t;
        return Vec2 {
            x: c0 * _.p0.x + c1 * _.p1.x + c2 * _.p2.x + c3 * _.p3.x,
            y: c0 * _.p0.y + c1 * _.p1.y + c2 * _.p2.y + c3 * _.p3.y,
        };
    }

    /// Evaluates first derivative `B'(t)`.
    fn derivative(t: f64): Vec2 {
        const u = 1.0 - t;
        const c0 = 3.0 * u * u;
        const c1 = 6.0 * u * t;
        const c2 = 3.0 * t * t;
        return Vec2 {
            x: c0 * (_.p1.x - _.p0.x) + c1 * (_.p2.x - _.p1.x) + c2 * (_.p3.x - _.p2.x),
            y: c0 * (_.p1.y - _.p0.y) + c1 * (_.p2.y - _.p1.y) + c2 * (_.p3.y - _.p2.y),
        };
    }

    /// Evaluates second derivative `B''(t)`.
    fn secondDerivative(t: f64): Vec2 {
        const u = 1.0 - t;
        const ax = _.p2.x - 2.0 * _.p1.x + _.p0.x;
        const ay = _.p2.y - 2.0 * _.p1.y + _.p0.y;
        const bx = _.p3.x - 2.0 * _.p2.x + _.p1.x;
        const by = _.p3.y - 2.0 * _.p2.y + _.p1.y;
        return Vec2 {
            x: 6.0 * u * ax + 6.0 * t * bx,
            y: 6.0 * u * ay + 6.0 * t * by,
        };
    }

    /// Returns unit tangent vector at parameter `t`.
    fn tangent(t: f64): Vec2 {
        return _.derivative(t).normalize();
    }

    /// Returns unit left-hand normal vector `(-ty, tx)` at parameter `t`.
    fn normal(t: f64): Vec2 {
        const tan = _.tangent(t);
        return Vec2 { x: -tan.y, y: tan.x };
    }

    /// Computes signed curvature `kappa(t) = (B'(t) x B''(t)) / |B'(t)|^3`.
    /// Returns `0.0` at degenerate cusp points where `|B'(t)| <= 1.0e-12`.
    fn curvature(t: f64): f64 {
        const d1 = _.derivative(t);
        const speed = d1.len();
        if (speed <= 1.0e-12) {
            return 0.0;
        }
        const d2 = _.secondDerivative(t);
        return d1.cross(d2) / (speed * speed * speed);
    }

    /// Subdivides the cubic curve at parameter `t` via De Casteljau's algorithm.
    fn split(t: f64): CubicPair {
        const q0 = _.p0.lerp(_.p1, t);
        const q1 = _.p1.lerp(_.p2, t);
        const q2 = _.p2.lerp(_.p3, t);
        const r0 = q0.lerp(q1, t);
        const r1 = q1.lerp(q2, t);
        const s = r0.lerp(r1, t);
        return CubicPair {
            left: CubicBezier { p0: _.p0, p1: q0, p2: r0, p3: s },
            right: CubicBezier { p0: s, p1: r1, p2: q2, p3: _.p3 },
        };
    }

    /// Computes the exact tight axis-aligned bounding box by solving `a*t^2 + b*t + c = 0`
    /// for both X and Y derivatives.
    fn bbox(): BBox2 {
        let box = BBox2 {
            min_x: _.p0.x,
            min_y: _.p0.y,
            max_x: _.p0.x,
            max_y: _.p0.y,
        };
        box.expand(_.p3);

        // X derivative roots: B'_x(t) / 3 = ax*t^2 + bx*t + cx = 0
        const ax: f64 = (3.0 * _.p1.x - 3.0 * _.p2.x + _.p3.x) - _.p0.x;
        const bx: f64 = 2.0 * (_.p0.x - 2.0 * _.p1.x + _.p2.x);
        const cx: f64 = _.p1.x - _.p0.x;
        if (absF64(ax) <= 1.0e-12) {
            if (absF64(bx) > 1.0e-12) {
                const tx: f64 = (0.0 - cx) / bx;
                if (tx > 0.0 && tx < 1.0) {
                    box.expand(_.eval(tx));
                }
            }
        } else {
            const disc_x: f64 = bx * bx - 4.0 * ax * cx;
            if (disc_x >= 0.0) {
                const root_x: f64 = std.math.sqrt(disc_x);
                const t1: f64 = ((0.0 - bx) - root_x) / (2.0 * ax);
                const t2: f64 = ((0.0 - bx) + root_x) / (2.0 * ax);
                if (t1 > 0.0 && t1 < 1.0) { box.expand(_.eval(t1)); }
                if (t2 > 0.0 && t2 < 1.0) { box.expand(_.eval(t2)); }
            }
        }

        // Y derivative roots: B'_y(t) / 3 = ay*t^2 + by*t + cy = 0
        const ay: f64 = (3.0 * _.p1.y - 3.0 * _.p2.y + _.p3.y) - _.p0.y;
        const by: f64 = 2.0 * (_.p0.y - 2.0 * _.p1.y + _.p2.y);
        const cy: f64 = _.p1.y - _.p0.y;
        if (absF64(ay) <= 1.0e-12) {
            if (absF64(by) > 1.0e-12) {
                const ty: f64 = (0.0 - cy) / by;
                if (ty > 0.0 && ty < 1.0) {
                    box.expand(_.eval(ty));
                }
            }
        } else {
            const disc_y: f64 = by * by - 4.0 * ay * cy;
            if (disc_y >= 0.0) {
                const root_y: f64 = std.math.sqrt(disc_y);
                const t1: f64 = ((0.0 - by) - root_y) / (2.0 * ay);
                const t2: f64 = ((0.0 - by) + root_y) / (2.0 * ay);
                if (t1 > 0.0 && t1 < 1.0) { box.expand(_.eval(t1)); }
                if (t2 > 0.0 && t2 < 1.0) { box.expand(_.eval(t2)); }
            }
        }

        return box;
    }

    /// Computes arc length over `[0, t_end]` via composite 5-point Gauss-Legendre quadrature.
    fn arcLengthTo(t_end: f64): f64 {
        if (t_end <= 0.0) {
            return 0.0;
        }
        const segs: usize = 4;
        const step = t_end / (segs as f64);
        let total: f64 = 0.0;
        let s: usize = 0;
        while (s < segs) {
            const a = (s as f64) * step;
            const b = a + step;
            const half = 0.5 * (b - a);
            const mid = 0.5 * (a + b);
            let acc: f64 = 0.0;
            let k: usize = 0;
            while (k < 5) {
                const t = mid + half * GL5_NODES[k];
                acc += GL5_WEIGHTS[k] * _.derivative(t).len();
                k += 1;
            }
            total += half * acc;
            s += 1;
        }
        return total;
    }

    /// Computes total arc length over `[0, 1]`.
    fn arcLength(): f64 {
        return _.arcLengthTo(1.0);
    }

    /// Uniformly samples `out.len` points in parameter `t ∈ [0, 1]` into `out` (zero-alloc).
    /// Returns `null` if `out.len == 0`.
    fn sampleUniform(out: []Vec2): usize? {
        if (out.len == 0) {
            return null;
        }
        if (out.len == 1) {
            out[0] = _.eval(0.0);
            return 1;
        }
        const denom = (out.len - 1) as f64;
        let i: usize = 0;
        while (i < out.len) {
            out[i] = _.eval((i as f64) / denom);
            i += 1;
        }
        return out.len;
    }
}

/// Inverts arc length to parameter `t ∈ [0, 1]` via bisection + Gauss-Legendre integration
/// such that `curve.arcLengthTo(t) ≈ target_len`.
pub fn tAtArcLength(curve: CubicBezier, target_len: f64): f64 {
    if (target_len <= 0.0) {
        return 0.0;
    }
    const total = curve.arcLength();
    if (total <= 1.0e-15 || target_len >= total) {
        return 1.0;
    }

    let lo: f64 = 0.0;
    let hi: f64 = 1.0;
    let iter: usize = 0;
    while (iter < 48) {
        const mid = 0.5 * (lo + hi);
        if (curve.arcLengthTo(mid) < target_len) {
            lo = mid;
        } else {
            hi = mid;
        }
        iter += 1;
    }
    return 0.5 * (lo + hi);
}
