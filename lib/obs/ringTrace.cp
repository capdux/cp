// Origin: Pure-CP Stdlib (Zero-Alloc Distributed Tracing Flight Recorder / Ring Span Buffer) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation ring span buffer & trace analyzer:
//   - Fixed 128-slot power-of-two ring span buffer with O(1) oldest-span overwrite
//   - Triggered snapshot freeze on error span or high-latency threshold (isolated 64-slot frozen snapshot)
//   - Strictly iterative (zero-recursion) Critical Path, Self-Time, Max-Depth & Bottleneck Span analysis
//   - Zero-alloc trace summary & Chrome Trace Event Format (chrome://tracing / Perfetto) JSON line exporter

const std = @import("std");

/// Fixed power-of-two capacity of the primary ring span buffer.
pub const RING_TRACE_CAPACITY: usize = 128;

/// Fixed capacity of the isolated frozen trace snapshot buffer.
pub const RING_TRACE_FROZEN_CAPACITY: usize = 64;

/// Maximum byte length of a span operation name.
pub const RING_TRACE_OP_MAX_LEN: usize = 48;

/// Maximum byte length of a span attribute tag string.
pub const RING_TRACE_TAG_MAX_LEN: usize = 48;

// ============================================================================
// Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Execution status of a recorded span.
pub enum RingSpanStatus: u8 {
    ok = 0,
    err = 1,
}

/// Reason why the recorder's frozen snapshot was triggered.
pub enum RingFreezeReason: u8 {
    none = 0,
    manual = 1,
    error_span = 2,
    high_latency = 3,
}

/// Fixed-size distributed tracing span stored in the ring buffer or frozen snapshot.
pub struct RingSpan {
    trace_id: u64 = 0;
    span_id: u64 = 0;
    parent_span_id: u64 = 0;
    op_buf: [RING_TRACE_OP_MAX_LEN]u8 = [0; RING_TRACE_OP_MAX_LEN];
    op_len: usize = 0;
    tag_buf: [RING_TRACE_TAG_MAX_LEN]u8 = [0; RING_TRACE_TAG_MAX_LEN];
    tag_len: usize = 0;
    start_us: u64 = 0;
    duration_us: u64 = 0;
    status: RingSpanStatus = RingSpanStatus.ok;
    occupied: bool = false;

    static fn empty(): RingSpan {
        return RingSpan{};
    }

    fn opSlice(): []const u8 {
        const n: usize = if (_.op_len <= RING_TRACE_OP_MAX_LEN) _.op_len else RING_TRACE_OP_MAX_LEN;
        return _.op_buf[0..n];
    }

    fn tagSlice(): []const u8 {
        const n: usize = if (_.tag_len <= RING_TRACE_TAG_MAX_LEN) _.tag_len else RING_TRACE_TAG_MAX_LEN;
        return _.tag_buf[0..n];
    }

    /// Returns saturated end timestamp `start_us + duration_us`.
    fn endUs(): u64 {
        const max_v: u64 = std.math.maxInt(u64);
        if (_.start_us > max_v - _.duration_us) return max_v;
        return _.start_us + _.duration_us;
    }
}

/// Per-span self-time, tree depth, and critical-path membership computed by `analyzeTraceSpans`.
pub struct SpanSelfTime {
    span_id: u64 = 0;
    total_us: u64 = 0;
    self_us: u64 = 0;
    depth: u16 = 0;
    is_on_critical_path: bool = false;
}

/// Aggregated trace analysis result (critical path, wall time, errors, max depth, bottleneck span).
pub struct TraceAnalysis {
    trace_id: u64 = 0;
    span_count: usize = 0;
    error_span_count: usize = 0;
    total_wall_us: u64 = 0;
    critical_path_us: u64 = 0;
    critical_path_len: usize = 0;
    critical_path_span_ids: [RING_TRACE_FROZEN_CAPACITY]u64 = [0; RING_TRACE_FROZEN_CAPACITY];
    max_depth: u16 = 0;
    bottleneck_span_id: u64 = 0;
    bottleneck_self_us: u64 = 0;

