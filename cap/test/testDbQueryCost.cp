// Origin: Pure-CP Stdlib Test Suite (lib/db/queryCost.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Relational Query Cost Estimator & Left-Deep Join Planner:
//   1. Predicate selectivity estimation across all 8 `PredicateKind` variants & AND/OR combinators
//   2. Access path crossover: low selectivity -> `index_scan`, high selectivity -> `seq_scan`
//   3. Covering index preference (`index_only_scan` strictly cheaper than `index_scan`)
//   4. Binary join algorithm selection: small indexed `nested_loop`, large unsorted `hash_join`, sorted `merge_join`
//   5. 4-table greedy left-deep join order planner prioritizing smallest driving table
//   6. Pre-sorted multi-table join leveraging `merge_join` across steps
//   7. Boundary safety (empty tables, zero NDV, selectivity clamping `[1e-6, 1.0]`) & `cap.db` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    QCOST_MAX_TABLES,
    QCOST_DP_MAX_TABLES,
    QCOST_BPS_SCALE,
    QueryOptError,
    CostModelParams,
    PredicateKind,
    AccessPathKind,
    JoinAlgoKind,
    TableStats,
    estimatePredicateSelectivity,
    combineSelectivityAnd,
    combineSelectivityOr,
    chooseBestAccessPath,
    chooseBestJoinAlgo,
    planLeftDeepJoinOrder,
    McvEntry,
    HistBucket,
    ColumnHistogram,
    buildEquiDepthHistogram,
    estimateEqSelectivityHist,
    estimateRangeSelectivityHist,
    JoinGraphEdge,
    DpJoinState,
    dpJoinWorkRequiredLen,
    planOptimalJoinOrderDp,
    planGreedyJoinOrderUnbounded,
} from "cap:db";

fn approxEq(a: f64, b: f64, tol: f64): bool {
    const d: f64 = if (a > b) a - b else b - a;
    return d <= tol;
}

test("QueryCost: predicate selectivity estimation & AND/OR combinators") {
    // 10% nulls (1000 bps), ndv = 100
    const s_eq = estimatePredicateSelectivity(PredicateKind.eq, 100.0, 1000, 0, 0.0);
    expect_true(approxEq(s_eq, 0.009, 1.0e-6)); // 0.9 / 100

    const s_neq = estimatePredicateSelectivity(PredicateKind.neq, 100.0, 1000, 0, 0.0);
    expect_true(approxEq(s_neq, 0.891, 1.0e-6)); // 0.9 * 0.99

    const s_null = estimatePredicateSelectivity(PredicateKind.is_null, 100.0, 1000, 0, 0.0);
    expect_true(approxEq(s_null, 0.1, 1.0e-6));

    const s_not_null = estimatePredicateSelectivity(PredicateKind.is_not_null, 100.0, 1000, 0, 0.0);
    expect_true(approxEq(s_not_null, 0.9, 1.0e-6));

    const s_in = estimatePredicateSelectivity(PredicateKind.in_list, 100.0, 0, 5, 0.0);
    expect_true(approxEq(s_in, 0.05, 1.0e-6));

    const s_range_default = estimatePredicateSelectivity(PredicateKind.range_between, 100.0, 0, 0, 0.0);
    expect_true(approxEq(s_range_default, 0.25, 1.0e-6));

    const s_range_hist = estimatePredicateSelectivity(PredicateKind.range_half, 100.0, 0, 0, 0.15);
    expect_true(approxEq(s_range_hist, 0.15, 1.0e-6));

    const s_like = estimatePredicateSelectivity(PredicateKind.like_prefix, 100.0, 0, 0, 0.0);
    expect_true(approxEq(s_like, 0.1, 1.0e-6));

    // Combinators
    expect_true(approxEq(combineSelectivityAnd(0.2, 0.5), 0.1, 1.0e-6));
    expect_true(approxEq(combineSelectivityOr(0.2, 0.5), 0.6, 1.0e-6)); // 0.2 + 0.5 - 0.1
}

