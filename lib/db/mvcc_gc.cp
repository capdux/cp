// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc MVCC Snapshot Isolation & Low-Watermark GC Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation Multi-Version Concurrency Control (MVCC) version store,
// snapshot-isolation visibility reader, and Low-Watermark Safe-Horizon Garbage Collector
// modeled on PostgreSQL / TiKV / RocksDB storage engines:
//   - Inline pools (`MVCC_GC_MAX_VERSIONS = 256`, `MVCC_GC_MAX_READERS = 32`, zero dynamic heap)
//   - Strictly monotonic per-key `commit_ts` enforcement protecting historical snapshot immutability
//   - Safe-horizon calculation pinned by the minimum active reader `read_ts` (or `latest_commit_ts` if idle)
//   - Complete GC reclamation of obsolete overwritten versions & orphaned tombstones below the safe horizon

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum resident version records in `MvccVersionGcStore` (zero dynamic heap).
pub const MVCC_GC_MAX_VERSIONS: usize = 256;

/// Maximum concurrent active read-snapshot transactions tracked for GC horizon pinning.
pub const MVCC_GC_MAX_READERS: usize = 32;

/// Single committed version (or deletion tombstone) stored in the inline version pool.
pub struct MvccVersionRecord {
    key_hash: u64 = 0;
    commit_ts: u64 = 0;
    val_tag: u64 = 0;
    is_tombstone: bool = false;
    occupied: bool = false;
}

/// Active read transaction snapshot descriptor pinning the MVCC GC safe horizon.
pub struct MvccReadSnapshot {
    tx_id: u64 = 0;
    read_ts: u64 = 0;
    active: bool = false;
}

/// Result returned by `MvccVersionGcStore.read_visible`.
pub struct MvccReadResult {
    found: bool = false;
    is_deleted: bool = false;
    val_tag: u64 = 0;
    commit_ts: u64 = 0;
}

/// Summary report returned by `MvccVersionGcStore.collect_garbage`.
pub struct MvccGcReport {
    safe_horizon_ts: u64 = 0;
    reclaimed_versions: usize = 0;
    reclaimed_tombstones: usize = 0;
    remaining_versions: usize = 0;
}

/// Operational statistics snapshot for `MvccVersionGcStore`.
pub struct MvccGcStats {
    active_readers: usize = 0;
    live_versions: usize = 0;
    latest_commit_ts: u64 = 0;
    total_reclaimed: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc MVCC Version Store & Safe-Horizon Garbage Collector
// ============================================================================

/// Zero-heap-allocation MVCC multi-version store with snapshot isolation and safe-horizon GC.
pub struct MvccVersionGcStore {
    versions: [MVCC_GC_MAX_VERSIONS]MvccVersionRecord = [MvccVersionRecord{}; MVCC_GC_MAX_VERSIONS];
    readers: [MVCC_GC_MAX_READERS]MvccReadSnapshot = [MvccReadSnapshot{}; MVCC_GC_MAX_READERS];
    active_readers: usize = 0;
    live_versions: usize = 0;
    latest_commit_ts: u64 = 0;
    total_reclaimed: u64 = 0;

    /// Initializes an empty `MvccVersionGcStore`.
    static fn init(): MvccVersionGcStore {
        return MvccVersionGcStore{
            versions: [MvccVersionRecord{}; MVCC_GC_MAX_VERSIONS],
            readers: [MvccReadSnapshot{}; MVCC_GC_MAX_READERS],
            active_readers: 0,
            live_versions: 0,
            latest_commit_ts: 0,
            total_reclaimed: 0,
        };
    }

