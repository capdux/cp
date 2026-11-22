// Origin: Pure-CP Stdlib (Zero-Alloc StreamingLLM + H2O KV-Cache Compactor using @fma) | Created: 2026-11-22
// Pure CP zero-heap-allocation StreamingLLM + H2O (Heavy-Hitter Oracle) KV-Cache Compactor
// for long-context LLM inference and multi-turn AI Agents:
//   - Fixed-capacity inline KV slot table (`KV_COMPACTOR_MAX_SLOTS = 256`), zero heap allocation
//   - Per-decode-step in-place EMA attention importance update via hardware `@fma`:
//       attn_score_ema = @fma(ema_decay, attn_score_ema, sanitized_attn_weight)
//   - Automatic three-zone budget classification:
//       sink_budget (pinned initial attention sinks)
//     + recent_budget (sliding recent token window)
//     + heavy_budget (Top-H middle-zone heavy hitters by EMA importance)
//   - One-shot in-place compaction (`compactToBudget`) preserving relative chronological order
//     and returning evicted logical positions + detailed `KvCompactReport`

const std = @import("std");

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

pub const KV_COMPACTOR_MAX_SLOTS: usize = 256;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn sanitizeAttnWeight(w: f32): f32 {
    if (!isFiniteF32(w) || w <= 0.0) {
        return 0.0;
    }
    return w;
}

/// Returns true if slot `a` has strictly higher retention priority than slot `b`
/// in the H2O heavy-hitter middle zone.
fn isStrongerHeavyHitter(a: KvCompactorSlot, b: KvCompactorSlot): bool {
    if (a.attn_score_ema > b.attn_score_ema) {
        return true;
    }
    if (a.attn_score_ema < b.attn_score_ema) {
        return false;
    }
    if (a.last_step > b.last_step) {
        return true;
    }
    if (a.last_step < b.last_step) {
        return false;
    }
    return a.pos_id > b.pos_id;
}

// ============================================================================
// 1. Role, Slot, Config, Report & Stats Models
// ============================================================================

/// Zone role assigned to each active slot in `KvCacheCompactor`.
pub enum KvSlotRole {
    sink,
    heavy_hitter,
    recent_window,
    evictable,
}

/// Metadata for a single logical KV-Cache token slot.
pub struct KvCompactorSlot {
    pos_id: u64 = 0;
    attn_score_ema: f32 = 0.0;
    last_step: u64 = 0;
    role: KvSlotRole = KvSlotRole.evictable;
}

/// Three-zone KV-Cache compaction budget and EMA decay configuration.
pub struct KvCompactorConfig {
    sink_budget: usize = 4;
    heavy_budget: usize = 16;
    recent_budget: usize = 16;
    ema_decay: f32 = 0.9;
    auto_compact_on_full: bool = true;

    /// Returns default production configuration (`sink=4, heavy=16, recent=16, ema_decay=0.9`).
    fn defaults(): KvCompactorConfig {
        return KvCompactorConfig {};
    }

    /// Returns total retained KV slot budget (`sink_budget + heavy_budget + recent_budget`).
    fn totalBudget(): usize {
        return _.sink_budget + _.heavy_budget + _.recent_budget;
    }

    /// Validates budget bounds (`1 <= totalBudget() <= KV_COMPACTOR_MAX_SLOTS`)
    /// and ensures `ema_decay` is finite and within `(0.0, 1.0]`.
    fn isValid(): bool {
        const budget: usize = _.totalBudget();
        if (budget == 0 || budget > KV_COMPACTOR_MAX_SLOTS) {
            return false;
        }
        if (!isFiniteF32(_.ema_decay) || _.ema_decay <= 0.0 || _.ema_decay > 1.0) {
            return false;
        }
        return true;
    }
}

/// Detailed summary returned by `compactToBudget`.
pub struct KvCompactReport {
    before_len: usize = 0;
    after_len: usize = 0;
    evicted_count: usize = 0;
    written_evicted_pos: usize = 0;
    sink_kept: usize = 0;
    heavy_kept: usize = 0;
    recent_kept: usize = 0;
    step: u64 = 0;
}

