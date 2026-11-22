// Origin: Pure-CP Stdlib (Zero-Alloc LSM-Tree Leveled Compaction Planner & SSTable Overlap Selector) | Created: 2026-11-22
// Pure CP zero-heap-allocation RocksDB/Pebble-grade LSM-Tree leveled compaction planner:
//   - Fixed 128-SSTable inline metadata table (`MAX_LSM_SST_FILES = 128`), zero heap allocations
//   - Byte-order key-range overlap detection (`overlapsRange`) with closed-interval semantics
//   - Level pressure scoring (L0 file count trigger, L1+ exponential byte target)
//   - L0 overlap-cluster expansion + L1 fanout, Ln->Ln+1 trivial-move fast path (zero write-amp),
//     high-tombstone reclamation (`tombstone_reclaim`), and `being_compacted` concurrency conflict avoidance

const std = @import("std");

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

pub const MAX_LSM_LEVELS: usize = 7;
pub const MAX_LSM_SST_FILES: usize = 128;
pub const MAX_SST_KEY_LEN: usize = 64;
pub const MAX_COMPACTION_INPUTS: usize = 32;

fn copyBytes(dst: *[]u8, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len && i < dst.len) : (i += 1) {
        dst[i] = src[i];
    }
    return i;
}

fn saturatingMulU64(a: u64, b: u64): u64 {
    if (a == 0 || b == 0) {
        return 0;
    }
    const max_u64: u64 = std.math.maxInt(u64);
    if (a > max_u64 / b) {
        return max_u64;
    }
    return a * b;
}

fn saturatingAddU64(a: u64, b: u64): u64 {
    const max_u64: u64 = std.math.maxInt(u64);
    if (b > max_u64 - a) {
        return max_u64;
    }
    return a + b;
}

