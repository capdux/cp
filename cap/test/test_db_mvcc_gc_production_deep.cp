// Origin: Pure-CP Stdlib Test Suite (lib/db/mvcc_gc.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc MVCC Snapshot Isolation & Safe-Horizon GC Store:
//   1. Multi-version writes & exact time-travel snapshot reads across historical timestamps
//   2. Tombstone deletion: invisible to new snapshots (is_deleted=true) while old snapshots still read history
//   3. Long-running active read transaction pins safe_horizon_ts and shields older versions from GC
//   4. Reader release advances safe_horizon_ts and reclaims obsolete overwritten versions cleanly
//   5. Orphaned tombstone below safe_horizon_ts is fully reclaimed, while tombstones with newer versions stay intact
//   6. Per-key non-monotonic commit_ts rejection protecting historical snapshot immutability
//   7. Boundary guards: duplicate tx_id, 32-reader cap, 256-version pool cap & post-GC slot reuse

import { test, expect_eq } from "cap:test";
import {
    MVCC_GC_MAX_VERSIONS,
    MVCC_GC_MAX_READERS,
    MvccReadResult,
    MvccGcReport,
    MvccGcStats,
    MvccVersionGcStore,
} from "cap:db";

// ============================================================================
// Test 1: Multi-Version Time-Travel Snapshot Reads
// ============================================================================
test("mvcc_gc: multi-version writes & exact historical snapshot visibility", () => {
    expect_eq(MVCC_GC_MAX_VERSIONS, 256);
    expect_eq(MVCC_GC_MAX_READERS, 32);

    let store = MvccVersionGcStore.init();
    const k: u64 = 0x1111;

    expect_eq(store.write_version(k, 10, 100, false), true);
    expect_eq(store.write_version(k, 20, 200, false), true);
    expect_eq(store.write_version(k, 30, 300, false), true);

    // Before first commit -> not found, not deleted
    const r5: MvccReadResult = store.read_visible(k, 5);
    expect_eq(r5.found, false);
    expect_eq(r5.is_deleted, false);

    // Exact & intermediate snapshots return newest version <= snapshot_ts
    const r10 = store.read_visible(k, 10);
    expect_eq(r10.found, true);
    expect_eq(r10.val_tag, 100);
    expect_eq(r10.commit_ts, 10);

    const r25 = store.read_visible(k, 25);
    expect_eq(r25.found, true);
    expect_eq(r25.val_tag, 200);
    expect_eq(r25.commit_ts, 20);

    const r99 = store.read_visible(k, 99);
    expect_eq(r99.found, true);
    expect_eq(r99.val_tag, 300);
    expect_eq(r99.commit_ts, 30);
});

// ============================================================================
// Test 2: Tombstone Visibility Isolation Between Old & New Snapshots
// ============================================================================
test("mvcc_gc: tombstone hides key from new snapshots while old snapshot reads history", () => {
    let store = MvccVersionGcStore.init();
    const k: u64 = 0x2222;

    expect_eq(store.write_version(k, 10, 42, false), true);
    expect_eq(store.write_version(k, 20, 0, true), true); // delete tombstone

    // Old snapshot at ts=15 still reads value 42
    const old_read = store.read_visible(k, 15);
    expect_eq(old_read.found, true);
    expect_eq(old_read.is_deleted, false);
    expect_eq(old_read.val_tag, 42);

    // New snapshot at ts=20 or ts=50 sees deletion tombstone
    const del_read = store.read_visible(k, 20);
    expect_eq(del_read.found, false);
    expect_eq(del_read.is_deleted, true);
    expect_eq(del_read.commit_ts, 20);
});

