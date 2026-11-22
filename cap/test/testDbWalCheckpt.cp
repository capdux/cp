// Origin: Pure-CP Stdlib Test Suite (lib/db/walCheckpt.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc ARIES-Style WAL Fuzzy Checkpoint & Truncation Coordinator:
//   1. Exact ARIES watermark calculation: `redo_lsn` from DPT `rec_lsn` & `safe_truncate_lsn` pinned by long txn
//   2. Fuzzy page flush race guard: `markPageFlushed` clears page ONLY when `flushed_lsn >= last_lsn`
//   3. WAL segment boundary & open-segment protection (`end_lsn < safe_truncate_lsn && closed`)
//   4. Checkpoint commit advancement, batch `reclaimSafeSegments` & telemetry stats
//   5. Out-of-order segment registration produces ascending `reclaimableIds()` order
//   6. 128-page / 32-txn / 32-segment capacity & invalid/duplicate argument guards
//   7. `cap.db` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    WAL_CKPT_MAX_DIRTY_PAGES,
    WAL_CKPT_MAX_ACTIVE_TXNS,
    WAL_CKPT_MAX_SEGMENTS,
    WalCheckptOpStatus,
    WalDirtyPageEntry,
    WalCheckpointCoordinator,
} from "cap:db";

test("WalCheckpt: ARIES redo_lsn & active long-txn safe_truncate_lsn pinning") {
    let ckpt = WalCheckpointCoordinator.init();

    // Empty DPT & no active txns -> redo_lsn == safe_truncate_lsn == current_flushed_lsn
    const p0 = ckpt.computeCheckpointPlan(500);
    expect_eq(p0.redo_lsn, 500 as u64);
    expect_eq(p0.safe_truncate_lsn, 500 as u64);

    // Page 10 dirtied at 200, then updated at 260 -> rec_lsn stays 200, last_lsn becomes 260
    expect_eq(ckpt.markPageDirty(10, 200), WalCheckptOpStatus.ok);
    expect_eq(ckpt.markPageDirty(10, 260), WalCheckptOpStatus.ok);
    const dp10 = ckpt.getDirtyPage(10) ?? WalDirtyPageEntry{};
    expect_eq(dp10.rec_lsn, 200 as u64);
    expect_eq(dp10.last_lsn, 260 as u64);

    // Page 20 dirtied at 150 -> redo_lsn drops to 150
    expect_eq(ckpt.markPageDirty(20, 150), WalCheckptOpStatus.ok);
    const p1 = ckpt.computeCheckpointPlan(500);
    expect_eq(p1.redo_lsn, 150 as u64);
    expect_eq(p1.safe_truncate_lsn, 150 as u64);

    // Long-running snapshot txn 99 pins LSN 120 (< redo_lsn 150) -> safe_truncate_lsn pinned at 120!
    expect_eq(ckpt.beginTxn(99, 120), WalCheckptOpStatus.ok);
    expect_eq(ckpt.beginTxn(100, 300), WalCheckptOpStatus.ok);
    const p2 = ckpt.computeCheckpointPlan(500);
    expect_eq(p2.redo_lsn, 150 as u64);
    expect_eq(p2.min_active_tx_lsn, 120 as u64);
    expect_eq(p2.safe_truncate_lsn, 120 as u64);

    // Ending long txn 99 unpins safe_truncate_lsn back up to redo_lsn (150)
    expect_eq(ckpt.endTxn(99), WalCheckptOpStatus.ok);
    const p3 = ckpt.computeCheckpointPlan(500);
    expect_eq(p3.min_active_tx_lsn, 300 as u64);
    expect_eq(p3.safe_truncate_lsn, 150 as u64);
}

