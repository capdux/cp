// Origin: Pure-CP Stdlib (Unbounded-Slice Numerical Optimization & Two-Phase Simplex LP Suite) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-workspace numerical optimization engine:
//   - 1D derivative-free minimization (`brentMinimize1d`) & bracketed root-finding (`brentRoot1d`)
//   - Two-Phase Simplex Linear Programming solver (`solveLinearProgramSimplex`) with Bland's anti-cycling rule,
//     supporting arbitrary `n` variables, `m` inequality constraints (`A x <= b, x >= 0`), negative RHS Phase I
//     feasibility, and exact `optimal` / `infeasible` / `unbounded` classification
//   - Central finite-difference gradient (`approxGradientCentral`) & Armijo backtracking line search
//   - Polak-Ribiere+ Conjugate Gradient (`minimizeConjugateGradient`) with automatic descent restart
//   - Limited-Memory BFGS (`minimizeLbfgs`) using Nocedal's O(m*n) Two-Loop Recursion
//   - Nelder-Mead Derivative-Free Simplex optimizer (`minimizeNelderMead`) with reflection/expansion/contraction/shrink

const std = @import("std");

// ============================================================================
// 1. Callback Types, Errors, Statuses & Summary Models
// ============================================================================

/// 1D scalar function callback `f(x)`.
pub type OptScalarFn = fn(x: f64): f64;

/// Multi-dimensional objective callback `f(x)`.
pub type OptObjectiveFn = fn(x: []const f64): f64;

/// Multi-dimensional gradient callback writing `grad f(x)` into `out_grad[0..x.len]`.
pub type OptGradientFn = fn(x: []const f64, out_grad: *[]f64);

/// Error set returned by numerical optimization and LP routines.
pub err OptError {
    InvalidDimension,
    InvalidBounds,
    InvalidTolerance,
    WorkBufferTooSmall,
    NonFiniteValue,
}

/// Convergence status for 1D and multi-dimensional optimization routines.
pub enum OptStatus {
    converged,
    max_iters_reached,
    infeasible,
    unbounded,
    stalled,
}

/// Unified optimization result summary.
pub struct OptSummary {
    status: OptStatus = OptStatus.converged;
    x_best: f64 = 0.0;
    objective: f64 = 0.0;
    grad_norm: f64 = 0.0;
    iterations: usize = 0;
    func_evals: usize = 0;
}

/// Termination status of Two-Phase Simplex Linear Programming.
pub enum LpStatus {
    optimal,
    infeasible,
    unbounded,
    max_iters_reached,
}

/// Result of `solveLinearProgramSimplex`.
pub struct LpResult {
    status: LpStatus = LpStatus.optimal;
    objective: f64 = 0.0;
    iterations: usize = 0;
}

// ============================================================================
// 2. Internal Vector & Numerical Helpers
// ============================================================================

const CGOLD_F64: f64 = 0.3819660112501051;
const ZEPS_F64: f64 = 1.0e-12;
const LP_EPS_F64: f64 = 1.0e-9;

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn dotSlice(a: []const f64, b: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        s += a[i] * b[i];
    }
    return s;
}

fn norm2Slice(a: []const f64): f64 {
    return std.math.sqrt(dotSlice(a, a));
}

fn normInfSlice(a: []const f64): f64 {
    let m: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const v: f64 = absF64(a[i]);
        if (v > m) m = v;
    }
    return m;
}

fn copySliceF64(dst: *[]f64, src: []const f64) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

// ============================================================================
// 3. 1D Brent Minimization & Bracketed Root-Finding
// ============================================================================

/// Minimizes a 1D scalar function `f_fn` on `[a_in, b_in]` using Brent's parabolic interpolation
/// and golden-section hybrid method. Returns `OptSummary` with `x_best` and `objective`.
pub fn brentMinimize1d(
    f_fn: OptScalarFn,
    a_in: f64,
    b_in: f64,
    tol: f64,
    max_iters: usize,
): OptSummary ! OptError {
    if (!isFinite(a_in) || !isFinite(b_in) || a_in >= b_in) {
        return OptError.InvalidBounds;
    }
    if (!isFinite(tol) || tol <= 0.0 || max_iters == 0) {
        return OptError.InvalidTolerance;
    }

    let a: f64 = a_in;
    let b: f64 = b_in;
    let x: f64 = a + CGOLD_F64 * (b - a);
    let w: f64 = x;
    let v: f64 = x;
    let fx: f64 = f_fn(x);
    if (!isFinite(fx)) {
        return OptError.NonFiniteValue;
    }
    let fw: f64 = fx;
    let fv: f64 = fx;
    let evals: usize = 1;

    let d: f64 = 0.0;
    let e: f64 = 0.0;
    let iter: usize = 0;

    while (iter < max_iters) : (iter += 1) {
        const xm: f64 = 0.5 * (a + b);
        const tol1: f64 = tol * absF64(x) + ZEPS_F64;
        const tol2: f64 = 2.0 * tol1;

        if (absF64(x - xm) <= tol2 - 0.5 * (b - a)) {
            return OptSummary{
                status: OptStatus.converged,
                x_best: x,
                objective: fx,
                grad_norm: 0.5 * (b - a),
                iterations: iter,
                func_evals: evals,
            };
        }

        let use_golden: bool = true;
        if (absF64(e) > tol1) {
            const r: f64 = (x - w) * (fx - fv);
            let q: f64 = (x - v) * (fx - fw);
            let p: f64 = (x - v) * q - (x - w) * r;
            q = 2.0 * (q - r);
            if (q > 0.0) {
                p = -p;
            } else {
                q = -q;
            }
            const etemp: f64 = e;
            e = d;
            if (absF64(p) < absF64(0.5 * q * etemp) && p > q * (a - x) && p < q * (b - x)) {
                d = p / q;
                const u_cand: f64 = x + d;
                if ((u_cand - a) < tol2 || (b - u_cand) < tol2) {
                    d = if (xm - x >= 0.0) tol1 else -tol1;
                }
                use_golden = false;
            }
        }

        if (use_golden) {
            e = if (x >= xm) a - x else b - x;
            d = CGOLD_F64 * e;
        }

        const step: f64 = if (absF64(d) >= tol1) d else (if (d >= 0.0) tol1 else -tol1);
        const u: f64 = x + step;
        const fu: f64 = f_fn(u);
        evals += 1;
        if (!isFinite(fu)) {
            return OptError.NonFiniteValue;
        }

        if (fu <= fx) {
            if (u >= x) {
                a = x;
            } else {
                b = x;
            }
            v = w;
            fv = fw;
            w = x;
            fw = fx;
            x = u;
            fx = fu;
        } else {
            if (u < x) {
                a = u;
            } else {
                b = u;
            }
            if (fu <= fw || w == x) {
                v = w;
                fv = fw;
                w = u;
                fw = fu;
            } else if (fu <= fv || v == x || v == w) {
                v = u;
                fv = fu;
            }
        }
    }

    return OptSummary{
        status: OptStatus.max_iters_reached,
        x_best: x,
        objective: fx,
        grad_norm: 0.5 * (b - a),
        iterations: iter,
        func_evals: evals,
    };
}

