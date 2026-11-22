// Origin: Pure-CP Stdlib Test Suite (Vector Metadata Bitset Filter & Selectivity Planner) | Created: 2026-11-22
// Deep Production Verification Suite for lib/vector/filter.cp
// Covers:
//   1. FilterBitset boolean ops (set, clear, test, popcount, andWith, orWith, notInPlace, andNotWith, firstSetBit)
//   2. evalInt64Filter & evalFloat64Filter across Eq/Neq/Gt/Gte/Lt/Lte/Range predicates + compound AND/OR
//   3. planFilterStrategy adaptive routing across low (PreFilterBrute), medium (InlineGraphFilter) & high (PostFilterExpand) selectivity
//   4. maskedTopKCosine bit-jumping Top-K precision & exclusion of higher-scoring unmasked docs
//   5. Empty mask, OOB bits, invalid range, zero dims & NaN boundary defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    FILTER_BITSET_MAX_BITS,
    FilterBitset,
    FilterOp,
    NumericFilter,
    FilterStrategy,
    FilterPlan,
    evalInt64Filter,
    evalFloat64Filter,
    planFilterStrategy,
    maskedTopKCosine,
} from "../../lib/vector/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testBitsetBooleanOpsAndPopcount()! {
    let a = FilterBitset.empty();
    expect_eq(a.popcount(), 0);
    expect_eq(a.firstSetBit() == null, true);

    expect_eq(a.set(5), true);
    expect_eq(a.set(64), true);
    expect_eq(a.set(1023), true);
    expect_eq(a.set(1024), false); // OOB rejected
    expect_eq(a.popcount(), 3);
    expect_eq(a.testBit(5), true);
    expect_eq(a.testBit(64), true);
    expect_eq(a.testBit(1023), true);
    expect_eq(a.testBit(63), false);
    expect_eq(a.firstSetBit() ?? 9999, 5);

    expect_eq(a.clear(5), true);
    expect_eq(a.firstSetBit() ?? 9999, 64);

    let b = FilterBitset.empty();
    expect_eq(b.set(64), true);
    expect_eq(b.set(128), true);

    // AND: {64, 1023} & {64, 128} = {64}
    let inter = a;
    inter.andWith(b);
    expect_eq(inter.popcount(), 1);
    expect_eq(inter.firstSetBit() ?? 0, 64);

    // OR: {64, 1023} | {64, 128} = {64, 128, 1023}
    let uni = a;
    uni.orWith(b);
    expect_eq(uni.popcount(), 3);

    // AND-NOT: {64, 1023} \ {64, 128} = {1023}
    let diff = a;
    diff.andNotWith(b);
    expect_eq(diff.popcount(), 1);
    expect_eq(diff.firstSetBit() ?? 0, 1023);

    // NOT masked to 10 bits: ~{2, 7} over [0..10) has 8 bits set
    let c = FilterBitset.empty();
    expect_eq(c.set(2), true);
    expect_eq(c.set(7), true);
    c.notInPlace(10);
    expect_eq(c.popcount(), 8);
    expect_eq(c.testBit(0), true);
    expect_eq(c.testBit(2), false);
    expect_eq(c.testBit(7), false);
    expect_eq(c.testBit(9), true);
    expect_eq(c.testBit(10), false);
}

fn testNumericAndRangePredicateFilters()! {
    const years: [6]i64 = [2020, 2021, 2022, 2023, 2024, 2025];
    const prices: [6]f64 = [9.9, 19.5, 49.0, 99.0, 150.0, std.math.nan(f64)];

    let m_year = FilterBitset.empty();
    const n_yr = evalInt64Filter(
        years[0..],
        NumericFilter{ op: FilterOp.Range, i64_lo: 2022, i64_hi: 2024 },
        &m_year,
    ) ?? 99;
    expect_eq(n_yr, 3); // indices 2, 3, 4
    expect_eq(m_year.testBit(2), true);
    expect_eq(m_year.testBit(4), true);
    expect_eq(m_year.testBit(5), false);

    let m_price = FilterBitset.empty();
    const n_pr = evalFloat64Filter(
        prices[0..],
        NumericFilter{ op: FilterOp.Lt, f64_lo: 100.0 },
        &m_price,
    ) ?? 99;
    expect_eq(n_pr, 4); // indices 0, 1, 2, 3 (NaN at 5 does NOT match)
    expect_eq(m_price.testBit(5), false);

    // Compound predicate: (year in [2022, 2024]) AND (price < 100.0) -> indices {2, 3}
    m_year.andWith(m_price);
    expect_eq(m_year.popcount(), 2);
    expect_eq(m_year.testBit(2), true);
    expect_eq(m_year.testBit(3), true);
    expect_eq(m_year.testBit(4), false);
}

