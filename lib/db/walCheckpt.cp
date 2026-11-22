// Origin: Pure-CP Stdlib (Zero-Alloc ARIES-Style WAL Fuzzy Checkpoint & Safe Log Truncation Coordinator) | Created: 2026-11-23
// Pure CP zero-heap-allocation ARIES-style WAL checkpoint & safe log truncation coordinator:
//   - Fixed Dirty Page Table (DPT, up to 128 pages tracking `page_id`, first-dirty `rec_lsn`, latest `last_lsn`)
//   - Fixed Active Transaction/Snapshot Table (up to 32 active readers/writers tracking `tx_id`, `pinned_lsn`)
//   - Fixed WAL Segment Table (up to 32 segments tracking `seg_id`, `start_lsn`, `end_lsn`, `size_bytes`, `closed`)
//   - Exact ARIES watermark planner (`computeCheckpointPlan`):
//       * `redo_lsn = min(dirty_page.rec_lsn)` (or `current_flushed_lsn` when DPT is empty)
//       * `safe_truncate_lsn = min(redo_lsn, min(active_tx.pinned_lsn))`
//       * Identifies all closed segments strictly below `safe_truncate_lsn` (`seg.end_lsn < safe_truncate_lsn`)
//   - Safe fuzzy page flush confirmation (`markPageFlushed`: evicts dirty page ONLY if `flushed_lsn >= last_lsn`),
//     checkpoint commit advancement, guarded segment reclamation & zero-alloc telemetry snapshot

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum dirty pages tracked inline in the Dirty Page Table (DPT).
pub const WAL_CKPT_MAX_DIRTY_PAGES: usize = 128;

/// Maximum concurrent active transactions / snapshots pinning WAL LSNs.
pub const WAL_CKPT_MAX_ACTIVE_TXNS: usize = 32;

/// Maximum WAL log segments tracked inline.
pub const WAL_CKPT_MAX_SEGMENTS: usize = 32;

/// Operation status codes for WAL checkpoint coordinator mutations.
pub enum WalCheckptOpStatus {
    ok,
    invalid_arg,
    duplicate_id,
    capacity_full,
    not_found,
    segment_not_reclaimable,
}

/// Dirty Page Table (DPT) entry.
pub struct WalDirtyPageEntry {
    occupied: bool = false,
    page_id: u64 = 0,
    /// LSN of the earliest unflushed modification that made this page dirty.
    rec_lsn: u64 = 0,
    /// LSN of the most recent modification to this page.
    last_lsn: u64 = 0,
}

/// Active transaction / snapshot entry pinning a minimum WAL LSN.
pub struct WalActiveTxnEntry {
    occupied: bool = false,
    tx_id: u64 = 0,
    pinned_lsn: u64 = 0,
}

/// WAL log segment metadata entry.
pub struct WalSegmentEntry {
    occupied: bool = false,
    seg_id: u64 = 0,
    start_lsn: u64 = 0,
    end_lsn: u64 = 0,
    size_bytes: u64 = 0,
    closed: bool = false,
}

/// ARIES fuzzy checkpoint & safe log truncation plan.
pub struct WalCheckpointPlan {
    current_flushed_lsn: u64 = 0,
    redo_lsn: u64 = 0,
    min_active_tx_lsn: u64 = 0,
    safe_truncate_lsn: u64 = 0,
    dirty_page_count: usize = 0,
    active_tx_count: usize = 0,
    reclaimable_seg_count: usize = 0,
    reclaimable_bytes: u64 = 0,
    reclaimable_seg_ids: [WAL_CKPT_MAX_SEGMENTS]u64 = [0; WAL_CKPT_MAX_SEGMENTS],

    /// Returns the slice of reclaimable segment IDs ordered by ascending `end_lsn`.
    pub fn reclaimableIds(): []const u64 {
        return _.reclaimable_seg_ids[0.._.reclaimable_seg_count];
    }
}

