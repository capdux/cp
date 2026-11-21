// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc O(n^3) Jonker-Volgenant / Kuhn-Munkres Hungarian Linear Assignment Solver)

const INF_I64: i64 = 4000000000000000000;
const MAX_ABS_I64: i64 = 1000000000000000;
const INF_F64: f64 = 1.0e300;

/// Result returned by `solveMinF64` and `solveMaxF64`.
pub struct AssignmentResultF64 {
    totalCost: f64,
    matchedCount: usize,
}

/// Default floating-point assignment result alias.
pub type AssignmentResult = AssignmentResultF64;

/// Result returned by `solveMinI64` and `solveMaxI64`.
pub struct AssignmentResultI64 {
    totalCost: i64,
    matchedCount: usize,
}

/// Caller-owned workspace for `f64` Hungarian solver.
/// Let `r = min(rows, cols)` and `c = max(rows, cols)`.
/// Required lengths: `u.len >= r + 1`, and `v`, `minv`, `p`, `way`, `used` each `>= c + 1`.
pub struct HungarianWorkspaceF64 {
    u: []f64,
    v: []f64,
    minv: []f64,
    p: []usize,
    way: []usize,
    used: []u8,
}

/// Default floating-point workspace alias.
pub type HungarianWorkspace = HungarianWorkspaceF64;

