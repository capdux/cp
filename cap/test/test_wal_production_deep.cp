// Source: cp/cap/test/test_wal_production_deep.cp
// Deep Production Test Suite for cap:wal
// Covers:
//   1. append / appendTyped / appendBatch and recoverStrict full LSN order round-trip
//   2. Bit-flip tampering on payload & header CRC32C ChecksumMismatch + half-written TruncatedTail
//   3. recoverWithTailTruncation crash recovery, valid_bytes/truncated_tail_bytes accounting & continued append
//   4. compactBefore LSN compaction, PayloadTooLarge & SequenceOutOfOrder boundary guards
//   5. Dynamic `import cap from "cap"; cap.wal` namespace mounting & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    WAL_MAGIC,
    WAL_HEADER_SIZE,
    WalError,
    WalRecordType,
    WalRecord,
    WalRecoveryResult,
    WalLog,
    crc32c,
} from "cap:wal";
import cap from "cap";

fn testAppendBatchAndStrictRecover()! {
    const alloc = std.testing.allocator;

    let wal = WalLog.init(alloc, 4096);
    defer wal.deinit();

    const lsn1 = wal.append("put:user:1=alice")?;
    const lsn2 = wal.appendTyped(WalRecordType.Checkpoint, "ckpt:v1")?;
    const lsn3 = wal.appendTyped(WalRecordType.Tombstone, "del:user:0")?;

    const batch: [2][]const u8 = ["batch:k1=v1", "batch:k2=v2"];
    const lsn5 = wal.appendBatch(&batch)?;

    expect_eq(lsn1, 1 as u64);
    expect_eq(lsn2, 2 as u64);
    expect_eq(lsn3, 3 as u64);
    expect_eq(lsn5, 5 as u64);
    expect_eq(wal.lastLsn(), 5 as u64);
    expect_eq(wal.recordCount(), 5 as usize);

    let res = WalLog.recoverStrict(alloc, wal.bytes())?;
    defer res.deinit(alloc);

    expect_eq(res.records.len, 5 as usize);
    expect_eq(res.valid_bytes, wal.byteLen());
    expect_eq(res.truncated_tail_bytes, 0 as usize);
    expect_eq(res.last_lsn, 5 as u64);

    expect_eq(res.records[0].lsn, 1 as u64);
    expect_eq(res.records[0].record_type == WalRecordType.Data, true);
    expect_eq(std.mem.eql(u8, res.records[0].payload, "put:user:1=alice"), true);

    expect_eq(res.records[1].lsn, 2 as u64);
    expect_eq(res.records[1].record_type == WalRecordType.Checkpoint, true);
    expect_eq(std.mem.eql(u8, res.records[1].payload, "ckpt:v1"), true);

    expect_eq(res.records[2].lsn, 3 as u64);
    expect_eq(res.records[2].record_type == WalRecordType.Tombstone, true);
    expect_eq(std.mem.eql(u8, res.records[2].payload, "del:user:0"), true);

    expect_eq(res.records[3].lsn, 4 as u64);
    expect_eq(std.mem.eql(u8, res.records[3].payload, "batch:k1=v1"), true);
    expect_eq(res.records[4].lsn, 5 as u64);
    expect_eq(std.mem.eql(u8, res.records[4].payload, "batch:k2=v2"), true);
}