/// Lexicographical unsigned byte comparison: returns `< 0` if `a < b`, `0` if equal, `> 0` if `a > b`.
pub fn compareKeyBytesLex(a: []const u8, b: []const u8): i32 {
    const min_len: usize = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

fn rangesOverlap(a_min: []const u8, a_max: []const u8, b_min: []const u8, b_max: []const u8): bool {
    if (a_min.len == 0 || a_max.len == 0 || b_min.len == 0 || b_max.len == 0) {
        return false;
    }
    return compareKeyBytesLex(a_min, b_max) <= 0 && compareKeyBytesLex(b_min, a_max) <= 0;
}

// ============================================================================
// 1. Reason, Config, SSTable Metadata, Plan & Level Stats Models
// ============================================================================

/// Trigger reason for a planned LSM compaction job.
pub enum LsmCompactionReason {
    none,
    l0_file_count,
    level_size_overflow,
    tombstone_reclaim,
    trivial_move,
}

/// Leveled compaction sizing and threshold configuration.
pub struct LsmCompactionConfig {
    max_levels: usize = 7;
    l0_compaction_trigger: usize = 4;
    base_level_bytes: u64 = 67108864;
    level_multiplier: u64 = 10;
    tombstone_trigger_bps: u16 = 3000;

    /// Validates configuration invariants (`2 <= max_levels <= 7`, positive triggers, `bps <= 10000`).
    fn isValid(): bool {
        if (_.max_levels < 2 || _.max_levels > MAX_LSM_LEVELS) {
            return false;
        }
        if (_.l0_compaction_trigger == 0 || _.base_level_bytes == 0 || _.level_multiplier < 2) {
            return false;
        }
        if (_.tombstone_trigger_bps == 0 || _.tombstone_trigger_bps > 10000) {
            return false;
        }
        return true;
    }

    /// Returns the target byte budget for `level` (`L1 = base_level_bytes`, `Ln = base * mult^(n-1)`).
    /// Returns `0` for `level == 0` or `level >= max_levels`.
    fn levelTargetBytes(level: u8): u64 {
        const lvl: usize = level as usize;
        if (lvl == 0 || lvl >= _.max_levels) {
            return 0;
        }
        let target: u64 = _.base_level_bytes;
        let step: usize = 1;
        while (step < lvl) : (step += 1) {
            target = saturatingMulU64(target, _.level_multiplier);
        }
        return target;
    }
}

/// Fixed-size inline SSTable metadata with closed key range `[min_key, max_key]`.
pub struct SstFileMeta {
    file_id: u64 = 0;
    level: u8 = 0;
    size_bytes: u64 = 0;
    min_key_buf: [64]u8 = [0; 64];
    min_key_len: usize = 0;
    max_key_buf: [64]u8 = [0; 64];
    max_key_len: usize = 0;
    tombstone_bps: u16 = 0;
    being_compacted: bool = false;
    occupied: bool = false;

    /// Constructs and validates an `SstFileMeta` (`min_key <= max_key`, non-zero `file_id`, `bps <= 10000`).
    static fn init(
        file_id: u64,
        level: u8,
        size_bytes: u64,
        min_key: []const u8,
        max_key: []const u8,
        tombstone_bps: u16,
    ): SstFileMeta? {
        if (file_id == 0 || min_key.len == 0 || max_key.len == 0) {
            return null;
        }
        if (min_key.len > MAX_SST_KEY_LEN || max_key.len > MAX_SST_KEY_LEN) {
            return null;
        }
        if (compareKeyBytesLex(min_key, max_key) > 0 || tombstone_bps > 10000) {
            return null;
        }
        let meta = SstFileMeta{
            file_id: file_id,
            level: level,
            size_bytes: size_bytes,
            tombstone_bps: tombstone_bps,
            being_compacted: false,
            occupied: true,
        };
        let lo_dst: []u8 = meta.min_key_buf[0..];
        let hi_dst: []u8 = meta.max_key_buf[0..];
        meta.min_key_len = copyBytes(lo_dst, min_key);
        meta.max_key_len = copyBytes(hi_dst, max_key);
        return meta;
    }

    /// Returns `min_key` as a borrowed byte slice.
    fn minKeySlice(): []const u8 {
        return _.min_key_buf[0.._.min_key_len];
    }

    /// Returns `max_key` as a borrowed byte slice.
    fn maxKeySlice(): []const u8 {
        return _.max_key_buf[0.._.max_key_len];
    }

    /// Closed-interval byte-order overlap test against `[lo, hi]`.
    fn overlapsRange(lo: []const u8, hi: []const u8): bool {
        return rangesOverlap(_.minKeySlice(), _.maxKeySlice(), lo, hi);
    }
}

/// Output plan produced by `LsmCompactionPlanner.plan_next_compaction`.
pub struct CompactionPlan {
    valid: bool = false;
    reason: LsmCompactionReason = LsmCompactionReason.none;
    source_level: u8 = 0;
    target_level: u8 = 0;
    is_trivial_move: bool = false;
    source_file_ids: [32]u64 = [0; 32];
    source_count: usize = 0;
    target_file_ids: [32]u64 = [0; 32];
    target_count: usize = 0;
    estimated_read_bytes: u64 = 0;
    priority_score_f32: f32 = 0.0;
}

/// Per-level file count, byte size, and normalized compaction pressure score.
pub struct LsmLevelStats {
    level: u8 = 0;
    file_count: usize = 0;
    total_bytes: u64 = 0;
    compacting_files: usize = 0;
    score_f32: f32 = 0.0;
}

// ============================================================================
// 2. Zero-Alloc LSM-Tree Compaction Planner
// ============================================================================

/// Zero-heap-allocation leveled LSM-Tree compaction planner backed by 128 inline SSTable slots.
pub struct LsmCompactionPlanner {
    cfg: LsmCompactionConfig = LsmCompactionConfig{};
    files: [128]SstFileMeta = [SstFileMeta{}; 128];

    /// Initializes a zero-alloc planner. Returns `null` if `cfg` is invalid.
    static fn init(cfg: LsmCompactionConfig): LsmCompactionPlanner? {
        if (!cfg.isValid()) {
            return null;
        }
        return LsmCompactionPlanner{
            cfg: cfg,
            files: [SstFileMeta{}; 128],
        };
    }

    /// Finds the slot index for `file_id`, or `null` if absent.
    fn find_file(file_id: u64): usize? {
        if (file_id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < MAX_LSM_SST_FILES) : (i += 1) {
            if (_.files[i].occupied && _.files[i].file_id == file_id) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new SSTable metadata entry. Rejects invalid level/keys, duplicate `file_id`, or full table.
    fn add_sst(*_, meta: SstFileMeta): bool {
        if (meta.file_id == 0 || (meta.level as usize) >= _.cfg.max_levels) {
            return false;
        }
        if (meta.min_key_len == 0 || meta.max_key_len == 0 ||
            meta.min_key_len > MAX_SST_KEY_LEN || meta.max_key_len > MAX_SST_KEY_LEN)
        {
            return false;
        }
        if (compareKeyBytesLex(meta.minKeySlice(), meta.maxKeySlice()) > 0 || meta.tombstone_bps > 10000) {
            return false;
        }
        if (_.find_file(meta.file_id) != null) {
            return false;
        }

        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < MAX_LSM_SST_FILES) : (i += 1) {
            if (!_.files[i].occupied) {
                free_idx = i;
                break;
            }
        }
        const idx = free_idx ?? return false;

        _.files[idx] = meta;
        _.files[idx].occupied = true;
        return true;
    }

    /// Removes an SSTable by `file_id`. Returns `false` if not found.
    fn remove_sst(*_, file_id: u64): bool {
        const idx = _.find_file(file_id) ?? return false;
        _.files[idx] = SstFileMeta{};
        return true;
    }

    /// Marks or clears `being_compacted` on `file_id` to prevent overlapping concurrent compactions.
    fn mark_compacting(*_, file_id: u64, flag: bool): bool {
        const idx = _.find_file(file_id) ?? return false;
        _.files[idx].being_compacted = flag;
        return true;
    }

    /// Computes file count, total bytes, compacting count, and pressure score for `level`.
    fn level_stats(level: u8): LsmLevelStats {
        let st = LsmLevelStats{ level: level };
        if ((level as usize) >= _.cfg.max_levels) {
            return st;
        }
        let i: usize = 0;
        while (i < MAX_LSM_SST_FILES) : (i += 1) {
            if (_.files[i].occupied && _.files[i].level == level) {
                st.file_count += 1;
                st.total_bytes = saturatingAddU64(st.total_bytes, _.files[i].size_bytes);
                if (_.files[i].being_compacted) {
                    st.compacting_files += 1;
                }
            }
        }
        if (level == 0) {
            st.score_f32 = (st.file_count as f32) / (_.cfg.l0_compaction_trigger as f32);
        } else if ((level as usize) < _.cfg.max_levels - 1) {
            const target_bytes: u64 = _.cfg.levelTargetBytes(level);
            if (target_bytes > 0) {
                st.score_f32 = (st.total_bytes as f32) / (target_bytes as f32);
            }
        }
        return st;
    }

    /// Attempts to build a conflict-free L0 -> L1 compaction plan by expanding overlapping L0 clusters.
    fn tryPlanL0(score: f32): CompactionPlan {
        let seed_i: usize = 0;
        while (seed_i < MAX_LSM_SST_FILES) : (seed_i += 1) {
            const seed = _.files[seed_i];
            if (!seed.occupied || seed.level != 0 || seed.being_compacted) {
                continue;
            }

            // Fixed-point L0 overlap cluster expansion
            let in_cluster: [128]bool = [false; 128];
            in_cluster[seed_i] = true;

            let min_buf: [64]u8 = [0; 64];
            let max_buf: [64]u8 = [0; 64];
            let min_s: []u8 = min_buf[0..];
            let max_s: []u8 = max_buf[0..];
            let min_len: usize = copyBytes(min_s, seed.minKeySlice());
            let max_len: usize = copyBytes(max_s, seed.maxKeySlice());

            let conflict = false;
            let expanded = true;
            while (expanded && !conflict) {
                expanded = false;
                let k: usize = 0;
                while (k < MAX_LSM_SST_FILES) : (k += 1) {
                    if (!_.files[k].occupied || _.files[k].level != 0 || in_cluster[k]) {
                        continue;
                    }
                    if (_.files[k].overlapsRange(min_buf[0..min_len], max_buf[0..max_len])) {
                        if (_.files[k].being_compacted) {
                            conflict = true;
                            break;
                        }
                        in_cluster[k] = true;
                        expanded = true;
                        if (compareKeyBytesLex(_.files[k].minKeySlice(), min_buf[0..min_len]) < 0) {
                            let md: []u8 = min_buf[0..];
                            min_len = copyBytes(md, _.files[k].minKeySlice());
                        }
                        if (compareKeyBytesLex(_.files[k].maxKeySlice(), max_buf[0..max_len]) > 0) {
                            let xd: []u8 = max_buf[0..];
                            max_len = copyBytes(xd, _.files[k].maxKeySlice());
                        }
                    }
                }
            }
            if (conflict) {
                continue;
            }

            // Collect all overlapping L1 files and verify none are being compacted
            let plan = CompactionPlan{
                valid: true,
                reason: LsmCompactionReason.l0_file_count,
                source_level: 0,
                target_level: 1,
                is_trivial_move: false,
                priority_score_f32: score,
            };

            let k: usize = 0;
            while (k < MAX_LSM_SST_FILES) : (k += 1) {
                if (in_cluster[k]) {
                    if (plan.source_count < MAX_COMPACTION_INPUTS) {
                        plan.source_file_ids[plan.source_count] = _.files[k].file_id;
                        plan.source_count += 1;
                    }
                    plan.estimated_read_bytes = saturatingAddU64(plan.estimated_read_bytes, _.files[k].size_bytes);
                }
            }

            let t: usize = 0;
            while (t < MAX_LSM_SST_FILES) : (t += 1) {
                if (!_.files[t].occupied || _.files[t].level != 1) {
                    continue;
                }
                if (_.files[t].overlapsRange(min_buf[0..min_len], max_buf[0..max_len])) {
                    if (_.files[t].being_compacted) {
                        conflict = true;
                        break;
                    }
                    if (plan.target_count < MAX_COMPACTION_INPUTS) {
                        plan.target_file_ids[plan.target_count] = _.files[t].file_id;
                        plan.target_count += 1;
                    }
                    plan.estimated_read_bytes = saturatingAddU64(plan.estimated_read_bytes, _.files[t].size_bytes);
                }
            }
            if (!conflict && plan.source_count > 0) {
                return plan;
            }
        }
        return CompactionPlan{ valid: false };
    }

    /// Attempts to build a conflict-free Ln -> Ln+1 compaction plan for a specific source `src_idx`.
    fn tryPlanSingleFile(
        src_idx: usize,
        reason: LsmCompactionReason,
        score: f32,
        allow_trivial_move: bool,
    ): CompactionPlan {
        const src = _.files[src_idx];
        const next_lvl: u8 = src.level + 1;
        let plan = CompactionPlan{
            valid: true,
            reason: reason,
            source_level: src.level,
            target_level: next_lvl,
            is_trivial_move: false,
            priority_score_f32: score,
        };
        plan.source_file_ids[0] = src.file_id;
        plan.source_count = 1;
        plan.estimated_read_bytes = src.size_bytes;

        let t: usize = 0;
        while (t < MAX_LSM_SST_FILES) : (t += 1) {
            if (!_.files[t].occupied || _.files[t].level != next_lvl) {
                continue;
            }
            if (_.files[t].overlapsRange(src.minKeySlice(), src.maxKeySlice())) {
                if (_.files[t].being_compacted) {
                    return CompactionPlan{ valid: false };
                }
                if (plan.target_count < MAX_COMPACTION_INPUTS) {
                    plan.target_file_ids[plan.target_count] = _.files[t].file_id;
                    plan.target_count += 1;
                }
                plan.estimated_read_bytes = saturatingAddU64(plan.estimated_read_bytes, _.files[t].size_bytes);
            }
        }

        // Zero-write-amplification trivial move fast path when Ln+1 has zero overlap
        if (allow_trivial_move && plan.target_count == 0) {
            plan.is_trivial_move = true;
            plan.reason = LsmCompactionReason.trivial_move;
            plan.estimated_read_bytes = 0;
        }
        return plan;
    }

    /// Attempts to plan an Ln -> Ln+1 (`n >= 1`) size-overflow compaction, selecting largest / highest-tombstone
    /// non-compacting file whose Ln+1 overlap set has no active conflict.
    fn tryPlanLevelN(level: u8, score: f32): CompactionPlan {
        let tried: [128]bool = [false; 128];
        while (true) {
            let best_opt: usize? = null;
            let i: usize = 0;
            while (i < MAX_LSM_SST_FILES) : (i += 1) {
                if (!_.files[i].occupied || _.files[i].level != level || _.files[i].being_compacted || tried[i]) {
                    continue;
                }
                if (b_idx := best_opt) {
                    const cur = _.files[i];
                    const best = _.files[b_idx];
                    const better: bool = cur.size_bytes > best.size_bytes ||
                        (cur.size_bytes == best.size_bytes &&
                            (cur.tombstone_bps > best.tombstone_bps ||
                                (cur.tombstone_bps == best.tombstone_bps && cur.file_id < best.file_id)));
                    if (better) {
                        best_opt = i;
                    }
                } else {
                    best_opt = i;
                }
            }
            const pick = best_opt ?? break;
            tried[pick] = true;

            const plan = _.tryPlanSingleFile(pick, LsmCompactionReason.level_size_overflow, score, true);
            if (plan.valid) {
                return plan;
            }
        }
        return CompactionPlan{ valid: false };
    }

    /// Selects the highest-priority conflict-free compaction job across all non-bottom levels:
    /// 1. Level pressure >= 1.0 (highest score first): L0 overlap cluster or Ln->Ln+1 (with trivial_move)
    /// 2. Fallback: high-tombstone reclamation (`tombstone_bps >= tombstone_trigger_bps`)
    fn plan_next_compaction(): CompactionPlan {
        const last_movable_lvl: usize = _.cfg.max_levels - 1;
        let tried_level: [7]bool = [false; 7];

        // 1. Pressure-driven compaction (highest score >= 1.0 first)
        while (true) {
            let best_lvl_opt: usize? = null;
            let best_score: f32 = 0.0;
            let lvl: usize = 0;
            while (lvl < last_movable_lvl) : (lvl += 1) {
                if (tried_level[lvl]) {
                    continue;
                }
                const st = _.level_stats(lvl as u8);
                if (st.score_f32 >= 1.0) {
                    if (best_lvl_opt == null || st.score_f32 > best_score) {
                        best_lvl_opt = lvl;
                        best_score = st.score_f32;
                    }
                }
            }
            const chosen_lvl = best_lvl_opt ?? break;
            tried_level[chosen_lvl] = true;

            const plan = if (chosen_lvl == 0) {
                _.tryPlanL0(best_score)
            } else {
                _.tryPlanLevelN(chosen_lvl as u8, best_score)
            };
            if (plan.valid) {
                return plan;
            }
        }

        // 2. Fallback: high-tombstone reclamation across all non-bottom levels
        let tried_tomb: [128]bool = [false; 128];
        while (true) {
            let best_opt: usize? = null;
            let i: usize = 0;
            while (i < MAX_LSM_SST_FILES) : (i += 1) {
                const f = _.files[i];
                if (!f.occupied || f.being_compacted || tried_tomb[i]) {
                    continue;
                }
                if ((f.level as usize) >= last_movable_lvl) {
                    continue;
                }
                if (f.tombstone_bps < _.cfg.tombstone_trigger_bps) {
                    continue;
                }
                if (b_idx := best_opt) {
                    const best = _.files[b_idx];
                    const better: bool = f.tombstone_bps > best.tombstone_bps ||
                        (f.tombstone_bps == best.tombstone_bps &&
                            (f.size_bytes > best.size_bytes ||
                                (f.size_bytes == best.size_bytes && f.file_id < best.file_id)));
                    if (better) {
                        best_opt = i;
                    }
                } else {
                    best_opt = i;
                }
            }
            const pick = best_opt ?? break;
            tried_tomb[pick] = true;

            const score: f32 = (_.files[pick].tombstone_bps as f32) / (_.cfg.tombstone_trigger_bps as f32);
            const plan = _.tryPlanSingleFile(pick, LsmCompactionReason.tombstone_reclaim, score, false);
            if (plan.valid) {
                return plan;
            }
        }

        return CompactionPlan{ valid: false };
    }
}