/// Aggregate occupancy, role distribution, and lifecycle counters for `KvCacheCompactor`.
pub struct KvCompactorStats {
    active_len: usize = 0;
    total_budget: usize = 0;
    sink_count: usize = 0;
    heavy_count: usize = 0;
    recent_count: usize = 0;
    evictable_count: usize = 0;
    total_appended: u64 = 0;
    total_evicted: u64 = 0;
    total_compactions: u64 = 0;
    current_step: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc StreamingLLM + H2O KV-Cache Compactor
// ============================================================================

/// Zero-heap-allocation StreamingLLM + H2O KV-Cache Compactor backed by a fixed
/// inline slot array (`KV_COMPACTOR_MAX_SLOTS = 256`).
pub struct KvCacheCompactor {
    slots: [KV_COMPACTOR_MAX_SLOTS]KvCompactorSlot = [KvCompactorSlot {}; KV_COMPACTOR_MAX_SLOTS];
    active_len: usize = 0;
    cfg: KvCompactorConfig = KvCompactorConfig {};
    current_step: u64 = 0;
    total_appended: u64 = 0;
    total_evicted: u64 = 0;
    total_compactions: u64 = 0;

    /// Initializes a compactor with validated three-zone budget configuration.
    /// Returns `null` if budget sum is 0, exceeds `KV_COMPACTOR_MAX_SLOTS`, or `ema_decay` is invalid.
    fn init(cfg: KvCompactorConfig): KvCacheCompactor? {
        if (!cfg.isValid()) {
            return null;
        }
        return KvCacheCompactor {
            cfg: cfg,
        };
    }

    /// Returns the number of currently resident KV slots.
    fn len(): usize {
        return _.active_len;
    }

    /// Returns the physical fixed slot capacity (`KV_COMPACTOR_MAX_SLOTS`).
    fn capacity(): usize {
        return KV_COMPACTOR_MAX_SLOTS;
    }

    /// Returns the configured target retained budget (`sink + heavy + recent`).
    fn budget(): usize {
        return _.cfg.totalBudget();
    }

    /// Returns `true` when active slot count exceeds the configured three-zone budget.
    fn needsCompaction(): bool {
        return _.active_len > _.cfg.totalBudget();
    }

    /// Returns a copy of the slot at physical index `index`, or `null` if out of bounds.
    fn slotAt(index: usize): KvCompactorSlot? {
        if (index >= _.active_len) {
            return null;
        }
        return _.slots[index];
    }

    /// Returns a read-only slice view of all currently active slots in chronological order.
    fn activeSlots(): []const KvCompactorSlot {
        return _.slots[0.._.active_len];
    }

    /// Finds the physical slot index matching logical token position `pos_id`.
    fn findByPosId(pos_id: u64): usize? {
        let i: usize = 0;
        while (i < _.active_len) : (i += 1) {
            if (_.slots[i].pos_id == pos_id) {
                return i;
            }
        }
        return null;
    }

    /// Reclassifies all active slots into `sink`, `recent_window`, `heavy_hitter`, and `evictable`.
    fn classifyRoles(*_) {
        const n: usize = _.active_len;
        if (n == 0) {
            return;
        }

        // 1. Attention Sinks: first `s = min(sink_budget, n)` slots are permanently pinned.
        const sink_end: usize = if (_.cfg.sink_budget < n) _.cfg.sink_budget else n;
        let i: usize = 0;
        while (i < sink_end) : (i += 1) {
            _.slots[i].role = KvSlotRole.sink;
        }

        // 2. Recent Window: trailing `r = min(recent_budget, n - sink_end)` slots are retained.
        const rem_after_sink: usize = n - sink_end;
        const recent_len: usize = if (_.cfg.recent_budget < rem_after_sink) _.cfg.recent_budget else rem_after_sink;
        const recent_start: usize = n - recent_len;

        i = recent_start;
        while (i < n) : (i += 1) {
            _.slots[i].role = KvSlotRole.recent_window;
        }

        // 3. Middle Candidate Zone `[sink_end .. recent_start)`:
        //    Default all to `evictable`, then promote Top-H heavy hitters deterministically.
        const mid_len: usize = recent_start - sink_end;
        if (mid_len == 0) {
            return;
        }

        i = sink_end;
        while (i < recent_start) : (i += 1) {
            _.slots[i].role = KvSlotRole.evictable;
        }

        const heavy_keep: usize = if (_.cfg.heavy_budget < mid_len) _.cfg.heavy_budget else mid_len;
        let picked: usize = 0;
        while (picked < heavy_keep) : (picked += 1) {
            let best_idx: usize = recent_start;
            let j: usize = sink_end;
            while (j < recent_start) : (j += 1) {
                if (_.slots[j].role != KvSlotRole.evictable) {
                    continue;
                }
                if (best_idx == recent_start || isStrongerHeavyHitter(_.slots[j], _.slots[best_idx])) {
                    best_idx = j;
                }
            }
            if (best_idx == recent_start) {
                break;
            }
            _.slots[best_idx].role = KvSlotRole.heavy_hitter;
        }
    }