    /// Returns the slice of span IDs along the critical path from root to leaf.
    fn criticalPathSlice(): []const u64 {
        const n: usize = if (_.critical_path_len <= RING_TRACE_FROZEN_CAPACITY) _.critical_path_len else RING_TRACE_FROZEN_CAPACITY;
        return _.critical_path_span_ids[0..n];
    }
}

/// Telemetry snapshot for `RingTraceRecorder`.
pub struct RingTraceStats {
    buffered_spans: usize = 0;
    capacity: usize = RING_TRACE_CAPACITY;
    total_recorded: u64 = 0;
    total_overwritten: u64 = 0;
    frozen: bool = false;
    frozen_trace_id: u64 = 0;
    frozen_span_count: usize = 0;
    freeze_reason: RingFreezeReason = RingFreezeReason.none;
}

// ============================================================================
// Internal Formatting & Saturation Helpers
// ============================================================================

fn saturatingAddU64(a: u64, b: u64): u64 {
    const max_v: u64 = std.math.maxInt(u64);
    if (a > max_v - b) return max_v;
    return a + b;
}

fn appendRaw(out: *[]u8, cursor: *usize, src: []const u8): bool {
    if (cursor.* + src.len > out.len) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[cursor.* + i] = src[i];
    }
    cursor.* += src.len;
    return true;
}

fn appendU64(out: *[]u8, cursor: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = ('0' as u64 + (v % 10)) as u8;
            v /= 10;
            n += 1;
        }
    }
    if (cursor.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[cursor.* + i] = tmp[n - 1 - i];
    }
    cursor.* += n;
    return true;
}

fn appendJsonEscaped(out: *[]u8, cursor: *usize, src: []const u8): bool {
    if (!appendRaw(out, cursor, "\"")) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const c: u8 = src[i];
        if (c == '"') {
            if (!appendRaw(out, cursor, "\\\"")) return false;
        } else if (c == '\\') {
            if (!appendRaw(out, cursor, "\\\\")) return false;
        } else if (c == '\n') {
            if (!appendRaw(out, cursor, "\\n")) return false;
        } else if (c == '\r') {
            if (!appendRaw(out, cursor, "\\r")) return false;
        } else if (c == '\t') {
            if (!appendRaw(out, cursor, "\\t")) return false;
        } else if (c < 0x20) {
            if (!appendRaw(out, cursor, " ")) return false;
        } else {
            if (cursor.* >= out.len) return false;
            out[cursor.*] = c;
            cursor.* += 1;
        }
    }
    return appendRaw(out, cursor, "\"");
}

// ============================================================================
// 1. Ring Span Flight Recorder
// ============================================================================

/// Zero-heap-allocation distributed tracing flight recorder with O(1) ring overwrite
/// and automatic anomaly/high-latency snapshot freezing.
pub struct RingTraceRecorder {
    ring: [RING_TRACE_CAPACITY]RingSpan = [RingSpan.empty(); RING_TRACE_CAPACITY];
    write_pos: usize = 0;
    buffered_count: usize = 0;
    total_recorded: u64 = 0;
    total_overwritten: u64 = 0;

    latency_threshold_us: u64 = 0;
    auto_freeze_on_error: bool = true;

    frozen: bool = false;
    frozen_trace_id: u64 = 0;
    freeze_reason: RingFreezeReason = RingFreezeReason.none;
    frozen_spans: [RING_TRACE_FROZEN_CAPACITY]RingSpan = [RingSpan.empty(); RING_TRACE_FROZEN_CAPACITY];
    frozen_count: usize = 0;

    /// Initializes a `RingTraceRecorder`.
    /// - `latency_threshold_us > 0` triggers automatic snapshot freeze on slow spans.
    /// - `auto_freeze_on_error` triggers automatic snapshot freeze on `RingSpanStatus.err` spans.
    static fn init(latency_threshold_us: u64, auto_freeze_on_error: bool): RingTraceRecorder {
        return RingTraceRecorder{
            ring: [RingSpan.empty(); RING_TRACE_CAPACITY],
            write_pos: 0,
            buffered_count: 0,
            total_recorded: 0,
            total_overwritten: 0,
            latency_threshold_us: latency_threshold_us,
            auto_freeze_on_error: auto_freeze_on_error,
            frozen: false,
            frozen_trace_id: 0,
            freeze_reason: RingFreezeReason.none,
            frozen_spans: [RingSpan.empty(); RING_TRACE_FROZEN_CAPACITY],
            frozen_count: 0,
        };
    }

