// Reference-hardened against fjall-rs/lsm-tree & rocksdb (MIT OR Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc MVCC Snapshot Isolation ReadView, Visibility & GC Watermark Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation MVCC Snapshot Isolation (ReadView) & version-chain GC engine:
//   - 4-lane @Vector(4, u64) SIMD (`BloomMvccVec4u64`) active-tx membership scan (`simdMvccContainsTxId`)
//     wired directly into `MvccReadView.isTxInActiveSet` / `isVisible` with 0-diff scalar reference
//   - Monotonic transaction ID allocator (`beginTx` / `commitTx` / `abortTx`) with fixed-capacity active table
//   - Standard PostgreSQL / InnoDB style `MvccReadView` (`xmin`, `xmax`, active tx set)
//   - Strict Snapshot Isolation visibility (`isVersionVisible`: self-write, committed < xmax & not in active set,
//     aborted invisible) and First-Committer-Wins write-write conflict detection (`detectWriteConflict`)
//   - Multi-version tuple chain (`begin_tx` / `end_tx` / `prev_version_slot`) read/write/delete operations
//   - Global minimum-active-tx GC safety watermark (`computeGcWatermark` / `canReclaimVersion` / `reclaimDeadVersions`)

const std = @import("std");

/// Module-local 4-lane u64 vector alias for MVCC active transaction table hot path.
type MvccSnapVec4u64 = @Vector(4, u64);

/// Maximum concurrent active transactions tracked in `MvccSnapshotManager`.
pub const MVCC_SNAP_MAX_ACTIVE_TXNS: usize = 64;

/// Maximum transaction outcome history slots retained in the fixed-size status ring.
pub const MVCC_SNAP_MAX_TX_HISTORY: usize = 512;

/// Maximum tuple versions stored across all version chains (zero dynamic heap).
pub const MVCC_SNAP_MAX_VERSIONS: usize = 256;

/// Sentinel invalid transaction ID.
pub const MVCC_SNAP_INVALID_TX: u64 = 0;

/// Sentinel `end_tx` indicating a version has not been superseded or deleted.
pub const MVCC_SNAP_INF_TX: u64 = std.math.maxInt(u64);

/// Sentinel slot index indicating end of a version chain.
pub const MVCC_SNAP_NO_SLOT: u16 = 0xFFFF;

// ============================================================================
// 1. 4-Lane SIMD Active Transaction Table Lookup (0-Diff with Scalar)
// ============================================================================

/// Scalar reference: linear membership check for `target` in `tx_ids`.
pub fn scalarMvccContainsTxId(tx_ids: []const u64, target: u64): bool {
    let i: usize = 0;
    while (i < tx_ids.len) : (i += 1) {
        if (tx_ids[i] == target) return true;
    }
    return false;
}

/// 4-lane SIMD accelerated membership check for `target` in `tx_ids` (wired into ReadView hot path).
pub fn simdMvccContainsTxId(tx_ids: []const u64, target: u64): bool {
    const needle: MvccSnapVec4u64 = @splat(target);
    let i: usize = 0;
    while (i + 4 <= tx_ids.len) {
        const v: MvccSnapVec4u64 = tx_ids[i .. i + 4][0..4].*;
        const eq = v == needle;
        if (@reduce(.Or, eq)) return true;
        i += 4;
    }
    while (i < tx_ids.len) : (i += 1) {
        if (tx_ids[i] == target) return true;
    }
    return false;
}

// ============================================================================
// 2. Public Enums & Data Models
// ============================================================================

/// Lifecycle state of a transaction ID.
pub enum MvccTxStatus {
    unknown,
    active,
    committed,
    aborted,
}

/// Classification of a Snapshot Isolation write-write conflict.
pub enum MvccConflictKind {
    none,
    active_writer_conflict,
    committed_after_snapshot,
    invalid_tx,
}

/// Immutable point-in-time Snapshot Isolation `ReadView` (`xmin` / `xmax` + sorted active tx set).
pub struct MvccReadView {
    viewer_tx_id: u64 = MVCC_SNAP_INVALID_TX;
    xmin: u64 = 1;
    xmax: u64 = 1;
    active_tx_ids: [MVCC_SNAP_MAX_ACTIVE_TXNS]u64 = [0; MVCC_SNAP_MAX_ACTIVE_TXNS];
    active_count: usize = 0;