/// Finds a root `f_fn(x) = 0` inside bracket `[a_in, b_in]` (requiring `f(a)*f(b) <= 0`)
/// using Brent's superlinear inverse-quadratic/secant/bisection hybrid method.
pub fn brentRoot1d(
    f_fn: OptScalarFn,
    a_in: f64,
    b_in: f64,
    tol: f64,
    max_iters: usize,
): f64 ! OptError {
    if (!isFinite(a_in) || !isFinite(b_in) || a_in >= b_in) {
        return OptError.InvalidBounds;
    }
    if (!isFinite(tol) || tol <= 0.0 || max_iters == 0) {
        return OptError.InvalidTolerance;
    }

    let a: f64 = a_in;
    let b: f64 = b_in;
    let fa: f64 = f_fn(a);
    let fb: f64 = f_fn(b);
    if (!isFinite(fa) || !isFinite(fb)) {
        return OptError.NonFiniteValue;
    }
    if (fa == 0.0) return a;
    if (fb == 0.0) return b;
    if ((fa > 0.0 && fb > 0.0) || (fa < 0.0 && fb < 0.0)) {
        return OptError.InvalidBounds;
    }

    let c: f64 = b;
    let fc: f64 = fb;
    let d: f64 = b - a;
    let e: f64 = d;

    let iter: usize = 0;
    while (iter < max_iters) : (iter += 1) {
        if ((fb > 0.0 && fc > 0.0) || (fb < 0.0 && fc < 0.0)) {
            c = a;
            fc = fa;
            d = b - a;
            e = d;
        }
        if (absF64(fc) < absF64(fb)) {
            a = b;
            b = c;
            c = a;
            fa = fb;
            fb = fc;
            fc = fa;
        }

        const tol1: f64 = 2.0 * 2.220446049250313e-16 * absF64(b) + 0.5 * tol;
        const xm: f64 = 0.5 * (c - b);
        if (absF64(xm) <= tol1 || fb == 0.0) {
            return b;
        }

        if (absF64(e) >= tol1 && absF64(fa) > absF64(fb)) {
            const s: f64 = fb / fa;
            let p: f64 = 0.0;
            let q: f64 = 0.0;
            if (a == c) {
                p = 2.0 * xm * s;
                q = 1.0 - s;
            } else {
                const q_ratio: f64 = fa / fc;
                const r_ratio: f64 = fb / fc;
                p = s * (2.0 * xm * q_ratio * (q_ratio - r_ratio) - (b - a) * (r_ratio - 1.0));
                q = (q_ratio - 1.0) * (r_ratio - 1.0) * (s - 1.0);
            }
            if (p > 0.0) {
                q = -q;
            } else {
                p = -p;
            }
            const min1: f64 = 3.0 * xm * q - absF64(tol1 * q);
            const min2: f64 = absF64(e * q);
            const bound: f64 = if (min1 < min2) min1 else min2;
            if (2.0 * p < bound) {
                e = d;
                d = p / q;
            } else {
                d = xm;
                e = d;
            }
        } else {
            d = xm;
            e = d;
        }

        a = b;
        fa = fb;
        if (absF64(d) > tol1) {
            b += d;
        } else {
            b += if (xm >= 0.0) tol1 else -tol1;
        }
        fb = f_fn(b);
        if (!isFinite(fb)) {
            return OptError.NonFiniteValue;
        }
    }

    return b;
}

// ============================================================================
// 4. Two-Phase Simplex Linear Programming Solver (Bland's Anti-Cycling Rule)
// ============================================================================

/// Returns the minimum `tableau_work.len` required by `solveLinearProgramSimplex`.
pub fn lpTableauRequiredLen(num_vars: usize, num_constraints: usize): usize {
    const rows: usize = num_constraints + 1;
    const cols: usize = num_vars + 2 * num_constraints + 1;
    return rows * cols;
}

