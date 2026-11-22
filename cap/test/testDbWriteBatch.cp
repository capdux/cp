// Origin: Pure-CP Stdlib Test Suite (lib/db/writeBatch.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Atomic WriteBatch & LSM Group Commit Coordinator:
//   1. put / delete / singleDelete / merge encoding, multi-byte varint32 & zero-copy framed iteration
//   2. Nested SavePoint rollback, popSavePoint commit, stack-depth limit & clear() reuse
//   3. Multi-writer Leader/Follower GroupCommitCoordinator coalescing & contiguous monotonic seq ranges
//   4. Multi-round Group Commit seq continuity, max_writers / max_batch_bytes backpressure & stats
//   5. Integrity guards: truncated frames, 1-bit CRC32 tamper, invalid tag, malformed varint, count mismatch
//   6. Buffer capacity boundary, empty-key guard, invalid GroupCommitConfig & output-buffer guards
//   7. cap.db namespace facade parity verification

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    WRITE_BATCH_HEADER_SIZE,
    WRITE_BATCH_TRAILER_SIZE,
    WRITE_BATCH_MAX_BYTES,
    WRITE_BATCH_MAX_SAVEPOINTS,
    GROUP_COMMIT_MAX_WRITERS,
    GROUP_COMMIT_MAX_BYTES,
    writeBatchCrc32,
    WriteBatchOp,
    WriteBatchStatus,
    WriteBatchRecord,
    verifyWriteBatchRaw,
    verifyWriteBatchFramed,
    WriteBatchIter,
    WriteBatch,
    GroupCommitConfig,
    GroupCommitWriterStatus,
    GroupCommitReceipt,
    GroupCommitFlushResult,
    GroupCommitStats,
    GroupCommitCoordinator,
} from "cap:db";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("WriteBatch: all opcodes, multi-byte varint32, framed CRC32 & zero-copy iteration") {
    let wb = WriteBatch.init();
    expect_true(wb.isEmpty());
    expect_eq(wb.byteSize(), WRITE_BATCH_HEADER_SIZE);
    wb.setSequence(500);
    expect_eq(wb.sequence(), 500 as u64);

    // 160-byte value triggers 2-byte LEB128 varint32
    let big_val: [160]u8 = [0x41; 160];
    expect_eq(wb.put("user:1", big_val[0..]), WriteBatchStatus.ok);
    expect_eq(wb.delete("user:2"), WriteBatchStatus.ok);
    expect_eq(wb.singleDelete("temp:3"), WriteBatchStatus.ok);
    expect_eq(wb.merge("counter:4", "+42"), WriteBatchStatus.ok);
    expect_eq(wb.put("empty_val_key", ""), WriteBatchStatus.ok);

    expect_eq(wb.recordCount(), 5 as u32);
    expect_eq(wb.verify(), WriteBatchStatus.ok);

    let framed: [1024]u8 = [0; 1024];
    let framed_s: []u8 = framed[0..];
    const framed_len = wb.encodeFramed(*framed_s) orelse {
        expect_true(false);
        return;
    };
    expect_eq(framed_len, wb.byteSize() + WRITE_BATCH_TRAILER_SIZE);
    expect_eq(verifyWriteBatchFramed(framed[0..framed_len]), WriteBatchStatus.ok);

    let it = WriteBatchIter.fromFramed(framed[0..framed_len]);
    const r0 = it.next() orelse { expect_true(false); return; };
    expect_eq(r0.op, WriteBatchOp.put);
    expect_eq(r0.seq, 500 as u64);
    expect_true(bytesEq(r0.key, "user:1"));
    expect_true(bytesEq(r0.value, big_val[0..]));

    const r1 = it.next() orelse { expect_true(false); return; };
    expect_eq(r1.op, WriteBatchOp.delete);
    expect_eq(r1.seq, 501 as u64);
    expect_true(bytesEq(r1.key, "user:2"));
    expect_eq(r1.value.len, 0 as usize);

    const r2 = it.next() orelse { expect_true(false); return; };
    expect_eq(r2.op, WriteBatchOp.single_delete);
    expect_eq(r2.seq, 502 as u64);
    expect_true(bytesEq(r2.key, "temp:3"));

    const r3 = it.next() orelse { expect_true(false); return; };
    expect_eq(r3.op, WriteBatchOp.merge);
    expect_eq(r3.seq, 503 as u64);
    expect_true(bytesEq(r3.key, "counter:4"));
    expect_true(bytesEq(r3.value, "+42"));

    const r4 = it.next() orelse { expect_true(false); return; };
    expect_eq(r4.op, WriteBatchOp.put);
    expect_eq(r4.seq, 504 as u64);
    expect_true(bytesEq(r4.key, "empty_val_key"));
    expect_eq(r4.value.len, 0 as usize);

    expect_true(it.next() == null);
    expect_eq(it.status(), WriteBatchStatus.ok);

    wb.clear();
    expect_true(wb.isEmpty());
    expect_eq(wb.sequence(), 0 as u64);
    expect_eq(wb.byteSize(), WRITE_BATCH_HEADER_SIZE);
}