    /// Registers an active read transaction pinning GC at `read_ts`.
    /// Rejects duplicate active `tx_id` or full 32-reader table.
    fn begin_read_tx(*_, tx_id: u64, read_ts: u64): bool {
        let free_slot: usize = MVCC_GC_MAX_READERS;
        let i: usize = 0;
        while (i < MVCC_GC_MAX_READERS) : (i += 1) {
            if (_.readers[i].active) {
                if (_.readers[i].tx_id == tx_id) return false;
            } else if (free_slot == MVCC_GC_MAX_READERS) {
                free_slot = i;
            }
        }
        if (free_slot >= MVCC_GC_MAX_READERS) return false;

        _.readers[free_slot] = MvccReadSnapshot{
            tx_id: tx_id,
            read_ts: read_ts,
            active: true,
        };
        _.active_readers += 1;
        return true;
    }

    /// Releases an active read transaction, allowing the GC safe horizon to advance.
    fn end_read_tx(*_, tx_id: u64): bool {
        let i: usize = 0;
        while (i < MVCC_GC_MAX_READERS) : (i += 1) {
            if (_.readers[i].active && _.readers[i].tx_id == tx_id) {
                _.readers[i] = MvccReadSnapshot{};
                _.active_readers -= 1;
                return true;
            }
        }
        return false;
    }

    /// Computes the current low-watermark GC safe horizon:
    /// - Minimum `read_ts` among all active read transactions, if any exist;
    /// - Otherwise `latest_commit_ts`.
    fn safe_horizon_ts(): u64 {
        let has_reader: bool = false;
        let min_ts: u64 = 0;
        let i: usize = 0;
        while (i < MVCC_GC_MAX_READERS) : (i += 1) {
            if (!_.readers[i].active) continue;
            if (!has_reader || _.readers[i].read_ts < min_ts) {
                has_reader = true;
                min_ts = _.readers[i].read_ts;
            }
        }
        if (has_reader) return min_ts;
        return _.latest_commit_ts;
    }

    /// Appends a new committed version (or tombstone) for `key_hash` at `commit_ts`.
    /// Enforces strictly increasing `commit_ts` per key and 256-version pool capacity.
    fn write_version(
        *_,
        key_hash: u64,
        commit_ts: u64,
        val_tag: u64,
        is_tombstone: bool,
    ): bool {
        let free_slot: usize = MVCC_GC_MAX_VERSIONS;
        let has_key: bool = false;
        let max_key_ts: u64 = 0;

        let i: usize = 0;
        while (i < MVCC_GC_MAX_VERSIONS) : (i += 1) {
            if (_.versions[i].occupied) {
                if (_.versions[i].key_hash == key_hash) {
                    if (!has_key || _.versions[i].commit_ts > max_key_ts) {
                        has_key = true;
                        max_key_ts = _.versions[i].commit_ts;
                    }
                }
            } else if (free_slot == MVCC_GC_MAX_VERSIONS) {
                free_slot = i;
            }
        }

        if (has_key && commit_ts <= max_key_ts) return false;
        if (free_slot >= MVCC_GC_MAX_VERSIONS) return false;

        _.versions[free_slot] = MvccVersionRecord{
            key_hash: key_hash,
            commit_ts: commit_ts,
            val_tag: if (is_tombstone) 0 else val_tag,
            is_tombstone: is_tombstone,
            occupied: true,
        };
        _.live_versions += 1;
        if (commit_ts > _.latest_commit_ts) {
            _.latest_commit_ts = commit_ts;
        }
        return true;
    }

    /// Performs a snapshot-isolated point read for `key_hash` at `snapshot_ts`:
    /// selects the newest version with `commit_ts <= snapshot_ts`.
    /// Returns `found = false, is_deleted = true` if the visible version is a tombstone.
    fn read_visible(key_hash: u64, snapshot_ts: u64): MvccReadResult {
        let found_any: bool = false;
        let best_idx: usize = 0;
        let best_ts: u64 = 0;

        let i: usize = 0;
        while (i < MVCC_GC_MAX_VERSIONS) : (i += 1) {
            const rec = _.versions[i];
            if (!rec.occupied || rec.key_hash != key_hash) continue;
            if (rec.commit_ts <= snapshot_ts) {
                if (!found_any || rec.commit_ts > best_ts) {
                    found_any = true;
                    best_ts = rec.commit_ts;
                    best_idx = i;
                }
            }
        }

        if (!found_any) {
            return MvccReadResult{
                found: false,
                is_deleted: false,
                val_tag: 0,
                commit_ts: 0,
            };
        }

        const best = _.versions[best_idx];
        if (best.is_tombstone) {
            return MvccReadResult{
                found: false,
                is_deleted: true,
                val_tag: 0,
                commit_ts: best.commit_ts,
            };
        }

        return MvccReadResult{
            found: true,
            is_deleted: false,
            val_tag: best.val_tag,
            commit_ts: best.commit_ts,
        };
    }

