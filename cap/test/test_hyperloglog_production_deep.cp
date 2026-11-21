// Source: cp/cap/test/test_hyperloglog_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { DEFAULT_PRECISION, HllError, HyperLogLog } from "cap:hyperloglog";

test "hll: 10000 unique items within 3% relative error" {
    const alloc = std.testing.allocator;
    let hll = try HyperLogLog.init(alloc, DEFAULT_PRECISION);
    defer hll.deinit(alloc);

    let i: u64 = 0;
    while (i < 10000) : (i += 1) {
        hll.addU64(i);
    }
    // Re-adding duplicates must not inflate cardinality
    let j: u64 = 0;
    while (j < 5000) : (j += 1) {
        hll.addU64(j);
    }
    const est = hll.count();
    try expect_eq(est >= 9700 and est <= 10300, true);
}

test "hll: small-range LinearCounting accuracy & empty count" {
    const alloc = std.testing.allocator;
    let hll = try HyperLogLog.init(alloc, 10);
    defer hll.deinit(alloc);
    try expect_eq(hll.count(), 0);

    let i: u64 = 0;
    while (i < 50) : (i += 1) {
        hll.addU64(i * 17);
    }
    const est = hll.count();
    try expect_eq(est >= 45 and est <= 55, true);
}

test "hll: merge two disjoint shards and precision mismatch guard" {
    const alloc = std.testing.allocator;
    let a = try HyperLogLog.init(alloc, 12);
    defer a.deinit(alloc);
    let b = try HyperLogLog.init(alloc, 12);
    defer b.deinit(alloc);
    let c = try HyperLogLog.init(alloc, 10);
    defer c.deinit(alloc);

    let i: u64 = 0;
    while (i < 2000) : (i += 1) { a.addU64(i); }
    while (i < 4000) : (i += 1) { b.addU64(i); }

    try a.merge(b);
    const est = a.count();
    try expect_eq(est >= 3800 and est <= 4200, true);
    try expect_eq(a.merge(c) == HllError.PrecisionMismatch, true);
}

test "hll: serialize & deserialize roundtrip" {
    const alloc = std.testing.allocator;
    let hll = try HyperLogLog.init(alloc, 10);
    defer hll.deinit(alloc);
    hll.addBytes("alpha");
    hll.addBytes("beta");
    hll.addBytes("gamma");

    const raw = try hll.serialize(alloc);
    defer alloc.free(raw);
    let hll2 = try HyperLogLog.deserialize(alloc, raw);
    defer hll2.deinit(alloc);
    try expect_eq(hll.count(), hll2.count());
}

test "hll: cap.hyperloglog dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    let hll = try cap.hyperloglog.HyperLogLog.init(alloc, 8);
    defer hll.deinit(alloc);
    hll.addBytes("dyn-key");
    try expect_eq(hll.count(), 1);
}