    /// Searches the active transaction set captured in this `ReadView` using 4-lane SIMD.
    fn isTxInActiveSet(tx_id: u64): bool {
        if (_.active_count == 0 || tx_id < _.xmin || tx_id >= _.xmax) return false;
        return simdMvccContainsTxId(_.active_tx_ids[0.._.active_count], tx_id);
    }

    /// Pure Snapshot Isolation visibility predicate for a committed transaction ID under this `ReadView`
    /// (self-write visible; `< xmin` visible; `>= xmax` invisible; active-set checked via 4-lane SIMD).
    fn isVisible(tx_id: u64): bool {
        if (tx_id == MVCC_SNAP_INVALID_TX || tx_id == MVCC_SNAP_INF_TX) return false;
        if (_.viewer_tx_id != MVCC_SNAP_INVALID_TX && tx_id == _.viewer_tx_id) return true;
        if (tx_id >= _.xmax) return false;
        if (tx_id < _.xmin) return true;
        return !_.isTxInActiveSet(tx_id);
    }
}

/// Single version node in a key's multi-version tuple chain (`[begin_tx, end_tx)`).
pub struct MvccTupleVersion {
    key_hash: u64 = 0;
    value: u64 = 0;
    begin_tx: u64 = MVCC_SNAP_INVALID_TX;
    end_tx: u64 = MVCC_SNAP_INF_TX;
    prev_version_slot: u16 = MVCC_SNAP_NO_SLOT;
    deleted: bool = false;
    occupied: bool = false;
}

/// Outcome of a Snapshot Isolation write-write conflict check.
pub struct MvccWriteConflict {
    has_conflict: bool = false;
    kind: MvccConflictKind = MvccConflictKind.none;
    conflicting_tx_id: u64 = MVCC_SNAP_INVALID_TX;
    version_slot: u16 = MVCC_SNAP_NO_SLOT;
}

/// Operational telemetry and GC watermark snapshot for `MvccSnapshotManager`.
pub struct MvccSnapshotStats {
    next_tx_id: u64 = 1;
    active_tx_count: usize = 0;
    committed_tx_count: u64 = 0;
    aborted_tx_count: u64 = 0;
    occupied_versions: usize = 0;
    reclaimed_versions: u64 = 0;
    gc_watermark: u64 = 1;
}

struct MvccTxHistorySlot {
    tx_id: u64 = MVCC_SNAP_INVALID_TX;
    status: MvccTxStatus = MvccTxStatus.unknown;
}

fn mvccSortU64Asc(arr: *[]u64, count: usize) {
    let i: usize = 1;
    while (i < count) : (i += 1) {
        const key: u64 = arr[i];
        let j: usize = i;
        while (j > 0 && arr[j - 1] > key) : (j -= 1) {
            arr[j] = arr[j - 1];
        }
        arr[j] = key;
    }
}

// ============================================================================
// 3. Zero-Alloc MVCC Snapshot Manager & Version-Chain GC Engine
// ============================================================================

/// Zero-heap-allocation MVCC transaction, `ReadView` visibility, write-conflict, and GC engine.
pub struct MvccSnapshotManager {
    next_tx_id: u64 = 1;
    active_txs: [MVCC_SNAP_MAX_ACTIVE_TXNS]u64 = [0; MVCC_SNAP_MAX_ACTIVE_TXNS];
    active_count: usize = 0;
    history: [MVCC_SNAP_MAX_TX_HISTORY]MvccTxHistorySlot = [MvccTxHistorySlot{}; MVCC_SNAP_MAX_TX_HISTORY];
    versions: [MVCC_SNAP_MAX_VERSIONS]MvccTupleVersion = [MvccTupleVersion{}; MVCC_SNAP_MAX_VERSIONS];
    occupied_version_count: usize = 0;
    committed_tx_count: u64 = 0;
    aborted_tx_count: u64 = 0;
    reclaimed_versions: u64 = 0;

    /// Initializes an empty `MvccSnapshotManager` with `next_tx_id = 1`.
    static fn init(): MvccSnapshotManager {
        return MvccSnapshotManager{
            next_tx_id: 1,
            active_txs: [0; MVCC_SNAP_MAX_ACTIVE_TXNS],
            active_count: 0,
            history: [MvccTxHistorySlot{}; MVCC_SNAP_MAX_TX_HISTORY],
            versions: [MvccTupleVersion{}; MVCC_SNAP_MAX_VERSIONS],
            occupied_version_count: 0,
            committed_tx_count: 0,
            aborted_tx_count: 0,
            reclaimed_versions: 0,
        };
    }

