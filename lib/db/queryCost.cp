// Origin: Pure-CP Stdlib (Zero-Alloc Relational Query Cost Estimator, Selectivity & Left-Deep Join Planner) | Created: 2026-11-23
// Pure CP zero-heap-allocation relational query optimizer core (System R / PostgreSQL cost model):
//   - Predicate selectivity estimation (`eq`, `neq`, `range_half`, `range_between`, `is_null`,
//     `is_not_null`, `like_prefix`, `in_list`) with strict `[1e-6, 1.0]` clamping & AND/OR combinators
//   - Single-table access path selection (`seq_scan` vs `index_scan` vs `index_only_scan`)
//   - Binary join physical algorithm selection (`nested_loop` vs `hash_join` vs `merge_join`)
//   - Zero-alloc greedy left-deep multi-table join order planner (`<= 8` tables)

const std = @import("std");

// ============================================================================
// 1. Constants & Enums
// ============================================================================

/// Maximum number of tables supported by the zero-alloc `planLeftDeepJoinOrder` planner.
pub const QCOST_MAX_TABLES: usize = 8;

/// Basis-point denominator (`10000` = 100.00%) for `null_frac_bps`.
pub const QCOST_BPS_SCALE: u16 = 10000;

const MIN_SELECTIVITY: f64 = 1.0e-6;
const MAX_SELECTIVITY: f64 = 1.0;

/// Supported relational predicate kinds for selectivity estimation.
pub enum PredicateKind {
    eq,
    neq,
    range_half,
    range_between,
    is_null,
    is_not_null,
    like_prefix,
    in_list,
}

/// Physical single-table scan access path.
pub enum AccessPathKind {
    seq_scan,
    index_scan,
    index_only_scan,
}

/// Physical binary join algorithm.
pub enum JoinAlgoKind {
    nested_loop,
    hash_join,
    merge_join,
}

// ============================================================================
// 2. Cost Model Parameters & Plan Structs
// ============================================================================

/// Tunable I/O and CPU cost weights modeled after PostgreSQL / System R.
pub struct CostModelParams {
    seq_page_cost: f64 = 1.0;
    random_page_cost: f64 = 4.0;
    cpu_tuple_cost: f64 = 0.01;
    cpu_index_tuple_cost: f64 = 0.005;
    hash_build_cost: f64 = 0.015;
    sort_cost_factor: f64 = 0.02;

    /// Returns standard PostgreSQL-style default cost weights.
    static fn defaults(): CostModelParams {
        return CostModelParams{};
    }
}

/// Catalog statistics for a single relation participating in a query.
pub struct TableStats {
    table_id: u16 = 0;
    row_count: f64 = 0.0;
    page_count: f64 = 0.0;
    ndv: f64 = 1.0;
    null_frac_bps: u16 = 0;
    has_index: bool = false;
    is_covering_index: bool = false;
    is_sorted_on_join_key: bool = false;
}

/// Estimated cost breakdown for a chosen single-table access path.
pub struct AccessPathCost {
    kind: AccessPathKind = AccessPathKind.seq_scan;
    estimated_rows: f64 = 0.0;
    io_cost: f64 = 0.0;
    cpu_cost: f64 = 0.0;
    total_cost: f64 = 0.0;
}

/// Estimated cost and cardinality for a binary join step.
pub struct JoinPlanCost {
    algo: JoinAlgoKind = JoinAlgoKind.hash_join;
    output_rows: f64 = 0.0;
    total_cost: f64 = 0.0;
}

/// Complete left-deep join order plan produced by `planLeftDeepJoinOrder`.
pub struct JoinOrderPlan {
    table_count: usize = 0;
    order: [QCOST_MAX_TABLES]u16 = [0; QCOST_MAX_TABLES];
    step_algos: [QCOST_MAX_TABLES]JoinAlgoKind = [JoinAlgoKind.hash_join; QCOST_MAX_TABLES];
    total_cost: f64 = 0.0;
    final_rows: f64 = 0.0;
}

// ============================================================================
// 3. Internal Numeric Helpers
// ============================================================================

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clampSelectivity(s: f64): f64 {
    if (!isFiniteF64(s) || s < MIN_SELECTIVITY) return MIN_SELECTIVITY;
    if (s > MAX_SELECTIVITY) return MAX_SELECTIVITY;
    return s;
}

