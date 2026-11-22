// Origin: Pure-CP Stdlib Test Suite (lib/db/wal_recovery.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc CRC32 Torn-Write-Safe WAL Recovery Engine:
//   1. Multi-frame Put/Delete/Commit/Checkpoint roundtrip + IEEE 802.3 CRC32 standard vector check
//   2. Torn-tail crash recovery (both partial payload & partial header truncation)
//   3. Single-bit flip CRC32 corruption interception (header & payload tamper)
//   4. Non-monotonic LSN interception (scanner + writer enforcement)
//   5. Atomic small-buffer rejection (no half-writes) & out_frames truncation guard
//   6. Empty stream, zero-length payload frames & writer reset() reuse

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    WAL_FRAME_MAGIC,
    WAL_FRAME_HEADER_SIZE,
    walCrc32Ieee,
    WalRecordKind,
    WalFrameView,
    WalRecoveryStatus,
    WalRecoveryReport,
    WalFrameWriter,
    WalRecoveryScanner,
} from "cap:db";

fn bytesEq(a: []const u8, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Multi-Frame Roundtrip & Standard IEEE CRC32 Check Vector
// ============================================================================

fn testMultiFrameRoundtripAndCrc32()! {
    // IEEE 802.3 CRC32 standard check value for "123456789" is 0xCBF43926
    expect_eq(cap.db.walCrc32Ieee("123456789"), 0xCBF43926);
    expect_eq(WAL_FRAME_MAGIC, 0x57414C46);
    expect_eq(WAL_FRAME_HEADER_SIZE, 24);

    let log_buf: [512]u8 = [0; 512];
    let writer = WalFrameWriter.init(log_buf[0..]);

    const n1 = writer.appendFrame(10, WalRecordKind.Put, "user:1=alice") ?? return error.WriteFailed;
    const n2 = writer.appendFrame(11, WalRecordKind.Delete, "user:2") ?? return error.WriteFailed;
    const n3 = writer.appendFrame(12, WalRecordKind.Commit, "tx_1") ?? return error.WriteFailed;
    const n4 = writer.appendFrame(20, WalRecordKind.Checkpoint, "cp_v1") ?? return error.WriteFailed;
    expect_eq(n1, 24 + 12);
    expect_eq(n2, 24 + 6);
    expect_eq(n3, 24 + 4);
    expect_eq(n4, 24 + 5);
    expect_eq(writer.frameCount(), 4);
    expect_eq(writer.lastLsn(), 20);

    let frames: [8]WalFrameView = [WalFrameView{}; 8];
    const rep: WalRecoveryReport = WalRecoveryScanner.scanAndRecover(writer.writtenBytes(), frames[0..]);

    expect_eq(rep.isClean(), true);
    expect_eq(rep.recovered_count, 4);
    expect_eq(rep.frames_stored, 4);
    expect_eq(rep.frames_truncated, false);
    expect_eq(rep.valid_end_offset, writer.position());
    expect_eq(rep.torn_tail_bytes, 0);
    expect_eq(rep.has_corruption, false);
    expect_eq(rep.has_valid_lsn, true);
    expect_eq(rep.last_valid_lsn, 20);
    expect_eq(rep.has_checkpoint, true);
    expect_eq(rep.last_checkpoint_lsn, 20);

    expect_eq(frames[0].lsn, 10);
    expect_eq(frames[0].kind == WalRecordKind.Put, true);
    expect_eq(bytesEq(frames[0].payload, "user:1=alice"), true);

    expect_eq(frames[1].lsn, 11);
    expect_eq(frames[1].kind == WalRecordKind.Delete, true);
    expect_eq(bytesEq(frames[1].payload, "user:2"), true);

    expect_eq(frames[2].lsn, 12);
    expect_eq(frames[2].kind == WalRecordKind.Commit, true);
    expect_eq(bytesEq(frames[2].payload, "tx_1"), true);

    expect_eq(frames[3].lsn, 20);
    expect_eq(frames[3].kind == WalRecordKind.Checkpoint, true);
    expect_eq(bytesEq(frames[3].payload, "cp_v1"), true);
}

// ============================================================================
// Test 2: Torn-Tail Crash Recovery (Partial Payload & Partial Header)
// ============================================================================

fn testTornTailRecovery()! {
    let log_buf: [512]u8 = [0; 512];
    let writer = WalFrameWriter.init(log_buf[0..]);

    * = writer.appendFrame(1, WalRecordKind.Put, "alpha") ?? return error.WriteFailed;
    * = writer.appendFrame(2, WalRecordKind.Checkpoint, "ckpt_2") ?? return error.WriteFailed;
    const end_after_2 = writer.position();
    * = writer.appendFrame(3, WalRecordKind.Put, "unfinished_payload_bytes") ?? return error.WriteFailed;
    const full = writer.writtenBytes();

    let frames: [8]WalFrameView = [WalFrameView{}; 8];

    // 2a. Simulate crash cutting off the last 7 bytes of frame 3's payload
    const torn_payload = full[0 .. full.len - 7];
    const rep1 = WalRecoveryScanner.scanAndRecover(torn_payload, frames[0..]);
    expect_eq(rep1.isClean(), false);
    expect_eq(rep1.status == WalRecoveryStatus.TornTailTruncated, true);
    expect_eq(rep1.recovered_count, 2);
    expect_eq(rep1.valid_end_offset, end_after_2);
    expect_eq(rep1.torn_tail_bytes, torn_payload.len - end_after_2);
    expect_eq(rep1.has_corruption, false);
    expect_eq(rep1.last_valid_lsn, 2);
    expect_eq(rep1.last_checkpoint_lsn, 2);

    // 2b. Simulate crash leaving only 10 bytes of frame 3's header
    const torn_header = full[0 .. end_after_2 + 10];
    const rep2 = WalRecoveryScanner.scanAndRecover(torn_header, frames[0..]);
    expect_eq(rep2.status == WalRecoveryStatus.TornTailTruncated, true);
    expect_eq(rep2.recovered_count, 2);
    expect_eq(rep2.valid_end_offset, end_after_2);
    expect_eq(rep2.torn_tail_bytes, 10);
    expect_eq(rep2.last_valid_lsn, 2);
}

// ============================================================================
// Test 3: Single-Bit Flip CRC32 Tamper Interception
// ============================================================================

fn testSingleBitFlipCrcInterception()! {
    let log_buf: [512]u8 = [0; 512];
    let writer = WalFrameWriter.init(log_buf[0..]);

    * = writer.appendFrame(1, WalRecordKind.Put, "frame_one") ?? return error.WriteFailed;
    const off_frame_2 = writer.position();
    * = writer.appendFrame(2, WalRecordKind.Put, "frame_two_secret") ?? return error.WriteFailed;
    * = writer.appendFrame(3, WalRecordKind.Commit, "ok") ?? return error.WriteFailed;
    const total_len = writer.position();

    let frames: [8]WalFrameView = [WalFrameView{}; 8];

    // 3a. Flip 1 bit inside frame 2's payload
    log_buf[off_frame_2 + WAL_FRAME_HEADER_SIZE + 3] = log_buf[off_frame_2 + WAL_FRAME_HEADER_SIZE + 3] ^ 0x01;
    const rep_payload = WalRecoveryScanner.scanAndRecover(log_buf[0..total_len], frames[0..]);
    expect_eq(rep_payload.status == WalRecoveryStatus.CorruptedTruncated, true);
    expect_eq(rep_payload.has_corruption, true);
    expect_eq(rep_payload.corrupted_offset, off_frame_2);
    expect_eq(rep_payload.valid_end_offset, off_frame_2);
    expect_eq(rep_payload.recovered_count, 1);
    expect_eq(rep_payload.last_valid_lsn, 1);

    // Restore payload bit, then flip 1 bit in frame 2's LSN header byte
    log_buf[off_frame_2 + WAL_FRAME_HEADER_SIZE + 3] = log_buf[off_frame_2 + WAL_FRAME_HEADER_SIZE + 3] ^ 0x01;
    log_buf[off_frame_2 + 4] = log_buf[off_frame_2 + 4] ^ 0x08;
    const rep_hdr = WalRecoveryScanner.scanAndRecover(log_buf[0..total_len], frames[0..]);
    expect_eq(rep_hdr.status == WalRecoveryStatus.CorruptedTruncated, true);
    expect_eq(rep_hdr.corrupted_offset, off_frame_2);
    expect_eq(rep_hdr.recovered_count, 1);
}

// ============================================================================
// Test 4: Non-Monotonic LSN Interception (Writer & Scanner)
// ============================================================================

fn testNonMonotonicLsnInterception()! {
    // 4a. Writer rejects duplicate or decreasing LSN up front
    let buf_a: [256]u8 = [0; 256];
    let wa = WalFrameWriter.init(buf_a[0..]);
    * = wa.appendFrame(10, WalRecordKind.Put, "a") ?? return error.WriteFailed;
    expect_eq(wa.appendFrame(10, WalRecordKind.Put, "dup") == null, true);
    expect_eq(wa.appendFrame(9, WalRecordKind.Put, "back") == null, true);
    expect_eq(wa.frameCount(), 1);

    // 4b. Concatenate two individually valid frames where second has lower LSN (10 -> 5)
    let buf_b: [128]u8 = [0; 128];
    let wb = WalFrameWriter.init(buf_b[0..]);
    * = wb.appendFrame(5, WalRecordKind.Put, "stale") ?? return error.WriteFailed;

    let combined: [256]u8 = [0; 256];
    const part1 = wa.writtenBytes();
    const part2 = wb.writtenBytes();
    let i: usize = 0;
    while (i < part1.len) : (i += 1) { combined[i] = part1[i]; }
    let j: usize = 0;
    while (j < part2.len) : (j += 1) { combined[part1.len + j] = part2[j]; }

    let frames: [4]WalFrameView = [WalFrameView{}; 4];
    const rep = WalRecoveryScanner.scanAndRecover(combined[0 .. part1.len + part2.len], frames[0..]);
    expect_eq(rep.status == WalRecoveryStatus.NonMonotonicLsnTruncated, true);
    expect_eq(rep.has_corruption, true);
    expect_eq(rep.corrupted_offset, part1.len);
    expect_eq(rep.valid_end_offset, part1.len);
    expect_eq(rep.recovered_count, 1);
    expect_eq(rep.last_valid_lsn, 10);
}

// ============================================================================
// Test 5: Atomic Small-Buffer Rejection & Out-Frames Truncation Guard
// ============================================================================

fn testAtomicSmallBufferAndViewTruncation()! {
    // Buffer can fit exactly 1 frame of 24 + 6 = 30 bytes, plus 10 spare bytes (< 24)
    let small: [40]u8 = [0; 40];
    let writer = WalFrameWriter.init(small[0..]);

    const n1 = writer.appendFrame(1, WalRecordKind.Put, "abcdef") ?? return error.WriteFailed;
    expect_eq(n1, 30);
    expect_eq(writer.remaining(), 10);

    // Next frame needs 24 + 2 = 26 bytes > 10 -> rejected atomically, offset untouched
    expect_eq(writer.appendFrame(2, WalRecordKind.Put, "xy") == null, true);
    expect_eq(writer.position(), 30);
    expect_eq(writer.frameCount(), 1);
    expect_eq(writer.lastLsn(), 1);

    // Verify out_frames truncation when caller passes smaller view array (len 1 for 3 frames)
    let big: [256]u8 = [0; 256];
    let wb = WalFrameWriter.init(big[0..]);
    * = wb.appendFrame(1, WalRecordKind.Put, "p1") ?? return error.WriteFailed;
    * = wb.appendFrame(2, WalRecordKind.Put, "p2") ?? return error.WriteFailed;
    * = wb.appendFrame(3, WalRecordKind.Checkpoint, "p3") ?? return error.WriteFailed;

    let one_view: [1]WalFrameView = [WalFrameView{}; 1];
    const rep = WalRecoveryScanner.scanAndRecover(wb.writtenBytes(), one_view[0..]);
    expect_eq(rep.isClean(), true);
    expect_eq(rep.recovered_count, 3);
    expect_eq(rep.frames_stored, 1);
    expect_eq(rep.frames_truncated, true);
    expect_eq(rep.last_valid_lsn, 3);
    expect_eq(rep.last_checkpoint_lsn, 3);
}

// ============================================================================
// Test 6: Empty Stream, Zero-Length Payloads & Writer Reset Reuse
// ============================================================================

fn testEmptyStreamAndZeroPayload()! {
    let empty: [0]u8 = [];
    let views: [4]WalFrameView = [WalFrameView{}; 4];

    const empty_rep = WalRecoveryScanner.scanAndRecover(empty[0..], views[0..]);
    expect_eq(empty_rep.isClean(), true);
    expect_eq(empty_rep.recovered_count, 0);
    expect_eq(empty_rep.valid_end_offset, 0);
    expect_eq(empty_rep.has_valid_lsn, false);
    expect_eq(empty_rep.has_checkpoint, false);

    // Zero-length payload Commit & Checkpoint markers + reset() reuse
    let buf: [128]u8 = [0; 128];
    let writer = WalFrameWriter.init(buf[0..]);
    * = writer.appendFrame(1, WalRecordKind.Commit, "") ?? return error.WriteFailed;
    * = writer.appendFrame(2, WalRecordKind.Checkpoint, "") ?? return error.WriteFailed;
    expect_eq(writer.position(), 48);

    const rep = WalRecoveryScanner.scanAndRecover(writer.writtenBytes(), views[0..]);
    expect_eq(rep.isClean(), true);
    expect_eq(rep.recovered_count, 2);
    expect_eq(views[0].payload.len, 0);
    expect_eq(views[1].payload.len, 0);
    expect_eq(rep.last_checkpoint_lsn, 2);

    writer.reset();
    expect_eq(writer.position(), 0);
    expect_eq(writer.frameCount(), 0);
    * = writer.appendFrame(1, WalRecordKind.Put, "reused") ?? return error.WriteFailed;
    expect_eq(writer.frameCount(), 1);
}

test("db.wal_recovery: multi-frame Put/Delete/Commit/Checkpoint roundtrip & IEEE CRC32", testMultiFrameRoundtripAndCrc32);
test("db.wal_recovery: torn-tail crash recovery (partial payload & partial header)", testTornTailRecovery);
test("db.wal_recovery: single-bit flip CRC32 corruption interception", testSingleBitFlipCrcInterception);
test("db.wal_recovery: non-monotonic LSN interception (writer & scanner)", testNonMonotonicLsnInterception);
test("db.wal_recovery: atomic small-buffer rejection & out_frames truncation guard", testAtomicSmallBufferAndViewTruncation);
test("db.wal_recovery: empty stream, zero-length payloads & writer reset reuse", testEmptyStreamAndZeroPayload);
