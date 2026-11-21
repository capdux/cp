// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Least-Squares Polyfit, Givens QR, Ridge Regression & Root Finding)

import std from "std";

/// Maximum supported polynomial degree for zero-allocation stack QR solver.
pub const POLYFIT_MAX_DEG: usize = 15;

/// Result of ordinary least-squares simple linear regression (`y = slope * x + intercept`).
pub struct LinearFit {
    slope: f64,
    intercept: f64,
    r_squared: f64,
    rmse: f64,
}

/// Goodness-of-fit residual metrics for a fitted polynomial.
pub struct FitMetrics {
    sse: f64,
    mse: f64,
    rmse: f64,
    r_squared: f64,
    max_abs_err: f64,
}

fn absF64(v: f64): f64 {
    if (v < 0.0) {
        return 0.0 - v;
    }
    return v;
}

/// Evaluates polynomial `coeffs[0] + coeffs[1]*x + ... + coeffs[n-1]*x^(n-1)` via Horner's method.
pub fn polyEval(coeffs: []const f64, x: f64): f64 {
    if (coeffs.len == 0) {
        return 0.0;
    }
    let acc: f64 = 0.0;
    let i: usize = coeffs.len;
    while (i > 0) {
        i -= 1;
        acc = acc * x + coeffs[i];
    }
    return acc;
}

/// Evaluates the first derivative `P'(x)` of polynomial `coeffs` via Horner's method.
pub fn polyDerivEval(coeffs: []const f64, x: f64): f64 {
    if (coeffs.len <= 1) {
        return 0.0;
    }
    let acc: f64 = 0.0;
    let i: usize = coeffs.len - 1;
    while (i > 0) {
        acc = acc * x + coeffs[i] * (i as f64);
        i -= 1;
    }
    return acc;
}

/// Computes the coefficient array of the derivative `P'(x)` into `out`.
/// For a constant polynomial (`coeffs.len == 1`), writes `[0.0]` and returns `1`.
/// Returns `null` if `coeffs` is empty or `out` is too small.
pub fn polyDerivCoeffs(coeffs: []const f64, out: *[]f64): usize? {
    if (coeffs.len == 0) {
        return null;
    }
    if (coeffs.len == 1) {
        if (out.*.len < 1) {
            return null;
        }
        out.*[0] = 0.0;
        return 1;
    }
    const out_len: usize = coeffs.len - 1;
    if (out.*.len < out_len) {
        return null;
    }
    let i: usize = 0;
    while (i < out_len) {
        out.*[i] = coeffs[i + 1] * ((i + 1) as f64);
        i += 1;
    }
    return out_len;
}

/// Computes the antiderivative coefficients of `P(x)` with constant of integration `c0` into `out`.
/// Writes `coeffs.len + 1` coefficients; returns `null` if `out` is too small.
pub fn polyIntegrateCoeffs(coeffs: []const f64, c0: f64, out: *[]f64): usize? {
    const out_len: usize = coeffs.len + 1;
    if (out.*.len < out_len) {
        return null;
    }
    out.*[0] = c0;
    let i: usize = 0;
    while (i < coeffs.len) {
        out.*[i + 1] = coeffs[i] / ((i + 1) as f64);
        i += 1;
    }
    return out_len;
}

/// Centered single-pass ordinary least-squares linear regression.
/// Returns `null` if `xs.len < 2`, `xs.len != ys.len`, or all `xs` are identical.
pub fn linearFit(xs: []const f64, ys: []const f64): LinearFit? {
    const n: usize = xs.len;
    if (n < 2 || n != ys.len) {
        return null;
    }

    let sum_x: f64 = 0.0;
    let sum_y: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        sum_x += xs[i];
        sum_y += ys[i];
        i += 1;
    }
    const nf: f64 = n as f64;
    const mx: f64 = sum_x / nf;
    const my: f64 = sum_y / nf;

    let sxx: f64 = 0.0;
    let sxy: f64 = 0.0;
    let syy: f64 = 0.0;
    i = 0;
    while (i < n) {
        const dx: f64 = xs[i] - mx;
        const dy: f64 = ys[i] - my;
        sxx += dx * dx;
        sxy += dx * dy;
        syy += dy * dy;
        i += 1;
    }

    if (sxx <= 1.0e-30) {
        return null;
    }

    const slope: f64 = sxy / sxx;
    const intercept: f64 = my - slope * mx;

    let sse: f64 = 0.0;
    i = 0;
    while (i < n) {
        const err: f64 = ys[i] - (slope * xs[i] + intercept);
        sse += err * err;
        i += 1;
    }

    let r2: f64 = 1.0;
    if (syy > 1.0e-30) {
        r2 = 1.0 - (sse / syy);
    } else if (sse > 1.0e-30) {
        r2 = 0.0;
    }

    return LinearFit {
        slope: slope,
        intercept: intercept,
        r_squared: r2,
        rmse: std.math.sqrt(sse / nf),
    };
}

