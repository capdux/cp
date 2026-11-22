// Deep Production Test Suite for lib/db/lsmEngine.cp (Wave 27 Round 2)
// Verifies: CRC32 WAL framing + crash tail truncation + tamper detection,
// MVCC ordered MemTable snapshot isolation & range scan,
// Block SSTable prefix compression + Index binary search + Bloom Filter FPR (<2%),
// 3-way K-Way Leveled Compaction (dedup + snapshot preservation + tombstone GC),
// 320+ KV end-to-end write -> flush -> compact -> crash recovery loop, and boundary guards.

import { test, expect_eq, expect_true } from "cap:test";
import {
    LSM_WAL_MAGIC,
    LSM_SST_MAGIC,
    LSM_CURSOR_KEY_STRIDE,
    LsmKvError,
    compareBytes,
    LsmWalOp,
    LsmWalRecordView,
    LsmWalRecoveryStats,
    LsmWalCodec,
    LsmMemEntryMeta,
    LsmMemBuffers,
    LsmLookupStatus,
    LsmMemGetResult,
    LsmMemScanItem,
    LsmMemTable,
    SstBuilderConfig,
    SstBuildStats,
    SstLookupResult,
    sstBloomWorkMinBytes,
    SstBuilder,
    SstReader,
    SstCursor,
    SstCompactionConfig,
    SstCompactionStats,
    sstCompactionKeyBufMinBytes,
    compactSstables,
    LsmDbEngine,
} from "../../lib/db/index.cp";

fn formatNumKey(prefix: []const u8, num: usize, out: *[]u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len && pos < out.len) : (i += 1) {
        out[pos] = prefix[i];
        pos += 1;
    }
    // Fixed 4-digit zero-padded decimal so lexicographical order matches numeric order
    if (pos + 4 <= out.len) {
        out[pos + 0] = @as(u8, @intCast('0' + ((num / 1000) % 10)));
        out[pos + 1] = @as(u8, @intCast('0' + ((num / 100) % 10)));
        out[pos + 2] = @as(u8, @intCast('0' + ((num / 10) % 10)));
        out[pos + 3] = @as(u8, @intCast('0' + (num % 10)));
        pos += 4;
    }
    return pos;
}

// ============================================================================
// Test 1: WAL Put/Delete Framing, Crash Tail Truncation & CRC32 Tamper Guard
// ============================================================================
test("LsmWalCodec: Put/Delete framing, torn tail truncation & single-bit tamper guard") {
    let wal: [1024]u8 = [0; 1024];
    let pos: usize = 0;

    let tail1: []u8 = wal[pos..];
    pos += LsmWalCodec.encodePut("user:001", "alice", 10, *tail1) catch { expect_true(false); return; };
    let tail2: []u8 = wal[pos..];
    pos += LsmWalCodec.encodePut("user:002", "bob", 11, *tail2) catch { expect_true(false); return; };
    let tail3: []u8 = wal[pos..];
    pos += LsmWalCodec.encodeDelete("user:001", 12, *tail3) catch { expect_true(false); return; };
    const valid_end: usize = pos;

    // Simulate a torn 9-byte partial write at the tail
    let k: usize = 0;
    while (k < 9) : (k += 1) {
        wal[pos + k] = @as(u8, @intCast(0xA0 + k));
    }
    const torn_len: usize = valid_end + 9;

    let recs: [8]LsmWalRecordView = [LsmWalRecordView{}; 8];
    let recs_s: []LsmWalRecordView = recs[0..];
    const rstats: LsmWalRecoveryStats = LsmWalCodec.recoverLog(wal[0..torn_len], *recs_s);
    expect_eq(rstats.recovered_records, 3);
    expect_eq(rstats.valid_bytes, valid_end);
    expect_eq(rstats.corrupt_tail_bytes, 9);
    expect_eq(rstats.max_seq, 12);
    expect_true(rstats.truncated_tail);

    expect_true(recs[0].op == LsmWalOp.put);
    expect_eq(recs[0].seq, 10);
    expect_eq(compareBytes(recs[0].key, "user:001"), 0);
    expect_eq(compareBytes(recs[0].value, "alice"), 0);
    expect_true(recs[2].op == LsmWalOp.delete);
    expect_eq(recs[2].seq, 12);

    // Flip 1 bit in frame 0 payload -> decodeFrameAt must return ChecksumMismatch
    wal[25] ^= 0x01;
    let got_crc_err: bool = false;
    * = LsmWalCodec.decodeFrameAt(wal[0..valid_end], 0) catch |e| {
        if (e == LsmKvError.ChecksumMismatch) got_crc_err = true;
    };
    expect_true(got_crc_err);
    * = LSM_WAL_MAGIC;
}