    fn captureSnapshot(*_, trace_id: u64, reason: RingFreezeReason): usize {
        let out_s: []RingSpan = _.frozen_spans[0..];
        const copied: usize = _.extractTrace(trace_id, *out_s);
        _.frozen_count = copied;
        _.frozen_trace_id = trace_id;
        _.freeze_reason = reason;
        _.frozen = true;
        return copied;
    }

    /// Records a span into the ring buffer in O(1), overwriting the oldest span when full.
    /// Automatically freezes a snapshot of `trace_id` if an error or latency trigger fires.
    /// Rejects `trace_id == 0`, `span_id == 0`, self-parenting, empty/oversized op name, or oversized tag.
    fn recordSpan(
        *_,
        trace_id: u64,
        span_id: u64,
        parent_span_id: u64,
        op_name: []const u8,
        tag: []const u8,
        start_us: u64,
        duration_us: u64,
        status: RingSpanStatus,
    ): bool {
        if (trace_id == 0 || span_id == 0 || span_id == parent_span_id) {
            return false;
        }
        if (op_name.len == 0 || op_name.len > RING_TRACE_OP_MAX_LEN || tag.len > RING_TRACE_TAG_MAX_LEN) {
            return false;
        }

        let sp = RingSpan.empty();
        sp.trace_id = trace_id;
        sp.span_id = span_id;
        sp.parent_span_id = parent_span_id;
        sp.op_len = op_name.len;
        let i: usize = 0;
        while (i < op_name.len) : (i += 1) {
            sp.op_buf[i] = op_name[i];
        }
        sp.tag_len = tag.len;
        let j: usize = 0;
        while (j < tag.len) : (j += 1) {
            sp.tag_buf[j] = tag[j];
        }
        sp.start_us = start_us;
        sp.duration_us = duration_us;
        sp.status = status;
        sp.occupied = true;

        const slot_idx: usize = _.write_pos & (RING_TRACE_CAPACITY - 1);
        if (_.ring[slot_idx].occupied) {
            _.total_overwritten += 1;
        } else {
            _.buffered_count += 1;
        }
        _.ring[slot_idx] = sp;
        _.write_pos += 1;
        _.total_recorded += 1;

        // Trigger isolated snapshot freeze if not already frozen
        if (!_.frozen) {
            if (_.auto_freeze_on_error && status == RingSpanStatus.err) {
                * = _.captureSnapshot(trace_id, RingFreezeReason.error_span);
            } else if (_.latency_threshold_us > 0 && duration_us >= _.latency_threshold_us) {
                * = _.captureSnapshot(trace_id, RingFreezeReason.high_latency);
            }
        }
        return true;
    }

    /// Extracts all currently buffered spans belonging to `trace_id` in oldest-to-newest order.
    fn extractTrace(trace_id: u64, out_spans: *[]RingSpan): usize {
        if (trace_id == 0 || out_spans.len == 0 || _.buffered_count == 0) {
            return 0;
        }
        const oldest_pos: usize = if (_.write_pos >= _.buffered_count) (_.write_pos - _.buffered_count) else 0;
        let written: usize = 0;
        let k: usize = 0;
        while (k < _.buffered_count && written < out_spans.len) : (k += 1) {
            const idx: usize = (oldest_pos + k) & (RING_TRACE_CAPACITY - 1);
            const sp = _.ring[idx];
            if (sp.occupied && sp.trace_id == trace_id) {
                out_spans[written] = sp;
                written += 1;
            }
        }
        return written;
    }

    /// Manually freezes all currently buffered spans of `trace_id` into the isolated snapshot.
    fn freezeTrace(*_, trace_id: u64): usize {
        if (trace_id == 0) return 0;
        return _.captureSnapshot(trace_id, RingFreezeReason.manual);
    }