    /// Evicts a single weakest non-sink slot to free 1 slot when physical capacity is full
    /// and `totalBudget() == KV_COMPACTOR_MAX_SLOTS`. Returns `false` if all slots are pinned sinks.
    fn evictOneWeakestNonSink(*_): bool {
        const n: usize = _.active_len;
        if (n == 0) {
            return false;
        }
        _.classifyRoles();
        const sink_end: usize = if (_.cfg.sink_budget < n) _.cfg.sink_budget else n;
        if (sink_end >= n) {
            return false;
        }

        // Prefer evicting an already-evictable slot; otherwise evict the weakest non-sink slot.
        let victim: usize = sink_end;
        let i: usize = sink_end + 1;
        while (i < n) : (i += 1) {
            const cur_evict: bool = _.slots[i].role == KvSlotRole.evictable;
            const vic_evict: bool = _.slots[victim].role == KvSlotRole.evictable;
            if (cur_evict && !vic_evict) {
                victim = i;
            } else if (cur_evict == vic_evict) {
                if (isStrongerHeavyHitter(_.slots[victim], _.slots[i])) {
                    victim = i;
                }
            }
        }

        // Shift left in-place to preserve chronological order.
        let k: usize = victim;
        while (k + 1 < n) : (k += 1) {
            _.slots[k] = _.slots[k + 1];
        }
        _.slots[n - 1] = KvCompactorSlot {};
        _.active_len = n - 1;
        _.total_evicted += 1;
        _.total_compactions += 1;
        return true;
    }

    /// Appends a new decoded token slot with logical position `pos_id` and sanitized initial
    /// attention weight. When the physical pool (`KV_COMPACTOR_MAX_SLOTS`) is full:
    /// - If `auto_compact_on_full` is `true`, automatically compacts to budget (or evicts 1
    ///   weakest non-sink slot if `totalBudget() == KV_COMPACTOR_MAX_SLOTS`).
    /// - If `auto_compact_on_full` is `false` or all slots are pinned sinks, returns `false`.
    fn appendToken(*_, pos_id: u64, initial_attn: f32): bool {
        if (_.active_len >= KV_COMPACTOR_MAX_SLOTS) {
            if (!_.cfg.auto_compact_on_full) {
                return false;
            }
            let empty_out: [0]u64 = [];
            let empty_s: []u64 = empty_out[0..];
            * = _.compactToBudget(*empty_s);
            if (_.active_len >= KV_COMPACTOR_MAX_SLOTS) {
                if (!_.evictOneWeakestNonSink()) {
                    return false;
                }
            }
        }

        _.current_step += 1;
        _.total_appended += 1;
        const clean_w: f32 = sanitizeAttnWeight(initial_attn);
        _.slots[_.active_len] = KvCompactorSlot {
            pos_id: pos_id,
            attn_score_ema: clean_w,
            last_step: _.current_step,
            role: KvSlotRole.recent_window,
        };
        _.active_len += 1;
        _.classifyRoles();
        return true;
    }