test("WalCheckpt: fuzzy checkpoint flush race guard (flushed_lsn >= last_lsn)") {
    let ckpt = WalCheckpointCoordinator.init();
    * = ckpt.markPageDirty(10, 100);
    * = ckpt.markPageDirty(10, 150); // concurrent write after flush started
    * = ckpt.markPageDirty(20, 220);

    // Flushing snapshot from LSN 120 (< last_lsn 150) MUST retain page 10 in DPT!
    expect_true(!ckpt.markPageFlushed(10, 120));
    expect_true(ckpt.getDirtyPage(10) != null);
    expect_eq(ckpt.computeCheckpointPlan(300).redo_lsn, 100 as u64);

    // Flushing up to LSN 150 (== last_lsn) cleanly clears page 10 -> redo_lsn advances to 220!
    expect_true(ckpt.markPageFlushed(10, 150));
    expect_true(ckpt.getDirtyPage(10) == null);
    expect_eq(ckpt.computeCheckpointPlan(300).redo_lsn, 220 as u64);

    // Flushing non-existent page returns false safely
    expect_true(!ckpt.markPageFlushed(10, 200));
}

test("WalCheckpt: WAL segment truncation boundary & open-segment protection") {
    let ckpt = WalCheckpointCoordinator.init();
    expect_eq(ckpt.registerSegment(1, 1, 100, 4096, true), WalCheckptOpStatus.ok);
    expect_eq(ckpt.registerSegment(2, 101, 200, 4096, true), WalCheckptOpStatus.ok);
    expect_eq(ckpt.registerSegment(3, 201, 250, 2048, false), WalCheckptOpStatus.ok); // still open!

    // Pin watermark at 200: only seg 1 (end_lsn 100 < 200) is reclaimable; seg 2 (200 == 200) is NOT!
    * = ckpt.markPageDirty(7, 200);
    const plan = ckpt.computeCheckpointPlan(300);
    expect_eq(plan.safe_truncate_lsn, 200 as u64);
    expect_eq(plan.reclaimable_seg_count, 1 as usize);
    expect_eq(plan.reclaimable_bytes, 4096 as u64);
    expect_eq(plan.reclaimableIds()[0], 1 as u64);

    // Attempting to reclaim boundary seg 2 or open seg 3 is rejected
    expect_eq(ckpt.reclaimSegment(2, plan.safe_truncate_lsn), WalCheckptOpStatus.segment_not_reclaimable);
    expect_eq(ckpt.reclaimSegment(3, 500), WalCheckptOpStatus.segment_not_reclaimable);

    // Reclaiming seg 1 succeeds
    expect_eq(ckpt.reclaimSegment(1, plan.safe_truncate_lsn), WalCheckptOpStatus.ok);

    // Close seg 3 and flush page 7 -> now both seg 2 and seg 3 become reclaimable at LSN 300!
    expect_eq(ckpt.closeSegment(3, 250, 2048), WalCheckptOpStatus.ok);
    expect_true(ckpt.markPageFlushed(7, 200));
    const plan2 = ckpt.computeCheckpointPlan(300);
    expect_eq(plan2.reclaimable_seg_count, 2 as usize);
    expect_eq(plan2.reclaimable_bytes, 6144 as u64);
}

test("WalCheckpt: commitCheckpoint, batch reclaimSafeSegments & telemetry stats") {
    let ckpt = WalCheckpointCoordinator.init();
    * = ckpt.registerSegment(10, 1, 99, 8192, true);
    * = ckpt.registerSegment(11, 100, 199, 8192, true);
    * = ckpt.registerSegment(12, 200, 299, 8192, true);
    * = ckpt.markPageDirty(1, 210);

    const p = ckpt.commitCheckpoint(250, 250);
    expect_eq(p.safe_truncate_lsn, 210 as u64);
    expect_eq(p.reclaimable_seg_count, 2 as usize);

    expect_eq(ckpt.reclaimSafeSegments(p.safe_truncate_lsn), 2 as usize);
    const st = ckpt.stats(250);
    expect_eq(st.segment_count, 1 as usize);
    expect_eq(st.last_checkpoint_lsn, 250 as u64);
    expect_eq(st.last_redo_lsn, 210 as u64);
    expect_eq(st.last_safe_truncate_lsn, 210 as u64);
    expect_eq(st.total_checkpoints_committed, 1 as u64);
    expect_eq(st.total_segments_reclaimed, 2 as u64);
    expect_eq(st.total_bytes_reclaimed, 16384 as u64);
}

