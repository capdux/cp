// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/factLedger.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Bi-Temporal Agent Fact Ledger:
//   1. Conflict auto-superseding chain (`superseded_by`, valid_to/tx_to closure) & queryCurrentFacts
//   2. Classic bi-temporal `queryAsOf` time-travel across chronological belief transitions
//   3. Retroactive correction: distinguishing "what agent believed before correction" vs "after correction"
//   4. Identical fact reinforcement (zero extra slot) & multi-predicate isolation per subject
//   5. Explicit `retractFact` visibility before/after retraction + unknown/duplicate/monotonic guards
//   6. Complete chronological `tracePredicateHistory` audit trail across active/superseded/retracted
//   7. Bounded valid-time window `[valid_from, valid_to)` half-open interval semantics
//   8. Full boundary guards (invalid_time_range, invalid_confidence, 256-record capacity_full, clear())

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FACT_LEDGER_MAX_RECORDS,
    FACT_TIME_INF,
    FACT_NO_ID,
    FactStatus,
    FactLedgerOpStatus,
    BiTemporalFact,
    BiTemporalFactLedger,
} from "cap:agentmem";

test("factLedger: conflict auto-superseding chain & queryCurrentFacts", () => {
    let ledger = BiTemporalFactLedger.init();
    const SUB: u64 = 1001;
    const PRED_CITY: u64 = 10;

    // t=100: City = Beijing (0x11)
    const r1 = ledger.assertFact(SUB, PRED_CITY, 0x11, 100, 100, 9000);
    expect_eq(r1.status, FactLedgerOpStatus.ok);
    expect_eq(r1.fact_id, 1 as u64);
    expect_eq(r1.superseded_count, 0 as usize);

    // t=200: City = Shanghai (0x22) -> supersedes fact 1
    const r2 = ledger.assertFact(SUB, PRED_CITY, 0x22, 200, 200, 9500);
    expect_eq(r2.status, FactLedgerOpStatus.ok);
    expect_eq(r2.fact_id, 2 as u64);
    expect_eq(r2.superseded_count, 1 as usize);
    expect_eq(r2.first_superseded_id, 1 as u64);

    // t=300: City = Shenzhen (0x33) -> supersedes fact 2
    const r3 = ledger.assertFact(SUB, PRED_CITY, 0x33, 300, 300, 9800);
    expect_eq(r3.status, FactLedgerOpStatus.ok);
    expect_eq(r3.fact_id, 3 as u64);
    expect_eq(r3.superseded_count, 1 as usize);
    expect_eq(r3.first_superseded_id, 2 as u64);

    // Verify closed intervals & superseded_by chain
    const f1 = ledger.getFact(1) ?? unreachable;
    expect_eq(f1.status, FactStatus.superseded);
    expect_eq(f1.valid_to_ms, 200 as i64);
    expect_eq(f1.tx_to_ms, 200 as i64);
    expect_eq(f1.superseded_by, 2 as u64);

    const f2 = ledger.getFact(2) ?? unreachable;
    expect_eq(f2.status, FactStatus.superseded);
    expect_eq(f2.valid_to_ms, 300 as i64);
    expect_eq(f2.tx_to_ms, 300 as i64);
    expect_eq(f2.superseded_by, 3 as u64);

    // queryCurrentFacts at t=350 returns only fact 3
    let out: [4]BiTemporalFact = [BiTemporalFact {}; 4];
    let out_s: []BiTemporalFact = out[0..];
    const n = ledger.queryCurrentFacts(SUB, 350, *out_s);
    expect_eq(n, 1 as usize);
    expect_eq(out[0].fact_id, 3 as u64);
    expect_eq(out[0].object_hash, 0x33 as u64);
});

test("factLedger: bi-temporal queryAsOf time-travel across belief transitions", () => {
    let ledger = BiTemporalFactLedger.init();
    const SUB: u64 = 42;
    const PRED_ROLE: u64 = 7;

    * = ledger.assertFact(SUB, PRED_ROLE, 0xA1, 1000, 1000, 8500);
    * = ledger.assertFact(SUB, PRED_ROLE, 0xB2, 2000, 2000, 9200);
    * = ledger.assertFact(SUB, PRED_ROLE, 0xC3, 3000, 3000, 9900);

    let out: [4]BiTemporalFact = [BiTemporalFact {}; 4];
    let out_s: []BiTemporalFact = out[0..];

    // Before anything was recorded (t=500): 0 facts
    expect_eq(ledger.queryAsOf(SUB, 500, 500, *out_s), 0 as usize);

    // At tx=1500, valid=1500 -> believed 0xA1
    expect_eq(ledger.queryAsOf(SUB, 1500, 1500, *out_s), 1 as usize);
    expect_eq(out[0].object_hash, 0xA1 as u64);

    // At tx=2500, valid=2500 -> believed 0xB2
    expect_eq(ledger.queryAsOf(SUB, 2500, 2500, *out_s), 1 as usize);
    expect_eq(out[0].object_hash, 0xB2 as u64);

    // At tx=3500, valid=3500 -> believed 0xC3
    expect_eq(ledger.queryAsOf(SUB, 3500, 3500, *out_s), 1 as usize);
    expect_eq(out[0].object_hash, 0xC3 as u64);
});

