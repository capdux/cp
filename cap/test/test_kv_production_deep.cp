// Source: cp/cap/test/test_kv_production_deep.cp
// Deep Production Test Suite for cap:kv & cap.kv
// Covers:
//   1. WalCodec CRC32 encode/decode, single-byte tamper detection & TornTail vs CorruptRecord
//   2. KvStore real disk file put/get/delete/prefixScan/keys/stats & close/reopen persistence
//   3. Crash simulation: torn tail bytes appended -> open auto-ftruncate recovery vs strict rejection
//   4. Atomic compact() reclaiming dead records from overwrites/deletes & preserving data on reopen
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.kv`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    WAL_MAGIC,
    WAL_HEADER_SIZE,
    KvError,
    WalOp,
    WalRecord,
    WalDecodeResult,
    WalCodec,
    KvStoreOptions,
    KvStats,
    KvStore,
} from "cap:kv";
import cap from "cap";

let test_seq: std.atomic.Value(u64) = std.atomic.Value(u64).init(1);

fn makeTmpDirPath(buf: []u8): string {
    const seq = test_seq.fetchAdd(1, .monotonic);
    let ts: std.os.linux.timespec = undefined;
    * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
    const pid = std.os.linux.getpid();
    const mix = (@as(u64, @intCast(ts.nsec)) ^ (@as(u64, @intCast(pid)) << 16)) +% seq;
    return std.fmt.bufPrint(buf, "/tmp/cp_kv_test_{x}_{d}", .{ mix, seq }) catch unreachable;
}

fn toPathZ(path: string, buf: []u8): [*:0]const u8 {
    @memcpy(buf[0..path.len], path);
    buf[path.len] = 0;
    return buf[0..path.len :0].ptr;
}

fn createDir(path: string) {
    let buf: [4096]u8 = undefined;
    const z = toPathZ(path, &buf);
    * = std.os.linux.mkdir(z, 0o755);
}

fn removeFile(path: string) {
    let buf: [4096]u8 = undefined;
    const z = toPathZ(path, &buf);
    * = std.c.unlink(z);
}

fn removeDir(path: string) {
    let buf: [4096]u8 = undefined;
    const z = toPathZ(path, &buf);
    * = std.os.linux.rmdir(z);
}

fn fileExists(path: string): bool {
    let buf: [4096]u8 = undefined;
    const z = toPathZ(path, &buf);
    return std.c.access(z, 0) == 0;
}

fn fileSizeBytes(path: string): u64 {
    let buf: [4096]u8 = undefined;
    const z = toPathZ(path, &buf);
    let stx: std.os.linux.Statx = undefined;
    const rc = std.c.statx(std.os.linux.AT.FDCWD, z, 0, std.os.linux.STATX{ .SIZE = true }, &stx);
    if (rc != 0) return 0;
    return stx.size;
}

fn appendRawBytes(path: string, data: []const u8): ! {
    let buf: [4096]u8 = undefined;
    const z = toPathZ(path, &buf);
    let z_flags = std.c.O{ .APPEND = true, .CLOEXEC = true };
    z_flags.ACCMODE = .WRONLY;
    const fd = std.c.open(z, z_flags, @as(std.c.mode_t, 0));
    expect_eq(fd >= 0, true);
    defer * = std.c.close(fd);
    let written: usize = 0;
    while (written < data.len) {
        const n = std.c.write(fd, data[written..data.len].ptr, data.len - written);
        expect_eq(n > 0, true);
        written += (n as usize);
    }
}

fn flipFileByte(path: string, offset: usize): ! {
    let buf: [4096]u8 = undefined;
    const z = toPathZ(path, &buf);
    let z_flags = std.c.O{ .CLOEXEC = true };
    z_flags.ACCMODE = .RDWR;
    const fd = std.c.open(z, z_flags, @as(std.c.mode_t, 0));
    expect_eq(fd >= 0, true);
    defer * = std.c.close(fd);

    * = std.c.lseek(fd, @intCast(offset), 0);
    let b: [1]u8 = [0];
    const r = std.c.read(fd, &b, 1);
    expect_eq(r == 1, true);
    b[0] = b[0] ^ 0x55;
    * = std.c.lseek(fd, @intCast(offset), 0);
    const w = std.c.write(fd, &b, 1);
    expect_eq(w == 1, true);
}

fn testWalCodecCrc32TamperAndTornTail()! {
    const alloc = std.heap.page_allocator;

    // 1. Empty key rejected with InvalidKey
    let invalid_key_hit = false;
    * = WalCodec.encode(alloc, WalRecord.put("", "v")) catch |e| {
        if (e == @as(anyerror, KvError.InvalidKey)) invalid_key_hit = true;
    };
    expect_eq(invalid_key_hit, true);

    // 2. Encode Put & Delete records and verify exact frame layout
    const f1 = WalCodec.encode(alloc, WalRecord.put("agent:1:goal", "persist-state"))?;
    defer alloc.free(f1);
    const f2 = WalCodec.encode(alloc, WalRecord.del("agent:1:tmp"))?;
    defer alloc.free(f2);

    expect_eq(f1.len, WAL_HEADER_SIZE + "agent:1:goal".len + "persist-state".len);
    expect_eq(f2.len, WAL_HEADER_SIZE + "agent:1:tmp".len);
    expect_eq(std.mem.eql(u8, f1[0..4], "CPKV"), true);
    expect_eq(f1[0], WAL_MAGIC[0]);

    // Verify CRC32 covers [op + key_len + val_len + key + val]
    const expected_crc = WalCodec.crc32(f1[8..f1.len]);
    const stored_crc: u32 = (f1[4] as u32) | ((f1[5] as u32) << 8) | ((f1[6] as u32) << 16) | ((f1[7] as u32) << 24);
    expect_eq(stored_crc, expected_crc);

    // 3. Sequential multi-frame decode + Eof
    let combined = alloc.alloc(u8, f1.len + f2.len) catch unreachable;
    defer alloc.free(combined);
    @memcpy(combined[0..f1.len], f1);
    @memcpy(combined[f1.len..combined.len], f2);

    const d1 = WalCodec.decodeNext(combined, 0);
    const frame1 = d1.asRecord().?;
    expect_eq(frame1.record.op == .Put, true);
    expect_eq(std.mem.eql(u8, frame1.record.key, "agent:1:goal"), true);
    expect_eq(std.mem.eql(u8, frame1.record.value, "persist-state"), true);
    expect_eq(frame1.next_offset, f1.len);

    const d2 = WalCodec.decodeNext(combined, frame1.next_offset);
    const frame2 = d2.asRecord().?;
    expect_eq(frame2.record.op == .Delete, true);
    expect_eq(std.mem.eql(u8, frame2.record.key, "agent:1:tmp"), true);
    expect_eq(frame2.record.value.len, 0 as usize);
    expect_eq(frame2.next_offset, combined.len);

    const d_eof = WalCodec.decodeNext(combined, frame2.next_offset);
    expect_eq(d_eof.isEof(), true);

    // 4. Single-byte tamper detection at magic, crc, op, key, and value positions
    const tamper_offsets: [5]usize = [1, 5, 8, WAL_HEADER_SIZE + 2, f1.len - 1];
    for (off of tamper_offsets[0..5]) {
        let corrupted = alloc.dupe(u8, combined) catch unreachable;
        defer alloc.free(corrupted);
        corrupted[off] = corrupted[off] ^ 0x01;
        const dec_bad = WalCodec.decodeNext(corrupted, 0);
        expect_eq(dec_bad.isCorrupt(), true);
    }

    // Bad short magic (< 4B) -> CorruptRecord
    expect_eq(WalCodec.decodeNext("\xFF\x00", 0).isCorrupt(), true);

    // 5. TornTail detection (partial magic, partial header < 17B, partial payload)
    expect_eq(WalCodec.decodeNext(f1[0..2], 0).isTornTail(), true);
    expect_eq(WalCodec.decodeNext(f1[0..10], 0).isTornTail(), true);
    expect_eq(WalCodec.decodeNext(f1[0 .. f1.len - 1], 0).isTornTail(), true);
}

fn testKvStoreDiskCrudPrefixScanAndReopen()! {
    const alloc = std.heap.page_allocator;
    let dir_buf: [256]u8 = undefined;
    const dir = makeTmpDirPath(&dir_buf);
    createDir(dir);
    defer removeDir(dir);

    let db_buf: [512]u8 = undefined;
    const db_path = std.fmt.bufPrint(&db_buf, "{s}/state.wal", .{dir}) catch unreachable;
    defer removeFile(db_path);

    {
        let store = KvStore.open(alloc, db_path, KvStoreOptions{ sync_on_write: true })?;
        defer store.deinit();

        expect_eq(store.len(), 0 as usize);

        store.put("agent/z_last", "z")?;
        store.put("agent/alpha/step", "1")?;
        store.put("agent/alpha/goal", "build-kv")?;
        store.put("agent/beta/step", "9")?;
        store.put("session/id", "sess-42")?;
        store.put("agent/alpha/step", "2")?; // overwrite -> 1 dead record

        expect_eq(store.len(), 5 as usize);
        expect_eq(store.has("agent/alpha/step"), true);
        expect_eq(std.mem.eql(u8, store.get("agent/alpha/step").?, "2"), true);
        expect_eq(store.get("missing_key") == null, true);

        const del_ok = store.delete("agent/beta/step")?;
        expect_eq(del_ok, true);
        const del_miss = store.delete("agent/beta/step")?;
        expect_eq(del_miss, false);
        expect_eq(store.has("agent/beta/step"), false);
        expect_eq(store.get("agent/beta/step") == null, true);
        expect_eq(store.len(), 4 as usize);

        // Prefix scan sorted lexicographically
        const alpha_keys = store.prefixScan(alloc, "agent/alpha/")?;
        defer KvStore.freeKeys(alloc, alpha_keys);
        expect_eq(alpha_keys.len, 2 as usize);
        expect_eq(std.mem.eql(u8, alpha_keys[0], "agent/alpha/goal"), true);
        expect_eq(std.mem.eql(u8, alpha_keys[1], "agent/alpha/step"), true);

        // Full keys() sorted lexicographically
        const all_keys = store.keys(alloc)?;
        defer KvStore.freeKeys(alloc, all_keys);
        expect_eq(all_keys.len, 4 as usize);
        expect_eq(std.mem.eql(u8, all_keys[0], "agent/alpha/goal"), true);
        expect_eq(std.mem.eql(u8, all_keys[1], "agent/alpha/step"), true);
        expect_eq(std.mem.eql(u8, all_keys[2], "agent/z_last"), true);
        expect_eq(std.mem.eql(u8, all_keys[3], "session/id"), true);

        store.sync()?;
        store.close();
    }

    // Reopen from disk and verify 100% state recovery + stats
    let reopened = KvStore.open(alloc, db_path, KvStoreOptions{})?;
    defer reopened.deinit();

    expect_eq(reopened.len(), 4 as usize);
    expect_eq(std.mem.eql(u8, reopened.get("agent/alpha/goal").?, "build-kv"), true);
    expect_eq(std.mem.eql(u8, reopened.get("agent/alpha/step").?, "2"), true);
    expect_eq(std.mem.eql(u8, reopened.get("agent/z_last").?, "z"), true);
    expect_eq(std.mem.eql(u8, reopened.get("session/id").?, "sess-42"), true);
    expect_eq(reopened.has("agent/beta/step"), false);

    const st = reopened.stats();
    expect_eq(st.live_keys, 4 as usize);
    expect_eq(st.total_records, 7 as u64);
    expect_eq(st.dead_records, 3 as u64);
    expect_eq(st.wal_bytes, fileSizeBytes(db_path));
}

fn testCrashTornTailRecoveryAndMidCorruptionGuard()! {
    const alloc = std.heap.page_allocator;
    let dir_buf: [256]u8 = undefined;
    const dir = makeTmpDirPath(&dir_buf);
    createDir(dir);
    defer removeDir(dir);

    let db_buf: [512]u8 = undefined;
    const db_path = std.fmt.bufPrint(&db_buf, "{s}/crash.wal", .{dir}) catch unreachable;
    defer removeFile(db_path);

    let valid_size: u64 = 0;
    {
        let store = KvStore.open(alloc, db_path, KvStoreOptions{ sync_on_write: true })?;
        defer store.deinit();
        store.put("ckpt:1", "step-100")?;
        store.put("ckpt:2", "step-200")?;
        store.sync()?;
        valid_size = store.stats().wal_bytes;
    }

    // Simulate crash: append half-written WAL frame bytes at tail
    const torn_frame = WalCodec.encode(alloc, WalRecord.put("ckpt:uncommitted", "partial-payload"))?;
    defer alloc.free(torn_frame);
    appendRawBytes(db_path, torn_frame[0..19])?;
    expect_eq(fileSizeBytes(db_path) > valid_size, true);

    // 1. Strict mode (recover_torn_tail: false) rejects torn tail with CorruptRecord
    let strict_err = false;
    * = KvStore.open(alloc, db_path, KvStoreOptions{ recover_torn_tail: false }) catch |e| {
        if (e == @as(anyerror, KvError.CorruptRecord)) strict_err = true;
    };
    expect_eq(strict_err, true);

    // 2. Recovery mode (recover_torn_tail: true) auto-truncates tail via ftruncate
    {
        let recovered = KvStore.open(alloc, db_path, KvStoreOptions{ recover_torn_tail: true })?;
        defer recovered.deinit();

        expect_eq(recovered.len(), 2 as usize);
        expect_eq(std.mem.eql(u8, recovered.get("ckpt:1").?, "step-100"), true);
        expect_eq(std.mem.eql(u8, recovered.get("ckpt:2").?, "step-200"), true);
        expect_eq(recovered.has("ckpt:uncommitted"), false);
        expect_eq(recovered.stats().wal_bytes, valid_size);
        expect_eq(fileSizeBytes(db_path), valid_size);

        // Verify post-recovery append works cleanly
        recovered.put("ckpt:3", "step-300")?;
        recovered.sync()?;
    }

    // Reopen again to confirm post-recovery record survived cleanly
    {
        let reopened = KvStore.open(alloc, db_path, KvStoreOptions{})?;
        defer reopened.deinit();
        expect_eq(reopened.len(), 3 as usize);
        expect_eq(std.mem.eql(u8, reopened.get("ckpt:3").?, "step-300"), true);
    }

    // 3. Mid-file corruption (flip byte in record #1) MUST fail even with recover_torn_tail: true
    flipFileByte(db_path, WAL_HEADER_SIZE + 1)?;
    let mid_corrupt = false;
    * = KvStore.open(alloc, db_path, KvStoreOptions{ recover_torn_tail: true }) catch |e| {
        if (e == @as(anyerror, KvError.CorruptRecord)) mid_corrupt = true;
    };
    expect_eq(mid_corrupt, true);
}

fn testAtomicCompactionReclaimsSpaceAndPreservesData()! {
    const alloc = std.heap.page_allocator;
    let dir_buf: [256]u8 = undefined;
    const dir = makeTmpDirPath(&dir_buf);
    createDir(dir);
    defer removeDir(dir);

    let db_buf: [512]u8 = undefined;
    const db_path = std.fmt.bufPrint(&db_buf, "{s}/compact.wal", .{dir}) catch unreachable;
    defer removeFile(db_path);

    let tmp_buf: [540]u8 = undefined;
    const tmp_path = std.fmt.bufPrint(&tmp_buf, "{s}.compact.tmp", .{db_path}) catch unreachable;
    defer removeFile(tmp_path);

    let k_buf: [32]u8 = undefined;
    let v_buf: [64]u8 = undefined;

    let before_bytes: u64 = 0;
    {
        let store = KvStore.open(alloc, db_path, KvStoreOptions{})?;
        defer store.deinit();

        // 50 keys overwritten 10 times each = 500 records
        let round: usize = 0;
        while (round < 10) : (round += 1) {
            let idx: usize = 0;
            while (idx < 50) : (idx += 1) {
                const k = std.fmt.bufPrint(&k_buf, "key:{d:0>3}", .{idx}) catch unreachable;
                const v = std.fmt.bufPrint(&v_buf, "round-{d}-payload-value-{d}", .{ round, idx }) catch unreachable;
                store.put(k, v)?;
            }
        }

        // Delete keys 0..40, leaving keys 40..50 (10 live keys)
        let del_i: usize = 0;
        while (del_i < 40) : (del_i += 1) {
            const k = std.fmt.bufPrint(&k_buf, "key:{d:0>3}", .{del_i}) catch unreachable;
            expect_eq(store.delete(k)?, true);
        }

        const st_before = store.stats();
        expect_eq(st_before.live_keys, 10 as usize);
        expect_eq(st_before.total_records, 540 as u64);
        expect_eq(st_before.dead_records, 530 as u64);
        before_bytes = st_before.wal_bytes;

        store.compact()?;

        const st_after = store.stats();
        expect_eq(st_after.live_keys, 10 as usize);
        expect_eq(st_after.total_records, 10 as u64);
        expect_eq(st_after.dead_records, 0 as u64);
        expect_eq(st_after.compactions, 1 as u64);
        expect_eq(st_after.wal_bytes < (before_bytes / 10), true);
        expect_eq(fileSizeBytes(db_path), st_after.wal_bytes);
        expect_eq(fileExists(tmp_path), false);
    }

    // Reopen compacted WAL and verify 100% integrity
    let reopened = KvStore.open(alloc, db_path, KvStoreOptions{})?;
    defer reopened.deinit();

    expect_eq(reopened.len(), 10 as usize);
    let check_i: usize = 0;
    while (check_i < 40) : (check_i += 1) {
        const k = std.fmt.bufPrint(&k_buf, "key:{d:0>3}", .{check_i}) catch unreachable;
        expect_eq(reopened.has(k), false);
    }
    while (check_i < 50) : (check_i += 1) {
        const k = std.fmt.bufPrint(&k_buf, "key:{d:0>3}", .{check_i}) catch unreachable;
        const expected_v = std.fmt.bufPrint(&v_buf, "round-9-payload-value-{d}", .{check_i}) catch unreachable;
        expect_eq(std.mem.eql(u8, reopened.get(k).?, expected_v), true);
    }
}

fn testCapKvDynamicLazyMount()! {
    const alloc = std.heap.page_allocator;
    let dir_buf: [256]u8 = undefined;
    const dir = makeTmpDirPath(&dir_buf);
    createDir(dir);
    defer removeDir(dir);

    let db_buf: [512]u8 = undefined;
    const db_path = std.fmt.bufPrint(&db_buf, "{s}/hub.wal", .{dir}) catch unreachable;
    defer removeFile(db_path);

    // Test WalCodec via cap.kv
    const raw = cap.kv.WalCodec.encode(alloc, cap.kv.WalRecord.put("hub:k", "hub:v"))?;
    defer alloc.free(raw);
    const dec_hub = cap.kv.WalCodec.decodeNext(raw, 0);
    expect_eq(dec_hub.asRecord().?.record.op == .Put, true);

    // Test KvStore via cap.kv
    {
        let store = cap.kv.KvStore.open(alloc, db_path, cap.kv.KvStoreOptions{ sync_on_write: true })?;
        defer store.deinit();
        store.put("memory:01", "active")?;
        store.put("memory:02", "standby")?;
        store.put("memory:01", "committed")?;
        store.compact()?;
        expect_eq(store.stats().compactions, 1 as u64);
    }

    let reopened = cap.kv.KvStore.open(alloc, db_path, cap.kv.KvStoreOptions{})?;
    defer reopened.deinit();
    expect_eq(reopened.len(), 2 as usize);
    expect_eq(std.mem.eql(u8, reopened.get("memory:01").?, "committed"), true);
    expect_eq(std.mem.eql(u8, reopened.get("memory:02").?, "standby"), true);
}

fn main() {
    test("1. WalCodec CRC32 Frame Codec, Tamper Detection & TornTail vs CorruptRecord", () => {
        testWalCodecCrc32TamperAndTornTail()?;
    });

    test("2. KvStore Disk Persistence, CRUD, Sorted PrefixScan & Reopen Recovery", () => {
        testKvStoreDiskCrudPrefixScanAndReopen()?;
    });

    test("3. Crash Simulation: TornTail Auto-Truncate Recovery & Mid-File Corruption Guard", () => {
        testCrashTornTailRecoveryAndMidCorruptionGuard()?;
    });

    test("4. Atomic Compaction: Reclaim Dead Overwrite/Delete Records & Preserve Integrity", () => {
        testAtomicCompactionReclaimsSpaceAndPreservesData()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.kv`", () => {
        testCapKvDynamicLazyMount()?;
    });
}