test("WriteBatch: nested SavePoint rollback, popSavePoint & stack limit") {
    let wb = WriteBatch.init();
    expect_eq(wb.rollbackToSavePoint(), WriteBatchStatus.no_savepoint);
    expect_eq(wb.popSavePoint(), WriteBatchStatus.no_savepoint);

    expect_eq(wb.put("base", "v0"), WriteBatchStatus.ok);
    const len_after_base: usize = wb.byteSize();

    // Level 1 savepoint
    expect_eq(wb.setSavePoint(), WriteBatchStatus.ok);
    expect_eq(wb.put("k1", "v1"), WriteBatchStatus.ok);
    const len_after_k1: usize = wb.byteSize();

    // Level 2 savepoint (committed via popSavePoint)
    expect_eq(wb.setSavePoint(), WriteBatchStatus.ok);
    expect_eq(wb.merge("k2", "+1"), WriteBatchStatus.ok);
    expect_eq(wb.popSavePoint(), WriteBatchStatus.ok);
    expect_eq(wb.savePointDepth(), 1 as usize);
    expect_eq(wb.recordCount(), 3 as u32);

    // Level 2b savepoint (rolled back)
    expect_eq(wb.setSavePoint(), WriteBatchStatus.ok);
    expect_eq(wb.delete("k1"), WriteBatchStatus.ok);
    expect_eq(wb.singleDelete("k2"), WriteBatchStatus.ok);
    expect_eq(wb.recordCount(), 5 as u32);
    expect_eq(wb.rollbackToSavePoint(), WriteBatchStatus.ok);
    expect_eq(wb.recordCount(), 3 as u32);
    expect_eq(wb.verify(), WriteBatchStatus.ok);

    // Roll back level 1 -> back to only "base"
    expect_eq(wb.rollbackToSavePoint(), WriteBatchStatus.ok);
    expect_eq(wb.recordCount(), 1 as u32);
    expect_eq(wb.byteSize(), len_after_base);
    expect_true(len_after_k1 > len_after_base);
    expect_eq(wb.verify(), WriteBatchStatus.ok);

    // Verify exact remaining record is "base"
    let it = wb.iter();
    const only = it.next() orelse { expect_true(false); return; };
    expect_true(bytesEq(only.key, "base"));
    expect_true(bytesEq(only.value, "v0"));
    expect_true(it.next() == null);

    // Exhaust savepoint stack (16 allowed, 17th rejected)
    let i: usize = 0;
    while (i < WRITE_BATCH_MAX_SAVEPOINTS) : (i += 1) {
        expect_eq(wb.setSavePoint(), WriteBatchStatus.ok);
    }
    expect_eq(wb.setSavePoint(), WriteBatchStatus.savepoint_stack_full);
}

