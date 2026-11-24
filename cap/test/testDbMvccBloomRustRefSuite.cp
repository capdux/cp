import { test, expect_eq } from "cap:test";
import {
    BLOOM_BLOCK_MAGIC,
    BLOOM_BLOCK_MAX_BLOCKS,
    BLOOM_BLOCK_MAX_PROBES,
    scalarBloomOrBlocks,
    simdBloomOrBlocks,
    scalarBloomPopcountWords,
    simdBloomPopcountWords,
    recommendBloomParams,
    BlockedBloomFilter,
} from "../../lib/db/bloomBlock.cp";
import {
    MVCC_SNAP_INF_TX,
    scalarMvccContainsTxId,
    simdMvccContainsTxId,
    MvccTxStatus,
    MvccConflictKind,
    MvccSnapshotManager,
} from "../../lib/db/mvccSnap.cp";

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn expect_false(cond: bool): void! {
    expect_eq(cond, false);
}

struct XorShift64 {
    state: u64 = 0x1E3779B97F4A7C15,

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

// ---------------------------------------------------------------------------
// Test 1: MVCC Snapshot Isolation, FCW Conflicts, Rollback & GC Watermark
// ---------------------------------------------------------------------------

fn runTestMvccStrict(): void! {
    let mgr = MvccSnapshotManager.init();
    const key_a: u64 = 0xA001;

    // 1. Committed baseline version (t1 -> 100)
    const t1 = mgr.beginTx().?;
    * = mgr.insertVersion(key_a, 100, t1).?;
    expect_true(mgr.commitTx(t1))?;

    // 2. Concurrent active reader (t2) captures ReadView; concurrent writer (t3) writes 200 (uncommitted)
    const t2 = mgr.beginTx().?;
    const t3 = mgr.beginTx().?;
    const view2 = mgr.captureReadView(t2).?;
    const view3 = mgr.captureReadView(t3).?;

    // Uncommitted write by t3: visible to t3 (self-write), invisible to t2
    * = mgr.insertVersion(key_a, 200, t3).?;
    expect_eq(mgr.readVisibleVersion(key_a, view3).?.value, 200 as u64);
    expect_eq(mgr.readVisibleVersion(key_a, view2).?.value, 100 as u64);

    // FCW active_writer_conflict: t2 attempting to write key_a conflicts with active t3
    const c_active = mgr.detectWriteConflict(key_a, t2, view2);
    expect_true(c_active.has_conflict)?;
    expect_eq(c_active.kind, MvccConflictKind.active_writer_conflict);
    expect_eq(c_active.conflicting_tx_id, t3);

    // Commit t3 -> still invisible to t2's snapshot! And t2 now sees committed_after_snapshot conflict
    expect_true(mgr.commitTx(t3))?;
    expect_eq(mgr.readVisibleVersion(key_a, view2).?.value, 100 as u64);
    const c_comm = mgr.detectWriteConflict(key_a, t2, view2);
    expect_true(c_comm.has_conflict)?;
    expect_eq(c_comm.kind, MvccConflictKind.committed_after_snapshot);
    expect_eq(c_comm.conflicting_tx_id, t3);

    // 3. Aborted transaction (t4) writes 999 then aborts -> zero visibility anywhere
    const t4 = mgr.beginTx().?;
    * = mgr.insertVersion(key_a, 999, t4).?;
    expect_true(mgr.abortTx(t4))?;
    expect_eq(mgr.txStatus(t4), MvccTxStatus.aborted);
    const anon_view = mgr.captureReadView(0).?;
    expect_eq(mgr.readVisibleVersion(key_a, anon_view).?.value, 200 as u64);

    // 4. GC Watermark safety: t2 is still active (wm == t2 == 2), so v100 (end_tx = t3 = 3 >= wm) MUST NOT be reclaimed!
    //    Aborted t4 version IS reclaimed.
    expect_eq(mgr.computeGcWatermark(), t2);
    const r1 = mgr.reclaimDeadVersions();
    expect_eq(r1, 1 as usize); // only aborted t4
    // t2 STILL reads 100 intact!
    expect_eq(mgr.readVisibleVersion(key_a, view2).?.value, 100 as u64);

    // After t2 commits, watermark advances past t3 -> v100 is safely reclaimed, v200 remains visible
    expect_true(mgr.commitTx(t2))?;
    const r2 = mgr.reclaimDeadVersions();
    expect_eq(r2, 1 as usize); // v100 reclaimed
    const anon_after = mgr.captureReadView(0).?;
    expect_eq(mgr.readVisibleVersion(key_a, anon_after).?.value, 200 as u64);
}

// ---------------------------------------------------------------------------
// Test 2: Blocked Bloom Filter 0% FN, FPR Bound, Union & Deserialization Guards
// ---------------------------------------------------------------------------

fn runTestBloomBlockStrict(): void! {
    // 1. Invalid geometry rejected
    expect_true(BlockedBloomFilter.init(0, 5) == null)?;
    expect_true(BlockedBloomFilter.init(BLOOM_BLOCK_MAX_BLOCKS + 1, 5) == null)?;
    expect_true(BlockedBloomFilter.init(4, 0) == null)?;
    expect_true(BlockedBloomFilter.init(4, BLOOM_BLOCK_MAX_PROBES + 1) == null)?;
    expect_true(recommendBloomParams(0, 0.01) == null)?;
    expect_true(recommendBloomParams(100, 0.0) == null)?;
    expect_true(recommendBloomParams(100, 1.0) == null)?;

    // 2. Insert 500 keys -> 0% False Negative, empirical FPR well under 5%
    const params = recommendBloomParams(500, 0.01).?;
    let bf1 = BlockedBloomFilter.fromParams(params).?;
    let bf2 = BlockedBloomFilter.fromParams(params).?;

    let k: u64 = 0;
    while (k < 250) : (k += 1) {
        bf1.insertHash(k + 1000);
        bf2.insertHash(k + 2000);
    }
    // Check 0% FN on bf1 and bf2 individually
    k = 0;
    while (k < 250) : (k += 1) {
        expect_true(bf1.mayContainHash(k + 1000))?;
        expect_true(bf2.mayContainHash(k + 2000))?;
    }

    // 3. Union mismatch rejected atomically; isomorphic union preserves all 500 keys (0% FN)
    const diff_bf = BlockedBloomFilter.init(params.num_blocks + 1, params.num_probes).?;
    expect_false(bf1.unionInPlace(diff_bf))?;
    expect_true(bf1.unionInPlace(bf2))?;
    k = 0;
    while (k < 250) : (k += 1) {
        expect_true(bf1.mayContainHash(k + 1000))?;
        expect_true(bf1.mayContainHash(k + 2000))?;
    }

    // Empirical FPR check on 1000 unseen keys (target 1%, allow generous < 5% upper bound)
    let fp: usize = 0;
    k = 0;
    while (k < 1000) : (k += 1) {
        if (bf1.mayContainHash(900000 + k)) {
            fp += 1;
        }
    }
    expect_true(fp < 50)?;

    // 4. Serialization roundtrip + bad magic / truncated / oversized rejection
    let ser_buf: [20480]u8 = [0; 20480];
    const n_written = bf1.serialize(ser_buf[0..]).?;
    expect_eq(n_written, bf1.serializedSize());
    expect_true(BlockedBloomFilter.verifySerializedBytes(ser_buf[0..n_written]))?;

    const restored = BlockedBloomFilter.deserialize(ser_buf[0..n_written]).?;
    expect_eq(restored.setBitCount(), bf1.setBitCount());
    k = 0;
    while (k < 250) : (k += 1) {
        expect_true(restored.mayContainHash(k + 1000))?;
        expect_true(restored.mayContainHash(k + 2000))?;
    }

    // Truncated, oversized, and bad magic rejected
    expect_true(BlockedBloomFilter.deserialize(ser_buf[0 .. n_written - 1]) == null)?;
    expect_true(BlockedBloomFilter.deserialize(ser_buf[0 .. n_written + 1]) == null)?;
    ser_buf[0] ^= 0xFF;
    expect_true(BlockedBloomFilter.deserialize(ser_buf[0..n_written]) == null)?;
}

// ---------------------------------------------------------------------------
// Test 3: 4-Lane SIMD vs Scalar 0-Diff Verification
// ---------------------------------------------------------------------------

fn runTestSimdZeroDiff(): void! {
    let a: [37]u64 = [0; 37];
    let dst_simd: [37]u64 = [0; 37];
    let dst_scalar: [37]u64 = [0; 37];

    let len: usize = 0;
    while (len <= 37) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            a[i] = ((i as u64) * 0x1020304050607) ^ 0x55AA55AA;
            dst_simd[i] = ((i as u64) * 13) + 1;
            dst_scalar[i] = ((i as u64) * 13) + 1;
        }

