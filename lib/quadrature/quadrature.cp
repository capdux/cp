// Origin: Pure-CP Stdlib (Zero-Alloc Numerical Quadrature: Gauss-Legendre 5/10, Composite Simpson, Adaptive G7-K15 & Romberg) | Created: 2026-11-22

/// Scalar integrand function signature `y = f(x)`.
pub type IntegrandFn = fn(x: f64): f64;

/// Single-interval Gauss-Kronrod G7-K15 integral and error estimate.
pub struct Gk15Estimate {
    value: f64,
    abs_error: f64,
}

/// Summary result returned by adaptive and extrapolation quadrature routines.
pub struct QuadResult {
    value: f64,
    abs_error: f64,
    evals: usize,
    converged: bool,
}

// 5-point Gauss-Legendre nodes & weights on [-1, 1] (center + 2 symmetric pairs)
const GL5_W0: f64 = 0.56888888888888888889;
const GL5_X: [2]f64 = [
    0.53846931010568309104,
    0.90617984593866399280,
];
const GL5_W: [2]f64 = [
    0.47862867049936646804,
    0.23692688505618908751,
];

// 10-point Gauss-Legendre nodes & weights on [-1, 1] (5 symmetric pairs)
const GL10_X: [5]f64 = [
    0.14887433898163121088,
    0.43339539412924719080,
    0.67940956829902440623,
    0.86506336668898451073,
    0.97390652851717172008,
];
const GL10_W: [5]f64 = [
    0.29552422471475287017,
    0.26926671930999635509,
    0.21908636251598204400,
    0.14945134915058059315,
    0.06667134430868813759,
];

// QUADPACK G7-K15 Gauss-Kronrod abscissae & weights on [-1, 1]
const GK15_X: [7]f64 = [
    0.99145537112081263921,
    0.94910791234275852453,
    0.86486442335976907279,
    0.74153118559939443986,
    0.58608723546769113029,
    0.40584515137739716691,
    0.20778495500789846760,
];
const GK15_WK: [7]f64 = [
    0.02293532201052922496,
    0.06309209262997855329,
    0.10479001032225018384,
    0.14065325971552591875,
    0.16900472663926790283,
    0.19035057806478540991,
    0.20443294007529889241,
];
const GK15_WK_CENTER: f64 = 0.20948214108472782801;

// Embedded 7-point Gauss weights corresponding to GK15_X[1], GK15_X[3], GK15_X[5] and center
const GK15_WG: [3]f64 = [
    0.12948496616886969327,
    0.27970539148927666790,
    0.38183005050511894495,
];
const GK15_WG_CENTER: f64 = 0.41795918367346938776;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

/// 5-point Gauss-Legendre quadrature on `[a, b]` (exact for polynomials up to degree 9).
pub fn gaussLegendre5(f: IntegrandFn, a: f64, b: f64): f64 {
    const mid: f64 = 0.5 * (a + b);
    const half: f64 = 0.5 * (b - a);
    let sum: f64 = GL5_W0 * f(mid);
    let i: usize = 0;
    while (i < 2) {
        const dx: f64 = half * GL5_X[i];
        sum += GL5_W[i] * (f(mid - dx) + f(mid + dx));
        i += 1;
    }
    return half * sum;
}

/// 10-point Gauss-Legendre quadrature on `[a, b]` (exact for polynomials up to degree 19).
pub fn gaussLegendre10(f: IntegrandFn, a: f64, b: f64): f64 {
    const mid: f64 = 0.5 * (a + b);
    const half: f64 = 0.5 * (b - a);
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < 5) {
        const dx: f64 = half * GL10_X[i];
        sum += GL10_W[i] * (f(mid - dx) + f(mid + dx));
        i += 1;
    }
    return half * sum;
}

/// Composite Simpson's 1/3 rule on `[a, b]` with an even number of `panels > 0`.
/// Returns `null` if `panels == 0`, `panels` is odd, or `a`/`b`/evaluations are non-finite.
pub fn compositeSimpson(f: IntegrandFn, a: f64, b: f64, panels: usize): f64? {
    if (!isFiniteF64(a) || !isFiniteF64(b) || panels == 0 || (panels % 2) != 0) {
        return null;
    }
    if (a == b) {
        return 0.0;
    }
    const h: f64 = (b - a) / (panels as f64);
    let sum: f64 = f(a) + f(b);
    let i: usize = 1;
    while (i < panels) {
        const x: f64 = a + (i as f64) * h;
        const coeff: f64 = if ((i % 2) == 1) 4.0 else 2.0;
        sum += coeff * f(x);
        i += 1;
    }
    const res: f64 = sum * (h / 3.0);
    if (!isFiniteF64(res)) {
        return null;
    }
    return res;
}