test("factLedger: retroactive correction preserves pre-correction belief in As-Of history", () => {
    let ledger = BiTemporalFactLedger.init();
    const SUB: u64 = 77;
    const PRED_PLAN: u64 = 3;

    // At tx=100, agent mistakenly records Plan = Free (0x01) valid from 100
    * = ledger.assertFact(SUB, PRED_PLAN, 0x01, 100, 100, 7000);

    // At tx=300, agent learns user was actually Pro (0x02) retroactively since valid_from=100
    const corr = ledger.assertFact(SUB, PRED_PLAN, 0x02, 100, 300, 10000);
    expect_eq(corr.status, FactLedgerOpStatus.ok);
    expect_eq(corr.superseded_count, 1 as usize);

    let out: [4]BiTemporalFact = [BiTemporalFact {}; 4];
    let out_s: []BiTemporalFact = out[0..];

    // What did agent believe at tx=200 (before correction) about valid_at=150? -> 0x01 (Free)
    expect_eq(ledger.queryAsOf(SUB, 150, 200, *out_s), 1 as usize);
    expect_eq(out[0].object_hash, 0x01 as u64);

    // What does agent believe at tx=350 (after correction) about valid_at=150? -> 0x02 (Pro)
    expect_eq(ledger.queryAsOf(SUB, 150, 350, *out_s), 1 as usize);
    expect_eq(out[0].object_hash, 0x02 as u64);
});

test("factLedger: identical fact reinforcement & multi-predicate isolation", () => {
    let ledger = BiTemporalFactLedger.init();
    const SUB: u64 = 9;

    const r1 = ledger.assertFactWindow(SUB, 1, 0xABC, 100, FACT_TIME_INF, 100, 7500, 1);
    expect_eq(r1.reinforced_existing, false);

    // Re-asserting identical (subject=9, predicate=1, object=0xABC) reinforces in-place
    const r2 = ledger.assertFactWindow(SUB, 1, 0xABC, 120, FACT_TIME_INF, 120, 9200, 5);
    expect_eq(r2.status, FactLedgerOpStatus.ok);
    expect_eq(r2.reinforced_existing, true);
    expect_eq(r2.fact_id, r1.fact_id);
    expect_eq(ledger.count(), 1 as usize);

    const f = ledger.getFact(r1.fact_id) ?? unreachable;
    expect_eq(f.confidence_bps, 9200 as u16);
    expect_eq(f.source_turn_id, 5 as u64);

    // Different predicate (predicate=2) on same subject does not supersede predicate=1
    const r3 = ledger.assertFact(SUB, 2, 0xDEF, 130, 130, 8800);
    expect_eq(r3.superseded_count, 0 as usize);
    expect_eq(ledger.count(), 2 as usize);

    let out: [4]BiTemporalFact = [BiTemporalFact {}; 4];
    let out_s: []BiTemporalFact = out[0..];
    expect_eq(ledger.queryCurrentFacts(SUB, 200, *out_s), 2 as usize);
});

test("factLedger: explicit retractFact & boundary guards (not found, already closed, tx time)", () => {
    let ledger = BiTemporalFactLedger.init();
    const r = ledger.assertFact(5, 2, 0x777, 100, 100, 8000);

    // Unknown fact_id & backward tx_now_ms rejected
    expect_eq(ledger.retractFact(999, 200), FactLedgerOpStatus.fact_not_found);
    expect_eq(ledger.retractFact(r.fact_id, 50), FactLedgerOpStatus.invalid_tx_time);

    // Valid retraction at tx=250
    expect_eq(ledger.retractFact(r.fact_id, 250), FactLedgerOpStatus.ok);
    // Double retraction rejected
    expect_eq(ledger.retractFact(r.fact_id, 300), FactLedgerOpStatus.already_closed);

    let out: [4]BiTemporalFact = [BiTemporalFact {}; 4];
    let out_s: []BiTemporalFact = out[0..];
    // Not in current facts anymore
    expect_eq(ledger.queryCurrentFacts(5, 300, *out_s), 0 as usize);
    // Still visible in As-Of before retraction (tx=200), gone at/after retraction (tx=250)
    expect_eq(ledger.queryAsOf(5, 150, 200, *out_s), 1 as usize);
    expect_eq(ledger.queryAsOf(5, 150, 250, *out_s), 0 as usize);
});

