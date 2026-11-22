// Origin: Pure-CP Stdlib Test Suite (lib/obs/flameFold.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Sampling Profiler & Folded Flamegraph Exporter:
//   1. Symbol table registration, duplicate/empty guards, & FNV-1a stack signature aggregation
//   2. Self-Time vs Inclusive-Time Top-K hotspot ranking & basis-point share accuracy
//   3. Recursion-safe Inclusive-Time deduplication (main -> fib -> fib -> fib counted once)
//   4. Standard Brendan Gregg Folded Stack export (weight-descending exact text match)
//   5. Unresolved symbol fallback (`sym#<id>`) & delimiter sanitization
//   6. Differential flamegraph regression comparison (`diffHotspots`) across baseline vs current
//   7. Deep stack truncation (>32 frames), empty stack, zero weight, & undersized export buffer
//   8. Full stack-table (128 stacks) overflow guard, clearSamples(), & full reset()

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FLAME_MAX_STACKS,
    FLAME_MAX_DEPTH,
    FlameRecordStatus,
    FlameHotspot,
    FlameHotspotDiff,
    FlameFoldProfiler,
    diffHotspots,
} from "cap:obs";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("flameFold: symbol registration & identical call-stack signature aggregation", () => {
    let prof = FlameFoldProfiler.init();
    expect_eq(prof.registerSymbol(1, ""), false);
    expect_eq(prof.registerSymbol(1, "main"), true);
    expect_eq(prof.registerSymbol(1, "main_dup"), false);
    expect_eq(prof.registerSymbol(2, "dispatch"), true);
    expect_eq(prof.registerSymbol(3, "matmul"), true);

    expect_eq(strEq(prof.resolveSymbolName(3) ?? "", "matmul"), true);
    expect_eq(prof.resolveSymbolName(99) == null, true);

    const s1: [3]u32 = [1, 2, 3];
    const s2: [2]u32 = [1, 2];
    expect_eq(prof.recordStack(s1[0..], 30), FlameRecordStatus.ok);
    expect_eq(prof.recordStack(s1[0..], 45), FlameRecordStatus.ok); // aggregates with s1 -> 75
    expect_eq(prof.recordStack(s2[0..], 25), FlameRecordStatus.ok);

    const st = prof.stats();
    expect_eq(st.symbol_count, 3 as usize);
    expect_eq(st.unique_stacks, 2 as usize);
    expect_eq(st.total_samples, 3 as u64);
    expect_eq(st.total_weight, 100 as u64);
});

test("flameFold: Self-Time vs Inclusive-Time Top-K hotspot ranking & bps shares", () => {
    let prof = FlameFoldProfiler.init();
    // Total weight = 100:
    //   [main, parse, lex] -> 60 (leaf = lex)
    //   [main, parse]      -> 10 (leaf = parse)
    //   [main, render]     -> 30 (leaf = render)
    // Self:      lex=60 (6000 bps), render=30 (3000 bps), parse=10 (1000 bps), main=0
    // Inclusive: main=100 (10000 bps), parse=70 (7000 bps), lex=60 (6000 bps), render=30 (3000 bps)
    const st1: [3]u32 = [1, 2, 3];
    const st2: [2]u32 = [1, 2];
    const st3: [2]u32 = [1, 4];
    * = prof.recordStack(st1[0..], 60);
    * = prof.recordStack(st2[0..], 10);
    * = prof.recordStack(st3[0..], 30);

    let by_self: [4]FlameHotspot = [FlameHotspot {}; 4];
    let self_s: []FlameHotspot = by_self[0..];
    expect_eq(prof.topHotspots(true, *self_s), 4 as usize);
    expect_eq(by_self[0].symbol_id, 3 as u32); // lex
    expect_eq(by_self[0].self_weight, 60 as u64);
    expect_eq(by_self[0].self_bps, 6000 as u32);
    expect_eq(by_self[1].symbol_id, 4 as u32); // render
    expect_eq(by_self[1].self_weight, 30 as u64);
    expect_eq(by_self[2].symbol_id, 2 as u32); // parse
    expect_eq(by_self[2].self_weight, 10 as u64);

    let by_inc: [4]FlameHotspot = [FlameHotspot {}; 4];
    let inc_s: []FlameHotspot = by_inc[0..];
    expect_eq(prof.topHotspots(false, *inc_s), 4 as usize);
    expect_eq(by_inc[0].symbol_id, 1 as u32); // main
    expect_eq(by_inc[0].inclusive_weight, 100 as u64);
    expect_eq(by_inc[0].inclusive_bps, 10000 as u32);
    expect_eq(by_inc[1].symbol_id, 2 as u32); // parse
    expect_eq(by_inc[1].inclusive_weight, 70 as u64);
    expect_eq(by_inc[1].inclusive_bps, 7000 as u32);
});

