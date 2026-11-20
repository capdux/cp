// Source: cp/cap/test/test_perf_production_deep.cp (5 deep tests for cap:perf & cap.perf)

import std from "std";
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    PerfClock,
    PerfError,
    PerfPhase,
    PerfRecorder,
    PerfRecorderOptions,
    PerfTimer,
    exportChromeTraceJson,
    formatSummaryTable,
    freeSummaries,
    summarizeByName,
} from "cap:perf";

test("1. deterministic virtual clock nested spans and begin/end pairing") {
    const alloc = std.testing.allocator;
    let rec = PerfRecorder.init(alloc, PerfClock.virtual(10_000), PerfRecorderOptions{});
    defer rec.deinit();

    let outer = try rec.begin("outer_pass", "compiler");
    expect_eq(rec.openSpanCount(), @as(usize, 1));
    expect_eq(outer.depth, @as(u32, 0));

    rec.clock.advance(5_000);
    let inner = try rec.begin("inner_lex", "compiler");
    expect_eq(rec.openSpanCount(), @as(usize, 2));
    expect_eq(inner.depth, @as(u32, 1));

    rec.clock.advance(2_500);
    const inner_dur = try rec.end(&inner);
    expect_eq(inner_dur, @as(u64, 2_500));
    expect_eq(inner.active, false);
    expect_eq(rec.openSpanCount(), @as(usize, 1));

    rec.clock.advance(1_500);
    const outer_dur = try rec.end(&outer);
    expect_eq(outer_dur, @as(u64, 9_000));
    expect_eq(rec.openSpanCount(), @as(usize, 0));
    expect_eq(rec.eventCount(), @as(usize, 4));

    const events = rec.items();
    expect_eq(events[0].phase == .Begin, true);
    expect_eq(events[1].phase == .Begin, true);
    expect_eq(events[2].phase == .End, true);
    expect_eq(events[3].phase == .End, true);
}

test("2. summarizeByName multi-event aggregation and P95/P99 calculation") {
    const alloc = std.testing.allocator;
    let rec = PerfRecorder.init(alloc, PerfClock.virtual(0), PerfRecorderOptions{});
    defer rec.deinit();

    // Record 20 durations for "parse": 100, 200, ..., 2000 ns
    let i: u64 = 1;
    while (i <= 20) {
        try rec.recordComplete("parse", "frontend", i * 1_000, i * 100);
        i += 1;
    }
    try rec.recordComplete("codegen", "backend", 50_000, 750);
    try rec.recordInstant("checkpoint", "meta");

    const summaries = try summarizeByName(alloc, rec.items());
    defer freeSummaries(alloc, summaries);

    expect_eq(summaries.len, @as(usize, 2));
    expect_eq(std.mem.eql(u8, summaries[0].name, "parse"), true);
    expect_eq(summaries[0].count, @as(usize, 20));
    expect_eq(summaries[0].min_ns, @as(u64, 100));
    expect_eq(summaries[0].max_ns, @as(u64, 2_000));
    expect_eq(summaries[0].total_ns, @as(u64, 21_000));
    expect_eq(summaries[0].avg_ns, @as(u64, 1_050));
    expect_eq(summaries[0].p95_ns, @as(u64, 1_900));
    expect_eq(summaries[0].p99_ns, @as(u64, 2_000));

    expect_eq(std.mem.eql(u8, summaries[1].name, "codegen"), true);
    expect_eq(summaries[1].count, @as(usize, 1));
    expect_eq(summaries[1].p99_ns, @as(u64, 750));

    const table = try formatSummaryTable(alloc, summaries);
    defer alloc.free(table);
    expect_eq(std.mem.indexOf(u8, table, "parse\t20\t21000") != null, true);
}

test("3. Chrome Trace JSON export with RFC 8259 escaping and all B/E/X/C/i phases") {
    const alloc = std.testing.allocator;
    let rec = PerfRecorder.init(alloc, PerfClock.virtual(1_000_000), PerfRecorderOptions{
        default_pid: 42,
        default_tid: 7,
    });
    defer rec.deinit();

    let s = try rec.begin("step\"quote\"\\slash\nline", "cat\t1");
    rec.clock.advance(500_250);
    * = try rec.end(&s);
    try rec.recordComplete("work_x", "io", 2_000_000, 12_500);
    try rec.recordInstant("mark_i", "sync");
    try rec.recordCounter("mem_kb", "mem", 4096);

    const json = try exportChromeTraceJson(alloc, rec.items());
    defer alloc.free(json);

    expect_eq(std.mem.startsWith(u8, json, "{\"traceEvents\":["), true);
    expect_eq(std.mem.endsWith(u8, json, "]}"), true);
    expect_eq(std.mem.indexOf(u8, json, "\\\"quote\\\"\\\\slash\\nline") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"cat\":\"cat\\t1\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"ph\":\"B\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"ph\":\"E\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"ph\":\"X\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"dur\":12.500") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"ph\":\"i\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"s\":\"t\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"ph\":\"C\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"args\":{\"value\":4096}") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"pid\":42,\"tid\":7") != null, true);
}

test("4. boundary guards: unpaired end, empty name, capacity limits") {
    const alloc = std.testing.allocator;
    let rec = PerfRecorder.init(alloc, PerfClock.virtual(100), PerfRecorderOptions{
        max_events: 3,
        max_open_spans: 2,
    });
    defer rec.deinit();

    // Empty name rejected
    let empty_err = false;
    * = rec.begin("", "perf") catch |e| {
        empty_err = (e == PerfError.EmptyName);
    };
    expect_eq(empty_err, true);

    // Unpaired endByName rejected
    let unpaired_err = false;
    * = rec.endByName("missing") catch |e| {
        unpaired_err = (e == PerfError.UnpairedEnd);
    };
    expect_eq(unpaired_err, true);

    // Mismatched LIFO end rejected
    let a = try rec.begin("a", "perf");
    let b = try rec.begin("b", "perf");
    let mismatch_err = false;
    * = rec.end(&a) catch |e| {
        mismatch_err = (e == PerfError.UnpairedEnd);
    };
    expect_eq(mismatch_err, true);

    // Capacity limit (max_events = 3, currently 2 Begin events recorded)
    rec.clock.advance(50);
    * = try rec.end(&b);
    let cap_err = false;
    rec.recordInstant("overflow", "perf") catch |e| {
        cap_err = (e == PerfError.CapacityExceeded);
    };
    expect_eq(cap_err, true);
}

test("5. cap.perf dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;
    let clock = cap.perf.PerfClock.virtual(5_000);
    let timer = PerfTimer.startWithClock(clock);
    expect_eq(timer.readNs(), @as(u64, 0));

    let rec = cap.perf.PerfRecorder.init(alloc, clock, cap.perf.PerfRecorderOptions{});
    defer rec.deinit();

    let span = try rec.begin("mounted_op", "cap_perf");
    rec.clock.advance(1_234);
    const dur = try rec.end(&span);
    expect_eq(dur, @as(u64, 1_234));

    const summaries = try cap.perf.summarizeByName(alloc, rec.items());
    defer cap.perf.freeSummaries(alloc, summaries);
    expect_eq(summaries.len, @as(usize, 1));
    expect_eq(summaries[0].total_ns, @as(u64, 1_234));

    const json = try cap.perf.exportChromeTraceJson(alloc, rec.items());
    defer alloc.free(json);
    expect_eq(std.mem.indexOf(u8, json, "\"mounted_op\"") != null, true);

    // Also verify monotonic clock returns non-zero or valid u64 without crashing
    const mono_now = cap.perf.osMonotonicNs();
    expect_eq(mono_now >= 0, true);
}
