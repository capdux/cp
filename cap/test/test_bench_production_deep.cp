// Source: cp/cap/test/test_bench_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:bench

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    HwTimer,
    Stopwatch,
    BenchStats,
    LatencyHistogram,
    TracePhase,
    TraceEvent,
    TraceRingBuffer,
    PerfTrace,
    BenchCaseResult,
    BenchSuite,
    blackBox,
    BenchOptions,
    BenchResult,
    BenchRunner,
    BenchVerdict,
    BenchComparison,
    BenchTableFormatter,
    AllocSnapshot,
    CountingAllocator,
} from "cap:bench";

fn hotLoopWorkload() {
    let acc: u64 = 0;
    let i: u64 = 0;
    while (i < 64) {
        acc = acc +% (i * 7 + 3);
        i += 1;
    }
    * = blackBox(acc);
}

fn testBlackBoxAndRunner(): !void {
    // 1. blackBox barrier preserves values across scalar and slice types
    const in_u64: u64 = 0x123456789ABCDEF0;
    const v_u64 = blackBox(in_u64);
    expect_eq(v_u64, 0x123456789ABCDEF0);
    const in_f64: f64 = 3.14159;
    const v_f64 = blackBox(in_f64);
    expect_eq(v_f64 > 3.14 && v_f64 < 3.15, true);
    const str_in: []const u8 = "cp-bench-ok";
    const v_str = blackBox(str_in);
    expect_eq(std.mem.eql(u8, v_str, "cp-bench-ok"), true);
    const v_inferred = blackBox(42 as u64);
    expect_eq(v_inferred, 42);

    // 2. Deterministic runSamples with unsorted input (verifies non-mutating sort & exact quantiles)
    const raw_samples: [10]u64 = [900, 100, 500, 300, 700, 200, 800, 400, 600, 1000];
    const res = BenchRunner.runSamples("deterministic_sort", &raw_samples);
    expect_eq(std.mem.eql(u8, res.name, "deterministic_sort"), true);
    expect_eq(res.samples.len, 10);
    expect_eq(res.min_ns, 100);
    expect_eq(res.max_ns, 1000);
    expect_eq(res.mean_ns > 549.9 && res.mean_ns < 550.1, true);
    expect_eq(res.stddev_ns > 302.7 && res.stddev_ns < 302.9, true);
    expect_eq(res.p50_ns, 500);
    expect_eq(res.p95_ns, 900);
    expect_eq(res.p99_ns, 900);
    expect_eq(res.ops_per_sec > 1818000.0 && res.ops_per_sec < 1818500.0, true);
    // Input slice remains untouched
    expect_eq(raw_samples[0], 900);
    expect_eq(raw_samples[1], 100);

    // 3. Empty sample slice edge case
    const empty_slice: []const u64 = [];
    const empty_res = BenchRunner.runSamples("empty", empty_slice);
    expect_eq(empty_res.samples.len, 0);
    expect_eq(empty_res.min_ns, 0);
    expect_eq(empty_res.max_ns, 0);
    expect_eq(empty_res.mean_ns, 0.0);
    expect_eq(empty_res.ops_per_sec, 0.0);

    // 4. Live BenchRunner.measure with warmup & calibrated sample sizing
    const opts = BenchOptions {
        warmup_iters: 3,
        sample_count: 15,
        min_sample_ns: 2000,
    };
    const measured = BenchRunner.measure("hot_loop_64", opts, hotLoopWorkload);
    expect_eq(std.mem.eql(u8, measured.name, "hot_loop_64"), true);
    expect_eq(measured.samples.len, 15);
    expect_eq(measured.effectiveSampleCount(), 15);
    expect_eq(measured.min_ns <= measured.p50_ns, true);
    expect_eq(measured.p50_ns <= measured.p95_ns, true);
    expect_eq(measured.p95_ns <= measured.p99_ns, true);
    expect_eq(measured.p99_ns <= measured.max_ns, true);
}