// ============================================================================
// Test 2: Ordered MVCC MemTable Snapshot Isolation & Range Scan
// ============================================================================
test("LsmMemTable: MVCC snapshot isolation, tombstone delete & ordered range scan") {
    let entries: [64]LsmMemEntryMeta = [LsmMemEntryMeta{}; 64];
    let order: [64]u32 = [0; 64];
    let arena: [4096]u8 = [0; 4096];
    const bufs: LsmMemBuffers = LsmMemBuffers{
        entries: entries[0..],
        order: order[0..],
        arena: arena[0..],
    };

    let mem: LsmMemTable = LsmMemTable.init(bufs) catch { expect_true(false); return; };

    // Out-of-order key inserts with multi-version MVCC sequence numbers
    * = mem.put("k03", "v3_old", 1) catch { expect_true(false); return; };
    * = mem.put("k01", "v1_init", 2) catch { expect_true(false); return; };
    * = mem.put("k05", "v5_init", 3) catch { expect_true(false); return; };
    * = mem.put("k02", "v2_init", 4) catch { expect_true(false); return; };
    * = mem.put("k03", "v3_new", 5) catch { expect_true(false); return; };
    * = mem.delete("k02", 6) catch { expect_true(false); return; };
    * = mem.put("k04", "v4_future", 7) catch { expect_true(false); return; };

    // Snapshot @ seq=4: k03 sees "v3_old", k02 sees "v2_init", k04 not yet visible
    const r_k3_s4: LsmMemGetResult = mem.getAt("k03", 4);
    expect_true(r_k3_s4.status == LsmLookupStatus.found);
    expect_eq(compareBytes(r_k3_s4.value, "v3_old"), 0);
    expect_eq(r_k3_s4.seq, 1);

    const r_k2_s4: LsmMemGetResult = mem.getAt("k02", 4);
    expect_true(r_k2_s4.status == LsmLookupStatus.found);
    expect_eq(compareBytes(r_k2_s4.value, "v2_init"), 0);

    const r_k4_s4: LsmMemGetResult = mem.getAt("k04", 4);
    expect_true(r_k4_s4.status == LsmLookupStatus.not_found);

    // Snapshot @ seq=7: k03 sees "v3_new", k02 is deleted, k04 is visible
    const r_k3_s7: LsmMemGetResult = mem.getAt("k03", 7);
    expect_true(r_k3_s7.status == LsmLookupStatus.found);
    expect_eq(compareBytes(r_k3_s7.value, "v3_new"), 0);

    const r_k2_s7: LsmMemGetResult = mem.getAt("k02", 7);
    expect_true(r_k2_s7.status == LsmLookupStatus.deleted);
    expect_eq(r_k2_s7.seq, 6);

    // Range scan [k01, k05) @ seq=7 -> returns k01, k03(v3_new), k04 (skips deleted k02 and exclusive k05)
    let items: [8]LsmMemScanItem = [LsmMemScanItem{}; 8];
    let items_s: []LsmMemScanItem = items[0..];
    const sc: usize = mem.scanRange("k01", "k05", 7, *items_s);
    expect_eq(sc, 3);
    expect_eq(compareBytes(items[0].key, "k01"), 0);
    expect_eq(compareBytes(items[1].key, "k03"), 0);
    expect_eq(compareBytes(items[1].value, "v3_new"), 0);
    expect_eq(compareBytes(items[2].key, "k04"), 0);

    // Range scan [k01, k05) @ seq=4 -> k02 still visible, k03 is v3_old, k04 invisible
    const sc4: usize = mem.scanRange("k01", "k05", 4, *items_s);
    expect_eq(sc4, 3);
    expect_eq(compareBytes(items[0].key, "k01"), 0);
    expect_eq(compareBytes(items[1].key, "k02"), 0);
    expect_eq(compareBytes(items[2].key, "k03"), 0);
    expect_eq(compareBytes(items[2].value, "v3_old"), 0);
}