fn nonNegF64(x: f64): f64 {
    if (!isFiniteF64(x) || x < 0.0) return 0.0;
    return x;
}

fn log2Safe(x: f64): f64 {
    if (!isFiniteF64(x) || x <= 1.0) return 0.0;
    return std.math.log2(x);
}

// ============================================================================
// 4. Predicate Selectivity & Combinators
// ============================================================================

/// Estimates single-predicate selectivity clamped strictly to `[1e-6, 1.0]`.
///
/// - `ndv`: number of distinct values (`<= 0` safely treated as `1.0`).
/// - `null_frac_bps`: fraction of NULL rows in basis points (`0..=10000`).
/// - `in_list_len`: cardinality of `IN (...)` list (used for `PredicateKind.in_list`).
/// - `range_frac`: optional histogram range fraction in `(0, 1]` (falls back to System R defaults when `<= 0`).
pub fn estimatePredicateSelectivity(
    kind: PredicateKind,
    ndv: f64,
    null_frac_bps: u16,
    in_list_len: usize,
    range_frac: f64,
): f64 {
    const capped_bps: u16 = if (null_frac_bps > QCOST_BPS_SCALE) QCOST_BPS_SCALE else null_frac_bps;
    const null_frac: f64 = (capped_bps as f64) / (QCOST_BPS_SCALE as f64);
    const non_null: f64 = 1.0 - null_frac;
    const safe_ndv: f64 = if (isFiniteF64(ndv) && ndv >= 1.0) ndv else 1.0;

    let safe_range: f64 = 0.0;
    if (isFiniteF64(range_frac) && range_frac > 0.0) {
        safe_range = if (range_frac > 1.0) 1.0 else range_frac;
    }

    let raw: f64 = MIN_SELECTIVITY;
    if (kind == PredicateKind.eq) {
        raw = non_null / safe_ndv;
    } else if (kind == PredicateKind.neq) {
        raw = non_null * (1.0 - (1.0 / safe_ndv));
    } else if (kind == PredicateKind.range_half) {
        const rf: f64 = if (safe_range > 0.0) safe_range else 0.3333333333333333;
        raw = non_null * rf;
    } else if (kind == PredicateKind.range_between) {
        const rf: f64 = if (safe_range > 0.0) safe_range else 0.25;
        raw = non_null * rf;
    } else if (kind == PredicateKind.is_null) {
        raw = null_frac;
    } else if (kind == PredicateKind.is_not_null) {
        raw = non_null;
    } else if (kind == PredicateKind.like_prefix) {
        const rf: f64 = if (safe_range > 0.0) safe_range else 0.1;
        raw = non_null * rf;
    } else {
        // PredicateKind.in_list
        if (in_list_len == 0) {
            raw = MIN_SELECTIVITY;
        } else {
            const list_frac: f64 = (in_list_len as f64) / safe_ndv;
            const capped_list: f64 = if (list_frac > 1.0) 1.0 else list_frac;
            raw = non_null * capped_list;
        }
    }
    return clampSelectivity(raw);
}

/// Combines two independent predicate selectivities under logical `AND`: `s1 * s2`.
pub fn combineSelectivityAnd(s1: f64, s2: f64): f64 {
    return clampSelectivity(clampSelectivity(s1) * clampSelectivity(s2));
}

/// Combines two independent predicate selectivities under logical `OR`: `s1 + s2 - s1 * s2`.
pub fn combineSelectivityOr(s1: f64, s2: f64): f64 {
    const a: f64 = clampSelectivity(s1);
    const b: f64 = clampSelectivity(s2);
    return clampSelectivity(a + b - a * b);
}

// ============================================================================
// 5. Access Path & Join Algorithm Selection
// ============================================================================

