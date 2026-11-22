// Production Deep Test Suite for cap:db MVCC Snapshot Isolation & GC Engine (lib/db/mvcc_snapshot.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    MVCC_SNAP_MAX_ACTIVE_TXNS,
    MVCC_SNAP_INVALID_TX,
    MVCC_SNAP_INF_TX,
    MVCC_SNAP_NO_SLOT,
    MvccTxStatus,
    MvccConflictKind,
    MvccSnapshotManager,
} from "cap:db";

// ============================================================================
// Test 1: Monotonic tx lifecycle, ReadView xmin/xmax & sorted active set
// ============================================================================
test("mvcc_snapshot: monotonic tx lifecycle, ReadView xmin/xmax & sorted active set", fn() {
    let mgr = MvccSnapshotManager.init();

    const t1 = mgr.beginTx() ?? 0;
    const t2 = mgr.beginTx() ?? 0;
    const t3 = mgr.beginTx() ?? 0;
    const t4 = mgr.beginTx() ?? 0;
    expect_eq(t1, 1 as u64);
    expect_eq(t2, 2 as u64);
    expect_eq(t3, 3 as u64);
    expect_eq(t4, 4 as u64);

    // Commit t1 and abort t3 -> active set is {t2=2, t4=4}
    expect_true(mgr.commitTx(t1));
    expect_true(mgr.abortTx(t3));
    expect_eq(mgr.txStatus(t1), MvccTxStatus.committed);
    expect_eq(mgr.txStatus(t2), MvccTxStatus.active);
    expect_eq(mgr.txStatus(t3), MvccTxStatus.aborted);
    expect_eq(mgr.txStatus(t4), MvccTxStatus.active);

    const view = mgr.captureReadView(t2) ?? return;
    expect_eq(view.viewer_tx_id, t2);
    expect_eq(view.xmin, 2 as u64);
    expect_eq(view.xmax, 5 as u64);
    expect_eq(view.active_count, 2 as usize);
    expect_true(view.isTxInActiveSet(t2));
    expect_true(view.isTxInActiveSet(t4));
    expect_true(!view.isTxInActiveSet(t1));
    expect_true(!view.isTxInActiveSet(t3));
    expect_true(!view.isTxInActiveSet(5));
});

// ============================================================================
// Test 2: Strict Snapshot Isolation visibility (self-write, committed, active, future)
// ============================================================================
test("mvcc_snapshot: strict Snapshot Isolation visibility", fn() {
    let mgr = MvccSnapshotManager.init();
    const KEY: u64 = 0x1001;

    // t1 inserts v=100 and commits before snapshot
    const t1 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(KEY, 100, t1);
    expect_true(mgr.commitTx(t1));

    // t2 starts (stays active during snapshot capture)
    const t2 = mgr.beginTx() ?? return;
    // t3 starts and captures ReadView while t2 is active
    const t3 = mgr.beginTx() ?? return;
    const view_t3 = mgr.captureReadView(t3) ?? return;

    // t2 updates KEY -> 200 and commits AFTER t3 captured its ReadView
    * = mgr.insertVersion(KEY, 200, t2);
    expect_true(mgr.commitTx(t2));

    // t4 starts AFTER view_t3 (t4 >= view_t3.xmax), writes 300 and commits
    const t4 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(KEY, 300, t4);
    expect_true(mgr.commitTx(t4));

    // Under Snapshot Isolation, t3 must still see 100 (t1), NOT 200 (t2 was active) and NOT 300 (t4 >= xmax)
    if (v := mgr.readVisibleVersion(KEY, view_t3)) {
        expect_eq(v.value, 100 as u64);
        expect_eq(v.begin_tx, t1);
    } else {
        expect_true(false);
    }

    // Self-write visibility: t3 writes to another key KEY2 -> t3 immediately sees its own uncommitted write
    const KEY2: u64 = 0x2002;
    * = mgr.insertVersion(KEY2, 777, t3);
    if (v_self := mgr.readVisibleVersion(KEY2, view_t3)) {
        expect_eq(v_self.value, 777 as u64);
        expect_eq(v_self.begin_tx, t3);
    } else {
        expect_true(false);
    }

    // Anonymous read-only view (viewer_tx_id = 0) sees t4's 300 on KEY, but NOT t3's uncommitted KEY2
    const ro_view = mgr.captureReadView(MVCC_SNAP_INVALID_TX) ?? return;
    if (v_ro := mgr.readVisibleVersion(KEY, ro_view)) {
        expect_eq(v_ro.value, 300 as u64);
    } else {
        expect_true(false);
    }
    expect_true(mgr.readVisibleVersion(KEY2, ro_view) == null);
});