fn lpPivot(
    tableau: *[]f64,
    rows: usize,
    cols: usize,
    pivot_row: usize,
    pivot_col: usize,
) {
    const piv_off: usize = pivot_row * cols;
    const inv_piv: f64 = (1.0 as f64) / tableau[piv_off + pivot_col];

    let c: usize = 0;
    while (c < cols) : (c += 1) {
        tableau[piv_off + c] *= inv_piv;
    }
    tableau[piv_off + pivot_col] = 1.0;

    let r: usize = 0;
    while (r < rows) : (r += 1) {
        if (r == pivot_row) continue;
        const r_off: usize = r * cols;
        const factor: f64 = tableau[r_off + pivot_col];
        if (absF64(factor) <= 1.0e-15) continue;
        let k: usize = 0;
        while (k < cols) : (k += 1) {
            tableau[r_off + k] -= factor * tableau[piv_off + k];
        }
        tableau[r_off + pivot_col] = 0.0;
    }
}

fn lpRunSimplexPhase(
    tableau: *[]f64,
    basis: *[]usize,
    num_constraints: usize,
    cols: usize,
    active_col_limit: usize,
    max_iters: usize,
    total_iters: *usize,
): LpStatus {
    const rows: usize = num_constraints + 1;
    const obj_off: usize = num_constraints * cols;
    const rhs_col: usize = cols - 1;

    while (total_iters.* < max_iters) {
        // Bland's rule: pick smallest column index with strictly negative reduced cost.
        let enter_col: usize = cols;
        let j: usize = 0;
        while (j < active_col_limit) : (j += 1) {
            if (tableau[obj_off + j] < -LP_EPS_F64) {
                enter_col = j;
                break;
            }
        }
        if (enter_col == cols) {
            return LpStatus.optimal;
        }

        // Minimum ratio test with Bland's tie-breaker (smallest basis variable index).
        let leave_row: usize = rows;
        let min_ratio: f64 = 0.0;
        let i: usize = 0;
        while (i < num_constraints) : (i += 1) {
            const a_ij: f64 = tableau[i * cols + enter_col];
            if (a_ij > LP_EPS_F64) {
                let rhs_i: f64 = tableau[i * cols + rhs_col];
                if (rhs_i < 0.0 && rhs_i > -LP_EPS_F64) {
                    rhs_i = 0.0;
                }
                const ratio: f64 = rhs_i / a_ij;
                if (leave_row == rows ||
                    ratio < min_ratio - LP_EPS_F64 ||
                    (absF64(ratio - min_ratio) <= LP_EPS_F64 && basis[i] < basis[leave_row]))
                {
                    min_ratio = ratio;
                    leave_row = i;
                }
            }
        }

        if (leave_row == rows) {
            return LpStatus.unbounded;
        }

        lpPivot(tableau, rows, cols, leave_row, enter_col);
        basis[leave_row] = enter_col;
        total_iters.* += 1;
    }

    return LpStatus.max_iters_reached;
}