// ============================================================================
// Test 3: Block SSTable Prefix Compression, Multi-Block Index Search & CRC32 Guard
// ============================================================================
test("SstBuilder & SstReader: multi-block prefix compression, binary search & CRC32 guard") {
    const cfg: SstBuilderConfig = SstBuilderConfig{
        block_target_bytes: 128, // force multiple Data Blocks
        bloom_bits_per_key: 10,
        bloom_num_hashes: 7,
        expected_keys: 64,
    };
    let sst_buf: [16384]u8 = [0; 16384];
    let idx_work: [4096]u8 = [0; 4096];
    let blm_work: [256]u8 = [0; 256];
    let sst_s: []u8 = sst_buf[0..];
    let idx_s: []u8 = idx_work[0..];
    let blm_s: []u8 = blm_work[0..];

    let builder: SstBuilder = SstBuilder.init(cfg, *sst_s, *idx_s, *blm_s) catch {
        expect_true(false);
        return;
    };

    let kbuf: [32]u8 = [0; 32];
    let vbuf: [32]u8 = [0; 32];
    let i: usize = 0;
    while (i < 60) : (i += 1) {
        let ks: []u8 = kbuf[0..];
        let vs: []u8 = vbuf[0..];
        const kl: usize = formatNumKey("account/user/", i, *ks);
        const vl: usize = formatNumKey("payload_", i * 3, *vs);
        * = builder.addEntry(kbuf[0..kl], vbuf[0..vl], @as(u64, @intCast(i + 1)), false) catch {
            expect_true(false);
            return;
        };
    }

    // Out-of-order key insertion must fail with InvalidKeyOrder
    let got_order_err: bool = false;
    * = builder.addEntry("account/user/0000", "bad", 99, false) catch |e| {
        if (e == LsmKvError.InvalidKeyOrder) got_order_err = true;
    };
    expect_true(got_order_err);

    const bstats: SstBuildStats = builder.finish() catch { expect_true(false); return; };
    expect_eq(bstats.entry_count, 60);
    expect_true(bstats.block_count >= 5);

    const reader: SstReader = SstReader.open(sst_buf[0..bstats.total_bytes]) catch {
        expect_true(false);
        return;
    };

    // Verify every inserted key via Index Block binary search + prefix decompression
    let scratch_k: [256]u8 = [0; 256];
    let sk_s: []u8 = scratch_k[0..];
    i = 0;
    while (i < 60) : (i += 1) {
        let ks: []u8 = kbuf[0..];
        let vs: []u8 = vbuf[0..];
        const kl: usize = formatNumKey("account/user/", i, *ks);
        const vl: usize = formatNumKey("payload_", i * 3, *vs);
        const res: SstLookupResult = reader.getAt(kbuf[0..kl], 1000, *sk_s) catch {
            expect_true(false);
            return;
        };
        expect_true(res.status == LsmLookupStatus.found);
        expect_eq(compareBytes(res.value, vbuf[0..vl]), 0);
    }

    // Corrupt 1 byte inside first Data Block -> getAt must fail with ChecksumMismatch
    sst_buf[10] ^= 0x80;
    let got_blk_crc: bool = false;
    * = reader.getAt("account/user/0000", 1000, *sk_s) catch |e| {
        if (e == LsmKvError.ChecksumMismatch) got_blk_crc = true;
    };
    expect_true(got_blk_crc);
    sst_buf[10] ^= 0x80; // restore

    // Corrupt 1 byte inside Footer -> SstReader.open must fail with ChecksumMismatch
    sst_buf[bstats.total_bytes - 10] ^= 0x01;
    let got_ftr_crc: bool = false;
    * = SstReader.open(sst_buf[0..bstats.total_bytes]) catch |e| {
        if (e == LsmKvError.ChecksumMismatch) got_ftr_crc = true;
    };
    expect_true(got_ftr_crc);
    * = LSM_SST_MAGIC;
}