    fn recordHistory(*_, tx_id: u64, status: MvccTxStatus) {
        const idx: usize = (tx_id as usize) % MVCC_SNAP_MAX_TX_HISTORY;
        _.history[idx] = MvccTxHistorySlot{
            tx_id: tx_id,
            status: status,
        };
    }

    fn findActiveSlot(tx_id: u64): usize? {
        if (tx_id == MVCC_SNAP_INVALID_TX) return null;
        if (!simdMvccContainsTxId(_.active_txs[0.._.active_count], tx_id)) return null;
        let i: usize = 0;
        while (i < _.active_count) : (i += 1) {
            if (_.active_txs[i] == tx_id) return i;
        }
        return null;
    }

    /// Returns the lifecycle status of `tx_id` (`unknown` for `0` or unallocated IDs).
    fn txStatus(tx_id: u64): MvccTxStatus {
        if (tx_id == MVCC_SNAP_INVALID_TX || tx_id >= _.next_tx_id) {
            return MvccTxStatus.unknown;
        }
        if (simdMvccContainsTxId(_.active_txs[0.._.active_count], tx_id)) {
            return MvccTxStatus.active;
        }
        const idx: usize = (tx_id as usize) % MVCC_SNAP_MAX_TX_HISTORY;
        if (_.history[idx].tx_id == tx_id) {
            return _.history[idx].status;
        }
        return MvccTxStatus.unknown;
    }

    /// Allocates a new monotonically increasing transaction ID and registers it as active.
    /// Returns `null` if the active transaction table is full (`MVCC_SNAP_MAX_ACTIVE_TXNS`).
    fn beginTx(*_): u64? {
        if (_.active_count >= MVCC_SNAP_MAX_ACTIVE_TXNS) return null;
        if (_.next_tx_id == MVCC_SNAP_INF_TX) return null;

        const tx_id: u64 = _.next_tx_id;
        _.next_tx_id += 1;
        _.active_txs[_.active_count] = tx_id;
        _.active_count += 1;
        _.recordHistory(tx_id, MvccTxStatus.active);
        return tx_id;
    }

    /// Marks an active transaction as `committed`. Rejects `tx_id == 0`, already committed/aborted, or unknown IDs.
    fn commitTx(*_, tx_id: u64): bool {
        if (slot := _.findActiveSlot(tx_id)) {
            _.active_count -= 1;
            _.active_txs[slot] = _.active_txs[_.active_count];
            _.active_txs[_.active_count] = MVCC_SNAP_INVALID_TX;
            _.recordHistory(tx_id, MvccTxStatus.committed);
            _.committed_tx_count += 1;
            return true;
        }
        return false;
    }

    /// Marks an active transaction as `aborted`. Rejects `tx_id == 0`, already committed/aborted, or unknown IDs.
    fn abortTx(*_, tx_id: u64): bool {
        if (slot := _.findActiveSlot(tx_id)) {
            _.active_count -= 1;
            _.active_txs[slot] = _.active_txs[_.active_count];
            _.active_txs[_.active_count] = MVCC_SNAP_INVALID_TX;
            _.recordHistory(tx_id, MvccTxStatus.aborted);
            _.aborted_tx_count += 1;
            return true;
        }
        return false;
    }

    /// Captures a point-in-time Snapshot Isolation `ReadView` (`xmin`, `xmax`, sorted active tx IDs).
    /// Pass `viewer_tx_id = 0` for an anonymous read-only snapshot, or an active transaction ID for self-write visibility.
    /// Returns `null` if a non-zero `viewer_tx_id` is not currently active.
    fn captureReadView(viewer_tx_id: u64): MvccReadView? {
        if (viewer_tx_id != MVCC_SNAP_INVALID_TX && _.findActiveSlot(viewer_tx_id) == null) {
            return null;
        }

        let view = MvccReadView{
            viewer_tx_id: viewer_tx_id,
            xmin: _.next_tx_id,
            xmax: _.next_tx_id,
            active_tx_ids: [0; MVCC_SNAP_MAX_ACTIVE_TXNS],
            active_count: _.active_count,
        };

        let i: usize = 0;
        while (i < _.active_count) : (i += 1) {
            view.active_tx_ids[i] = _.active_txs[i];
        }
        if (view.active_count > 0) {
            let act_s: []u64 = view.active_tx_ids[0..view.active_count];
            mvccSortU64Asc(act_s, view.active_count);
            view.xmin = view.active_tx_ids[0];
        }
        return view;
    }

