// Source: native-cp-stdlib (OpenMetrics / Prometheus Text Format)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { parse, MetricType, MetricsDocument } from "cap:obs";

test "parse multi-family Counter Gauge Histogram Summary with HELP TYPE UNIT" {
    const alloc = std.testing.allocator;
    const payload =
        "# HELP http_requests_total Total HTTP requests\n" ++
        "# TYPE http_requests_total counter\n" ++
        "http_requests_total{method=\"GET\",code=\"200\"} 42\n" ++
        "# HELP memory_usage_bytes Resident memory\n" ++
        "# UNIT memory_usage_bytes bytes\n" ++
        "# TYPE memory_usage_bytes gauge\n" ++
        "memory_usage_bytes 1048576\n" ++
        "# HELP req_duration_seconds Request latency\n" ++
        "# TYPE req_duration_seconds histogram\n" ++
        "req_duration_seconds_bucket{le=\"0.1\"} 10\n" ++
        "req_duration_seconds_bucket{le=\"+Inf\"} 12\n" ++
        "req_duration_seconds_sum 1.5\n" ++
        "req_duration_seconds_count 12\n" ++
        "# HELP rpc_latency_seconds RPC summary\n" ++
        "# TYPE rpc_latency_seconds summary\n" ++
        "rpc_latency_seconds{quantile=\"0.5\"} 0.02\n" ++
        "rpc_latency_seconds_sum 2.4\n" ++
        "rpc_latency_seconds_count 100\n";

    let doc = try parse(alloc, payload);
    defer doc.deinit(alloc);

    expect_eq(doc.families.len, @as(usize, 4));
    expect_eq(doc.has_eof, false);

    const fam_counter = doc.findFamily("http_requests_total") orelse unreachable;
    expect_eq(fam_counter.metric_type, MetricType.Counter);
    expect_eq(fam_counter.samples.len, @as(usize, 1));
    expect_eq(fam_counter.samples[0].value, 42.0);

    const fam_gauge = doc.findFamily("memory_usage_bytes") orelse unreachable;
    expect_eq(fam_gauge.metric_type, MetricType.Gauge);
    expect_eq(std.mem.eql(u8, fam_gauge.unit orelse "", "bytes"), true);

    const fam_hist = doc.findFamily("req_duration_seconds") orelse unreachable;
    expect_eq(fam_hist.metric_type, MetricType.Histogram);
    expect_eq(fam_hist.samples.len, @as(usize, 4));

    const fam_sum = doc.findFamily("rpc_latency_seconds") orelse unreachable;
    expect_eq(fam_sum.metric_type, MetricType.Summary);
    expect_eq(fam_sum.samples.len, @as(usize, 3));
}

test "parse escaped labels optional timestamps and EOF marker" {
    const alloc = std.testing.allocator;
    const payload =
        "# HELP log_events_total Log events with \\\\n escapes\\nsecond line\n" ++
        "# TYPE log_events_total counter\n" ++
        "log_events_total{msg=\"hello \\\"world\\\"\\nline2\\\\end\",env=\"prod\"} 7 1700000000123\n" ++
        "log_events_total{msg=\"sec_ts\"} 9 1700000000.5\n" ++
        "# EOF\n";

    let doc = try parse(alloc, payload);
    defer doc.deinit(alloc);

    expect_eq(doc.has_eof, true);
    const fam = doc.findFamily("log_events_total") orelse unreachable;
    expect_eq(fam.samples.len, @as(usize, 2));
    expect_eq(std.mem.eql(u8, fam.samples[0].labels[0].value, "hello \"world\"\nline2\\end"), true);
    expect_eq(fam.samples[0].timestamp_ms orelse 0, @as(i64, 1700000000123));
    expect_eq(fam.samples[1].timestamp_ms orelse 0, @as(i64, 1700000000500));
}

test "parse special float values NaN +Inf -Inf and integers" {
    const alloc = std.testing.allocator;
    const payload =
        "# TYPE temp_celsius gauge\n" ++
        "temp_celsius{sensor=\"a\"} NaN\n" ++
        "temp_celsius{sensor=\"b\"} +Inf\n" ++
        "temp_celsius{sensor=\"c\"} -Inf\n" ++
        "temp_celsius{sensor=\"d\"} -15\n";

    let doc = try parse(alloc, payload);
    defer doc.deinit(alloc);

    const fam = doc.findFamily("temp_celsius") orelse unreachable;
    expect_eq(fam.samples.len, @as(usize, 4));
    expect_eq(std.math.isNan(fam.samples[0].value), true);
    expect_eq(std.math.isPositiveInf(fam.samples[1].value), true);
    expect_eq(std.math.isNegativeInf(fam.samples[2].value), true);
    expect_eq(fam.samples[3].value, -15.0);
}

test "serialize and re-parse roundtrip consistency" {
    const alloc = std.testing.allocator;
    const payload =
        "# HELP api_latency_seconds API latency\n" ++
        "# UNIT api_latency_seconds seconds\n" ++
        "# TYPE api_latency_seconds gauge\n" ++
        "api_latency_seconds{route=\"/v1/\\\"items\\\"\",ok=\"true\"} 0.125 1700000001000\n" ++
        "api_latency_seconds{route=\"/health\"} +Inf\n" ++
        "# EOF\n";

    let doc1 = try parse(alloc, payload);
    defer doc1.deinit(alloc);

    const serialized = try doc1.serialize(alloc);
    defer alloc.free(serialized);

    let doc2 = try parse(alloc, serialized);
    defer doc2.deinit(alloc);

    expect_eq(doc2.has_eof, true);
    expect_eq(doc2.families.len, doc1.families.len);
    const f1 = doc1.findFamily("api_latency_seconds") orelse unreachable;
    const f2 = doc2.findFamily("api_latency_seconds") orelse unreachable;
    expect_eq(f2.samples.len, f1.samples.len);
    expect_eq(f2.samples[0].value, f1.samples[0].value);
    expect_eq(f2.samples[0].timestamp_ms orelse 0, f1.samples[0].timestamp_ms orelse 0);
    expect_eq(std.mem.eql(u8, f2.samples[0].labels[0].value, f1.samples[0].labels[0].value), true);
    expect_eq(std.math.isPositiveInf(f2.samples[1].value), true);
}

test "cap.openmetrics dynamic mount and zero-leak allocator verification" {
    const alloc = std.testing.allocator;
    const raw =
        "# TYPE jobs_queued untyped\n" ++
        "jobs_queued{queue=\"default\"} 15\n" ++
        "# EOF\n";

    let doc = try cap.obs.parse(alloc, raw);
    defer doc.deinit(alloc);

    expect_eq(doc.has_eof, true);
    const fam = doc.findFamily("jobs_queued") orelse unreachable;
    expect_eq(fam.metric_type, MetricType.Untyped);
    expect_eq(fam.samples[0].value, 15.0);
}