fn testWelchComparison(): !void {
    const base_samples: [10]u64 = [980, 990, 1000, 1010, 1020, 985, 995, 1005, 1015, 1000];
    const fast_samples: [10]u64 = [490, 495, 500, 505, 510, 492, 498, 502, 508, 500];
    const slow_samples: [10]u64 = [1960, 1980, 2000, 2020, 2040, 1970, 1990, 2010, 2030, 2000];
    const tiny_diff_samples: [10]u64 = [965, 975, 985, 995, 1005, 970, 980, 990, 1000, 985];

    const baseline = BenchRunner.runSamples("v1_baseline", &base_samples);
    const faster = BenchRunner.runSamples("v2_fast", &fast_samples);
    const slower = BenchRunner.runSamples("v2_slow", &slow_samples);
    const tiny_diff = BenchRunner.runSamples("v2_tiny", &tiny_diff_samples);

    // 1. Faster verdict (~2x speedup)
    const cmp_fast = BenchComparison.compare(baseline, faster, 5.0);
    expect_eq(cmp_fast.verdict == BenchVerdict.Faster, true);
    expect_eq(cmp_fast.is_significant, true);
    expect_eq(cmp_fast.change_pct < -49.0 && cmp_fast.change_pct > -51.0, true);
    expect_eq(cmp_fast.speedup > 1.99 && cmp_fast.speedup < 2.01, true);
    expect_eq(cmp_fast.t_statistic < -50.0, true);
    expect_eq(cmp_fast.degrees_of_freedom > 10.0, true);

    // 2. Slower verdict (~2x regression)
    const cmp_slow = BenchComparison.compare(baseline, slower, 5.0);
    expect_eq(cmp_slow.verdict == BenchVerdict.Slower, true);
    expect_eq(cmp_slow.is_significant, true);
    expect_eq(cmp_slow.change_pct > 99.0 && cmp_slow.change_pct < 101.0, true);
    expect_eq(cmp_slow.speedup > 0.49 && cmp_slow.speedup < 0.51, true);
    expect_eq(cmp_slow.t_statistic > 50.0, true);

    // 3. NoChange due to statistical insignificance (high noise, small shift)
    const noisy_a: [6]u64 = [700, 1300, 800, 1200, 900, 1100];
    const noisy_b: [6]u64 = [720, 1320, 820, 1220, 920, 1120];
    const res_na = BenchRunner.runSamples("noisy_a", &noisy_a);
    const res_nb = BenchRunner.runSamples("noisy_b", &noisy_b);
    const cmp_noisy = BenchComparison.compare(res_na, res_nb, 1.0);
    expect_eq(cmp_noisy.is_significant, false);
    expect_eq(cmp_noisy.verdict == BenchVerdict.NoChange, true);

    // 4. NoChange due to practical threshold_pct (1.5% difference < 5.0% threshold)
    const cmp_below_thresh = BenchComparison.compare(baseline, tiny_diff, 5.0);
    expect_eq(cmp_below_thresh.verdict == BenchVerdict.NoChange, true);

    // 5. Zero-variance deterministic distributions
    const det_a: [4]u64 = [100, 100, 100, 100];
    const det_b: [4]u64 = [50, 50, 50, 50];
    const cmp_det = BenchComparison.compare(
        BenchRunner.runSamples("det_a", &det_a),
        BenchRunner.runSamples("det_b", &det_b),
        5.0
    );
    expect_eq(cmp_det.is_significant, true);
    expect_eq(cmp_det.verdict == BenchVerdict.Faster, true);
}

fn testMarkdownFormatting(): !void {
    const alloc = std.heap.page_allocator;
    const s1: [5]u64 = [280, 290, 300, 310, 320];
    const s2: [5]u64 = [140, 145, 150, 155, 160];
    const r1 = BenchRunner.runSamples("parse_v1", &s1);
    const r2 = BenchRunner.runSamples("parse_v2", &s2);
    const results: [2]BenchResult = [r1, r2];

    const md_table = BenchTableFormatter.formatMarkdown(alloc, &results)?;
    defer alloc.free(md_table);
    expect_eq(std.mem.indexOf(u8, md_table, "| Benchmark | Samples | Min (ns) |") != null, true);
    expect_eq(std.mem.indexOf(u8, md_table, "| parse_v1 | 5 | 280 |") != null, true);
    expect_eq(std.mem.indexOf(u8, md_table, "| parse_v2 | 5 | 140 |") != null, true);

    const cmp = BenchComparison.compare(r1, r2, 5.0);
    const md_cmp = BenchTableFormatter.formatComparison(alloc, cmp)?;
    defer alloc.free(md_cmp);
    expect_eq(std.mem.indexOf(u8, md_cmp, "| Baseline | Candidate |") != null, true);
    expect_eq(std.mem.indexOf(u8, md_cmp, "| parse_v1 | parse_v2 |") != null, true);
    expect_eq(std.mem.indexOf(u8, md_cmp, "2.00x") != null, true);
    expect_eq(std.mem.indexOf(u8, md_cmp, "Faster") != null, true);
}