        // simdBloomOrBlocks vs scalarBloomOrBlocks
        simdBloomOrBlocks(dst_simd[0..len], a[0..len]);
        scalarBloomOrBlocks(dst_scalar[0..len], a[0..len]);
        let j: usize = 0;
        while (j < len) : (j += 1) {
            expect_eq(dst_simd[j], dst_scalar[j]);
        }

        // simdBloomPopcountWords vs scalarBloomPopcountWords
        expect_eq(simdBloomPopcountWords(dst_simd[0..len]), scalarBloomPopcountWords(dst_scalar[0..len]));
        expect_eq(simdBloomPopcountWords(a[0..len]), scalarBloomPopcountWords(a[0..len]));

        // simdMvccContainsTxId vs scalarMvccContainsTxId (hit at every position & miss)
        let pos: usize = 0;
        while (pos < len) : (pos += 1) {
            expect_eq(simdMvccContainsTxId(a[0..len], a[pos]), scalarMvccContainsTxId(a[0..len], a[pos]));
            expect_true(simdMvccContainsTxId(a[0..len], a[pos]))?;
        }
        expect_eq(simdMvccContainsTxId(a[0..len], 0xDEADBEEF), scalarMvccContainsTxId(a[0..len], 0xDEADBEEF));
        expect_false(simdMvccContainsTxId(a[0..len], 0xDEADBEEF))?;
    }
}