    /// Determines whether effects produced by `tx_id` are visible under Snapshot Isolation `view`.
    fn isTxVisibleInView(tx_id: u64, view: MvccReadView): bool {
        if (tx_id == MVCC_SNAP_INVALID_TX || tx_id == MVCC_SNAP_INF_TX) return false;

        const st = _.txStatus(tx_id);
        if (st == MvccTxStatus.aborted || st == MvccTxStatus.unknown) {
            return false;
        }

        // Self-write visibility: a transaction always sees its own uncommitted (or committed) writes.
        if (view.viewer_tx_id != MVCC_SNAP_INVALID_TX && tx_id == view.viewer_tx_id) {
            return true;
        }

        // Other transactions must be committed and satisfy `view.isVisible(tx_id)` (4-lane SIMD).
        if (st != MvccTxStatus.committed) return false;
        return view.isVisible(tx_id);
    }

    /// Evaluates Snapshot Isolation visibility of a tuple version `[begin_tx, end_tx)` under `view`.
    /// A version is visible iff its creator (`begin_tx`) is visible AND its terminator (`end_tx`) is NOT visible.
    fn isVersionVisible(version: MvccTupleVersion, view: MvccReadView): bool {
        if (!version.occupied) return false;
        if (!_.isTxVisibleInView(version.begin_tx, view)) return false;
        if (version.end_tx == MVCC_SNAP_INF_TX) return true;
        return !_.isTxVisibleInView(version.end_tx, view);
    }

    fn findLatestHeadSlot(key_hash: u64): u16 {
        let best_slot: u16 = MVCC_SNAP_NO_SLOT;
        let best_begin_tx: u64 = 0;
        let i: usize = 0;
        while (i < MVCC_SNAP_MAX_VERSIONS) : (i += 1) {
            const v = _.versions[i];
            if (!v.occupied || v.key_hash != key_hash) continue;
            if (best_slot == MVCC_SNAP_NO_SLOT || v.begin_tx > best_begin_tx ||
                (v.begin_tx == best_begin_tx && (i as u16) > best_slot)) {
                best_slot = i as u16;
                best_begin_tx = v.begin_tx;
            }
        }
        return best_slot;
    }

    fn allocVersionSlot(): u16? {
        let i: usize = 0;
        while (i < MVCC_SNAP_MAX_VERSIONS) : (i += 1) {
            if (!_.versions[i].occupied) return i as u16;
        }
        return null;
    }

    /// Detects Snapshot Isolation write-write conflicts (First-Committer-Wins) for `writer_tx_id`
    /// intending to modify `key_hash` under `view`.
    fn detectWriteConflict(
        key_hash: u64,
        writer_tx_id: u64,
        view: MvccReadView,
    ): MvccWriteConflict {
        if (writer_tx_id == MVCC_SNAP_INVALID_TX || _.txStatus(writer_tx_id) != MvccTxStatus.active) {
            return MvccWriteConflict{
                has_conflict: true,
                kind: MvccConflictKind.invalid_tx,
                conflicting_tx_id: writer_tx_id,
                version_slot: MVCC_SNAP_NO_SLOT,
            };
        }

        let i: usize = 0;
        while (i < MVCC_SNAP_MAX_VERSIONS) : (i += 1) {
            const v = _.versions[i];
            if (!v.occupied || v.key_hash != key_hash) continue;

            // Case 1: Another active transaction created or deleted a version of this key.
            if (v.begin_tx != writer_tx_id && _.txStatus(v.begin_tx) == MvccTxStatus.active) {
                return MvccWriteConflict{
                    has_conflict: true,
                    kind: MvccConflictKind.active_writer_conflict,
                    conflicting_tx_id: v.begin_tx,
                    version_slot: i as u16,
                };
            }
            if (v.end_tx != MVCC_SNAP_INF_TX && v.end_tx != writer_tx_id && _.txStatus(v.end_tx) == MvccTxStatus.active) {
                return MvccWriteConflict{
                    has_conflict: true,
                    kind: MvccConflictKind.active_writer_conflict,
                    conflicting_tx_id: v.end_tx,
                    version_slot: i as u16,
                };
            }

            // Case 2: Another transaction committed a write/delete after `view` was captured (invisible to `view`).
            if (v.begin_tx != writer_tx_id && _.txStatus(v.begin_tx) == MvccTxStatus.committed && !_.isTxVisibleInView(v.begin_tx, view)) {
                return MvccWriteConflict{
                    has_conflict: true,
                    kind: MvccConflictKind.committed_after_snapshot,
                    conflicting_tx_id: v.begin_tx,
                    version_slot: i as u16,
                };
            }
            if (v.end_tx != MVCC_SNAP_INF_TX && v.end_tx != writer_tx_id &&
                _.txStatus(v.end_tx) == MvccTxStatus.committed && !_.isTxVisibleInView(v.end_tx, view)) {
                return MvccWriteConflict{
                    has_conflict: true,
                    kind: MvccConflictKind.committed_after_snapshot,
                    conflicting_tx_id: v.end_tx,
                    version_slot: i as u16,
                };
            }
        }

        return MvccWriteConflict{
            has_conflict: false,
            kind: MvccConflictKind.none,
            conflicting_tx_id: MVCC_SNAP_INVALID_TX,
            version_slot: MVCC_SNAP_NO_SLOT,
        };
    }

