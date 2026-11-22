// Source: cp/cap/test/test_xorfilter_production_deep.cp
// Deep Production Verification Suite for cap:xorfilter & cap.xorfilter
// Covers:
//   1. Xor8Filter large batch (with duplicates & empty boundary): zero false negatives, FPR < 1.0%, < 11 bits/key
//   2. Xor16Filter ultra-low false positive rate (<= 2 hits over 10,000 non-members) & zero false negatives
//   3. serialize / deserialize binary roundtrip & InvalidSerializedData interception on malformed/truncated payloads
//   4. buildHashes / containsHash raw 64-bit hash stream interface
//   5. `import cap from "cap"; cap.xorfilter` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    XorFilterError,
    Xor8Filter,
    Xor16Filter,
} from "cap:db";

fn formatKey(buf: []u8, prefix: string, num: usize): []const u8 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len && pos < buf.len) : (i += 1) {
        buf[pos] = prefix[i];
        pos += 1;
    }
    if (num == 0) {
        if (pos < buf.len) {
            buf[pos] = '0';
            pos += 1;
        }
        return buf[0..pos];
    }
    let digits: [24]u8 = undefined;
    let dlen: usize = 0;
    let v = num;
    while (v > 0) {
        digits[dlen] = ('0' + (v % 10)) as u8;
        dlen += 1;
        v /= 10;
    }
    while (dlen > 0 && pos < buf.len) {
        dlen -= 1;
        buf[pos] = digits[dlen];
        pos += 1;
    }
    return buf[0..pos];
}

test("cap:xorfilter - Xor8Filter large batch (duplicates & empty), 0 FN, FPR < 1.0%, < 11 bits/key") {
    const alloc = std.testing.allocator;

    // Empty boundary
    let empty_keys: [0]string = [];
    let empty_f = Xor8Filter.build(alloc, &empty_keys) catch unreachable;
    defer empty_f.deinit();
    expect_eq(empty_f.len(), 0);
    expect_eq(empty_f.byteSize(), 0);
    expect_eq(empty_f.bitsPerKey() == 0.0, true);
    expect_eq(empty_f.contains("anything"), false);

    // 5000 unique keys + 200 duplicate keys
    const unique_count: usize = 5000;
    const dup_count: usize = 200;
    const total_count = unique_count + dup_count;

    let key_mem = alloc.alloc(u8, total_count * 32) catch unreachable;
    defer alloc.free(key_mem);
    let keys = alloc.alloc(string, total_count) catch unreachable;
    defer alloc.free(keys);

    let i: usize = 0;
    while (i < unique_count) : (i += 1) {
        const start = i * 32;
        const slice = formatKey(key_mem[start..(start + 32)], "xor8-key:", i);
        keys[i] = slice;
    }
    let d: usize = 0;
    while (d < dup_count) : (d += 1) {
        keys[unique_count + d] = keys[d * 17];
    }

    let f8 = Xor8Filter.build(alloc, keys) catch unreachable;
    defer f8.deinit();

    expect_eq(f8.len(), unique_count);
    expect_eq(f8.bitsPerKey() < 11.0, true);
    expect_eq(f8.bitsPerKey() > 8.0, true);

    // 100% Zero False Negatives
    i = 0;
    while (i < unique_count) : (i += 1) {
        expect_eq(f8.contains(keys[i]), true);
    }

    // False positive rate over 10,000 disjoint probes must be < 1.0% (< 100 hits)
    let probe_buf: [64]u8 = undefined;
    let fp_count: usize = 0;
    let j: usize = 0;
    while (j < 10000) : (j += 1) {
        const probe = formatKey(&probe_buf, "xor8-nonmember:", j + 500000);
        if (f8.contains(probe)) {
            fp_count += 1;
        }
    }
    expect_eq(fp_count < 100, true);
}

