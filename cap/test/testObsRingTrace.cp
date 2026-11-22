// Origin: Pure-CP Stdlib Test Suite (lib/obs/ringTrace.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Distributed Tracing Flight Recorder / Ring Span Buffer:
//   1. O(1) ring buffer recording, per-trace extraction & oldest-span overwrite past 128 capacity
//   2. Auto-freeze on error span protects forensic snapshot even when primary ring is flushed by 200+ new spans
//   3. Auto-freeze on high-latency threshold & manual freezeTrace / unfreeze lifecycle
//   4. Strictly iterative Critical Path, Self-Time, Max-Depth & Bottleneck Span analysis
//   5. Overlapping parallel children self-time saturation & malformed cycle-safe analysis
//   6. Zero-alloc trace summary & Chrome Trace Event Format JSON line exporter (with escaping & bounds guard)
//   7. Full boundary guards (zero IDs, self-parent, empty/oversized op/tag, clear) & cap.obs facade

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    RING_TRACE_CAPACITY,
    RingSpanStatus,
    RingFreezeReason,
    RingSpan,
    SpanSelfTime,
    RingTraceRecorder,
    analyzeTraceSpans,
    formatTraceSummary,
    formatChromeTraceEvent,
} from "cap:obs";

fn containsBytes(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        let ok: bool = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

// ============================================================================
// Test 1: Ring Overwrite Past 128 Capacity & Per-Trace Extraction
// ============================================================================
test("ringTrace: O(1) ring buffer overwrites oldest spans cleanly past 128 capacity", () => {
    let rec = RingTraceRecorder.init(0, false);

    // Record 150 spans across trace 1 (early, gets overwritten) and trace 2 (recent)
    let i: u64 = 1;
    while (i <= 150) : (i += 1) {
        const tid: u64 = if (i <= 22) 1 else 2;
        expect_true(rec.recordSpan(tid, i, 0, "op", "", i * 10, 5, RingSpanStatus.ok));
    }

    const st = rec.stats();
    expect_eq(st.buffered_spans, RING_TRACE_CAPACITY);
    expect_eq(st.total_recorded, 150);
    expect_eq(st.total_overwritten, 22);

    // Trace 1 (spans 1..22) was completely evicted; Trace 2 (spans 23..150) has all 128 spans
    let out: [16]RingSpan = [RingSpan.empty(); 16];
    let out_s: []RingSpan = out[0..];
    expect_eq(rec.extractTrace(1, *out_s), 0);
    expect_eq(rec.extractTrace(2, *out_s), 16);
    expect_eq(out[0].span_id, 23); // oldest surviving span
});

// ============================================================================
// Test 2: Error-Triggered Snapshot Freeze Survives Ring Flush
// ============================================================================
test("ringTrace: error span auto-freezes trace snapshot and survives 200 new spans", () => {
    let rec = RingTraceRecorder.init(0, true);

    expect_true(rec.recordSpan(77, 1, 0, "gateway", "env=prod", 1000, 500, RingSpanStatus.ok));
    expect_true(rec.recordSpan(77, 2, 1, "auth", "", 1050, 50, RingSpanStatus.ok));
    expect_true(rec.recordSpan(77, 3, 1, "db.exec", "err=deadlock", 1100, 300, RingSpanStatus.err));

    const st1 = rec.stats();
    expect_true(st1.frozen);
    expect_eq(st1.frozen_trace_id, 77);
    expect_eq(st1.frozen_span_count, 3);
    expect_eq(st1.freeze_reason, RingFreezeReason.error_span);

    // Flood 200 spans of trace 88 to completely flush the 128-slot primary ring
    let k: u64 = 1;
    while (k <= 200) : (k += 1) {
        expect_true(rec.recordSpan(88, 1000 + k, 0, "noise", "", k * 10, 1, RingSpanStatus.ok));
    }

    // Primary ring no longer has trace 77, but frozen snapshot still preserves all 3 spans!
    let tmp: [4]RingSpan = [RingSpan.empty(); 4];
    let tmp_s: []RingSpan = tmp[0..];
    expect_eq(rec.extractTrace(77, *tmp_s), 0);
    const snap = rec.frozenSpans();
    expect_eq(snap.len, 3);
    expect_eq(snap[0].span_id, 1);
    expect_eq(snap[2].span_id, 3);
    expect_eq(snap[2].status, RingSpanStatus.err);

    // After unfreeze, a new error trace can be captured
    rec.unfreeze();
    expect_true(!rec.stats().frozen);
    expect_true(rec.recordSpan(99, 500, 0, "worker", "oom", 5000, 10, RingSpanStatus.err));
    expect_eq(rec.stats().frozen_trace_id, 99);
});

// ============================================================================
// Test 3: High-Latency Auto-Freeze & Manual Freeze
// ============================================================================
test("ringTrace: high-latency threshold auto-freezes & manual freezeTrace works", () => {
    let rec = RingTraceRecorder.init(10_000, false);

    expect_true(rec.recordSpan(10, 1, 0, "fast_op", "", 0, 200, RingSpanStatus.ok));
    expect_true(!rec.stats().frozen);

    // Span with duration 12000 >= 10000 triggers high_latency freeze
    expect_true(rec.recordSpan(10, 2, 1, "slow_llm", "model=70b", 200, 12_000, RingSpanStatus.ok));
    expect_true(rec.stats().frozen);
    expect_eq(rec.stats().freeze_reason, RingFreezeReason.high_latency);
    expect_eq(rec.frozenSpans().len, 2);

    // Manual freeze replaces/captures a specific trace after unfreeze
    rec.unfreeze();
    expect_true(rec.recordSpan(20, 10, 0, "manual_op", "", 100, 50, RingSpanStatus.ok));
    expect_eq(rec.freezeTrace(20), 1);
    expect_eq(rec.stats().freeze_reason, RingFreezeReason.manual);
    expect_eq(rec.stats().frozen_trace_id, 20);
});

// ============================================================================
// Test 4: Iterative Critical Path, Self-Time, Max-Depth & Bottleneck Analysis
// ============================================================================
test("ringTrace: analyzeTraceSpans computes critical path, self-time, max depth & bottleneck", () => {
    let rec = RingTraceRecorder.init(0, false);
    // Tree structure for trace 55:
    //   span 1 (root):      start=1000, dur=1000 (children: 2[100] + 3[700] => self=200)
    //   span 2 (fast):      parent=1, start=1050, dur=100 (leaf => self=100)
    //   span 3 (slow_mid):  parent=1, start=1150, dur=700 (child: 4[550] => self=150)
    //   span 4 (db_query):  parent=3, start=1200, dur=550 (leaf => self=550, err)
    expect_true(rec.recordSpan(55, 1, 0, "http.post", "", 1000, 1000, RingSpanStatus.ok));
    expect_true(rec.recordSpan(55, 2, 1, "cache.get", "", 1050, 100, RingSpanStatus.ok));
    expect_true(rec.recordSpan(55, 3, 1, "svc.process", "", 1150, 700, RingSpanStatus.ok));
    expect_true(rec.recordSpan(55, 4, 3, "db.query", "slow", 1200, 550, RingSpanStatus.err));

    let spans: [8]RingSpan = [RingSpan.empty(); 8];
    let spans_s: []RingSpan = spans[0..];
    const n = rec.extractTrace(55, *spans_s);
    expect_eq(n, 4);

    let selfs: [8]SpanSelfTime = [SpanSelfTime{}; 8];
    let selfs_s: []SpanSelfTime = selfs[0..];
    const an = analyzeTraceSpans(spans[0..n], *selfs_s) ?? return;

    expect_eq(an.trace_id, 55);
    expect_eq(an.span_count, 4);
    expect_eq(an.error_span_count, 1);
    expect_eq(an.total_wall_us, 1000);
    expect_eq(an.max_depth, 2);
    expect_eq(an.bottleneck_span_id, 4);
    expect_eq(an.bottleneck_self_us, 550);
    // Critical path: 1 (self 200) -> 3 (self 150) -> 4 (leaf 550) = 900us
    expect_eq(an.critical_path_us, 900);
    const cp = an.criticalPathSlice();
    expect_eq(cp.len, 3);
    expect_eq(cp[0], 1);
    expect_eq(cp[1], 3);
    expect_eq(cp[2], 4);

    // Check per-span self times & critical path flags
    expect_eq(selfs[0].self_us, 200);
    expect_true(selfs[0].is_on_critical_path);
    expect_eq(selfs[1].self_us, 100);
    expect_true(!selfs[1].is_on_critical_path);
    expect_eq(selfs[2].self_us, 150);
    expect_true(selfs[2].is_on_critical_path);
    expect_eq(selfs[3].self_us, 550);
    expect_true(selfs[3].is_on_critical_path);
});

// ============================================================================
// Test 5: Parallel Overlapping Children Saturation & Cycle Safety
// ============================================================================
test("ringTrace: parallel children saturate self_us to 0 and malformed parent cycles terminate", () => {
    let rec = RingTraceRecorder.init(0, false);
    // Two parallel children each 400us under a 500us parent -> sum=800 > 500 -> parent self_us == 0
    expect_true(rec.recordSpan(66, 1, 0, "fanout", "", 0, 500, RingSpanStatus.ok));
    expect_true(rec.recordSpan(66, 2, 1, "rpc_a", "", 50, 400, RingSpanStatus.ok));
    expect_true(rec.recordSpan(66, 3, 1, "rpc_b", "", 50, 400, RingSpanStatus.ok));

    let spans: [4]RingSpan = [RingSpan.empty(); 4];
    let spans_s: []RingSpan = spans[0..];
    const n = rec.extractTrace(66, *spans_s);
    let selfs: [4]SpanSelfTime = [SpanSelfTime{}; 4];
    let selfs_s: []SpanSelfTime = selfs[0..];
    const an = analyzeTraceSpans(spans[0..n], *selfs_s) ?? return;
    expect_eq(selfs[0].self_us, 0);
    expect_eq(an.bottleneck_self_us, 400);

    // Malformed mutual cycle (span 10 -> parent 11, span 11 -> parent 10) must terminate safely
    let cyc_rec = RingTraceRecorder.init(0, false);
    expect_true(cyc_rec.recordSpan(67, 10, 11, "a", "", 0, 100, RingSpanStatus.ok));
    expect_true(cyc_rec.recordSpan(67, 11, 10, "b", "", 0, 100, RingSpanStatus.ok));
    const cn = cyc_rec.extractTrace(67, *spans_s);
    expect_true(analyzeTraceSpans(spans[0..cn], *selfs_s) != null);
});

// ============================================================================
// Test 6: Zero-Alloc Summary & Chrome Trace Event JSON Line Exporter
// ============================================================================
test("ringTrace: formatTraceSummary & formatChromeTraceEvent emit valid lines with bounds guard", () => {
    let rec = RingTraceRecorder.init(0, false);
    expect_true(rec.recordSpan(123, 10, 0, "http \"GET\"", "code=500", 5000, 250, RingSpanStatus.err));

    let spans: [2]RingSpan = [RingSpan.empty(); 2];
    let spans_s: []RingSpan = spans[0..];
    const n = rec.extractTrace(123, *spans_s);
    let selfs: [2]SpanSelfTime = [SpanSelfTime{}; 2];
    let selfs_s: []SpanSelfTime = selfs[0..];
    const an = analyzeTraceSpans(spans[0..n], *selfs_s) ?? return;

    let sum_buf: [160]u8 = [0; 160];
    let sum_s: []u8 = sum_buf[0..];
    const sn = formatTraceSummary(an, *sum_s) ?? 0;
    expect_true(sn > 0);
    expect_true(containsBytes(sum_buf[0..sn], "trace=123"));
    expect_true(containsBytes(sum_buf[0..sn], "errors=1"));
    expect_true(containsBytes(sum_buf[0..sn], "bottleneck=10(250us)"));

    let json_buf: [256]u8 = [0; 256];
    let json_s: []u8 = json_buf[0..];
    const jn = formatChromeTraceEvent(spans[0], 42, 7, *json_s) ?? 0;
    expect_true(jn > 0);
    expect_true(containsBytes(json_buf[0..jn], "\"name\":\"http \\\"GET\\\"\""));
    expect_true(containsBytes(json_buf[0..jn], "\"ph\":\"X\""));
    expect_true(containsBytes(json_buf[0..jn], "\"ts\":5000"));
    expect_true(containsBytes(json_buf[0..jn], "\"dur\":250"));
    expect_true(containsBytes(json_buf[0..jn], "\"status\":\"error\""));
    expect_true(containsBytes(json_buf[0..jn], "\"tag\":\"code=500\""));

    // Undersized buffer returns null safely
    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatTraceSummary(an, *tiny_s) == null);
    expect_true(formatChromeTraceEvent(spans[0], 1, 1, *tiny_s) == null);
});