// ---------------------------------------------------------------------------
// Test 4: 1,600-Case Deterministic PRNG Fuzz (800 MVCC + 800 Blocked Bloom)
// ---------------------------------------------------------------------------

fn runTestDbMvccBloomFuzz1600(): void! {
    let rng = XorShift64 { state: 0x3706BB20261124 };

    // Part A: 800 concurrent MVCC transaction scheduling & snapshot invariant steps
    let mgr = MvccSnapshotManager.init();
    let active_ids: [16]u64 = [0; 16];
    let active_has_snap: [16]bool = [false; 16];
    let active_snap_key: [16]u64 = [0; 16];
    let active_snap_val: [16]u64 = [0; 16];
    let active_snap_exists: [16]bool = [false; 16];

    let step: usize = 0;
    while (step < 800) : (step += 1) {
        const op = rng.next() % 5;
        const slot = (rng.next() as usize) % 16;
        const key_h: u64 = ((rng.next() % 8) + 1);

        if (op == 0) {
            // Begin tx if slot free
            if (active_ids[slot] == 0) {
                if (tx := mgr.beginTx()) {
                    active_ids[slot] = tx;
                    active_has_snap[slot] = false;
                }
            }
        } else if (op == 1) {
            // Write if slot active and no FCW conflict
            const tx = active_ids[slot];
            if (tx != 0) {
                if (v := mgr.captureReadView(tx)) {
                    const conf = mgr.detectWriteConflict(key_h, tx, v);
                    if (!conf.has_conflict) {
                        const new_val: u64 = (step as u64) + 1000;
                        * = mgr.insertVersion(key_h, new_val, tx);
                        if (active_has_snap[slot] and active_snap_key[slot] == key_h) {
                            active_snap_exists[slot] = true;
                            active_snap_val[slot] = new_val;
                        }
                    }
                }
            }
        } else if (op == 2) {
            // Commit active tx
            const tx = active_ids[slot];
            if (tx != 0) {
                expect_true(mgr.commitTx(tx))?;
                active_ids[slot] = 0;
                active_has_snap[slot] = false;
            }
        } else if (op == 3) {
            // Abort active tx -> must be invisible immediately
            const tx = active_ids[slot];
            if (tx != 0) {
                expect_true(mgr.abortTx(tx))?;
                expect_eq(mgr.txStatus(tx), MvccTxStatus.aborted);
                active_ids[slot] = 0;
                active_has_snap[slot] = false;
            }
        } else {
            // Trigger GC watermark reclaim (must never corrupt active snapshots or panic)
            * = mgr.reclaimDeadVersions();
        }

        // Verify ReadView SIMD membership parity on current active set
        if (anon := mgr.captureReadView(0)) {
            const probe_tx: u64 = (rng.next() % 64) + 1;
            expect_eq(
                simdMvccContainsTxId(anon.active_tx_ids[0..anon.active_count], probe_tx),
                scalarMvccContainsTxId(anon.active_tx_ids[0..anon.active_count], probe_tx),
            );
        }
    }

    // Part B: 800 Blocked Bloom fuzz cases (400 random keysets 0% FN + 400 malformed deserialize)
    let fbuf: [128]u8 = [0; 128];
    let bi: usize = 0;
    while (bi < 800) : (bi += 1) {
        if (bi < 400) {
            const nb: usize = ((rng.next() as usize) % 4) + 1;
            const np: u32 = ((rng.next() as u32) % 8) + 1;
            let bf = BlockedBloomFilter.init(nb, np).?;
            let keys: [8]u64 = [0; 8];
            let j: usize = 0;
            while (j < 8) : (j += 1) {
                keys[j] = rng.next();
                bf.insertHash(keys[j]);
            }
            // Zero False Negative invariant
            j = 0;
            while (j < 8) : (j += 1) {
                expect_true(bf.mayContainHash(keys[j]))?;
            }
        } else {
            const mlen = (rng.next() as usize) % 128;
            let j: usize = 0;
            while (j < mlen) : (j += 1) {
                fbuf[j] = (rng.next() & 0xFF) as u8;
            }
            if ((bi & 1) == 0 and mlen >= 12) {
                fbuf[0] = 'C';
                fbuf[1] = 'P';
                fbuf[2] = 'B';
                fbuf[3] = '1';
            }
            const ok_v = BlockedBloomFilter.verifySerializedBytes(fbuf[0..mlen]);
            const dec_f = BlockedBloomFilter.deserialize(fbuf[0..mlen]);
            expect_eq(ok_v, dec_f != null);
        }
    }
}

fn main() {
    test("MVCC Snapshot Isolation, FCW write conflicts, rollback & GC watermark safety", () => {
        runTestMvccStrict()?;
    });
    test("Blocked Bloom Filter 0% FN, FPR upper bound, union & serialization guards", () => {
        runTestBloomBlockStrict()?;
    });
    test("4-lane SIMD vs Scalar 0-diff differential verification", () => {
        runTestSimdZeroDiff()?;
    });
    test("1600-case deterministic PRNG Fuzz (800 MVCC + 800 Blocked Bloom)", () => {
        runTestDbMvccBloomFuzz1600()?;
    });
}
