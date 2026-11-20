// Source: cp/cap/test/test_lsm_sst_production_deep.cp
const std = @import("std");
const cap = @import("cap");
import { test, expect_eq } from "cap:test";
import {
    SstError,
    BlockBuilder,
    BlockReader,
    BloomFilter,
    buildBloomFilter,
    mayContain,
    SstOptions,
    SstBuilder,
    SstReader,
    SstMergeIter,
} from "cap:lsm_sst";

test("lsm_sst: BlockBuilder/BlockReader prefix compression, restart seek and sequential scan") {
    const alloc = std.testing.allocator;
    let builder = BlockBuilder.init(alloc, 4);
    defer builder.deinit();

    let key_buf: [32]u8 = undefined;
    let val_buf: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 50) {
        const k = try std.fmt.bufPrint(&key_buf, "user:profile:{d:0>4}", .{i * 2});
        const v = try std.fmt.bufPrint(&val_buf, "val_{d:0>4}", .{i * 2});
        try builder.add(k, v);
        i += 1;
    }

    const raw = try builder.finish();
    const reader = try BlockReader.init(raw);
    expect_eq(reader.num_restarts, @as(u32, 13));

    let iter = reader.iter(alloc);
    defer iter.deinit();

    // Seek to exact key
    const found1 = try iter.seek("user:profile:0042");
    try std.testing.expect(found1);
    try std.testing.expect(std.mem.eql(u8, iter.key(), "user:profile:0042"));
    try std.testing.expect(std.mem.eql(u8, iter.value(), "val_0042"));

    // Seek to non-existent key -> lands on next greater key
    const found2 = try iter.seek("user:profile:0043");
    try std.testing.expect(found2);
    try std.testing.expect(std.mem.eql(u8, iter.key(), "user:profile:0044"));

    // Seek past end -> invalid
    const found3 = try iter.seek("user:profile:9999");
    try std.testing.expect(!found3);
    try std.testing.expect(!iter.isValid());

    // Full sequential traversal
    try iter.seekToFirst();
    let count: usize = 0;
    while (iter.isValid()) {
        const expected_k = try std.fmt.bufPrint(&key_buf, "user:profile:{d:0>4}", .{count * 2});
        try std.testing.expect(std.mem.eql(u8, iter.key(), expected_k));
        count += 1;
        * = try iter.next();
    }
    expect_eq(count, @as(usize, 50));
}

test("lsm_sst: BloomFilter zero false negatives and low false positive rate") {
    const alloc = std.testing.allocator;
    let keys_storage = std.ArrayList([]u8).empty;
    defer {
        for (k of keys_storage.items) {
            alloc.free(k);
        }
        keys_storage.deinit(alloc);
    }

    let key_buf: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 500) {
        const s = try std.fmt.bufPrint(&key_buf, "present_key_{d:0>5}", .{i});
        const dup = try alloc.dupe(u8, s);
        try keys_storage.append(alloc, dup);
        i += 1;
    }

    const filter = try BloomFilter.buildFromMutKeys(alloc, keys_storage.items, 10);
    defer alloc.free(filter);

    // Zero false negatives
    for (k of keys_storage.items) {
        try std.testing.expect(mayContain(filter, k));
    }

    // Low false positive rate (< 3% for 10 bits/key)
    let fp: usize = 0;
    let j: usize = 0;
    while (j < 1000) {
        const absent = try std.fmt.bufPrint(&key_buf, "absent_key_{d:0>5}", .{j});
        if (mayContain(filter, absent)) {
            fp += 1;
        }
        j += 1;
    }
    try std.testing.expect(fp < 30);

    // Also verify top-level buildBloomFilter
    let static_keys: [3][]const u8 = .{ "alpha", "beta", "gamma" };
    const f2 = try buildBloomFilter(alloc, &static_keys, 10);
    defer alloc.free(f2);
    try std.testing.expect(mayContain(f2, "alpha"));
    try std.testing.expect(mayContain(f2, "gamma"));
}