/// Solves the linear program `minimize c^T x` subject to `A x <= b, x >= 0` using Two-Phase Simplex
/// with Bland's anti-cycling rule. Supports arbitrary `num_vars` and `num_constraints` via caller
/// workspaces (`tableau_work.len >= lpTableauRequiredLen(num_vars, num_constraints)`,
/// `basis_work.len >= num_constraints`, `out_x.len >= num_vars`).
pub fn solveLinearProgramSimplex(
    c: []const f64,
    a_matrix: []const f64,
    b_rhs: []const f64,
    num_vars: usize,
    num_constraints: usize,
    out_x: *[]f64,
    tableau_work: *[]f64,
    basis_work: *[]usize,
    max_iters: usize,
): LpResult ! OptError {
    if (num_vars == 0 || num_constraints == 0) {
        return OptError.InvalidDimension;
    }
    if (c.len < num_vars || b_rhs.len < num_constraints || a_matrix.len < num_constraints * num_vars) {
        return OptError.InvalidDimension;
    }
    const req_len: usize = lpTableauRequiredLen(num_vars, num_constraints);
    if (out_x.len < num_vars || tableau_work.len < req_len || basis_work.len < num_constraints) {
        return OptError.WorkBufferTooSmall;
    }
    if (max_iters == 0) {
        return OptError.InvalidTolerance;
    }

    const rows: usize = num_constraints + 1;
    const cols: usize = num_vars + 2 * num_constraints + 1;
    const art_start: usize = num_vars + num_constraints;
    const rhs_col: usize = cols - 1;

    // Zero out tableau and output vector.
    let z: usize = 0;
    while (z < req_len) : (z += 1) {
        tableau_work[z] = 0.0;
    }
    let vx: usize = 0;
    while (vx < num_vars) : (vx += 1) {
        out_x[vx] = 0.0;
    }

    // Build constraint rows; flip rows with b[i] < 0 and introduce artificial variables.
    let has_artificial: bool = false;
    let i: usize = 0;
    while (i < num_constraints) : (i += 1) {
        const bi: f64 = b_rhs[i];
        if (!isFinite(bi)) {
            return OptError.NonFiniteValue;
        }
        const r_off: usize = i * cols;
        const a_off: usize = i * num_vars;

        if (bi >= 0.0) {
            let j: usize = 0;
            while (j < num_vars) : (j += 1) {
                const aij: f64 = a_matrix[a_off + j];
                if (!isFinite(aij)) return OptError.NonFiniteValue;
                tableau_work[r_off + j] = aij;
            }
            tableau_work[r_off + num_vars + i] = 1.0; // slack
            tableau_work[r_off + rhs_col] = bi;
            basis_work[i] = num_vars + i;
        } else {
            // Multiply row by -1: (-A_i) x - s_i + a_i = -b_i > 0
            let j: usize = 0;
            while (j < num_vars) : (j += 1) {
                const aij: f64 = a_matrix[a_off + j];
                if (!isFinite(aij)) return OptError.NonFiniteValue;
                tableau_work[r_off + j] = -aij;
            }
            tableau_work[r_off + num_vars + i] = -1.0; // surplus
            tableau_work[r_off + art_start + i] = 1.0; // artificial
            tableau_work[r_off + rhs_col] = -bi;
            basis_work[i] = art_start + i;
            has_artificial = true;
        }
    }

    let j_chk: usize = 0;
    while (j_chk < num_vars) : (j_chk += 1) {
        if (!isFinite(c[j_chk])) return OptError.NonFiniteValue;
    }

    let total_iters: usize = 0;
    const obj_off: usize = num_constraints * cols;

    // Phase I: minimize sum of artificial variables if any b[i] < 0.
    if (has_artificial) {
        let r1: usize = 0;
        while (r1 < num_constraints) : (r1 += 1) {
            if (basis_work[r1] >= art_start) {
                tableau_work[obj_off + basis_work[r1]] = 1.0;
                // Eliminate artificial basic variable from Phase I objective row.
                let col_k: usize = 0;
                while (col_k < cols) : (col_k += 1) {
                    tableau_work[obj_off + col_k] -= tableau_work[r1 * cols + col_k];
                }
            }
        }

        const p1_status = lpRunSimplexPhase(
            tableau_work,
            basis_work,
            num_constraints,
            cols,
            rhs_col,
            max_iters,
            *total_iters,
        );
        if (p1_status == LpStatus.max_iters_reached) {
            return LpResult{
                status: LpStatus.max_iters_reached,
                objective: 0.0,
                iterations: total_iters,
            };
        }

        const phase1_obj: f64 = -tableau_work[obj_off + rhs_col];
        if (phase1_obj > 1.0e-7) {
            return LpResult{
                status: LpStatus.infeasible,
                objective: 0.0,
                iterations: total_iters,
            };
        }

        // Pivot any degenerate zero-valued artificial variable out of the basis when possible.
        let r_art: usize = 0;
        while (r_art < num_constraints) : (r_art += 1) {
            if (basis_work[r_art] >= art_start) {
                let piv_c: usize = art_start;
                let jc: usize = 0;
                while (jc < art_start) : (jc += 1) {
                    if (absF64(tableau_work[r_art * cols + jc]) > LP_EPS_F64) {
                        piv_c = jc;
                        break;
                    }
                }
                if (piv_c < art_start) {
                    lpPivot(tableau_work, rows, cols, r_art, piv_c);
                    basis_work[r_art] = piv_c;
                }
            }
        }
    }

    // Phase II: install original objective `minimize c^T x` over non-artificial columns [0..art_start).
    let col_z: usize = 0;
    while (col_z < cols) : (col_z += 1) {
        tableau_work[obj_off + col_z] = 0.0;
    }
    let j2: usize = 0;
    while (j2 < num_vars) : (j2 += 1) {
        tableau_work[obj_off + j2] = c[j2];
    }

    // Eliminate basic decision variables from the Phase II objective row.
    let r2: usize = 0;
    while (r2 < num_constraints) : (r2 += 1) {
        const bv: usize = basis_work[r2];
        if (bv < num_vars) {
            const cb: f64 = c[bv];
            if (absF64(cb) > 1.0e-15) {
                let col_k: usize = 0;
                while (col_k < cols) : (col_k += 1) {
                    tableau_work[obj_off + col_k] -= cb * tableau_work[r2 * cols + col_k];
                }
            }
        }
    }

    const p2_status = lpRunSimplexPhase(
        tableau_work,
        basis_work,
        num_constraints,
        cols,
        art_start,
        max_iters,
        *total_iters,
    );

    if (p2_status != LpStatus.optimal) {
        return LpResult{
            status: p2_status,
            objective: 0.0,
            iterations: total_iters,
        };
    }

    // Extract primal solution x >= 0.
    let r3: usize = 0;
    while (r3 < num_constraints) : (r3 += 1) {
        const bv: usize = basis_work[r3];
        if (bv < num_vars) {
            let val: f64 = tableau_work[r3 * cols + rhs_col];
            if (val < 0.0 && val > -1.0e-9) val = 0.0;
            out_x[bv] = val;
        }
    }

    return LpResult{
        status: LpStatus.optimal,
        objective: -tableau_work[obj_off + rhs_col],
        iterations: total_iters,
    };
}

// ============================================================================
// 5. Finite-Difference Gradient & Armijo Backtracking Line Search
// ============================================================================

/// Approximates `grad f(x)` via central finite differences `(f(x+e_i*eps) - f(x-e_i*eps)) / (2*eps)`.
pub fn approxGradientCentral(
    f_fn: OptObjectiveFn,
    x: *[]f64,
    eps: f64,
    out_grad: *[]f64,
): void ! OptError {
    if (x.len == 0) {
        return OptError.InvalidDimension;
    }
    if (out_grad.len < x.len) {
        return OptError.WorkBufferTooSmall;
    }
    if (!isFinite(eps) || eps <= 0.0) {
        return OptError.InvalidTolerance;
    }
    const inv_2eps: f64 = (0.5 as f64) / eps;
    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        const orig: f64 = x[i];
        x[i] = orig + eps;
        const fp: f64 = f_fn(x.*);
        x[i] = orig - eps;
        const fm: f64 = f_fn(x.*);
        x[i] = orig;
        if (!isFinite(fp) || !isFinite(fm)) {
            return OptError.NonFiniteValue;
        }
        out_grad[i] = (fp - fm) * inv_2eps;
    }
}