test("WalCheckpt: out-of-order segment registration yields ascending reclaimableIds()") {
    let ckpt = WalCheckpointCoordinator.init();
    * = ckpt.registerSegment(30, 201, 300, 1024, true);
    * = ckpt.registerSegment(10, 1, 100, 1024, true);
    * = ckpt.registerSegment(20, 101, 200, 1024, true);

    const plan = ckpt.computeCheckpointPlan(500);
    expect_eq(plan.reclaimable_seg_count, 3 as usize);
    const ids = plan.reclaimableIds();
    expect_eq(ids[0], 10 as u64);
    expect_eq(ids[1], 20 as u64);
    expect_eq(ids[2], 30 as u64);
}

test("WalCheckpt: 128-page / 32-txn / 32-segment capacity & invalid-arg guards") {
    let ckpt = WalCheckpointCoordinator.init();

    // Invalid args
    expect_eq(ckpt.markPageDirty(0, 10), WalCheckptOpStatus.invalid_arg);
    expect_eq(ckpt.markPageDirty(1, 0), WalCheckptOpStatus.invalid_arg);
    expect_eq(ckpt.beginTxn(0, 10), WalCheckptOpStatus.invalid_arg);
    expect_eq(ckpt.registerSegment(1, 50, 40, 100, true), WalCheckptOpStatus.invalid_arg);

    // Fill 128 dirty pages
    let p: u64 = 1;
    while (p <= WAL_CKPT_MAX_DIRTY_PAGES as u64) : (p += 1) {
        expect_eq(ckpt.markPageDirty(p, p * 10), WalCheckptOpStatus.ok);
    }
    expect_eq(ckpt.markPageDirty(999, 100), WalCheckptOpStatus.capacity_full);

    // Fill 32 active txns
    let t: u64 = 1;
    while (t <= WAL_CKPT_MAX_ACTIVE_TXNS as u64) : (t += 1) {
        expect_eq(ckpt.beginTxn(t, t * 5), WalCheckptOpStatus.ok);
    }
    expect_eq(ckpt.beginTxn(1, 5), WalCheckptOpStatus.duplicate_id);
    expect_eq(ckpt.beginTxn(999, 5), WalCheckptOpStatus.capacity_full);

    // Fill 32 WAL segments
    let s: u64 = 1;
    while (s <= WAL_CKPT_MAX_SEGMENTS as u64) : (s += 1) {
        expect_eq(ckpt.registerSegment(s, s * 10, s * 10 + 9, 512, true), WalCheckptOpStatus.ok);
    }
    expect_eq(ckpt.registerSegment(1, 1, 2, 512, true), WalCheckptOpStatus.duplicate_id);
    expect_eq(ckpt.registerSegment(999, 1, 2, 512, true), WalCheckptOpStatus.capacity_full);
}

test("WalCheckpt: cap.db namespace facade parity") {
    let ckpt = cap.db.WalCheckpointCoordinator.init();
    expect_eq(ckpt.registerSegment(1, 1, 50, 2048, true), cap.db.WalCheckptOpStatus.ok);
    expect_eq(ckpt.markPageDirty(5, 80), cap.db.WalCheckptOpStatus.ok);
    const plan = ckpt.commitCheckpoint(100, 100);
    expect_eq(plan.redo_lsn, 80 as u64);
    expect_eq(plan.safe_truncate_lsn, 80 as u64);
    expect_eq(plan.reclaimable_seg_count, 1 as usize);
    expect_eq(ckpt.reclaimSafeSegments(plan.safe_truncate_lsn), 1 as usize);
}