    /// Updates every resident slot's exponential moving average attention score in-place
    /// using hardware `@fma`: `score = @fma(ema_decay, score, clean_weight)`.
    /// Atomically returns `false` without mutation if `attn_weights.len != _.len()`.
    /// NaN, +/-Inf, and negative weights are sanitized to `0.0`.
    fn updateAttentionStep(*_, attn_weights: []const f32): bool {
        if (attn_weights.len != _.active_len) {
            return false;
        }
        _.current_step += 1;
        if (_.active_len == 0) {
            return true;
        }

        const decay: f32 = _.cfg.ema_decay;
        let i: usize = 0;
        while (i < _.active_len) : (i += 1) {
            const w: f32 = sanitizeAttnWeight(attn_weights[i]);
            _.slots[i].attn_score_ema = @fma(decay, _.slots[i].attn_score_ema, w);
            if (w > 0.0) {
                _.slots[i].last_step = _.current_step;
            }
        }
        _.classifyRoles();
        return true;
    }

    /// Compacts the KV slot table in-place down to the configured three-zone budget
    /// (`sink_budget + heavy_budget + recent_budget`), preserving chronological order of
    /// retained slots. Evicted logical `pos_id` values are written into `out_evicted_pos`
    /// up to `out_evicted_pos.len`, and a full `KvCompactReport` is returned.
    fn compactToBudget(*_, out_evicted_pos: *[]u64): KvCompactReport {
        const before: usize = _.active_len;
        if (before == 0) {
            return KvCompactReport {
                step: _.current_step,
            };
        }

        _.classifyRoles();

        let write_idx: usize = 0;
        let evicted_count: usize = 0;
        let written: usize = 0;
        let sink_kept: usize = 0;
        let heavy_kept: usize = 0;
        let recent_kept: usize = 0;

        let i: usize = 0;
        while (i < before) : (i += 1) {
            const slot = _.slots[i];
            if (slot.role == KvSlotRole.evictable) {
                if (written < out_evicted_pos.len) {
                    out_evicted_pos[written] = slot.pos_id;
                    written += 1;
                }
                evicted_count += 1;
            } else {
                if (write_idx != i) {
                    _.slots[write_idx] = slot;
                }
                if (slot.role == KvSlotRole.sink) {
                    sink_kept += 1;
                } else if (slot.role == KvSlotRole.heavy_hitter) {
                    heavy_kept += 1;
                } else {
                    recent_kept += 1;
                }
                write_idx += 1;
            }
        }

        // Clear vacated tail slots.
        let tail: usize = write_idx;
        while (tail < before) : (tail += 1) {
            _.slots[tail] = KvCompactorSlot {};
        }

        _.active_len = write_idx;
        if (evicted_count > 0) {
            _.total_evicted += evicted_count as u64;
            _.total_compactions += 1;
        }

        return KvCompactReport {
            before_len: before,
            after_len: write_idx,
            evicted_count: evicted_count,
            written_evicted_pos: written,
            sink_kept: sink_kept,
            heavy_kept: heavy_kept,
            recent_kept: recent_kept,
            step: _.current_step,
        };
    }

    /// Computes a snapshot of current slot role counts and cumulative lifecycle counters.
    fn stats(): KvCompactorStats {
        let sink_c: usize = 0;
        let heavy_c: usize = 0;
        let recent_c: usize = 0;
        let evict_c: usize = 0;

        let i: usize = 0;
        while (i < _.active_len) : (i += 1) {
            const role = _.slots[i].role;
            if (role == KvSlotRole.sink) {
                sink_c += 1;
            } else if (role == KvSlotRole.heavy_hitter) {
                heavy_c += 1;
            } else if (role == KvSlotRole.recent_window) {
                recent_c += 1;
            } else {
                evict_c += 1;
            }
        }

        return KvCompactorStats {
            active_len: _.active_len,
            total_budget: _.cfg.totalBudget(),
            sink_count: sink_c,
            heavy_count: heavy_c,
            recent_count: recent_c,
            evictable_count: evict_c,
            total_appended: _.total_appended,
            total_evicted: _.total_evicted,
            total_compactions: _.total_compactions,
            current_step: _.current_step,
        };
    }

    /// Resets all active slots and lifecycle counters while preserving configuration.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.active_len) : (i += 1) {
            _.slots[i] = KvCompactorSlot {};
        }
        _.active_len = 0;
        _.current_step = 0;
        _.total_appended = 0;
        _.total_evicted = 0;
        _.total_compactions = 0;
    }
}