/// Global checkpoint & log-reclamation telemetry snapshot.
pub struct WalCheckpointStats {
    dirty_page_count: usize = 0,
    dirty_page_capacity: usize = WAL_CKPT_MAX_DIRTY_PAGES,
    active_tx_count: usize = 0,
    active_tx_capacity: usize = WAL_CKPT_MAX_ACTIVE_TXNS,
    segment_count: usize = 0,
    segment_capacity: usize = WAL_CKPT_MAX_SEGMENTS,
    last_checkpoint_lsn: u64 = 0,
    last_redo_lsn: u64 = 0,
    last_safe_truncate_lsn: u64 = 0,
    reclaimable_seg_count: usize = 0,
    reclaimable_bytes: u64 = 0,
    total_checkpoints_committed: u64 = 0,
    total_segments_reclaimed: u64 = 0,
    total_bytes_reclaimed: u64 = 0,
}

// ============================================================================
// 2. Core Zero-Alloc ARIES WAL Checkpoint Coordinator
// ============================================================================

/// Zero-heap-allocation ARIES-style WAL fuzzy checkpoint & safe log truncation coordinator.
pub struct WalCheckpointCoordinator {
    dirty_pages: [WAL_CKPT_MAX_DIRTY_PAGES]WalDirtyPageEntry = [WalDirtyPageEntry{}; WAL_CKPT_MAX_DIRTY_PAGES],
    dirty_count: usize = 0,
    active_txns: [WAL_CKPT_MAX_ACTIVE_TXNS]WalActiveTxnEntry = [WalActiveTxnEntry{}; WAL_CKPT_MAX_ACTIVE_TXNS],
    tx_count: usize = 0,
    segments: [WAL_CKPT_MAX_SEGMENTS]WalSegmentEntry = [WalSegmentEntry{}; WAL_CKPT_MAX_SEGMENTS],
    seg_count: usize = 0,
    last_checkpoint_lsn: u64 = 0,
    last_redo_lsn: u64 = 0,
    last_safe_truncate_lsn: u64 = 0,
    total_checkpoints_committed: u64 = 0,
    total_segments_reclaimed: u64 = 0,
    total_bytes_reclaimed: u64 = 0,

    /// Creates a new empty `WalCheckpointCoordinator`.
    pub fn init(): WalCheckpointCoordinator {
        return WalCheckpointCoordinator{};
    }

    // ---- Internal Lookup Helpers ----

    fn findDirtySlot(page_id: u64): usize? {
        let i: usize = 0;
        while (i < WAL_CKPT_MAX_DIRTY_PAGES) : (i += 1) {
            if (_.dirty_pages[i].occupied && _.dirty_pages[i].page_id == page_id) return i;
        }
        return null;
    }

    fn findFreeDirtySlot(): usize? {
        let i: usize = 0;
        while (i < WAL_CKPT_MAX_DIRTY_PAGES) : (i += 1) {
            if (!_.dirty_pages[i].occupied) return i;
        }
        return null;
    }

    fn findTxnSlot(tx_id: u64): usize? {
        let i: usize = 0;
        while (i < WAL_CKPT_MAX_ACTIVE_TXNS) : (i += 1) {
            if (_.active_txns[i].occupied && _.active_txns[i].tx_id == tx_id) return i;
        }
        return null;
    }

    fn findFreeTxnSlot(): usize? {
        let i: usize = 0;
        while (i < WAL_CKPT_MAX_ACTIVE_TXNS) : (i += 1) {
            if (!_.active_txns[i].occupied) return i;
        }
        return null;
    }

    fn findSegSlot(seg_id: u64): usize? {
        let i: usize = 0;
        while (i < WAL_CKPT_MAX_SEGMENTS) : (i += 1) {
            if (_.segments[i].occupied && _.segments[i].seg_id == seg_id) return i;
        }
        return null;
    }

    fn findFreeSegSlot(): usize? {
        let i: usize = 0;
        while (i < WAL_CKPT_MAX_SEGMENTS) : (i += 1) {
            if (!_.segments[i].occupied) return i;
        }
        return null;
    }

    // ---- Dirty Page Table (DPT) API ----

