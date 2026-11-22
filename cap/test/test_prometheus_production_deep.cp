// Source: cp/cap/test/test_prometheus_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:prometheus & cap.prometheus

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PromError,
    MetricType,
    LabelPair,
    ParsedSample,
    PromRegistry,
    parseExposition,
    freeParsedSamples,
} from "cap:time";

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    let reg = PromRegistry.init(alloc);
    defer reg.deinit();

    reg.registerCounter("http_requests_total", "Total HTTP requests\nwith \"quotes\" and \\backslash")?;
    reg.registerGauge("active_connections", "Current active connections")?;

    const l1: [2]LabelPair = [
        LabelPair{ name: "method", value: "GET" },
        LabelPair{ name: "path", value: "/v1/\"items\"\\list\nline2" },
    ];
    const l2: [2]LabelPair = [
        LabelPair{ name: "method", value: "POST" },
        LabelPair{ name: "path", value: "/v1/submit" },
    ];

    reg.incCounter("http_requests_total", &l1, 3.0)?;
    reg.incCounter("http_requests_total", &l1, 2.0)?;
    reg.incCounter("http_requests_total", &l2, 7.0)?;

    const g_lbl: [1]LabelPair = [
        LabelPair{ name: "pool", value: "primary" },
    ];
    reg.setGauge("active_connections", &g_lbl, 10.0)?;
    reg.addGauge("active_connections", &g_lbl, 5.0)?;
    reg.addGauge("active_connections", &g_lbl, -3.5)?;

    const out = reg.exportText(alloc)?;
    defer alloc.free(out);

    expect_eq(std.mem.indexOf(u8, out, "# HELP http_requests_total Total HTTP requests\\nwith \"quotes\" and \\\\backslash\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "# TYPE http_requests_total counter\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "http_requests_total{method=\"GET\",path=\"/v1/\\\"items\\\"\\\\list\\nline2\"} 5\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "http_requests_total{method=\"POST\",path=\"/v1/submit\"} 7\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "# TYPE active_connections gauge\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "active_connections{pool=\"primary\"} 11.5\n") != null, true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    let reg = PromRegistry.init(alloc);
    defer reg.deinit();

    const buckets: [3]f64 = [0.1, 0.5, 1.0];
    reg.registerHistogram("req_latency_seconds", "Request latency in seconds", &buckets)?;

    const lbl: [1]LabelPair = [
        LabelPair{ name: "route", value: "/search" },
    ];
    // Observations: 0.05 (<=0.1,0.5,1.0,+Inf), 0.3 (<=0.5,1.0,+Inf), 0.8 (<=1.0,+Inf), 2.5 (+Inf only)
    reg.observeHistogram("req_latency_seconds", &lbl, 0.05)?;
    reg.observeHistogram("req_latency_seconds", &lbl, 0.3)?;
    reg.observeHistogram("req_latency_seconds", &lbl, 0.8)?;
    reg.observeHistogram("req_latency_seconds", &lbl, 2.5)?;

    const out = reg.exportText(alloc)?;
    defer alloc.free(out);

    expect_eq(std.mem.indexOf(u8, out, "# HELP req_latency_seconds Request latency in seconds\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "# TYPE req_latency_seconds histogram\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "req_latency_seconds_bucket{route=\"/search\",le=\"0.1\"} 1\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "req_latency_seconds_bucket{route=\"/search\",le=\"0.5\"} 2\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "req_latency_seconds_bucket{route=\"/search\",le=\"1\"} 3\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "req_latency_seconds_bucket{route=\"/search\",le=\"+Inf\"} 4\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "req_latency_seconds_sum{route=\"/search\"} 3.65\n") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "req_latency_seconds_count{route=\"/search\"} 4\n") != null, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;
    let reg = PromRegistry.init(alloc);
    defer reg.deinit();

    reg.registerCounter("events_total", "Events processed\\nper node")?;
    const c_lbl: [1]LabelPair = [
        LabelPair{ name: "msg", value: "a\\b\"c\nd" },
    ];
    reg.incCounter("events_total", &c_lbl, 42.0)?;

    reg.registerGauge("queue_depth", "Pending items")?;
    const empty_lbl: [0]LabelPair = [];
    reg.setGauge("queue_depth", &empty_lbl, -4.25)?;

    const b: [2]f64 = [1.0, 5.0];
    reg.registerHistogram("job_size", "Job size", &b)?;
    reg.observeHistogram("job_size", &empty_lbl, 2.0)?;

    const exported = reg.exportText(alloc)?;
    defer alloc.free(exported);

    // Prepend an ordinary comment line to ensure parser skips arbitrary comments
    const with_comment = std.fmt.allocPrint(alloc, "# Arbitrary comment line\n\n{s}", .{exported}) catch unreachable;
    defer alloc.free(with_comment);

    const samples = parseExposition(alloc, with_comment)?;
    defer freeParsedSamples(alloc, samples);

    // 1 counter + 1 gauge + 5 histogram lines (2 buckets + +Inf + _sum + _count) = 7 samples
    expect_eq(samples.len, 7 as usize);

    expect_eq(std.mem.eql(u8, samples[0].name, "events_total"), true);
    expect_eq(samples[0].metric_type.? == MetricType.Counter, true);
    expect_eq(std.mem.eql(u8, samples[0].help.?, "Events processed\nper node"), true);
    expect_eq(samples[0].labels.len, 1 as usize);
    expect_eq(std.mem.eql(u8, samples[0].labels[0].name, "msg"), true);
    expect_eq(std.mem.eql(u8, samples[0].labels[0].value, "a\\b\"c\nd"), true);
    expect_eq(samples[0].value, 42.0);

    expect_eq(std.mem.eql(u8, samples[1].name, "queue_depth"), true);
    expect_eq(samples[1].metric_type.? == MetricType.Gauge, true);
    expect_eq(samples[1].labels.len, 0 as usize);
    expect_eq(samples[1].value, -4.25);

    expect_eq(std.mem.eql(u8, samples[2].name, "job_size_bucket"), true);
    expect_eq(samples[2].metric_type.? == MetricType.Histogram, true);
    expect_eq(std.mem.eql(u8, samples[2].labels[0].value, "1"), true);
    expect_eq(samples[2].value, 0.0);

    expect_eq(std.mem.eql(u8, samples[4].name, "job_size_bucket"), true);
    expect_eq(std.mem.eql(u8, samples[4].labels[0].value, "+Inf"), true);
    expect_eq(samples[4].value, 1.0);

    expect_eq(std.mem.eql(u8, samples[5].name, "job_size_sum"), true);
    expect_eq(samples[5].value, 2.0);

    expect_eq(std.mem.eql(u8, samples[6].name, "job_size_count"), true);
    expect_eq(samples[6].value, 1.0);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;
    let reg = PromRegistry.init(alloc);
    defer reg.deinit();

    // 1. Invalid metric name (starts with digit / contains hyphen)
    let err_bad_name = false;
    reg.registerCounter("123_bad", "help") catch e => {
        err_bad_name = (e == PromError.InvalidMetricName);
    };
    expect_eq(err_bad_name, true);

    let err_hyphen = false;
    reg.registerGauge("bad-metric", "help") catch e => {
        err_hyphen = (e == PromError.InvalidMetricName);
    };
    expect_eq(err_hyphen, true);

    // 2. DuplicateMetric
    reg.registerCounter("valid_metric", "first")?;
    let err_dup = false;
    reg.registerCounter("valid_metric", "second") catch e => {
        err_dup = (e == PromError.DuplicateMetric);
    };
    expect_eq(err_dup, true);

    // 3. InvalidLabelName & reserved __ prefix
    const bad_lbl: [1]LabelPair = [
        LabelPair{ name: "bad-label", value: "v" },
    ];
    let err_bad_lbl = false;
    reg.incCounter("valid_metric", &bad_lbl, 1.0) catch e => {
        err_bad_lbl = (e == PromError.InvalidLabelName);
    };
    expect_eq(err_bad_lbl, true);

    const reserved_lbl: [1]LabelPair = [
        LabelPair{ name: "__reserved", value: "v" },
    ];
    let err_res_lbl = false;
    reg.incCounter("valid_metric", &reserved_lbl, 1.0) catch e => {
        err_res_lbl = (e == PromError.InvalidLabelName);
    };
    expect_eq(err_res_lbl, true);

    // 4. NegativeCounterIncrement
    const ok_lbl: [1]LabelPair = [
        LabelPair{ name: "env", value: "prod" },
    ];
    let err_neg = false;
    reg.incCounter("valid_metric", &ok_lbl, -1.0) catch e => {
        err_neg = (e == PromError.NegativeCounterIncrement);
    };
    expect_eq(err_neg, true);

    // 5. UnsortedBuckets
    const unsorted: [3]f64 = [0.5, 0.1, 1.0];
    let err_unsorted = false;
    reg.registerHistogram("bad_hist", "help", &unsorted) catch e => {
        err_unsorted = (e == PromError.UnsortedBuckets);
    };
    expect_eq(err_unsorted, true);

    const equal_buckets: [2]f64 = [0.5, 0.5];
    let err_equal = false;
    reg.registerHistogram("bad_hist2", "help", &equal_buckets) catch e => {
        err_equal = (e == PromError.UnsortedBuckets);
    };
    expect_eq(err_equal, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mount: import cap from "cap"; cap.prometheus
    let reg = cap.time.PromRegistry.init(alloc);
    defer reg.deinit();

    reg.registerCounter("cap_dyn_requests_total", "Dynamic mount counter")?;
    const lbl: [1]cap.time.LabelPair = [
        cap.time.LabelPair{ name: "service", value: "gateway" },
    ];
    reg.incCounter("cap_dyn_requests_total", &lbl, 9.0)?;

    const text = reg.exportText(alloc)?;
    defer alloc.free(text);

    const samples = cap.time.parseExposition(alloc, text)?;
    defer cap.time.freeParsedSamples(alloc, samples);

    expect_eq(samples.len, 1 as usize);
    expect_eq(std.mem.eql(u8, samples[0].name, "cap_dyn_requests_total"), true);
    expect_eq(samples[0].metric_type.? == cap.time.MetricType.Counter, true);
    expect_eq(samples[0].value, 9.0);
}

test("cap:prometheus Counter/Gauge multi-label registration, HELP/TYPE & label escaping") {
    runTest1() catch unreachable;
}

test("cap:prometheus Histogram cumulative buckets, +Inf, _sum/_count & exportText") {
    runTest2() catch unreachable;
}

test("cap:prometheus parseExposition roundtrip & escaped label unescaping") {
    runTest3() catch unreachable;
}

test("cap:prometheus validation guards: invalid names, __reserved, negative counter, unsorted/duplicate") {
    runTest4() catch unreachable;
}

test("cap.prometheus dynamic mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
