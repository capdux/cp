// Production deep regression tests for lib/event/delayMonitor.cp (cap:event)

import { test, expect_eq } from "cap:test";
import {
    DELAY_HISTOGRAM_BUCKETS,
    DELAY_DEFAULT_RESOLUTION_MS,
    DELAY_NS_PER_MS,
    DelayMonitorError,
    DelayHistogram,
    EventLoopDelayMonitor,
} from "cap:event";

test("delayMonitor: constants and logarithmic bucket mapping", () => {
    expect_eq(DELAY_HISTOGRAM_BUCKETS, 64);
    expect_eq(DELAY_DEFAULT_RESOLUTION_MS, 10);
    expect_eq(DELAY_NS_PER_MS, 1_000_000);

    expect_eq(DelayHistogram.bucketIndex(0), 0);
    expect_eq(DelayHistogram.bucketIndex(1), 1);
    expect_eq(DelayHistogram.bucketIndex(2), 2);
    expect_eq(DelayHistogram.bucketIndex(3), 2);
    expect_eq(DelayHistogram.bucketIndex(4), 3);
    expect_eq(DelayHistogram.bucketIndex(7), 3);
    expect_eq(DelayHistogram.bucketIndex(1_000_000), 20);

    expect_eq(DelayHistogram.bucketUpperBoundNs(0), 0);
    expect_eq(DelayHistogram.bucketUpperBoundNs(1), 1);
    expect_eq(DelayHistogram.bucketUpperBoundNs(2), 3);
    expect_eq(DelayHistogram.bucketUpperBoundNs(3), 7);
});

test("delayMonitor: enable/disable lifecycle and baseline firing", () => {
    let mon = EventLoopDelayMonitor.init(10) catch EventLoopDelayMonitor{};
    expect_eq(mon.isEnabled(), false);
    expect_eq(mon.expectedIntervalNs(), 10_000_000);

    // Disabled monitor ignores onFire and disable
    expect_eq(mon.onFire(100_000_000) == null, true);
    expect_eq(mon.disable(), false);
    expect_eq(mon.shouldFire(100_000_000), false);

    // Enable is idempotent
    expect_eq(mon.enable(1_000_000_000), true);
    expect_eq(mon.enable(1_000_000_000), false);
    expect_eq(mon.isEnabled(), true);
    expect_eq(mon.next_fire_ns, 1_010_000_000);
    expect_eq(mon.shouldFire(1_009_999_999), false);
    expect_eq(mon.shouldFire(1_010_000_000), true);

    // First fire establishes baseline without recording a sample
    expect_eq(mon.onFire(1_010_000_000) == null, true);
    expect_eq(mon.histogram.count, 0);
    expect_eq(mon.last_fire_ns, 1_010_000_000);
    expect_eq(mon.next_fire_ns, 1_020_000_000);

    // Disable clears active schedule but preserves histogram
    expect_eq(mon.disable(), true);
    expect_eq(mon.isEnabled(), false);
    expect_eq(mon.last_fire_ns, 0);
});

test("delayMonitor: delay measurement, zero-jitter & clock-backward guard", () => {
    let mon = EventLoopDelayMonitor.init(10) catch EventLoopDelayMonitor{};
    * = mon.enable(100_000_000);
    // Baseline fire at 110ms
    * = mon.onFire(110_000_000);

    // Exact on-time fire at 120ms => 0ns delay
    const d1 = mon.onFire(120_000_000);
    expect_eq(d1 ?? 999, 0);

    // Early fire at 128ms (8ms < 10ms) => 0ns delay
    const d2 = mon.onFire(128_000_000);
    expect_eq(d2 ?? 999, 0);

    // Delayed fire at 143ms (15ms > 10ms) => 5_000_000ns delay
    const d3 = mon.onFire(143_000_000);
    expect_eq(d3 ?? 0, 5_000_000);

    // Clock step backward (140ms < 143ms) => clamped safely to 0ns
    const d4 = mon.onFire(140_000_000);
    expect_eq(d4 ?? 999, 0);

    expect_eq(mon.histogram.count, 4);
    expect_eq(mon.histogram.min_ns, 0);
    expect_eq(mon.histogram.max_ns, 5_000_000);
    expect_eq(mon.histogram.meanNs(), 1_250_000);
});

test("delayMonitor: histogram percentiles, clamping & reset", () => {
    let mon = EventLoopDelayMonitor.initWithMax(10, 50_000_000) catch EventLoopDelayMonitor{};
    * = mon.enable(0);
    * = mon.onFire(10_000_000); // baseline

    // Record 4 samples: 1ms, 2ms, 4ms, 100ms (clamped to 50ms)
    * = mon.onFire(21_000_000);  // +11ms => delay 1ms
    * = mon.onFire(33_000_000);  // +12ms => delay 2ms
    * = mon.onFire(47_000_000);  // +14ms => delay 4ms
    * = mon.onFire(157_000_000); // +110ms => delay 100ms -> clamped to 50ms

    expect_eq(mon.histogram.count, 4);
    expect_eq(mon.histogram.exceeds_count, 1);
    expect_eq(mon.histogram.min_ns, 1_000_000);
    expect_eq(mon.histogram.max_ns, 50_000_000);

    const p0 = mon.histogram.percentile(0.0) catch 999;
    const p50 = mon.histogram.percentile(50.0) catch 999;
    const p100 = mon.histogram.percentile(100.0) catch 999;
    expect_eq(p0, 1_000_000);
    expect_eq(p50 >= 2_000_000, true);
    expect_eq(p50 <= 4_194_303, true);
    expect_eq(p100, 50_000_000);

    // Reset clears stats but preserves enabled state and max_recordable_ns
    mon.reset();
    expect_eq(mon.isEnabled(), true);
    expect_eq(mon.histogram.count, 0);
    expect_eq(mon.histogram.exceeds_count, 0);
    expect_eq(mon.histogram.max_recordable_ns, 50_000_000);
    expect_eq(mon.histogram.meanNs(), 0);
});

test("delayMonitor: validation error guards (InvalidResolution & InvalidPercentile)", () => {
    let bad_res = false;
    * = EventLoopDelayMonitor.init(0) catch |e| {
        bad_res = (e == DelayMonitorError.InvalidResolution);
        EventLoopDelayMonitor{}
    };
    expect_eq(bad_res, true);

    let bad_res_max = false;
    * = EventLoopDelayMonitor.initWithMax(0, 1000) catch |e| {
        bad_res_max = (e == DelayMonitorError.InvalidResolution);
        EventLoopDelayMonitor{}
    };
    expect_eq(bad_res_max, true);

    let h = DelayHistogram.init();
    h.record(100);

    let bad_neg = false;
    * = h.percentile(-1.0) catch |e| {
        bad_neg = (e == DelayMonitorError.InvalidPercentile);
        0
    };
    expect_eq(bad_neg, true);

    let bad_over = false;
    * = h.percentile(100.5) catch |e| {
        bad_over = (e == DelayMonitorError.InvalidPercentile);
        0
    };
    expect_eq(bad_over, true);
});
