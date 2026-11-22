// Source: native-cp-stdlib (Bloom, Counting Bloom & Cuckoo Filters)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { BloomFilter, CountingBloomFilter, CuckooFilter, FilterError } from "cap:db";

test "BloomFilter optimal bit/hash params, 5000 keys zero FN and FP < 2%" {
    const alloc = std.testing.allocator;
    let bf = try BloomFilter.initWithRate(alloc, 5000, 0.01);
    defer bf.deinit(alloc);

    expect_eq(bf.bitCount() >= 47000, true);
    expect_eq(bf.hashCount() >= 6, true);

    let buf: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 5000) {
        const key = std.fmt.bufPrint(&buf, "ins-{d}", .{i}) catch unreachable;
        bf.insert(key);
        i = i + 1;
    }

    // Zero false negatives
    i = 0;
    while (i < 5000) {
        const key = std.fmt.bufPrint(&buf, "ins-{d}", .{i}) catch unreachable;
        expect_eq(bf.contains(key), true);
        i = i + 1;
    }

    // False positive rate < 2% over 5000 unseen keys
    let fp: usize = 0;
    i = 0;
    while (i < 5000) {
        const key = std.fmt.bufPrint(&buf, "probe-{d}", .{i}) catch unreachable;
        if (bf.contains(key)) {
            fp = fp + 1;
        }
        i = i + 1;
    }
    expect_eq(fp < 100, true);
}

test "CountingBloomFilter insert/remove without deleting other keys and saturation" {
    const alloc = std.testing.allocator;
    let cbf = try CountingBloomFilter.initWithRate(alloc, 1024, 0.01);
    defer cbf.deinit(alloc);

    cbf.insert("alpha");
    cbf.insert("beta");
    cbf.insert("gamma");
    expect_eq(cbf.contains("alpha"), true);
    expect_eq(cbf.contains("beta"), true);
    expect_eq(cbf.contains("gamma"), true);

    expect_eq(cbf.remove("beta"), true);
    expect_eq(cbf.contains("beta"), false);
    expect_eq(cbf.contains("alpha"), true);
    expect_eq(cbf.contains("gamma"), true);
    expect_eq(cbf.remove("nonexistent"), false);

    // 8-bit saturation: 260 inserts must not wrap counter to 0
    let j: usize = 0;
    while (j < 260) {
        cbf.insert("hot-key");
        j = j + 1;
    }
    expect_eq(cbf.contains("hot-key"), true);

    cbf.clear();
    expect_eq(cbf.contains("alpha"), false);
    expect_eq(cbf.contains("hot-key"), false);
}

test "CuckooFilter insert, contains, remove, and FilterFull on overload" {
    const alloc = std.testing.allocator;
    let cf = try CuckooFilter.init(alloc, 32);
    defer cf.deinit(alloc);

    try cf.insert("k1");
    try cf.insert("k2");
    try cf.insert("k3");
    expect_eq(cf.count(), 3);
    expect_eq(cf.contains("k1"), true);
    expect_eq(cf.contains("k2"), true);
    expect_eq(cf.contains("missing"), false);

    expect_eq(cf.remove("k2"), true);
    expect_eq(cf.count(), 2);
    expect_eq(cf.contains("k2"), false);
    expect_eq(cf.remove("k2"), false);

    // Fill until FilterFull
    let saw_full = false;
    let buf: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        const key = std.fmt.bufPrint(&buf, "cuckoo-{d}", .{i}) catch unreachable;
        let failed = false;
        cf.insert(key) catch |e| {
            expect_eq(e == FilterError.FilterFull, true);
            failed = true;
        };
        if (failed) {
            saw_full = true;
            break;
        }
        i = i + 1;
    }
    expect_eq(saw_full, true);
}

test "BloomFilter merge union, incompatible error, and clear" {
    const alloc = std.testing.allocator;
    let a = try BloomFilter.initWithRate(alloc, 1000, 0.01);
    defer a.deinit(alloc);
    let b = try BloomFilter.initWithRate(alloc, 1000, 0.01);
    defer b.deinit(alloc);
    let c = try BloomFilter.initWithRate(alloc, 5000, 0.01);
    defer c.deinit(alloc);

    a.insert("left-only");
    b.insert("right-only");
    try a.merge(b);

    expect_eq(a.contains("left-only"), true);
    expect_eq(a.contains("right-only"), true);

    let got_incompat = false;
    a.merge(c) catch |e| {
        expect_eq(e == FilterError.IncompatibleFilters, true);
        got_incompat = true;
    };
    expect_eq(got_incompat, true);

    a.clear();
    expect_eq(a.contains("left-only"), false);
    expect_eq(a.contains("right-only"), false);
}

test "cap.bloom_cuckoo dynamic mount and error validation" {
    const alloc = std.testing.allocator;
    const mod = cap.db;

    let invalid_cap = false;
    if (f := mod.BloomFilter.initWithRate(alloc, 0, 0.01)) {
        f.deinit(alloc);
    } else {
        invalid_cap = true;
    }
    expect_eq(invalid_cap, true);

    let invalid_fp = false;
    if (f := mod.BloomFilter.initWithRate(alloc, 100, 1.5)) {
        f.deinit(alloc);
    } else {
        invalid_fp = true;
    }
    expect_eq(invalid_fp, true);

    let cf = try mod.CuckooFilter.init(alloc, 64);
    defer cf.deinit(alloc);
    try cf.insert("dyn-mounted");
    expect_eq(cf.contains("dyn-mounted"), true);
}
