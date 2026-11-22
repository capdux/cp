// Source: cp/cap/test/test_cuckoo_production_deep.cp
// Deep Production Verification Suite for cap:cuckoo & cap.cuckoo
// Covers:
//   1. Zero False Negatives (2000 items 100% hit) & ultra-low False Positive Rate (< 0.5% over 10000 probes)
//   2. Native O(1) delete(): deleted keys absent while remaining keys stay 100% present, plus insertUnique()
//   3. Bounded eviction & rollback-safe FilterFull on small filter, plus recovery after delete()
//   4. stats(), loadFactor(), clear(), InvalidCapacity & std.testing.allocator zero-leak check
//   5. `import cap from "cap"; cap.cuckoo` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CuckooError,
    CuckooStats,
    CuckooFilter,
} from "cap:ds";

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

test("cap:cuckoo - zero false negatives (2000 items) & false positive rate < 0.5% (10000 probes)") {
    const alloc = std.heap.page_allocator;
    let cf = CuckooFilter.init(alloc, 4096) catch unreachable;
    defer cf.deinit();

    let key_buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 2000) : (i += 1) {
        const key = formatKey(&key_buf, "member:", i);
        cf.insert(key) catch unreachable;
    }
    expect_eq(cf.len(), 2000);

    // 100% Zero False Negatives
    i = 0;
    while (i < 2000) : (i += 1) {
        const key = formatKey(&key_buf, "member:", i);
        expect_eq(cf.contains(key), true);
    }

    // False positive rate over 10,000 disjoint keys must be < 0.5% (< 50 hits)
    let fp_count: usize = 0;
    let j: usize = 0;
    while (j < 10000) : (j += 1) {
        const key = formatKey(&key_buf, "nonmember:", j + 100000);
        if (cf.contains(key)) {
            fp_count += 1;
        }
    }
    expect_eq(fp_count < 50, true);
}

test("cap:cuckoo - native delete & insertUnique reliability under load") {
    const alloc = std.heap.page_allocator;
    let cf = CuckooFilter.init(alloc, 2048) catch unreachable;
    defer cf.deinit();

    let key_buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 1200) : (i += 1) {
        const key = formatKey(&key_buf, "item:", i);
        const inserted = cf.insertUnique(key) catch unreachable;
        expect_eq(inserted, true);
    }
    expect_eq(cf.len(), 1200);

    // Duplicate insertUnique must skip and return false
    const dup_key = formatKey(&key_buf, "item:", 42);
    const dup_res = cf.insertUnique(dup_key) catch unreachable;
    expect_eq(dup_res, false);
    expect_eq(cf.len(), 1200);

    // Delete all even items [0, 2, 4, ..., 1198]
    i = 0;
    while (i < 1200) : (i += 2) {
        const key = formatKey(&key_buf, "item:", i);
        expect_eq(cf.delete(key), true);
    }
    expect_eq(cf.len(), 600);

    // Deleted even items must not be present; odd items must remain 100% present
    i = 0;
    while (i < 1200) : (i += 1) {
        const key = formatKey(&key_buf, "item:", i);
        if ((i & 1) == 0) {
            expect_eq(cf.contains(key), false);
        } else {
            expect_eq(cf.contains(key), true);
        }
    }

    // Deleting non-existent item returns false and does not alter len
    expect_eq(cf.delete("never-inserted-key"), false);
    expect_eq(cf.len(), 600);
}

test("cap:cuckoo - eviction relocation, rollback-safe FilterFull & recovery after delete") {
    const alloc = std.heap.page_allocator;
    // 16 slots = 4 buckets * 4 slots
    let cf = CuckooFilter.init(alloc, 16) catch unreachable;
    defer cf.deinit();

    let key_buf: [64]u8 = undefined;
    let inserted_count: usize = 0;
    let hit_full = false;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        const key = formatKey(&key_buf, "small:", i);
        let ok = true;
        cf.insert(key) catch |e| {
            if (e == CuckooError.FilterFull) {
                hit_full = true;
                ok = false;
            } else {
                unreachable;
            }
        };
        if (!ok) break;
        inserted_count += 1;
    }

    expect_eq(hit_full, true);
    expect_eq(cf.len(), inserted_count);
    expect_eq(inserted_count > 0, true);
    expect_eq(inserted_count <= 16, true);

    // Rollback integrity: all previously inserted items must still be 100% present
    let k: usize = 0;
    while (k < inserted_count) : (k += 1) {
        const key = formatKey(&key_buf, "small:", k);
        expect_eq(cf.contains(key), true);
    }

    // Delete two items to free slots and verify insert succeeds again
    const del0 = formatKey(&key_buf, "small:", 0);
    const del1 = formatKey(&key_buf, "small:", 1);
    expect_eq(cf.delete(del0), true);
    expect_eq(cf.delete(del1), true);
    expect_eq(cf.len(), inserted_count - 2);

    cf.insert("recovered-key-alpha") catch unreachable;
    expect_eq(cf.contains("recovered-key-alpha"), true);
    expect_eq(cf.len(), inserted_count - 1);
}

test("cap:cuckoo - stats, loadFactor, clear, InvalidCapacity & std.testing.allocator leak check") {
    const t_alloc = std.testing.allocator;

    // Zero capacity must return InvalidCapacity
    let invalid_caught = false;
    let bad = CuckooFilter.init(t_alloc, 0) catch |e| blk: {
        if (e == CuckooError.InvalidCapacity) {
            invalid_caught = true;
        }
        break :blk null;
    };
    expect_eq(invalid_caught, true);
    expect_eq(bad == null, true);

    let cf = CuckooFilter.init(t_alloc, 24) catch unreachable;
    defer cf.deinit();

    // 24 slots -> min 6 buckets -> power-of-two = 8 buckets -> 32 slots
    const st0: CuckooStats = cf.stats();
    expect_eq(st0.bucket_count, 8);
    expect_eq(st0.slot_count, 32);
    expect_eq(st0.item_count, 0);
    expect_eq(st0.load_factor == 0.0, true);
    expect_eq(st0.total_kicks, 0);

    cf.insert("alpha") catch unreachable;
    cf.insert("beta") catch unreachable;
    cf.insert("gamma") catch unreachable;
    cf.insert("delta") catch unreachable;

    const st1 = cf.stats();
    expect_eq(st1.item_count, 4);
    // 4 / 32 = 0.125
    expect_eq(st1.load_factor > 0.12 && st1.load_factor < 0.13, true);
    expect_eq(cf.loadFactor() == st1.load_factor, true);

    cf.clear();
    expect_eq(cf.len(), 0);
    expect_eq(cf.loadFactor() == 0.0, true);
    expect_eq(cf.contains("alpha"), false);

    cf.insert("reused") catch unreachable;
    expect_eq(cf.contains("reused"), true);
}

test("cap.cuckoo - dynamic mounting via import cap from \"cap\"") {
    const alloc = std.testing.allocator;
    let cf = cap.ds.CuckooFilter.init(alloc, 32) catch unreachable;
    defer cf.deinit();

    cf.insert("route:/api/v1/chat") catch unreachable;
    cf.insert("route:/api/v1/embed") catch unreachable;

    expect_eq(cf.contains("route:/api/v1/chat"), true);
    expect_eq(cf.contains("route:/api/v1/embed"), true);
    expect_eq(cf.contains("route:/api/v1/unknown"), false);
    expect_eq(cf.delete("route:/api/v1/chat"), true);
    expect_eq(cf.contains("route:/api/v1/chat"), false);
    expect_eq(cf.len(), 1);
}
