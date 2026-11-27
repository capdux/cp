// b-spline — Zero-heap de Boor B-Spline / NURBS (1D/2D/3D), Hermite & Catmull-Rom in pure CP
// MIT License — Clean-room implementation with parity to npm `b-spline`.

pub const BSPLINE_MAX_DEGREE: usize = 16;
pub const BSPLINE_MAX_KNOTS: usize = 256;

pub struct Pt2 {
    pub x: f64,
    pub y: f64,
}

pub struct Pt3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
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

/// Generates a clamped (open) uniform knot vector of length `num_points + degree + 1` in `[0.0, 1.0]`.
/// First `degree + 1` knots are `0.0`, last `degree + 1` knots are `1.0`, interior knots are uniform.
/// Returns the number of knots written, or `0` if invalid or `out_knots` is too small.
pub fn generateClampedKnots(num_points: usize, degree: usize, out_knots: []f64): usize {
    if (num_points == 0 || degree == 0 || degree >= num_points) {
        return 0;
    }
    const total = num_points + degree + 1;
    if (out_knots.len < total) {
        return 0;
    }
    const interior_spans = num_points - degree;
    let i: usize = 0;
    while (i < total) {
        if (i <= degree) {
            out_knots[i] = 0.0;
        } else if (i >= num_points) {
            out_knots[i] = 1.0;
        } else {
            out_knots[i] = ((i - degree) as f64) / (interior_spans as f64);
        }
        i += 1;
    }
    return total;
}

/// Generates a uniform (unclamped) knot vector `0.0, 1.0, ..., num_points + degree`.
/// Returns the number of knots written, or `0` if invalid or `out_knots` is too small.
pub fn generateUniformKnots(num_points: usize, degree: usize, out_knots: []f64): usize {
    if (num_points == 0 || degree == 0 || degree >= num_points) {
        return 0;
    }
    const total = num_points + degree + 1;
    if (out_knots.len < total) {
        return 0;
    }
    let i: usize = 0;
    while (i < total) {
        out_knots[i] = i as f64;
        i += 1;
    }
    return total;
}

/// Finds knot span index `s` in `[degree, n - 1]` such that `knots[s] <= u < knots[s + 1]`
/// (or `s = n - 1` when `u >= knots[n]`).
pub fn findKnotSpan(n: usize, degree: usize, u: f64, knots: []const f64): usize {
    if (u >= knots[n]) {
        return n - 1;
    }
    if (u <= knots[degree]) {
        return degree;
    }
    let low: usize = degree;
    let high: usize = n;
    let mid: usize = (low + high) / 2;
    while (u < knots[mid] || u >= knots[mid + 1]) {
        if (u < knots[mid]) {
            high = mid;
        } else {
            low = mid;
        }
        mid = (low + high) / 2;
    }
    return mid;
}

fn bspResolveKnots(
    n: usize,
    degree: usize,
    knots: []const f64,
    auto_buf: []f64,
): []const f64 {
    const expected = n + degree + 1;
    if (knots.len == expected) {
        return knots;
    }
    const written = generateClampedKnots(n, degree, auto_buf);
    return auto_buf[0..written];
}

fn bspClamp01(t: f64): f64 {
    if (t <= 0.0) { return 0.0; }
    if (t >= 1.0) { return 1.0; }
    return t;
}