    /// Runs safe-horizon MVCC garbage collection:
    /// - Preserves all versions with `commit_ts > horizon`.
    /// - For each key, preserves the single newest version with `commit_ts <= horizon` as the
    ///   baseline for snapshots `>= horizon` and reclaims all older superseded versions.
    /// - If that baseline itself is a tombstone AND no version with `commit_ts > horizon` exists
    ///   for the key, reclaims the orphaned tombstone as well.
    fn collect_garbage(*_): MvccGcReport {
        const horizon: u64 = _.safe_horizon_ts();
        let processed: [MVCC_GC_MAX_VERSIONS]bool = [false; MVCC_GC_MAX_VERSIONS];
        let reclaimed_versions: usize = 0;
        let reclaimed_tombstones: usize = 0;

        let i: usize = 0;
        while (i < MVCC_GC_MAX_VERSIONS) : (i += 1) {
            if (!_.versions[i].occupied || processed[i]) continue;
            const k: u64 = _.versions[i].key_hash;

            // Pass 1: find baseline (newest version <= horizon) & check if any version > horizon exists
            let has_newer: bool = false;
            let has_base: bool = false;
            let base_idx: usize = 0;
            let base_ts: u64 = 0;

            let j: usize = i;
            while (j < MVCC_GC_MAX_VERSIONS) : (j += 1) {
                if (!_.versions[j].occupied || _.versions[j].key_hash != k) continue;
                processed[j] = true;
                const ts: u64 = _.versions[j].commit_ts;
                if (ts > horizon) {
                    has_newer = true;
                } else {
                    if (!has_base || ts > base_ts) {
                        has_base = true;
                        base_ts = ts;
                        base_idx = j;
                    }
                }
            }

            if (!has_base) continue;

            // Pass 2: reclaim obsolete versions strictly older than base_idx,
            // and reclaim base_idx itself if it is an orphaned tombstone (no newer versions > horizon)
            let m: usize = i;
            while (m < MVCC_GC_MAX_VERSIONS) : (m += 1) {
                if (!_.versions[m].occupied || _.versions[m].key_hash != k) continue;
                if (_.versions[m].commit_ts > horizon) continue;

                const is_obsolete: bool = m != base_idx;
                const is_orphan_tombstone: bool = (m == base_idx) && _.versions[m].is_tombstone && !has_newer;

                if (is_obsolete || is_orphan_tombstone) {
                    if (_.versions[m].is_tombstone) {
                        reclaimed_tombstones += 1;
                    }
                    reclaimed_versions += 1;
                    _.versions[m] = MvccVersionRecord{};
                    _.live_versions -= 1;
                }
            }
        }

        _.total_reclaimed += reclaimed_versions as u64;
        return MvccGcReport{
            safe_horizon_ts: horizon,
            reclaimed_versions: reclaimed_versions,
            reclaimed_tombstones: reclaimed_tombstones,
            remaining_versions: _.live_versions,
        };
    }

    /// Returns a snapshot of active readers, live versions, latest commit timestamp, and total reclaimed versions.
    fn stats(): MvccGcStats {
        return MvccGcStats{
            active_readers: _.active_readers,
            live_versions: _.live_versions,
            latest_commit_ts: _.latest_commit_ts,
            total_reclaimed: _.total_reclaimed,
        };
    }
}