// Rotates a single augmented row [row[0..m-1] | rhs] into upper-triangular [R | qty] via Givens rotations.
fn givensInsertRow(r_mat: *[16][17]f64, row_in: *[17]f64, m: usize) {
    let col: usize = 0;
    while (col < m) {
        const b: f64 = row_in.*[col];
        if (absF64(b) > 0.0) {
            const a: f64 = r_mat.*[col][col];
            const hyp: f64 = std.math.hypot(a, b);
            if (hyp > 0.0) {
                const c: f64 = a / hyp;
                const s: f64 = b / hyp;
                r_mat.*[col][col] = hyp;
                row_in.*[col] = 0.0;

                let k: usize = col + 1;
                while (k <= m) {
                    const r_val: f64 = r_mat.*[col][k];
                    const x_val: f64 = row_in.*[k];
                    r_mat.*[col][k] = c * r_val + s * x_val;
                    row_in.*[k] = c * x_val - s * r_val;
                    k += 1;
                }
            }
        }
        col += 1;
    }
}

fn polyfitCore(
    xs: []const f64,
    ys: []const f64,
    deg: usize,
    lambda: f64,
    out_coeffs: *[]f64,
): bool {
    const n: usize = xs.len;
    const m: usize = deg + 1;
    if (deg > POLYFIT_MAX_DEG || n < m || n != ys.len || out_coeffs.*.len < m || lambda < 0.0) {
        return false;
    }

    // Special case: degree 0 (constant mean).
    if (deg == 0) {
        let sum_y: f64 = 0.0;
        let i: usize = 0;
        while (i < n) {
            sum_y += ys[i];
            i += 1;
        }
        out_coeffs.*[0] = sum_y / (n as f64);
        return true;
    }

    // Compute coordinate shift & scale u = (x - x_mid) / x_half in [-1, 1] for optimal conditioning.
    let x_min: f64 = xs[0];
    let x_max: f64 = xs[0];
    let i: usize = 1;
    while (i < n) {
        if (xs[i] < x_min) {
            x_min = xs[i];
        }
        if (xs[i] > x_max) {
            x_max = xs[i];
        }
        i += 1;
    }

    const x_half: f64 = 0.5 * (x_max - x_min);
    if (x_half <= 1.0e-30) {
        return false; // Degenerate identical x values cannot fit deg >= 1.
    }
    const x_mid: f64 = 0.5 * (x_max + x_min);

    // Streaming Givens QR factorization on stack (zero heap allocation, O(m^2) stack).
    let r_mat: [16][17]f64 = [[0.0; 17]; 16];
    let row: [17]f64 = [0.0; 17];

    i = 0;
    while (i < n) {
        const u: f64 = (xs[i] - x_mid) / x_half;
        let upow: f64 = 1.0;
        let c: usize = 0;
        while (c < m) {
            row[c] = upow;
            upow *= u;
            c += 1;
        }
        row[m] = ys[i];
        givensInsertRow(*r_mat, *row, m);
        i += 1;
    }

    // Optional L2 ridge penalty on non-intercept terms (k = 1..deg).
    if (lambda > 0.0) {
        const sqrt_lam: f64 = std.math.sqrt(lambda);
        let k: usize = 1;
        while (k < m) {
            let z: usize = 0;
            while (z <= m) {
                row[z] = 0.0;
                z += 1;
            }
            row[k] = sqrt_lam;
            givensInsertRow(*r_mat, *row, m);
            k += 1;
        }
    }

    // Back-substitution R * cu = Q^T y.
    let cu: [16]f64 = [0.0; 16];
    let row_idx: usize = m;
    while (row_idx > 0) {
        row_idx -= 1;
        const diag: f64 = r_mat[row_idx][row_idx];
        if (absF64(diag) <= 1.0e-14) {
            return false; // Rank-deficient / singular Vandermonde system.
        }
        let rhs: f64 = r_mat[row_idx][m];
        let col: usize = row_idx + 1;
        while (col < m) {
            rhs -= r_mat[row_idx][col] * cu[col];
            col += 1;
        }
        cu[row_idx] = rhs / diag;
    }

    // Unscale polynomial P(x) = sum cu[k] * (alpha * x + beta)^k into original x-basis via polynomial Horner.
    const alpha: f64 = 1.0 / x_half;
    const beta: f64 = (0.0 - x_mid) / x_half;

    let cx: [16]f64 = [0.0; 16];
    let cur_len: usize = 0;
    let k: usize = m;
    while (k > 0) {
        k -= 1;
        // Multiply current cx polynomial by (alpha * x + beta), then add cu[k].
        let j: usize = cur_len;
        while (j > 0) {
            cx[j] = cx[j] * beta + cx[j - 1] * alpha;
            j -= 1;
        }
        cx[0] = cx[0] * beta + cu[k];
        cur_len += 1;
    }

    let w: usize = 0;
    while (w < m) {
        out_coeffs.*[w] = cx[w];
        w += 1;
    }
    return true;
}