/// Evaluates a 1D B-Spline / rational NURBS curve using de Boor's algorithm at `t` in `[0, 1]`.
/// - Pass empty `knots` (`&[]`) to auto-generate clamped uniform knots.
/// - Pass empty `weights` (`&[]`) for non-rational B-Spline, or `weights.len == points.len` for NURBS.
/// - Returns `null` on invalid degree, empty points, or mismatched knot/weight lengths.
pub fn bspline1D(
    t: f64,
    degree: usize,
    points: []const f64,
    knots: []const f64,
    weights: []const f64,
): f64? {
    const n = points.len;
    if (n == 0 || degree == 0 || degree >= n || degree > BSPLINE_MAX_DEGREE) {
        return null;
    }
    const expected_knots = n + degree + 1;
    if (knots.len != 0 && knots.len != expected_knots) {
        return null;
    }
    if (knots.len == 0 && expected_knots > BSPLINE_MAX_KNOTS) {
        return null;
    }
    const rational = weights.len != 0;
    if (rational && weights.len != n) {
        return null;
    }

    let auto_knots: [256]f64 = undefined;
    const kv = bspResolveKnots(n, degree, knots, auto_knots[0..256]);
    if (kv.len != expected_knots) {
        return null;
    }

    const ct = bspClamp01(t);
    const low_u = kv[degree];
    const high_u = kv[n];
    const u = low_u + ct * (high_u - low_u);
    const s = findKnotSpan(n, degree, u, kv);

    let vx: [17]f64 = undefined;
    let vw: [17]f64 = undefined;
    let j: usize = 0;
    while (j <= degree) {
        const idx = s - degree + j;
        const w: f64 = if (rational) weights[idx] else 1.0;
        vx[j] = points[idx] * w;
        vw[j] = w;
        j += 1;
    }

    let l: usize = 1;
    while (l <= degree) {
        let i: usize = degree;
        while (i >= l) {
            const left = kv[s - degree + i];
            const right = kv[s + 1 + i - l];
            const denom = right - left;
            const alpha: f64 = if (denom != 0.0) (u - left) / denom else 0.0;
            vx[i] = (1.0 - alpha) * vx[i - 1] + alpha * vx[i];
            vw[i] = (1.0 - alpha) * vw[i - 1] + alpha * vw[i];
            if (i == l) { break; }
            i -= 1;
        }
        l += 1;
    }

    const final_w = vw[degree];
    if (final_w == 0.0) { return null; }
    return vx[degree] / final_w;
}

/// Evaluates a 2D B-Spline / rational NURBS curve using de Boor's algorithm at `t` in `[0, 1]`.
pub fn bspline2D(
    t: f64,
    degree: usize,
    points: []const Pt2,
    knots: []const f64,
    weights: []const f64,
): Pt2? {
    const n = points.len;
    if (n == 0 || degree == 0 || degree >= n || degree > BSPLINE_MAX_DEGREE) {
        return null;
    }
    const expected_knots = n + degree + 1;
    if (knots.len != 0 && knots.len != expected_knots) {
        return null;
    }
    if (knots.len == 0 && expected_knots > BSPLINE_MAX_KNOTS) {
        return null;
    }
    const rational = weights.len != 0;
    if (rational && weights.len != n) {
        return null;
    }

    let auto_knots: [256]f64 = undefined;
    const kv = bspResolveKnots(n, degree, knots, auto_knots[0..256]);
    if (kv.len != expected_knots) {
        return null;
    }

    const ct = bspClamp01(t);
    const low_u = kv[degree];
    const high_u = kv[n];
    const u = low_u + ct * (high_u - low_u);
    const s = findKnotSpan(n, degree, u, kv);

    let vx: [17]f64 = undefined;
    let vy: [17]f64 = undefined;
    let vw: [17]f64 = undefined;
    let j: usize = 0;
    while (j <= degree) {
        const idx = s - degree + j;
        const w: f64 = if (rational) weights[idx] else 1.0;
        vx[j] = points[idx].x * w;
        vy[j] = points[idx].y * w;
        vw[j] = w;
        j += 1;
    }

    let l: usize = 1;
    while (l <= degree) {
        let i: usize = degree;
        while (i >= l) {
            const left = kv[s - degree + i];
            const right = kv[s + 1 + i - l];
            const denom = right - left;
            const alpha: f64 = if (denom != 0.0) (u - left) / denom else 0.0;
            vx[i] = (1.0 - alpha) * vx[i - 1] + alpha * vx[i];
            vy[i] = (1.0 - alpha) * vy[i - 1] + alpha * vy[i];
            vw[i] = (1.0 - alpha) * vw[i - 1] + alpha * vw[i];
            if (i == l) { break; }
            i -= 1;
        }
        l += 1;
    }

    const final_w = vw[degree];
    if (final_w == 0.0) { return null; }
    return Pt2 {
        x: vx[degree] / final_w,
        y: vy[degree] / final_w,
    };
}

