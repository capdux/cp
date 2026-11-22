// cap/test/testEventLoopDelay.cp
// Production-grade deep tests for cap:time eventLoopDelay.cp
// (Event loop delay monitor & zero-alloc log2 nanosecond histogram absorbed from Bun EventLoopDelayMonitor.cp)

import { test, assert, assertEq } from "cap:test";
import {
    EVENT_LOOP_DELAY_DEFAULT_RESOLUTION_MS,
    EVENT_LOOP_DELAY_NS_PER_MS,
    EVENT_LOOP_DELAY_MAX_RECORDABLE_NS,
    EVENT_LOOP_DELAY_HIST_BUCKETS,
    EventLoopDelayError,
    EventLoopDelaySample,
    EventLoopDelayStats,
    EventLoopDelayMonitor,
} from "cap:time";

test("EventLoopDelayMonitor: init, enable/disable lifecycle & baseline tick", () => {
    assertEq(EVENT_LOOP_DELAY_DEFAULT_RESOLUTION_MS, 10);
    assertEq(EVENT_LOOP_DELAY_NS_PER_MS, 1_000_000);
    assertEq(EVENT_LOOP_DELAY_HIST_BUCKETS, 44);

    let mon = EventLoopDelayMonitor.init(10) catch {
        assert(false);
        return;
    };
    assert(!mon.isEnabled());
    assertEq(mon.expectedIntervalNs(), 10_000_000);

    // Firing while disabled returns NotEnabled
    let got_not_enabled = false;
    * = mon.onFire(100_000_000) catch |e| {
        assertEq(e, EventLoopDelayError.NotEnabled);
        got_not_enabled = true;
        EventLoopDelaySample{}
    };
    assert(got_not_enabled);

    // Enable schedules first tick at now + 10ms; second enable is idempotent
    const t0: u64 = 1_000_000_000;
    const next1 = mon.enable(t0);
    assertEq(next1, t0 + 10_000_000);
    assert(mon.isEnabled());
    const next_dup = mon.enable(t0 + 5_000_000);
    assertEq(next_dup, next1);

    // First onFire establishes baseline without recording a delay sample
    const s0 = mon.onFire(t0 + 10_000_000) catch {
        assert(false);
        return;
    };
    assert(!s0.recorded);
    assertEq(s0.delay_ns, 0);
    assertEq(s0.next_fire_ns, t0 + 20_000_000);
    assertEq(mon.count, 0);

    // Disable clears active state
    mon.disable();
    assert(!mon.isEnabled());
    assertEq(mon.last_fire_ns, 0);
    assertEq(mon.next_fire_ns, 0);
});

test("EventLoopDelayMonitor: onFire delay computation, on-time ticks & clock regression", () => {
    let mon = EventLoopDelayMonitor.init(10) catch {
        assert(false);
        return;
    };
    const base: u64 = 5_000_000_000;
    * = mon.enable(base);

    // Baseline
    * = mon.onFire(base + 10_000_000) catch {
        assert(false);
        return;
    };

    // Exactly on time (actual == 10ms <= expected): no delay recorded
    const s_ontime = mon.onFire(base + 20_000_000) catch {
        assert(false);
        return;
    };
    assert(!s_ontime.recorded);
    assertEq(s_ontime.delay_ns, 0);
    assertEq(mon.count, 0);

    // Delayed by 3.5ms (actual == 13.5ms)
    const s_late1 = mon.onFire(base + 33_500_000) catch {
        assert(false);
        return;
    };
    assert(s_late1.recorded);
    assertEq(s_late1.delay_ns, 3_500_000);
    assert(!s_late1.exceeded_cap);
    assertEq(s_late1.next_fire_ns, base + 43_500_000);

    // Clock regression (now < last_fire_ns): safely clamped to 0, no panic or underflow
    const s_back = mon.onFire(base + 30_000_000) catch {
        assert(false);
        return;
    };
    assert(!s_back.recorded);
    assertEq(s_back.delay_ns, 0);
    assertEq(mon.count, 1);

    // Delayed by 12ms from new baseline (30ms -> 52ms, actual=22ms, expected=10ms)
    const s_late2 = mon.onFire(base + 52_000_000) catch {
        assert(false);
        return;
    };
    assert(s_late2.recorded);
    assertEq(s_late2.delay_ns, 12_000_000);
    assertEq(mon.count, 2);
    assertEq(mon.min_ns, 3_500_000);
    assertEq(mon.max_ns, 12_000_000);
    assertEq(mon.meanNs(), 7_750_000);
});

