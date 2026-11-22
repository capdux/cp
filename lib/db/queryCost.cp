// Origin: Pure-CP Stdlib (Zero-Alloc Relational Query Optimizer: MCV+Equi-Depth Histograms, System-R Selinger DP & Unbounded Join Planner) | Created: 2026-11-23
// Pure CP zero-heap-allocation relational query optimizer core (System R / PostgreSQL cost model):
//   - Predicate selectivity estimation (`eq`, `neq`, `range_half`, `range_between`, `is_null`,
//     `is_not_null`, `like_prefix`, `in_list`) with strict `[1e-6, 1.0]` clamping & AND/OR combinators
//   - PostgreSQL `pg_statistic`-style MCV (Most Common Values) + Equi-Depth Histogram selectivity engine
//     (`buildEquiDepthHistogram`, `estimateEqSelectivityHist`, `estimateRangeSelectivityHist`)
//   - Single-table access path selection (`seq_scan` vs `index_scan` vs `index_only_scan`)
//   - Binary join physical algorithm selection (`nested_loop` vs `hash_join` vs `merge_join`)
//   - Zero-alloc greedy left-deep multi-table join order planner (`<= 8` tables, backward-compatible)
//   - System-R / Selinger bitmask Dynamic Programming optimal join order planner (Left-Deep & Bushy trees,
//     up to 14 tables = 16,384 states in caller-provided DP workspace, prioritizing connected join subgraphs)
//   - Unbounded external-slice greedy join planner supporting 32/64+ table analytical queries with zero heap alloc

const std = @import("std");

// ============================================================================
// 1. Constants, Errors & Enums
// ============================================================================

/// Maximum number of tables supported by the fixed-buffer `planLeftDeepJoinOrder` planner.
pub const QCOST_MAX_TABLES: usize = 8;

/// Maximum number of tables supported by the bitmask Selinger DP planner (`1 << 14 = 16,384` states).
pub const QCOST_DP_MAX_TABLES: usize = 14;

/// Basis-point denominator (`10000` = 100.00%) for `null_frac_bps`.
pub const QCOST_BPS_SCALE: u16 = 10000;

const MIN_SELECTIVITY: f64 = 1.0e-6;
const MAX_SELECTIVITY: f64 = 1.0;
const MCV_EQ_EPSILON: f64 = 1.0e-12;

/// Error set returned by histogram builders and join-graph query planners.
pub err QueryOptError {
    InvalidHistogram,
    InvalidTables,
    BufferTooSmall,
    UnsupportedDpTableCount,
    InvalidEdge,
}

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