fn testCountingAllocator(): !void {
    let backing_buf: [8192]u8 = undefined;
    let fba = std.heap.FixedBufferAllocator.init(&backing_buf);
    let tracker = CountingAllocator.init(fba.allocator());
    const alloc = tracker.allocator();

    // 1. Allocate two buffers and check live & peak bytes
    let buf1 = alloc.alloc(u8, 128)?;
    let buf2 = alloc.alloc(u8, 256)?;
    let snap = tracker.snapshot();
    expect_eq(snap.alloc_count, 2);
    expect_eq(snap.free_count, 0);
    expect_eq(snap.total_bytes_allocated, 384);
    expect_eq(snap.live_bytes, 384);
    expect_eq(snap.peak_bytes, 384);
    expect_eq(snap.hasLeaks(), true);
    expect_eq(snap.netAllocCount(), 2);

    // 2. Free buf1 (middle free: FBA decrements live_bytes accurately)
    alloc.free(buf1);
    snap = tracker.snapshot();
    expect_eq(snap.free_count, 1);
    expect_eq(snap.live_bytes, 256);
    expect_eq(snap.peak_bytes, 384);

    // 3. Reallocate buf2 from 256 -> 512 bytes (hits FBA in-place resize / remap)
    buf2 = alloc.realloc(buf2, 512)?;
    snap = tracker.snapshot();
    expect_eq(snap.live_bytes, 512);
    expect_eq(snap.peak_bytes >= 512, true);
    expect_eq(snap.realloc_count + snap.alloc_count >= 3, true);

    // 4. Free remaining buffer -> 0 live bytes (no leaks)
    alloc.free(buf2);
    snap = tracker.snapshot();
    expect_eq(snap.live_bytes, 0);
    expect_eq(snap.hasLeaks(), false);

    // 5. reset() clears all counters for next benchmark iteration
    tracker.reset();
    snap = tracker.snapshot();
    expect_eq(snap.alloc_count, 0);
    expect_eq(snap.free_count, 0);
    expect_eq(snap.realloc_count, 0);
    expect_eq(snap.total_bytes_allocated, 0);
    expect_eq(snap.live_bytes, 0);
    expect_eq(snap.peak_bytes, 0);

    // 6. Verify page_allocator wrapping works cleanly too
    let page_tracker = CountingAllocator.init(std.heap.page_allocator);
    const palloc = page_tracker.allocator();
    const pbuf = palloc.alloc(u8, 64)?;
    expect_eq(page_tracker.snapshot().live_bytes, 64);
    palloc.free(pbuf);
    expect_eq(page_tracker.snapshot().live_bytes, 0);
    expect_eq(page_tracker.snapshot().peak_bytes, 64);
}

fn testBackwardCompatibility(): !void {
    let sw = Stopwatch.start();
    hotLoopWorkload();
    expect_eq(sw.readNs() >= 0, true);
    expect_eq(HwTimer.nowNs() > 0, true);

    let stats = BenchStats.init();
    stats.record(100);
    stats.record(200);
    stats.record(300);
    expect_eq(stats.meanNs(), 200);
    expect_eq(stats.p50Ns(), 200);

    let hist = LatencyHistogram.init();
    hist.record(50);
    hist.record(5000);
    expect_eq(hist.count(), 2);

    let trace = PerfTrace.init();
    trace.instant("compat_check", "bench");
    expect_eq(trace.len(), 1);

    let suite = BenchSuite.init("Compat Suite", 1, 5, 1000, 20);
    suite.run("hot_loop", hotLoopWorkload);
    expect_eq(suite.caseCount(), 1);
}

fn main() {
    test("1. blackBox barrier & BenchRunner deterministic statistics / live measure", () => {
        testBlackBoxAndRunner()?;
    });
    test("2. BenchComparison Welch t-test Faster / Slower / NoChange verdicts", () => {
        testWelchComparison()?;
    });
    test("3. BenchTableFormatter Markdown result & comparison formatting", () => {
        testMarkdownFormatting()?;
    });
    test("4. CountingAllocator exact alloc/free/realloc/peak/live tracking & reset", () => {
        testCountingAllocator()?;
    });
    test("5. cap:bench backward compatibility (HwTimer, Stopwatch, BenchStats, LatencyHistogram, PerfTrace, BenchSuite)", () => {
        testBackwardCompatibility()?;
    });
}