/// Evaluates `seq_scan`, `index_scan`, and `index_only_scan` (when available) for `tbl`
/// and returns the lowest-cost physical access path.
pub fn chooseBestAccessPath(
    tbl: TableStats,
    selectivity: f64,
    params: CostModelParams,
): AccessPathCost {
    const rows: f64 = nonNegF64(tbl.row_count);
    const pages: f64 = nonNegF64(tbl.page_count);
    const sel: f64 = clampSelectivity(selectivity);
    const est_rows: f64 = rows * sel;

    if (rows == 0.0 && pages == 0.0) {
        return AccessPathCost{
            kind: AccessPathKind.seq_scan,
            estimated_rows: 0.0,
            io_cost: 0.0,
            cpu_cost: 0.0,
            total_cost: 0.0,
        };
    }

    // 1. Sequential scan baseline
    const seq_io: f64 = pages * params.seq_page_cost;
    const seq_cpu: f64 = rows * params.cpu_tuple_cost;
    let best = AccessPathCost{
        kind: AccessPathKind.seq_scan,
        estimated_rows: est_rows,
        io_cost: seq_io,
        cpu_cost: seq_cpu,
        total_cost: seq_io + seq_cpu,
    };

    if (!tbl.has_index) return best;

    const tree_pages: f64 = if (pages >= 2.0) pages else 2.0;
    const tree_io: f64 = log2Safe(tree_pages) * params.random_page_cost * 0.25;
    const leaf_io: f64 = (pages * 0.2 * sel) * params.seq_page_cost;

    // 2. Index scan (B+Tree probe + leaf scan + random heap fetches)
    const heap_fetches: f64 = if (est_rows < pages) est_rows else pages;
    const idx_io: f64 = tree_io + leaf_io + heap_fetches * params.random_page_cost;
    const idx_cpu: f64 = est_rows * (params.cpu_index_tuple_cost + params.cpu_tuple_cost);
    const idx_total: f64 = idx_io + idx_cpu;
    if (idx_total < best.total_cost) {
        best = AccessPathCost{
            kind: AccessPathKind.index_scan,
            estimated_rows: est_rows,
            io_cost: idx_io,
            cpu_cost: idx_cpu,
            total_cost: idx_total,
        };
    }

    // 3. Index-only scan (covering index eliminates heap fetches)
    if (tbl.is_covering_index) {
        const io_only_io: f64 = tree_io + leaf_io;
        const io_only_cpu: f64 = est_rows * params.cpu_index_tuple_cost;
        const io_only_total: f64 = io_only_io + io_only_cpu;
        if (io_only_total < best.total_cost) {
            best = AccessPathCost{
                kind: AccessPathKind.index_only_scan,
                estimated_rows: est_rows,
                io_cost: io_only_io,
                cpu_cost: io_only_cpu,
                total_cost: io_only_total,
            };
        }
    }

    return best;
}

/// Evaluates `nested_loop`, `hash_join`, and `merge_join` and returns the lowest-cost binary join plan.
pub fn chooseBestJoinAlgo(
    left_rows: f64,
    right_rows: f64,
    join_selectivity: f64,
    right_indexed: bool,
    both_sorted: bool,
    params: CostModelParams,
): JoinPlanCost {
    const l: f64 = nonNegF64(left_rows);
    const r: f64 = nonNegF64(right_rows);
    const js: f64 = clampSelectivity(join_selectivity);
    const out_rows: f64 = l * r * js;

    // 1. Nested Loop Join (indexed probe vs full inner scan)
    let nl_cost: f64 = 0.0;
    if (right_indexed) {
        const r_probe: f64 = if (r >= 2.0) r else 2.0;
        const per_probe: f64 = log2Safe(r_probe) * params.cpu_index_tuple_cost + params.random_page_cost * 0.05;
        nl_cost = l * per_probe + out_rows * params.cpu_tuple_cost;
    } else {
        nl_cost = (l * r + out_rows) * params.cpu_tuple_cost;
    }

    let best = JoinPlanCost{
        algo: JoinAlgoKind.nested_loop,
        output_rows: out_rows,
        total_cost: nl_cost,
    };

    // 2. Hash Join (build smaller side + probe larger side + fixed hash table setup)
    const build_side: f64 = if (l < r) l else r;
    const probe_side: f64 = if (l < r) r else l;
    const hj_setup: f64 = 2.0;
    const hj_cost: f64 = hj_setup + build_side * params.hash_build_cost + (probe_side + out_rows) * params.cpu_tuple_cost;
    if (hj_cost < best.total_cost) {
        best = JoinPlanCost{
            algo: JoinAlgoKind.hash_join,
            output_rows: out_rows,
            total_cost: hj_cost,
        };
    }

    // 3. Merge Join (linear merge if both sorted; otherwise pay O(N log N) sort penalty)
    let mj_cost: f64 = 0.0;
    if (both_sorted) {
        mj_cost = (l + r) * params.cpu_tuple_cost * 0.8 + out_rows * params.cpu_tuple_cost;
    } else {
        const sort_l: f64 = l * log2Safe(l) * params.sort_cost_factor;
        const sort_r: f64 = r * log2Safe(r) * params.sort_cost_factor;
        mj_cost = sort_l + sort_r + (l + r + out_rows) * params.cpu_tuple_cost;
    }
    if (mj_cost < best.total_cost) {
        best = JoinPlanCost{
            algo: JoinAlgoKind.merge_join,
            output_rows: out_rows,
            total_cost: mj_cost,
        };
    }

    return best;
}

