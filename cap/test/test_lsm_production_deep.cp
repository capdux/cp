// Source: cp/cap/test/test_lsm_production_deep.cp
// Deep Production Verification Suite for cap:lsm & cap.lsm
// Covers:
//   1. put/delete/overwrite across multiple automatic flush() cycles -> latest seq wins & Tombstone blocks stale reads
//   2. rangeScan multi-way merge across MemTable + multiple SSTable layers (excludes deleted, preserves updated)
//   3. compact() multi-way compaction (deduplicates overwrites, drops bottom-level tombstones, shrinks sst_count)
//   4. SsTable corruption/tampering -> CorruptSst interception + std.testing.allocator zero memory leaks
//   5. `import cap from "cap"; cap.lsm` dynamic mounting verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    LsmError,
    LsmEntry,
    LsmStats,
    SstRecord,
    SstLookup,
    SsTable,
    LsmEngine,
} from "cap:db";

test("cap:lsm - put/delete/overwrite across auto-flushes, latest seq wins & tombstone isolation") {
    const alloc = std.testing.allocator;
    // Small threshold (180 bytes) forces frequent automatic flush() to L0 SSTables
    let engine = LsmEngine.init(alloc, 180);
    defer engine.deinit();

    let key_buf: [16]u8 = undefined;
    let val_buf: [24]u8 = undefined;

    // Phase 1: Insert 60 keys across multiple auto-flushes
    for (i of 0..60) {
        const k = std.fmt.bufPrint(key_buf[0..], "k:{d:0>3}", .{i}) catch unreachable;
        const v = std.fmt.bufPrint(val_buf[0..], "v1:{d:0>3}", .{i}) catch unreachable;
        engine.put(k, v) catch unreachable;
    }

    const st1 = engine.stats();
    expect_eq(st1.sst_count >= 5, true);

    // Phase 2: Overwrite even keys with v2, delete keys divisible by 3
    for (i of 0..60) {
        const k = std.fmt.bufPrint(key_buf[0..], "k:{d:0>3}", .{i}) catch unreachable;
        if (i % 3 == 0) {
            engine.delete(k) catch unreachable;
        } else if (i % 2 == 0) {
            const v = std.fmt.bufPrint(val_buf[0..], "v2:{d:0>3}", .{i}) catch unreachable;
            engine.put(k, v) catch unreachable;
        }
    }

    // Re-insert k:000 after deletion to verify resurrection with higher seq
    engine.put("k:000", "resurrected") catch unreachable;

    // Verify every key
    expect_eq(std.mem.eql(u8, engine.get("k:000").?, "resurrected"), true);
    for (i of 1..60) {
        const k = std.fmt.bufPrint(key_buf[0..], "k:{d:0>3}", .{i}) catch unreachable;
        const got = engine.get(k);
        if (i % 3 == 0) {
            // Tombstone must NOT fall through to older v1 in L0 SSTables
            expect_eq(got == null, true);
        } else if (i % 2 == 0) {
            const expected = std.fmt.bufPrint(val_buf[0..], "v2:{d:0>3}", .{i}) catch unreachable;
            expect_eq(got != null, true);
            expect_eq(std.mem.eql(u8, got.?, expected), true);
        } else {
            const expected = std.fmt.bufPrint(val_buf[0..], "v1:{d:0>3}", .{i}) catch unreachable;
            expect_eq(got != null, true);
            expect_eq(std.mem.eql(u8, got.?, expected), true);
        }
    }
    expect_eq(engine.get("k:999") == null, true);
}