fn testBitFlipAndTruncatedTailStrict()! {
    const alloc = std.testing.allocator;

    // Castagnoli CRC32C known check vector
    expect_eq(crc32c("123456789"), 0xE3069283 as u32);

    let wal = WalLog.init(alloc, 4096);
    defer wal.deinit();
    * = wal.append("alpha-record")?;
    * = wal.append("beta-record")?;

    const raw = wal.bytes();
    let tampered = alloc.dupe(u8, raw)?;
    defer alloc.free(tampered);

    // 1. Flip a bit in the first record's payload -> ChecksumMismatch
    tampered[WAL_HEADER_SIZE + 2] ^= 0x01;
    switch (WalLog.recoverStrict(alloc, tampered)) {
        v => {
            let r = v;
            r.deinit(alloc);
            expect_eq(true, false);
        },
        err(e) => expect_eq(e == WalError.ChecksumMismatch, true),
    }

    // Restore payload bit, then flip a bit in LSN header (offset 5) -> ChecksumMismatch
    tampered[WAL_HEADER_SIZE + 2] ^= 0x01;
    tampered[5] ^= 0x80;
    switch (WalLog.recoverStrict(alloc, tampered)) {
        v => {
            let r = v;
            r.deinit(alloc);
            expect_eq(true, false);
        },
        err(e) => expect_eq(e == WalError.ChecksumMismatch, true),
    }

    // Restore header bit, then flip magic byte (offset 0) -> CorruptedFrame
    tampered[5] ^= 0x80;
    tampered[0] = 'X';
    switch (WalLog.recoverStrict(alloc, tampered)) {
        v => {
            let r = v;
            r.deinit(alloc);
            expect_eq(true, false);
        },
        err(e) => expect_eq(e == WalError.CorruptedFrame, true),
    }

    // 2. Half-written tail (cut off last 5 bytes) -> TruncatedTail
    const half_tail = raw[0 .. raw.len - 5];
    switch (WalLog.recoverStrict(alloc, half_tail)) {
        v => {
            let r = v;
            r.deinit(alloc);
            expect_eq(true, false);
        },
        err(e) => expect_eq(e == WalError.TruncatedTail, true),
    }

    // 3. Half-written header (only 10 bytes of second frame) -> TruncatedTail
    const frame1_len: usize = WAL_HEADER_SIZE + "alpha-record".len;
    const half_hdr = raw[0 .. frame1_len + 10];
    switch (WalLog.recoverStrict(alloc, half_hdr)) {
        v => {
            let r = v;
            r.deinit(alloc);
            expect_eq(true, false);
        },
        err(e) => expect_eq(e == WalError.TruncatedTail, true),
    }
}

fn testTailTruncationRecoveryAndContinue()! {
    const alloc = std.testing.allocator;

    let wal = WalLog.init(alloc, 4096);
    defer wal.deinit();
    * = wal.append("frame-1")?;
    * = wal.append("frame-2")?;
    const valid_end = wal.byteLen();
    * = wal.append("frame-3-torn")?;

    // Simulate power-loss torn write in the middle of frame 3's payload
    const torn_raw = wal.bytes()[0 .. valid_end + WAL_HEADER_SIZE + 4];
    const torn_len = torn_raw.len - valid_end;

    let res = WalLog.recoverWithTailTruncation(alloc, torn_raw)?;
    defer res.deinit(alloc);

    expect_eq(res.records.len, 2 as usize);
    expect_eq(res.valid_bytes, valid_end);
    expect_eq(res.truncated_tail_bytes, torn_len);
    expect_eq(res.last_lsn, 2 as u64);
    expect_eq(std.mem.eql(u8, res.records[0].payload, "frame-1"), true);
    expect_eq(std.mem.eql(u8, res.records[1].payload, "frame-2"), true);

    // Restore WalLog from truncated stream and continue appending monotonically
    let restored = WalLog.restoreTruncated(alloc, 4096, torn_raw)?;
    defer restored.deinit();

    expect_eq(restored.lastLsn(), 2 as u64);
    expect_eq(restored.recordCount(), 2 as usize);

    const next_lsn = restored.append("frame-3-recovered")?;
    expect_eq(next_lsn, 3 as u64);
    expect_eq(restored.recordCount(), 3 as usize);

    let strict_after = WalLog.recoverStrict(alloc, restored.bytes())?;
    defer strict_after.deinit(alloc);
    expect_eq(strict_after.records.len, 3 as usize);
    expect_eq(std.mem.eql(u8, strict_after.records[2].payload, "frame-3-recovered"), true);
}