/// Evaluates a 3D B-Spline / rational NURBS curve using de Boor's algorithm at `t` in `[0, 1]`.
pub fn bspline3D(
    t: f64,
    degree: usize,
    points: []const Pt3,
    knots: []const f64,
    weights: []const f64,
): Pt3? {
    const n = points.len;
    if (n == 0 || degree == 0 || degree >= n || degree > BSPLINE_MAX_DEGREE) {
        return null;
    }
    const expected_knots = n + degree + 1;
    if (knots.len != 0 && knots.len != expected_knots) {
        return null;
    }
    if (knots.len == 0 && expected_knots > BSPLINE_MAX_KNOTS) {
        return null;
    }
    const rational = weights.len != 0;
    if (rational && weights.len != n) {
        return null;
    }

    let auto_knots: [256]f64 = undefined;
    const kv = bspResolveKnots(n, degree, knots, auto_knots[0..256]);
    if (kv.len != expected_knots) {
        return null;
    }

    const ct = bspClamp01(t);
    const low_u = kv[degree];
    const high_u = kv[n];
    const u = low_u + ct * (high_u - low_u);
    const s = findKnotSpan(n, degree, u, kv);

    let vx: [17]f64 = undefined;
    let vy: [17]f64 = undefined;
    let vz: [17]f64 = undefined;
    let vw: [17]f64 = undefined;
    let j: usize = 0;
    while (j <= degree) {
        const idx = s - degree + j;
        const w: f64 = if (rational) weights[idx] else 1.0;
        vx[j] = points[idx].x * w;
        vy[j] = points[idx].y * w;
        vz[j] = points[idx].z * w;
        vw[j] = w;
        j += 1;
    }

    let l: usize = 1;
    while (l <= degree) {
        let i: usize = degree;
        while (i >= l) {
            const left = kv[s - degree + i];
            const right = kv[s + 1 + i - l];
            const denom = right - left;
            const alpha: f64 = if (denom != 0.0) (u - left) / denom else 0.0;
            vx[i] = (1.0 - alpha) * vx[i - 1] + alpha * vx[i];
            vy[i] = (1.0 - alpha) * vy[i - 1] + alpha * vy[i];
            vz[i] = (1.0 - alpha) * vz[i - 1] + alpha * vz[i];
            vw[i] = (1.0 - alpha) * vw[i - 1] + alpha * vw[i];
            if (i == l) { break; }
            i -= 1;
        }
        l += 1;
    }

    const final_w = vw[degree];
    if (final_w == 0.0) { return null; }
    return Pt3 {
        x: vx[degree] / final_w,
        y: vy[degree] / final_w,
        z: vz[degree] / final_w,
    };
}

/// Convenience clamped non-rational 1D B-Spline evaluation.
pub fn bspline1DUniform(t: f64, degree: usize, points: []const f64): f64? {
    const dummy: [1]f64 = [0.0];
    return bspline1D(t, degree, points, dummy[0..0], dummy[0..0]);
}

/// Convenience clamped non-rational 2D B-Spline evaluation.
pub fn bspline2DUniform(t: f64, degree: usize, points: []const Pt2): Pt2? {
    const dummy: [1]f64 = [0.0];
    return bspline2D(t, degree, points, dummy[0..0], dummy[0..0]);
}

/// Convenience clamped non-rational 3D B-Spline evaluation.
pub fn bspline3DUniform(t: f64, degree: usize, points: []const Pt3): Pt3? {
    const dummy: [1]f64 = [0.0];
    return bspline3D(t, degree, points, dummy[0..0], dummy[0..0]);
}

/// Uniformly samples `out.len` points along a 2D B-Spline/NURBS curve for `t` in `[0, 1]`.
pub fn sampleBSpline2D(
    degree: usize,
    points: []const Pt2,
    knots: []const f64,
    weights: []const f64,
    out: []Pt2,
): usize {
    if (out.len == 0) { return 0; }
    let i: usize = 0;
    while (i < out.len) {
        const t: f64 = if (out.len == 1) 0.0 else (i as f64) / ((out.len - 1) as f64);
        const pt = bspline2D(t, degree, points, knots, weights);
        if (pt == null) { return i; }
        out[i] = pt.?;
        i += 1;
    }
    return out.len;
}

export {
    BSPLINE_MAX_DEGREE,
    BSPLINE_MAX_KNOTS,
    Pt2,
    Pt3,
    hermite1D,
    hermite2D,
    catmullRom1D,
    catmullRom2D,
    uniformCubicBSpline1D,
    uniformCubicBSpline2D,
    evalCatmullRomChain,
    generateClampedKnots,
    generateUniformKnots,
    findKnotSpan,
    bspline1D,
    bspline2D,
    bspline3D,
    bspline1DUniform,
    bspline2DUniform,
    bspline3DUniform,
    sampleBSpline2D,
};