test("cap:lsm - rangeScan multi-way merge across MemTable and multiple SSTable layers") {
    const alloc = std.testing.allocator;
    let engine = LsmEngine.init(alloc, 160);
    defer engine.deinit();

    // Layer 1 (flushed to L0)
    engine.put("a:01", "old_01") catch unreachable;
    engine.put("a:02", "old_02") catch unreachable;
    engine.put("a:03", "old_03") catch unreachable;
    engine.put("a:04", "old_04") catch unreachable;
    engine.put("a:05", "old_05") catch unreachable;
    engine.flush() catch unreachable;

    // Layer 2 (flushed to L0): update a:02, delete a:03, add a:06
    engine.put("a:02", "mid_02") catch unreachable;
    engine.delete("a:03") catch unreachable;
    engine.put("a:06", "mid_06") catch unreachable;
    engine.flush() catch unreachable;

    // Layer 3 (active MemTable): update a:04, delete a:05, add a:07
    engine.put("a:04", "mem_04") catch unreachable;
    engine.delete("a:05") catch unreachable;
    engine.put("a:07", "mem_07") catch unreachable;

    // Range [a:02, a:06] -> expects a:02(mid_02), a:04(mem_04), a:06(mid_06)
    const res = engine.rangeScan(alloc, "a:02", "a:06") catch unreachable;
    defer alloc.free(res);

    expect_eq(res.len, 3);
    expect_eq(std.mem.eql(u8, res[0].key, "a:02"), true);
    expect_eq(std.mem.eql(u8, res[0].value, "mid_02"), true);
    expect_eq(std.mem.eql(u8, res[1].key, "a:04"), true);
    expect_eq(std.mem.eql(u8, res[1].value, "mem_04"), true);
    expect_eq(std.mem.eql(u8, res[2].key, "a:06"), true);
    expect_eq(std.mem.eql(u8, res[2].value, "mid_06"), true);

    // InvalidRange check
    const bad = engine.rangeScan(alloc, "z:00", "a:00");
    expect_eq(bad == LsmError.InvalidRange, true);
}

test("cap:lsm - compact() multi-way compaction drops overwrites & tombstones, shrinks sst_count") {
    const alloc = std.testing.allocator;
    let engine = LsmEngine.init(alloc, 140);
    defer engine.deinit();

    let key_buf: [16]u8 = undefined;
    let val_buf: [24]u8 = undefined;

    // Write 40 keys, then overwrite 20 of them, then delete 15 of them
    for (i of 0..40) {
        const k = std.fmt.bufPrint(key_buf[0..], "c:{d:0>3}", .{i}) catch unreachable;
        const v = std.fmt.bufPrint(val_buf[0..], "init:{d:0>3}", .{i}) catch unreachable;
        engine.put(k, v) catch unreachable;
    }
    for (i of 0..20) {
        const k = std.fmt.bufPrint(key_buf[0..], "c:{d:0>3}", .{i}) catch unreachable;
        const v = std.fmt.bufPrint(val_buf[0..], "upd:{d:0>3}", .{i}) catch unreachable;
        engine.put(k, v) catch unreachable;
    }
    for (i of 25..40) {
        const k = std.fmt.bufPrint(key_buf[0..], "c:{d:0>3}", .{i}) catch unreachable;
        engine.delete(k) catch unreachable;
    }
    engine.flush() catch unreachable;

    const before = engine.stats();
    expect_eq(before.sst_count >= 2, true);
    expect_eq(before.total_sst_entries > 25, true);

    engine.compact() catch unreachable;

    const after = engine.stats();
    expect_eq(after.sst_count, 1);
    expect_eq(after.total_sst_entries, 25);
    expect_eq(after.compactions_run, 1);

    // Verify 100% data consistency after compaction
    for (i of 0..40) {
        const k = std.fmt.bufPrint(key_buf[0..], "c:{d:0>3}", .{i}) catch unreachable;
        const got = engine.get(k);
        if (i >= 25) {
            expect_eq(got == null, true);
        } else if (i < 20) {
            const expected = std.fmt.bufPrint(val_buf[0..], "upd:{d:0>3}", .{i}) catch unreachable;
            expect_eq(std.mem.eql(u8, got.?, expected), true);
        } else {
            const expected = std.fmt.bufPrint(val_buf[0..], "init:{d:0>3}", .{i}) catch unreachable;
            expect_eq(std.mem.eql(u8, got.?, expected), true);
        }
    }

    const full_scan = engine.rangeScan(alloc, "c:000", "c:999") catch unreachable;
    defer alloc.free(full_scan);
    expect_eq(full_scan.len, 25);
}

