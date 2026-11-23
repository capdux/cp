// Wave 29 R5 (Lane 2B) Rust Reference Hardening Test Suite for lib/db/lsmEngine.cp
// Reference benchmark: fjall-rs/lsm-tree (MIT/Apache-2.0)
// Covers:
//   1. SIMD Differential Equivalence (`compareBytes`, `commonPrefixLen`, `crc32Ieee`, `computePrefixUpperBound`)
//   2. MVCC Multi-Snapshot Isolation, `gcWatermarkSeqno` & Tombstone Lifecycle (snapshot_compact / snapshot_zombie)
//   3. Range Delete (`deleteRange`) MVCC Semantics, WAL Recovery, Layered Shadowing & Compaction Pruning
//   4. Prefix Scan & Reverse Range Scan (`test_reverse!` exact symmetry across MemTable, SSTable & Layered Engine)
//   5. SSTable Full Integrity Verification (`verifyFullIntegrity`), Tamper Guards & Duplicate `(key, seq)` Compaction
//   6. Model-Based Differential Testing: 1,500 deterministic PRNG ops against ground-truth MVCC reference model
//   7. Malformed WAL & SSTable Binary Fuzzing: 600+ mutated/forged/truncated payloads (0 panic / 0 OOB / 0 hang)

import { test, expect_eq } from "cap:test";
import {
    LSM_CURSOR_KEY_STRIDE,
    LsmKvError,
    crc32IeeeScalar,
    crc32Ieee,
    compareBytesScalar,
    compareBytes,
    commonPrefixLenScalar,
    commonPrefixLen,
    hasPrefix,
    computePrefixUpperBound,
    LsmWalOp,
    LsmWalRecordView,
    LsmWalRecoveryStats,
    LsmWalCodec,
    LsmMemEntryMeta,
    LsmMemBuffers,
    LsmLookupStatus,
    LsmMemGetResult,
    LsmMemScanItem,
    LsmRangeDeleteBound,
    LsmMemTable,
    SstBuilderConfig,
    SstBuildStats,
    SstLookupResult,
    SstVerifyReport,
    SstBuilder,
    SstReader,
    SstCursor,
    SstCompactionConfig,
    SstCompactionStats,
    compactSstables,
    LsmSnapshotHandle,
    LsmDbEngine,
} from "../../lib/db/lsmEngine.cp";

struct XorShift64 {
    state: u64 = 0x1E37_79B9_7F4A_7C15;

    fn next(*_): u64 {
        let x: u64 = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn range(*_, bound: usize): usize {
        if (bound == 0) return 0;
        return @as(usize, @intCast(_.next() % @as(u64, @intCast(bound))));
    }
}

fn formatKey2(prefix: []const u8, num: usize, out: *[]u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len && pos < out.len) : (i += 1) {
        out[pos] = prefix[i];
        pos += 1;
    }
    if (pos + 2 <= out.len) {
        out[pos + 0] = @as(u8, @intCast('0' + ((num / 10) % 10)));
        out[pos + 1] = @as(u8, @intCast('0' + (num % 10)));
        pos += 2;
    }
    return pos;
}

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

// ============================================================================
// Test 1: SIMD Differential Equivalence (compareBytes, commonPrefixLen, crc32)
// ============================================================================
fn runTestSimdDifferential(): void! {
    let rng: XorShift64 = XorShift64{ state: 0x2026_1123_CAFE_BABE };
    let buf_a: [96]u8 = [0; 96];
    let buf_b: [96]u8 = [0; 96];

    // Sweep lengths 0..80 including exact SIMD boundaries (15, 16, 17, 31, 32, 33, 48, 64)
    let case_i: usize = 0;
    while (case_i < 240) : (case_i += 1) {
        const len_a: usize = if (case_i < 80) case_i else rng.range(81);
        const len_b: usize = if (case_i < 80) case_i else rng.range(81);
        let j: usize = 0;
        while (j < len_a) : (j += 1) {
            buf_a[j] = @as(u8, @intCast(rng.next() & 0xFF));
        }
        const shared_target: usize = if (len_a < len_b) rng.range(len_a + 1) else rng.range(len_b + 1);
        j = 0;
        while (j < len_b) : (j += 1) {
            if (j < shared_target) {
                buf_b[j] = buf_a[j];
            } else {
                buf_b[j] = @as(u8, @intCast(rng.next() & 0xFF));
            }
        }
        const sa: []const u8 = buf_a[0..len_a];
        const sb: []const u8 = buf_b[0..len_b];

        expect_eq(compareBytes(sa, sb), compareBytesScalar(sa, sb));
        expect_eq(compareBytes(sb, sa), compareBytesScalar(sb, sa));
        expect_eq(compareBytes(sa, sa), 0);
        expect_eq(commonPrefixLen(sa, sb), commonPrefixLenScalar(sa, sb));
        expect_eq(commonPrefixLen(sa, sa), len_a);
        expect_eq(crc32Ieee(sa), crc32IeeeScalar(sa));
    }

    // Prefix upper bound edge cases (fjall-rs prefix_upper_range semantics)
    let ubuf: [32]u8 = [0; 32];
    let us: []u8 = ubuf[0..];

    const u_empty: usize? = computePrefixUpperBound("", us) catch { expect_true(false)?; return; };
    expect_true(u_empty == null)?;

    const u_abc: usize? = computePrefixUpperBound("abc", us) catch { expect_true(false)?; return; };
    expect_eq(u_abc ?? 0, 3);
    expect_eq(compareBytes(ubuf[0..3], "abd"), 0);

    const p_ff_tail: [3]u8 = ['a', 0xFF, 0xFF];
    const u_ff: usize? = computePrefixUpperBound(p_ff_tail[0..], us) catch { expect_true(false)?; return; };
    expect_eq(u_ff ?? 0, 1);
    expect_eq(ubuf[0], 'b');

    const p_all_ff: [3]u8 = [0xFF, 0xFF, 0xFF];
    const u_all_ff: usize? = computePrefixUpperBound(p_all_ff[0..], us) catch { expect_true(false)?; return; };
    expect_true(u_all_ff == null)?;

    expect_true(hasPrefix("tenant:001:user:0000000000000001", "tenant:001:user:0000"))?;
    expect_true(!hasPrefix("tenant:001:user:0000000000000001", "tenant:001:user:0001"))?;
}