/// Caller-owned workspace for `i64` Hungarian solver.
/// Let `r = min(rows, cols)` and `c = max(rows, cols)`.
/// Required lengths: `u.len >= r + 1`, and `v`, `minv`, `p`, `way`, `used` each `>= c + 1`.
pub struct HungarianWorkspaceI64 {
    u: []i64,
    v: []i64,
    minv: []i64,
    p: []usize,
    way: []usize,
    used: []u8,
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn checkCostMatrixLen(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn checkWsF64(ws: *HungarianWorkspaceF64, r: usize, c: usize): bool {
    return ws.*.u.len >= r + 1
        && ws.*.v.len >= c + 1
        && ws.*.minv.len >= c + 1
        && ws.*.p.len >= c + 1
        && ws.*.way.len >= c + 1
        && ws.*.used.len >= c + 1;
}

fn checkWsI64(ws: *HungarianWorkspaceI64, r: usize, c: usize): bool {
    return ws.*.u.len >= r + 1
        && ws.*.v.len >= c + 1
        && ws.*.minv.len >= c + 1
        && ws.*.p.len >= c + 1
        && ws.*.way.len >= c + 1
        && ws.*.used.len >= c + 1;
}

fn getEffCostF64(
    cost: []const f64,
    cols: usize,
    transposed: bool,
    negate: bool,
    row1: usize,
    col1: usize,
): f64 {
    const raw: f64 = if (!transposed)
        cost[(row1 - 1) * cols + (col1 - 1)]
    else
        cost[(col1 - 1) * cols + (row1 - 1)];
    return if (negate) (0.0 - raw) else raw;
}

fn getEffCostI64(
    cost: []const i64,
    cols: usize,
    transposed: bool,
    negate: bool,
    row1: usize,
    col1: usize,
): i64 {
    const raw: i64 = if (!transposed)
        cost[(row1 - 1) * cols + (col1 - 1)]
    else
        cost[(col1 - 1) * cols + (row1 - 1)];
    return if (negate) (0 - raw) else raw;
}

fn solveF64Core(
    cost: []const f64,
    rows: usize,
    cols: usize,
    negate: bool,
    rowToCol: *[]i32,
    ws: *HungarianWorkspaceF64,
): AssignmentResultF64? {
    if (!checkCostMatrixLen(cost.len, rows, cols) || rowToCol.*.len < rows) {
        return null;
    }
    const total_cells: usize = rows * cols;
    let chk: usize = 0;
    while (chk < total_cells) {
        if (!isFiniteF64(cost[chk])) {
            return null;
        }
        chk += 1;
    }

    const transposed: bool = rows > cols;
    const r: usize = if (transposed) cols else rows;
    const c: usize = if (transposed) rows else cols;
    if (!checkWsF64(ws, r, c)) {
        return null;
    }

    let iu: usize = 0;
    while (iu <= r) {
        ws.*.u[iu] = 0.0;
        iu += 1;
    }
    let iv: usize = 0;
    while (iv <= c) {
        ws.*.v[iv] = 0.0;
        ws.*.p[iv] = 0;
        ws.*.way[iv] = 0;
        iv += 1;
    }

    let i: usize = 1;
    while (i <= r) {
        ws.*.p[0] = i;
        let j0: usize = 0;
        let init_j: usize = 0;
        while (init_j <= c) {
            ws.*.minv[init_j] = INF_F64;
            ws.*.used[init_j] = 0;
            init_j += 1;
        }

        while (true) {
            ws.*.used[j0] = 1;
            const row0: usize = ws.*.p[j0];
            let delta: f64 = INF_F64;
            let has_delta: bool = false;
            let j1: usize = 0;

            let j: usize = 1;
            while (j <= c) {
                if (ws.*.used[j] == 0) {
                    const eff: f64 = getEffCostF64(cost, cols, transposed, negate, row0, j);
                    const cur: f64 = eff - ws.*.u[row0] - ws.*.v[j];
                    if (cur < ws.*.minv[j]) {
                        ws.*.minv[j] = cur;
                        ws.*.way[j] = j0;
                    }
                    if (!has_delta || ws.*.minv[j] < delta) {
                        delta = ws.*.minv[j];
                        j1 = j;
                        has_delta = true;
                    }
                }
                j += 1;
            }

            let ju: usize = 0;
            while (ju <= c) {
                if (ws.*.used[ju] != 0) {
                    ws.*.u[ws.*.p[ju]] += delta;
                    ws.*.v[ju] -= delta;
                } else {
                    ws.*.minv[ju] -= delta;
                }
                ju += 1;
            }

            j0 = j1;
            if (ws.*.p[j0] == 0) {
                break;
            }
        }

        while (true) {
            const prev_j: usize = ws.*.way[j0];
            ws.*.p[j0] = ws.*.p[prev_j];
            j0 = prev_j;
            if (j0 == 0) {
                break;
            }
        }
        i += 1;
    }

    let ri: usize = 0;
    while (ri < rows) {
        rowToCol.*[ri] = -1;
        ri += 1;
    }

    let total_cost: f64 = 0.0;
    let matched: usize = 0;
    let cj: usize = 1;
    while (cj <= c) {
        const pi: usize = ws.*.p[cj];
        if (pi != 0) {
            if (!transposed) {
                const orig_r: usize = pi - 1;
                const orig_c: usize = cj - 1;
                rowToCol.*[orig_r] = orig_c as i32;
                total_cost += cost[orig_r * cols + orig_c];
            } else {
                const orig_r: usize = cj - 1;
                const orig_c: usize = pi - 1;
                rowToCol.*[orig_r] = orig_c as i32;
                total_cost += cost[orig_r * cols + orig_c];
            }
            matched += 1;
        }
        cj += 1;
    }

    return AssignmentResultF64 {
        totalCost: total_cost,
        matchedCount: matched,
    };
}

fn solveI64Core(
    cost: []const i64,
    rows: usize,
    cols: usize,
    negate: bool,
    rowToCol: *[]i32,
    ws: *HungarianWorkspaceI64,
): AssignmentResultI64? {
    if (!checkCostMatrixLen(cost.len, rows, cols) || rowToCol.*.len < rows) {
        return null;
    }
    const total_cells: usize = rows * cols;
    let chk: usize = 0;
    while (chk < total_cells) {
        const v: i64 = cost[chk];
        if (v < (0 - MAX_ABS_I64) || v > MAX_ABS_I64) {
            return null;
        }
        chk += 1;
    }

    const transposed: bool = rows > cols;
    const r: usize = if (transposed) cols else rows;
    const c: usize = if (transposed) rows else cols;
    if (!checkWsI64(ws, r, c)) {
        return null;
    }

    let iu: usize = 0;
    while (iu <= r) {
        ws.*.u[iu] = 0;
        iu += 1;
    }
    let iv: usize = 0;
    while (iv <= c) {
        ws.*.v[iv] = 0;
        ws.*.p[iv] = 0;
        ws.*.way[iv] = 0;
        iv += 1;
    }

    let i: usize = 1;
    while (i <= r) {
        ws.*.p[0] = i;
        let j0: usize = 0;
        let init_j: usize = 0;
        while (init_j <= c) {
            ws.*.minv[init_j] = INF_I64;
            ws.*.used[init_j] = 0;
            init_j += 1;
        }

        while (true) {
            ws.*.used[j0] = 1;
            const row0: usize = ws.*.p[j0];
            let delta: i64 = INF_I64;
            let has_delta: bool = false;
            let j1: usize = 0;

            let j: usize = 1;
            while (j <= c) {
                if (ws.*.used[j] == 0) {
                    const eff: i64 = getEffCostI64(cost, cols, transposed, negate, row0, j);
                    const cur: i64 = eff - ws.*.u[row0] - ws.*.v[j];
                    if (cur < ws.*.minv[j]) {
                        ws.*.minv[j] = cur;
                        ws.*.way[j] = j0;
                    }
                    if (!has_delta || ws.*.minv[j] < delta) {
                        delta = ws.*.minv[j];
                        j1 = j;
                        has_delta = true;
                    }
                }
                j += 1;
            }

            let ju: usize = 0;
            while (ju <= c) {
                if (ws.*.used[ju] != 0) {
                    ws.*.u[ws.*.p[ju]] += delta;
                    ws.*.v[ju] -= delta;
                } else {
                    ws.*.minv[ju] -= delta;
                }
                ju += 1;
            }

            j0 = j1;
            if (ws.*.p[j0] == 0) {
                break;
            }
        }

        while (true) {
            const prev_j: usize = ws.*.way[j0];
            ws.*.p[j0] = ws.*.p[prev_j];
            j0 = prev_j;
            if (j0 == 0) {
                break;
            }
        }
        i += 1;
    }

    let ri: usize = 0;
    while (ri < rows) {
        rowToCol.*[ri] = -1;
        ri += 1;
    }

    let total_cost: i64 = 0;
    let matched: usize = 0;
    let cj: usize = 1;
    while (cj <= c) {
        const pi: usize = ws.*.p[cj];
        if (pi != 0) {
            if (!transposed) {
                const orig_r: usize = pi - 1;
                const orig_c: usize = cj - 1;
                rowToCol.*[orig_r] = orig_c as i32;
                total_cost += cost[orig_r * cols + orig_c];
            } else {
                const orig_r: usize = cj - 1;
                const orig_c: usize = pi - 1;
                rowToCol.*[orig_r] = orig_c as i32;
                total_cost += cost[orig_r * cols + orig_c];
            }
            matched += 1;
        }
        cj += 1;
    }

    return AssignmentResultI64 {
        totalCost: total_cost,
        matchedCount: matched,
    };
}

/// Solves minimum-weight linear assignment for a row-major `rows x cols` `f64` cost matrix.
/// Supports square and arbitrary rectangular matrices (`rows <= cols` and `rows > cols`).
/// Unmatched rows (when `rows > cols`) receive `-1` in `rowToCol`.
/// Final dual potentials remain in `ws.u[1..=min(rows,cols)]` and `ws.v[1..=max(rows,cols)]`.
pub fn solveMinF64(
    cost: []const f64,
    rows: usize,
    cols: usize,
    rowToCol: *[]i32,
    ws: *HungarianWorkspaceF64,
): AssignmentResultF64? {
    return solveF64Core(cost, rows, cols, false, rowToCol, ws);
}

/// Solves maximum-weight linear assignment for a row-major `rows x cols` `f64` weight matrix.
pub fn solveMaxF64(
    weight: []const f64,
    rows: usize,
    cols: usize,
    rowToCol: *[]i32,
    ws: *HungarianWorkspaceF64,
): AssignmentResultF64? {
    return solveF64Core(weight, rows, cols, true, rowToCol, ws);
}

/// Solves minimum-weight linear assignment for a row-major `rows x cols` `i64` cost matrix.
/// Entries must satisfy `|cost[i]| <= 1_000_000_000_000_000` to prevent potential overflow.
pub fn solveMinI64(
    cost: []const i64,
    rows: usize,
    cols: usize,
    rowToCol: *[]i32,
    ws: *HungarianWorkspaceI64,
): AssignmentResultI64? {
    return solveI64Core(cost, rows, cols, false, rowToCol, ws);
}

/// Solves maximum-weight linear assignment for a row-major `rows x cols` `i64` weight matrix.
pub fn solveMaxI64(
    weight: []const i64,
    rows: usize,
    cols: usize,
    rowToCol: *[]i32,
    ws: *HungarianWorkspaceI64,
): AssignmentResultI64? {
    return solveI64Core(weight, rows, cols, true, rowToCol, ws);
}