test("EventLoopDelayMonitor: histogram percentiles, stats snapshot & reset", () => {
    let mon = EventLoopDelayMonitor.init(5) catch {
        assert(false);
        return;
    };

    // Record 100 deterministic samples: 90 fast (100us), 5 medium (2ms), 5 slow (50ms)
    let i: usize = 0;
    while (i < 90) {
        * = mon.recordDelay(100_000);
        i += 1;
    }
    i = 0;
    while (i < 5) {
        * = mon.recordDelay(2_000_000);
        i += 1;
    }
    i = 0;
    while (i < 5) {
        * = mon.recordDelay(50_000_000);
        i += 1;
    }

    const st = mon.stats();
    assertEq(st.count, 100);
    assertEq(st.exceeds, 0);
    assertEq(st.min_ns, 100_000);
    assertEq(st.max_ns, 50_000_000);
    assertEq(st.mean_ns, 2_690_000);

    // Verify monotonic percentiles in expected log2 ranges
    assert(st.p50_ns >= 100_000 && st.p50_ns <= 200_000);
    assert(st.p95_ns >= 2_000_000 && st.p95_ns <= 4_194_303);
    assert(st.p99_ns >= 50_000_000 && st.p99_ns <= 50_000_000);

    const p0 = mon.percentileNs(0.0) catch {
        assert(false);
        return;
    };
    const p100 = mon.percentileNs(100.0) catch {
        assert(false);
        return;
    };
    assertEq(p0, 100_000);
    assertEq(p100, 50_000_000);

    // Disable preserves collected stats; reset clears them
    * = mon.enable(1000);
    mon.disable();
    assertEq(mon.count, 100);

    mon.reset();
    const empty = mon.stats();
    assertEq(empty.count, 0);
    assertEq(empty.min_ns, 0);
    assertEq(empty.max_ns, 0);
    assertEq(empty.mean_ns, 0);
    assertEq(empty.p50_ns, 0);
});

test("EventLoopDelayMonitor: max_recordable_ns clamping & exceeds counter", () => {
    let mon = EventLoopDelayMonitor.initWithMax(10, 5_000_000) catch {
        assert(false);
        return;
    };
    * = mon.enable(1_000_000);
    * = mon.onFire(11_000_000) catch {
        assert(false);
        return;
    };

    // Delay = 20ms > cap 5ms -> clamped to 5ms, exceeds incremented
    const s = mon.onFire(41_000_000) catch {
        assert(false);
        return;
    };
    assert(s.recorded);
    assert(s.exceeded_cap);
    assertEq(s.delay_ns, 20_000_000);
    assertEq(mon.exceeds, 1);
    assertEq(mon.max_ns, 5_000_000);
    assertEq(mon.min_ns, 5_000_000);
});

test("EventLoopDelayMonitor: error guards (InvalidResolution, InvalidPercentile)", () => {
    let got_bad_res = false;
    * = EventLoopDelayMonitor.init(0) catch |e| {
        assertEq(e, EventLoopDelayError.InvalidResolution);
        got_bad_res = true;
        EventLoopDelayMonitor{}
    };
    assert(got_bad_res);

    let mon = EventLoopDelayMonitor.init(10) catch {
        assert(false);
        return;
    };
    assertEq(mon.max_recordable_ns, EVENT_LOOP_DELAY_MAX_RECORDABLE_NS);

    let got_neg_p = false;
    * = mon.percentileNs(-1.0) catch |e| {
        assertEq(e, EventLoopDelayError.InvalidPercentile);
        got_neg_p = true;
        0
    };
    assert(got_neg_p);

    let got_high_p = false;
    * = mon.percentileNs(100.5) catch |e| {
        assertEq(e, EventLoopDelayError.InvalidPercentile);
        got_high_p = true;
        0
    };
    assert(got_high_p);
});