    /// Records a page modification at `lsn`.
    /// Preserves the earliest `rec_lsn` if the page is already dirty and advances `last_lsn`.
    pub fn markPageDirty(*_, page_id: u64, lsn: u64): WalCheckptOpStatus {
        if (page_id == 0 || lsn == 0) return WalCheckptOpStatus.invalid_arg;

        if (idx := _.findDirtySlot(page_id)) {
            if (lsn < _.dirty_pages[idx].rec_lsn) {
                _.dirty_pages[idx].rec_lsn = lsn;
            }
            if (lsn > _.dirty_pages[idx].last_lsn) {
                _.dirty_pages[idx].last_lsn = lsn;
            }
            return WalCheckptOpStatus.ok;
        }

        const slot = _.findFreeDirtySlot() ?? return WalCheckptOpStatus.capacity_full;
        _.dirty_pages[slot] = WalDirtyPageEntry{
            occupied: true,
            page_id: page_id,
            rec_lsn: lsn,
            last_lsn: lsn,
        };
        _.dirty_count += 1;
        return WalCheckptOpStatus.ok;
    }

    /// Confirms that `page_id` was flushed to durable storage up to `flushed_lsn`.
    /// Clears the page from the DPT ONLY if `flushed_lsn >= entry.last_lsn` (fuzzy checkpoint safety);
    /// otherwise retains the entry because newer in-memory writes have not yet been flushed.
    /// Returns `true` if the dirty page entry was cleared.
    pub fn markPageFlushed(*_, page_id: u64, flushed_lsn: u64): bool {
        if (page_id == 0) return false;
        const idx = _.findDirtySlot(page_id) ?? return false;
        if (flushed_lsn < _.dirty_pages[idx].last_lsn) {
            return false;
        }
        _.dirty_pages[idx] = WalDirtyPageEntry{};
        if (_.dirty_count > 0) _.dirty_count -= 1;
        return true;
    }

    /// Looks up a dirty page entry by `page_id`.
    pub fn getDirtyPage(page_id: u64): WalDirtyPageEntry? {
        if (page_id == 0) return null;
        const idx = _.findDirtySlot(page_id) ?? return null;
        return _.dirty_pages[idx];
    }

    // ---- Active Transaction / Snapshot Pinning API ----

    /// Registers an active transaction or snapshot pinning WAL logs at `pinned_lsn`.
    pub fn beginTxn(*_, tx_id: u64, pinned_lsn: u64): WalCheckptOpStatus {
        if (tx_id == 0 || pinned_lsn == 0) return WalCheckptOpStatus.invalid_arg;
        if (_.findTxnSlot(tx_id) != null) return WalCheckptOpStatus.duplicate_id;
        const slot = _.findFreeTxnSlot() ?? return WalCheckptOpStatus.capacity_full;

        _.active_txns[slot] = WalActiveTxnEntry{
            occupied: true,
            tx_id: tx_id,
            pinned_lsn: pinned_lsn,
        };
        _.tx_count += 1;
        return WalCheckptOpStatus.ok;
    }

    /// Ends (commits or aborts) an active transaction, releasing its LSN pin.
    pub fn endTxn(*_, tx_id: u64): WalCheckptOpStatus {
        if (tx_id == 0) return WalCheckptOpStatus.invalid_arg;
        const idx = _.findTxnSlot(tx_id) ?? return WalCheckptOpStatus.not_found;
        _.active_txns[idx] = WalActiveTxnEntry{};
        if (_.tx_count > 0) _.tx_count -= 1;
        return WalCheckptOpStatus.ok;
    }

    // ---- WAL Segment Registry & Safe Reclamation API ----

    /// Registers a WAL segment with `[start_lsn, end_lsn]` coverage and byte size.
    pub fn registerSegment(
        *_,
        seg_id: u64,
        start_lsn: u64,
        end_lsn: u64,
        size_bytes: u64,
        closed: bool,
    ): WalCheckptOpStatus {
        if (seg_id == 0 || start_lsn == 0 || end_lsn < start_lsn) return WalCheckptOpStatus.invalid_arg;
        if (_.findSegSlot(seg_id) != null) return WalCheckptOpStatus.duplicate_id;
        const slot = _.findFreeSegSlot() ?? return WalCheckptOpStatus.capacity_full;

        _.segments[slot] = WalSegmentEntry{
            occupied: true,
            seg_id: seg_id,
            start_lsn: start_lsn,
            end_lsn: end_lsn,
            size_bytes: size_bytes,
            closed: closed,
        };
        _.seg_count += 1;
        return WalCheckptOpStatus.ok;
    }