test("lsm_sst: SstBuilder -> SstReader multi-block get, rangeScan and UnsortedKeys guard") {
    const alloc = std.testing.allocator;
    let builder = SstBuilder.init(alloc, SstOptions{
        .block_size = 256,
        .restart_interval = 4,
        .bloom_bits_per_key = 10,
    });
    defer builder.deinit();

    let key_buf: [32]u8 = undefined;
    let val_buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 120) {
        const k = try std.fmt.bufPrint(&key_buf, "k_{d:0>4}", .{i});
        const v = try std.fmt.bufPrint(&val_buf, "payload_value_{d:0>4}_padding", .{i});
        try builder.put(k, v);
        i += 1;
    }

    // Unsorted key must be rejected
    try std.testing.expectError(SstError.UnsortedKeys, builder.put("k_0050", "dup"));
    try std.testing.expectError(SstError.UnsortedKeys, builder.put("k_0010", "older"));

    const sst_bytes = try builder.finish();
    defer alloc.free(sst_bytes);

    let reader = try SstReader.init(alloc, sst_bytes);
    defer reader.deinit();
    expect_eq(reader.entry_count, @as(u64, 120));

    // Point lookups across multiple data blocks
    const g0 = (try reader.get("k_0000")).?;
    try std.testing.expect(!g0.is_tombstone);
    try std.testing.expect(std.mem.eql(u8, g0.value, "payload_value_0000_padding"));

    const g77 = (try reader.get("k_0077")).?;
    try std.testing.expect(std.mem.eql(u8, g77.value, "payload_value_0077_padding"));

    const g_miss = try reader.get("k_9999");
    try std.testing.expect(g_miss == null);

    // Range scan [k_0025, k_0035)
    const scanned = try reader.rangeScan(alloc, "k_0025", "k_0035");
    defer {
        for (item of scanned) {
            alloc.free(item.key);
            alloc.free(item.value);
        }
        alloc.free(scanned);
    }
    expect_eq(scanned.len, @as(usize, 10));
    try std.testing.expect(std.mem.eql(u8, scanned[0].key, "k_0025"));
    try std.testing.expect(std.mem.eql(u8, scanned[9].key, "k_0034"));
}

test("lsm_sst: SstMergeIter multi-level override, tombstone deletion and corruption detection") {
    const alloc = std.testing.allocator;

    // Older SSTable (L1)
    let b_old = SstBuilder.init(alloc, SstOptions{ .block_size = 128 });
    defer b_old.deinit();
    try b_old.put("a", "old_a");
    try b_old.put("b", "old_b");
    try b_old.put("c", "old_c");
    try b_old.put("d", "old_d");
    const sst_old = try b_old.finish();
    defer alloc.free(sst_old);

    // Newer SSTable (L0) overrides "b", deletes "c", adds "e"
    let b_new = SstBuilder.init(alloc, SstOptions{ .block_size = 128 });
    defer b_new.deinit();
    try b_new.put("b", "new_b");
    try b_new.delete("c");
    try b_new.put("e", "new_e");
    const sst_new = try b_new.finish();
    defer alloc.free(sst_new);

    let r_new = try SstReader.init(alloc, sst_new);
    defer r_new.deinit();
    let r_old = try SstReader.init(alloc, sst_old);
    defer r_old.deinit();

    let readers: [2]*const SstReader = .{ &r_new, &r_old };
    let merge = try SstMergeIter.init(alloc, &readers);
    defer merge.deinit();

    try std.testing.expect(try merge.seekToFirst());
    try std.testing.expect(std.mem.eql(u8, merge.key(), "a"));
    try std.testing.expect(std.mem.eql(u8, merge.value(), "old_a"));

    try std.testing.expect(try merge.next());
    try std.testing.expect(std.mem.eql(u8, merge.key(), "b"));
    try std.testing.expect(std.mem.eql(u8, merge.value(), "new_b"));

    // "c" is deleted by tombstone -> skipped!
    try std.testing.expect(try merge.next());
    try std.testing.expect(std.mem.eql(u8, merge.key(), "d"));
    try std.testing.expect(std.mem.eql(u8, merge.value(), "old_d"));

    try std.testing.expect(try merge.next());
    try std.testing.expect(std.mem.eql(u8, merge.key(), "e"));
    try std.testing.expect(std.mem.eql(u8, merge.value(), "new_e"));

    try std.testing.expect(!try merge.next());

    // Tamper CRC32C in data block (byte 0) -> CorruptedChecksum
    let corrupted_crc = try alloc.dupe(u8, sst_new);
    defer alloc.free(corrupted_crc);
    corrupted_crc[0] ^= 0xFF;
    let r_bad_crc = try SstReader.init(alloc, corrupted_crc);
    defer r_bad_crc.deinit();
    try std.testing.expectError(SstError.CorruptedChecksum, r_bad_crc.get("b"));

    // Tamper Footer magic -> InvalidFooter
    let corrupted_magic = try alloc.dupe(u8, sst_new);
    defer alloc.free(corrupted_magic);
    corrupted_magic[corrupted_magic.len - 1] ^= 0x01;
    try std.testing.expectError(SstError.InvalidFooter, SstReader.init(alloc, corrupted_magic));
}

test("lsm_sst: cap.lsm_sst dynamic mount and zero memory leaks") {
    const alloc = std.testing.allocator;
    let builder = cap.lsm_sst.SstBuilder.init(alloc, cap.lsm_sst.SstOptions{});
    defer builder.deinit();

    try builder.put("mount_key", "mount_val");
    const bytes = try builder.finish();
    defer alloc.free(bytes);

    let reader = try cap.lsm_sst.SstReader.init(alloc, bytes);
    defer reader.deinit();

    const got = (try reader.get("mount_key")).?;
    try std.testing.expect(std.mem.eql(u8, got.value, "mount_val"));
}