fn testCompactionAndBoundaryGuards()! {
    const alloc = std.testing.allocator;

    let wal = WalLog.init(alloc, 32);
    defer wal.deinit();

    * = wal.append("rec-1")?;
    * = wal.append("rec-2")?;
    * = wal.append("rec-3")?;
    * = wal.append("rec-4")?;
    const before_bytes = wal.byteLen();

    // Compact LSN < 3 (removes rec-1 and rec-2, keeps rec-3 and rec-4)
    wal.compactBefore(alloc, 3)?;
    expect_eq(wal.recordCount(), 2 as usize);
    expect_eq(wal.lastLsn(), 4 as u64);
    expect_eq(wal.byteLen() < before_bytes, true);

    let after_compact = WalLog.recoverStrict(alloc, wal.bytes())?;
    defer after_compact.deinit(alloc);
    expect_eq(after_compact.records.len, 2 as usize);
    expect_eq(after_compact.records[0].lsn, 3 as u64);
    expect_eq(after_compact.records[1].lsn, 4 as u64);

    // Subsequent append continues from high-water mark (LSN 5)
    const lsn5 = wal.append("rec-5")?;
    expect_eq(lsn5, 5 as u64);

    // PayloadTooLarge guard
    switch (wal.append("012345678901234567890123456789012345")) {
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == WalError.PayloadTooLarge, true),
    }

    // SequenceOutOfOrder guard on appendWithLsn
    switch (wal.appendWithLsn(5, WalRecordType.Data, "dup-lsn")) {
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == WalError.SequenceOutOfOrder, true),
    }

    // SequenceOutOfOrder guard on recoverStrict (concatenate two independent logs starting at LSN 1)
    let log_a = WalLog.init(alloc, 1024);
    defer log_a.deinit();
    * = log_a.append("a1")?;
    * = log_a.append("a2")?;

    let log_b = WalLog.init(alloc, 1024);
    defer log_b.deinit();
    * = log_b.append("b1")?;

    const combined = alloc.alloc(u8, log_a.byteLen() + log_b.byteLen())?;
    defer alloc.free(combined);
    std.mem.copyForwards(u8, combined[0..log_a.byteLen()], log_a.bytes());
    std.mem.copyForwards(u8, combined[log_a.byteLen()..], log_b.bytes());

    switch (WalLog.recoverStrict(alloc, combined)) {
        v => {
            let r = v;
            r.deinit(alloc);
            expect_eq(true, false);
        },
        err(e) => expect_eq(e == WalError.SequenceOutOfOrder, true),
    }
}

fn testCapDynamicNamespaceMount()! {
    const alloc = std.testing.allocator;

    expect_eq(cap.wal.WAL_HEADER_SIZE, 24 as usize);
    expect_eq(cap.wal.crc32c("123456789"), 0xE3069283 as u32);
    expect_eq(std.mem.eql(u8, &cap.wal.WAL_MAGIC, &WAL_MAGIC), true);

    let wal = cap.wal.WalLog.init(alloc, 1024);
    defer wal.deinit();
    const lsn = wal.appendTyped(cap.wal.WalRecordType.Checkpoint, "cap-wal-mount")?;
    expect_eq(lsn, 1 as u64);

    let res = cap.wal.WalLog.recoverStrict(alloc, wal.bytes())?;
    defer res.deinit(alloc);
    expect_eq(res.records.len, 1 as usize);
    expect_eq(std.mem.eql(u8, res.records[0].payload, "cap-wal-mount"), true);
}

test("wal: append / appendTyped / appendBatch and recoverStrict LSN round-trip", () => {
    testAppendBatchAndStrictRecover() catch unreachable;
});

test("wal: bit-flip CRC32C ChecksumMismatch, CorruptedFrame and TruncatedTail guards", () => {
    testBitFlipAndTruncatedTailStrict() catch unreachable;
});

test("wal: recoverWithTailTruncation crash recovery and continued append", () => {
    testTailTruncationRecoveryAndContinue() catch unreachable;
});

test("wal: compactBefore LSN compaction, PayloadTooLarge and SequenceOutOfOrder guards", () => {
    testCompactionAndBoundaryGuards() catch unreachable;
});

test("wal: dynamic cap.wal namespace mounting and zero-leak verification", () => {
    testCapDynamicNamespaceMount() catch unreachable;
});