// ============================================================================
// Test 3: Aborted insert and aborted update/delete rollback invisibility
// ============================================================================
test("mvcc_snapshot: aborted insert and aborted update/delete rollback invisibility", fn() {
    let mgr = MvccSnapshotManager.init();
    const KEY: u64 = 0x3003;

    const t1 = mgr.beginTx() ?? return;
    const s1 = mgr.insertVersion(KEY, 10, t1) ?? MVCC_SNAP_NO_SLOT;
    expect_true(s1 != MVCC_SNAP_NO_SLOT);
    expect_true(mgr.commitTx(t1));

    // t2 updates KEY -> 20 then ABORTS -> t1's version must remain visible!
    const t2 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(KEY, 20, t2);
    expect_true(mgr.abortTx(t2));

    const view_after_abort_update = mgr.captureReadView(0) ?? return;
    if (v := mgr.readVisibleVersion(KEY, view_after_abort_update)) {
        expect_eq(v.value, 10 as u64);
        expect_eq(v.begin_tx, t1);
    } else {
        expect_true(false);
    }

    // t3 deletes KEY then ABORTS -> KEY must still be visible and reclaimable by subsequent t4!
    const t3 = mgr.beginTx() ?? return;
    expect_true(mgr.deleteKey(KEY, t3));
    expect_true(mgr.abortTx(t3));

    const view_after_abort_del = mgr.captureReadView(0) ?? return;
    if (v2 := mgr.readVisibleVersion(KEY, view_after_abort_del)) {
        expect_eq(v2.value, 10 as u64);
    } else {
        expect_true(false);
    }

    // t4 cleanly updates KEY -> 40 after aborted t3 and commits
    const t4 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(KEY, 40, t4);
    expect_true(mgr.commitTx(t4));

    const view_final = mgr.captureReadView(0) ?? return;
    if (v3 := mgr.readVisibleVersion(KEY, view_final)) {
        expect_eq(v3.value, 40 as u64);
        expect_eq(v3.begin_tx, t4);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 4: First-Committer-Wins write-write conflict detection
// ============================================================================
test("mvcc_snapshot: First-Committer-Wins write-write conflict detection", fn() {
    let mgr = MvccSnapshotManager.init();
    const KEY: u64 = 0x4004;

    const t1 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(KEY, 1, t1);
    expect_true(mgr.commitTx(t1));

    // Two concurrent writers t2 and t3 capture snapshots at the same point
    const t2 = mgr.beginTx() ?? return;
    const t3 = mgr.beginTx() ?? return;
    const view_t2 = mgr.captureReadView(t2) ?? return;
    const view_t3 = mgr.captureReadView(t3) ?? return;

    // Neither has written yet -> no conflict
    expect_true(!mgr.detectWriteConflict(KEY, t2, view_t2).has_conflict);
    expect_true(!mgr.detectWriteConflict(KEY, t3, view_t3).has_conflict);

    // t2 writes KEY -> t3 immediately sees active_writer_conflict
    * = mgr.insertVersion(KEY, 2, t2);
    const c_active = mgr.detectWriteConflict(KEY, t3, view_t3);
    expect_true(c_active.has_conflict);
    expect_eq(c_active.kind, MvccConflictKind.active_writer_conflict);
    expect_eq(c_active.conflicting_tx_id, t2);

    // t2 commits -> t3 now sees committed_after_snapshot (First-Committer-Wins)
    expect_true(mgr.commitTx(t2));
    const c_committed = mgr.detectWriteConflict(KEY, t3, view_t3);
    expect_true(c_committed.has_conflict);
    expect_eq(c_committed.kind, MvccConflictKind.committed_after_snapshot);
    expect_eq(c_committed.conflicting_tx_id, t2);

    // Invalid/committed writer tx is flagged as invalid_tx
    const c_inv = mgr.detectWriteConflict(KEY, t2, view_t2);
    expect_true(c_inv.has_conflict);
    expect_eq(c_inv.kind, MvccConflictKind.invalid_tx);
});

// ============================================================================
// Test 5: Multi-version tuple chain & logical delete across concurrent snapshots
// ============================================================================
test("mvcc_snapshot: multi-version tuple chain & logical delete across concurrent snapshots", fn() {
    let mgr = MvccSnapshotManager.init();
    const KEY: u64 = 0x5005;

    const t1 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(KEY, 100, t1);
    expect_true(mgr.commitTx(t1));

    const r1 = mgr.beginTx() ?? return;
    const v_r1 = mgr.captureReadView(r1) ?? return;

    const t2 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(KEY, 200, t2);
    expect_true(mgr.commitTx(t2));

    const r2 = mgr.beginTx() ?? return;
    const v_r2 = mgr.captureReadView(r2) ?? return;

    // t3 deletes KEY and commits
    const t3 = mgr.beginTx() ?? return;
    expect_true(mgr.deleteKey(KEY, t3));
    expect_true(mgr.commitTx(t3));

    const r3 = mgr.beginTx() ?? return;
    const v_r3 = mgr.captureReadView(r3) ?? return;

    // r1 sees 100, r2 sees 200, r3 sees null (deleted)
    if (h1 := mgr.readVisibleVersion(KEY, v_r1)) {
        expect_eq(h1.value, 100 as u64);
    } else {
        expect_true(false);
    }
    if (h2 := mgr.readVisibleVersion(KEY, v_r2)) {
        expect_eq(h2.value, 200 as u64);
    } else {
        expect_true(false);
    }
    expect_true(mgr.readVisibleVersion(KEY, v_r3) == null);
});

// ============================================================================
// Test 6: GC safety watermark, pinned snapshot protection & chain integrity
// ============================================================================
test("mvcc_snapshot: GC watermark, pinned snapshot protection & version reclamation", fn() {
    let mgr = MvccSnapshotManager.init();
    const KEY: u64 = 0x6006;

    // v1 (t1=1) -> v2 (t2=2) -> v3 (t4=4), with long-running reader r (t3=3) pinning watermark at 3
    const t1 = mgr.beginTx() ?? return;
    const s1 = mgr.insertVersion(KEY, 1, t1) ?? MVCC_SNAP_NO_SLOT;
    expect_true(mgr.commitTx(t1));

    const t2 = mgr.beginTx() ?? return;
    const s2 = mgr.insertVersion(KEY, 2, t2) ?? MVCC_SNAP_NO_SLOT;
    expect_true(mgr.commitTx(t2));

    const reader = mgr.beginTx() ?? return; // tx_id = 3 -> pins gc_watermark = 3
    const view_reader = mgr.captureReadView(reader) ?? return;

    const t4 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(KEY, 3, t4);
    expect_true(mgr.commitTx(t4));

    // Also add an aborted write t5 -> always reclaimable regardless of watermark
    const t5 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(KEY, 999, t5);
    expect_true(mgr.abortTx(t5));

    expect_eq(mgr.computeGcWatermark(), reader); // 3
    if (ver1 := mgr.getVersion(s1)) {
        // v1 ended at t2=2 < watermark=3 -> reclaimable!
        expect_true(mgr.canReclaimVersion(ver1, reader));
    } else {
        expect_true(false);
    }
    if (ver2 := mgr.getVersion(s2)) {
        // v2 ended at t4=4 >= watermark=3 -> MUST BE PROTECTED for reader!
        expect_true(!mgr.canReclaimVersion(ver2, reader));
    } else {
        expect_true(false);
    }

    // Reclaim while reader is active: reclaims v1 (s1) + aborted t5 = 2 versions
    const reclaimed1 = mgr.reclaimDeadVersions();
    expect_eq(reclaimed1, 2 as usize);

    // Reader MUST still see v2 = 2 with unbroken chain!
    if (vr := mgr.readVisibleVersion(KEY, view_reader)) {
        expect_eq(vr.value, 2 as u64);
    } else {
        expect_true(false);
    }

    // Once reader finishes, watermark advances to next_tx_id (6), so v2 (end_tx=4 < 6) is reclaimed!
    expect_true(mgr.commitTx(reader));
    expect_eq(mgr.computeGcWatermark(), 6 as u64);
    const reclaimed2 = mgr.reclaimDeadVersions();
    expect_eq(reclaimed2, 1 as usize);

    const st = mgr.stats();
    expect_eq(st.occupied_versions, 1 as usize); // only latest v3 remains
    expect_eq(st.reclaimed_versions, 3 as u64);
    const latest_view = mgr.captureReadView(0) ?? return;
    if (vl := mgr.readVisibleVersion(KEY, latest_view)) {
        expect_eq(vl.value, 3 as u64);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 7: Strict boundary guards (tx_id=0, double commit/abort, full active table)
// ============================================================================
test("mvcc_snapshot: strict boundary guards (tx_id=0, double commit/abort, full active table)", fn() {
    let mgr = MvccSnapshotManager.init();

    // Invalid tx_id=0 and unallocated tx_id
    expect_true(!mgr.commitTx(MVCC_SNAP_INVALID_TX));
    expect_true(!mgr.abortTx(MVCC_SNAP_INVALID_TX));
    expect_true(!mgr.commitTx(999));
    expect_true(!mgr.abortTx(999));
    expect_eq(mgr.txStatus(MVCC_SNAP_INVALID_TX), MvccTxStatus.unknown);
    expect_eq(mgr.txStatus(999), MvccTxStatus.unknown);
    expect_true(mgr.insertVersion(0xAA, 1, MVCC_SNAP_INVALID_TX) == null);
    expect_true(!mgr.deleteKey(0xAA, MVCC_SNAP_INVALID_TX));
    expect_true(mgr.captureReadView(999) == null);

    // Double commit / double abort rejected
    const t1 = mgr.beginTx() ?? return;
    expect_true(mgr.commitTx(t1));
    expect_true(!mgr.commitTx(t1));
    expect_true(!mgr.abortTx(t1));
    expect_true(mgr.captureReadView(t1) == null); // committed tx cannot open active viewer snapshot

    const t2 = mgr.beginTx() ?? return;
    expect_true(mgr.abortTx(t2));
    expect_true(!mgr.abortTx(t2));
    expect_true(!mgr.commitTx(t2));

    // Fill active transaction table to MVCC_SNAP_MAX_ACTIVE_TXNS (64)
    let txs: [MVCC_SNAP_MAX_ACTIVE_TXNS]u64 = [0; MVCC_SNAP_MAX_ACTIVE_TXNS];
    let i: usize = 0;
    while (i < MVCC_SNAP_MAX_ACTIVE_TXNS) : (i += 1) {
        if (tid := mgr.beginTx()) {
            txs[i] = tid;
        } else {
            expect_true(false);
        }
    }
    // 65th concurrent active tx must return null
    expect_true(mgr.beginTx() == null);
    expect_eq(mgr.stats().active_tx_count, MVCC_SNAP_MAX_ACTIVE_TXNS);

    // Freeing one slot allows a new beginTx
    expect_true(mgr.commitTx(txs[0]));
    expect_true(mgr.beginTx() != null);
    expect_true(MVCC_SNAP_INF_TX > 0);
});