// ============================================================================
// Test 3: Active Long-Running Reader Pins safe_horizon_ts & Protects History
// ============================================================================
test("mvcc_gc: active reader pins safe_horizon_ts & protects required old versions", () => {
    let store = MvccVersionGcStore.init();
    const k: u64 = 0x3333;

    expect_eq(store.write_version(k, 10, 1, false), true);
    expect_eq(store.write_version(k, 20, 2, false), true);
    expect_eq(store.write_version(k, 30, 3, false), true);
    expect_eq(store.write_version(k, 40, 4, false), true);

    // Two readers: tx 1 pinned at ts=25, tx 2 pinned at ts=35 -> safe_horizon_ts == 25
    expect_eq(store.begin_read_tx(1, 25), true);
    expect_eq(store.begin_read_tx(2, 35), true);
    expect_eq(store.safe_horizon_ts(), 25);

    // GC with horizon=25:
    // - Keeps ts=30, ts=40 (> horizon)
    // - Keeps ts=20 (newest <= horizon=25, baseline for tx 1)
    // - Reclaims ONLY ts=10!
    const rep: MvccGcReport = store.collect_garbage();
    expect_eq(rep.safe_horizon_ts, 25);
    expect_eq(rep.reclaimed_versions, 1);
    expect_eq(rep.reclaimed_tombstones, 0);
    expect_eq(rep.remaining_versions, 3);

    // Both active readers still observe exact snapshot values
    expect_eq(store.read_visible(k, 25).val_tag, 2);
    expect_eq(store.read_visible(k, 35).val_tag, 3);
    expect_eq(store.read_visible(k, 40).val_tag, 4);
});

// ============================================================================
// Test 4: Reader Completion Advances Safe Horizon & Reclaims Remaining History
// ============================================================================
test("mvcc_gc: ending readers advances horizon & reclaims obsolete versions", () => {
    let store = MvccVersionGcStore.init();
    const k: u64 = 0x4444;

    expect_eq(store.write_version(k, 10, 101, false), true);
    expect_eq(store.write_version(k, 20, 102, false), true);
    expect_eq(store.write_version(k, 30, 103, false), true);

    expect_eq(store.begin_read_tx(10, 15), true);
    expect_eq(store.collect_garbage().reclaimed_versions, 0); // ts=10 is baseline for 15, ts=20 & 30 > 15

    // End reader tx 10 -> no active readers -> horizon advances to latest_commit_ts (30)
    expect_eq(store.end_read_tx(10), true);
    expect_eq(store.end_read_tx(10), false); // already ended
    expect_eq(store.safe_horizon_ts(), 30);

    const rep = store.collect_garbage();
    expect_eq(rep.safe_horizon_ts, 30);
    expect_eq(rep.reclaimed_versions, 2); // ts=10 & ts=20 reclaimed
    expect_eq(rep.remaining_versions, 1); // ts=30 retained as baseline
    expect_eq(store.read_visible(k, 30).val_tag, 103);

    const st: MvccGcStats = store.stats();
    expect_eq(st.active_readers, 0);
    expect_eq(st.live_versions, 1);
    expect_eq(st.total_reclaimed, 2);
});