test("GroupCommit: Leader/Follower coalescing, contiguous seq ranges & single-block decode") {
    let cfg = GroupCommitConfig.defaults();
    cfg.max_writers = 8;
    cfg.max_batch_bytes = 4096;
    cfg.initial_seq = 100;

    let coord = GroupCommitCoordinator.init(cfg) orelse {
        expect_true(false);
        return;
    };

    // Writer 10 (Leader): 2 ops
    let wb1 = WriteBatch.init();
    * = wb1.put("acct:a", "100");
    * = wb1.put("acct:b", "200");

    // Writer 20 (Follower 1): 1 op
    let wb2 = WriteBatch.init();
    * = wb2.delete("session:expired");

    // Writer 30 (Follower 2): 3 ops
    let wb3 = WriteBatch.init();
    * = wb3.merge("hits:home", "+1");
    * = wb3.singleDelete("cache:tmp");
    * = wb3.put("status", "ok");

    expect_eq(coord.enqueueBatch(10, wb1), GroupCommitWriterStatus.committed);
    expect_eq(coord.enqueueBatch(20, wb2), GroupCommitWriterStatus.committed);
    expect_eq(coord.enqueueBatch(30, wb3), GroupCommitWriterStatus.committed);
    expect_eq(coord.pendingWriters(), 3 as usize);

    let framed: [4096]u8 = [0; 4096];
    let framed_s: []u8 = framed[0..];
    let receipts: [8]GroupCommitReceipt = [GroupCommitReceipt{}; 8];
    let receipts_s: []GroupCommitReceipt = receipts[0..];

    const res = coord.flush(*framed_s, *receipts_s) orelse {
        expect_true(false);
        return;
    };
    expect_eq(res.leader_writer_id, 10 as u32);
    expect_eq(res.writer_count, 3 as usize);
    expect_eq(res.total_ops, 6 as u32);
    expect_eq(res.start_seq, 100 as u64);
    expect_eq(res.end_seq, 106 as u64);
    expect_eq(coord.nextSeq(), 106 as u64);
    expect_eq(coord.pendingWriters(), 0 as usize);

    // Verify per-writer receipts: Leader + Followers with contiguous seq intervals
    expect_eq(receipts[0].writer_id, 10 as u32);
    expect_true(receipts[0].is_leader);
    expect_eq(receipts[0].start_seq, 100 as u64);
    expect_eq(receipts[0].end_seq, 102 as u64);
    expect_eq(receipts[0].op_count, 2 as u32);

    expect_eq(receipts[1].writer_id, 20 as u32);
    expect_eq(receipts[1].is_leader, false);
    expect_eq(receipts[1].start_seq, 102 as u64);
    expect_eq(receipts[1].end_seq, 103 as u64);
    expect_eq(receipts[1].op_count, 1 as u32);

    expect_eq(receipts[2].writer_id, 30 as u32);
    expect_eq(receipts[2].is_leader, false);
    expect_eq(receipts[2].start_seq, 103 as u64);
    expect_eq(receipts[2].end_seq, 106 as u64);
    expect_eq(receipts[2].op_count, 3 as u32);

    // Verify coalesced disk block passes framed integrity & decodes all 6 ops with exact seqs
    const block = framed[0..res.merged_framed_bytes];
    expect_eq(verifyWriteBatchFramed(block), WriteBatchStatus.ok);

    let it = WriteBatchIter.fromFramed(block);
    let expected_seq: u64 = 100;
    let decoded_count: usize = 0;
    while (rec := it.next()) {
        expect_eq(rec.seq, expected_seq);
        if (decoded_count == 0) expect_true(bytesEq(rec.key, "acct:a"));
        if (decoded_count == 2) {
            expect_eq(rec.op, WriteBatchOp.delete);
            expect_true(bytesEq(rec.key, "session:expired"));
        }
        if (decoded_count == 5) {
            expect_eq(rec.op, WriteBatchOp.put);
            expect_true(bytesEq(rec.key, "status"));
            expect_true(bytesEq(rec.value, "ok"));
        }
        expected_seq += 1;
        decoded_count += 1;
    }
    expect_eq(decoded_count, 6 as usize);
    expect_eq(it.status(), WriteBatchStatus.ok);
}