fn testSelectivityAdaptivePlanner()! {
    // Low selectivity (20 / 1000 = 2% < 5%) -> PreFilterBrute
    if (p_low := planFilterStrategy(1000, 20, 10)) {
        expect_eq(p_low.strategy, FilterStrategy.PreFilterBrute);
        expect_eq(p_low.oversample_factor, 1);
        expect_eq(approxF64(p_low.selectivity, 0.02, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    // Medium selectivity (200 / 1000 = 20%) -> InlineGraphFilter, oversample = ceil(1/0.2) = 5
    if (p_mid := planFilterStrategy(1000, 200, 10)) {
        expect_eq(p_mid.strategy, FilterStrategy.InlineGraphFilter);
        expect_eq(p_mid.oversample_factor, 5);
    } else {
        expect_eq(true, false);
    }

    // High selectivity (500 / 1000 = 50%) -> PostFilterExpand, oversample = ceil(1/0.5) = 2
    if (p_hi := planFilterStrategy(1000, 500, 10)) {
        expect_eq(p_hi.strategy, FilterStrategy.PostFilterExpand);
        expect_eq(p_hi.oversample_factor, 2);
    } else {
        expect_eq(true, false);
    }

    // Small matching set relative to top_k (15 <= 2*10) -> PreFilterBrute even at 15% selectivity
    if (p_small := planFilterStrategy(100, 15, 10)) {
        expect_eq(p_small.strategy, FilterStrategy.PreFilterBrute);
    } else {
        expect_eq(true, false);
    }
}

fn testMaskedTopKCosineSearch()! {
    // 5 docs in 2D; query = [1, 0]
    // doc 0: [1, 0]    cos = 1.0   (FILTERED OUT)
    // doc 1: [0.6,0.8] cos = 0.6   (MASKED IN)
    // doc 2: [0, 1]    cos = 0.0   (MASKED IN)
    // doc 3: [0.8,0.6] cos = 0.8   (MASKED IN)
    // doc 4: [-1, 0]   cos = -1.0  (MASKED IN)
    const query: [2]f64 = [1.0, 0.0];
    const corpus: [10]f64 = [
        1.0, 0.0,
        0.6, 0.8,
        0.0, 1.0,
        0.8, 0.6,
        -1.0, 0.0,
    ];

    let mask = FilterBitset.full(5);
    expect_eq(mask.clear(0), true); // exclude doc 0 despite cos=1.0

    let ids_buf: [2]usize = [0; 2];
    let scores_buf: [2]f64 = [0.0; 2];
    let ids: []usize = ids_buf[0..];
    let scores: []f64 = scores_buf[0..];

    const hits = maskedTopKCosine(query[0..], corpus[0..], 5, 2, mask, 2, ids, scores) ?? 99;
    expect_eq(hits, 2);
    expect_eq(ids_buf[0], 3);
    expect_eq(approxF64(scores_buf[0], 0.8, EPS_F64), true);
    expect_eq(ids_buf[1], 1);
    expect_eq(approxF64(scores_buf[1], 0.6, EPS_F64), true);

    // Empty mask returns 0 hits cleanly
    const empty_mask = FilterBitset.empty();
    expect_eq(maskedTopKCosine(query[0..], corpus[0..], 5, 2, empty_mask, 2, ids, scores) ?? 99, 0);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_val = std.math.nan(f64);
    let mask = FilterBitset.empty();

    // Invalid ranges & NaN threshold
    const ints: [2]i64 = [1, 2];
    const floats: [2]f64 = [1.0, 2.0];
    expect_eq(evalInt64Filter(ints[0..], NumericFilter{ op: FilterOp.Range, i64_lo: 5, i64_hi: 2 }, &mask) == null, true);
    expect_eq(evalFloat64Filter(floats[0..], NumericFilter{ op: FilterOp.Gt, f64_lo: nan_val }, &mask) == null, true);
    expect_eq(evalFloat64Filter(floats[0..], NumericFilter{ op: FilterOp.Range, f64_lo: 5.0, f64_hi: 1.0 }, &mask) == null, true);

    // Invalid planner inputs
    expect_eq(planFilterStrategy(0, 0, 5) == null, true);
    expect_eq(planFilterStrategy(10, 11, 5) == null, true);
    expect_eq(planFilterStrategy(10, 5, 0) == null, true);

    // Invalid maskedTopKCosine inputs
    const q_ok: [2]f64 = [1.0, 0.0];
    const q_nan: [2]f64 = [nan_val, 0.0];
    const c_ok: [4]f64 = [1.0, 0.0, 0.0, 1.0];
    let ids_buf: [2]usize = [0; 2];
    let scores_buf: [2]f64 = [0.0; 2];
    let ids: []usize = ids_buf[0..];
    let scores: []f64 = scores_buf[0..];
    const full2 = FilterBitset.full(2);

    expect_eq(maskedTopKCosine(q_ok[0..], c_ok[0..], 0, 2, full2, 1, ids, scores) == null, true);
    expect_eq(maskedTopKCosine(q_ok[0..], c_ok[0..], FILTER_BITSET_MAX_BITS + 1, 2, full2, 1, ids, scores) == null, true);
    expect_eq(maskedTopKCosine(q_ok[0..], c_ok[0..], 2, 0, full2, 1, ids, scores) == null, true);
    expect_eq(maskedTopKCosine(q_ok[0..], c_ok[0..], 2, 2, full2, 0, ids, scores) == null, true);
    expect_eq(maskedTopKCosine(q_nan[0..], c_ok[0..], 2, 2, full2, 1, ids, scores) == null, true);
    expect_eq(maskedTopKCosine(q_ok[0..], c_ok[0..], 2, 2, full2, 2, ids_buf[0..1], scores) == null, true);
}

fn main() {
    test("1. filter: FilterBitset set/clear/test/popcount/AND/OR/NOT/ANDNOT/firstSetBit", () => {
        testBitsetBooleanOpsAndPopcount()?;
    });
    test("2. filter: evalInt64Filter & evalFloat64Filter range/numeric predicates & compound AND", () => {
        testNumericAndRangePredicateFilters()?;
    });
    test("3. filter: planFilterStrategy adaptive PreFilterBrute/InlineGraphFilter/PostFilterExpand", () => {
        testSelectivityAdaptivePlanner()?;
    });
    test("4. filter: maskedTopKCosine bit-jumping Top-K precision & unmasked exclusion", () => {
        testMaskedTopKCosineSearch()?;
    });
    test("5. filter: OOB bits, invalid range, planner & NaN boundary defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
