// Source: cp/cap/test/test_tdigest_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { TDigestError, TDigest } from "cap:tdigest";

test "tdigest: 10000 uniform samples p50, p95, p99 accuracy" {
    const alloc = std.testing.allocator;
    let td = try TDigest.init(alloc, 100.0);
    defer td.deinit();

    let i: usize = 1;
    while (i <= 10000) : (i += 1) {
        const v: f64 = @floatFromInt(i);
        try td.observe(v);
    }

    const p50 = try td.quantile(0.50);
    const p95 = try td.quantile(0.95);
    const p99 = try td.quantile(0.99);

    try expect_eq(@abs(p50 - 5000.0) < 100.0, true);
    try expect_eq(@abs(p95 - 9500.0) < 100.0, true);
    try expect_eq(@abs(p99 - 9900.0) < 50.0, true);
    try expect_eq(td.count(), 10000.0);
}

test "tdigest: merge two sharded digests preserves quantiles" {
    const alloc = std.testing.allocator;
    let a = try TDigest.init(alloc, 100.0);
    defer a.deinit();
    let b = try TDigest.init(alloc, 100.0);
    defer b.deinit();

    let i: usize = 1;
    while (i <= 500) : (i += 1) { try a.observe(@floatFromInt(i)); }
    while (i <= 1000) : (i += 1) { try b.observe(@floatFromInt(i)); }

    try a.merge(&b);
    try expect_eq(a.count(), 1000.0);
    const p50 = try a.quantile(0.50);
    const p90 = try a.quantile(0.90);
    try expect_eq(@abs(p50 - 500.0) < 25.0, true);
    try expect_eq(@abs(p90 - 900.0) < 25.0, true);
}

test "tdigest: min/max extremes and single-point digest" {
    const alloc = std.testing.allocator;
    let td = try TDigest.init(alloc, 50.0);
    defer td.deinit();
    try td.observe(42.5);
    try expect_eq(try td.quantile(0.0), 42.5);
    try expect_eq(try td.quantile(0.5), 42.5);
    try expect_eq(try td.quantile(1.0), 42.5);
}

test "tdigest: error guards for empty, invalid compression, and invalid quantile" {
    const alloc = std.testing.allocator;
    try expect_eq(TDigest.init(alloc, 5.0) == TDigestError.InvalidCompression, true);
    let td = try TDigest.init(alloc, 50.0);
    defer td.deinit();
    try expect_eq(td.quantile(0.5) == TDigestError.EmptyDigest, true);
    try expect_eq(td.quantile(1.5) == TDigestError.InvalidQuantile, true);
}

test "tdigest: cap.tdigest dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    let td = try cap.tdigest.TDigest.init(alloc, 50.0);
    defer td.deinit();
    try td.observe(10.0);
    try td.observe(20.0);
    const q = try td.quantile(0.5);
    try expect_eq(q >= 10.0 and q <= 20.0, true);
}
