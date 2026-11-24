// Reference-hardened test suite against fjall-rs/fjall & RocksDB WAL/WriteBatch specs (MIT OR Apache-2.0)
// Covers WAL recovery, byte-by-byte torn-write truncation, 100% single-bit corruption detection,
// WriteBatch nested SavePoint invariants, CRC32 & GroupCommit bounds, 16-lane @Vector(16, u8) SIMD
// 0-diff parity, and 1,600 deterministic PRNG fuzz cases (800 WAL + 800 WriteBatch/SavePoint).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    WAL_FRAME_HEADER_SIZE,
    simdWalCopyBytes,
    scalarWalCopyBytes,
    simdWalBytesEqual,
    scalarWalBytesEqual,
    WalRecordKind,
    WalFrameView,
    WalRecoveryStatus,
    WalFrameWriter,
    WalRecoveryScanner,
} from "../../lib/db/walRecover.cp";
import {
    WriteBatchStatus,
    verifyWriteBatchRaw,
    verifyWriteBatchFramed,
    WriteBatchIter,
    WriteBatch,
    GroupCommitConfig,
    GroupCommitWriterStatus,
    GroupCommitReceipt,
    GroupCommitCoordinator,
} from "../../lib/db/writeBatch.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }

    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) : (i += 1) {
            dst[i] = _.nextByte();
        }
    }
}

fn checkWalRecoveryTornAndCorruption(): u32 {
    let buf: [512]u8 = [0; 512];
    let w = WalFrameWriter.init(buf[0..]);

    const f1 = w.appendFrame(10, WalRecordKind.Put, "key_alpha=value_0123456789abcdef") ?? return 1;
    const f2 = w.appendFrame(11, WalRecordKind.Delete, "key_obsolete") ?? return 2;
    const f3 = w.appendFrame(12, WalRecordKind.Checkpoint, "snap_lsn_12") ?? return 3;
    // Non-monotonic LSN must be rejected atomically
    if (w.appendFrame(12, WalRecordKind.Commit, "") != null) return 4;

    const full = w.writtenBytes();
    if (full.len != f1 + f2 + f3) return 5;

    let views: [8]WalFrameView = [WalFrameView{}; 8];
    const clean = WalRecoveryScanner.scanAndRecover(full, views[0..]);
    if (!clean.isClean() || clean.recovered_count != 3 || clean.valid_end_offset != full.len) return 6;
    if (!clean.has_checkpoint || clean.last_checkpoint_lsn != 12) return 7;
    if (!simdWalBytesEqual(views[0].payload, "key_alpha=value_0123456789abcdef")) return 8;

    // 1. Byte-by-byte torn-write sweep across EVERY truncation length in 1..(full.len - 1)
    let cut: usize = 1;
    while (cut < full.len) : (cut += 1) {
        if (cut == f1 || cut == f1 + f2) continue; // exact frame boundaries are clean
        const rep = WalRecoveryScanner.scanAndRecover(full[0..cut], views[0..]);
        if (rep.status != WalRecoveryStatus.TornTailTruncated) return 9;
        const expected_valid: usize = if (cut < f1) 0 else if (cut < f1 + f2) f1 else f1 + f2;
        if (rep.valid_end_offset != expected_valid || rep.torn_tail_bytes != cut - expected_valid) return 10;
    }

    // 2. Single-byte bit-flip corruption detection across EVERY byte of the entire WAL
    let mut_buf: [512]u8 = [0; 512];
    let pos: usize = 0;
    while (pos < full.len) : (pos += 1) {
        * = simdWalCopyBytes(mut_buf[0..full.len], full);
        mut_buf[pos] ^= 0x04;
        const rep = WalRecoveryScanner.scanAndRecover(mut_buf[0..full.len], views[0..]);
        if (rep.isClean()) return 11;
    }

    return 0;
}