// ============================================================================
// Test 7: Boundary Guards, clear() & cap.obs Facade
// ============================================================================
test("ringTrace: rejects invalid span inputs, clear() resets & cap.obs facade works", () => {
    let rec = cap.obs.RingTraceRecorder.init(0, true);

    expect_true(!rec.recordSpan(0, 1, 0, "op", "", 0, 10, RingSpanStatus.ok));   // trace_id == 0
    expect_true(!rec.recordSpan(1, 0, 0, "op", "", 0, 10, RingSpanStatus.ok));   // span_id == 0
    expect_true(!rec.recordSpan(1, 5, 5, "op", "", 0, 10, RingSpanStatus.ok));   // self-parent
    expect_true(!rec.recordSpan(1, 1, 0, "", "", 0, 10, RingSpanStatus.ok));     // empty op
    const long_op: [49]u8 = ['a'; 49];
    expect_true(!rec.recordSpan(1, 1, 0, long_op[0..], "", 0, 10, RingSpanStatus.ok));

    // Empty slice analysis returns null
    const empty_spans: [0]RingSpan = [];
    let selfs: [1]SpanSelfTime = [SpanSelfTime{}; 1];
    let selfs_s: []SpanSelfTime = selfs[0..];
    expect_true(cap.obs.analyzeTraceSpans(empty_spans[0..], *selfs_s) == null);

    // Valid record + clear
    expect_true(rec.recordSpan(1, 1, 0, "ok_op", "", 100, 50, RingSpanStatus.err));
    expect_true(rec.stats().frozen);
    rec.clear();
    expect_eq(rec.stats().buffered_spans, 0);
    expect_true(!rec.stats().frozen);
});