// ============================================================================
// Test 4: Embedded Bloom Filter Empirical False-Positive Rate (< 2%)
// ============================================================================
test("SstReader: embedded Bloom Filter zero false negatives & FPR < 2%") {
    const N_KEYS: usize = 256;
    const cfg: SstBuilderConfig = SstBuilderConfig{
        block_target_bytes: 512,
        bloom_bits_per_key: 10,
        bloom_num_hashes: 7,
        expected_keys: N_KEYS,
    };
    expect_true(sstBloomWorkMinBytes(cfg) >= 320);

    let sst_buf: [32768]u8 = [0; 32768];
    let idx_work: [4096]u8 = [0; 4096];
    let blm_work: [512]u8 = [0; 512];
    let sst_s: []u8 = sst_buf[0..];
    let idx_s: []u8 = idx_work[0..];
    let blm_s: []u8 = blm_work[0..];

    let builder: SstBuilder = SstBuilder.init(cfg, *sst_s, *idx_s, *blm_s) catch {
        expect_true(false);
        return;
    };

    let kbuf: [32]u8 = [0; 32];
    let i: usize = 0;
    while (i < N_KEYS) : (i += 1) {
        let ks: []u8 = kbuf[0..];
        const kl: usize = formatNumKey("present_key_", i, *ks);
        * = builder.addEntry(kbuf[0..kl], "val", @as(u64, @intCast(i + 1)), false) catch {
            expect_true(false);
            return;
        };
    }
    const bstats: SstBuildStats = builder.finish() catch { expect_true(false); return; };
    const reader: SstReader = SstReader.open(sst_buf[0..bstats.total_bytes]) catch {
        expect_true(false);
        return;
    };

    // Zero false negatives across all 256 inserted keys
    i = 0;
    while (i < N_KEYS) : (i += 1) {
        let ks: []u8 = kbuf[0..];
        const kl: usize = formatNumKey("present_key_", i, *ks);
        expect_true(reader.mayContain(kbuf[0..kl]));
    }

    // Measure false positive rate across 1,000 absent keys (must be < 20, i.e., < 2.0%)
    let false_positives: usize = 0;
    let bloom_rejects: usize = 0;
    let scratch_k: [256]u8 = [0; 256];
    let sk_s: []u8 = scratch_k[0..];
    i = 0;
    while (i < 1000) : (i += 1) {
        let ks: []u8 = kbuf[0..];
        const kl: usize = formatNumKey("absent_probe_", i, *ks);
        if (reader.mayContain(kbuf[0..kl])) {
            false_positives += 1;
        }
        const res: SstLookupResult = reader.getAt(kbuf[0..kl], 9999, *sk_s) catch {
            expect_true(false);
            return;
        };
        expect_true(res.status == LsmLookupStatus.not_found);
        if (res.bloom_rejected) bloom_rejects += 1;
    }
    expect_true(false_positives < 20); // strictly < 2% FPR
    expect_eq(bloom_rejects, 1000 - false_positives);
}

