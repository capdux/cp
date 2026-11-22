// Origin: Pure-CP Stdlib (Zero-Alloc OTLP Span Batch Exporter & Logarithmic-Bucket Latency Histogram) | Created: 2026-11-22
// Pure CP zero-heap-allocation observability telemetry primitives:
//   1. `LatencyBucketHistogram`: 16-bucket logarithmic microsecond latency histogram with O(1) observe,
//      p50/p95/p99 linear-interpolated quantile estimation & zero-heap Prometheus/OpenMetrics text rendering
//   2. `OtlpSpanRecord` / `OtlpSpanBatch`: fixed-slot distributed tracing span collector & RFC 8259 escaped
//      OTLP/HTTP JSON batch exporter

const std = @import("std");

/// Number of finite upper-bound buckets in `LatencyBucketHistogram` (plus 1 `+Inf` overflow bucket).
pub const HISTOGRAM_BUCKET_COUNT: usize = 16;

// ============================================================================
// Internal Buffer, Numeric & RFC 8259 JSON Escaping Helpers
// ============================================================================

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn writeByte(out_buf: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeRaw(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn hexDigitLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn writeU64Dec(out_buf: *[]u8, pos: *usize, value: u64): bool {
    let digits: [20]u8 = [0; 20];
    let len: usize = 0;
    let v: u64 = value;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) : (len += 1) {
            digits[len] = ('0' as u8) + ((v % 10) as u8);
            v = v / 10;
        }
    }
    while (len > 0) {
        len -= 1;
        if (!writeByte(out_buf, pos, digits[len])) return false;
    }
    return true;
}

fn writeJsonString(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (!writeByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b: u8 = s[i];
        if (b == '"') {
            if (!writeRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!writeRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!writeRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!writeRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!writeRaw(out_buf, pos, "\\t")) return false;
        } else if (b == 0x08) {
            if (!writeRaw(out_buf, pos, "\\b")) return false;
        } else if (b == 0x0C) {
            if (!writeRaw(out_buf, pos, "\\f")) return false;
        } else if (b < 0x20) {
            if (!writeRaw(out_buf, pos, "\\u00")) return false;
            if (!writeByte(out_buf, pos, hexDigitLower((b >> 4) & 0x0F))) return false;
            if (!writeByte(out_buf, pos, hexDigitLower(b & 0x0F))) return false;
        } else {
            if (!writeByte(out_buf, pos, b)) return false;
        }
    }
    return writeByte(out_buf, pos, '"');
}

// ============================================================================
// 1. Fixed Logarithmic-Bucket Latency Histogram (`LatencyBucketHistogram`)
// ============================================================================

/// Zero-heap-allocation 16-bucket logarithmic microsecond latency histogram.
/// Tracks exact `count`, saturating `sum`, `min`, `max`, supports Prometheus-style linear-interpolated
/// `quantile(q)` estimation (p50/p95/p99), and renders OpenMetrics/Prometheus exposition text.
pub struct LatencyBucketHistogram {
    bounds_us: [16]u64 = [
        50, 100, 250, 500,
        1000, 2500, 5000, 10000,
        25000, 50000, 100000, 250000,
        500000, 1000000, 2500000, 10000000,
    ];
    bucket_counts: [17]u64 = [0; 17];
    total_count: u64 = 0;
    total_sum_us: u64 = 0;
    min_us: u64 = 0;
    max_us: u64 = 0;

    /// Initializes a latency histogram with default 16 logarithmic microsecond bounds (50us .. 10s).
    static fn init(): LatencyBucketHistogram {
        return LatencyBucketHistogram{};
    }

    /// Initializes a latency histogram with custom strictly-increasing positive upper bounds.
    /// Returns `null` if `bounds_us[0] == 0` or any bound is not strictly greater than the previous.
    static fn initWithBounds(bounds_us: [16]u64): LatencyBucketHistogram? {
        if (bounds_us[0] == 0) return null;
        let i: usize = 1;
        while (i < HISTOGRAM_BUCKET_COUNT) : (i += 1) {
            if (bounds_us[i] <= bounds_us[i - 1]) return null;
        }
        return LatencyBucketHistogram{
            bounds_us: bounds_us,
            bucket_counts: [0; 17],
            total_count: 0,
            total_sum_us: 0,
            min_us: 0,
            max_us: 0,
        };
    }