test("factLedger: tracePredicateHistory chronological audit trail", () => {
    let ledger = BiTemporalFactLedger.init();
    const r1 = ledger.assertFact(10, 20, 0x1, 100, 100, 8000);
    const r2 = ledger.assertFact(10, 20, 0x2, 200, 200, 8500);
    * = ledger.assertFact(10, 99, 0x9, 210, 210, 9000); // unrelated predicate
    * = ledger.retractFact(r2.fact_id, 250);
    const r3 = ledger.assertFact(10, 20, 0x3, 300, 300, 9500);

    let hist: [8]BiTemporalFact = [BiTemporalFact {}; 8];
    let hist_s: []BiTemporalFact = hist[0..];
    const n = ledger.tracePredicateHistory(10, 20, *hist_s);
    expect_eq(n, 3 as usize);
    expect_eq(hist[0].fact_id, r1.fact_id);
    expect_eq(hist[0].status, FactStatus.superseded);
    expect_eq(hist[1].fact_id, r2.fact_id);
    expect_eq(hist[1].status, FactStatus.retracted);
    expect_eq(hist[2].fact_id, r3.fact_id);
    expect_eq(hist[2].status, FactStatus.active);

    const st = ledger.stats();
    expect_eq(st.total_records, 4 as usize);
    expect_eq(st.active_records, 2 as usize);
    expect_eq(st.superseded_records, 1 as usize);
    expect_eq(st.retracted_records, 1 as usize);
});

test("factLedger: bounded valid-time window [valid_from, valid_to) half-open semantics", () => {
    let ledger = BiTemporalFactLedger.init();
    const r = ledger.assertFactWindow(55, 8, 0xBEEF, 1000, 2000, 900, 10000, 12);
    expect_eq(r.status, FactLedgerOpStatus.ok);

    let out: [2]BiTemporalFact = [BiTemporalFact {}; 2];
    let out_s: []BiTemporalFact = out[0..];
    expect_eq(ledger.queryCurrentFacts(55, 999, *out_s), 0 as usize);
    expect_eq(ledger.queryCurrentFacts(55, 1000, *out_s), 1 as usize);
    expect_eq(ledger.queryCurrentFacts(55, 1999, *out_s), 1 as usize);
    expect_eq(ledger.queryCurrentFacts(55, 2000, *out_s), 0 as usize);
});

test("factLedger: boundary guards (invalid range, confidence > 10000, 256-cap atomic, clear)", () => {
    let ledger = BiTemporalFactLedger.init();

    // Invalid valid-time range (from >= to)
    expect_eq(ledger.assertFactWindow(1, 1, 0x1, 500, 500, 500, 9000, 0).status, FactLedgerOpStatus.invalid_time_range);
    expect_eq(ledger.assertFactWindow(1, 1, 0x1, 600, 500, 500, 9000, 0).status, FactLedgerOpStatus.invalid_time_range);

    // Invalid confidence_bps > 10000
    expect_eq(ledger.assertFact(1, 1, 0x1, 100, 100, 10001).status, FactLedgerOpStatus.invalid_confidence);

    // Fill ledger to FACT_LEDGER_MAX_RECORDS (256)
    let i: u64 = 0;
    while (i < FACT_LEDGER_MAX_RECORDS as u64) : (i += 1) {
        expect_eq(ledger.assertFact(i + 1, 1, i + 100, 100, 100, 9000).status, FactLedgerOpStatus.ok);
    }
    expect_eq(ledger.count(), FACT_LEDGER_MAX_RECORDS);

    // Attempting a conflicting write when full must fail atomically WITHOUT closing existing record 1
    const full_res = ledger.assertFact(1, 1, 0xDEAD, 200, 200, 9500);
    expect_eq(full_res.status, FactLedgerOpStatus.capacity_full);
    expect_eq((ledger.getFact(1) ?? unreachable).status, FactStatus.active);

    // clear() resets ledger completely
    ledger.clear();
    expect_eq(ledger.count(), 0 as usize);
    expect_eq(ledger.assertFact(1, 1, 0xDEAD, 200, 200, 9500).fact_id, 1 as u64);
});