    /// Inserts a new tuple version for `key_hash` under active transaction `tx_id`, superseding
    /// the latest live (non-aborted, open-ended) version of `key_hash` by setting its `end_tx = tx_id`.
    /// Returns the allocated version slot index, or `null` if `tx_id` is not active or storage is full.
    fn insertVersion(*_, key_hash: u64, value: u64, tx_id: u64): u16? {
        if (tx_id == MVCC_SNAP_INVALID_TX || _.txStatus(tx_id) != MvccTxStatus.active) return null;

        let new_slot: u16 = MVCC_SNAP_NO_SLOT;
        if (s := _.allocVersionSlot()) {
            new_slot = s;
        } else {
            return null;
        }

        const prev_head: u16 = _.findLatestHeadSlot(key_hash);

        // Mark latest open-ended (or aborted-terminated) non-aborted version as superseded by `tx_id`.
        let cur: u16 = prev_head;
        let guard: usize = 0;
        while (cur != MVCC_SNAP_NO_SLOT && guard < MVCC_SNAP_MAX_VERSIONS) : (guard += 1) {
            const c_idx: usize = cur as usize;
            if (!_.versions[c_idx].occupied) break;
            const b_st = _.txStatus(_.versions[c_idx].begin_tx);
            const e_tx: u64 = _.versions[c_idx].end_tx;
            const is_open: bool = (e_tx == MVCC_SNAP_INF_TX) || (_.txStatus(e_tx) == MvccTxStatus.aborted);
            if (b_st != MvccTxStatus.aborted && is_open) {
                _.versions[c_idx].end_tx = tx_id;
                break;
            }
            cur = _.versions[c_idx].prev_version_slot;
        }

        _.versions[new_slot as usize] = MvccTupleVersion{
            key_hash: key_hash,
            value: value,
            begin_tx: tx_id,
            end_tx: MVCC_SNAP_INF_TX,
            prev_version_slot: prev_head,
            deleted: false,
            occupied: true,
        };
        _.occupied_version_count += 1;
        return new_slot;
    }

    /// Logically deletes the latest live version of `key_hash` under active transaction `tx_id`
    /// by setting its `end_tx = tx_id`. Returns `false` if `tx_id` is inactive or no live version exists.
    fn deleteKey(*_, key_hash: u64, tx_id: u64): bool {
        if (tx_id == MVCC_SNAP_INVALID_TX || _.txStatus(tx_id) != MvccTxStatus.active) return false;

        let cur: u16 = _.findLatestHeadSlot(key_hash);
        let guard: usize = 0;
        while (cur != MVCC_SNAP_NO_SLOT && guard < MVCC_SNAP_MAX_VERSIONS) : (guard += 1) {
            const c_idx: usize = cur as usize;
            if (!_.versions[c_idx].occupied) break;
            const b_st = _.txStatus(_.versions[c_idx].begin_tx);
            const e_tx: u64 = _.versions[c_idx].end_tx;
            const is_open: bool = (e_tx == MVCC_SNAP_INF_TX) || (_.txStatus(e_tx) == MvccTxStatus.aborted);
            if (b_st != MvccTxStatus.aborted && is_open) {
                _.versions[c_idx].end_tx = tx_id;
                return true;
            }
            cur = _.versions[c_idx].prev_version_slot;
        }
        return false;
    }

    /// Reads the visible tuple version for `key_hash` under Snapshot Isolation `view`.
    fn readVisibleVersion(key_hash: u64, view: MvccReadView): MvccTupleVersion? {
        let cur: u16 = _.findLatestHeadSlot(key_hash);
        let guard: usize = 0;
        while (cur != MVCC_SNAP_NO_SLOT && guard < MVCC_SNAP_MAX_VERSIONS) : (guard += 1) {
            const c_idx: usize = cur as usize;
            const v = _.versions[c_idx];
            if (!v.occupied) break;
            if (_.isVersionVisible(v, view) && !v.deleted) {
                return v;
            }
            cur = v.prev_version_slot;
        }
        return null;
    }

