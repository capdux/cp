// Source: cp/cap/test/test_sketch_production_deep.cp
// Deep Production Verification Suite for cap:sketch & cap.sketch
// Covers:
//   1. HyperLogLog small cardinality (50), medium scale (20,000 unique + heavy duplicates) < 5% error, and lossless merge
//   2. QuantileDigest 10,000 streaming values (P50/P95/P99 accuracy) and distributed merge across split halves
//   3. ReservoirSampler(u64) 10,000 observations retaining fixed k=100 samples deterministically, seenCount == 10000
//   4. Boundary parameter rejection (InvalidPrecision, InvalidQuantile, InvalidValue(NaN/Inf), InvalidCapacity, IncompatibleMerge)
//   5. `import cap from "cap"; cap.sketch` dynamic mounting verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SketchError,
    HyperLogLog,
    QuantileDigest,
    ReservoirSampler,
} from "cap:db";

test("cap:sketch - HyperLogLog small, 20k cardinality with duplicates & lossless merge") {
    const alloc = std.heap.page_allocator;

    // Small cardinality (~50) using LinearCounting bias correction
    let hll_small = HyperLogLog.init(alloc, 12) catch unreachable;
    defer hll_small.deinit();

    let s: u64 = 0;
    while (s < 50) : (s += 1) {
        hll_small.addU64(s);
        hll_small.addU64(s); // duplicate
    }
    const est_small = hll_small.count();
    expect_eq(est_small >= 45 && est_small <= 55, true);

    // Medium scale: 20,000 distinct items + heavy duplicates (precision 14 -> 16 KiB, std error ~0.81%)
    let hll = HyperLogLog.init(alloc, 14) catch unreachable;
    defer hll.deinit();

    let buf: [32]u8 = [0; 32];
    let i: u64 = 0;
    while (i < 20000) : (i += 1) {
        const msg = std.fmt.bufPrint(&buf, "user-id-{d}", .{i}) catch unreachable;
        hll.add(msg);
        if ((i % 3) == 0) {
            hll.add(msg); // duplicate
        }
    }
    const est_20k = hll.count();
    // 20,000 +/- 5% => [19000, 21000]
    expect_eq(est_20k >= 19000 && est_20k <= 21000, true);

    // Distributed merge: two shards with overlapping keys ([0..12000) and [8000..20000))
    let shard_a = HyperLogLog.init(alloc, 14) catch unreachable;
    defer shard_a.deinit();
    let shard_b = HyperLogLog.init(alloc, 14) catch unreachable;
    defer shard_b.deinit();

    let a: u64 = 0;
    while (a < 12000) : (a += 1) {
        shard_a.addU64(a + 1000000);
    }
    let b: u64 = 8000;
    while (b < 20000) : (b += 1) {
        shard_b.addU64(b + 1000000);
    }

    shard_a.merge(shard_b) catch unreachable;
    const merged_est = shard_a.count();
    expect_eq(merged_est >= 19000 && merged_est <= 21000, true);
}

test("cap:sketch - QuantileDigest 10k streaming P50/P95/P99 & distributed merge") {
    const alloc = std.heap.page_allocator;

    let qd = QuantileDigest.init(alloc, 128) catch unreachable;
    defer qd.deinit();

    // Empty digest returns null
    const empty_q = qd.quantile(0.5) catch unreachable;
    expect_eq(empty_q == null, true);
    expect_eq(qd.min() == null, true);
    expect_eq(qd.max() == null, true);

    // Push 10,000 values in pseudo-permuted order spanning [0.0, 9999.0]
    let i: u64 = 0;
    while (i < 10000) : (i += 1) {
        // Bijection mod 10000 since gcd(7919, 10000) == 1
        const v = ((i * 7919) % 10000) as f64;
        qd.add(v) catch unreachable;
    }

    expect_eq(qd.count(), 10000);
    expect_eq(qd.min().?, 0.0);
    expect_eq(qd.max().?, 9999.0);

    const p50 = (qd.quantile(0.50) catch unreachable).?;
    const p95 = (qd.quantile(0.95) catch unreachable).?;
    const p99 = (qd.quantile(0.99) catch unreachable).?;

    // True values: P50=4999.5, P95=9499.5, P99=9899.5
    expect_eq(p50 >= 4900.0 && p50 <= 5100.0, true);
    expect_eq(p95 >= 9400.0 && p95 <= 9600.0, true);
    expect_eq(p99 >= 9850.0 && p99 <= 9950.0, true);

    // Distributed merge across two disjoint halves: [0..5000) and [5000..10000)
    let left = QuantileDigest.init(alloc, 100) catch unreachable;
    defer left.deinit();
    let right = QuantileDigest.init(alloc, 100) catch unreachable;
    defer right.deinit();

    let k: u64 = 0;
    while (k < 5000) : (k += 1) {
        left.add(k as f64) catch unreachable;
        right.add((k + 5000) as f64) catch unreachable;
    }

    left.merge(right);
    expect_eq(left.count(), 10000);
    const merged_p95 = (left.quantile(0.95) catch unreachable).?;
    expect_eq(merged_p95 >= 9400.0 && merged_p95 <= 9600.0, true);
}