test("GroupCommit: multi-round continuity, max_writers/max_batch_bytes backpressure & stats") {
    let cfg = GroupCommitConfig.defaults();
    cfg.max_writers = 2;
    cfg.max_batch_bytes = 64; // 12B hdr + payload + 4B trailer <= 64 => max 48B payload
    cfg.initial_seq = 1;

    let coord = GroupCommitCoordinator.init(cfg) orelse {
        expect_true(false);
        return;
    };

    // Empty batch is a clean noop (not rejected)
    let empty_wb = WriteBatch.init();
    expect_eq(coord.enqueueBatch(1, empty_wb), GroupCommitWriterStatus.empty_noop);
    expect_eq(coord.pendingWriters(), 0 as usize);

    let wb_a = WriteBatch.init();
    * = wb_a.put("k1", "val1"); // payload = 1 + 1 + 2 + 1 + 4 = 9B
    let wb_b = WriteBatch.init();
    * = wb_b.put("k2", "val2"); // payload = 9B
    let wb_c = WriteBatch.init();
    * = wb_c.put("k3", "val3");

    expect_eq(coord.enqueueBatch(1, wb_a), GroupCommitWriterStatus.committed);
    expect_eq(coord.enqueueBatch(2, wb_b), GroupCommitWriterStatus.committed);
    // 3rd writer hits max_writers = 2
    expect_eq(coord.enqueueBatch(3, wb_c), GroupCommitWriterStatus.rejected_queue_full);

    let framed: [256]u8 = [0; 256];
    let framed_s: []u8 = framed[0..];
    let receipts: [4]GroupCommitReceipt = [GroupCommitReceipt{}; 4];
    let receipts_s: []GroupCommitReceipt = receipts[0..];

    const f1 = coord.flush(*framed_s, *receipts_s) orelse { expect_true(false); return; };
    expect_eq(f1.start_seq, 1 as u64);
    expect_eq(f1.end_seq, 3 as u64);

    // Round 2: oversized batch (> 48B payload) rejected cleanly
    let big_wb = WriteBatch.init();
    let big_v: [60]u8 = [0x5A; 60];
    * = big_wb.put("oversize", big_v[0..]);
    expect_eq(coord.enqueueBatch(4, big_wb), GroupCommitWriterStatus.rejected_oversize);

    // Enqueue wb_c in round 2 -> seq continues monotonically at 3..4
    expect_eq(coord.enqueueBatch(3, wb_c), GroupCommitWriterStatus.committed);
    const f2 = coord.flush(*framed_s, *receipts_s) orelse { expect_true(false); return; };
    expect_eq(f2.leader_writer_id, 3 as u32);
    expect_eq(f2.start_seq, 3 as u64);
    expect_eq(f2.end_seq, 4 as u64);

    const st = coord.stats();
    expect_eq(st.flushes, 2 as u64);
    expect_eq(st.total_writers_committed, 3 as u64);
    expect_eq(st.total_ops_committed, 3 as u64);
    expect_eq(st.next_seq, 4 as u64);
    expect_eq(st.rejected_count, 2 as u64);
    expect_true(st.total_bytes_flushed == (f1.merged_framed_bytes + f2.merged_framed_bytes) as u64);
}

test("WriteBatch: integrity guards catch truncation, CRC tamper, bad tag, varint overflow & count mismatch") {
    let wb = WriteBatch.init();
    * = wb.put("alpha", "beta");
    * = wb.delete("gamma");

    let framed: [128]u8 = [0; 128];
    let framed_s: []u8 = framed[0..];
    const n = wb.encodeFramed(*framed_s) orelse { expect_true(false); return; };

    // 1. Truncated raw header (< 12B) and truncated framed (< 16B)
    expect_eq(verifyWriteBatchRaw(framed[0..8]), WriteBatchStatus.truncated);
    expect_eq(verifyWriteBatchFramed(framed[0..14]), WriteBatchStatus.truncated);

    // 2. 1-bit tamper in payload -> checksum_mismatch
    framed[15] ^= 0x01;
    expect_eq(verifyWriteBatchFramed(framed[0..n]), WriteBatchStatus.checksum_mismatch);
    let bad_it = WriteBatchIter.fromFramed(framed[0..n]);
    expect_true(bad_it.next() == null);
    expect_eq(bad_it.status(), WriteBatchStatus.checksum_mismatch);
    framed[15] ^= 0x01; // restore
    expect_eq(verifyWriteBatchFramed(framed[0..n]), WriteBatchStatus.ok);

    // 3. Invalid opcode tag in raw batch
    let bad_tag_raw: [16]u8 = [0; 16];
    bad_tag_raw[8] = 1; // count = 1
    bad_tag_raw[12] = 0x7F; // invalid op tag
    expect_eq(verifyWriteBatchRaw(bad_tag_raw[0..13]), WriteBatchStatus.invalid_tag);

    // 4. Malformed 5-byte overflowing varint32
    let bad_varint: [20]u8 = [0; 20];
    bad_varint[8] = 1;
    bad_varint[12] = 2; // delete
    bad_varint[13] = 0x80;
    bad_varint[14] = 0x80;
    bad_varint[15] = 0x80;
    bad_varint[16] = 0x80;
    bad_varint[17] = 0xF0; // upper 4 bits set -> overflow
    expect_eq(verifyWriteBatchRaw(bad_varint[0..18]), WriteBatchStatus.malformed_varint);

    // 5. Truncated key payload
    let trunc_key: [16]u8 = [0; 16];
    trunc_key[8] = 1;
    trunc_key[12] = 2; // delete
    trunc_key[13] = 10; // claims 10-byte key, only 2 bytes follow
    trunc_key[14] = 0x61;
    trunc_key[15] = 0x62;
    expect_eq(verifyWriteBatchRaw(trunc_key[0..16]), WriteBatchStatus.truncated);

    // 6. Header count mismatch (claims 5 records, actually 2)
    let raw_copy: [128]u8 = [0; 128];
    const raw_n: usize = wb.byteSize();
    const raw_src = wb.rawBytes();
    let i: usize = 0;
    while (i < raw_n) : (i += 1) {
        raw_copy[i] = raw_src[i];
    }
    raw_copy[8] = 5;
    expect_eq(verifyWriteBatchRaw(raw_copy[0..raw_n]), WriteBatchStatus.count_mismatch);

    // 7. GroupCommitCoordinator rejects malformed raw batch
    let coord = GroupCommitCoordinator.init(GroupCommitConfig.defaults()) orelse {
        expect_true(false);
        return;
    };
    expect_eq(coord.enqueueRaw(99, raw_copy[0..raw_n]), GroupCommitWriterStatus.rejected_invalid);
    expect_eq(coord.stats().rejected_count, 1 as u64);
}