    /// Returns a copy of the version stored at `slot_idx`, or `null` if out of bounds or unoccupied.
    fn getVersion(slot_idx: u16): MvccTupleVersion? {
        const idx: usize = slot_idx as usize;
        if (idx >= MVCC_SNAP_MAX_VERSIONS || !_.versions[idx].occupied) return null;
        return _.versions[idx];
    }

    /// Computes the global GC safety watermark: the smallest active transaction ID,
    /// or `next_tx_id` if no transactions are currently active.
    fn computeGcWatermark(): u64 {
        if (_.active_count == 0) return _.next_tx_id;
        let min_tx: u64 = _.active_txs[0];
        let i: usize = 1;
        while (i < _.active_count) : (i += 1) {
            if (_.active_txs[i] < min_tx) {
                min_tx = _.active_txs[i];
            }
        }
        return min_tx;
    }

    /// Returns `true` iff `version` can be safely garbage-collected under `gc_watermark`:
    /// either its creator aborted, or its committed `end_tx` is strictly below `gc_watermark`.
    fn canReclaimVersion(version: MvccTupleVersion, gc_watermark: u64): bool {
        if (!version.occupied) return false;
        if (_.txStatus(version.begin_tx) == MvccTxStatus.aborted) return true;
        if (version.end_tx != MVCC_SNAP_INF_TX &&
            _.txStatus(version.end_tx) == MvccTxStatus.committed &&
            version.end_tx < gc_watermark) {
            return true;
        }
        return false;
    }

    /// Reclaims all dead versions below the current `computeGcWatermark()` while preserving
    /// unbroken `prev_version_slot` links for surviving versions. Returns reclaimed slot count.
    fn reclaimDeadVersions(*_): usize {
        const wm: u64 = _.computeGcWatermark();
        let to_reclaim: [MVCC_SNAP_MAX_VERSIONS]bool = [false; MVCC_SNAP_MAX_VERSIONS];
        let reclaim_count: usize = 0;

        let i: usize = 0;
        while (i < MVCC_SNAP_MAX_VERSIONS) : (i += 1) {
            if (_.canReclaimVersion(_.versions[i], wm)) {
                to_reclaim[i] = true;
                reclaim_count += 1;
            }
        }
        if (reclaim_count == 0) return 0;

        // Bypass reclaimed slots in every surviving version's `prev_version_slot` pointer.
        let j: usize = 0;
        while (j < MVCC_SNAP_MAX_VERSIONS) : (j += 1) {
            if (!_.versions[j].occupied || to_reclaim[j]) continue;
            let next_prev: u16 = _.versions[j].prev_version_slot;
            let guard: usize = 0;
            while (next_prev != MVCC_SNAP_NO_SLOT && guard < MVCC_SNAP_MAX_VERSIONS) : (guard += 1) {
                const p_idx: usize = next_prev as usize;
                if (p_idx >= MVCC_SNAP_MAX_VERSIONS || !_.versions[p_idx].occupied) {
                    next_prev = MVCC_SNAP_NO_SLOT;
                    break;
                }
                if (!to_reclaim[p_idx]) break;
                next_prev = _.versions[p_idx].prev_version_slot;
            }
            _.versions[j].prev_version_slot = next_prev;
        }

        // Clear reclaimed slots.
        let k: usize = 0;
        while (k < MVCC_SNAP_MAX_VERSIONS) : (k += 1) {
            if (to_reclaim[k]) {
                _.versions[k] = MvccTupleVersion{};
                if (_.occupied_version_count > 0) {
                    _.occupied_version_count -= 1;
                }
            }
        }
        _.reclaimed_versions += reclaim_count as u64;
        return reclaim_count;
    }

    /// Returns aggregate transaction, version, and GC watermark statistics.
    fn stats(): MvccSnapshotStats {
        return MvccSnapshotStats{
            next_tx_id: _.next_tx_id,
            active_tx_count: _.active_count,
            committed_tx_count: _.committed_tx_count,
            aborted_tx_count: _.aborted_tx_count,
            occupied_versions: _.occupied_version_count,
            reclaimed_versions: _.reclaimed_versions,
            gc_watermark: _.computeGcWatermark(),
        };
    }
}