    /// Unfreezes the snapshot so future anomalies can capture a new trace.
    fn unfreeze(*_) {
        _.frozen = false;
        _.frozen_trace_id = 0;
        _.freeze_reason = RingFreezeReason.none;
        _.frozen_count = 0;
    }

    /// Returns a read-only slice over the currently frozen trace snapshot.
    fn frozenSpans(): []const RingSpan {
        const n: usize = if (_.frozen_count <= RING_TRACE_FROZEN_CAPACITY) _.frozen_count else RING_TRACE_FROZEN_CAPACITY;
        return _.frozen_spans[0..n];
    }

    /// Clears both the primary ring buffer and any frozen snapshot.
    fn clear(*_) {
        let i: usize = 0;
        while (i < RING_TRACE_CAPACITY) : (i += 1) {
            _.ring[i] = RingSpan.empty();
        }
        _.write_pos = 0;
        _.buffered_count = 0;
        _.unfreeze();
    }

    /// Returns full recorder telemetry.
    fn stats(): RingTraceStats {
        return RingTraceStats{
            buffered_spans: _.buffered_count,
            capacity: RING_TRACE_CAPACITY,
            total_recorded: _.total_recorded,
            total_overwritten: _.total_overwritten,
            frozen: _.frozen,
            frozen_trace_id: _.frozen_trace_id,
            frozen_span_count: _.frozen_count,
            freeze_reason: _.freeze_reason,
        };
    }
}

// ============================================================================
// 2. Strictly Iterative Critical Path & Self-Time Analyzer (Zero Recursion)
// ============================================================================