test("WriteBatch & GroupCommit: capacity limits, empty-key guard, invalid configs & cap.db facade") {
    let wb = WriteBatch.init();
    expect_eq(wb.put("", "val"), WriteBatchStatus.empty_key);
    expect_eq(wb.delete(""), WriteBatchStatus.empty_key);
    expect_eq(wb.singleDelete(""), WriteBatchStatus.empty_key);
    expect_eq(wb.merge("", "op"), WriteBatchStatus.empty_key);

    // Fill inline buffer until buffer_full, verify batch remains structurally intact
    let chunk: [256]u8 = [0x42; 256];
    let inserted: u32 = 0;
    while (true) {
        const st = wb.put("chunk_key", chunk[0..]);
        if (st == WriteBatchStatus.buffer_full) break;
        expect_eq(st, WriteBatchStatus.ok);
        inserted += 1;
    }
    expect_true(inserted >= 10);
    expect_eq(wb.recordCount(), inserted);
    expect_eq(wb.verify(), WriteBatchStatus.ok);

    // Output buffer too small for encodeFramed
    let tiny_out: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny_out[0..];
    expect_true(wb.encodeFramed(*tiny_s) == null);

    // Invalid GroupCommitConfig guards
    let c1 = GroupCommitConfig.defaults();
    c1.max_writers = 0;
    expect_true(GroupCommitCoordinator.init(c1) == null);

    let c2 = GroupCommitConfig.defaults();
    c2.max_writers = GROUP_COMMIT_MAX_WRITERS + 1;
    expect_true(GroupCommitCoordinator.init(c2) == null);

    let c3 = GroupCommitConfig.defaults();
    c3.max_batch_bytes = 10;
    expect_true(GroupCommitCoordinator.init(c3) == null);

    let c4 = GroupCommitConfig.defaults();
    c4.max_batch_bytes = GROUP_COMMIT_MAX_BYTES + 1;
    expect_true(GroupCommitCoordinator.init(c4) == null);

    let c5 = GroupCommitConfig.defaults();
    c5.initial_seq = 0;
    expect_true(GroupCommitCoordinator.init(c5) == null);

    // Empty flush & small output buffers return null
    let coord = GroupCommitCoordinator.init(GroupCommitConfig.defaults()) orelse {
        expect_true(false);
        return;
    };
    let out_f: [64]u8 = [0; 64];
    let out_f_s: []u8 = out_f[0..];
    let out_r: [2]GroupCommitReceipt = [GroupCommitReceipt{}; 2];
    let out_r_s: []GroupCommitReceipt = out_r[0..];
    expect_true(coord.flush(*out_f_s, *out_r_s) == null);

    let small_wb = WriteBatch.init();
    * = small_wb.put("k", "v");
    expect_eq(coord.enqueueBatch(1, small_wb), GroupCommitWriterStatus.committed);
    expect_true(coord.flush(*tiny_s, *out_r_s) == null);
    let empty_r_s: []GroupCommitReceipt = out_r[0..0];
    expect_true(coord.flush(*out_f_s, *empty_r_s) == null);

    // Verify cap.db namespace facade parity
    expect_eq(cap.db.WRITE_BATCH_HEADER_SIZE, WRITE_BATCH_HEADER_SIZE);
    expect_eq(cap.db.WRITE_BATCH_MAX_BYTES, WRITE_BATCH_MAX_BYTES);
    expect_eq(cap.db.GROUP_COMMIT_MAX_WRITERS, GROUP_COMMIT_MAX_WRITERS);
    expect_eq(cap.db.writeBatchCrc32("123456789"), writeBatchCrc32("123456789"));
    // Standard IEEE CRC32 check value for "123456789" is 0xCBF43926
    expect_eq(writeBatchCrc32("123456789"), 0xCBF43926 as u32);
}