// ============================================================================
// Test 5: Orphaned Tombstone Reclamation vs Active Newer-Version Preservation
// ============================================================================
test("mvcc_gc: orphaned tombstones reclaimed cleanly while non-orphan tombstones preserved", () => {
    let store = MvccVersionGcStore.init();
    const k_orphan: u64 = 0xAAAA;
    const k_reborn: u64 = 0xBBBB;

    // k_orphan: written at 10, deleted at 20 (no newer versions)
    expect_eq(store.write_version(k_orphan, 10, 1, false), true);
    expect_eq(store.write_version(k_orphan, 20, 0, true), true);

    // k_reborn: written at 10, deleted at 20, re-inserted at 30
    expect_eq(store.write_version(k_reborn, 10, 1, false), true);
    expect_eq(store.write_version(k_reborn, 20, 0, true), true);
    expect_eq(store.write_version(k_reborn, 30, 99, false), true);

    // Pin reader at ts=25:
    // - k_orphan has baseline ts=20 (tombstone) and NO newer versions -> BOTH ts=10 and tombstone ts=20 reclaimed!
    // - k_reborn has baseline ts=20 (tombstone) AND newer ts=30 -> ts=10 reclaimed, tombstone ts=20 KEPT for ts=25 reader!
    expect_eq(store.begin_read_tx(1, 25), true);
    const rep1 = store.collect_garbage();
    expect_eq(rep1.reclaimed_versions, 3);  // k_orphan(10, 20) + k_reborn(10)
    expect_eq(rep1.reclaimed_tombstones, 1); // k_orphan(20)
    expect_eq(rep1.remaining_versions, 2);  // k_reborn(20, 30)

    // Reader at ts=25 still sees k_reborn as deleted, and ts=30 sees val 99
    expect_eq(store.read_visible(k_reborn, 25).is_deleted, true);
    expect_eq(store.read_visible(k_reborn, 30).val_tag, 99);

    // Release reader -> horizon advances to 30 -> k_reborn tombstone at ts=20 is now reclaimed too!
    expect_eq(store.end_read_tx(1), true);
    const rep2 = store.collect_garbage();
    expect_eq(rep2.reclaimed_versions, 1);
    expect_eq(rep2.reclaimed_tombstones, 1);
    expect_eq(rep2.remaining_versions, 1);
    expect_eq(store.read_visible(k_reborn, 30).val_tag, 99);
});

// ============================================================================
// Test 6: Non-Monotonic Per-Key commit_ts Rejection
// ============================================================================
test("mvcc_gc: rejects non-monotonic per-key commit_ts writes", () => {
    let store = MvccVersionGcStore.init();
    expect_eq(store.write_version(1, 50, 500, false), true);
    // Equal or older commit_ts for the same key must fail
    expect_eq(store.write_version(1, 50, 501, false), false);
    expect_eq(store.write_version(1, 49, 499, false), false);
    // Another independent key CAN still write at ts=40
    expect_eq(store.write_version(2, 40, 400, false), true);
    expect_eq(store.stats().live_versions, 2);
});

// ============================================================================
// Test 7: Reader Cap (32), Duplicate tx_id & 256-Version Pool Reuse After GC
// ============================================================================
test("mvcc_gc: 32-reader cap, duplicate tx_id & 256-version pool slot reuse after GC", () => {
    let store = MvccVersionGcStore.init();

    // Register 32 distinct readers -> 33rd fails; duplicate tx_id fails
    let tx: u64 = 1;
    while (tx <= 32) : (tx += 1) {
        expect_eq(store.begin_read_tx(tx, tx * 10), true);
    }
    expect_eq(store.begin_read_tx(1, 10), false);
    expect_eq(store.begin_read_tx(33, 330), false);
    expect_eq(store.stats().active_readers, MVCC_GC_MAX_READERS);

    // Release all 32 readers
    let rel: u64 = 1;
    while (rel <= 32) : (rel += 1) {
        expect_eq(store.end_read_tx(rel), true);
    }
    expect_eq(store.stats().active_readers, 0);

    // Fill all 256 version slots on key 7 -> 257th fails cleanly
    let ts: u64 = 1;
    while (ts <= 256) : (ts += 1) {
        expect_eq(store.write_version(7, ts, ts * 10, false), true);
    }
    expect_eq(store.write_version(7, 257, 2570, false), false);
    expect_eq(store.stats().live_versions, MVCC_GC_MAX_VERSIONS);

    // GC with no active readers reclaims 255 obsolete versions -> 255 slots freed for reuse!
    const rep = store.collect_garbage();
    expect_eq(rep.reclaimed_versions, 255);
    expect_eq(rep.remaining_versions, 1);

    // New write at ts=257 succeeds cleanly using reclaimed slots
    expect_eq(store.write_version(7, 257, 2570, false), true);
    expect_eq(store.read_visible(7, 257).val_tag, 2570);
});