test("QueryCost: low selectivity selects index_scan vs high selectivity switches to seq_scan") {
    const params = CostModelParams.defaults();
    const tbl = TableStats{
        table_id: 1,
        row_count: 100000.0,
        page_count: 2000.0,
        ndv: 10000.0,
        has_index: true,
        is_covering_index: false,
    };

    // Low selectivity (0.1% -> 100 rows): index_scan wins easily
    const low_plan = chooseBestAccessPath(tbl, 0.001, params);
    expect_true(low_plan.kind == AccessPathKind.index_scan);
    expect_true(approxEq(low_plan.estimated_rows, 100.0, 1.0e-4));

    // High selectivity (50% -> 50000 rows): random heap fetches exceed seq scan -> seq_scan wins
    const high_plan = chooseBestAccessPath(tbl, 0.5, params);
    expect_true(high_plan.kind == AccessPathKind.seq_scan);
    expect_true(approxEq(high_plan.estimated_rows, 50000.0, 1.0e-4));
    expect_true(low_plan.total_cost < high_plan.total_cost);
}

test("QueryCost: covering index prioritizes index_only_scan") {
    const params = CostModelParams.defaults();
    const tbl_non_cov = TableStats{
        table_id: 2,
        row_count: 50000.0,
        page_count: 1000.0,
        has_index: true,
        is_covering_index: false,
    };
    const tbl_cov = TableStats{
        table_id: 2,
        row_count: 50000.0,
        page_count: 1000.0,
        has_index: true,
        is_covering_index: true,
    };

    const p_idx = chooseBestAccessPath(tbl_non_cov, 0.02, params);
    const p_cov = chooseBestAccessPath(tbl_cov, 0.02, params);
    expect_true(p_cov.kind == AccessPathKind.index_only_scan);
    expect_true(p_cov.total_cost < p_idx.total_cost);
}

test("QueryCost: join algorithm selection (nested_loop vs hash_join vs merge_join)") {
    const params = CostModelParams.defaults();

    // 1. Tiny outer (5 rows) + indexed inner (10000 rows) -> nested_loop wins (avoids hash setup + full scan)
    const nl = chooseBestJoinAlgo(5.0, 10000.0, 0.0001, true, false, params);
    expect_true(nl.algo == JoinAlgoKind.nested_loop);

    // 2. Large unsorted relations (10000 x 10000) -> hash_join wins over O(N*M) nested loop & O(N log N) sort merge
    const hj = chooseBestJoinAlgo(10000.0, 10000.0, 0.0001, false, false, params);
    expect_true(hj.algo == JoinAlgoKind.hash_join);

    // 3. Large pre-sorted relations (10000 x 10000, both_sorted=true) -> merge_join wins with zero sort cost
    const mj = chooseBestJoinAlgo(10000.0, 10000.0, 0.0001, false, true, params);
    expect_true(mj.algo == JoinAlgoKind.merge_join);
    expect_true(mj.total_cost < hj.total_cost);
}

test("QueryCost: 4-table left-deep join order planner picks smallest driving table first") {
    const params = CostModelParams.defaults();
    const tables: [4]TableStats = [
        TableStats{ table_id: 10, row_count: 50000.0, page_count: 1000.0, has_index: true },
        TableStats{ table_id: 20, row_count: 50.0, page_count: 2.0, has_index: true },
        TableStats{ table_id: 30, row_count: 5000.0, page_count: 100.0, has_index: true },
        TableStats{ table_id: 40, row_count: 500.0, page_count: 10.0, has_index: true },
    ];

    const plan = planLeftDeepJoinOrder(tables[0..], 0.001, params);
    expect_eq(plan.table_count, @as(usize, 4));
    // Smallest table (id=20, 50 rows) must be chosen as the driving table
    expect_eq(plan.order[0], @as(u16, 20));
    // Next smallest (id=40, 500 rows) joins second, followed by 30 (5000), then 10 (50000)
    expect_eq(plan.order[1], @as(u16, 40));
    expect_eq(plan.order[2], @as(u16, 30));
    expect_eq(plan.order[3], @as(u16, 10));
    expect_true(plan.total_cost > 0.0);
    expect_true(plan.final_rows > 0.0);
}

test("QueryCost: pre-sorted tables leverage merge_join in multi-table plan") {
    const params = CostModelParams.defaults();
    const tables: [2]TableStats = [
        TableStats{ table_id: 1, row_count: 8000.0, page_count: 160.0, is_sorted_on_join_key: true },
        TableStats{ table_id: 2, row_count: 12000.0, page_count: 240.0, is_sorted_on_join_key: true },
    ];

    const plan = planLeftDeepJoinOrder(tables[0..], 0.0001, params);
    expect_eq(plan.table_count, @as(usize, 2));
    expect_eq(plan.order[0], @as(u16, 1));
    expect_eq(plan.order[1], @as(u16, 2));
    expect_true(plan.step_algos[1] == JoinAlgoKind.merge_join);
}