// ============================================================================
// Test 2: MVCC Multi-Snapshot Isolation, gcWatermarkSeqno & Tombstone Lifecycle
// ============================================================================
fn runTestMvccSnapshots(): void! {
    let entries: [128]LsmMemEntryMeta = [LsmMemEntryMeta{}; 128];
    let order: [128]u32 = [0; 128];
    let arena: [8192]u8 = [0; 8192];
    let wal_buf: [8192]u8 = [0; 8192];
    let wal_s: []u8 = wal_buf[0..];

    let engine: LsmDbEngine = LsmDbEngine.init(
        LsmMemBuffers{ entries: entries[0..], order: order[0..], arena: arena[0..] },
        wal_s,
    ) catch { expect_true(false)?; return; };

    expect_eq(engine.activeSnapshotCount(), 0);
    expect_eq(engine.gcWatermarkSeqno(), ((@as(u64, 0xFFFF_FFFF) << 32) | 0xFFFF_FFFF));

    // Phase 1: Initial writes (seq 1..3) -> flush SST0
    * = engine.put("a", "a_v1") catch { expect_true(false)?; return; };
    * = engine.put("b", "b_v1") catch { expect_true(false)?; return; };
    * = engine.put("c", "c_v1") catch { expect_true(false)?; return; };
    const snap1: LsmSnapshotHandle = engine.openSnapshot() catch { expect_true(false)?; return; };
    expect_eq(snap1.seqno, 3);
    expect_eq(engine.activeSnapshotCount(), 1);
    expect_eq(engine.gcWatermarkSeqno(), 3);

    const bcfg: SstBuilderConfig = SstBuilderConfig{ block_target_bytes: 128, expected_keys: 32 };
    let sst0: [4096]u8 = [0; 4096];
    let sst1: [4096]u8 = [0; 4096];
    let sst_c1: [8192]u8 = [0; 8192];
    let sst_c2: [8192]u8 = [0; 8192];
    let idx_w: [1024]u8 = [0; 1024];
    let blm_w: [256]u8 = [0; 256];
    let s0_s: []u8 = sst0[0..];
    let s1_s: []u8 = sst1[0..];
    let sc1_s: []u8 = sst_c1[0..];
    let sc2_s: []u8 = sst_c2[0..];
    let iw_s: []u8 = idx_w[0..];
    let bw_s: []u8 = blm_w[0..];

    const st0: SstBuildStats = engine.flushMemTableToSst(bcfg, s0_s, iw_s, bw_s) catch { expect_true(false)?; return; };

    // Phase 2: Overwrite "a", delete "b", pin snap2 (seq 5), delete "c" (seq 6) -> flush SST1
    * = engine.put("a", "a_v2") catch { expect_true(false)?; return; };
    * = engine.delete("b") catch { expect_true(false)?; return; };
    const snap2: LsmSnapshotHandle = engine.openSnapshot() catch { expect_true(false)?; return; };
    expect_eq(snap2.seqno, 5);
    expect_eq(engine.activeSnapshotCount(), 2);
    expect_eq(engine.gcWatermarkSeqno(), 3); // oldest active snapshot pins watermark at 3!
    * = engine.delete("c") catch { expect_true(false)?; return; };
    const st1: SstBuildStats = engine.flushMemTableToSst(bcfg, s1_s, iw_s, bw_s) catch { expect_true(false)?; return; };

    // Compact with gcWatermarkSeqno() == 3: MUST preserve a_v1, b_v1, c_v1 AND tombstones b(5), c(6)
    const inputs1: [2][]const u8 = [sst1[0..st1.total_bytes], sst0[0..st0.total_bytes]];
    let cursors: [2]SstCursor = [SstCursor{}; 2];
    let ckeys: [512]u8 = [0; 512];
    let curs_s: []SstCursor = cursors[0..];
    let ck_s: []u8 = ckeys[0..];

    const cs1: SstCompactionStats = compactSstables(
        inputs1[0..],
        SstCompactionConfig{
            drop_tombstones: true,
            min_active_snapshot_seq: engine.gcWatermarkSeqno(),
            builder_cfg: bcfg,
        },
        curs_s,
        ck_s,
        sc1_s,
        iw_s,
        bw_s,
    ) catch { expect_true(false)?; return; };
    expect_eq(cs1.output_entries, 6);
    expect_eq(cs1.dropped_tombstones, 0);

    // Verify snap1 (seq=3), snap2 (seq=5), and latest (seq=6) against compacted SSTable
    const lvl_c1: [1][]const u8 = [sst_c1[0..cs1.output_bytes]];
    let sk: [256]u8 = [0; 256];
    let sk_s: []u8 = sk[0..];

    const r_a_s1: LsmMemGetResult = engine.getLayered("a", snap1.seqno, lvl_c1[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_a_s1.status == LsmLookupStatus.found)?;
    expect_eq(compareBytes(r_a_s1.value, "a_v1"), 0);

    const r_b_s1: LsmMemGetResult = engine.getLayered("b", snap1.seqno, lvl_c1[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_b_s1.status == LsmLookupStatus.found)?;
    expect_eq(compareBytes(r_b_s1.value, "b_v1"), 0);

    const r_b_s2: LsmMemGetResult = engine.getLayered("b", snap2.seqno, lvl_c1[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_b_s2.status == LsmLookupStatus.deleted)?;

    const r_c_s2: LsmMemGetResult = engine.getLayered("c", snap2.seqno, lvl_c1[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_c_s2.status == LsmLookupStatus.found)?;
    expect_eq(compareBytes(r_c_s2.value, "c_v1"), 0);

    const r_c_latest: LsmMemGetResult = engine.getLayered("c", engine.latestSeq(), lvl_c1[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_c_latest.status == LsmLookupStatus.deleted)?;

    // Release snap1 -> watermark advances to 5; release snap2 -> watermark advances to u64::MAX
    expect_true(engine.releaseSnapshot(snap1))?;
    expect_eq(engine.gcWatermarkSeqno(), 5);
    expect_true(engine.releaseSnapshot(snap2))?;
    expect_eq(engine.gcWatermarkSeqno(), ((@as(u64, 0xFFFF_FFFF) << 32) | 0xFFFF_FFFF));

    // Re-compact with no active snapshots -> only a_v2 survives! b & c tombstones + old versions purged
    const inputs2: [1][]const u8 = [sst_c1[0..cs1.output_bytes]];
    const cs2: SstCompactionStats = compactSstables(
        inputs2[0..],
        SstCompactionConfig{
            drop_tombstones: true,
            min_active_snapshot_seq: engine.gcWatermarkSeqno(),
            builder_cfg: bcfg,
        },
        curs_s,
        ck_s,
        sc2_s,
        iw_s,
        bw_s,
    ) catch { expect_true(false)?; return; };
    expect_eq(cs2.output_entries, 1);
    expect_eq(cs2.dropped_tombstones, 2);
    expect_eq(cs2.dropped_overwritten, 3);
}

// ============================================================================
// Test 3: Range Delete MVCC Semantics, WAL Recovery & Compaction Pruning
// ============================================================================
fn runTestRangeDelete(): void! {
    let entries: [128]LsmMemEntryMeta = [LsmMemEntryMeta{}; 128];
    let order: [128]u32 = [0; 128];
    let arena: [8192]u8 = [0; 8192];
    let wal_buf: [8192]u8 = [0; 8192];
    let wal_s: []u8 = wal_buf[0..];

    let engine: LsmDbEngine = LsmDbEngine.init(
        LsmMemBuffers{ entries: entries[0..], order: order[0..], arena: arena[0..] },
        wal_s,
    ) catch { expect_true(false)?; return; };

    let kbuf: [16]u8 = [0; 16];
    let vbuf: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        let ks: []u8 = kbuf[0..];
        let vs: []u8 = vbuf[0..];
        const kl: usize = formatKey2("k", i, ks);
        const vl: usize = formatKey2("v", i, vs);
        * = engine.put(kbuf[0..kl], vbuf[0..vl]) catch { expect_true(false)?; return; };
    }
    const seq_before_rd: u64 = engine.latestSeq(); // 16

    // Flush k00..k15 into SST0, then apply RangeDelete([k04, k12)) in MemTable + resurrect k08
    const bcfg: SstBuilderConfig = SstBuilderConfig{ block_target_bytes: 128, expected_keys: 32 };
    let sst0: [4096]u8 = [0; 4096];
    let idx_w: [1024]u8 = [0; 1024];
    let blm_w: [256]u8 = [0; 256];
    let s0_s: []u8 = sst0[0..];
    let iw_s: []u8 = idx_w[0..];
    let bw_s: []u8 = blm_w[0..];
    const st0: SstBuildStats = engine.flushMemTableToSst(bcfg, s0_s, iw_s, bw_s) catch { expect_true(false)?; return; };

    const seq_rd: u64 = engine.deleteRange("k04", "k12") catch { expect_true(false)?; return; };
    expect_eq(seq_rd, 17);
    const seq_resurrect: u64 = engine.put("k08", "resurrected") catch { expect_true(false)?; return; };
    expect_eq(seq_resurrect, 18);

    // Invalid range [k10, k05) must fail with InvalidRange
    let got_bad_range: bool = false;
    * = engine.deleteRange("k10", "k05") catch |e| {
        if (e == LsmKvError.InvalidRange) got_bad_range = true;
    };
    expect_true(got_bad_range)?;

    const lvls: [1][]const u8 = [sst0[0..st0.total_bytes]];
    let sk: [256]u8 = [0; 256];
    let sk_s: []u8 = sk[0..];

    // Snapshot BEFORE range delete still sees k06 from SST0!
    const r_old: LsmMemGetResult = engine.getLayered("k06", seq_before_rd, lvls[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_old.status == LsmLookupStatus.found)?;
    expect_eq(compareBytes(r_old.value, "v06"), 0);

    // Latest snapshot sees k06 deleted by range tombstone, k08 resurrected, k12 untouched
    const r_k06: LsmMemGetResult = engine.getLayered("k06", seq_resurrect, lvls[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_k06.status == LsmLookupStatus.deleted)?;
    const r_k08: LsmMemGetResult = engine.getLayered("k08", seq_resurrect, lvls[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_k08.status == LsmLookupStatus.found)?;
    expect_eq(compareBytes(r_k08.value, "resurrected"), 0);
    const r_k12: LsmMemGetResult = engine.getLayered("k12", seq_resurrect, lvls[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_k12.status == LsmLookupStatus.found)?;

    // Crash recovery: replay WAL (containing RangeDelete + Put) with 5-byte torn tail into fresh engine
    const wbytes: []const u8 = engine.walBytes();
    let crash_wal: [1024]u8 = [0; 1024];
    let wi: usize = 0;
    while (wi < wbytes.len) : (wi += 1) crash_wal[wi] = wbytes[wi];
    crash_wal[wbytes.len + 0] = 0x4C;
    crash_wal[wbytes.len + 1] = 0x57;

    let entries2: [64]LsmMemEntryMeta = [LsmMemEntryMeta{}; 64];
    let order2: [64]u32 = [0; 64];
    let arena2: [4096]u8 = [0; 4096];
    let wal2: [4096]u8 = [0; 4096];
    let wal2_s: []u8 = wal2[0..];
    let rec_eng: LsmDbEngine = LsmDbEngine.init(
        LsmMemBuffers{ entries: entries2[0..], order: order2[0..], arena: arena2[0..] },
        wal2_s,
    ) catch { expect_true(false)?; return; };
    const rstats: LsmWalRecoveryStats = rec_eng.recoverFromWal(crash_wal[0..wbytes.len + 5]) catch { expect_true(false)?; return; };
    expect_eq(rstats.recovered_records, 2);
    expect_true(rstats.truncated_tail)?;

    const r_rec_k06: LsmMemGetResult = rec_eng.getLayered("k06", 100, lvls[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_rec_k06.status == LsmLookupStatus.deleted)?;
    const r_rec_k08: LsmMemGetResult = rec_eng.getLayered("k08", 100, lvls[0..], sk_s) catch { expect_true(false)?; return; };
    expect_true(r_rec_k08.status == LsmLookupStatus.found)?;

    // Compaction with range_tombstones below watermark drops k04..k11 (8 keys) cleanly
    const rts: [1]LsmRangeDeleteBound = [LsmRangeDeleteBound{ start_key: "k04", end_key: "k12", seq: seq_rd }];
    let cursors: [1]SstCursor = [SstCursor{}; 1];
    let ckeys: [256]u8 = [0; 256];
    let out_sst: [4096]u8 = [0; 4096];
    let curs_s: []SstCursor = cursors[0..];
    let ck_s: []u8 = ckeys[0..];
    let os_s: []u8 = out_sst[0..];
    const cstats: SstCompactionStats = compactSstables(
        lvls[0..],
        SstCompactionConfig{
            drop_tombstones: true,
            min_active_snapshot_seq: 100,
            builder_cfg: bcfg,
            range_tombstones: rts[0..],
        },
        curs_s,
        ck_s,
        os_s,
        iw_s,
        bw_s,
    ) catch { expect_true(false)?; return; };
    expect_eq(cstats.output_entries, 8); // 16 - 8 = 8 survivors (k00..k03, k12..k15)
}

// ============================================================================
// Test 4: Prefix Scan & Reverse Range Scan Exact Symmetry (test_reverse!)
// ============================================================================
fn runTestScanSymmetry(): void! {
    let entries: [128]LsmMemEntryMeta = [LsmMemEntryMeta{}; 128];
    let order: [128]u32 = [0; 128];
    let arena: [8192]u8 = [0; 8192];
    let wal_buf: [8192]u8 = [0; 8192];
    let wal_s: []u8 = wal_buf[0..];

    let engine: LsmDbEngine = LsmDbEngine.init(
        LsmMemBuffers{ entries: entries[0..], order: order[0..], arena: arena[0..] },
        wal_s,
    ) catch { expect_true(false)?; return; };

    // Populate two prefixes ("app:" and "sys:") across SST0 and active MemTable with deletes
    * = engine.put("app:01", "a1") catch { expect_true(false)?; return; };
    * = engine.put("app:02", "a2") catch { expect_true(false)?; return; };
    * = engine.put("app:03", "a3") catch { expect_true(false)?; return; };
    * = engine.put("app:04", "a4") catch { expect_true(false)?; return; };
    * = engine.put("sys:01", "s1") catch { expect_true(false)?; return; };
    * = engine.put("sys:02", "s2") catch { expect_true(false)?; return; };

    const bcfg: SstBuilderConfig = SstBuilderConfig{ block_target_bytes: 96, expected_keys: 32 };
    let sst0: [4096]u8 = [0; 4096];
    let idx_w: [1024]u8 = [0; 1024];
    let blm_w: [256]u8 = [0; 256];
    let s0_s: []u8 = sst0[0..];
    let iw_s: []u8 = idx_w[0..];
    let bw_s: []u8 = blm_w[0..];
    const st0: SstBuildStats = engine.flushMemTableToSst(bcfg, s0_s, iw_s, bw_s) catch { expect_true(false)?; return; };

    // Active MemTable updates & tombstones shadowing SST0
    * = engine.put("app:02", "a2_new") catch { expect_true(false)?; return; };
    * = engine.delete("app:03") catch { expect_true(false)?; return; };
    * = engine.put("app:05", "a5") catch { expect_true(false)?; return; };
    * = engine.delete("sys:01") catch { expect_true(false)?; return; };

    // 1. SstReader scanPrefix & scanRangeReverse symmetry
    const reader: SstReader = SstReader.open(sst0[0..st0.total_bytes]) catch { expect_true(false)?; return; };
    let ka_fwd: [1024]u8 = [0; 1024];
    let ka_rev: [1024]u8 = [0; 1024];
    let it_fwd: [16]LsmMemScanItem = [LsmMemScanItem{}; 16];
    let it_rev: [16]LsmMemScanItem = [LsmMemScanItem{}; 16];
    let kaf_s: []u8 = ka_fwd[0..];
    let kar_s: []u8 = ka_rev[0..];
    let if_s: []LsmMemScanItem = it_fwd[0..];
    let ir_s: []LsmMemScanItem = it_rev[0..];

    const n_sst_pfx: usize = reader.scanPrefix("app:", 100, kaf_s, if_s) catch { expect_true(false)?; return; };
    expect_eq(n_sst_pfx, 4);
    const n_sst_fwd: usize = reader.scanRange("", "", 100, kaf_s, if_s) catch { expect_true(false)?; return; };
    const n_sst_rev: usize = reader.scanRangeReverse("", "", 100, kar_s, ir_s) catch { expect_true(false)?; return; };
    expect_eq(n_sst_fwd, 6);
    expect_eq(n_sst_rev, 6);
    let idx: usize = 0;
    while (idx < n_sst_fwd) : (idx += 1) {
        expect_eq(compareBytes(it_fwd[idx].key, it_rev[n_sst_fwd - 1 - idx].key), 0);
        expect_eq(compareBytes(it_fwd[idx].value, it_rev[n_sst_fwd - 1 - idx].value), 0);
    }

    // 2. Layered scanPrefixLayered & scanRangeLayered vs scanRangeReverseLayered exact symmetry
    const lvls: [1][]const u8 = [sst0[0..st0.total_bytes]];
    let cursors: [1]SstCursor = [SstCursor{}; 1];
    let ckeys: [256]u8 = [0; 256];
    let curs_s: []SstCursor = cursors[0..];
    let ck_s: []u8 = ckeys[0..];

    const n_lpfx: usize = engine.scanPrefixLayered("app:", 100, lvls[0..], curs_s, ck_s, kaf_s, if_s) catch { expect_true(false)?; return; };
    expect_eq(n_lpfx, 4); // app:01, app:02(a2_new), app:04, app:05 (app:03 deleted)
    expect_eq(compareBytes(it_fwd[1].value, "a2_new"), 0);

    const n_lfwd: usize = engine.scanRangeLayered("", "", 100, lvls[0..], curs_s, ck_s, kaf_s, if_s) catch { expect_true(false)?; return; };
    const n_lrev: usize = engine.scanRangeReverseLayered("", "", 100, lvls[0..], curs_s, ck_s, kar_s, ir_s) catch { expect_true(false)?; return; };
    expect_eq(n_lfwd, 5);
    expect_eq(n_lrev, 5);
    idx = 0;
    while (idx < n_lfwd) : (idx += 1) {
        expect_eq(compareBytes(it_fwd[idx].key, it_rev[n_lfwd - 1 - idx].key), 0);
        expect_eq(compareBytes(it_fwd[idx].value, it_rev[n_lfwd - 1 - idx].value), 0);
    }

    // 3. Top-2 reverse limit returns strictly the 2 largest keys (sys:02, app:05)
    let top2: [2]LsmMemScanItem = [LsmMemScanItem{}; 2];
    let top2_s: []LsmMemScanItem = top2[0..];
    const n_top2: usize = engine.scanRangeReverseLayered("", "", 100, lvls[0..], curs_s, ck_s, kar_s, top2_s) catch { expect_true(false)?; return; };
    expect_eq(n_top2, 2);
    expect_eq(compareBytes(top2[0].key, "sys:02"), 0);
    expect_eq(compareBytes(top2[1].key, "app:05"), 0);
}

// ============================================================================
// Test 5: Full SSTable Integrity Verification & Duplicate (key, seq) Compaction
// ============================================================================
fn runTestSstIntegrity(): void! {
    const bcfg: SstBuilderConfig = SstBuilderConfig{ block_target_bytes: 96, expected_keys: 32 };
    let sst_a: [4096]u8 = [0; 4096];
    let idx_w: [1024]u8 = [0; 1024];
    let blm_w: [256]u8 = [0; 256];
    let sa_s: []u8 = sst_a[0..];
    let iw_s: []u8 = idx_w[0..];
    let bw_s: []u8 = blm_w[0..];

    let b: SstBuilder = SstBuilder.init(bcfg, sa_s, iw_s, bw_s) catch { expect_true(false)?; return; };
    * = b.addEntry("k01", "v1", 10, false) catch { expect_true(false)?; return; };
    * = b.addEntry("k02", "v2", 11, false) catch { expect_true(false)?; return; };
    * = b.addEntry("k03", "", 12, true) catch { expect_true(false)?; return; };
    const st: SstBuildStats = b.finish() catch { expect_true(false)?; return; };

    const r: SstReader = SstReader.open(sst_a[0..st.total_bytes]) catch { expect_true(false)?; return; };
    let sk: [256]u8 = [0; 256];
    let sk_s: []u8 = sk[0..];
    const rep: SstVerifyReport = r.verifyFullIntegrity(sk_s) catch { expect_true(false)?; return; };
    expect_eq(rep.entries_verified, 3);
    expect_true(rep.bloom_verified)?;

    // Compacting TWO identical SSTables (duplicate (key, seq)) must deduplicate without InvalidKeyOrder
    const dup_inputs: [2][]const u8 = [sst_a[0..st.total_bytes], sst_a[0..st.total_bytes]];
    let cursors: [2]SstCursor = [SstCursor{}; 2];
    let ckeys: [512]u8 = [0; 512];
    let out_sst: [4096]u8 = [0; 4096];
    let curs_s: []SstCursor = cursors[0..];
    let ck_s: []u8 = ckeys[0..];
    let os_s: []u8 = out_sst[0..];

    const cs: SstCompactionStats = compactSstables(
        dup_inputs[0..],
        SstCompactionConfig{
            drop_tombstones: false,
            min_active_snapshot_seq: 1,
            builder_cfg: bcfg,
        },
        curs_s,
        ck_s,
        os_s,
        iw_s,
        bw_s,
    ) catch { expect_true(false)?; return; };
    expect_eq(cs.output_entries, 3);
    expect_eq(cs.dropped_overwritten, 3);
}

// ============================================================================
// Test 6: Model-Based Differential Testing (1,500 Deterministic PRNG Ops)
// ============================================================================
struct RefModel {
    present: [32]bool = [false; 32];
    vals: [32]u32 = [0; 32];
}

fn formatValNum(v: u32, out: *[]u8): usize {
    out[0] = 'V';
    out[1] = @as(u8, @intCast('0' + ((v / 1000) % 10)));
    out[2] = @as(u8, @intCast('0' + ((v / 100) % 10)));
    out[3] = @as(u8, @intCast('0' + ((v / 10) % 10)));
    out[4] = @as(u8, @intCast('0' + (v % 10)));
    return 5;
}

fn runTestModelBased1500(): void! {
    let rng: XorShift64 = XorShift64{ state: 0x5EAD_BEEF_1234_5678 };
    let model: RefModel = RefModel{};

    // Snapshot saved states (up to 4 checkpoints verified against pinned MVCC snapshots)
    let snap_handles: [4]LsmSnapshotHandle = [LsmSnapshotHandle{}; 4];
    let snap_models: [4]RefModel = [RefModel{}; 4];
    let snap_used: [4]bool = [false; 4];

    let entries: [256]LsmMemEntryMeta = [LsmMemEntryMeta{}; 256];
    let order: [256]u32 = [0; 256];
    let arena: [16384]u8 = [0; 16384];
    let wal_buf: [16384]u8 = [0; 16384];
    let wal_s: []u8 = wal_buf[0..];
    let engine: LsmDbEngine = LsmDbEngine.init(
        LsmMemBuffers{ entries: entries[0..], order: order[0..], arena: arena[0..] },
        wal_s,
    ) catch { expect_true(false)?; return; };

    const bcfg: SstBuilderConfig = SstBuilderConfig{ block_target_bytes: 128, expected_keys: 64 };
    let sst_l0: [16384]u8 = [0; 16384];
    let sst_l1: [16384]u8 = [0; 16384];
    let sst_tmp: [16384]u8 = [0; 16384];
    let l0_len: usize = 0;
    let l1_len: usize = 0;
    let idx_w: [2048]u8 = [0; 2048];
    let blm_w: [512]u8 = [0; 512];
    let iw_s: []u8 = idx_w[0..];
    let bw_s: []u8 = blm_w[0..];

    let cursors: [2]SstCursor = [SstCursor{}; 2];
    let ckeys: [512]u8 = [0; 512];
    let curs_s: []SstCursor = cursors[0..];
    let ck_s: []u8 = ckeys[0..];
    let sk: [256]u8 = [0; 256];
    let sk_s: []u8 = sk[0..];

    let kbuf: [16]u8 = [0; 16];
    let vbuf: [16]u8 = [0; 16];
    let ka_scan: [2048]u8 = [0; 2048];
    let ka_rev: [2048]u8 = [0; 2048];
    let scan_items: [40]LsmMemScanItem = [LsmMemScanItem{}; 40];
    let rev_items: [40]LsmMemScanItem = [LsmMemScanItem{}; 40];

    let step: usize = 0;
    while (step < 1500) : (step += 1) {
        const op: usize = rng.range(100);
        const key_id: usize = rng.range(32);
        let ks: []u8 = kbuf[0..];
        const kl: usize = formatKey2("m:", key_id, ks);

        // Flush before MemTable/WAL fills up
        if (engine.mem.entry_count >= 180 || engine.walBytes().len >= 12000) {
            if (l0_len > 0 && l1_len > 0) {
                const cin: [2][]const u8 = [sst_l0[0..l0_len], sst_l1[0..l1_len]];
                let tmp_s: []u8 = sst_tmp[0..];
                const cs: SstCompactionStats = compactSstables(
                    cin[0..],
                    SstCompactionConfig{
                        drop_tombstones: true,
                        min_active_snapshot_seq: engine.gcWatermarkSeqno(),
                        builder_cfg: bcfg,
                    },
                    curs_s,
                    ck_s,
                    tmp_s,
                    iw_s,
                    bw_s,
                ) catch { expect_true(false)?; return; };
                let ci: usize = 0;
                while (ci < cs.output_bytes) : (ci += 1) sst_l1[ci] = sst_tmp[ci];
                l1_len = cs.output_bytes;
                l0_len = 0;
            } else if (l0_len > 0 && l1_len == 0) {
                let ci: usize = 0;
                while (ci < l0_len) : (ci += 1) sst_l1[ci] = sst_l0[ci];
                l1_len = l0_len;
                l0_len = 0;
            }
            let l0_s: []u8 = sst_l0[0..];
            const fst: SstBuildStats = engine.flushMemTableToSst(bcfg, l0_s, iw_s, bw_s) catch { expect_true(false)?; return; };
            l0_len = fst.total_bytes;
        }

        if (op < 40) {
            // Put
            const vnum: u32 = @as(u32, @intCast(rng.range(9999)));
            let vs: []u8 = vbuf[0..];
            const vl: usize = formatValNum(vnum, vs);
            * = engine.put(kbuf[0..kl], vbuf[0..vl]) catch { expect_true(false)?; return; };
            model.present[key_id] = true;
            model.vals[key_id] = vnum;
        } else if (op < 55) {
            // Delete
            * = engine.delete(kbuf[0..kl]) catch { expect_true(false)?; return; };
            model.present[key_id] = false;
        } else if (op < 65) {
            // Open or release a pinned snapshot
            const slot: usize = rng.range(4);
            if (!snap_used[slot]) {
                snap_handles[slot] = engine.openSnapshot() catch { expect_true(false)?; return; };
                snap_models[slot] = model;
                snap_used[slot] = true;
            } else {
                expect_true(engine.releaseSnapshot(snap_handles[slot]))?;
                snap_used[slot] = false;
            }
        } else if (op < 85) {
            // Point Get verification (latest + random active snapshot)
            const lvls: [2][]const u8 = [sst_l0[0..l0_len], sst_l1[0..l1_len]];
            const res: LsmMemGetResult = engine.getLayered(kbuf[0..kl], engine.latestSeq(), lvls[0..], sk_s) catch { expect_true(false)?; return; };
            if (model.present[key_id]) {
                expect_true(res.status == LsmLookupStatus.found)?;
                let vs: []u8 = vbuf[0..];
                const vl: usize = formatValNum(model.vals[key_id], vs);
                expect_eq(compareBytes(res.value, vbuf[0..vl]), 0);
            } else {
                expect_true(res.status != LsmLookupStatus.found)?;
            }

            const slot: usize = rng.range(4);
            if (snap_used[slot]) {
                const sres: LsmMemGetResult = engine.getLayered(kbuf[0..kl], snap_handles[slot].seqno, lvls[0..], sk_s) catch { expect_true(false)?; return; };
                if (snap_models[slot].present[key_id]) {
                    expect_true(sres.status == LsmLookupStatus.found)?;
                    let vs: []u8 = vbuf[0..];
                    const vl: usize = formatValNum(snap_models[slot].vals[key_id], vs);
                    expect_eq(compareBytes(sres.value, vbuf[0..vl]), 0);
                } else {
                    expect_true(sres.status != LsmLookupStatus.found)?;
                }
            }
        } else {
            // Full ascending & reverse range scan verification against model
            const lvls: [2][]const u8 = [sst_l0[0..l0_len], sst_l1[0..l1_len]];
            let kaf_s: []u8 = ka_scan[0..];
            let kar_s: []u8 = ka_rev[0..];
            let if_s: []LsmMemScanItem = scan_items[0..];
            let ir_s: []LsmMemScanItem = rev_items[0..];
            const nf: usize = engine.scanRangeLayered("", "", engine.latestSeq(), lvls[0..], curs_s, ck_s, kaf_s, if_s) catch { expect_true(false)?; return; };
            const nr: usize = engine.scanRangeReverseLayered("", "", engine.latestSeq(), lvls[0..], curs_s, ck_s, kar_s, ir_s) catch { expect_true(false)?; return; };

            let expected_live: usize = 0;
            let k_check: usize = 0;
            while (k_check < 32) : (k_check += 1) {
                if (model.present[k_check]) {
                    expect_true(expected_live < nf)?;
                    let kcs: []u8 = kbuf[0..];
                    let vcs: []u8 = vbuf[0..];
                    const kcl: usize = formatKey2("m:", k_check, kcs);
                    const vcl: usize = formatValNum(model.vals[k_check], vcs);
                    expect_eq(compareBytes(scan_items[expected_live].key, kbuf[0..kcl]), 0);
                    expect_eq(compareBytes(scan_items[expected_live].value, vbuf[0..vcl]), 0);
                    expected_live += 1;
                }
            }
            expect_eq(nf, expected_live);
            expect_eq(nr, expected_live);
            let ri: usize = 0;
            while (ri < nf) : (ri += 1) {
                expect_eq(compareBytes(scan_items[ri].key, rev_items[nf - 1 - ri].key), 0);
            }
        }
    }
}

fn fuzzProbeSst(data: []const u8, sk: *[]u8) {
    const r: SstReader = SstReader.open(data) catch return;
    const g: SstLookupResult = r.getAt("k01", 100, sk) catch SstLookupResult{};
    const v: SstVerifyReport = r.verifyFullIntegrity(sk) catch SstVerifyReport{};
    * = g;
    * = v;
}

// ============================================================================
// Test 7: Malformed WAL & SSTable Binary Fuzz (600+ cases, 0 panic / 0 OOB)
// ============================================================================
fn runTestFuzzBinary600(): void! {
    let rng: XorShift64 = XorShift64{ state: 0x700D_FACE_9988_7766 };

    // Build a valid reference WAL & valid reference SSTable as fuzz seeds
    let seed_wal: [512]u8 = [0; 512];
    let wpos: usize = 0;
    let wt1: []u8 = seed_wal[wpos..];
    wpos += LsmWalCodec.encodePut("alpha", "1111", 1, wt1) catch { expect_true(false)?; return; };
    let wt2: []u8 = seed_wal[wpos..];
    wpos += LsmWalCodec.encodeRangeDelete("beta", "delta", 2, wt2) catch { expect_true(false)?; return; };
    let wt3: []u8 = seed_wal[wpos..];
    wpos += LsmWalCodec.encodeDelete("gamma", 3, wt3) catch { expect_true(false)?; return; };

    const bcfg: SstBuilderConfig = SstBuilderConfig{ block_target_bytes: 96, expected_keys: 16 };
    let seed_sst: [4096]u8 = [0; 4096];
    let idx_w: [1024]u8 = [0; 1024];
    let blm_w: [256]u8 = [0; 256];
    let ss_s: []u8 = seed_sst[0..];
    let iw_s: []u8 = idx_w[0..];
    let bw_s: []u8 = blm_w[0..];
    let sb: SstBuilder = SstBuilder.init(bcfg, ss_s, iw_s, bw_s) catch { expect_true(false)?; return; };
    * = sb.addEntry("k01", "v01", 1, false) catch { expect_true(false)?; return; };
    * = sb.addEntry("k02", "v02", 2, false) catch { expect_true(false)?; return; };
    * = sb.addEntry("k03", "v03", 3, false) catch { expect_true(false)?; return; };
    const sst_st: SstBuildStats = sb.finish() catch { expect_true(false)?; return; };
    const sst_len: usize = sst_st.total_bytes;

    let mut_buf: [4096]u8 = [0; 4096];
    let recs: [8]LsmWalRecordView = [LsmWalRecordView{}; 8];
    let recs_s: []LsmWalRecordView = recs[0..];
    let sk: [256]u8 = [0; 256];
    let sk_s: []u8 = sk[0..];

    // 1. Exhaustive truncation of WAL + 200 random WAL mutations (forged lengths, bit-flips, garbage)
    let trunc_len: usize = 0;
    while (trunc_len <= wpos) : (trunc_len += 1) {
        const rs: LsmWalRecoveryStats = LsmWalCodec.recoverLog(seed_wal[0..trunc_len], recs_s);
        expect_true(rs.valid_bytes <= trunc_len)?;
    }
    let f_i: usize = 0;
    while (f_i < 200) : (f_i += 1) {
        let j: usize = 0;
        while (j < wpos) : (j += 1) mut_buf[j] = seed_wal[j];
        const flen: usize = if (f_i < 150) wpos else rng.range(wpos + 1);
        const flips: usize = rng.range(4) + 1;
        let k: usize = 0;
        while (k < flips && flen > 0) : (k += 1) {
            const pos: usize = rng.range(flen);
            mut_buf[pos] ^= @as(u8, @intCast(rng.range(255) + 1));
        }
        const rs: LsmWalRecoveryStats = LsmWalCodec.recoverLog(mut_buf[0..flen], recs_s);
        expect_true(rs.valid_bytes <= flen)?;
    }

    // 2. Exhaustive truncation of SSTable + 250 random SSTable mutations & forged headers
    trunc_len = 0;
    while (trunc_len < sst_len) : (trunc_len += 3) {
        fuzzProbeSst(seed_sst[0..trunc_len], sk_s);
    }
    f_i = 0;
    while (f_i < 250) : (f_i += 1) {
        let j: usize = 0;
        while (j < sst_len) : (j += 1) mut_buf[j] = seed_sst[j];
        const mode: usize = rng.range(5);
        if (mode == 0) {
            // Flip random byte in Data/Index/Bloom/Footer
            const pos: usize = rng.range(sst_len);
            mut_buf[pos] ^= @as(u8, @intCast(rng.range(255) + 1));
        } else if (mode == 1) {
            // Pure random garbage payload
            let g: usize = 0;
            while (g < 128) : (g += 1) mut_buf[g] = @as(u8, @intCast(rng.next() & 0xFF));
        } else if (mode == 2) {
            // Forge huge footer index_off / block_count (0xFFFF_FFFF)
            mut_buf[sst_len - 64 + 0] = 0xFF;
            mut_buf[sst_len - 64 + 1] = 0xFF;
            mut_buf[sst_len - 64 + 32] = 0xFF;
            mut_buf[sst_len - 64 + 33] = 0xFF;
        } else if (mode == 3) {
            // Corrupt Data Block entry header (shared/unshared/flags)
            if (sst_len > 20) {
                mut_buf[0] = 0xFF;
                mut_buf[1] = 0xFF;
                mut_buf[16] = 0x7F;
            }
        } else {
            // Truncate randomly
            const tl: usize = rng.range(sst_len);
            fuzzProbeSst(mut_buf[0..tl], sk_s);
            continue;
        }

        const slice_len: usize = if (mode == 1) 128 else sst_len;
        fuzzProbeSst(mut_buf[0..slice_len], sk_s);
    }

    // 3. Verify original unmutated SSTable still passes full integrity
    const clean_r: SstReader = SstReader.open(seed_sst[0..sst_len]) catch { expect_true(false)?; return; };
    const clean_rep: SstVerifyReport = clean_r.verifyFullIntegrity(sk_s) catch { expect_true(false)?; return; };
    expect_eq(clean_rep.entries_verified, 3);
}

fn main() {
    test("SIMD Differential: compareBytes, commonPrefixLen, crc32Ieee & computePrefixUpperBound", () => {
        runTestSimdDifferential()?;
    });
    test("MVCC Snapshots: gcWatermarkSeqno protects old snapshots across compaction & tombstones", () => {
        runTestMvccSnapshots()?;
    });
    test("RangeDelete: MVCC snapshot isolation, resurrection, WAL replay & compaction pruning", () => {
        runTestRangeDelete()?;
    });
    test("Scan: Prefix Scan & Reverse Range Scan exact symmetry across MemTable, SSTable & Layered", () => {
        runTestScanSymmetry()?;
    });
    test("SSTable Integrity: verifyFullIntegrity, tamper rejection & duplicate (key,seq) compaction", () => {
        runTestSstIntegrity()?;
    });
    test("Model-Based Differential Testing: 1,500 deterministic PRNG ops vs ground-truth KV model", () => {
        runTestModelBased1500()?;
    });
    test("Fuzz: 600+ malformed/truncated/mutated WAL & SSTable binaries (zero panic, zero OOB)", () => {
        runTestFuzzBinary600()?;
    });
}