fn clampUnit(x: f64): f64 {
    if (!isFiniteF64(x) || x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;
    return x;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn nonNegF64(x: f64): f64 {
    if (!isFiniteF64(x) || x < 0.0) return 0.0;
    return x;
}

fn log2Safe(x: f64): f64 {
    if (!isFiniteF64(x) || x <= 1.0) return 0.0;
    return std.math.log2(x);
}

fn popcountU16(mask: u16): u32 {
    let m: u16 = mask;
    let c: u32 = 0;
    while (m != 0) {
        m = m & (m - 1);
        c += 1;
    }
    return c;
}

fn trailingZeroIndexU16(mask: u16): usize {
    let idx: usize = 0;
    let m: u16 = mask;
    while (idx < 16 && (m & 1) == 0) : (idx += 1) {
        m = m >> 1;
    }
    return idx;
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
// 6. Zero-Alloc Greedy Left-Deep Join Order Planner (Legacy <= 8 Tables)
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

// ============================================================================
// 7. PostgreSQL `pg_statistic`-Style MCV + Equi-Depth Histogram Engine
// ============================================================================

/// Most Common Value (MCV) entry capturing a skewed heavy-hitter value and its exact frequency in `(0, 1]`.
pub struct McvEntry {
    value: f64 = 0.0;
    freq: f64 = 0.0;
}

/// Equi-depth histogram bucket `[low, high]` with distinct-value count inside the bucket.
pub struct HistBucket {
    low: f64 = 0.0;
    high: f64 = 0.0;
    distinct_count: f64 = 1.0;
}

/// Caller-slice-backed column statistics modeled after PostgreSQL `pg_statistic`:
/// combines exact MCV heavy-hitter frequencies with equi-depth histogram buckets for the residual tail.
pub struct ColumnHistogram {
    mcvs: []const McvEntry = [];
    buckets: []const HistBucket = [];
    null_frac: f64 = 0.0;
    ndv: f64 = 1.0;
    total_rows: f64 = 0.0;

    /// Returns the total clamped probability mass covered by all MCV entries.
    fn mcvTotalFreq(): f64 {
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < mcvs.len) : (i += 1) {
            sum += clampUnit(mcvs[i].freq);
        }
        const non_null: f64 = 1.0 - clampUnit(null_frac);
        return if (sum > non_null) non_null else sum;
    }

    /// Returns the remaining non-NULL, non-MCV probability mass represented by `buckets`.
    fn nonMcvMass(): f64 {
        const non_null: f64 = 1.0 - clampUnit(null_frac);
        const mcv_sum: f64 = mcvTotalFreq();
        return if (non_null > mcv_sum) non_null - mcv_sum else 0.0;
    }
}

/// Builds an equi-depth histogram of `min(num_buckets, sorted_values.len)` buckets from an arbitrary-length
/// sorted sample slice into `out_buckets` with zero dynamic heap allocation.
pub fn buildEquiDepthHistogram(
    sorted_values: []const f64,
    num_buckets: usize,
    out_buckets: []HistBucket,
): usize ! QueryOptError {
    if (sorted_values.len == 0 || num_buckets == 0) {
        return QueryOptError.InvalidHistogram;
    }

    // Verify finite non-decreasing order
    if (!isFiniteF64(sorted_values[0])) {
        return QueryOptError.InvalidHistogram;
    }
    let vi: usize = 1;
    while (vi < sorted_values.len) : (vi += 1) {
        if (!isFiniteF64(sorted_values[vi]) || sorted_values[vi] < sorted_values[vi - 1]) {
            return QueryOptError.InvalidHistogram;
        }
    }

    const actual_buckets: usize = if (num_buckets < sorted_values.len) num_buckets else sorted_values.len;
    if (out_buckets.len < actual_buckets) {
        return QueryOptError.BufferTooSmall;
    }

    const n: usize = sorted_values.len;
    let b: usize = 0;
    while (b < actual_buckets) : (b += 1) {
        const start: usize = (b * n) / actual_buckets;
        const end: usize = ((b + 1) * n) / actual_buckets;
        if (end <= start) {
            return QueryOptError.InvalidHistogram;
        }
        const lo: f64 = sorted_values[start];
        const hi: f64 = sorted_values[end - 1];

        let distinct: usize = 1;
        let k: usize = start + 1;
        while (k < end) : (k += 1) {
            if (absF64(sorted_values[k] - sorted_values[k - 1]) > MCV_EQ_EPSILON) {
                distinct += 1;
            }
        }
        out_buckets[b] = HistBucket{
            low: lo,
            high: hi,
            distinct_count: distinct as f64,
        };
    }

    return actual_buckets;
}

/// Estimates equality selectivity `P(col = target)` using PostgreSQL `pg_statistic` semantics:
/// 1. If `target` matches an MCV entry, returns its exact skewed frequency immediately.
/// 2. Otherwise distributes the residual non-MCV, non-NULL probability mass over the remaining
///    distinct values (`max(1, ndv - mcvs.len)`).
pub fn estimateEqSelectivityHist(hist: ColumnHistogram, target: f64): f64 {
    if (!isFiniteF64(target)) {
        return MIN_SELECTIVITY;
    }

    // 1. Exact MCV heavy-hitter lookup
    let i: usize = 0;
    while (i < hist.mcvs.len) : (i += 1) {
        const entry = hist.mcvs[i];
        if (isFiniteF64(entry.value) && absF64(entry.value - target) <= MCV_EQ_EPSILON) {
            return clampSelectivity(clampUnit(entry.freq));
        }
    }

    // 2. Non-MCV residual uniform spread
    const rem_mass: f64 = hist.nonMcvMass();
    if (rem_mass <= 0.0) {
        return MIN_SELECTIVITY;
    }
    const safe_ndv: f64 = if (isFiniteF64(hist.ndv) && hist.ndv >= 1.0) hist.ndv else 1.0;
    const mcv_cnt: f64 = hist.mcvs.len as f64;
    const rem_ndv: f64 = if (safe_ndv > mcv_cnt) safe_ndv - mcv_cnt else 1.0;

    return clampSelectivity(rem_mass / rem_ndv);
}

fn valueInRange(v: f64, low: f64, high: f64, include_low: bool, include_high: bool): bool {
    const ge_low: bool = if (include_low) (v >= low - MCV_EQ_EPSILON) else (v > low + MCV_EQ_EPSILON);
    const le_high: bool = if (include_high) (v <= high + MCV_EQ_EPSILON) else (v < high - MCV_EQ_EPSILON);
    return ge_low && le_high;
}

/// Estimates range selectivity `P(low <[=] col <[=] high)` by summing:
/// 1. Exact frequencies of all MCV entries falling inside the query interval; and
/// 2. Equi-depth histogram bucket contributions (full weight for enclosed buckets + linear
///    interpolation across partially overlapping boundary buckets).
pub fn estimateRangeSelectivityHist(
    hist: ColumnHistogram,
    low: f64,
    high: f64,
    include_low: bool,
    include_high: bool,
): f64 {
    if (!isFiniteF64(low) || !isFiniteF64(high) || low > high) {
        return MIN_SELECTIVITY;
    }
    if (absF64(high - low) <= MCV_EQ_EPSILON) {
        if (include_low && include_high) {
            return estimateEqSelectivityHist(hist, low);
        }
        return MIN_SELECTIVITY;
    }

    // 1. MCV contribution inside [low, high]
    let mcv_hit: f64 = 0.0;
    let mi: usize = 0;
    while (mi < hist.mcvs.len) : (mi += 1) {
        const entry = hist.mcvs[mi];
        if (isFiniteF64(entry.value) && valueInRange(entry.value, low, high, include_low, include_high)) {
            mcv_hit += clampUnit(entry.freq);
        }
    }

    // 2. Equi-depth histogram contribution over residual non-MCV probability mass
    const rem_mass: f64 = hist.nonMcvMass();
    let hist_hit: f64 = 0.0;
    if (rem_mass > 0.0) {
        if (hist.buckets.len == 0) {
            hist_hit = rem_mass * 0.25;
        } else {
            const bucket_mass: f64 = rem_mass / (hist.buckets.len as f64);
            let bi: usize = 0;
            while (bi < hist.buckets.len) : (bi += 1) {
                const b = hist.buckets[bi];
                if (!isFiniteF64(b.low) || !isFiniteF64(b.high) || b.high < b.low) {
                    continue;
                }
                const span: f64 = b.high - b.low;
                if (span <= MCV_EQ_EPSILON) {
                    // Degenerate single-value bucket
                    if (valueInRange(b.low, low, high, include_low, include_high)) {
                        hist_hit += bucket_mass;
                    }
                    continue;
                }

                // Disjoint check
                if (b.high < low || b.low > high) {
                    continue;
                }
                if (b.high == low && !include_low) {
                    continue;
                }
                if (b.low == high && !include_high) {
                    continue;
                }

                // Fully enclosed bucket
                if (b.low >= low && b.high <= high) {
                    hist_hit += bucket_mass;
                    continue;
                }

                // Partial overlap: linear interpolation
                const ov_lo: f64 = if (b.low > low) b.low else low;
                const ov_hi: f64 = if (b.high < high) b.high else high;
                if (ov_hi > ov_lo) {
                    const frac: f64 = (ov_hi - ov_lo) / span;
                    hist_hit += bucket_mass * clampUnit(frac);
                }
            }
        }
    }

    const non_null: f64 = 1.0 - clampUnit(hist.null_frac);
    const total: f64 = mcv_hit + hist_hit;
    const capped: f64 = if (total > non_null) non_null else total;
    return clampSelectivity(capped);
}

// ============================================================================
// 8. System-R / Selinger Bitmask DP Optimal Join Planner (Left-Deep & Bushy)
// ============================================================================

/// Undirected join predicate edge between two tables (`left_idx`, `right_idx` in `0..tables.len`).
pub struct JoinGraphEdge {
    left_idx: u16 = 0;
    right_idx: u16 = 0;
    selectivity: f64 = 0.1;
    right_indexed: bool = false;
    both_sorted: bool = false;
}

/// Dynamic programming memoization slot for a table subset bitmask (`1..=(1<<N)-1`).
pub struct DpJoinState {
    cost: f64 = 0.0;
    rows: f64 = 0.0;
    left_mask: u16 = 0;
    right_mask: u16 = 0;
    algo: JoinAlgoKind = JoinAlgoKind.hash_join;
    cartesian_count: u16 = 0;
    is_sorted: bool = false;
    valid: bool = false;
}

/// Summary of an optimal DP or unbounded greedy multi-table join plan.
pub struct DpJoinPlanResult {
    table_count: usize = 0;
    is_bushy: bool = false;
    total_cost: f64 = 0.0;
    final_rows: f64 = 0.0;
    states_evaluated: usize = 0;
    cartesian_joins_used: usize = 0;
}

struct CrossEdgeInfo {
    has_edge: bool = false;
    selectivity: f64 = 1.0;
    right_indexed: bool = false;
    both_sorted: bool = false;
}

/// Returns the required `dp_work` slice length (`1 << num_tables`) for `planOptimalJoinOrderDp`,
/// or `0` if `num_tables == 0` or `num_tables > QCOST_DP_MAX_TABLES` (`14`).
pub fn dpJoinWorkRequiredLen(num_tables: usize): usize {
    if (num_tables == 0 || num_tables > QCOST_DP_MAX_TABLES) {
        return 0;
    }
    return @as(usize, 1) << (num_tables as u6);
}

fn validateJoinEdges(num_tables: usize, edges: []const JoinGraphEdge): void ! QueryOptError {
    let i: usize = 0;
    while (i < edges.len) : (i += 1) {
        const e = edges[i];
        if ((e.left_idx as usize) >= num_tables || (e.right_idx as usize) >= num_tables || e.left_idx == e.right_idx) {
            return QueryOptError.InvalidEdge;
        }
    }
}

fn evaluateCrossEdgesMask(
    left_mask: u16,
    right_mask: u16,
    edges: []const JoinGraphEdge,
    tables: []const TableStats,
    left_sorted: bool,
    right_sorted: bool,
): CrossEdgeInfo {
    let info = CrossEdgeInfo{};
    let combined_sel: f64 = 1.0;
    let i: usize = 0;
    while (i < edges.len) : (i += 1) {
        const e = edges[i];
        const lbit: u16 = @as(u16, 1) << (e.left_idx as u4);
        const rbit: u16 = @as(u16, 1) << (e.right_idx as u4);
        const lr: bool = ((left_mask & lbit) != 0) && ((right_mask & rbit) != 0);
        const rl: bool = ((left_mask & rbit) != 0) && ((right_mask & lbit) != 0);
        if (lr || rl) {
            info.has_edge = true;
            combined_sel = clampSelectivity(combined_sel * clampSelectivity(e.selectivity));
            if (lr && e.right_indexed) {
                info.right_indexed = true;
            } else if (rl && tables[e.left_idx as usize].has_index) {
                info.right_indexed = true;
            }
            if (e.both_sorted) {
                info.both_sorted = true;
            }
        }
    }
    if (info.has_edge) {
        info.selectivity = combined_sel;
        // Single-table right side with index also enables indexed nested loop
        if (popcountU16(right_mask) == 1) {
            const ridx = trailingZeroIndexU16(right_mask);
            if (tables[ridx].has_index) {
                info.right_indexed = true;
            }
        }
        if (left_sorted && right_sorted) {
            info.both_sorted = true;
        }
    } else {
        // Disconnected Cartesian product
        info.selectivity = 1.0;
        info.right_indexed = false;
        info.both_sorted = false;
    }
    return info;
}

fn tryRelaxDpState(
    dp_work: []DpJoinState,
    mask: usize,
    left_mask: u16,
    right_mask: u16,
    tables: []const TableStats,
    edges: []const JoinGraphEdge,
    params: CostModelParams,
    states_eval: *usize,
) {
    const lstate = dp_work[left_mask as usize];
    const rstate = dp_work[right_mask as usize];
    if (!lstate.valid || !rstate.valid) return;

    const ce = evaluateCrossEdgesMask(left_mask, right_mask, edges, tables, lstate.is_sorted, rstate.is_sorted);
    const jp = chooseBestJoinAlgo(
        lstate.rows,
        rstate.rows,
        ce.selectivity,
        ce.right_indexed,
        ce.both_sorted,
        params,
    );

    states_eval.* += 1;
    const cand_cost: f64 = lstate.cost + rstate.cost + jp.total_cost;
    const cand_cart: u16 = lstate.cartesian_count + rstate.cartesian_count + (if (ce.has_edge) @as(u16, 0) else @as(u16, 1));
    const cur = dp_work[mask];

    let better: bool = false;
    if (!cur.valid) {
        better = true;
    } else if (cand_cart < cur.cartesian_count) {
        better = true;
    } else if (cand_cart == cur.cartesian_count && cand_cost < cur.cost) {
        better = true;
    }

    if (better) {
        dp_work[mask] = DpJoinState{
            cost: cand_cost,
            rows: jp.output_rows,
            left_mask: left_mask,
            right_mask: right_mask,
            algo: jp.algo,
            cartesian_count: cand_cart,
            is_sorted: jp.algo == JoinAlgoKind.merge_join,
            valid: true,
        };
    }
}

/// Plans the globally optimal multi-table join order using System-R / Selinger bitmask Dynamic Programming
/// over caller-supplied `dp_work` (`len >= 1 << tables.len`, up to `QCOST_DP_MAX_TABLES = 14`).
///
/// - When `allow_bushy` is `false`, restricts search to left-deep trees (`|right| == 1`).
/// - When `allow_bushy` is `true`, explores full bushy join trees (`left | right = mask`).
/// - Lexicographically minimizes Cartesian products first, then total estimated I/O+CPU cost.
/// - Writes the left-to-right leaf evaluation order (table IDs) into `out_eval_order[0..tables.len]`.
pub fn planOptimalJoinOrderDp(
    tables: []const TableStats,
    edges: []const JoinGraphEdge,
    allow_bushy: bool,
    params: CostModelParams,
    dp_work: []DpJoinState,
    out_eval_order: []u16,
): DpJoinPlanResult ! QueryOptError {
    if (tables.len == 0) {
        return QueryOptError.InvalidTables;
    }
    if (tables.len > QCOST_DP_MAX_TABLES) {
        return QueryOptError.UnsupportedDpTableCount;
    }
    const req_len: usize = dpJoinWorkRequiredLen(tables.len);
    if (dp_work.len < req_len || out_eval_order.len < tables.len) {
        return QueryOptError.BufferTooSmall;
    }
    validateJoinEdges(tables.len, edges)?;

    // Reset DP table
    let zi: usize = 0;
    while (zi < req_len) : (zi += 1) {
        dp_work[zi] = DpJoinState{};
    }

    let states_evaluated: usize = 0;

    // Base cases: single-table scans
    let ti: usize = 0;
    while (ti < tables.len) : (ti += 1) {
        const mask: usize = @as(usize, 1) << (ti as u6);
        const scan = chooseBestAccessPath(tables[ti], 1.0, params);
        dp_work[mask] = DpJoinState{
            cost: scan.total_cost,
            rows: nonNegF64(tables[ti].row_count),
            left_mask: 0,
            right_mask: 0,
            algo: JoinAlgoKind.nested_loop,
            cartesian_count: 0,
            is_sorted: tables[ti].is_sorted_on_join_key,
            valid: true,
        };
        states_evaluated += 1;
    }

    // Bottom-up DP by subset cardinality `sz = 2 .. tables.len`
    const full_mask: usize = req_len - 1;
    let sz: u32 = 2;
    while (sz <= (tables.len as u32)) : (sz += 1) {
        let mask: usize = 1;
        while (mask <= full_mask) : (mask += 1) {
            if (popcountU16(mask as u16) != sz) continue;

            if (!allow_bushy) {
                // Left-deep: join any (sz - 1)-subset `left` with single table `right`
                let r: usize = 0;
                while (r < tables.len) : (r += 1) {
                    const rbit: u16 = @as(u16, 1) << (r as u4);
                    if (((mask as u16) & rbit) == 0) continue;
                    const lmask: u16 = (mask as u16) ^ rbit;
                    tryRelaxDpState(dp_work, mask, lmask, rbit, tables, edges, params, *states_evaluated);
                }
            } else {
                // Bushy: enumerate all non-empty proper submasks `sub` of `mask`
                const m16: u16 = mask as u16;
                let sub: u16 = (m16 - 1) & m16;
                while (sub > 0) : (sub = (sub - 1) & m16) {
                    const other: u16 = m16 ^ sub;
                    tryRelaxDpState(dp_work, mask, sub, other, tables, edges, params, *states_evaluated);
                }
            }
        }
    }

    const root = dp_work[full_mask];
    if (!root.valid) {
        return QueryOptError.InvalidTables;
    }

    // Extract left-to-right leaf evaluation order & detect whether optimal plan is bushy
    let stack: [32]u16 = [0; 32];
    let sp: usize = 0;
    stack[sp] = full_mask as u16;
    sp += 1;

    let out_pos: usize = 0;
    let is_bushy: bool = false;
    while (sp > 0) {
        sp -= 1;
        const cur_mask: u16 = stack[sp];
        const st = dp_work[cur_mask as usize];
        if (st.left_mask == 0 && st.right_mask == 0) {
            const idx = trailingZeroIndexU16(cur_mask);
            if (out_pos < tables.len) {
                out_eval_order[out_pos] = tables[idx].table_id;
                out_pos += 1;
            }
        } else {
            if (popcountU16(st.left_mask) > 1 && popcountU16(st.right_mask) > 1) {
                is_bushy = true;
            }
            // Push right first so left is popped and visited first
            stack[sp] = st.right_mask;
            sp += 1;
            stack[sp] = st.left_mask;
            sp += 1;
        }
    }

    return DpJoinPlanResult{
        table_count: tables.len,
        is_bushy: is_bushy,
        total_cost: root.cost,
        final_rows: root.rows,
        states_evaluated: states_evaluated,
        cartesian_joins_used: root.cartesian_count as usize,
    };
}

// ============================================================================
// 9. Unbounded External-Slice Greedy Join Order Planner (32 / 64+ Tables)
// ============================================================================

/// Plans a left-deep join order for an arbitrary number of tables (32/64+ tables, no fixed cap)
/// using caller-provided working slices `used_flags`, `out_order`, and `out_algos`.
///
/// Strongly prefers graph-connected next tables (avoiding Cartesian products whenever an edge
/// connects an unused table to the already-joined prefix) and picks the lowest incremental cost.
pub fn planGreedyJoinOrderUnbounded(
    tables: []const TableStats,
    edges: []const JoinGraphEdge,
    params: CostModelParams,
    used_flags: []bool,
    out_order: []u16,
    out_algos: []JoinAlgoKind,
): DpJoinPlanResult ! QueryOptError {
    const n: usize = tables.len;
    if (n == 0) {
        return QueryOptError.InvalidTables;
    }
    if (used_flags.len < n || out_order.len < n || out_algos.len < n) {
        return QueryOptError.BufferTooSmall;
    }
    validateJoinEdges(n, edges)?;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        used_flags[i] = false;
    }

    // Step 0: pick driving table with smallest row_count (tie-break by scan cost, then table_id)
    let best_first: usize = 0;
    let best_first_rows: f64 = nonNegF64(tables[0].row_count);
    let best_first_scan: f64 = chooseBestAccessPath(tables[0], 1.0, params).total_cost;
    let ti: usize = 1;
    while (ti < n) : (ti += 1) {
        const r: f64 = nonNegF64(tables[ti].row_count);
        const sc: f64 = chooseBestAccessPath(tables[ti], 1.0, params).total_cost;
        if (r < best_first_rows || (r == best_first_rows && (sc < best_first_scan || (sc == best_first_scan && tables[ti].table_id < tables[best_first].table_id)))) {
            best_first = ti;
            best_first_rows = r;
            best_first_scan = sc;
        }
    }

    used_flags[best_first] = true;
    out_order[0] = tables[best_first].table_id;
    out_algos[0] = JoinAlgoKind.nested_loop;

    let acc_rows: f64 = best_first_rows;
    let acc_cost: f64 = best_first_scan;
    let acc_sorted: bool = tables[best_first].is_sorted_on_join_key;
    let states_evaluated: usize = n;
    let cartesian_used: usize = 0;

    // Steps 1 .. n-1: greedily attach best connected unused table (falling back to Cartesian only if disconnected)
    let step: usize = 1;
    while (step < n) : (step += 1) {
        let best_idx: usize = n;
        let best_connected: bool = false;
        let best_step_cost: f64 = 0.0;
        let best_join = JoinPlanCost{};

        let j: usize = 0;
        while (j < n) : (j += 1) {
            if (used_flags[j]) continue;

            // Aggregate all join edges between `j` and any already-used table
            let has_edge: bool = false;
            let eff_sel: f64 = 1.0;
            let edge_ridx: bool = tables[j].has_index;
            let edge_sorted: bool = acc_sorted && tables[j].is_sorted_on_join_key;

            let ei: usize = 0;
            while (ei < edges.len) : (ei += 1) {
                const e = edges[ei];
                const l_idx: usize = e.left_idx as usize;
                const r_idx: usize = e.right_idx as usize;
                const match_lr: bool = used_flags[l_idx] && (r_idx == j);
                const match_rl: bool = used_flags[r_idx] && (l_idx == j);
                if (match_lr || match_rl) {
                    has_edge = true;
                    eff_sel = clampSelectivity(eff_sel * clampSelectivity(e.selectivity));
                    if (match_lr && e.right_indexed) edge_ridx = true;
                    if (e.both_sorted) edge_sorted = true;
                }
            }

            if (!has_edge) {
                eff_sel = 1.0;
                edge_ridx = false;
                edge_sorted = false;
            }

            const scan_c: f64 = chooseBestAccessPath(tables[j], 1.0, params).total_cost;
            const jp = chooseBestJoinAlgo(acc_rows, tables[j].row_count, eff_sel, edge_ridx, edge_sorted, params);
            const total_step: f64 = scan_c + jp.total_cost;
            states_evaluated += 1;

            let take: bool = false;
            if (best_idx == n) {
                take = true;
            } else if (has_edge && !best_connected) {
                take = true;
            } else if (has_edge == best_connected) {
                if (total_step < best_step_cost) {
                    take = true;
                } else if (total_step == best_step_cost && nonNegF64(tables[j].row_count) < nonNegF64(tables[best_idx].row_count)) {
                    take = true;
                }
            }

            if (take) {
                best_idx = j;
                best_connected = has_edge;
                best_step_cost = total_step;
                best_join = jp;
            }
        }

        used_flags[best_idx] = true;
        out_order[step] = tables[best_idx].table_id;
        out_algos[step] = best_join.algo;
        acc_cost += best_step_cost;
        acc_rows = best_join.output_rows;
        acc_sorted = (best_join.algo == JoinAlgoKind.merge_join);
        if (!best_connected) {
            cartesian_used += 1;
        }
    }

    return DpJoinPlanResult{
        table_count: n,
        is_bushy: false,
        total_cost: acc_cost,
        final_rows: acc_rows,
        states_evaluated: states_evaluated,
        cartesian_joins_used: cartesian_used,
    };
}