/// Computes per-span self-time, tree depth, bottleneck span, and critical path over `spans`
/// using strictly iterative bottom-up DP (zero recursion, cycle-safe).
/// Writes per-span details into `out_self_times[0..min(spans.len, out_self_times.len)]`.
/// Returns `null` if `spans` is empty or exceeds `RING_TRACE_FROZEN_CAPACITY`.
pub fn analyzeTraceSpans(
    spans: []const RingSpan,
    out_self_times: *[]SpanSelfTime,
): TraceAnalysis? {
    const n: usize = spans.len;
    if (n == 0 || n > RING_TRACE_FROZEN_CAPACITY) {
        return null;
    }

    const NO_IDX: usize = RING_TRACE_FROZEN_CAPACITY + 1;
    let parent_idx: [RING_TRACE_FROZEN_CAPACITY]usize = [NO_IDX; RING_TRACE_FROZEN_CAPACITY];
    let child_dur_sum: [RING_TRACE_FROZEN_CAPACITY]u64 = [0; RING_TRACE_FROZEN_CAPACITY];
    let self_us: [RING_TRACE_FROZEN_CAPACITY]u64 = [0; RING_TRACE_FROZEN_CAPACITY];
    let depths: [RING_TRACE_FROZEN_CAPACITY]u16 = [0; RING_TRACE_FROZEN_CAPACITY];
    let dp_path_us: [RING_TRACE_FROZEN_CAPACITY]u64 = [0; RING_TRACE_FROZEN_CAPACITY];
    let best_child: [RING_TRACE_FROZEN_CAPACITY]usize = [NO_IDX; RING_TRACE_FROZEN_CAPACITY];
    let on_crit: [RING_TRACE_FROZEN_CAPACITY]bool = [false; RING_TRACE_FROZEN_CAPACITY];

    let min_start: u64 = spans[0].start_us;
    let max_end: u64 = spans[0].endUs();
    let error_count: usize = 0;

    // Pass 1: resolve parent indices, wall bounds, error count, and direct children duration sums
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const sp = spans[i];
        if (sp.start_us < min_start) min_start = sp.start_us;
        const e_us: u64 = sp.endUs();
        if (e_us > max_end) max_end = e_us;
        if (sp.status == RingSpanStatus.err) error_count += 1;

        if (sp.parent_span_id != 0) {
            let p: usize = 0;
            while (p < n) : (p += 1) {
                if (p != i && spans[p].span_id == sp.parent_span_id) {
                    parent_idx[i] = p;
                    child_dur_sum[p] = saturatingAddU64(child_dur_sum[p], sp.duration_us);
                    break;
                }
            }
        }
    }

    // Pass 2: compute self_us, bottleneck span, and cycle-safe iterative depth
    let max_depth: u16 = 0;
    let bottleneck_id: u64 = spans[0].span_id;
    let bottleneck_self: u64 = 0;
    let has_bottleneck: bool = false;

    let j: usize = 0;
    while (j < n) : (j += 1) {
        const dur: u64 = spans[j].duration_us;
        const csum: u64 = child_dur_sum[j];
        const su: u64 = if (dur > csum) (dur - csum) else 0;
        self_us[j] = su;

        if (!has_bottleneck || su > bottleneck_self || (su == bottleneck_self && spans[j].span_id < bottleneck_id)) {
            bottleneck_self = su;
            bottleneck_id = spans[j].span_id;
            has_bottleneck = true;
        }

        // Walk up parent_idx with bounded step count `n` to guard against malformed cycles
        let d: u16 = 0;
        let cur: usize = parent_idx[j];
        let steps: usize = 0;
        while (cur != NO_IDX && steps < n) : (steps += 1) {
            d += 1;
            cur = parent_idx[cur];
        }
        depths[j] = d;
        if (d > max_depth) max_depth = d;
    }

    // Pass 3: bottom-up iterative DP from max_depth down to 0
    // dp_path_us[u] = self_us[u] + max_{child c of u} dp_path_us[c] (or duration_us if leaf)
    let cur_depth: i32 = max_depth as i32;
    while (cur_depth >= 0) : (cur_depth -= 1) {
        const target_d: u16 = cur_depth as u16;
        let u: usize = 0;
        while (u < n) : (u += 1) {
            if (depths[u] != target_d) continue;

            let max_c_dp: u64 = 0;
            let best_c: usize = NO_IDX;
            let c: usize = 0;
            while (c < n) : (c += 1) {
                if (parent_idx[c] == u) {
                    if (best_c == NO_IDX || dp_path_us[c] > max_c_dp) {
                        max_c_dp = dp_path_us[c];
                        best_c = c;
                    }
                }
            }
            best_child[u] = best_c;
            if (best_c == NO_IDX) {
                dp_path_us[u] = spans[u].duration_us;
            } else {
                dp_path_us[u] = saturatingAddU64(self_us[u], max_c_dp);
            }
        }
    }

    // Pass 4: pick root (depth == 0) with maximum dp_path_us and trace critical path downward
    let best_root: usize = 0;
    let best_root_us: u64 = 0;
    let found_root: bool = false;
    let r: usize = 0;
    while (r < n) : (r += 1) {
        if (depths[r] == 0) {
            if (!found_root || dp_path_us[r] > best_root_us) {
                best_root = r;
                best_root_us = dp_path_us[r];
                found_root = true;
            }
        }
    }

    let analysis = TraceAnalysis{
        trace_id: spans[0].trace_id,
        span_count: n,
        error_span_count: error_count,
        total_wall_us: if (max_end >= min_start) (max_end - min_start) else 0,
        critical_path_us: best_root_us,
        critical_path_len: 0,
        critical_path_span_ids: [0; RING_TRACE_FROZEN_CAPACITY],
        max_depth: max_depth,
        bottleneck_span_id: bottleneck_id,
        bottleneck_self_us: bottleneck_self,
    };

    if (found_root) {
        let cur: usize = best_root;
        let hops: usize = 0;
        while (cur != NO_IDX && hops < n && analysis.critical_path_len < RING_TRACE_FROZEN_CAPACITY) : (hops += 1) {
            on_crit[cur] = true;
            analysis.critical_path_span_ids[analysis.critical_path_len] = spans[cur].span_id;
            analysis.critical_path_len += 1;
            cur = best_child[cur];
        }
    }

    // Write per-span self-time results into caller buffer
    const out_n: usize = if (n < out_self_times.len) n else out_self_times.len;
    let w: usize = 0;
    while (w < out_n) : (w += 1) {
        out_self_times[w] = SpanSelfTime{
            span_id: spans[w].span_id,
            total_us: spans[w].duration_us,
            self_us: self_us[w],
            depth: depths[w],
            is_on_critical_path: on_crit[w],
        };
    }

    return analysis;
}