/// Armijo sufficient-decrease backtracking line search along descent direction `dir`.
/// Writes the accepted iterate into `x_trial` and returns the accepted step size `alpha` (`0.0` if failed).
pub fn armijoBacktrackingLineSearch(
    f_fn: OptObjectiveFn,
    x: []const f64,
    dir: []const f64,
    f0: f64,
    g_dot_d: f64,
    init_step: f64,
    c1: f64,
    shrink: f64,
    max_steps: usize,
    x_trial: *[]f64,
    out_evals: *usize,
): f64 {
    if (g_dot_d >= 0.0 || init_step <= 0.0 || shrink <= 0.0 || shrink >= 1.0) {
        return 0.0;
    }
    let alpha: f64 = init_step;
    let s: usize = 0;
    while (s < max_steps) : (s += 1) {
        let i: usize = 0;
        while (i < x.len) : (i += 1) {
            x_trial[i] = x[i] + alpha * dir[i];
        }
        const f_try: f64 = f_fn(x_trial.*[0..x.len]);
        out_evals.* += 1;
        if (isFinite(f_try) && f_try <= f0 + c1 * alpha * g_dot_d) {
            return alpha;
        }
        alpha *= shrink;
    }
    return 0.0;
}

// ============================================================================
// 6. Polak-Ribiere+ Conjugate Gradient Optimizer (CG-PR+)
// ============================================================================

/// Returns the minimum `work.len` required by `minimizeConjugateGradient` for dimension `dim`.
pub fn cgWorkRequiredLen(dim: usize): usize {
    return 4 * dim;
}

/// Minimizes a smooth objective `f_fn` with gradient `grad_fn` using Polak-Ribiere+ Conjugate
/// Gradient (`beta = max(0, beta_PR)`) with automatic steepest-descent restart.
/// Requires `work.len >= cgWorkRequiredLen(x.len)`.
pub fn minimizeConjugateGradient(
    f_fn: OptObjectiveFn,
    grad_fn: OptGradientFn,
    x: *[]f64,
    tol_grad: f64,
    max_iters: usize,
    work: *[]f64,
): OptSummary ! OptError {
    const n: usize = x.len;
    if (n == 0) return OptError.InvalidDimension;
    if (!isFinite(tol_grad) || tol_grad <= 0.0 || max_iters == 0) {
        return OptError.InvalidTolerance;
    }
    if (work.len < cgWorkRequiredLen(n)) {
        return OptError.WorkBufferTooSmall;
    }

    let g: []f64 = work.*[0..n];
    let g_prev: []f64 = work.*[n..(2 * n)];
    let dir: []f64 = work.*[(2 * n)..(3 * n)];
    let x_trial: []f64 = work.*[(3 * n)..(4 * n)];

    let f_val: f64 = f_fn(x.*);
    if (!isFinite(f_val)) return OptError.NonFiniteValue;
    grad_fn(x.*, *g);
    let evals: usize = 1;

    let idx0: usize = 0;
    while (idx0 < n) : (idx0 += 1) {
        dir[idx0] = -g[idx0];
    }

    let g_norm: f64 = normInfSlice(g);
    if (g_norm <= tol_grad) {
        return OptSummary{
            status: OptStatus.converged,
            x_best: x[0],
            objective: f_val,
            grad_norm: g_norm,
            iterations: 0,
            func_evals: evals,
        };
    }

    let iter: usize = 0;
    while (iter < max_iters) : (iter += 1) {
        let g_dot_d: f64 = dotSlice(g, dir);
        if (g_dot_d >= 0.0) {
            let k: usize = 0;
            while (k < n) : (k += 1) {
                dir[k] = -g[k];
            }
            g_dot_d = dotSlice(g, dir);
        }

        const alpha: f64 = armijoBacktrackingLineSearch(
            f_fn,
            x.*,
            dir,
            f_val,
            g_dot_d,
            1.0,
            1.0e-4,
            0.5,
            40,
            *x_trial,
            *evals,
        );
        if (alpha == 0.0) {
            return OptSummary{
                status: OptStatus.stalled,
                x_best: x[0],
                objective: f_val,
                grad_norm: g_norm,
                iterations: iter,
                func_evals: evals,
            };
        }

        copySliceF64(*g_prev, g);
        copySliceF64(x, x_trial);
        f_val = f_fn(x.*);
        evals += 1;
        grad_fn(x.*, *g);

        g_norm = normInfSlice(g);
        if (g_norm <= tol_grad) {
            return OptSummary{
                status: OptStatus.converged,
                x_best: x[0],
                objective: f_val,
                grad_norm: g_norm,
                iterations: iter + 1,
                func_evals: evals,
            };
        }

        // Polak-Ribiere+ update: beta = max(0, g^T (g - g_prev) / ||g_prev||^2)
        const denom: f64 = dotSlice(g_prev, g_prev);
        let beta: f64 = 0.0;
        if (denom > 1.0e-30) {
            let num: f64 = 0.0;
            let k2: usize = 0;
            while (k2 < n) : (k2 += 1) {
                num += g[k2] * (g[k2] - g_prev[k2]);
            }
            beta = num / denom;
            if (beta < 0.0) beta = 0.0;
        }

        let k3: usize = 0;
        while (k3 < n) : (k3 += 1) {
            dir[k3] = -g[k3] + beta * dir[k3];
        }
    }

    return OptSummary{
        status: OptStatus.max_iters_reached,
        x_best: x[0],
        objective: f_val,
        grad_norm: g_norm,
        iterations: iter,
        func_evals: evals,
    };
}

