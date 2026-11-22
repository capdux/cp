// Source: cp/cap/test/test_hll_production_deep.cp
// Deep Production Verification Suite for cap:hll & cap.hll
// Covers:
//   1. Small cardinality (0, 10, 100 with duplicates) LinearCounting accurate estimation
//   2. Large cardinality (20,000 distinct keys + duplicates, p=14) < 3% relative error & metadata/clear
//   3. Multi-shard (2 & 3 instances) parallel counting + lossless merge + PrecisionMismatch interception
//   4. serialize/deserialize binary round-trip + InvalidSerializedData (bad magic/truncated) & InvalidPrecision interception
//   5. `import cap from "cap"; cap.hll` dynamic mounting + `std.testing.allocator` zero-leak verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { HyperLogLog, HllError } from "cap:ds";

test("cap:hll - small cardinality (0, 10, 100 with duplicates) LinearCounting") {
    const alloc = std.testing.allocator;

    let hll = HyperLogLog.init(alloc, 12) catch unreachable;
    defer hll.deinit();

    // 0 distinct items
    expect_eq(hll.count(), 0);

    // 10 distinct items with duplicates
    let i: u64 = 0;
    while (i < 10) : (i += 1) {
        hll.addHash(i);
        hll.addHash(i);
    }
    const est_10 = hll.count();
    expect_eq(est_10 >= 9 && est_10 <= 11, true);

    // 100 distinct items (including previous 10) with duplicates
    while (i < 100) : (i += 1) {
        hll.addHash(i);
        hll.addHash(i);
    }
    const est_100 = hll.count();
    expect_eq(est_100 >= 92 && est_100 <= 108, true);
}

test("cap:hll - large cardinality (20,000 distinct + duplicates, p=14) < 3% error & clear") {
    const alloc = std.testing.allocator;

    let hll = HyperLogLog.init(alloc, 14) catch unreachable;
    defer hll.deinit();

    expect_eq(hll.precision(), 14);
    expect_eq(hll.registerCount(), 16384);
    const err_bound = hll.relativeErrorBound();
    expect_eq(err_bound > 0.008 && err_bound < 0.009, true);

    let buf: [32]u8 = [0; 32];
    let i: u64 = 0;
    while (i < 20000) : (i += 1) {
        const key = std.fmt.bufPrint(&buf, "tenant-key-{d}", .{i}) catch unreachable;
        hll.add(key);
        if ((i % 2) == 0) {
            hll.add(key); // heavy duplicate
        }
    }

    const est = hll.count();
    // 20,000 +/- 3% => [19400, 20600]
    expect_eq(est >= 19400 && est <= 20600, true);

    hll.clear();
    expect_eq(hll.count(), 0);
}

test("cap:hll - 3-shard distributed merge & PrecisionMismatch interception") {
    const alloc = std.testing.allocator;

    let s1 = HyperLogLog.init(alloc, 14) catch unreachable;
    defer s1.deinit();
    let s2 = HyperLogLog.init(alloc, 14) catch unreachable;
    defer s2.deinit();
    let s3 = HyperLogLog.init(alloc, 14) catch unreachable;
    defer s3.deinit();

    // Overlapping shards covering [0, 18000)
    let k: u64 = 0;
    while (k < 8000) : (k += 1) {
        s1.addHash(k + 50000);
    }
    k = 5000;
    while (k < 13000) : (k += 1) {
        s2.addHash(k + 50000);
    }
    k = 10000;
    while (k < 18000) : (k += 1) {
        s3.addHash(k + 50000);
    }

    s1.merge(s2) catch unreachable;
    s1.merge(s3) catch unreachable;
    const merged_est = s1.count();
    // 18,000 +/- 3% => [17460, 18540]
    expect_eq(merged_est >= 17460 && merged_est <= 18540, true);

    // PrecisionMismatch interception
    let other_p = HyperLogLog.init(alloc, 10) catch unreachable;
    defer other_p.deinit();
    let mismatch_caught = false;
    s1.merge(other_p) catch |e| {
        if (e == HllError.PrecisionMismatch) {
            mismatch_caught = true;
        }
    };
    expect_eq(mismatch_caught, true);
}

test("cap:hll - serialize/deserialize round-trip & malformed/precision error interception") {
    const alloc = std.testing.allocator;

    // InvalidPrecision interception (p < 4 or p > 16)
    let low_p_err = false;
    * = HyperLogLog.init(alloc, 3) catch |e| {
        if (e == HllError.InvalidPrecision) {
            low_p_err = true;
        }
    };
    expect_eq(low_p_err, true);

    let high_p_err = false;
    * = HyperLogLog.init(alloc, 17) catch |e| {
        if (e == HllError.InvalidPrecision) {
            high_p_err = true;
        }
    };
    expect_eq(high_p_err, true);

    // Serialize & deserialize exact round-trip
    let src = HyperLogLog.init(alloc, 10) catch unreachable;
    defer src.deinit();
    let i: u64 = 0;
    while (i < 500) : (i += 1) {
        src.addHash(i * 31 + 7);
    }
    const before_count = src.count();

    const payload = src.serialize(alloc) catch unreachable;
    defer alloc.free(payload);
    expect_eq(payload.len, 5 + 1024);

    let dst = HyperLogLog.deserialize(alloc, payload) catch unreachable;
    defer dst.deinit();
    expect_eq(dst.precision(), 10);
    expect_eq(dst.registerCount(), 1024);
    expect_eq(dst.count(), before_count);

    // Truncated payload -> InvalidSerializedData
    let trunc_err = false;
    * = HyperLogLog.deserialize(alloc, payload[0..10]) catch |e| {
        if (e == HllError.InvalidSerializedData) {
            trunc_err = true;
        }
    };
    expect_eq(trunc_err, true);

    // Malformed magic header -> InvalidSerializedData
    let bad_magic: [21]u8 = [0; 21];
    bad_magic[0] = 'B';
    bad_magic[1] = 'A';
    bad_magic[2] = 'D';
    bad_magic[3] = '1';
    bad_magic[4] = 4;
    let magic_err = false;
    * = HyperLogLog.deserialize(alloc, &bad_magic) catch |e| {
        if (e == HllError.InvalidSerializedData) {
            magic_err = true;
        }
    };
    expect_eq(magic_err, true);
}

test("cap:hll - import cap from \"cap\"; cap.hll dynamic mounting & zero leak") {
    const alloc = std.testing.allocator;

    let hll = cap.ds.HyperLogLog.init(alloc, 12) catch unreachable;
    defer hll.deinit();

    hll.add("alpha");
    hll.add("beta");
    hll.add("gamma");
    hll.add("alpha");
    expect_eq(hll.count(), 3);
    expect_eq(hll.precision(), 12);
    expect_eq(hll.registerCount(), 4096);
}