test("flameFold: recursion-safe Inclusive-Time deduplication", () => {
    let prof = FlameFoldProfiler.init();
    // Recursive stack: main(1) -> fib(2) -> fib(2) -> fib(2) with weight 100
    const rec: [4]u32 = [1, 2, 2, 2];
    expect_eq(prof.recordStack(rec[0..], 100), FlameRecordStatus.ok);

    let hot: [4]FlameHotspot = [FlameHotspot {}; 4];
    let hot_s: []FlameHotspot = hot[0..];
    expect_eq(prof.topHotspots(true, *hot_s), 2 as usize);

    // fib(2) must have self=100 AND inclusive=100 (NOT 300!)
    expect_eq(hot[0].symbol_id, 2 as u32);
    expect_eq(hot[0].self_weight, 100 as u64);
    expect_eq(hot[0].inclusive_weight, 100 as u64);
    expect_eq(hot[0].inclusive_bps, 10000 as u32);

    // main(1) has self=0, inclusive=100
    expect_eq(hot[1].symbol_id, 1 as u32);
    expect_eq(hot[1].self_weight, 0 as u64);
    expect_eq(hot[1].inclusive_weight, 100 as u64);
});

test("flameFold: Brendan Gregg Folded Stack exact text export", () => {
    let prof = FlameFoldProfiler.init();
    * = prof.registerSymbol(1, "main");
    * = prof.registerSymbol(2, "encode");
    * = prof.registerSymbol(3, "hash");

    const s_small: [2]u32 = [1, 2];
    const s_big: [3]u32 = [1, 2, 3];
    * = prof.recordStack(s_small[0..], 15);
    * = prof.recordStack(s_big[0..], 80);

    let out: [256]u8 = [0; 256];
    let out_s: []u8 = out[0..];
    const n = prof.exportFoldedText(*out_s) ?? unreachable;

    // Descending by weight: "main;encode;hash 80\nmain;encode 15\n"
    const expected: []const u8 = "main;encode;hash 80\nmain;encode 15\n";
    expect_eq(strEq(out[0..n], expected), true);
});

test("flameFold: unresolved symbol sym#<id> fallback & delimiter sanitization", () => {
    let prof = FlameFoldProfiler.init();
    * = prof.registerSymbol(1, "foo;bar"); // ';' sanitized to '_' on export
    // Symbol 404 intentionally unregistered -> falls back to "sym#404"
    const s: [2]u32 = [1, 404];
    * = prof.recordStack(s[0..], 7);

    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];
    const n = prof.exportFoldedText(*out_s) ?? unreachable;
    expect_eq(strEq(out[0..n], "foo_bar;sym#404 7\n"), true);
});