// ============================================================================
// 3. Zero-Alloc Summary & Chrome Trace Event JSON Exporter
// ============================================================================

/// Formats a compact single-line diagnostic summary of `analysis` into `out_buf`.
/// Returns bytes written or `null` if `out_buf` is too small.
pub fn formatTraceSummary(analysis: TraceAnalysis, out_buf: *[]u8): usize? {
    let cur: usize = 0;
    if (!appendRaw(out_buf, *cur, "trace=")) return null;
    if (!appendU64(out_buf, *cur, analysis.trace_id)) return null;
    if (!appendRaw(out_buf, *cur, " spans=")) return null;
    if (!appendU64(out_buf, *cur, analysis.span_count as u64)) return null;
    if (!appendRaw(out_buf, *cur, " errors=")) return null;
    if (!appendU64(out_buf, *cur, analysis.error_span_count as u64)) return null;
    if (!appendRaw(out_buf, *cur, " wall_us=")) return null;
    if (!appendU64(out_buf, *cur, analysis.total_wall_us)) return null;
    if (!appendRaw(out_buf, *cur, " crit_us=")) return null;
    if (!appendU64(out_buf, *cur, analysis.critical_path_us)) return null;
    if (!appendRaw(out_buf, *cur, " max_depth=")) return null;
    if (!appendU64(out_buf, *cur, analysis.max_depth as u64)) return null;
    if (!appendRaw(out_buf, *cur, " bottleneck=")) return null;
    if (!appendU64(out_buf, *cur, analysis.bottleneck_span_id)) return null;
    if (!appendRaw(out_buf, *cur, "(")) return null;
    if (!appendU64(out_buf, *cur, analysis.bottleneck_self_us)) return null;
    if (!appendRaw(out_buf, *cur, "us)")) return null;
    return cur;
}

/// Formats a single `RingSpan` as a Chrome Trace Event Format (`ph: "X"`) JSON line into `out_buf`.
/// Returns bytes written or `null` if `span` is unoccupied or `out_buf` is too small.
pub fn formatChromeTraceEvent(span: RingSpan, pid: u32, tid: u32, out_buf: *[]u8): usize? {
    if (!span.occupied) return null;
    let cur: usize = 0;
    if (!appendRaw(out_buf, *cur, "{\"name\":")) return null;
    if (!appendJsonEscaped(out_buf, *cur, span.opSlice())) return null;
    if (!appendRaw(out_buf, *cur, ",\"cat\":\"trace\",\"ph\":\"X\",\"ts\":")) return null;
    if (!appendU64(out_buf, *cur, span.start_us)) return null;
    if (!appendRaw(out_buf, *cur, ",\"dur\":")) return null;
    if (!appendU64(out_buf, *cur, span.duration_us)) return null;
    if (!appendRaw(out_buf, *cur, ",\"pid\":")) return null;
    if (!appendU64(out_buf, *cur, pid as u64)) return null;
    if (!appendRaw(out_buf, *cur, ",\"tid\":")) return null;
    if (!appendU64(out_buf, *cur, tid as u64)) return null;
    if (!appendRaw(out_buf, *cur, ",\"args\":{\"trace_id\":")) return null;
    if (!appendU64(out_buf, *cur, span.trace_id)) return null;
    if (!appendRaw(out_buf, *cur, ",\"span_id\":")) return null;
    if (!appendU64(out_buf, *cur, span.span_id)) return null;
    if (!appendRaw(out_buf, *cur, ",\"parent_span_id\":")) return null;
    if (!appendU64(out_buf, *cur, span.parent_span_id)) return null;
    if (!appendRaw(out_buf, *cur, ",\"status\":")) return null;
    if (span.status == RingSpanStatus.ok) {
        if (!appendRaw(out_buf, *cur, "\"ok\"")) return null;
    } else {
        if (!appendRaw(out_buf, *cur, "\"error\"")) return null;
    }
    if (span.tag_len > 0) {
        if (!appendRaw(out_buf, *cur, ",\"tag\":")) return null;
        if (!appendJsonEscaped(out_buf, *cur, span.tagSlice())) return null;
    }
    if (!appendRaw(out_buf, *cur, "}}")) return null;
    return cur;
}