// ============================================================================
// Test 5: 3-Way K-Way Merge Compaction (Dedup, Snapshot Retention & Tombstone GC)
// ============================================================================
test("compactSstables: 3-way K-Way merge dedup, snapshot protection & tombstone GC") {
    const bcfg: SstBuilderConfig = SstBuilderConfig{
        block_target_bytes: 256,
        bloom_bits_per_key: 10,
        bloom_num_hashes: 7,
        expected_keys: 32,
    };

    // Build 3 SSTables:
    //   SST0 (oldest, seq 1..4): a=1(1), b=1(2), c=1(3), d=1(4)
    //   SST1 (middle, seq 5..7): b=2(5), c=DEL(6), e=1(7)
    //   SST2 (newest, seq 8..10): a=2(8), d=DEL(9), f=1(10)
    let sst0: [4096]u8 = [0; 4096];
    let sst1: [4096]u8 = [0; 4096];
    let sst2: [4096]u8 = [0; 4096];
    let idx_w: [1024]u8 = [0; 1024];
    let blm_w: [256]u8 = [0; 256];

    let s0_s: []u8 = sst0[0..];
    let iw_s: []u8 = idx_w[0..];
    let bw_s: []u8 = blm_w[0..];
    let b0: SstBuilder = SstBuilder.init(bcfg, *s0_s, *iw_s, *bw_s) catch { expect_true(false); return; };
    * = b0.addEntry("a", "v_a1", 1, false) catch { expect_true(false); return; };
    * = b0.addEntry("b", "v_b1", 2, false) catch { expect_true(false); return; };
    * = b0.addEntry("c", "v_c1", 3, false) catch { expect_true(false); return; };
    * = b0.addEntry("d", "v_d1", 4, false) catch { expect_true(false); return; };
    const st0: SstBuildStats = b0.finish() catch { expect_true(false); return; };

    let s1_s: []u8 = sst1[0..];
    let b1: SstBuilder = SstBuilder.init(bcfg, *s1_s, *iw_s, *bw_s) catch { expect_true(false); return; };
    * = b1.addEntry("b", "v_b2", 5, false) catch { expect_true(false); return; };
    * = b1.addEntry("c", "", 6, true) catch { expect_true(false); return; };
    * = b1.addEntry("e", "v_e1", 7, false) catch { expect_true(false); return; };
    const st1: SstBuildStats = b1.finish() catch { expect_true(false); return; };

    let s2_s: []u8 = sst2[0..];
    let b2: SstBuilder = SstBuilder.init(bcfg, *s2_s, *iw_s, *bw_s) catch { expect_true(false); return; };
    * = b2.addEntry("a", "v_a2", 8, false) catch { expect_true(false); return; };
    * = b2.addEntry("d", "", 9, true) catch { expect_true(false); return; };
    * = b2.addEntry("f", "v_f1", 10, false) catch { expect_true(false); return; };
    const st2: SstBuildStats = b2.finish() catch { expect_true(false); return; };

    const inputs: [3][]const u8 = [
        sst2[0..st2.total_bytes],
        sst1[0..st1.total_bytes],
        sst0[0..st0.total_bytes],
    ];

    expect_eq(sstCompactionKeyBufMinBytes(3), 3 * LSM_CURSOR_KEY_STRIDE);
    let cursors: [3]SstCursor = [SstCursor{}; 3];
    let ckeys: [768]u8 = [0; 768];
    let out_snap: [8192]u8 = [0; 8192];
    let curs_s: []SstCursor = cursors[0..];
    let ck_s: []u8 = ckeys[0..];
    let os_s: []u8 = out_snap[0..];

    // Case A: Active snapshot @ seq=4, non-bottommost -> preserves a(1) & a(8), d(4) & d(DEL@9)
    const cstats_snap: SstCompactionStats = compactSstables(
        inputs[0..],
        SstCompactionConfig{
            drop_tombstones: false,
            min_active_snapshot_seq: 4,
            builder_cfg: bcfg,
        },
        *curs_s,
        *ck_s,
        *os_s,
        *iw_s,
        *bw_s,
    ) catch { expect_true(false); return; };

    expect_eq(cstats_snap.input_entries, 10);
    expect_eq(cstats_snap.output_entries, 10); // all versions needed for snapshot=4 or newer
    const r_snap: SstReader = SstReader.open(out_snap[0..cstats_snap.output_bytes]) catch {
        expect_true(false);
        return;
    };
    let sk: [256]u8 = [0; 256];
    let sk_s: []u8 = sk[0..];
    const ra_old: SstLookupResult = r_snap.getAt("a", 4, *sk_s) catch { expect_true(false); return; };
    expect_true(ra_old.status == LsmLookupStatus.found);
    expect_eq(compareBytes(ra_old.value, "v_a1"), 0);
    const ra_new: SstLookupResult = r_snap.getAt("a", 10, *sk_s) catch { expect_true(false); return; };
    expect_true(ra_new.status == LsmLookupStatus.found);
    expect_eq(compareBytes(ra_new.value, "v_a2"), 0);

    // Case B: Bottommost compaction (no old snapshot, drop_tombstones = true)
    // Survivors: a=v_a2(8), b=v_b2(5), e=v_e1(7), f=v_f1(10) => 4 entries!
    // Dropped overwritten: a(1), b(2), c(3), d(4) => 4; Dropped tombstones: c(6), d(9) => 2!
    let out_bot: [8192]u8 = [0; 8192];
    let ob_s: []u8 = out_bot[0..];
    const cstats_bot: SstCompactionStats = compactSstables(
        inputs[0..],
        SstCompactionConfig{
            drop_tombstones: true,
            min_active_snapshot_seq: 0xFFFF_FFFF_FFFF_FFFF,
            builder_cfg: bcfg,
        },
        *curs_s,
        *ck_s,
        *ob_s,
        *iw_s,
        *bw_s,
    ) catch { expect_true(false); return; };

    expect_eq(cstats_bot.input_entries, 10);
    expect_eq(cstats_bot.output_entries, 4);
    expect_eq(cstats_bot.dropped_overwritten, 4);
    expect_eq(cstats_bot.dropped_tombstones, 2);
    expect_true(cstats_bot.output_bytes < cstats_bot.input_bytes);

    const r_bot: SstReader = SstReader.open(out_bot[0..cstats_bot.output_bytes]) catch {
        expect_true(false);
        return;
    };
    const rc: SstLookupResult = r_bot.getAt("c", 100, *sk_s) catch { expect_true(false); return; };
    expect_true(rc.status == LsmLookupStatus.not_found);
    const rb: SstLookupResult = r_bot.getAt("b", 100, *sk_s) catch { expect_true(false); return; };
    expect_true(rb.status == LsmLookupStatus.found);
    expect_eq(compareBytes(rb.value, "v_b2"), 0);
}

