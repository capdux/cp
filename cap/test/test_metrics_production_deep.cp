// Source: cp/cap/test/test_metrics_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:metrics & cap.metrics

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MetricsError,
    MetricLabel,
    MetricNameValidator,
    Counter,
    Gauge,
    DEFAULT_HTTP_BUCKETS,
    MetricsHistogram,
    MetricsRegistry,
} from "cap:metrics";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

fn testCounterGaugeAndValidators(): !void {
    // 1. Standalone Counter atomic operations
    let c = Counter.init();
    expect_eq(c.get(), 0 as u64);
    c.inc();
    expect_eq(c.get(), 1 as u64);
    c.add(41);
    expect_eq(c.get(), 42 as u64);
    c.add(0);
    expect_eq(c.get(), 42 as u64);
    c.reset();
    expect_eq(c.get(), 0 as u64);

    // 2. Standalone Gauge atomic operations (positive, negative, fractional)
    let g = Gauge.initWith(10.5);
    expect_eq(approxEq(g.get(), 10.5, 0.000001), true);
    g.inc();
    expect_eq(approxEq(g.get(), 11.5, 0.000001), true);
    g.dec();
    g.sub(14.75);
    expect_eq(approxEq(g.get(), -4.25, 0.000001), true);
    g.add(5.75);
    expect_eq(approxEq(g.get(), 1.5, 0.000001), true);
    g.set(99.0);
    expect_eq(approxEq(g.get(), 99.0, 0.000001), true);
    g.reset();
    expect_eq(approxEq(g.get(), 0.0, 0.000001), true);

    // 3. Prometheus MetricNameValidator: [a-zA-Z_:][a-zA-Z0-9_:]*
    expect_eq(MetricNameValidator.isValidMetricName("http_requests_total"), true);
    expect_eq(MetricNameValidator.isValidMetricName(":custom:metric_1"), true);
    expect_eq(MetricNameValidator.isValidMetricName("_internal_0"), true);
    expect_eq(MetricNameValidator.isValidMetricName("A"), true);

    expect_eq(MetricNameValidator.isValidMetricName(""), false);
    expect_eq(MetricNameValidator.isValidMetricName("123_invalid"), false);
    expect_eq(MetricNameValidator.isValidMetricName("http-requests"), false);
    expect_eq(MetricNameValidator.isValidMetricName("http.requests"), false);
    expect_eq(MetricNameValidator.isValidMetricName("metric name"), false);

    // 4. Prometheus Label Validator: [a-zA-Z_][a-zA-Z0-9_]*
    expect_eq(MetricNameValidator.isValidLabelName("method"), true);
    expect_eq(MetricNameValidator.isValidLabelName("_env_2"), true);
    expect_eq(MetricNameValidator.isValidLabelName("status_code"), true);

    expect_eq(MetricNameValidator.isValidLabelName(""), false);
    expect_eq(MetricNameValidator.isValidLabelName(":colon_start"), false);
    expect_eq(MetricNameValidator.isValidLabelName("label:colon"), false);
    expect_eq(MetricNameValidator.isValidLabelName("0digit_start"), false);
    expect_eq(MetricNameValidator.isValidLabelName("kebab-case"), false);

    // Duplicate label key check
    const dup_labels: [2]MetricLabel = [
        MetricLabel.init("env", "prod"),
        MetricLabel.init("env", "staging"),
    ];
    let dup_label_err = false;
    MetricNameValidator.validateLabels(&dup_labels) catch {
        dup_label_err = true;
    };
    expect_eq(dup_label_err, true);
}

