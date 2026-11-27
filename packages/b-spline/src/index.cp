// @cap/bspline — Zero-heap cubic Hermite, Catmull-Rom & uniform cubic B-Spline in pure CP
// MIT License — Clean-room implementation

pub struct Pt2 {
    pub x: f64,
    pub y: f64,
}

/// 1D cubic Hermite spline evaluation at parameter `t` in [0, 1].
/// Satisfies f(0) = p0, f(1) = p1, f'(0) = m0, f'(1) = m1.
pub fn hermite1D(p0: f64, m0: f64, p1: f64, m1: f64, t: f64): f64 {
    const t2: f64 = t * t;
    const t3: f64 = t2 * t;
    const h00: f64 = 2.0 * t3 - 3.0 * t2 + 1.0;
    const h10: f64 = t3 - 2.0 * t2 + t;
    const h01: f64 = -2.0 * t3 + 3.0 * t2;
    const h11: f64 = t3 - t2;
    return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
}

/// 2D cubic Hermite spline evaluation at parameter `t` in [0, 1].
pub fn hermite2D(p0: Pt2, m0: Pt2, p1: Pt2, m1: Pt2, t: f64): Pt2 {
    return Pt2 {
        x: hermite1D(p0.x, m0.x, p1.x, m1.x, t),
        y: hermite1D(p0.y, m0.y, p1.y, m1.y, t),
    };
}

/// 1D uniform Catmull-Rom spline passing through `p1` (t=0) and `p2` (t=1).
pub fn catmullRom1D(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64 {
    const t2: f64 = t * t;
    const t3: f64 = t2 * t;
    const c0: f64 = 2.0 * p1;
    const c1: f64 = -p0 + p2;
    const c2: f64 = 2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3;
    const c3: f64 = -p0 + 3.0 * p1 - 3.0 * p2 + p3;
    return 0.5 * (c0 + c1 * t + c2 * t2 + c3 * t3);
}

/// 2D uniform Catmull-Rom spline passing through `p1` (t=0) and `p2` (t=1).
pub fn catmullRom2D(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2, t: f64): Pt2 {
    return Pt2 {
        x: catmullRom1D(p0.x, p1.x, p2.x, p3.x, t),
        y: catmullRom1D(p0.y, p1.y, p2.y, p3.y, t),
    };
}

/// 1D uniform cubic B-Spline evaluation at parameter `t` in [0, 1].
/// Basis weights are non-negative and sum to 1 (convex hull & affine invariance).
pub fn uniformCubicBSpline1D(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64 {
    const u: f64 = 1.0 - t;
    const t2: f64 = t * t;
    const t3: f64 = t2 * t;
    const b0: f64 = (u * u * u) / 6.0;
    const b1: f64 = (3.0 * t3 - 6.0 * t2 + 4.0) / 6.0;
    const b2: f64 = (-3.0 * t3 + 3.0 * t2 + 3.0 * t + 1.0) / 6.0;
    const b3: f64 = t3 / 6.0;
    return b0 * p0 + b1 * p1 + b2 * p2 + b3 * p3;
}

/// 2D uniform cubic B-Spline evaluation at parameter `t` in [0, 1].
pub fn uniformCubicBSpline2D(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2, t: f64): Pt2 {
    return Pt2 {
        x: uniformCubicBSpline1D(p0.x, p1.x, p2.x, p3.x, t),
        y: uniformCubicBSpline1D(p0.y, p1.y, p2.y, p3.y, t),
    };
}

/// Evaluate a multi-segment 2D Catmull-Rom chain over `pts` for global parameter `u` in [0, 1].
/// Clamps `u` to [0, 1] and duplicates endpoints at boundaries so u=0 -> pts[0] and u=1 -> pts[len-1].
/// Handles degenerate slices (len 0, 1, 2, 3) safely without panicking.
pub fn evalCatmullRomChain(pts: []const Pt2, u: f64): Pt2 {
    if (pts.len == 0) {
        return Pt2 { x: 0.0, y: 0.0 };
    }
    if (pts.len == 1) {
        return pts[0];
    }
    let cu: f64 = u;
    if (cu <= 0.0) {
        return pts[0];
    }
    if (cu >= 1.0) {
        return pts[pts.len - 1];
    }
    const seg_count: usize = pts.len - 1;
    const scaled: f64 = cu * (seg_count as f64);
    let seg: usize = scaled as usize;
    if (seg >= seg_count) {
        seg = seg_count - 1;
    }
    const local_t: f64 = scaled - (seg as f64);
    const p0: Pt2 = if (seg == 0) pts[0] else pts[seg - 1];
    const p1: Pt2 = pts[seg];
    const p2: Pt2 = pts[seg + 1];
    const p3: Pt2 = if (seg + 2 < pts.len) pts[seg + 2] else pts[pts.len - 1];
    return catmullRom2D(p0, p1, p2, p3, local_t);
}