    /// Seals an open WAL segment once log rotation moves to a newer segment.
    pub fn closeSegment(
        *_,
        seg_id: u64,
        final_end_lsn: u64,
        final_size_bytes: u64,
    ): WalCheckptOpStatus {
        if (seg_id == 0) return WalCheckptOpStatus.invalid_arg;
        const idx = _.findSegSlot(seg_id) ?? return WalCheckptOpStatus.not_found;
        if (final_end_lsn < _.segments[idx].start_lsn) return WalCheckptOpStatus.invalid_arg;
        _.segments[idx].end_lsn = final_end_lsn;
        _.segments[idx].size_bytes = final_size_bytes;
        _.segments[idx].closed = true;
        return WalCheckptOpStatus.ok;
    }

    /// Reclaims a single WAL segment ONLY if it is closed and strictly below `safe_truncate_lsn`.
    pub fn reclaimSegment(*_, seg_id: u64, safe_truncate_lsn: u64): WalCheckptOpStatus {
        if (seg_id == 0) return WalCheckptOpStatus.invalid_arg;
        const idx = _.findSegSlot(seg_id) ?? return WalCheckptOpStatus.not_found;
        const seg = _.segments[idx];
        if (!seg.closed || seg.end_lsn >= safe_truncate_lsn) {
            return WalCheckptOpStatus.segment_not_reclaimable;
        }

        _.total_segments_reclaimed += 1;
        _.total_bytes_reclaimed += seg.size_bytes;
        _.segments[idx] = WalSegmentEntry{};
        if (_.seg_count > 0) _.seg_count -= 1;
        return WalCheckptOpStatus.ok;
    }

    /// Batch-reclaims all closed segments strictly below `safe_truncate_lsn`.
    /// Returns the number of segments reclaimed.
    pub fn reclaimSafeSegments(*_, safe_truncate_lsn: u64): usize {
        let reclaimed: usize = 0;
        let i: usize = 0;
        while (i < WAL_CKPT_MAX_SEGMENTS) : (i += 1) {
            if (!_.segments[i].occupied) continue;
            if (!_.segments[i].closed) continue;
            if (_.segments[i].end_lsn < safe_truncate_lsn) {
                _.total_segments_reclaimed += 1;
                _.total_bytes_reclaimed += _.segments[i].size_bytes;
                _.segments[i] = WalSegmentEntry{};
                if (_.seg_count > 0) _.seg_count -= 1;
                reclaimed += 1;
            }
        }
        return reclaimed;
    }

    // ---- ARIES Watermark Planning & Checkpoint Commit ----

