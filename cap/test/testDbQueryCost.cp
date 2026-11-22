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
    QCOST_BPS_SCALE,
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