/// Fits a least-squares polynomial of degree `deg` (`0..=15`) using coordinate normalization and Givens QR.
/// Writes `deg + 1` lowest-degree-first coefficients into `out_coeffs`.
pub fn polyfit(xs: []const f64, ys: []const f64, deg: usize, out_coeffs: *[]f64): bool {
    return polyfitCore(xs, ys, deg, 0.0, out_coeffs);
}

/// Fits an L2-regularized (Ridge) polynomial of degree `deg` (`0..=15`) with penalty `lambda >= 0`.
/// Shrinks higher-order coefficients to suppress overfitting while keeping the intercept unpenalized.
pub fn polyfitRidge(
    xs: []const f64,
    ys: []const f64,
    deg: usize,
    lambda: f64,
    out_coeffs: *[]f64,
): bool {
    return polyfitCore(xs, ys, deg, lambda, out_coeffs);
}

/// Evaluates SSE, MSE, RMSE, R^2, and max absolute error of polynomial `coeffs` on `(xs, ys)`.
pub fn evalFitMetrics(xs: []const f64, ys: []const f64, coeffs: []const f64): FitMetrics? {
    const n: usize = xs.len;
    if (n == 0 || n != ys.len || coeffs.len == 0) {
        return null;
    }

    let sum_y: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        sum_y += ys[i];
        i += 1;
    }
    const nf: f64 = n as f64;
    const mean_y: f64 = sum_y / nf;

    let sse: f64 = 0.0;
    let sst: f64 = 0.0;
    let max_err: f64 = 0.0;
    i = 0;
    while (i < n) {
        const pred: f64 = polyEval(coeffs, xs[i]);
        const err: f64 = absF64(ys[i] - pred);
        sse += err * err;
        const dy: f64 = ys[i] - mean_y;
        sst += dy * dy;
        if (err > max_err) {
            max_err = err;
        }
        i += 1;
    }

    const mse: f64 = sse / nf;
    let r2: f64 = 1.0;
    if (sst > 1.0e-30) {
        r2 = 1.0 - (sse / sst);
    } else if (sse > 1.0e-30) {
        r2 = 0.0;
    }

    return FitMetrics {
        sse: sse,
        mse: mse,
        rmse: std.math.sqrt(mse),
        r_squared: r2,
        max_abs_err: max_err,
    };
}

/// Finds a real root of polynomial `coeffs` on bracket `[a, b]` using bisection.
/// Returns `null` if `coeffs` is empty, `tol <= 0`, `max_iter == 0`, or `P(a)` and `P(b)` have the same sign.
pub fn polyRootBisect(
    coeffs: []const f64,
    a: f64,
    b: f64,
    tol: f64,
    max_iter: u32,
): f64? {
    if (coeffs.len == 0 || tol <= 0.0 || max_iter == 0) {
        return null;
    }

    let lo: f64 = a;
    let hi: f64 = b;
    let flo: f64 = polyEval(coeffs, lo);
    let fhi: f64 = polyEval(coeffs, hi);

    if (absF64(flo) <= tol) {
        return lo;
    }
    if (absF64(fhi) <= tol) {
        return hi;
    }
    if ((flo > 0.0 && fhi > 0.0) || (flo < 0.0 && fhi < 0.0)) {
        return null;
    }

    let iter: u32 = 0;
    while (iter < max_iter) {
        const mid: f64 = 0.5 * (lo + hi);
        const fmid: f64 = polyEval(coeffs, mid);
        if (absF64(fmid) <= tol || absF64(hi - lo) * 0.5 <= tol) {
            return mid;
        }
        if ((flo < 0.0 && fmid < 0.0) || (flo > 0.0 && fmid > 0.0)) {
            lo = mid;
            flo = fmid;
        } else {
            hi = mid;
        }
        iter += 1;
    }
    return 0.5 * (lo + hi);
}

/// Finds a real root of polynomial `coeffs` starting at `x0` using Newton-Raphson iteration.
/// Returns `null` if `coeffs.len < 2`, `tol <= 0`, `max_iter == 0`, derivative vanishes, or iteration fails to converge.
pub fn polyRootNewton(
    coeffs: []const f64,
    x0: f64,
    tol: f64,
    max_iter: u32,
): f64? {
    if (coeffs.len < 2 || tol <= 0.0 || max_iter == 0) {
        return null;
    }

    let x: f64 = x0;
    let iter: u32 = 0;
    while (iter < max_iter) {
        const fx: f64 = polyEval(coeffs, x);
        if (absF64(fx) <= tol) {
            return x;
        }
        const dfx: f64 = polyDerivEval(coeffs, x);
        if (absF64(dfx) <= 1.0e-15) {
            return null;
        }
        const x_next: f64 = x - (fx / dfx);
        if (absF64(x_next - x) <= tol) {
            return x_next;
        }
        x = x_next;
        iter += 1;
    }
    return null;
}