fn testHistogramBucketsSumCountAndQuantiles(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Default HTTP latency buckets
    let h_default = MetricsHistogram.initDefault(alloc)?;
    defer h_default.deinit();
    expect_eq(h_default.bounds().len, 11 as usize);
    expect_eq(h_default.bucketCounts().len, 12 as usize);
    expect_eq(approxEq(h_default.bounds()[0], 0.005, 0.000001), true);
    expect_eq(approxEq(h_default.bounds()[10], 10.0, 0.000001), true);
    expect_eq(approxEq(h_default.estimateQuantile(0.5), 0.0, 0.000001), true);

    h_default.observe(0.003);
    h_default.observe(0.200);
    h_default.observe(15.0); // only +Inf
    expect_eq(h_default.count(), 3 as u64);
    expect_eq(approxEq(h_default.sum(), 15.203, 0.000001), true);
    expect_eq(h_default.bucketCounts()[0], 1 as u64);  // <= 0.005
    expect_eq(h_default.bucketCounts()[5], 2 as u64);  // <= 0.25
    expect_eq(h_default.bucketCounts()[10], 2 as u64); // <= 10.0
    expect_eq(h_default.bucketCounts()[11], 3 as u64); // +Inf

    // 2. Custom ordered buckets + linear interpolation quantile check
    const custom_bounds: [3]f64 = [0.1, 0.5, 1.0];
    let h = MetricsHistogram.init(alloc, &custom_bounds)?;
    defer h.deinit();

    // 4 samples in (0, 0.1]
    h.observe(0.02);
    h.observe(0.05);
    h.observe(0.08);
    h.observe(0.10);
    // 4 samples in (0.1, 0.5]
    h.observe(0.20);
    h.observe(0.30);
    h.observe(0.40);
    h.observe(0.50);
    // 2 samples in (0.5, 1.0]
    h.observe(0.70);
    h.observe(0.90);
    // NaN must be ignored without poisoning sum or count
    h.observe(std.math.nan(f64));

    expect_eq(h.count(), 10 as u64);
    expect_eq(approxEq(h.sum(), 3.25, 0.000001), true);
    const counts = h.bucketCounts();
    expect_eq(counts.len, 4 as usize);
    expect_eq(counts[0], 4 as u64);
    expect_eq(counts[1], 8 as u64);
    expect_eq(counts[2], 10 as u64);
    expect_eq(counts[3], 10 as u64);

    // Linear interpolation checks:
    // p20: rank 2.0 in [0.0, 0.1] (0..4) -> 0.0 + 0.1 * 0.5 = 0.05
    expect_eq(approxEq(h.estimateQuantile(0.20), 0.05, 0.000001), true);
    // p50: rank 5.0 in (0.1, 0.5] (4..8) -> 0.1 + 0.4 * 0.25 = 0.20
    expect_eq(approxEq(h.estimateQuantile(0.50), 0.20, 0.000001), true);
    // p95: rank 9.5 in (0.5, 1.0] (8..10) -> 0.5 + 0.5 * 0.75 = 0.875
    expect_eq(approxEq(h.estimateQuantile(0.95), 0.875, 0.000001), true);

    // Outlier into +Inf bucket: p99 falls into +Inf bucket -> returns highest finite bound (1.0)
    h.observe(3.5);
    expect_eq(h.count(), 11 as u64);
    expect_eq(h.bucketCounts()[3], 11 as u64);
    expect_eq(approxEq(h.estimateQuantile(0.99), 1.0, 0.000001), true);

    // Reset clears values while preserving bounds
    h.reset();
    expect_eq(h.count(), 0 as u64);
    expect_eq(approxEq(h.sum(), 0.0, 0.000001), true);
    expect_eq(h.bounds().len, 3 as usize);
    expect_eq(h.bucketCounts()[0], 0 as u64);

    // Checked quantile validation
    let q_err = false;
    * = h.estimateQuantileChecked(1.5) catch {
        q_err = true;
    };
    expect_eq(q_err, true);
}