// ============================================================================
// 7. Limited-Memory BFGS Optimizer (Nocedal Two-Loop Recursion)
// ============================================================================

/// Returns the minimum `work.len` required by `minimizeLbfgs` for dimension `dim` and history `history_m`.
pub fn lbfgsWorkRequiredLen(dim: usize, history_m: usize): usize {
    return (2 * history_m + 5) * dim + 2 * history_m;
}

/// Minimizes a smooth objective `f_fn` with gradient `grad_fn` using L-BFGS (Nocedal Two-Loop
/// Recursion) in O(history_m * n) memory with zero dynamic heap allocation.
pub fn minimizeLbfgs(
    f_fn: OptObjectiveFn,
    grad_fn: OptGradientFn,
    x: *[]f64,
    history_m: usize,
    tol_grad: f64,
    max_iters: usize,
    work: *[]f64,
): OptSummary ! OptError {
    const n: usize = x.len;
    if (n == 0 || history_m == 0) {
        return OptError.InvalidDimension;
    }
    if (!isFinite(tol_grad) || tol_grad <= 0.0 || max_iters == 0) {
        return OptError.InvalidTolerance;
    }
    if (work.len < lbfgsWorkRequiredLen(n, history_m)) {
        return OptError.WorkBufferTooSmall;
    }

    // Partition caller workspace cleanly.
    let g: []f64 = work.*[0..n];
    let g_prev: []f64 = work.*[n..(2 * n)];
    let q_dir: []f64 = work.*[(2 * n)..(3 * n)];
    let x_prev: []f64 = work.*[(3 * n)..(4 * n)];
    let x_trial: []f64 = work.*[(4 * n)..(5 * n)];
    const s_base: usize = n * 5;
    const y_base: usize = s_base + history_m * n;
    const rho_base: usize = y_base + history_m * n;
    const alpha_base: usize = rho_base + history_m;

    let f_val: f64 = f_fn(x.*);
    if (!isFinite(f_val)) return OptError.NonFiniteValue;
    grad_fn(x.*, *g);
    let evals: usize = 1;

    let g_norm: f64 = normInfSlice(g);
    if (g_norm <= tol_grad) {
        return OptSummary{
            status: OptStatus.converged,
            x_best: x[0],
            objective: f_val,
            grad_norm: g_norm,
            iterations: 0,
            func_evals: evals,
        };
    }

    let hist_count: usize = 0;
    let hist_head: usize = 0; // Next ring-buffer slot to write.

    let iter: usize = 0;
    while (iter < max_iters) : (iter += 1) {
        // Two-Loop Recursion: compute q_dir = -H_k * g
        copySliceF64(*q_dir, g);

        if (hist_count > 0) {
            // Backward loop: newest to oldest
            let step_b: usize = 0;
            while (step_b < hist_count) : (step_b += 1) {
                const slot: usize = (hist_head + history_m - 1 - step_b) % history_m;
                const s_off: usize = s_base + slot * n;
                const y_off: usize = y_base + slot * n;
                const s_dot_q: f64 = dotSlice(work.*[s_off..(s_off + n)], q_dir);
                const a_i: f64 = work.*[rho_base + slot] * s_dot_q;
                work[alpha_base + slot] = a_i;

                let k: usize = 0;
                while (k < n) : (k += 1) {
                    q_dir[k] -= a_i * work.*[y_off + k];
                }
            }

            // Initial Hessian scaling gamma = (s_{k-1}^T y_{k-1}) / (y_{k-1}^T y_{k-1})
            const newest: usize = (hist_head + history_m - 1) % history_m;
            const s_new_off: usize = s_base + newest * n;
            const y_new_off: usize = y_base + newest * n;
            const sy: f64 = dotSlice(work.*[s_new_off..(s_new_off + n)], work.*[y_new_off..(y_new_off + n)]);
            const yy: f64 = dotSlice(work.*[y_new_off..(y_new_off + n)], work.*[y_new_off..(y_new_off + n)]);
            const gamma: f64 = if (yy > 1.0e-30 && sy > 0.0) sy / yy else 1.0;

            let k_sc: usize = 0;
            while (k_sc < n) : (k_sc += 1) {
                q_dir[k_sc] *= gamma;
            }

            // Forward loop: oldest to newest
            let step_f: usize = 0;
            while (step_f < hist_count) : (step_f += 1) {
                const slot: usize = (hist_head + history_m - hist_count + step_f) % history_m;
                const s_off: usize = s_base + slot * n;
                const y_off: usize = y_base + slot * n;
                const y_dot_r: f64 = dotSlice(work.*[y_off..(y_off + n)], q_dir);
                const beta_i: f64 = work.*[rho_base + slot] * y_dot_r;
                const diff: f64 = work.*[alpha_base + slot] - beta_i;

                let k: usize = 0;
                while (k < n) : (k += 1) {
                    q_dir[k] += work.*[s_off + k] * diff;
                }
            }
        }

        // Negate to get descent direction.
        let kn: usize = 0;
        while (kn < n) : (kn += 1) {
            q_dir[kn] = -q_dir[kn];
        }

        let g_dot_d: f64 = dotSlice(g, q_dir);
        if (g_dot_d >= 0.0) {
            // Reset to steepest descent if non-descent direction occurs.
            let kr: usize = 0;
            while (kr < n) : (kr += 1) {
                q_dir[kr] = -g[kr];
            }
            g_dot_d = dotSlice(g, q_dir);
            hist_count = 0;
        }

        const init_alpha: f64 = if (hist_count == 0)
            (1.0 as f64) / (1.0 + norm2Slice(g))
        else
            1.0;

        const alpha: f64 = armijoBacktrackingLineSearch(
            f_fn,
            x.*,
            q_dir,
            f_val,
            g_dot_d,
            init_alpha,
            1.0e-4,
            0.5,
            40,
            *x_trial,
            *evals,
        );
        if (alpha == 0.0) {
            return OptSummary{
                status: OptStatus.stalled,
                x_best: x[0],
                objective: f_val,
                grad_norm: g_norm,
                iterations: iter,
                func_evals: evals,
            };
        }

        copySliceF64(*x_prev, x.*);
        copySliceF64(*g_prev, g);
        copySliceF64(x, x_trial);
        f_val = f_fn(x.*);
        evals += 1;
        grad_fn(x.*, *g);

        g_norm = normInfSlice(g);
        if (g_norm <= tol_grad) {
            return OptSummary{
                status: OptStatus.converged,
                x_best: x[0],
                objective: f_val,
                grad_norm: g_norm,
                iterations: iter + 1,
                func_evals: evals,
            };
        }

        // Store curvature pair (s_k, y_k) if positive curvature condition holds.
        const s_off: usize = s_base + hist_head * n;
        const y_off: usize = y_base + hist_head * n;
        let sy: f64 = 0.0;
        let ss: f64 = 0.0;
        let yy: f64 = 0.0;
        let ku: usize = 0;
        while (ku < n) : (ku += 1) {
            const sk: f64 = x[ku] - x_prev[ku];
            const yk: f64 = g[ku] - g_prev[ku];
            work[s_off + ku] = sk;
            work[y_off + ku] = yk;
            sy += sk * yk;
            ss += sk * sk;
            yy += yk * yk;
        }
        if (sy > 1.0e-14 * std.math.sqrt(ss * yy)) {
            work[rho_base + hist_head] = (1.0 as f64) / sy;
            hist_head = (hist_head + 1) % history_m;
            if (hist_count < history_m) {
                hist_count += 1;
            }
        }
    }

    return OptSummary{
        status: OptStatus.max_iters_reached,
        x_best: x[0],
        objective: f_val,
        grad_norm: g_norm,
        iterations: iter,
        func_evals: evals,
    };
}