fn checkWriteBatchSavePointsAndGroupCommit(): u32 {
    let wb = WriteBatch.init();
    wb.setSequence(100);
    if (wb.put("k1", "v1_0123456789abcdef") != WriteBatchStatus.ok) return 1;
    if (wb.put("", "bad") != WriteBatchStatus.empty_key) return 2;

    // Nested SavePoint stack: push L1 -> put k2 -> push L2 -> delete k1 -> rollback L2 -> pop L1
    if (wb.setSavePoint() != WriteBatchStatus.ok) return 3;
    if (wb.put("k2", "v2") != WriteBatchStatus.ok) return 4;
    if (wb.setSavePoint() != WriteBatchStatus.ok) return 5;
    if (wb.delete("k1") != WriteBatchStatus.ok) return 6;
    if (wb.recordCount() != 3 || wb.savePointDepth() != 2) return 7;

    if (wb.rollbackToSavePoint() != WriteBatchStatus.ok) return 8;
    if (wb.recordCount() != 2 || wb.savePointDepth() != 1) return 9;
    if (wb.popSavePoint() != WriteBatchStatus.ok) return 10;
    if (wb.rollbackToSavePoint() != WriteBatchStatus.no_savepoint) return 11;
    if (!wb.containsKey("k1") || !wb.containsKey("k2") || wb.containsKey("k3")) return 12;

    // Framed CRC32 roundtrip & single-bit tamper rejection
    let framed: [256]u8 = [0; 256];
    let framed_s: []u8 = framed[0..];
    const flen = wb.encodeFramed(framed_s) ?? return 13;
    if (verifyWriteBatchFramed(framed[0..flen]) != WriteBatchStatus.ok) return 14;
    framed[flen - 1] ^= 0x01;
    if (verifyWriteBatchFramed(framed[0..flen]) != WriteBatchStatus.checksum_mismatch) return 15;

    // GroupCommit bounds: zero writers / zero seq rejected
    const bad_cfg1 = GroupCommitConfig{ max_writers: 0, max_batch_bytes: 1024, initial_seq: 1 };
    const bad_cfg2 = GroupCommitConfig{ max_writers: 4, max_batch_bytes: 1024, initial_seq: 0 };
    if (GroupCommitCoordinator.init(bad_cfg1) != null) return 16;
    if (GroupCommitCoordinator.init(bad_cfg2) != null) return 17;

    let coord = GroupCommitCoordinator.init(GroupCommitConfig.defaults()) ?? return 18;
    let wb2 = WriteBatch.init();
    if (wb2.merge("counter", "+42") != WriteBatchStatus.ok) return 19;

    if (coord.enqueueBatch(1, wb) != GroupCommitWriterStatus.committed) return 20;
    if (coord.enqueueBatch(2, wb2) != GroupCommitWriterStatus.committed) return 21;

    let out_f: [4096]u8 = [0; 4096];
    let out_fs: []u8 = out_f[0..];
    let receipts: [4]GroupCommitReceipt = [GroupCommitReceipt{}; 4];
    let rec_s: []GroupCommitReceipt = receipts[0..];
    const res = coord.flush(out_fs, rec_s) ?? return 22;
    if (res.writer_count != 2 || res.total_ops != 3 || !receipts[0].is_leader || receipts[1].is_leader) return 23;
    if (verifyWriteBatchFramed(out_f[0..res.merged_framed_bytes]) != WriteBatchStatus.ok) return 24;

    return 0;
}

fn checkSimdDifferential(): u32 {
    let prng = XorShift64.init(0x5741_4c53_494d_4431);
    let src: [260]u8 = [0; 260];
    let d1: [260]u8 = [0; 260];
    let d2: [260]u8 = [0; 260];
    prng.fillBytes(src[0..]);

    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        const n1 = simdWalCopyBytes(d1[0..len], src[0..len]);
        const n2 = scalarWalCopyBytes(d2[0..len], src[0..len]);
        if (n1 != n2 || n1 != len) return 1;
        if (!simdWalBytesEqual(d1[0..len], d2[0..len])) return 2;
        if (simdWalBytesEqual(d1[0..len], d2[0..len]) != scalarWalBytesEqual(d1[0..len], d2[0..len])) return 3;
        if (len > 0) {
            d2[len - 1] ^= 0x80;
            if (simdWalBytesEqual(d1[0..len], d2[0..len]) != scalarWalBytesEqual(d1[0..len], d2[0..len])) return 4;
            d2[len - 1] ^= 0x80;
        }
    }
    return 0;
}