/// Single-panel 15-point Gauss-Kronrod (G7-K15) quadrature with embedded error estimate.
/// Open quadrature rule: never evaluates `f` at endpoints `a` or `b`.
pub fn gaussKronrod15(f: IntegrandFn, a: f64, b: f64): Gk15Estimate {
    const mid: f64 = 0.5 * (a + b);
    const half: f64 = 0.5 * (b - a);

    const f_center: f64 = f(mid);
    let res_g: f64 = GK15_WG_CENTER * f_center;
    let res_k: f64 = GK15_WK_CENTER * f_center;

    // Pairs 0, 2, 4, 6 (Kronrod-only) and 1, 3, 5 (shared Gauss + Kronrod)
    let j: usize = 0;
    while (j < 3) {
        const k_idx: usize = (2 as usize) * j + 1;
        const dx: f64 = half * GK15_X[k_idx];
        const fsum: f64 = f(mid - dx) + f(mid + dx);
        res_g += GK15_WG[j] * fsum;
        res_k += GK15_WK[k_idx] * fsum;
        j += 1;
    }

    let j2: usize = 0;
    while (j2 < 4) {
        const k_idx: usize = (2 as usize) * j2;
        const dx: f64 = half * GK15_X[k_idx];
        const fsum: f64 = f(mid - dx) + f(mid + dx);
        res_k += GK15_WK[k_idx] * fsum;
        j2 += 1;
    }

    const val_k: f64 = res_k * half;
    const val_g: f64 = res_g * half;
    return Gk15Estimate {
        value: val_k,
        abs_error: absF64(val_k - val_g),
    };
}

/// Non-recursive adaptive Gauss-Kronrod G7-K15 quadrature using caller-owned explicit interval stacks.
/// Automatically handles reversed bounds (`a > b` negates the integral).
/// Returns `null` on non-finite bounds/tolerances/evaluations, `max_subdivisions == 0`, or stack capacity `< 2`.
pub fn adaptiveGk15(
    f: IntegrandFn,
    a: f64,
    b: f64,
    abs_tol: f64,
    rel_tol: f64,
    max_subdivisions: usize,
    stack_a: *[]f64,
    stack_b: *[]f64,
): QuadResult? {
    if (!isFiniteF64(a) || !isFiniteF64(b) || !isFiniteF64(abs_tol) || !isFiniteF64(rel_tol)) {
        return null;
    }
    if (abs_tol < 0.0 || rel_tol < 0.0 || (abs_tol == 0.0 && rel_tol == 0.0)) {
        return null;
    }
    if (max_subdivisions == 0) {
        return null;
    }
    const cap: usize = if (stack_a.*.len < stack_b.*.len) stack_a.*.len else stack_b.*.len;
    if (cap < 2) {
        return null;
    }
    if (a == b) {
        return QuadResult {
            value: 0.0,
            abs_error: 0.0,
            evals: 0,
            converged: true,
        };
    }

    const sign: f64 = if (a <= b) 1.0 else -1.0;
    const lo: f64 = if (a <= b) a else b;
    const hi: f64 = if (a <= b) b else a;
    const full_span: f64 = hi - lo;

    // Seed stack with initial interval [lo, hi]
    stack_a.*[0] = lo;
    stack_b.*[0] = hi;
    let top: usize = 1;

    let total_val: f64 = 0.0;
    let total_err: f64 = 0.0;
    let evals: usize = 0;
    let subdivisions: usize = 0;
    let converged: bool = true;
    let global_scale: f64 = 0.0;

    while (top > 0) {
        top -= 1;
        const sa: f64 = stack_a.*[top];
        const sb: f64 = stack_b.*[top];

        const est: Gk15Estimate = gaussKronrod15(f, sa, sb);
        evals += 15;
        if (!isFiniteF64(est.value) || !isFiniteF64(est.abs_error)) {
            return null;
        }
        const mag_est: f64 = absF64(est.value);
        if (evals == 15) {
            global_scale = mag_est;
        } else if (absF64(total_val) + mag_est > global_scale) {
            global_scale = absF64(total_val) + mag_est;
        }

        const frac: f64 = (sb - sa) / full_span;
        const scaled_ref: f64 = if (mag_est > global_scale * frac) mag_est else (global_scale * frac);
        const abs_t: f64 = abs_tol * frac;
        const rel_t: f64 = rel_tol * scaled_ref;
        let local_tol: f64 = if (abs_t > rel_t) abs_t else rel_t;
        const floor_tol: f64 = 1.0e-15 * mag_est;
        if (local_tol < floor_tol) {
            local_tol = floor_tol;
        }

        const mid: f64 = 0.5 * (sa + sb);
        if (est.abs_error <= local_tol || mid <= sa || mid >= sb) {
            total_val += est.value;
            total_err += est.abs_error;
        } else if (subdivisions < max_subdivisions && (top + 2) <= cap) {
            stack_a.*[top] = mid;
            stack_b.*[top] = sb;
            stack_a.*[top + 1] = sa;
            stack_b.*[top + 1] = mid;
            top += 2;
            subdivisions += 1;
        } else {
            total_val += est.value;
            total_err += est.abs_error;
            converged = false;
        }
    }

    return QuadResult {
        value: sign * total_val,
        abs_error: total_err,
        evals: evals,
        converged: converged,
    };
}