test("cap:lsm - SsTable sparse index lookup, CorruptSst tampering guard & zero leaks") {
    const alloc = std.testing.allocator;

    // Build an SSTable with 40 entries (> SPARSE_STRIDE=16) to exercise multi-block sparse index
    let recs: [40]SstRecord = undefined;
    let keys: [40][10]u8 = undefined;
    let vals: [40][10]u8 = undefined;
    for (i of 0..40) {
        const k = std.fmt.bufPrint(keys[i][0..], "s:{d:0>3}", .{i}) catch unreachable;
        const v = std.fmt.bufPrint(vals[i][0..], "val:{d:0>3}", .{i}) catch unreachable;
        recs[i] = SstRecord{
            key: k,
            value: v,
            seq: u64(i + 1),
            is_tombstone: (i == 20),
        };
    }

    let sst = SsTable.encodeFromEntries(alloc, recs[0..], 0, 1) catch unreachable;
    defer sst.deinit();

    expect_eq(sst.len(), 40);
    expect_eq(std.mem.eql(u8, sst.minKey(), "s:000"), true);
    expect_eq(std.mem.eql(u8, sst.maxKey(), "s:039"), true);

    // Lookup Value, Tombstone, NotFound (both out-of-range and in-range gap)
    switch (sst.get("s:025")) {
        .Value v => expect_eq(std.mem.eql(u8, v, "val:025"), true),
        else => expect_eq(false, true),
    }
    switch (sst.get("s:020")) {
        .Tombstone => expect_eq(true, true),
        else => expect_eq(false, true),
    }
    switch (sst.get("s:025z")) {
        .NotFound => expect_eq(true, true),
        else => expect_eq(false, true),
    }
    switch (sst.get("a:000")) {
        .NotFound => expect_eq(true, true),
        else => expect_eq(false, true),
    }

    // Tamper 1 byte in payload -> decodeAndVerify must reject with CorruptSst
    let tampered = alloc.dupe(u8, sst.rawBytes()) catch unreachable;
    defer alloc.free(tampered);
    tampered[10] ^= 0xFF;
    const err_res = SsTable.decodeAndVerify(alloc, tampered);
    expect_eq(err_res == LsmError.CorruptSst, true);

    // Tamper magic footer -> CorruptSst
    let bad_magic = alloc.dupe(u8, sst.rawBytes()) catch unreachable;
    defer alloc.free(bad_magic);
    bad_magic[bad_magic.len - 1] = 'X';
    const magic_res = SsTable.decodeAndVerify(alloc, bad_magic);
    expect_eq(magic_res == LsmError.CorruptSst, true);

    // Truncated buffer -> CorruptSst
    const trunc_res = SsTable.decodeAndVerify(alloc, sst.rawBytes()[0..12]);
    expect_eq(trunc_res == LsmError.CorruptSst, true);
}

test("cap:lsm - dynamic mount via import cap from \"cap\"; cap.lsm") {
    const alloc = std.testing.allocator;
    let engine = cap.db.LsmEngine.init(alloc, 256);
    defer engine.deinit();

    engine.put("dyn:1", "alpha") catch unreachable;
    engine.put("dyn:2", "beta") catch unreachable;
    engine.delete("dyn:1") catch unreachable;
    engine.flush() catch unreachable;
    engine.compact() catch unreachable;

    expect_eq(engine.get("dyn:1") == null, true);
    expect_eq(std.mem.eql(u8, engine.get("dyn:2").?, "beta"), true);
    expect_eq(engine.stats().sst_count, 1);
}