// ============================================================================
// Test 6: 320+ KV End-to-End Write -> Multi-Flush -> Compaction -> Crash Recovery
// ============================================================================
test("LsmDbEngine: 320+ KV end-to-end write, 3x flush, compaction & crash WAL recovery") {
    let entries: [256]LsmMemEntryMeta = [LsmMemEntryMeta{}; 256];
    let order: [256]u32 = [0; 256];
    let arena: [16384]u8 = [0; 16384];
    let wal_buf: [16384]u8 = [0; 16384];
    let wal_s: []u8 = wal_buf[0..];

    let engine: LsmDbEngine = LsmDbEngine.init(
        LsmMemBuffers{ entries: entries[0..], order: order[0..], arena: arena[0..] },
        *wal_s,
    ) catch { expect_true(false); return; };

    const bcfg: SstBuilderConfig = SstBuilderConfig{
        block_target_bytes: 512,
        bloom_bits_per_key: 10,
        bloom_num_hashes: 7,
        expected_keys: 320,
    };
    let sst_a: [32768]u8 = [0; 32768];
    let sst_b: [32768]u8 = [0; 32768];
    let sst_c: [32768]u8 = [0; 32768];
    let sst_merged: [65536]u8 = [0; 65536];
    let idx_w: [4096]u8 = [0; 4096];
    let blm_w: [1024]u8 = [0; 1024];
    let iw_s: []u8 = idx_w[0..];
    let bw_s: []u8 = blm_w[0..];

    let kbuf: [32]u8 = [0; 32];
    let vbuf: [32]u8 = [0; 32];

    // Batch 1: insert keys 0..159 -> flush to sst_a
    let i: usize = 0;
    while (i < 160) : (i += 1) {
        let ks: []u8 = kbuf[0..];
        let vs: []u8 = vbuf[0..];
        const kl: usize = formatNumKey("row:", i, *ks);
        const vl: usize = formatNumKey("v1_", i, *vs);
        * = engine.put(kbuf[0..kl], vbuf[0..vl]) catch { expect_true(false); return; };
    }
    let sa_s: []u8 = sst_a[0..];
    const sta: SstBuildStats = engine.flushMemTableToSst(bcfg, *sa_s, *iw_s, *bw_s) catch {
        expect_true(false);
        return;
    };

    // Batch 2: insert keys 160..319 -> flush to sst_b
    i = 160;
    while (i < 320) : (i += 1) {
        let ks: []u8 = kbuf[0..];
        let vs: []u8 = vbuf[0..];
        const kl: usize = formatNumKey("row:", i, *ks);
        const vl: usize = formatNumKey("v1_", i, *vs);
        * = engine.put(kbuf[0..kl], vbuf[0..vl]) catch { expect_true(false); return; };
    }
    let sb_s: []u8 = sst_b[0..];
    const stb: SstBuildStats = engine.flushMemTableToSst(bcfg, *sb_s, *iw_s, *bw_s) catch {
        expect_true(false);
        return;
    };

    // Batch 3: overwrite even keys 0..98 (50 updates) & delete odd keys 1..39 (20 deletes) -> flush to sst_c
    i = 0;
    while (i < 100) : (i += 2) {
        let ks: []u8 = kbuf[0..];
        let vs: []u8 = vbuf[0..];
        const kl: usize = formatNumKey("row:", i, *ks);
        const vl: usize = formatNumKey("v2_", i, *vs);
        * = engine.put(kbuf[0..kl], vbuf[0..vl]) catch { expect_true(false); return; };
    }
    i = 1;
    while (i < 40) : (i += 2) {
        let ks: []u8 = kbuf[0..];
        const kl: usize = formatNumKey("row:", i, *ks);
        * = engine.delete(kbuf[0..kl]) catch { expect_true(false); return; };
    }
    let sc_s: []u8 = sst_c[0..];
    const stc: SstBuildStats = engine.flushMemTableToSst(bcfg, *sc_s, *iw_s, *bw_s) catch {
        expect_true(false);
        return;
    };

    // Compact all 3 SSTables into a single bottommost SSTable
    const inputs: [3][]const u8 = [
        sst_c[0..stc.total_bytes],
        sst_b[0..stb.total_bytes],
        sst_a[0..sta.total_bytes],
    ];
    let cursors: [3]SstCursor = [SstCursor{}; 3];
    let ckeys: [768]u8 = [0; 768];
    let curs_s: []SstCursor = cursors[0..];
    let ck_s: []u8 = ckeys[0..];
    let sm_s: []u8 = sst_merged[0..];
    const cstats: SstCompactionStats = compactSstables(
        inputs[0..],
        SstCompactionConfig{
            drop_tombstones: true,
            min_active_snapshot_seq: 0xFFFF_FFFF_FFFF_FFFF,
            builder_cfg: bcfg,
        },
        *curs_s,
        *ck_s,
        *sm_s,
        *iw_s,
        *bw_s,
    ) catch { expect_true(false); return; };

    expect_eq(cstats.input_entries, 390);
    expect_eq(cstats.dropped_overwritten, 70); // 50 overwrites + 20 deleted base keys
    expect_eq(cstats.dropped_tombstones, 20);
    expect_eq(cstats.output_entries, 300);

    // Write 10 more hot records into WAL+MemTable, then simulate power-loss with 7-byte torn tail
    i = 300;
    while (i < 310) : (i += 1) {
        let ks: []u8 = kbuf[0..];
        let vs: []u8 = vbuf[0..];
        const kl: usize = formatNumKey("row:", i, *ks);
        const vl: usize = formatNumKey("hot", i, *vs);
        * = engine.put(kbuf[0..kl], vbuf[0..vl]) catch { expect_true(false); return; };
    }
    const persisted_wal_len: usize = engine.walBytes().len;
    let crash_wal: [4096]u8 = [0; 4096];
    let ci: usize = 0;
    while (ci < persisted_wal_len) : (ci += 1) crash_wal[ci] = engine.walBytes()[ci];
    crash_wal[persisted_wal_len + 0] = 0x4C;
    crash_wal[persisted_wal_len + 1] = 0x57;
    crash_wal[persisted_wal_len + 2] = 0xFF;

    // Cold restart into fresh LsmDbEngine and recover from torn WAL
    let entries2: [256]LsmMemEntryMeta = [LsmMemEntryMeta{}; 256];
    let order2: [256]u32 = [0; 256];
    let arena2: [16384]u8 = [0; 16384];
    let wal_buf2: [16384]u8 = [0; 16384];
    let wal2_s: []u8 = wal_buf2[0..];

    let restarted: LsmDbEngine = LsmDbEngine.init(
        LsmMemBuffers{ entries: entries2[0..], order: order2[0..], arena: arena2[0..] },
        *wal2_s,
    ) catch { expect_true(false); return; };
    const rstats: LsmWalRecoveryStats = restarted.recoverFromWal(crash_wal[0..persisted_wal_len + 7]) catch {
        expect_true(false);
        return;
    };
    expect_eq(rstats.recovered_records, 10);
    expect_eq(rstats.corrupt_tail_bytes, 7);
    expect_true(rstats.truncated_tail);

    // Layered read across recovered MemTable + compacted bottom SSTable
    const levels: [1][]const u8 = [sst_merged[0..cstats.output_bytes]];
    let sk: [256]u8 = [0; 256];
    let sk_s: []u8 = sk[0..];

    // Overwritten even key (row:0042 -> v2_0042)
    const r42: LsmMemGetResult = restarted.getLayered("row:0042", 9999, levels[0..], *sk_s) catch {
        expect_true(false);
        return;
    };
    expect_true(r42.status == LsmLookupStatus.found);
    expect_eq(compareBytes(r42.value, "v2_0042"), 0);

    // Deleted odd key (row:0015 -> purged by bottommost compaction -> not_found)
    const r15: LsmMemGetResult = restarted.getLayered("row:0015", 9999, levels[0..], *sk_s) catch {
        expect_true(false);
        return;
    };
    expect_true(r15.status == LsmLookupStatus.not_found);

    // Hot WAL-recovered key (row:0305 -> hot0305)
    const r305: LsmMemGetResult = restarted.getLayered("row:0305", 9999, levels[0..], *sk_s) catch {
        expect_true(false);
        return;
    };
    expect_true(r305.status == LsmLookupStatus.found);
    expect_eq(compareBytes(r305.value, "hot0305"), 0);

    // Untouched key from Batch 2 (row:0250 -> v1_0250)
    const r250: LsmMemGetResult = restarted.getLayered("row:0250", 9999, levels[0..], *sk_s) catch {
        expect_true(false);
        return;
    };
    expect_true(r250.status == LsmLookupStatus.found);
    expect_eq(compareBytes(r250.value, "v1_0250"), 0);
}