test("cap:xorfilter - Xor16Filter ultra-low FPR (<= 2 hits / 10000 probes) & zero false negatives") {
    const alloc = std.testing.allocator;
    const count: usize = 3000;

    let key_mem = alloc.alloc(u8, count * 32) catch unreachable;
    defer alloc.free(key_mem);
    let keys = alloc.alloc(string, count) catch unreachable;
    defer alloc.free(keys);

    let i: usize = 0;
    while (i < count) : (i += 1) {
        const start = i * 32;
        keys[i] = formatKey(key_mem[start..(start + 32)], "xor16-item:", i);
    }

    let f16 = Xor16Filter.build(alloc, keys) catch unreachable;
    defer f16.deinit();

    expect_eq(f16.len(), count);
    expect_eq(f16.bitsPerKey() < 22.0, true);
    expect_eq(f16.bitsPerKey() > 16.0, true);

    i = 0;
    while (i < count) : (i += 1) {
        expect_eq(f16.contains(keys[i]), true);
    }

    let probe_buf: [64]u8 = undefined;
    let fp_count: usize = 0;
    let j: usize = 0;
    while (j < 10000) : (j += 1) {
        const probe = formatKey(&probe_buf, "xor16-absent:", j + 900000);
        if (f16.contains(probe)) {
            fp_count += 1;
        }
    }
    expect_eq(fp_count <= 2, true);
}

test("cap:xorfilter - serialize / deserialize roundtrip & malformed data interception") {
    const alloc = std.testing.allocator;

    const keys: [5]string = [
        "node-a",
        "node-b",
        "node-c",
        "node-d",
        "node-e",
    ];

    // Xor8 roundtrip
    let f8 = Xor8Filter.build(alloc, &keys) catch unreachable;
    defer f8.deinit();
    const blob8 = f8.serialize(alloc) catch unreachable;
    defer alloc.free(blob8);

    let r8 = Xor8Filter.deserialize(alloc, blob8) catch unreachable;
    defer r8.deinit();
    expect_eq(r8.len(), f8.len());
    expect_eq(r8.byteSize(), f8.byteSize());
    for (k of keys) {
        expect_eq(r8.contains(k), true);
    }

    // Xor16 roundtrip
    let f16 = Xor16Filter.build(alloc, &keys) catch unreachable;
    defer f16.deinit();
    const blob16 = f16.serialize(alloc) catch unreachable;
    defer alloc.free(blob16);

    let r16 = Xor16Filter.deserialize(alloc, blob16) catch unreachable;
    defer r16.deinit();
    expect_eq(r16.len(), f16.len());
    expect_eq(r16.byteSize(), f16.byteSize());
    for (k of keys) {
        expect_eq(r16.contains(k), true);
    }

    // Empty filter roundtrip
    let empty_keys: [0]string = [];
    let empty_f = Xor8Filter.build(alloc, &empty_keys) catch unreachable;
    defer empty_f.deinit();
    const empty_blob = empty_f.serialize(alloc) catch unreachable;
    defer alloc.free(empty_blob);
    let empty_r = Xor8Filter.deserialize(alloc, empty_blob) catch unreachable;
    defer empty_r.deinit();
    expect_eq(empty_r.len(), 0);
    expect_eq(empty_r.contains("node-a"), false);

    // Truncated header
    let caught_short = false;
    let short_res = Xor8Filter.deserialize(alloc, blob8[0..16]) catch |e| blk: {
        if (e == XorFilterError.InvalidSerializedData) caught_short = true;
        break :blk null;
    };
    expect_eq(caught_short, true);
    expect_eq(short_res == null, true);

    // Truncated payload
    let caught_trunc = false;
    let trunc_res = Xor8Filter.deserialize(alloc, blob8[0..(blob8.len - 1)]) catch |e| blk: {
        if (e == XorFilterError.InvalidSerializedData) caught_trunc = true;
        break :blk null;
    };
    expect_eq(caught_trunc, true);
    expect_eq(trunc_res == null, true);

    // Bad magic (feed Xor8 blob to Xor16 deserializer)
    let caught_magic = false;
    let magic_res = Xor16Filter.deserialize(alloc, blob8) catch |e| blk: {
        if (e == XorFilterError.InvalidSerializedData) caught_magic = true;
        break :blk null;
    };
    expect_eq(caught_magic, true);
    expect_eq(magic_res == null, true);

    // Bad version
    let bad_ver = alloc.alloc(u8, blob8.len) catch unreachable;
    defer alloc.free(bad_ver);
    @memcpy(bad_ver, blob8);
    bad_ver[4] = 99;
    let caught_ver = false;
    let ver_res = Xor8Filter.deserialize(alloc, bad_ver) catch |e| blk: {
        if (e == XorFilterError.InvalidSerializedData) caught_ver = true;
        break :blk null;
    };
    expect_eq(caught_ver, true);
    expect_eq(ver_res == null, true);
}

