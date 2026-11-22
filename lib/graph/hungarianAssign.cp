// Origin: Pure-CP Stdlib (Zero-Alloc Hungarian / Kuhn-Munkres Optimal Assignment Solver) | Created: 2026-11-22
// Pure CP zero-heap-allocation O(N^3) Hungarian (Kuhn-Munkres) bipartite assignment solver:
//   - Supports square & rectangular `i64` matrices up to `32 x 32` (`HUNGARIAN_MAX_DIM = 32`)
//   - Iterative potential-function (`u[i] + v[j] <= cost[i][j]`) augmenting-path engine using `i128`
//     internal arithmetic to eliminate overflow even with large/negative `i64` costs
//   - Supports forbidden edge sentinel (`HUNGARIAN_FORBIDDEN_COST`) with exact infeasibility detection
//   - Provides both `solveMinCostAssignment` and `solveMaxProfitAssignment` returning `row_to_col` & `col_to_row`

// ============================================================================
// 1. Constants, Enums & Result Models
// ============================================================================

/// Maximum rows or columns supported inline by the zero-alloc Hungarian solver.
pub const HUNGARIAN_MAX_DIM: usize = 32;

/// Sentinel index indicating a row or column was not assigned in a rectangular match.
pub const HUNGARIAN_UNASSIGNED: u16 = 0xFFFF;

/// Cost/profit sentinel marking a forbidden edge that cannot be chosen in a feasible assignment.
pub const HUNGARIAN_FORBIDDEN_COST: i64 = 0x7FFFFFFFFFFFFFFF;

/// Result status returned by `solveMinCostAssignment` and `solveMaxProfitAssignment`.
pub enum HungarianStatus {
    ok,
    invalid_dimensions,
    matrix_length_mismatch,
    infeasible,
    overflow,
}

/// Complete optimal assignment output returned by the Hungarian solver.
pub struct HungarianAssignment {
    status: HungarianStatus = HungarianStatus.ok;
    rows: usize = 0;
    cols: usize = 0;
    matched_count: usize = 0;
    total_cost: i64 = 0;
    row_to_col: [HUNGARIAN_MAX_DIM]u16 = [HUNGARIAN_UNASSIGNED; HUNGARIAN_MAX_DIM];
    col_to_row: [HUNGARIAN_MAX_DIM]u16 = [HUNGARIAN_UNASSIGNED; HUNGARIAN_MAX_DIM];

    /// Returns the assigned column index for `row`, or `null` if unassigned / out of range.
    fn colForRow(row: usize): u16? {
        if (row >= _.rows || _.row_to_col[row] == HUNGARIAN_UNASSIGNED) {
            return null;
        }
        return _.row_to_col[row];
    }

    /// Returns the assigned row index for `col`, or `null` if unassigned / out of range.
    fn rowForCol(col: usize): u16? {
        if (col >= _.cols || _.col_to_row[col] == HUNGARIAN_UNASSIGNED) {
            return null;
        }
        return _.col_to_row[col];
    }
}

// ============================================================================
// 2. Internal i128 Potential-Function Hungarian Solver
// ============================================================================

// `2^80 (~1.2e24)` strictly exceeds `32 * |i64::MIN| (~2.95e20)`, guaranteeing any feasible matching
// beats any matching touching even one forbidden edge without ever overflowing `i128`.
const INTERNAL_FORBIDDEN: i128 = (1 as i128) << 80;
const INTERNAL_INF: i128 = (1 as i128) << 88;
const I64_MIN_128: i128 = -((1 as i128) << 63);
const I64_MAX_128: i128 = ((1 as i128) << 63) - 1;

