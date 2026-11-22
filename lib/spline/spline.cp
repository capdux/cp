// Origin: Pure-CP Stdlib (Zero-Alloc Natural/Clamped Cubic Spline, Fritsch-Carlson PCHIP & Catmull-Rom Interpolation) | Created: 2026-11-22

/// Single piecewise cubic polynomial segment on `[x0, x1]`:
/// `S(x) = a + b*(x - x0) + c*(x - x0)^2 + d*(x - x0)^3`.
pub struct CubicSegment {
    x0: f64,
    x1: f64,
    a: f64,
    b: f64,
    c: f64,
    d: f64,

    /// Evaluates `S(x)` via Horner's method.
    fn eval(x: f64): f64 {
        const dx: f64 = x - _.x0;
        return _.a + dx * (_.b + dx * (_.c + dx * _.d));
    }

    /// Evaluates first derivative `S'(x)`.
    fn derivative(x: f64): f64 {
        const dx: f64 = x - _.x0;
        return _.b + dx * (2.0 * _.c + dx * (3.0 * _.d));
    }

    /// Evaluates second derivative `S''(x)`.
    fn secondDerivative(x: f64): f64 {
        const dx: f64 = x - _.x0;
        return 2.0 * _.c + 6.0 * _.d * dx;
    }

    /// Exact analytic definite integral of `S(x)` over `[x0, x1]`.
    fn integrate(): f64 {
        const h: f64 = _.x1 - _.x0;
        return h * (_.a + h * (0.5 * _.b + h * ((_.c / 3.0) + h * (0.25 * _.d))));
    }
}

/// Piecewise cubic spline backed by a caller-provided `[]CubicSegment` slice (zero heap allocation).
pub struct CubicSpline {
    segments: []CubicSegment,

    fn findSegment(x: f64): usize {
        const n: usize = _.segments.len;
        if (n <= 1 || x <= _.segments[0].x0) {
            return 0;
        }
        if (x >= _.segments[n - 1].x0) {
            return n - 1;
        }
        let lo: usize = 0;
        let hi: usize = n - 1;
        while (lo + 1 < hi) {
            const mid: usize = lo + ((hi - lo) / 2);
            if (x < _.segments[mid].x0) {
                hi = mid;
            } else {
                lo = mid;
            }
        }
        return lo;
    }

    /// Evaluates the spline at `x` using O(log n) binary search (extrapolates via end segments outside domain).
    fn eval(x: f64): f64 {
        if (_.segments.len == 0) {
            return 0.0;
        }
        const idx: usize = _.findSegment(x);
        return _.segments[idx].eval(x);
    }

    /// Evaluates the first derivative `S'(x)` at `x`.
    fn derivative(x: f64): f64 {
        if (_.segments.len == 0) {
            return 0.0;
        }
        const idx: usize = _.findSegment(x);
        return _.segments[idx].derivative(x);
    }

    /// Evaluates the second derivative `S''(x)` at `x`.
    fn secondDerivative(x: f64): f64 {
        if (_.segments.len == 0) {
            return 0.0;
        }
        const idx: usize = _.findSegment(x);
        return _.segments[idx].secondDerivative(x);
    }

    /// Exact analytic definite integral over the full spline domain `[x_0, x_{n-1}]`.
    fn integrate(): f64 {
        let total: f64 = 0.0;
        let i: usize = 0;
        while (i < _.segments.len) {
            total += _.segments[i].integrate();
            i += 1;
        }
        return total;
    }
}

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

fn sameSignNonZero(a: f64, b: f64): bool {
    return (a > 0.0 && b > 0.0) || (a < 0.0 && b < 0.0);
}