test("cap:xorfilter - buildHashes / containsHash raw 64-bit hash stream interface") {
    const alloc = std.testing.allocator;

    // Empty hash slice
    let empty_hashes: [0]u64 = [];
    let ef = Xor16Filter.buildHashes(alloc, &empty_hashes) catch unreachable;
    defer ef.deinit();
    expect_eq(ef.len(), 0);
    expect_eq(ef.containsHash(0x123456789abcdef0), false);

    // 1000 unique hashes + 50 duplicates
    let hashes = alloc.alloc(u64, 1050) catch unreachable;
    defer alloc.free(hashes);

    let i: usize = 0;
    while (i < 1000) : (i += 1) {
        hashes[i] = ((i as u64) +% 1) *% 0x9e3779b97f4a7c15;
    }
    let d: usize = 0;
    while (d < 50) : (d += 1) {
        hashes[1000 + d] = hashes[d * 10];
    }

    let f8 = Xor8Filter.buildHashes(alloc, hashes) catch unreachable;
    defer f8.deinit();
    let f16 = Xor16Filter.buildHashes(alloc, hashes) catch unreachable;
    defer f16.deinit();

    expect_eq(f8.len(), 1000);
    expect_eq(f16.len(), 1000);

    i = 0;
    while (i < 1000) : (i += 1) {
        expect_eq(f8.containsHash(hashes[i]), true);
        expect_eq(f16.containsHash(hashes[i]), true);
    }

    let fp16: usize = 0;
    let j: usize = 0;
    while (j < 2000) : (j += 1) {
        const absent_h = ((j as u64) +% 1000000) *% 0xbf58476d1ce4e5b9;
        if (f16.containsHash(absent_h)) {
            fp16 += 1;
        }
    }
    expect_eq(fp16 <= 1, true);
}

test("cap.xorfilter - dynamic mounting via import cap from \"cap\" & zero-leak check") {
    const alloc = std.testing.allocator;
    const routes: [4]string = [
        "route:/v1/chat",
        "route:/v1/embed",
        "route:/v1/models",
        "route:/v1/health",
    ];

    let f8 = cap.db.Xor8Filter.build(alloc, &routes) catch unreachable;
    defer f8.deinit();
    expect_eq(f8.len(), 4);
    expect_eq(f8.contains("route:/v1/chat"), true);
    expect_eq(f8.contains("route:/v1/health"), true);

    const raw8 = f8.serialize(alloc) catch unreachable;
    defer alloc.free(raw8);
    let f8_copy = cap.db.Xor8Filter.deserialize(alloc, raw8) catch unreachable;
    defer f8_copy.deinit();
    expect_eq(f8_copy.contains("route:/v1/models"), true);

    let f16 = cap.db.Xor16Filter.build(alloc, &routes) catch unreachable;
    defer f16.deinit();
    expect_eq(f16.len(), 4);
    expect_eq(f16.contains("route:/v1/embed"), true);
    expect_eq(f16.contains("route:/v1/unknown"), false);
}