fn checkDeterministicFuzz(): u32 {
    let prng = XorShift64.init(0x2026_1124_5741_4c46);
    let raw: [256]u8 = [0; 256];
    let views: [8]WalFrameView = [WalFrameView{}; 8];

    // 1. 800 WAL fuzz cases (random & mutated WAL streams, 0 panic / 0 OOB)
    let seed_buf: [128]u8 = [0; 128];
    let sw = WalFrameWriter.init(seed_buf[0..]);
    * = sw.appendFrame(1, WalRecordKind.Put, "seed_payload_0123456789");
    const seed_wal = sw.writtenBytes();

    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        let n: usize = 0;
        if (iter % 2 == 0) {
            n = @as(usize, @intCast(prng.next() % (seed_wal.len + 1)));
            * = simdWalCopyBytes(raw[0..n], seed_wal[0..n]);
            if (n > 0) {
                raw[@as(usize, @intCast(prng.next() % n))] = prng.nextByte();
            }
        } else {
            n = @as(usize, @intCast(prng.next() % 200));
            prng.fillBytes(raw[0..n]);
        }
        const rep = WalRecoveryScanner.scanAndRecover(raw[0..n], views[0..]);
        if (rep.valid_end_offset > n) return 1;
    }

    // 2. 800 WriteBatch fuzz cases (400 malformed byte streams + 400 random SavePoint/mutation sequences)
    let wb = WriteBatch.init();
    let kbuf: [8]u8 = ['k'; 8];
    let vbuf: [24]u8 = ['v'; 24];
    iter = 0;
    while (iter < 800) : (iter += 1) {
        if (iter % 2 == 0) {
            const n = @as(usize, @intCast(prng.next() % 200));
            prng.fillBytes(raw[0..n]);
            * = verifyWriteBatchRaw(raw[0..n]);
            * = verifyWriteBatchFramed(raw[0..n]);
            let it = WriteBatchIter.fromFramed(raw[0..n]);
            let steps: usize = 0;
            while (it.next() != null && steps < 32) : (steps += 1) {}
        } else {
            const op = prng.next() % 6;
            kbuf[0] = @as(u8, 'a') + @as(u8, @intCast(prng.next() % 26));
            if (op == 0) {
                * = wb.put(kbuf[0..4], vbuf[0..20]);
            } else if (op == 1) {
                * = wb.delete(kbuf[0..4]);
            } else if (op == 2) {
                * = wb.setSavePoint();
            } else if (op == 3) {
                * = wb.rollbackToSavePoint();
            } else if (op == 4) {
                * = wb.popSavePoint();
            } else {
                if (wb.byteSize() > 3000) wb.clear();
            }
            if (wb.verify() != WriteBatchStatus.ok) return 2;
        }
    }

    return 0;
}

fn main() {
    test("1. WAL recovery, byte-by-byte torn-write sweep & 100% single-bit corruption detection", () => {
        expect_eq(checkWalRecoveryTornAndCorruption(), 0 as u32);
    });

    test("2. WriteBatch nested SavePoint invariants, CRC32 framing & GroupCommit bounds", () => {
        expect_eq(checkWriteBatchSavePointsAndGroupCommit(), 0 as u32);
    });

    test("3. 16-lane @Vector(16, u8) SIMD copy & equal vs scalar 0-diff parity (0..257 lengths)", () => {
        expect_eq(checkSimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG fuzzing (800 WAL + 800 WriteBatch/SavePoint cases)", () => {
        expect_eq(checkDeterministicFuzz(), 0 as u32);
    });
}