fn validateNodes(xs: []const f64, ys: []const f64): bool {
    if (xs.len < 2 || xs.len != ys.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 1 < xs.len) {
        if (!(xs[i] < xs[i + 1])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn solveCubicTridiagonal(
    xs: []const f64,
    ys: []const f64,
    clamped: bool,
    dy0: f64,
    dy_end: f64,
    segments: *[]CubicSegment,
    scratch: *[]f64,
): CubicSpline? {
    if (!validateNodes(xs, ys)) {
        return null;
    }
    const n: usize = xs.len;
    const m: usize = n - 1;
    if (segments.*.len < m || scratch.*.len < (3 as usize) * n) {
        return null;
    }

    // Partition scratch into 3 arrays of length n:
    // c_prime: scratch[0..n], d_prime: scratch[n..2*n], sol_c: scratch[2*n..3*n]
    const cp_off: usize = 0;
    const dp_off: usize = n;
    const sol_off: usize = (2 as usize) * n;

    // Row 0
    if (clamped) {
        const h0: f64 = xs[1] - xs[0];
        const delta0: f64 = (ys[1] - ys[0]) / h0;
        const diag0: f64 = 2.0 * h0;
        scratch.*[cp_off + 0] = h0 / diag0; // 0.5
        scratch.*[dp_off + 0] = (3.0 * (delta0 - dy0)) / diag0;
    } else {
        scratch.*[cp_off + 0] = 0.0;
        scratch.*[dp_off + 0] = 0.0;
    }

    // Interior rows 1..n-2
    let i: usize = 1;
    while (i + 1 < n) {
        const h_prev: f64 = xs[i] - xs[i - 1];
        const h_curr: f64 = xs[i + 1] - xs[i];
        const d_prev: f64 = (ys[i] - ys[i - 1]) / h_prev;
        const d_curr: f64 = (ys[i + 1] - ys[i]) / h_curr;

        const lower: f64 = h_prev;
        const diag: f64 = 2.0 * (h_prev + h_curr);
        const upper: f64 = h_curr;
        const rhs: f64 = 3.0 * (d_curr - d_prev);

        const denom: f64 = diag - lower * scratch.*[cp_off + i - 1];
        scratch.*[cp_off + i] = upper / denom;
        scratch.*[dp_off + i] = (rhs - lower * scratch.*[dp_off + i - 1]) / denom;
        i += 1;
    }

    // Last row n-1
    if (clamped) {
        const h_last: f64 = xs[n - 1] - xs[n - 2];
        const delta_last: f64 = (ys[n - 1] - ys[n - 2]) / h_last;
        const lower_n: f64 = h_last;
        const diag_n: f64 = 2.0 * h_last;
        const rhs_n: f64 = 3.0 * (dy_end - delta_last);
        const denom_n: f64 = diag_n - lower_n * scratch.*[cp_off + n - 2];
        scratch.*[sol_off + n - 1] = (rhs_n - lower_n * scratch.*[dp_off + n - 2]) / denom_n;
    } else {
        scratch.*[sol_off + n - 1] = 0.0;
    }

    // Back-substitution
    let k: usize = n - 1;
    while (k > 0) {
        k -= 1;
        scratch.*[sol_off + k] = scratch.*[dp_off + k] - scratch.*[cp_off + k] * scratch.*[sol_off + k + 1];
    }

    // Populate cubic segments
    let s: usize = 0;
    while (s < m) {
        const h: f64 = xs[s + 1] - xs[s];
        const delta: f64 = (ys[s + 1] - ys[s]) / h;
        const ci: f64 = scratch.*[sol_off + s];
        const ci1: f64 = scratch.*[sol_off + s + 1];

        segments.*[s] = CubicSegment {
            x0: xs[s],
            x1: xs[s + 1],
            a: ys[s],
            b: delta - (h * (2.0 * ci + ci1)) / 3.0,
            c: ci,
            d: (ci1 - ci) / (3.0 * h),
        };
        s += 1;
    }

    return CubicSpline { segments: segments.*[0..m] };
}

/// Builds a Natural Cubic Spline (`S''(x_0) = S''(x_{n-1}) = 0`) using O(n) Thomas tridiagonal solver.
/// Requires `xs.len == ys.len >= 2`, strictly increasing `xs`, `segments.len >= xs.len - 1`, and `scratch.len >= 3 * xs.len`.
pub fn buildNaturalCubicSpline(
    xs: []const f64,
    ys: []const f64,
    segments: *[]CubicSegment,
    scratch: *[]f64,
): CubicSpline? {
    return solveCubicTridiagonal(xs, ys, false, 0.0, 0.0, segments, scratch);
}

/// Builds a Clamped Cubic Spline with specified endpoint first derivatives `S'(x_0) = dy0` and `S'(x_{n-1}) = dy_end`.
/// Requires `xs.len == ys.len >= 2`, strictly increasing `xs`, `segments.len >= xs.len - 1`, and `scratch.len >= 3 * xs.len`.
pub fn buildClampedCubicSpline(
    xs: []const f64,
    ys: []const f64,
    dy0: f64,
    dy_end: f64,
    segments: *[]CubicSegment,
    scratch: *[]f64,
): CubicSpline? {
    return solveCubicTridiagonal(xs, ys, true, dy0, dy_end, segments, scratch);
}

fn pchipEndpointSlope(h1: f64, h2: f64, del1: f64, del2: f64): f64 {
    let d: f64 = ((2.0 * h1 + h2) * del1 - h1 * del2) / (h1 + h2);
    if (!sameSignNonZero(d, del1)) {
        return 0.0;
    }
    if (!sameSignNonZero(del1, del2) && absF64(d) > 3.0 * absF64(del1)) {
        d = 3.0 * del1;
    }
    return d;
}

fn pchipNodeSlope(xs: []const f64, ys: []const f64, idx: usize): f64 {
    const n: usize = xs.len;
    if (n == 2) {
        return (ys[1] - ys[0]) / (xs[1] - xs[0]);
    }
    if (idx == 0) {
        const h0: f64 = xs[1] - xs[0];
        const h1: f64 = xs[2] - xs[1];
        const del0: f64 = (ys[1] - ys[0]) / h0;
        const del1: f64 = (ys[2] - ys[1]) / h1;
        return pchipEndpointSlope(h0, h1, del0, del1);
    }
    if (idx == n - 1) {
        const hn1: f64 = xs[n - 1] - xs[n - 2];
        const hn2: f64 = xs[n - 2] - xs[n - 3];
        const del_last: f64 = (ys[n - 1] - ys[n - 2]) / hn1;
        const del_prev: f64 = (ys[n - 2] - ys[n - 3]) / hn2;
        return pchipEndpointSlope(hn1, hn2, del_last, del_prev);
    }

    const h_prev: f64 = xs[idx] - xs[idx - 1];
    const h_curr: f64 = xs[idx + 1] - xs[idx];
    const del_prev: f64 = (ys[idx] - ys[idx - 1]) / h_prev;
    const del_curr: f64 = (ys[idx + 1] - ys[idx]) / h_curr;
    if (!sameSignNonZero(del_prev, del_curr)) {
        return 0.0;
    }
    const w1: f64 = 2.0 * h_curr + h_prev;
    const w2: f64 = h_curr + 2.0 * h_prev;
    return (w1 + w2) / ((w1 / del_prev) + (w2 / del_curr));
}

/// Builds a Fritsch-Carlson Piecewise Cubic Hermite Interpolating Polynomial (PCHIP) spline.
/// Strictly preserves monotonicity on monotonic data with zero overshoot and requires no scratch buffer.
pub fn buildPchipSpline(
    xs: []const f64,
    ys: []const f64,
    segments: *[]CubicSegment,
): CubicSpline? {
    if (!validateNodes(xs, ys)) {
        return null;
    }
    const n: usize = xs.len;
    const m: usize = n - 1;
    if (segments.*.len < m) {
        return null;
    }

    let i: usize = 0;
    while (i < m) {
        const h: f64 = xs[i + 1] - xs[i];
        const delta: f64 = (ys[i + 1] - ys[i]) / h;
        const m0: f64 = pchipNodeSlope(xs, ys, i);
        const m1: f64 = pchipNodeSlope(xs, ys, i + 1);

        segments.*[i] = CubicSegment {
            x0: xs[i],
            x1: xs[i + 1],
            a: ys[i],
            b: m0,
            c: (3.0 * delta - 2.0 * m0 - m1) / h,
            d: (m0 + m1 - 2.0 * delta) / (h * h),
        };
        i += 1;
    }

    return CubicSpline { segments: segments.*[0..m] };
}

/// Evaluates a standard uniform Catmull-Rom spline segment through `p1` (at `t=0`) and `p2` (at `t=1`)
/// with neighboring control points `p0` and `p3`.
pub fn catmullRom1d(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64 {
    const t2: f64 = t * t;
    const t3: f64 = t2 * t;
    const c0: f64 = 2.0 * p1;
    const c1: f64 = p2 - p0;
    const c2: f64 = 2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3;
    const c3: f64 = 3.0 * p1 - p0 - 3.0 * p2 + p3;
    return 0.5 * (c0 + c1 * t + c2 * t2 + c3 * t3);
}

/// Evaluates the first derivative `dP/dt` of the Catmull-Rom spline segment at parameter `t`.
pub fn catmullRom1dDeriv(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64 {
    const t2: f64 = t * t;
    const c1: f64 = p2 - p0;
    const c2: f64 = 2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3;
    const c3: f64 = 3.0 * p1 - p0 - 3.0 * p2 + p3;
    return 0.5 * (c1 + 2.0 * c2 * t + 3.0 * c3 * t2);
}