    /// Computes the ARIES checkpoint plan and safe WAL truncation watermark:
    ///   - `redo_lsn = min(dirty_pages.rec_lsn)` (or `current_flushed_lsn` when no dirty pages exist)
    ///   - `safe_truncate_lsn = min(redo_lsn, min(active_txns.pinned_lsn))`
    ///   - Lists all closed segments with `end_lsn < safe_truncate_lsn`, ordered by ascending `end_lsn`.
    pub fn computeCheckpointPlan(current_flushed_lsn: u64): WalCheckpointPlan {
        // 1. Compute redo_lsn from Dirty Page Table
        let redo_lsn: u64 = current_flushed_lsn;
        let has_dirty: bool = false;
        let i: usize = 0;
        while (i < WAL_CKPT_MAX_DIRTY_PAGES) : (i += 1) {
            if (!_.dirty_pages[i].occupied) continue;
            if (!has_dirty || _.dirty_pages[i].rec_lsn < redo_lsn) {
                redo_lsn = _.dirty_pages[i].rec_lsn;
                has_dirty = true;
            }
        }

        // 2. Compute min_active_tx_lsn from active transactions
        let min_tx_lsn: u64 = 0;
        let has_tx: bool = false;
        let j: usize = 0;
        while (j < WAL_CKPT_MAX_ACTIVE_TXNS) : (j += 1) {
            if (!_.active_txns[j].occupied) continue;
            if (!has_tx || _.active_txns[j].pinned_lsn < min_tx_lsn) {
                min_tx_lsn = _.active_txns[j].pinned_lsn;
                has_tx = true;
            }
        }

        // 3. Safe truncation watermark
        const safe_truncate: u64 = if (has_tx && min_tx_lsn < redo_lsn) min_tx_lsn else redo_lsn;

        // 4. Gather reclaimable closed segments strictly below safe_truncate_lsn
        let plan = WalCheckpointPlan{
            current_flushed_lsn: current_flushed_lsn,
            redo_lsn: redo_lsn,
            min_active_tx_lsn: min_tx_lsn,
            safe_truncate_lsn: safe_truncate,
            dirty_page_count: _.dirty_count,
            active_tx_count: _.tx_count,
        };

        let seg_ends: [WAL_CKPT_MAX_SEGMENTS]u64 = [0; WAL_CKPT_MAX_SEGMENTS];
        let k: usize = 0;
        while (k < WAL_CKPT_MAX_SEGMENTS) : (k += 1) {
            const seg = _.segments[k];
            if (!seg.occupied || !seg.closed) continue;
            if (seg.end_lsn < safe_truncate) {
                const pos = plan.reclaimable_seg_count;
                plan.reclaimable_seg_ids[pos] = seg.seg_id;
                seg_ends[pos] = seg.end_lsn;
                plan.reclaimable_seg_count += 1;
                plan.reclaimable_bytes += seg.size_bytes;
            }
        }

        // Insertion sort reclaimable segments by ascending (end_lsn, seg_id)
        let s: usize = 1;
        while (s < plan.reclaimable_seg_count) : (s += 1) {
            const key_id = plan.reclaimable_seg_ids[s];
            const key_end = seg_ends[s];
            let p: usize = s;
            while (p > 0 && (key_end < seg_ends[p - 1] || (key_end == seg_ends[p - 1] && key_id < plan.reclaimable_seg_ids[p - 1]))) {
                plan.reclaimable_seg_ids[p] = plan.reclaimable_seg_ids[p - 1];
                seg_ends[p] = seg_ends[p - 1];
                p -= 1;
            }
            plan.reclaimable_seg_ids[p] = key_id;
            seg_ends[p] = key_end;
        }

        return plan;
    }

    /// Computes and commits a fuzzy checkpoint at `checkpoint_lsn`, advancing persisted checkpoint watermarks.
    pub fn commitCheckpoint(*_, checkpoint_lsn: u64, current_flushed_lsn: u64): WalCheckpointPlan {
        const plan = _.computeCheckpointPlan(current_flushed_lsn);
        if (checkpoint_lsn > _.last_checkpoint_lsn) {
            _.last_checkpoint_lsn = checkpoint_lsn;
        }
        _.last_redo_lsn = plan.redo_lsn;
        _.last_safe_truncate_lsn = plan.safe_truncate_lsn;
        _.total_checkpoints_committed += 1;
        return plan;
    }

    /// Returns a complete zero-alloc telemetry snapshot evaluated at `current_flushed_lsn`.
    pub fn stats(current_flushed_lsn: u64): WalCheckpointStats {
        const plan = _.computeCheckpointPlan(current_flushed_lsn);
        return WalCheckpointStats{
            dirty_page_count: _.dirty_count,
            dirty_page_capacity: WAL_CKPT_MAX_DIRTY_PAGES,
            active_tx_count: _.tx_count,
            active_tx_capacity: WAL_CKPT_MAX_ACTIVE_TXNS,
            segment_count: _.seg_count,
            segment_capacity: WAL_CKPT_MAX_SEGMENTS,
            last_checkpoint_lsn: _.last_checkpoint_lsn,
            last_redo_lsn: _.last_redo_lsn,
            last_safe_truncate_lsn: _.last_safe_truncate_lsn,
            reclaimable_seg_count: plan.reclaimable_seg_count,
            reclaimable_bytes: plan.reclaimable_bytes,
            total_checkpoints_committed: _.total_checkpoints_committed,
            total_segments_reclaimed: _.total_segments_reclaimed,
            total_bytes_reclaimed: _.total_bytes_reclaimed,
        };
    }
}