test("cap:sketch - ReservoirSampler(u64) 10k stream deterministic k=100 sampling") {
    const alloc = std.heap.page_allocator;

    let r1 = ReservoirSampler(u64).init(alloc, 100, 0xCAFEBABE) catch unreachable;
    defer r1.deinit();
    let r2 = ReservoirSampler(u64).init(alloc, 100, 0xCAFEBABE) catch unreachable;
    defer r2.deinit();

    let i: u64 = 0;
    while (i < 10000) : (i += 1) {
        r1.observe(i);
        r2.observe(i);
    }

    expect_eq(r1.seenCount(), 10000);
    expect_eq(r1.samples().len, 100);

    const s1 = r1.samples();
    const s2 = r2.samples();
    let idx: usize = 0;
    let sum: u64 = 0;
    while (idx < 100) : (idx += 1) {
        expect_eq(s1[idx], s2[idx]);
        expect_eq(s1[idx] < 10000, true);
        sum += s1[idx];
    }

    // Mean of uniform [0, 10000) is ~5000; 100 samples sum should be well within [300000, 700000]
    expect_eq(sum >= 300000 && sum <= 700000, true);

    // Reset and re-observe fewer than capacity
    r1.reset(42);
    expect_eq(r1.seenCount(), 0);
    expect_eq(r1.samples().len, 0);
    r1.observe(777);
    expect_eq(r1.seenCount(), 1);
    expect_eq(r1.samples().len, 1);
    expect_eq(r1.samples()[0], 777);
}

test("cap:sketch - boundary & error validation") {
    const alloc = std.heap.page_allocator;

    // InvalidPrecision (< 4 or > 16)
    let bad_p_low = false;
    * = HyperLogLog.init(alloc, 3) catch |e| {
        if (e == SketchError.InvalidPrecision) bad_p_low = true;
    };
    expect_eq(bad_p_low, true);

    let bad_p_high = false;
    * = HyperLogLog.init(alloc, 17) catch |e| {
        if (e == SketchError.InvalidPrecision) bad_p_high = true;
    };
    expect_eq(bad_p_high, true);

    // IncompatibleMerge
    let h1 = HyperLogLog.init(alloc, 8) catch unreachable;
    defer h1.deinit();
    let h2 = HyperLogLog.init(alloc, 10) catch unreachable;
    defer h2.deinit();
    let bad_merge = false;
    h1.merge(h2) catch |e| {
        if (e == SketchError.IncompatibleMerge) bad_merge = true;
    };
    expect_eq(bad_merge, true);

    // InvalidCapacity for QuantileDigest & ReservoirSampler
    let bad_qd_cap = false;
    * = QuantileDigest.init(alloc, 1) catch |e| {
        if (e == SketchError.InvalidCapacity) bad_qd_cap = true;
    };
    expect_eq(bad_qd_cap, true);

    let bad_rs_cap = false;
    * = ReservoirSampler(u64).init(alloc, 0, 1) catch |e| {
        if (e == SketchError.InvalidCapacity) bad_rs_cap = true;
    };
    expect_eq(bad_rs_cap, true);

    // InvalidValue (NaN / Inf) & InvalidQuantile (< 0 or > 1)
    let qd = QuantileDigest.init(alloc, 16) catch unreachable;
    defer qd.deinit();
    qd.add(10.0) catch unreachable;

    let bad_nan = false;
    qd.add(std.math.nan(f64)) catch |e| {
        if (e == SketchError.InvalidValue) bad_nan = true;
    };
    expect_eq(bad_nan, true);

    let bad_inf = false;
    qd.add(std.math.inf(f64)) catch |e| {
        if (e == SketchError.InvalidValue) bad_inf = true;
    };
    expect_eq(bad_inf, true);

    let bad_q_low = false;
    * = qd.quantile(-0.01) catch |e| {
        if (e == SketchError.InvalidQuantile) bad_q_low = true;
    };
    expect_eq(bad_q_low, true);

    let bad_q_high = false;
    * = qd.quantile(1.01) catch |e| {
        if (e == SketchError.InvalidQuantile) bad_q_high = true;
    };
    expect_eq(bad_q_high, true);
}

test("cap:sketch - import cap from \"cap\"; cap.sketch dynamic mounting") {
    const alloc = std.heap.page_allocator;

    let hll = cap.db.HyperLogLog.init(alloc, 10) catch unreachable;
    defer hll.deinit();
    hll.add("alpha");
    hll.add("beta");
    hll.add("alpha");
    expect_eq(hll.count(), 2);

    let qd = cap.db.QuantileDigest.init(alloc, 32) catch unreachable;
    defer qd.deinit();
    qd.add(10.0) catch unreachable;
    qd.add(20.0) catch unreachable;
    qd.add(30.0) catch unreachable;
    const med = (qd.quantile(0.5) catch unreachable).?;
    expect_eq(med >= 18.0 && med <= 22.0, true);

    let rs = cap.db.ReservoirSampler(i32).init(alloc, 4, 99) catch unreachable;
    defer rs.deinit();
    rs.observe(11);
    rs.observe(22);
    expect_eq(rs.seenCount(), 2);
    expect_eq(rs.samples().len, 2);
}