test("QueryCost: empty table, zero NDV, clamping boundaries & cap.db facade parity") {
    const params = CostModelParams.defaults();

    // Zero NDV & huge NDV clamped strictly into [1e-6, 1.0]
    const s_zero_ndv = estimatePredicateSelectivity(PredicateKind.eq, 0.0, 0, 0, 0.0);
    expect_true(approxEq(s_zero_ndv, 1.0, 1.0e-6));
    const s_huge_ndv = estimatePredicateSelectivity(PredicateKind.eq, 1.0e12, 0, 0, 0.0);
    expect_true(approxEq(s_huge_ndv, 1.0e-6, 1.0e-9));
    const s_empty_in = estimatePredicateSelectivity(PredicateKind.in_list, 100.0, 0, 0, 0.0);
    expect_true(approxEq(s_empty_in, 1.0e-6, 1.0e-9));

    // Empty table access path & empty join plan
    const empty_tbl = TableStats{ table_id: 9, row_count: 0.0, page_count: 0.0, has_index: true };
    const ap = chooseBestAccessPath(empty_tbl, 0.1, params);
    expect_true(ap.kind == AccessPathKind.seq_scan);
    expect_true(approxEq(ap.total_cost, 0.0, 1.0e-9));

    const one_tbl: [1]TableStats = [empty_tbl];
    const empty_slice: []const TableStats = one_tbl[0..0];
    const empty_plan = planLeftDeepJoinOrder(empty_slice, 0.01, params);
    expect_eq(empty_plan.table_count, @as(usize, 0));
    expect_true(approxEq(empty_plan.total_cost, 0.0, 1.0e-9));

    // `cap.db` facade parity
    expect_eq(cap.db.QCOST_MAX_TABLES, @as(usize, 8));
    expect_eq(cap.db.QCOST_BPS_SCALE, @as(u16, 10000));
}

test("QueryCost MCV + Histogram: skewed Zipf heavy-hitter equality & range accuracy vs uniform") {
    // 100,000 rows, 1,000 distinct values, 5% NULLs.
    // Heavy hitters: value=1 (45%), value=2 (20%), value=3 (10%) -> MCV total = 75%, residual = 20% across 997 values.
    const mcvs: [3]McvEntry = [
        McvEntry{ value: 1.0, freq: 0.45 },
        McvEntry{ value: 2.0, freq: 0.20 },
        McvEntry{ value: 3.0, freq: 0.10 },
    ];
    const buckets: [4]HistBucket = [
        HistBucket{ low: 10.0, high: 250.0, distinct_count: 250.0 },
        HistBucket{ low: 251.0, high: 500.0, distinct_count: 250.0 },
        HistBucket{ low: 501.0, high: 750.0, distinct_count: 250.0 },
        HistBucket{ low: 751.0, high: 1000.0, distinct_count: 247.0 },
    ];
    const hist = ColumnHistogram{
        mcvs: mcvs[0..],
        buckets: buckets[0..],
        null_frac: 0.05,
        ndv: 1000.0,
        total_rows: 100000.0,
    };

    // Uniform estimator severely underestimates heavy hitter: 0.95 / 1000 = 0.00095 (473x error!)
    const uniform_eq = estimatePredicateSelectivity(PredicateKind.eq, 1000.0, 500, 0, 0.0);
    const mcv_eq = estimateEqSelectivityHist(hist, 1.0);
    expect_true(approxEq(mcv_eq, 0.45, 1.0e-9));
    expect_true(mcv_eq > uniform_eq * 100.0);

    // Non-MCV tail value: residual mass 0.20 / (1000 - 3) = 0.0002006018...
    const tail_eq = estimateEqSelectivityHist(hist, 420.0);
    expect_true(approxEq(tail_eq, 0.20 / 997.0, 1.0e-9));

    // Range [1.0, 2.0] inclusive captures MCV(1)=0.45 + MCV(2)=0.20 = 0.65 (no buckets overlap)
    const mcv_range = estimateRangeSelectivityHist(hist, 1.0, 2.0, true, true);
    expect_true(approxEq(mcv_range, 0.65, 1.0e-9));

    // Range (1.0, 2.0] exclusive of 1.0 captures only MCV(2)=0.20
    const mcv_range_excl = estimateRangeSelectivityHist(hist, 1.0, 2.0, false, true);
    expect_true(approxEq(mcv_range_excl, 0.20, 1.0e-9));

    // Combined MCV + bucket range: [2.0, 500.0] -> MCV(2)+MCV(3) = 0.30 + 2 full buckets (2 * 0.05 = 0.10) = 0.40
    const combined_range = estimateRangeSelectivityHist(hist, 2.0, 500.0, true, true);
    expect_true(approxEq(combined_range, 0.40, 1.0e-6));
}