/// Romberg integration via Richardson extrapolation using two caller-owned row buffers
/// `prev_row` and `curr_row` (each length `>= max_order`, `max_order >= 2`, `max_order <= 24`).
/// Automatically supports reversed bounds `a > b`.
pub fn romberg(
    f: IntegrandFn,
    a: f64,
    b: f64,
    tol: f64,
    max_order: usize,
    prev_row: *[]f64,
    curr_row: *[]f64,
): QuadResult? {
    if (!isFiniteF64(a) || !isFiniteF64(b) || !isFiniteF64(tol) || tol <= 0.0) {
        return null;
    }
    if (max_order < 2 || max_order > 24 || prev_row.*.len < max_order || curr_row.*.len < max_order) {
        return null;
    }
    if (a == b) {
        return QuadResult {
            value: 0.0,
            abs_error: 0.0,
            evals: 0,
            converged: true,
        };
    }

    const fa: f64 = f(a);
    const fb: f64 = f(b);
    if (!isFiniteF64(fa) || !isFiniteF64(fb)) {
        return null;
    }

    let h: f64 = b - a;
    prev_row.*[0] = 0.5 * h * (fa + fb);
    let evals: usize = 2;
    let m_pts: usize = 1;
    let last_err: f64 = 0.0;

    let k: usize = 1;
    while (k < max_order) {
        h *= 0.5;
        let sum: f64 = 0.0;
        let j: usize = 0;
        while (j < m_pts) {
            const x: f64 = a + ((2 as usize) * j + 1) as f64 * h;
            const fx: f64 = f(x);
            if (!isFiniteF64(fx)) {
                return null;
            }
            sum += fx;
            j += 1;
        }
        evals += m_pts;
        curr_row.*[0] = 0.5 * prev_row.*[0] + h * sum;

        let pow4: f64 = 1.0;
        let col: usize = 1;
        while (col <= k) {
            pow4 *= 4.0;
            curr_row.*[col] = curr_row.*[col - 1]
                + (curr_row.*[col - 1] - prev_row.*[col - 1]) / (pow4 - 1.0);
            col += 1;
        }

        const est: f64 = curr_row.*[k];
        last_err = absF64(est - prev_row.*[k - 1]);
        if (k >= 2 && last_err <= tol * (1.0 + absF64(est))) {
            return QuadResult {
                value: est,
                abs_error: last_err,
                evals: evals,
                converged: true,
            };
        }

        let cp: usize = 0;
        while (cp <= k) {
            prev_row.*[cp] = curr_row.*[cp];
            cp += 1;
        }
        m_pts *= 2;
        k += 1;
    }

    return QuadResult {
        value: prev_row.*[max_order - 1],
        abs_error: last_err,
        evals: evals,
        converged: false,
    };
}