test("flameFold: differential flamegraph hotspot regression comparator (diffHotspots)", () => {
    let base = FlameFoldProfiler.init();
    let cur = FlameFoldProfiler.init();

    // Baseline: sym 2 = 20, sym 3 = 80 (total 100)
    const s2: [2]u32 = [1, 2];
    const s3: [2]u32 = [1, 3];
    const s4: [2]u32 = [1, 4];
    * = base.recordStack(s2[0..], 20);
    * = base.recordStack(s3[0..], 80);

    // Current: sym 2 regressed to 70 (+50), sym 3 improved to 30 (-50), sym 4 newly appeared (+25)
    * = cur.recordStack(s2[0..], 70);
    * = cur.recordStack(s3[0..], 30);
    * = cur.recordStack(s4[0..], 25);

    let diffs: [8]FlameHotspotDiff = [FlameHotspotDiff {}; 8];
    let diffs_s: []FlameHotspotDiff = diffs[0..];
    const n = diffHotspots(base, cur, true, *diffs_s);
    expect_eq(n, 4 as usize); // symbols 1, 2, 3, 4

    // Largest regression first: sym 2 (+50), then new sym 4 (+25), then sym 1 (0), then improved sym 3 (-50)
    expect_eq(diffs[0].symbol_id, 2 as u32);
    expect_eq(diffs[0].delta_weight, 50 as i64);
    expect_eq(diffs[1].symbol_id, 4 as u32);
    expect_eq(diffs[1].baseline_weight, 0 as u64);
    expect_eq(diffs[1].current_weight, 25 as u64);
    expect_eq(diffs[1].delta_weight, 25 as i64);
    expect_eq(diffs[3].symbol_id, 3 as u32);
    expect_eq(diffs[3].delta_weight, -50 as i64);
});

test("flameFold: deep stack truncation, empty/zero guards & undersized export buffer", () => {
    let prof = FlameFoldProfiler.init();

    // Empty stack & zero weight rejected
    const empty: [0]u32 = [];
    const one: [1]u32 = [1];
    expect_eq(prof.recordStack(empty[0..], 10), FlameRecordStatus.empty_stack);
    expect_eq(prof.recordStack(one[0..], 0), FlameRecordStatus.zero_weight);

    // Stack deeper than FLAME_MAX_DEPTH (32): 40 frames truncated cleanly
    let deep: [40]u32 = [0; 40];
    let i: usize = 0;
    while (i < 40) : (i += 1) {
        deep[i] = (i + 1) as u32;
    }
    expect_eq(prof.recordStack(deep[0..], 12), FlameRecordStatus.ok_truncated);
    expect_eq(prof.stats().truncated_samples, 1 as u64);
    expect_eq(prof.stacks[0].depth, FLAME_MAX_DEPTH as u8);

    // Undersized export buffer returns null safely
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(prof.exportFoldedText(*tiny_s) == null, true);
});

test("flameFold: 128-stack overflow guard, clearSamples() & reset()", () => {
    let prof = FlameFoldProfiler.init();
    * = prof.registerSymbol(1, "root");

    let i: u32 = 0;
    while (i < FLAME_MAX_STACKS as u32) : (i += 1) {
        const s: [2]u32 = [1, i + 100];
        expect_eq(prof.recordStack(s[0..], 1), FlameRecordStatus.ok);
    }
    expect_eq(prof.stats().unique_stacks, FLAME_MAX_STACKS);

    // 129th distinct stack rejected with table_full
    const extra: [2]u32 = [1, 9999];
    expect_eq(prof.recordStack(extra[0..], 5), FlameRecordStatus.table_full);
    expect_eq(prof.stats().dropped_overflow, 1 as u64);

    // clearSamples keeps symbol table intact while clearing stacks
    prof.clearSamples();
    expect_eq(prof.stats().unique_stacks, 0 as usize);
    expect_eq(prof.stats().symbol_count, 1 as usize);
    expect_eq(prof.resolveSymbolName(1) != null, true);

    // reset clears symbols as well
    prof.reset();
    expect_eq(prof.stats().symbol_count, 0 as usize);
    expect_eq(prof.resolveSymbolName(1) == null, true);
});