// ============================================================================
// 6. Zero-Alloc Greedy Left-Deep Join Order Planner
// ============================================================================

/// Plans a left-deep multi-table join order (up to `QCOST_MAX_TABLES = 8` tables) using
/// a zero-heap greedy cost strategy that selects the smallest-cardinality driving table first
/// and iteratively joins the lowest incremental-cost next relation.
pub fn planLeftDeepJoinOrder(
    tables: []const TableStats,
    join_selectivity: f64,
    params: CostModelParams,
): JoinOrderPlan {
    let plan = JoinOrderPlan{};
    const n: usize = if (tables.len < QCOST_MAX_TABLES) tables.len else QCOST_MAX_TABLES;
    if (n == 0) return plan;

    let used: [QCOST_MAX_TABLES]bool = [false; QCOST_MAX_TABLES];

    // Step 0: pick driving table with smallest row_count (tie-break by scan cost, then table_id)
    let best_first: usize = 0;
    let best_first_rows: f64 = nonNegF64(tables[0].row_count);
    let best_first_scan: f64 = chooseBestAccessPath(tables[0], 1.0, params).total_cost;
    let i: usize = 1;
    while (i < n) : (i += 1) {
        const r: f64 = nonNegF64(tables[i].row_count);
        const sc: f64 = chooseBestAccessPath(tables[i], 1.0, params).total_cost;
        if (r < best_first_rows || (r == best_first_rows && (sc < best_first_scan || (sc == best_first_scan && tables[i].table_id < tables[best_first].table_id)))) {
            best_first = i;
            best_first_rows = r;
            best_first_scan = sc;
        }
    }

    used[best_first] = true;
    plan.table_count = n;
    plan.order[0] = tables[best_first].table_id;
    plan.step_algos[0] = JoinAlgoKind.nested_loop;

    let acc_rows: f64 = best_first_rows;
    let acc_cost: f64 = best_first_scan;
    let acc_sorted: bool = tables[best_first].is_sorted_on_join_key;

    // Steps 1 .. n-1: greedily attach the remaining table with lowest incremental join + scan cost
    let step: usize = 1;
    while (step < n) : (step += 1) {
        let best_idx: usize = n;
        let best_step_cost: f64 = 0.0;
        let best_join = JoinPlanCost{};

        let j: usize = 0;
        while (j < n) : (j += 1) {
            if (used[j]) continue;
            const scan_c: f64 = chooseBestAccessPath(tables[j], 1.0, params).total_cost;
            const both_sorted: bool = acc_sorted && tables[j].is_sorted_on_join_key;
            const jp: JoinPlanCost = chooseBestJoinAlgo(
                acc_rows,
                tables[j].row_count,
                join_selectivity,
                tables[j].has_index,
                both_sorted,
                params,
            );
            const total_step: f64 = scan_c + jp.total_cost;
            if (best_idx == n || total_step < best_step_cost || (total_step == best_step_cost && nonNegF64(tables[j].row_count) < nonNegF64(tables[best_idx].row_count))) {
                best_idx = j;
                best_step_cost = total_step;
                best_join = jp;
            }
        }

        used[best_idx] = true;
        plan.order[step] = tables[best_idx].table_id;
        plan.step_algos[step] = best_join.algo;
        acc_cost += best_step_cost;
        acc_rows = best_join.output_rows;
        acc_sorted = (best_join.algo == JoinAlgoKind.merge_join);
    }

    plan.total_cost = acc_cost;
    plan.final_rows = acc_rows;
    return plan;
}