fn solveHungarianInternal(
    rows: usize,
    cols: usize,
    matrix: []const i64,
    maximize: bool,
): HungarianAssignment {
    if (rows == 0 || cols == 0 || rows > HUNGARIAN_MAX_DIM || cols > HUNGARIAN_MAX_DIM) {
        return HungarianAssignment{ status: HungarianStatus.invalid_dimensions };
    }
    if (matrix.len != rows * cols) {
        return HungarianAssignment{ status: HungarianStatus.matrix_length_mismatch };
    }

    // Orient internally so `n <= m` (transpose view when `rows > cols`).
    const transposed: bool = rows > cols;
    const n: usize = if (transposed) cols else rows;
    const m: usize = if (transposed) rows else cols;

    // 1-indexed internal cost matrix (`n x m`).
    let a: [HUNGARIAN_MAX_DIM + 1][HUNGARIAN_MAX_DIM + 1]i128 = [[0; HUNGARIAN_MAX_DIM + 1]; HUNGARIAN_MAX_DIM + 1];
    let i_init: usize = 1;
    while (i_init <= n) : (i_init += 1) {
        let j_init: usize = 1;
        while (j_init <= m) : (j_init += 1) {
            const orig_r: usize = if (transposed) (j_init - 1) else (i_init - 1);
            const orig_c: usize = if (transposed) (i_init - 1) else (j_init - 1);
            const raw: i64 = matrix[orig_r * cols + orig_c];
            if (raw == HUNGARIAN_FORBIDDEN_COST) {
                a[i_init][j_init] = INTERNAL_FORBIDDEN;
            } else if (maximize) {
                a[i_init][j_init] = -(raw as i128);
            } else {
                a[i_init][j_init] = raw as i128;
            }
        }
    }

    // Classic 1-indexed Kuhn-Munkres potential & augmenting-path loop.
    let u: [HUNGARIAN_MAX_DIM + 1]i128 = [0; HUNGARIAN_MAX_DIM + 1];
    let v: [HUNGARIAN_MAX_DIM + 1]i128 = [0; HUNGARIAN_MAX_DIM + 1];
    let p: [HUNGARIAN_MAX_DIM + 1]usize = [0; HUNGARIAN_MAX_DIM + 1];
    let way: [HUNGARIAN_MAX_DIM + 1]usize = [0; HUNGARIAN_MAX_DIM + 1];

    let i: usize = 1;
    while (i <= n) : (i += 1) {
        p[0] = i;
        let j0: usize = 0;
        let minv: [HUNGARIAN_MAX_DIM + 1]i128 = [INTERNAL_INF; HUNGARIAN_MAX_DIM + 1];
        let used: [HUNGARIAN_MAX_DIM + 1]bool = [false; HUNGARIAN_MAX_DIM + 1];

        while (true) {
            used[j0] = true;
            const cur_row: usize = p[j0];
            let delta: i128 = INTERNAL_INF;
            let j1: usize = 0;

            let j: usize = 1;
            while (j <= m) : (j += 1) {
                if (!used[j]) {
                    const cur: i128 = a[cur_row][j] - u[cur_row] - v[j];
                    if (cur < minv[j]) {
                        minv[j] = cur;
                        way[j] = j0;
                    }
                    if (minv[j] < delta) {
                        delta = minv[j];
                        j1 = j;
                    }
                }
            }

            let k: usize = 0;
            while (k <= m) : (k += 1) {
                if (used[k]) {
                    u[p[k]] += delta;
                    v[k] -= delta;
                } else {
                    minv[k] -= delta;
                }
            }

            j0 = j1;
            if (p[j0] == 0) {
                break;
            }
        }

        while (true) {
            const j1: usize = way[j0];
            p[j0] = p[j1];
            j0 = j1;
            if (j0 == 0) {
                break;
            }
        }
    }

    // Reconstruct matching and verify feasibility against forbidden edges.
    let res = HungarianAssignment{
        status: HungarianStatus.ok,
        rows: rows,
        cols: cols,
        matched_count: 0,
        total_cost: 0,
    };

    let sum_128: i128 = 0;
    let col_j: usize = 1;
    while (col_j <= m) : (col_j += 1) {
        if (p[col_j] == 0) continue;
        const row_i: usize = p[col_j];
        const orig_r: usize = if (transposed) (col_j - 1) else (row_i - 1);
        const orig_c: usize = if (transposed) (row_i - 1) else (col_j - 1);
        const orig_val: i64 = matrix[orig_r * cols + orig_c];

        if (orig_val == HUNGARIAN_FORBIDDEN_COST) {
            return HungarianAssignment{
                status: HungarianStatus.infeasible,
                rows: rows,
                cols: cols,
            };
        }

        res.row_to_col[orig_r] = orig_c as u16;
        res.col_to_row[orig_c] = orig_r as u16;
        res.matched_count += 1;
        sum_128 += orig_val as i128;
    }

    if (sum_128 < I64_MIN_128 || sum_128 > I64_MAX_128) {
        return HungarianAssignment{
            status: HungarianStatus.overflow,
            rows: rows,
            cols: cols,
        };
    }

    res.total_cost = sum_128 as i64;
    return res;
}

// ============================================================================
// 3. Public Min-Cost & Max-Profit Assignment Entrypoints
// ============================================================================

/// Solves the minimum-cost bipartite assignment problem for a `rows x cols` (`<= 32 x 32`)
/// row-major `cost_matrix` with zero dynamic heap allocation.
pub fn solveMinCostAssignment(
    rows: usize,
    cols: usize,
    cost_matrix: []const i64,
): HungarianAssignment {
    return solveHungarianInternal(rows, cols, cost_matrix, false);
}

/// Solves the maximum-profit bipartite assignment problem for a `rows x cols` (`<= 32 x 32`)
/// row-major `profit_matrix` with zero dynamic heap allocation (`total_cost` holds total profit).
pub fn solveMaxProfitAssignment(
    rows: usize,
    cols: usize,
    profit_matrix: []const i64,
): HungarianAssignment {
    return solveHungarianInternal(rows, cols, profit_matrix, true);
}