    /// Records a single latency observation in microseconds (`O(1)` fixed 4-step binary search).
    fn observe(*_, value_us: u64) {
        let lo: usize = 0;
        let hi: usize = HISTOGRAM_BUCKET_COUNT;
        while (lo < hi) {
            const mid: usize = lo + ((hi - lo) / 2);
            if (value_us <= _.bounds_us[mid]) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        _.bucket_counts[lo] += 1;

        if (_.total_count == 0) {
            _.min_us = value_us;
            _.max_us = value_us;
        } else {
            if (value_us < _.min_us) _.min_us = value_us;
            if (value_us > _.max_us) _.max_us = value_us;
        }
        _.total_count += 1;

        const max_u64: u64 = std.math.maxInt(u64);
        if (_.total_sum_us > max_u64 - value_us) {
            _.total_sum_us = max_u64;
        } else {
            _.total_sum_us += value_us;
        }
    }

    /// Resets all counters while keeping configured bucket boundaries.
    fn reset(*_) {
        let i: usize = 0;
        while (i < 17) : (i += 1) {
            _.bucket_counts[i] = 0;
        }
        _.total_count = 0;
        _.total_sum_us = 0;
        _.min_us = 0;
        _.max_us = 0;
    }

    /// Returns total number of observed samples.
    fn count(): u64 {
        return _.total_count;
    }

    /// Returns saturating sum of observed microsecond latencies.
    fn sumUs(): u64 {
        return _.total_sum_us;
    }

    /// Returns minimum observed latency, or `null` if no samples recorded.
    fn minUs(): u64? {
        if (_.total_count == 0) return null;
        return _.min_us;
    }

    /// Returns maximum observed latency, or `null` if no samples recorded.
    fn maxUs(): u64? {
        if (_.total_count == 0) return null;
        return _.max_us;
    }

    /// Returns arithmetic mean latency in microseconds, or `null` if empty.
    fn meanUs(): f64? {
        if (_.total_count == 0) return null;
        return (_.total_sum_us as f64) / (_.total_count as f64);
    }

    /// Estimates quantile `q` in `[0.0, 1.0]` (e.g. `0.50`, `0.95`, `0.99`) using Prometheus-standard
    /// linear interpolation within the target bucket. Returns `null` if empty or `q` is invalid.
    fn quantile(q: f64): f64? {
        if (!isFiniteF64(q) || q < 0.0 || q > 1.0) return null;
        if (_.total_count == 0) return null;
        if (q == 0.0) return _.min_us as f64;
        if (q == 1.0) return _.max_us as f64;

        const target_rank: f64 = q * (_.total_count as f64);
        let cum: u64 = 0;
        let prev_bound: f64 = 0.0;

        let i: usize = 0;
        while (i < HISTOGRAM_BUCKET_COUNT) : (i += 1) {
            const b_count: u64 = _.bucket_counts[i];
            const next_cum: u64 = cum + b_count;
            const upper_bound: f64 = _.bounds_us[i] as f64;
            if ((next_cum as f64) >= target_rank && b_count > 0) {
                const frac: f64 = (target_rank - (cum as f64)) / (b_count as f64);
                return prev_bound + (upper_bound - prev_bound) * frac;
            }
            cum = next_cum;
            prev_bound = upper_bound;
        }

        // Sample falls into +Inf overflow bucket: interpolate between last finite bound and observed max_us.
        const ovf_count: u64 = _.bucket_counts[HISTOGRAM_BUCKET_COUNT];
        if (ovf_count > 0) {
            const hi_bound: f64 = _.max_us as f64;
            if (hi_bound > prev_bound) {
                const frac: f64 = (target_rank - (cum as f64)) / (ovf_count as f64);
                return prev_bound + (hi_bound - prev_bound) * frac;
            }
        }
        return _.max_us as f64;
    }

    /// Renders Prometheus / OpenMetrics histogram exposition text (`_bucket`, `_sum`, `_count`)
    /// into caller-provided `out_buf` (zero heap allocation).
    /// Returns bytes written, or `null` if `metric_name` is empty or `out_buf` is too small.
    fn renderPrometheus(metric_name: string, out_buf: *[]u8): usize? {
        if (metric_name.len == 0) return null;

        let pos: usize = 0;
        if (!writeRaw(out_buf, &pos, "# TYPE ")) return null;
        if (!writeRaw(out_buf, &pos, metric_name)) return null;
        if (!writeRaw(out_buf, &pos, " histogram\n")) return null;

        let cum: u64 = 0;
        let i: usize = 0;
        while (i < HISTOGRAM_BUCKET_COUNT) : (i += 1) {
            cum += _.bucket_counts[i];
            if (!writeRaw(out_buf, &pos, metric_name)) return null;
            if (!writeRaw(out_buf, &pos, "_bucket{le=\"")) return null;
            if (!writeU64Dec(out_buf, &pos, _.bounds_us[i])) return null;
            if (!writeRaw(out_buf, &pos, "\"} ")) return null;
            if (!writeU64Dec(out_buf, &pos, cum)) return null;
            if (!writeByte(out_buf, &pos, '\n')) return null;
        }

        // +Inf bucket
        cum += _.bucket_counts[HISTOGRAM_BUCKET_COUNT];
        if (!writeRaw(out_buf, &pos, metric_name)) return null;
        if (!writeRaw(out_buf, &pos, "_bucket{le=\"+Inf\"} ")) return null;
        if (!writeU64Dec(out_buf, &pos, cum)) return null;
        if (!writeByte(out_buf, &pos, '\n')) return null;

        // _sum and _count
        if (!writeRaw(out_buf, &pos, metric_name)) return null;
        if (!writeRaw(out_buf, &pos, "_sum ")) return null;
        if (!writeU64Dec(out_buf, &pos, _.total_sum_us)) return null;
        if (!writeByte(out_buf, &pos, '\n')) return null;

        if (!writeRaw(out_buf, &pos, metric_name)) return null;
        if (!writeRaw(out_buf, &pos, "_count ")) return null;
        if (!writeU64Dec(out_buf, &pos, _.total_count)) return null;
        if (!writeByte(out_buf, &pos, '\n')) return null;

        return pos;
    }
}

// ============================================================================
// 2. Zero-Alloc OTLP/HTTP JSON Span Batch Exporter (`OtlpSpanBatch`)
// ============================================================================

/// Compact distributed tracing span record for OTLP batch export.
pub struct OtlpSpanRecord {
    trace_id_hex: string = "";
    span_id_hex: string = "";
    parent_span_id_hex: string = "";
    name: string = "";
    start_us: u64 = 0;
    end_us: u64 = 0;
    status_code: u16 = 0;
    is_error: bool = false;
}

/// Fixed-slot zero-heap OTLP span batch collector & RFC 8259 escaped JSON exporter.
pub struct OtlpSpanBatch {
    slots: []OtlpSpanRecord;
    count: usize = 0;
    dropped_count: u64 = 0;