// ============================================================================
// Test 7: Boundary & Defensive Error Guards (Empty SSTable, Invalid Config, Tiny Buffers)
// ============================================================================
test("LsmEngine: empty SSTable, invalid config & tiny buffer guards") {
    let sst_buf: [1024]u8 = [0; 1024];
    let idx_w: [256]u8 = [0; 256];
    let blm_w: [128]u8 = [0; 128];
    let sst_s: []u8 = sst_buf[0..];
    let idx_s: []u8 = idx_w[0..];
    let blm_s: []u8 = blm_w[0..];

    // Empty SSTable builds, opens, and returns not_found cleanly
    let b: SstBuilder = SstBuilder.init(SstBuilderConfig{ expected_keys: 16 }, *sst_s, *idx_s, *blm_s) catch {
        expect_true(false);
        return;
    };
    const st: SstBuildStats = b.finish() catch { expect_true(false); return; };
    expect_eq(st.entry_count, 0);
    expect_eq(st.block_count, 0);
    const r: SstReader = SstReader.open(sst_buf[0..st.total_bytes]) catch { expect_true(false); return; };
    let sk: [64]u8 = [0; 64];
    let sk_s: []u8 = sk[0..];
    const miss: SstLookupResult = r.getAt("any", 10, *sk_s) catch { expect_true(false); return; };
    expect_true(miss.status == LsmLookupStatus.not_found);

    // Invalid config (block_target_bytes < 64) -> InvalidConfig
    let got_bad_cfg: bool = false;
    * = SstBuilder.init(SstBuilderConfig{ block_target_bytes: 16 }, *sst_s, *idx_s, *blm_s) catch |e| {
        if (e == LsmKvError.InvalidConfig) got_bad_cfg = true;
    };
    expect_true(got_bad_cfg);

    // Tiny WAL buffer -> BufferTooSmall
    let tiny_wal: [8]u8 = [0; 8];
    let tw_s: []u8 = tiny_wal[0..];
    let got_small: bool = false;
    * = LsmWalCodec.encodePut("k", "v", 1, *tw_s) catch |e| {
        if (e == LsmKvError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);
}