test("QueryCost Equi-Depth Histogram: buildEquiDepthHistogram & linear boundary interpolation") {
    // 20 sorted values: 0, 10, 20, ..., 190 -> 4 equi-depth buckets of 5 elements each
    let vals: [20]f64 = [0.0; 20];
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        vals[i] = (i as f64) * 10.0;
    }

    let buckets: [4]HistBucket = [HistBucket{}; 4];
    const nb = buildEquiDepthHistogram(vals[0..], 4, buckets[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(nb, @as(usize, 4));
    // Bucket 0: [0, 40], Bucket 1: [50, 90], Bucket 2: [100, 140], Bucket 3: [150, 190]
    expect_true(approxEq(buckets[0].low, 0.0, 1.0e-9));
    expect_true(approxEq(buckets[0].high, 40.0, 1.0e-9));
    expect_true(approxEq(buckets[0].distinct_count, 5.0, 1.0e-9));
    expect_true(approxEq(buckets[3].low, 150.0, 1.0e-9));
    expect_true(approxEq(buckets[3].high, 190.0, 1.0e-9));

    const empty_mcvs: [0]McvEntry = [];
    const hist = ColumnHistogram{
        mcvs: empty_mcvs[0..],
        buckets: buckets[0..nb],
        null_frac: 0.0,
        ndv: 20.0,
        total_rows: 20.0,
    };

    // Query [20.0, 120.0]:
    //   Bucket 0 [0,40]: overlap [20,40] = 20/40 = 0.5 -> 0.5 * 0.25 = 0.125
    //   Bucket 1 [50,90]: fully covered -> 0.25
    //   Bucket 2 [100,140]: overlap [100,120] = 20/40 = 0.5 -> 0.5 * 0.25 = 0.125
    //   Bucket 3 [150,190]: disjoint -> 0.0
    //   Total = 0.50
    const sel = estimateRangeSelectivityHist(hist, 20.0, 120.0, true, true);
    expect_true(approxEq(sel, 0.50, 1.0e-6));

    // Full span covers all 4 buckets => 1.0
    const full_sel = estimateRangeSelectivityHist(hist, -10.0, 500.0, true, true);
    expect_true(approxEq(full_sel, 1.0, 1.0e-6));
}

test("QueryCost Selinger DP: Bushy join tree strictly beats Left-Deep on 4-table chain") {
    const params = CostModelParams.defaults();
    // 4-table chain: T0 -- T1 -- T2 -- T3
    // T0 (10000) ⋈ T1 (5000) with sel=1e-6 -> 50 rows
    // T2 (10000) ⋈ T3 (5000) with sel=1e-6 -> 50 rows
    // Middle edge T1 ⋈ T2 has loose selectivity 0.05!
    // Left-Deep must join (T0⋈T1=50) with T2 (10000) -> 50*10000*0.05 = 25,000 intermediate rows before T3!
    // Bushy computes (T0⋈T1=50) and (T2⋈T3=50) independently, then joins 50x50 -> massive win!
    const tables: [4]TableStats = [
        TableStats{ table_id: 101, row_count: 10000.0, page_count: 200.0 },
        TableStats{ table_id: 102, row_count: 5000.0, page_count: 100.0 },
        TableStats{ table_id: 103, row_count: 10000.0, page_count: 200.0 },
        TableStats{ table_id: 104, row_count: 5000.0, page_count: 100.0 },
    ];
    const edges: [3]JoinGraphEdge = [
        JoinGraphEdge{ left_idx: 0, right_idx: 1, selectivity: 1.0e-6 },
        JoinGraphEdge{ left_idx: 1, right_idx: 2, selectivity: 0.05 },
        JoinGraphEdge{ left_idx: 2, right_idx: 3, selectivity: 1.0e-6 },
    ];

    expect_eq(dpJoinWorkRequiredLen(4), @as(usize, 16));
    let dp_work: [16]DpJoinState = [DpJoinState{}; 16];
    let order_ld: [4]u16 = [0; 4];
    let order_bushy: [4]u16 = [0; 4];

    const res_ld = planOptimalJoinOrderDp(tables[0..], edges[0..], false, params, dp_work[0..], order_ld[0..]) catch {
        expect_true(false);
        return;
    };
    const res_bushy = planOptimalJoinOrderDp(tables[0..], edges[0..], true, params, dp_work[0..], order_bushy[0..]) catch {
        expect_true(false);
        return;
    };

    expect_eq(res_ld.table_count, @as(usize, 4));
    expect_eq(res_ld.cartesian_joins_used, @as(usize, 0));
    expect_true(!res_ld.is_bushy);

    expect_eq(res_bushy.table_count, @as(usize, 4));
    expect_eq(res_bushy.cartesian_joins_used, @as(usize, 0));
    expect_true(res_bushy.is_bushy);
    // Bushy tree must be strictly cheaper than Left-Deep on this classic symmetric chain
    expect_true(res_bushy.total_cost < res_ld.total_cost);
    expect_true(approxEq(res_bushy.final_rows, res_ld.final_rows, 1.0e-3));
}

test("QueryCost Selinger DP: 5-table star schema avoids Cartesian products & beats naive greedy") {
    const params = CostModelParams.defaults();
    // Star schema: Fact (idx 0, 100k rows) connected to 4 dimensions:
    //   Dim1 (idx 1, 10 rows, loose sel=0.5 -> expands Fact to 500k if joined first!)
    //   Dim2 (idx 2, 500 rows, ultra-selective sel=1e-5 -> shrinks Fact to 500 rows!)
    //   Dim3 (idx 3, 200 rows, sel=1e-3)
    //   Dim4 (idx 4, 300 rows, sel=1e-3)
    // Naive greedy picks Dim1 (smallest: 10 rows) -> connects to Fact (500k rows) -> expensive!
    // Selinger DP picks Fact ⋈ Dim2 first (shrinking to 500 rows) -> strictly lower total cost!
    const tables: [5]TableStats = [
        TableStats{ table_id: 1, row_count: 100000.0, page_count: 2000.0, has_index: true },
        TableStats{ table_id: 2, row_count: 10.0, page_count: 1.0, has_index: true },
        TableStats{ table_id: 3, row_count: 500.0, page_count: 10.0, has_index: true },
        TableStats{ table_id: 4, row_count: 200.0, page_count: 5.0, has_index: true },
        TableStats{ table_id: 5, row_count: 300.0, page_count: 6.0, has_index: true },
    ];
    const edges: [4]JoinGraphEdge = [
        JoinGraphEdge{ left_idx: 0, right_idx: 1, selectivity: 0.5, right_indexed: true },
        JoinGraphEdge{ left_idx: 0, right_idx: 2, selectivity: 1.0e-5, right_indexed: true },
        JoinGraphEdge{ left_idx: 0, right_idx: 3, selectivity: 1.0e-3, right_indexed: true },
        JoinGraphEdge{ left_idx: 0, right_idx: 4, selectivity: 1.0e-3, right_indexed: true },
    ];

    let dp_work: [32]DpJoinState = [DpJoinState{}; 32];
    let dp_order: [5]u16 = [0; 5];
    const dp_res = planOptimalJoinOrderDp(tables[0..], edges[0..], true, params, dp_work[0..], dp_order[0..]) catch {
        expect_true(false);
        return;
    };

    let used: [5]bool = [false; 5];
    let g_order: [5]u16 = [0; 5];
    let g_algos: [5]JoinAlgoKind = [JoinAlgoKind.hash_join; 5];
    const g_res = planGreedyJoinOrderUnbounded(tables[0..], edges[0..], params, used[0..], g_order[0..], g_algos[0..]) catch {
        expect_true(false);
        return;
    };

    expect_eq(dp_res.cartesian_joins_used, @as(usize, 0));
    expect_eq(g_res.cartesian_joins_used, @as(usize, 0));
    expect_true(dp_res.total_cost < g_res.total_cost);
    expect_true(dp_res.states_evaluated > 5);
}

test("QueryCost Unbounded Greedy: 32-table analytical chain query with zero Cartesian products") {
    const params = CostModelParams.defaults();
    let tables: [32]TableStats = [TableStats{}; 32];
    let edges: [31]JoinGraphEdge = [JoinGraphEdge{}; 31];

    let i: usize = 0;
    while (i < 32) : (i += 1) {
        tables[i] = TableStats{
            table_id: (1000 + i) as u16,
            row_count: ((32 - i) as f64) * 100.0,
            page_count: ((32 - i) as f64) * 2.0,
            has_index: true,
        };
        if (i < 31) {
            edges[i] = JoinGraphEdge{
                left_idx: i as u16,
                right_idx: (i + 1) as u16,
                selectivity: 0.002,
                right_indexed: true,
            };
        }
    }

    let used: [32]bool = [false; 32];
    let order: [32]u16 = [0; 32];
    let algos: [32]JoinAlgoKind = [JoinAlgoKind.hash_join; 32];

    const res = planGreedyJoinOrderUnbounded(tables[0..], edges[0..], params, used[0..], order[0..], algos[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(res.table_count, @as(usize, 32));
    expect_eq(res.cartesian_joins_used, @as(usize, 0));
    // Smallest table is idx=31 (id=1031, 100 rows); chain must walk 1031 -> 1030 -> ... -> 1000
    expect_eq(order[0], @as(u16, 1031));
    expect_eq(order[1], @as(u16, 1030));
    expect_eq(order[31], @as(u16, 1000));
    expect_true(res.total_cost > 0.0);
}

test("QueryCost Error Guards & cap.db facade parity for Histogram + DP + Unbounded planners") {
    const params = CostModelParams.defaults();

    // 1. InvalidHistogram (unsorted input)
    const unsorted: [3]f64 = [10.0, 5.0, 20.0];
    let bbuf: [4]HistBucket = [HistBucket{}; 4];
    let got_bad_hist: bool = false;
    * = cap.db.buildEquiDepthHistogram(unsorted[0..], 2, bbuf[0..]) catch |e| {
        if (e == QueryOptError.InvalidHistogram) got_bad_hist = true;
    };
    expect_true(got_bad_hist);

    // 2. BufferTooSmall in buildEquiDepthHistogram
    const sorted: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    let tiny_b: [1]HistBucket = [HistBucket{}; 1];
    let got_hist_small: bool = false;
    * = buildEquiDepthHistogram(sorted[0..], 3, tiny_b[0..]) catch |e| {
        if (e == QueryOptError.BufferTooSmall) got_hist_small = true;
    };
    expect_true(got_hist_small);

    // 3. UnsupportedDpTableCount (> 14 tables)
    let many_t: [15]TableStats = [TableStats{}; 15];
    const empty_e: [0]JoinGraphEdge = [];
    let tiny_dp: [16]DpJoinState = [DpJoinState{}; 16];
    let ord15: [15]u16 = [0; 15];
    let got_unsup: bool = false;
    * = cap.db.planOptimalJoinOrderDp(many_t[0..], empty_e[0..], false, params, tiny_dp[0..], ord15[0..]) catch |e| {
        if (e == QueryOptError.UnsupportedDpTableCount) got_unsup = true;
    };
    expect_true(got_unsup);
    expect_eq(cap.db.dpJoinWorkRequiredLen(15), @as(usize, 0));
    expect_eq(cap.db.QCOST_DP_MAX_TABLES, QCOST_DP_MAX_TABLES);

    // 4. InvalidEdge (out of bounds / self-loop)
    let two_t: [2]TableStats = [
        TableStats{ table_id: 1, row_count: 100.0, page_count: 2.0 },
        TableStats{ table_id: 2, row_count: 200.0, page_count: 4.0 },
    ];
    const bad_e: [1]JoinGraphEdge = [JoinGraphEdge{ left_idx: 0, right_idx: 0, selectivity: 0.1 }];
    let got_edge: bool = false;
    * = planOptimalJoinOrderDp(two_t[0..], bad_e[0..], true, params, tiny_dp[0..], ord15[0..2]) catch |e| {
        if (e == QueryOptError.InvalidEdge) got_edge = true;
    };
    expect_true(got_edge);

    // 5. BufferTooSmall in planOptimalJoinOrderDp
    let small_dp: [2]DpJoinState = [DpJoinState{}; 2];
    let got_dp_small: bool = false;
    * = planOptimalJoinOrderDp(two_t[0..], empty_e[0..], false, params, small_dp[0..], ord15[0..2]) catch |e| {
        if (e == QueryOptError.BufferTooSmall) got_dp_small = true;
    };
    expect_true(got_dp_small);
}