    /// Initializes an empty span batch over caller-provided `slots`.
    static fn init(slots: *[]OtlpSpanRecord): OtlpSpanBatch {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = OtlpSpanRecord{};
        }
        return OtlpSpanBatch{
            slots: slots,
            count: 0,
            dropped_count: 0,
        };
    }

    /// Appends a validated span to the batch.
    /// Rejects spans with empty `trace_id_hex`/`span_id_hex`/`name`, `end_us < start_us`,
    /// or when the batch is full (incrementing `dropped_count` on capacity overflow).
    fn push(*_, span: OtlpSpanRecord): bool {
        if (span.trace_id_hex.len == 0 || span.span_id_hex.len == 0 || span.name.len == 0) {
            return false;
        }
        if (span.end_us < span.start_us) return false;
        if (_.count >= _.slots.len) {
            _.dropped_count += 1;
            return false;
        }
        _.slots[_.count] = span;
        _.count += 1;
        return true;
    }

    /// Returns the number of buffered spans currently in the batch.
    fn len(): usize {
        return _.count;
    }

    /// Returns the fixed slot capacity of this batch.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Returns cumulative spans dropped due to full batch capacity.
    fn dropped(): u64 {
        return _.dropped_count;
    }

    /// Clears all buffered spans for reuse (preserves cumulative `dropped_count`).
    fn clear(*_) {
        _.count = 0;
    }

