// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Two-Phase Simplex LP Solver with Bland's Anti-Cycling Rule & Dual Shadow Prices)

const PIVOT_EPS: f64 = 1.0e-10;
const OPT_EPS: f64 = 1.0e-9;
const FEAS_EPS: f64 = 1.0e-7;
const TIE_EPS: f64 = 1.0e-12;

/// Linear constraint relation type: `a_i^T x <= b_i`, `>= b_i`, or `= b_i`.
pub enum ConstraintKind {
    Le,
    Ge,
    Eq,
}

/// Termination status of the Two-Phase Simplex solver.
pub enum SimplexStatus {
    Optimal,
    Unbounded,
    Infeasible,
    MaxIterations,
    InvalidInput,
}

/// Summary result returned by Simplex solvers.
pub struct SimplexResult {
    status: SimplexStatus,
    objective: f64,
    iterations: u32,
}

/// Caller-owned zero-allocation workspace for Two-Phase Simplex.
/// - `tableau`: length `>= (m + 1) * (n + n_slack + n_art + 1)` (worst-case `(m + 1) * (n + 2 * m + 1)`)
/// - `basis`: length `>= m`
pub struct SimplexWorkspace {
    tableau: []f64,
    basis: []usize,
}

/// Returns the worst-case tableau length `(m + 1) * (n + 2 * m + 1)` sufficient for any
/// mixed `Le`/`Ge`/`Eq` LP with `num_vars` decision variables and `num_constraints` constraints.
pub fn simplexRequiredTableauLen(num_vars: usize, num_constraints: usize): usize {
    const rows: usize = num_constraints + 1;
    const cols: usize = num_vars + (2 as usize) * num_constraints + 1;
    return rows * cols;
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn checkMatrixLen(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn checkFinitePrefix(data: []const f64, count: usize): bool {
    let i: usize = 0;
    while (i < count) {
        if (!isFiniteF64(data[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn makeResult(status: SimplexStatus, obj: f64, iters: u32): SimplexResult {
    return SimplexResult {
        status: status,
        objective: obj,
        iterations: iters,
    };
}

fn pivotTableau(
    tab: *[]f64,
    m: usize,
    ncols: usize,
    leave_row: usize,
    enter_col: usize,
) {
    const piv_base: usize = leave_row * ncols;
    const piv_val: f64 = tab.*[piv_base + enter_col];
    const inv_piv: f64 = 1.0 / piv_val;

    let j: usize = 0;
    while (j < ncols) {
        tab.*[piv_base + j] *= inv_piv;
        j += 1;
    }
    tab.*[piv_base + enter_col] = 1.0;

    let r: usize = 0;
    while (r <= m) {
        if (r != leave_row) {
            const r_base: usize = r * ncols;
            const factor: f64 = tab.*[r_base + enter_col];
            if (absF64(factor) > 0.0) {
                let c: usize = 0;
                while (c < ncols) {
                    tab.*[r_base + c] -= factor * tab.*[piv_base + c];
                    c += 1;
                }
                tab.*[r_base + enter_col] = 0.0;
            }
        }
        r += 1;
    }
}

/// Runs simplex iterations with Bland's anti-cycling rule.
/// Returns: 0 = Optimal, 1 = Unbounded, 2 = MaxIterations.
fn runSimplexLoop(
    tab: *[]f64,
    basis: *[]usize,
    m: usize,
    ncols: usize,
    active_cols: usize,
    max_iter: u32,
    iters: *u32,
): u32 {
    const rhs_col: usize = ncols - 1;
    const obj_base: usize = m * ncols;

    while (true) {
        // Bland's rule: choose smallest column index j < active_cols with reduced cost < -OPT_EPS
        let enter_col: usize = active_cols;
        let j: usize = 0;
        while (j < active_cols) {
            if (tab.*[obj_base + j] < (0.0 - OPT_EPS)) {
                enter_col = j;
                break;
            }
            j += 1;
        }
        if (enter_col == active_cols) {
            return 0;
        }

        if (iters.* >= max_iter) {
            return 2;
        }

        // Minimum ratio test with Bland's tie-breaking (smallest basis[i])
        let leave_row: usize = m;
        let best_ratio: f64 = 0.0;
        let has_leave: bool = false;

        let i: usize = 0;
        while (i < m) {
            const a_ij: f64 = tab.*[i * ncols + enter_col];
            if (a_ij > PIVOT_EPS) {
                let rhs_i: f64 = tab.*[i * ncols + rhs_col];
                if (rhs_i < 0.0 && rhs_i > (0.0 - FEAS_EPS)) {
                    rhs_i = 0.0;
                }
                const ratio: f64 = rhs_i / a_ij;
                if (!has_leave) {
                    leave_row = i;
                    best_ratio = ratio;
                    has_leave = true;
                } else {
                    const tol: f64 = TIE_EPS * (1.0 + absF64(best_ratio));
                    if (ratio < best_ratio - tol) {
                        leave_row = i;
                        best_ratio = ratio;
                    } else if (absF64(ratio - best_ratio) <= tol) {
                        if (basis.*[i] < basis.*[leave_row]) {
                            leave_row = i;
                            best_ratio = ratio;
                        }
                    }
                }
            }
            i += 1;
        }

        if (!has_leave) {
            return 1;
        }

        pivotTableau(tab, m, ncols, leave_row, enter_col);
        basis.*[leave_row] = enter_col;
        iters.* += 1;
    }
    return 0;
}

fn solveInternal(
    c: []const f64,
    a: []const f64,
    b: []const f64,
    kinds: []const ConstraintKind,
    use_default_le: bool,
    num_vars: usize,
    num_constraints: usize,
    is_max: bool,
    max_iter: u32,
    x_out: *[]f64,
    dual_out: *[]f64,
    ws: *SimplexWorkspace,
): SimplexResult {
    const n: usize = num_vars;
    const m: usize = num_constraints;

    if (n == 0 || m == 0) {
        return makeResult(SimplexStatus.InvalidInput, 0.0, 0);
    }
    if (c.len < n || b.len < m || !checkMatrixLen(a.len, m, n)) {
        return makeResult(SimplexStatus.InvalidInput, 0.0, 0);
    }
    if (!use_default_le && kinds.len < m) {
        return makeResult(SimplexStatus.InvalidInput, 0.0, 0);
    }
    if (x_out.*.len < n || ws.*.basis.len < m) {
        return makeResult(SimplexStatus.InvalidInput, 0.0, 0);
    }
    const want_dual: bool = dual_out.*.len > 0;
    if (want_dual && dual_out.*.len < m) {
        return makeResult(SimplexStatus.InvalidInput, 0.0, 0);
    }
    if (!checkFinitePrefix(c, n) || !checkFinitePrefix(b, m) || !checkFinitePrefix(a, m * n)) {
        return makeResult(SimplexStatus.InvalidInput, 0.0, 0);
    }

    // Pass 1: count slack/surplus and artificial columns after RHS >= 0 normalization
    let n_slack: usize = 0;
    let n_art: usize = 0;
    let i_cnt: usize = 0;
    while (i_cnt < m) {
        const raw_kind: ConstraintKind = if (use_default_le) ConstraintKind.Le else kinds[i_cnt];
        const flipped: bool = b[i_cnt] < 0.0;
        const norm_kind: ConstraintKind = if (!flipped) raw_kind else match (raw_kind) {
            .Le => ConstraintKind.Ge,
            .Ge => ConstraintKind.Le,
            .Eq => ConstraintKind.Eq,
        };
        match (norm_kind) {
            .Le => {
                n_slack += 1;
            },
            .Ge => {
                n_slack += 1;
                n_art += 1;
            },
            .Eq => {
                n_art += 1;
            },
        }
        i_cnt += 1;
    }

    const art_start: usize = n + n_slack;
    const total_vars: usize = art_start + n_art;
    const ncols: usize = total_vars + 1;
    const rhs_col: usize = total_vars;
    const needed_tab: usize = (m + 1) * ncols;
    if (ws.*.tableau.len < needed_tab) {
        return makeResult(SimplexStatus.InvalidInput, 0.0, 0);
    }

    // Zero tableau
    let z: usize = 0;
    while (z < needed_tab) {
        ws.*.tableau[z] = 0.0;
        z += 1;
    }

    // Pass 2: populate normalized constraint rows and initial basis
    let cur_slack: usize = n;
    let cur_art: usize = art_start;
    let i_fill: usize = 0;
    while (i_fill < m) {
        const raw_kind: ConstraintKind = if (use_default_le) ConstraintKind.Le else kinds[i_fill];
        const flipped: bool = b[i_fill] < 0.0;
        const sign: f64 = if (flipped) -1.0 else 1.0;
        const norm_kind: ConstraintKind = if (!flipped) raw_kind else match (raw_kind) {
            .Le => ConstraintKind.Ge,
            .Ge => ConstraintKind.Le,
            .Eq => ConstraintKind.Eq,
        };

        const r_base: usize = i_fill * ncols;
        let j: usize = 0;
        while (j < n) {
            ws.*.tableau[r_base + j] = sign * a[i_fill * n + j];
            j += 1;
        }
        ws.*.tableau[r_base + rhs_col] = sign * b[i_fill];

        match (norm_kind) {
            .Le => {
                ws.*.tableau[r_base + cur_slack] = 1.0;
                ws.*.basis[i_fill] = cur_slack;
                cur_slack += 1;
            },
            .Ge => {
                ws.*.tableau[r_base + cur_slack] = -1.0;
                ws.*.tableau[r_base + cur_art] = 1.0;
                ws.*.basis[i_fill] = cur_art;
                cur_slack += 1;
                cur_art += 1;
            },
            .Eq => {
                ws.*.tableau[r_base + cur_art] = 1.0;
                ws.*.basis[i_fill] = cur_art;
                cur_art += 1;
            },
        }
        i_fill += 1;
    }

    let tab_slice: []f64 = ws.*.tableau[0..needed_tab];
    let basis_slice: []usize = ws.*.basis[0..m];
    let iters: u32 = 0;
    const obj_base: usize = m * ncols;

    // Phase I: minimize sum of artificial variables (maximize -sum(r_k))
    if (n_art > 0) {
        let j_art: usize = art_start;
        while (j_art < total_vars) {
            tab_slice[obj_base + j_art] = 1.0;
            j_art += 1;
        }

        // Eliminate basic artificial variables from Phase-I objective row
        let r_elim: usize = 0;
        while (r_elim < m) {
            if (basis_slice[r_elim] >= art_start) {
                const r_base: usize = r_elim * ncols;
                let col: usize = 0;
                while (col < ncols) {
                    tab_slice[obj_base + col] -= tab_slice[r_base + col];
                    col += 1;
                }
            }
            r_elim += 1;
        }

        const p1_code: u32 = runSimplexLoop(*tab_slice, *basis_slice, m, ncols, total_vars, max_iter, *iters);
        if (p1_code == 2) {
            return makeResult(SimplexStatus.MaxIterations, 0.0, iters);
        }
        if (p1_code == 1) {
            return makeResult(SimplexStatus.Infeasible, 0.0, iters);
        }

        const p1_obj: f64 = tab_slice[obj_base + rhs_col];
        if (p1_obj < (0.0 - FEAS_EPS)) {
            return makeResult(SimplexStatus.Infeasible, 0.0, iters);
        }

        // Drive degenerate artificial variables out of the basis when possible
        let r_clean: usize = 0;
        while (r_clean < m) {
            if (basis_slice[r_clean] >= art_start) {
                const r_base: usize = r_clean * ncols;
                let pivot_col: usize = art_start;
                let best_abs: f64 = PIVOT_EPS;
                let c_try: usize = 0;
                while (c_try < art_start) {
                    const av: f64 = absF64(tab_slice[r_base + c_try]);
                    if (av > best_abs) {
                        best_abs = av;
                        pivot_col = c_try;
                    }
                    c_try += 1;
                }
                if (pivot_col < art_start) {
                    pivotTableau(*tab_slice, m, ncols, r_clean, pivot_col);
                    basis_slice[r_clean] = pivot_col;
                }
            }
            r_clean += 1;
        }
    }

    // Phase II: install true objective row and canonicalize against current basis
    let j_clr: usize = 0;
    while (j_clr < ncols) {
        tab_slice[obj_base + j_clr] = 0.0;
        j_clr += 1;
    }
    const obj_sign: f64 = if (is_max) 1.0 else -1.0;
    let j_obj: usize = 0;
    while (j_obj < n) {
        tab_slice[obj_base + j_obj] = 0.0 - (obj_sign * c[j_obj]);
        j_obj += 1;
    }

    let r_can: usize = 0;
    while (r_can < m) {
        const bcol: usize = basis_slice[r_can];
        if (bcol < n) {
            const factor: f64 = tab_slice[obj_base + bcol];
            if (absF64(factor) > 0.0) {
                const r_base: usize = r_can * ncols;
                let col: usize = 0;
                while (col < ncols) {
                    tab_slice[obj_base + col] -= factor * tab_slice[r_base + col];
                    col += 1;
                }
                tab_slice[obj_base + bcol] = 0.0;
            }
        }
        r_can += 1;
    }

    const p2_code: u32 = runSimplexLoop(*tab_slice, *basis_slice, m, ncols, art_start, max_iter, *iters);
    if (p2_code == 1) {
        return makeResult(SimplexStatus.Unbounded, 0.0, iters);
    }
    if (p2_code == 2) {
        return makeResult(SimplexStatus.MaxIterations, 0.0, iters);
    }

    // Extract primal solution x_out[0..n]
    let ix: usize = 0;
    while (ix < n) {
        x_out.*[ix] = 0.0;
        ix += 1;
    }
    let r_ext: usize = 0;
    while (r_ext < m) {
        const bcol: usize = basis_slice[r_ext];
        if (bcol < n) {
            let val: f64 = tab_slice[r_ext * ncols + rhs_col];
            if (val < 0.0 && val > (0.0 - FEAS_EPS)) {
                val = 0.0;
            }
            x_out.*[bcol] = val;
        }
        r_ext += 1;
    }

    let true_obj: f64 = 0.0;
    let j_dot: usize = 0;
    while (j_dot < n) {
        true_obj += c[j_dot] * x_out.*[j_dot];
        j_dot += 1;
    }

    // Extract dual shadow prices y_i from initial unit columns (satisfies c^T x* == b^T y*)
    if (want_dual) {
        let d_slack: usize = n;
        let d_art: usize = art_start;
        let i_d: usize = 0;
        while (i_d < m) {
            const raw_kind: ConstraintKind = if (use_default_le) ConstraintKind.Le else kinds[i_d];
            const flipped: bool = b[i_d] < 0.0;
            const norm_kind: ConstraintKind = if (!flipped) raw_kind else match (raw_kind) {
                .Le => ConstraintKind.Ge,
                .Ge => ConstraintKind.Le,
                .Eq => ConstraintKind.Eq,
            };

            let unit_col: usize = 0;
            match (norm_kind) {
                .Le => {
                    unit_col = d_slack;
                    d_slack += 1;
                },
                .Ge => {
                    unit_col = d_art;
                    d_slack += 1;
                    d_art += 1;
                },
                .Eq => {
                    unit_col = d_art;
                    d_art += 1;
                },
            }

            let y_i: f64 = tab_slice[obj_base + unit_col];
            if (flipped) {
                y_i = 0.0 - y_i;
            }
            if (!is_max) {
                y_i = 0.0 - y_i;
            }
            dual_out.*[i_d] = y_i;
            i_d += 1;
        }
    }

    return makeResult(SimplexStatus.Optimal, true_obj, iters);
}

/// Solves a general linear program with mixed `Le (<=)`, `Ge (>=)`, and `Eq (=)` constraints:
/// optimize `c^T x` (max if `is_max`, min otherwise) subject to `A x <rel> b, x >= 0`.
/// Pass an empty slice for `dual_out` if dual shadow prices are not needed.
pub fn solveGeneral(
    c: []const f64,
    a: []const f64,
    b: []const f64,
    kinds: []const ConstraintKind,
    num_vars: usize,
    num_constraints: usize,
    is_max: bool,
    max_iter: u32,
    x_out: *[]f64,
    dual_out: *[]f64,
    ws: *SimplexWorkspace,
): SimplexResult {
    return solveInternal(
        c, a, b, kinds, false, num_vars, num_constraints, is_max, max_iter, x_out, dual_out, ws,
    );
}

/// Convenience solver for `max c^T x` subject to `A x <= b, x >= 0` (supports negative `b_i` via Phase I).
pub fn solveMaxLe(
    c: []const f64,
    a: []const f64,
    b: []const f64,
    num_vars: usize,
    num_constraints: usize,
    max_iter: u32,
    x_out: *[]f64,
    dual_out: *[]f64,
    ws: *SimplexWorkspace,
): SimplexResult {
    const empty_kinds: [0]ConstraintKind = [];
    return solveInternal(
        c, a, b, empty_kinds[0..], true, num_vars, num_constraints, true, max_iter, x_out, dual_out, ws,
    );
}

/// Convenience solver for `min c^T x` subject to `A x <= b, x >= 0` (supports negative `b_i` via Phase I).
pub fn solveMinLe(
    c: []const f64,
    a: []const f64,
    b: []const f64,
    num_vars: usize,
    num_constraints: usize,
    max_iter: u32,
    x_out: *[]f64,
    dual_out: *[]f64,
    ws: *SimplexWorkspace,
): SimplexResult {
    const empty_kinds: [0]ConstraintKind = [];
    return solveInternal(
        c, a, b, empty_kinds[0..], true, num_vars, num_constraints, false, max_iter, x_out, dual_out, ws,
    );
}