fn testRegistryPrometheusExposition004(): !void {
    const alloc = std.heap.page_allocator;
    let reg = MetricsRegistry.init(alloc);
    defer reg.deinit();

    // Register counter #1 with special characters in label value & help
    const lbl_get: [2]MetricLabel = [
        MetricLabel.init("method", "GET"),
        MetricLabel.init("path", "/api/\"v1\"\\test\nline"),
    ];
    let c_get = reg.registerCounter("http_requests_total", "Total HTTP\nrequests \\ count", &lbl_get)?;
    c_get.add(7);

    // Register a Gauge in between to verify Prometheus family grouping merges headers!
    let g_mem = reg.registerGauge("agent_memory_mb", "Current memory MB", [])?;
    g_mem.set(128.5);

    // Register counter #2 in the same family as counter #1
    const lbl_post: [2]MetricLabel = [
        MetricLabel.init("method", "POST"),
        MetricLabel.init("path", "/chat"),
    ];
    let c_post = reg.registerCounter("http_requests_total", "Total HTTP\nrequests \\ count", &lbl_post)?;
    c_post.inc();
    c_post.add(2);

    // Register Histogram with custom buckets and label
    const h_labels: [1]MetricLabel = [MetricLabel.init("service", "planner")];
    const h_bounds: [2]f64 = [0.1, 0.5];
    let hist = reg.registerHistogram("step_latency_seconds", "Step latency", &h_labels, &h_bounds)?;
    hist.observe(0.0625);
    hist.observe(0.1875);
    hist.observe(1.25);

    const prom = reg.exportPrometheus(alloc)?;
    defer alloc.free(prom);

    const expected =
        "# HELP http_requests_total Total HTTP\\nrequests \\\\ count\n" ++
        "# TYPE http_requests_total counter\n" ++
        "http_requests_total{method=\"GET\",path=\"/api/\\\"v1\\\"\\\\test\\nline\"} 7\n" ++
        "http_requests_total{method=\"POST\",path=\"/chat\"} 3\n" ++
        "# HELP agent_memory_mb Current memory MB\n" ++
        "# TYPE agent_memory_mb gauge\n" ++
        "agent_memory_mb 128.5\n" ++
        "# HELP step_latency_seconds Step latency\n" ++
        "# TYPE step_latency_seconds histogram\n" ++
        "step_latency_seconds_bucket{service=\"planner\",le=\"0.1\"} 1\n" ++
        "step_latency_seconds_bucket{service=\"planner\",le=\"0.5\"} 2\n" ++
        "step_latency_seconds_bucket{service=\"planner\",le=\"+Inf\"} 3\n" ++
        "step_latency_seconds_sum{service=\"planner\"} 1.5\n" ++
        "step_latency_seconds_count{service=\"planner\"} 3\n";

    expect_eq(prom, expected);
}

fn testExportJsonAndBoundaryGuards(): !void {
    const alloc = std.heap.page_allocator;
    let reg = MetricsRegistry.init(alloc);
    defer reg.deinit();

    const lbls: [2]MetricLabel = [
        MetricLabel.init("model", "gpt-5"),
        MetricLabel.init("role", "coder"),
    ];
    let c = reg.registerCounter("tokens_total", "Token usage", &lbls)?;
    c.add(150);

    let g = reg.registerGauge("queue_depth", "Pending tasks", [])?;
    g.set(4.0);

    const bounds: [2]f64 = [0.5, 2.0];
    let h = reg.registerHistogram("infer_sec", "Inference duration", [], &bounds)?;
    h.observe(0.25);

    const json_bytes = reg.exportJson(alloc)?;
    defer alloc.free(json_bytes);

    // Parse exported JSON with std.json to guarantee strict RFC 8259 validity
    let parsed = std.json.parseFromSlice(std.json.Value, alloc, json_bytes, .{}) catch unreachable;
    defer parsed.deinit();

    const root = parsed.value.object;
    const counters_arr = root.get("counters").?.array;
    const gauges_arr = root.get("gauges").?.array;
    const hists_arr = root.get("histograms").?.array;
    expect_eq(counters_arr.items.len, 1 as usize);
    expect_eq(gauges_arr.items.len, 1 as usize);
    expect_eq(hists_arr.items.len, 1 as usize);
    expect_eq(counters_arr.items[0].object.get("name").?.string, "tokens_total");
    expect_eq(counters_arr.items[0].object.get("value").?.integer, 150 as i64);
    expect_eq(hists_arr.items[0].object.get("count").?.integer, 1 as i64);
    expect_eq(hists_arr.items[0].object.get("buckets").?.array.items.len, 3 as usize);

    // Boundary Guard 1: Duplicate metric (even with reordered labels) -> DuplicateMetric
    const lbls_rev: [2]MetricLabel = [
        MetricLabel.init("role", "coder"),
        MetricLabel.init("model", "gpt-5"),
    ];
    const dup_res = reg.registerCounter("tokens_total", "Token usage", &lbls_rev);
    const dup_err = switch (dup_res) {
        MetricsError.DuplicateMetric => true,
        else => false,
        ok => false,
    };
    expect_eq(dup_err, true);

    // Boundary Guard 2: Metric name reused with a different kind -> DuplicateMetric
    const kind_res = reg.registerGauge("tokens_total", "Conflict", []);
    const kind_err = switch (kind_res) {
        MetricsError.DuplicateMetric => true,
        else => false,
        ok => false,
    };
    expect_eq(kind_err, true);

    // Boundary Guard 3: Invalid metric name -> InvalidMetricName
    const bad_name_res = reg.registerCounter("9bad-metric", "help", []);
    const bad_name_err = switch (bad_name_res) {
        MetricsError.InvalidMetricName => true,
        else => false,
        ok => false,
    };
    expect_eq(bad_name_err, true);

    // Boundary Guard 4: Invalid label name & reserved "le" on histogram -> InvalidLabelName
    const bad_lbl: [1]MetricLabel = [MetricLabel.init("bad:key", "1")];
    const bad_lbl_res = reg.registerGauge("valid_gauge", "help", &bad_lbl);
    const bad_lbl_err = switch (bad_lbl_res) {
        MetricsError.InvalidLabelName => true,
        else => false,
        ok => false,
    };
    expect_eq(bad_lbl_err, true);

    const le_lbl: [1]MetricLabel = [MetricLabel.init("le", "0.5")];
    const le_res = reg.registerHistogram("valid_hist", "help", &le_lbl, []);
    const le_err = switch (le_res) {
        MetricsError.InvalidLabelName => true,
        else => false,
        ok => false,
    };
    expect_eq(le_err, true);

    // Boundary Guard 5: Non-monotonic bounds -> InvalidBounds
    const bad_bounds: [2]f64 = [1.0, 0.5];
    const bounds_res = reg.registerHistogram("valid_hist_2", "help", [], &bad_bounds);
    const bounds_err = switch (bounds_res) {
        MetricsError.InvalidBounds => true,
        else => false,
        ok => false,
    };
    expect_eq(bounds_err, true);
}