    /// Renders an OTLP/HTTP JSON `ExportTraceServiceRequest` payload into caller-provided `out_buf`.
    /// Returns bytes written, or `null` if `service_name` is empty or `out_buf` is too small.
    fn renderOtlpJson(service_name: string, out_buf: *[]u8): usize? {
        if (service_name.len == 0) return null;

        let pos: usize = 0;
        if (!writeRaw(out_buf, &pos, "{\"resourceSpans\":[{\"resource\":{\"attributes\":[{\"key\":\"service.name\",\"value\":{\"stringValue\":")) {
            return null;
        }
        if (!writeJsonString(out_buf, &pos, service_name)) return null;
        if (!writeRaw(out_buf, &pos, "}}]},\"scopeSpans\":[{\"spans\":[")) return null;

        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const sp = _.slots[i];
            if (i > 0) {
                if (!writeByte(out_buf, &pos, ',')) return null;
            }
            if (!writeRaw(out_buf, &pos, "{\"traceId\":")) return null;
            if (!writeJsonString(out_buf, &pos, sp.trace_id_hex)) return null;
            if (!writeRaw(out_buf, &pos, ",\"spanId\":")) return null;
            if (!writeJsonString(out_buf, &pos, sp.span_id_hex)) return null;

            if (sp.parent_span_id_hex.len > 0) {
                if (!writeRaw(out_buf, &pos, ",\"parentSpanId\":")) return null;
                if (!writeJsonString(out_buf, &pos, sp.parent_span_id_hex)) return null;
            }

            if (!writeRaw(out_buf, &pos, ",\"name\":")) return null;
            if (!writeJsonString(out_buf, &pos, sp.name)) return null;

            // OTLP JSON encodes 64-bit nanosecond timestamps as decimal strings.
            const max_us_for_ns: u64 = std.math.maxInt(u64) / 1000;
            const start_ns: u64 = if (sp.start_us > max_us_for_ns) std.math.maxInt(u64) else (sp.start_us * 1000);
            const end_ns: u64 = if (sp.end_us > max_us_for_ns) std.math.maxInt(u64) else (sp.end_us * 1000);

            if (!writeRaw(out_buf, &pos, ",\"startTimeUnixNano\":\"")) return null;
            if (!writeU64Dec(out_buf, &pos, start_ns)) return null;
            if (!writeRaw(out_buf, &pos, "\",\"endTimeUnixNano\":\"")) return null;
            if (!writeU64Dec(out_buf, &pos, end_ns)) return null;
            if (!writeByte(out_buf, &pos, '"')) return null;

            if (sp.status_code > 0) {
                if (!writeRaw(out_buf, &pos, ",\"attributes\":[{\"key\":\"http.status_code\",\"value\":{\"intValue\":\"")) return null;
                if (!writeU64Dec(out_buf, &pos, sp.status_code as u64)) return null;
                if (!writeRaw(out_buf, &pos, "\"}}]")) return null;
            }

            if (sp.is_error) {
                if (!writeRaw(out_buf, &pos, ",\"status\":{\"code\":2}}")) return null;
            } else {
                if (!writeRaw(out_buf, &pos, ",\"status\":{\"code\":1}}")) return null;
            }
        }

        if (!writeRaw(out_buf, &pos, "]}]}]}")) return null;
        return pos;
    }
}