// ============================================================================
// 8. Nelder-Mead Derivative-Free Simplex Optimizer
// ============================================================================

/// Returns the minimum `work.len` required by `minimizeNelderMead` for dimension `dim`.
pub fn nelderMeadWorkRequiredLen(dim: usize): usize {
    return (dim + 1) * dim + (dim + 1) + 4 * dim;
}

/// Minimizes a black-box (possibly non-smooth) objective `f_fn` using Nelder-Mead simplex search
/// (reflection, expansion, outside/inside contraction, and global shrink) with zero heap allocation.
pub fn minimizeNelderMead(
    f_fn: OptObjectiveFn,
    x: *[]f64,
    init_step: f64,
    tol_f: f64,
    max_iters: usize,
    work: *[]f64,
): OptSummary ! OptError {
    const n: usize = x.len;
    if (n == 0) return OptError.InvalidDimension;
    if (!isFinite(init_step) || init_step <= 0.0 || !isFinite(tol_f) || tol_f <= 0.0 || max_iters == 0) {
        return OptError.InvalidTolerance;
    }
    if (work.len < nelderMeadWorkRequiredLen(n)) {
        return OptError.WorkBufferTooSmall;
    }

    const nv: usize = n + 1;
    const simp_base: usize = 0;
    const fvals_base: usize = nv * n;
    const cent_base: usize = fvals_base + nv;
    const xr_base: usize = cent_base + n;
    const xe_base: usize = xr_base + n;
    const xc_base: usize = xe_base + n;

    // Initialize simplex: vertex 0 = x, vertex i+1 = x + init_step * e_i
    let evals: usize = 0;
    let v: usize = 0;
    while (v < nv) : (v += 1) {
        const v_off: usize = simp_base + v * n;
        let k: usize = 0;
        while (k < n) : (k += 1) {
            work[v_off + k] = x[k];
        }
        if (v > 0) {
            work[v_off + (v - 1)] += init_step;
        }
        const fv: f64 = f_fn(work.*[v_off..(v_off + n)]);
        evals += 1;
        if (!isFinite(fv)) return OptError.NonFiniteValue;
        work[fvals_base + v] = fv;
    }

    let iter: usize = 0;
    let best_idx: usize = 0;

    while (iter < max_iters) : (iter += 1) {
        // Identify best, worst, and second-worst vertices.
        best_idx = 0;
        let worst_idx: usize = 0;
        let v_i: usize = 1;
        while (v_i < nv) : (v_i += 1) {
            if (work.*[fvals_base + v_i] < work.*[fvals_base + best_idx]) {
                best_idx = v_i;
            }
            if (work.*[fvals_base + v_i] > work.*[fvals_base + worst_idx]) {
                worst_idx = v_i;
            }
        }
        let second_worst_idx: usize = best_idx;
        let v_j: usize = 0;
        while (v_j < nv) : (v_j += 1) {
            if (v_j == worst_idx) continue;
            if (work.*[fvals_base + v_j] > work.*[fvals_base + second_worst_idx]) {
                second_worst_idx = v_j;
            }
        }

        const f_best: f64 = work.*[fvals_base + best_idx];
        const f_worst: f64 = work.*[fvals_base + worst_idx];
        const f_second: f64 = work.*[fvals_base + second_worst_idx];
        const spread: f64 = f_worst - f_best;

        if (spread <= tol_f) {
            // Also check simplex spatial diameter to avoid premature stop on flat plateaus.
            let max_dist: f64 = 0.0;
            const b_off: usize = simp_base + best_idx * n;
            let vd: usize = 0;
            while (vd < nv) : (vd += 1) {
                if (vd == best_idx) continue;
                const d_off: usize = simp_base + vd * n;
                let k: usize = 0;
                while (k < n) : (k += 1) {
                    const diff: f64 = absF64(work.*[d_off + k] - work.*[b_off + k]);
                    if (diff > max_dist) max_dist = diff;
                }
            }
            if (max_dist <= std.math.sqrt(tol_f) || spread <= tol_f * 0.01) {
                let k_out: usize = 0;
                while (k_out < n) : (k_out += 1) {
                    x[k_out] = work.*[b_off + k_out];
                }
                return OptSummary{
                    status: OptStatus.converged,
                    x_best: x[0],
                    objective: f_best,
                    grad_norm: spread,
                    iterations: iter,
                    func_evals: evals,
                };
            }
        }

        // Compute centroid of all vertices except worst.
        const inv_n: f64 = (1.0 as f64) / @as(f64, @floatFromInt(n));
        let kc: usize = 0;
        while (kc < n) : (kc += 1) {
            let sum: f64 = 0.0;
            let vc: usize = 0;
            while (vc < nv) : (vc += 1) {
                if (vc == worst_idx) continue;
                sum += work.*[simp_base + vc * n + kc];
            }
            work[cent_base + kc] = sum * inv_n;
        }

        const w_off: usize = simp_base + worst_idx * n;

        // 1. Reflection: xr = centroid + 1.0 * (centroid - x_worst)
        let kr: usize = 0;
        while (kr < n) : (kr += 1) {
            work[xr_base + kr] = work.*[cent_base + kr] + (work.*[cent_base + kr] - work.*[w_off + kr]);
        }
        const f_r: f64 = f_fn(work.*[xr_base..(xr_base + n)]);
        evals += 1;

        if (f_r < f_best) {
            // 2. Expansion: xe = centroid + 2.0 * (xr - centroid)
            let ke: usize = 0;
            while (ke < n) : (ke += 1) {
                work[xe_base + ke] = work.*[cent_base + ke] + 2.0 * (work.*[xr_base + ke] - work.*[cent_base + ke]);
            }
            const f_e: f64 = f_fn(work.*[xe_base..(xe_base + n)]);
            evals += 1;
            if (f_e < f_r) {
                let k: usize = 0;
                while (k < n) : (k += 1) {
                    work[w_off + k] = work.*[xe_base + k];
                }
                work[fvals_base + worst_idx] = f_e;
            } else {
                let k: usize = 0;
                while (k < n) : (k += 1) {
                    work[w_off + k] = work.*[xr_base + k];
                }
                work[fvals_base + worst_idx] = f_r;
            }
        } else if (f_r < f_second) {
            let k: usize = 0;
            while (k < n) : (k += 1) {
                work[w_off + k] = work.*[xr_base + k];
            }
            work[fvals_base + worst_idx] = f_r;
        } else {
            // 3. Contraction (outside if f_r < f_worst, else inside).
            let contracted: bool = false;
            if (f_r < f_worst) {
                let kco: usize = 0;
                while (kco < n) : (kco += 1) {
                    work[xc_base + kco] = work.*[cent_base + kco] + 0.5 * (work.*[xr_base + kco] - work.*[cent_base + kco]);
                }
                const f_c: f64 = f_fn(work.*[xc_base..(xc_base + n)]);
                evals += 1;
                if (f_c <= f_r) {
                    let k: usize = 0;
                    while (k < n) : (k += 1) {
                        work[w_off + k] = work.*[xc_base + k];
                    }
                    work[fvals_base + worst_idx] = f_c;
                    contracted = true;
                }
            } else {
                let kci: usize = 0;
                while (kci < n) : (kci += 1) {
                    work[xc_base + kci] = work.*[cent_base + kci] + 0.5 * (work.*[w_off + kci] - work.*[cent_base + kci]);
                }
                const f_c: f64 = f_fn(work.*[xc_base..(xc_base + n)]);
                evals += 1;
                if (f_c < f_worst) {
                    let k: usize = 0;
                    while (k < n) : (k += 1) {
                        work[w_off + k] = work.*[xc_base + k];
                    }
                    work[fvals_base + worst_idx] = f_c;
                    contracted = true;
                }
            }

            // 4. Global shrink toward best vertex if contraction failed.
            if (!contracted) {
                const b_off: usize = simp_base + best_idx * n;
                let vs: usize = 0;
                while (vs < nv) : (vs += 1) {
                    if (vs == best_idx) continue;
                    const s_off: usize = simp_base + vs * n;
                    let k: usize = 0;
                    while (k < n) : (k += 1) {
                        work[s_off + k] = work.*[b_off + k] + 0.5 * (work.*[s_off + k] - work.*[b_off + k]);
                    }
                    work[fvals_base + vs] = f_fn(work.*[s_off..(s_off + n)]);
                    evals += 1;
                }
            }
        }
    }

    // Return best vertex found on max_iters.
    const final_b_off: usize = simp_base + best_idx * n;
    let k_fin: usize = 0;
    while (k_fin < n) : (k_fin += 1) {
        x[k_fin] = work.*[final_b_off + k_fin];
    }
    return OptSummary{
        status: OptStatus.max_iters_reached,
        x_best: x[0],
        objective: work.*[fvals_base + best_idx],
        grad_norm: 0.0,
        iterations: iter,
        func_evals: evals,
    };
}