fn testCapMetricsAutomaticLazyMount(): !void {
    const alloc = std.heap.page_allocator;

    // Verify `import cap from "cap"; cap.metrics.*` automatic dynamic lazy mounting
    expect_eq(cap.metrics.MetricNameValidator.isValidMetricName("agent_steps_total"), true);
    expect_eq(cap.metrics.DEFAULT_HTTP_BUCKETS.len, 11 as usize);

    let reg = cap.metrics.MetricsRegistry.init(alloc);
    defer reg.deinit();

    const lbls: [1]cap.metrics.MetricLabel = [cap.metrics.MetricLabel.init("cluster", "local")];
    let cnt: cap.metrics.Counter = reg.registerCounter("hub_events_total", "Hub events", &lbls)?;
    let gg: cap.metrics.Gauge = reg.registerGauge("hub_workers", "Active workers", [])?;
    let hh: cap.metrics.MetricsHistogram = reg.registerHistogram("hub_lat_sec", "Hub latency", [], [])?;

    cnt.inc();
    cnt.add(9);
    gg.set(5.0);
    gg.dec();
    hh.observe(0.02);
    hh.observe(0.08);

    expect_eq(cnt.get(), 10 as u64);
    expect_eq(approxEq(gg.get(), 4.0, 0.000001), true);
    expect_eq(hh.count(), 2 as u64);
    expect_eq(hh.estimateQuantile(0.5) > 0.0, true);

    const prom = reg.exportPrometheus(alloc)?;
    defer alloc.free(prom);
    expect_eq(std.mem.indexOf(u8, prom, "hub_events_total{cluster=\"local\"} 10") != null, true);
    expect_eq(std.mem.indexOf(u8, prom, "hub_workers 4") != null, true);
    expect_eq(std.mem.indexOf(u8, prom, "hub_lat_sec_bucket{le=\"+Inf\"} 2") != null, true);

    const js = reg.exportJson(alloc)?;
    defer alloc.free(js);
    expect_eq(std.mem.indexOf(u8, js, "\"name\":\"hub_events_total\"") != null, true);
}

fn main() {
    test("1. Counter/Gauge Atomic Operations & Prometheus Name/Label Validation", () => {
        testCounterGaugeAndValidators()?;
    });
    test("2. MetricsHistogram Observations, Cumulative Buckets, Sum/Count & Quantiles", () => {
        testHistogramBucketsSumCountAndQuantiles()?;
    });
    test("3. MetricsRegistry.exportPrometheus 0.0.4 Format, Escaping & +Inf Bucket", () => {
        testRegistryPrometheusExposition004()?;
    });
    test("4. MetricsRegistry.exportJson Snapshot & Duplicate/Invalid Boundary Guards", () => {
        testExportJsonAndBoundaryGuards()?;
    });
    test("5. import cap from \"cap\"; cap.metrics Automatic Dynamic Lazy Mounting", () => {
        testCapMetricsAutomaticLazyMount()?;
    });
}
