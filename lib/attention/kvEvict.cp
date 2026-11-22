// Origin: Pure-CP Stdlib (Zero-Alloc Long-Context KV-Cache Eviction: StreamingLLM Attention Sink + H2O Heavy-Hitter Oracle) | Created: 2026-11-22
// Pure CP zero-heap-allocation KV-Cache eviction policy engine for long-context LLM inference
// and multi-turn AI Agents, unifying:
//   - StreamingLLM Attention Sinks (permanently pinned initial tokens) + Sliding Recent Window
//   - H2O (Heavy-Hitter Oracle) cumulative attention score tracking with exponential decay (@fma)
//   - Hybrid three-zone (Sink + Heavy-Hitter + Recent Window) quota-conserving promotion & eviction

const std = @import("std");

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

pub const MAX_KV_EVICTION_SLOTS: usize = 256;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isWeakerVictim(a: KvSlotMeta, b: KvSlotMeta): bool {
    if (a.cum_attn_score < b.cum_attn_score) {
        return true;
    }
    if (a.cum_attn_score > b.cum_attn_score) {
        return false;
    }
    if (a.last_access_step < b.last_access_step) {
        return true;
    }
    if (a.last_access_step > b.last_access_step) {
        return false;
    }
    return a.token_pos < b.token_pos;
}

// ============================================================================
// 1. Policy, Role, Config, Slot, Result & Stats Models
// ============================================================================

/// Eviction strategy combining StreamingLLM Attention Sink, Sliding Window, and H2O Heavy-Hitter.
pub enum KvEvictionPolicyKind {
    streaming_sink_window,
    h2o_heavy_hitter,
    hybrid_sink_h2o_window,
}

/// Logical cache-zone assignment for a resident KV slot.
pub enum KvTokenRole {
    empty,
    attention_sink,
    heavy_hitter,
    recent_window,
}

/// Configuration for three-zone KV-Cache eviction budgets and attention score decay.
pub struct KvEvictionConfig {
    sink_budget: usize = 4;
    heavy_budget: usize = 16;
    recent_budget: usize = 16;
    score_decay_f32: f32 = 0.95;
    policy: KvEvictionPolicyKind = KvEvictionPolicyKind.hybrid_sink_h2o_window;

    /// Returns total resident KV slot capacity (`sink_budget + heavy_budget + recent_budget`).
    fn totalCapacity(): usize {
        return _.sink_budget + _.heavy_budget + _.recent_budget;
    }

    /// Validates budgets against `MAX_KV_EVICTION_SLOTS` and ensures `score_decay_f32 ∈ (0.0, 1.0]`.
    fn isValid(): bool {
        const cap: usize = _.totalCapacity();
        if (cap == 0 || cap > MAX_KV_EVICTION_SLOTS) {
            return false;
        }
        if (!isFiniteF32(_.score_decay_f32) || _.score_decay_f32 <= 0.0 || _.score_decay_f32 > 1.0) {
            return false;
        }
        if (_.policy == KvEvictionPolicyKind.streaming_sink_window && _.recent_budget == 0) {
            return false;
        }
        if (_.policy == KvEvictionPolicyKind.h2o_heavy_hitter && _.heavy_budget == 0 && _.recent_budget == 0) {
            return false;
        }
        return true;
    }
}

/// Metadata for a single resident KV-Cache token slot.
pub struct KvSlotMeta {
    token_pos: u64 = 0;
    kv_block_idx: u32 = 0;
    cum_attn_score: f32 = 0.0;
    last_access_step: u64 = 0;
    role: KvTokenRole = KvTokenRole.empty;
    occupied: bool = false;
}

/// Result of admitting a new token into `KvEvictionManager`.
pub struct KvEvictionResult {
    admitted: bool = false;
    evicted: bool = false;
    assigned_slot: usize = 0;
    evicted_token_pos: u64 = 0;
    evicted_kv_block_idx: u32 = 0;
}

/// Aggregate occupancy, role distribution, and compression statistics.
pub struct KvEvictionStats {
    total_admitted: u64 = 0;
    total_evicted: u64 = 0;
    sink_count: usize = 0;
    heavy_count: usize = 0;
    recent_count: usize = 0;
    compression_ratio_f32: f32 = 1.0;
}

// ============================================================================
// 2. Zero-Alloc KV-Cache Eviction Manager (StreamingLLM + H2O)
// ============================================================================

/// Zero-heap-allocation KV-Cache eviction manager backed by fixed inline slots (`MAX_KV_EVICTION_SLOTS = 256`).
pub struct KvEvictionManager {
    cfg: KvEvictionConfig = KvEvictionConfig{};
    slots: [256]KvSlotMeta = [KvSlotMeta{}; 256];
    occupied_count: usize = 0;
    sink_count: usize = 0;
    heavy_count: usize = 0;
    recent_count: usize = 0;
    total_admitted: u64 = 0;
    total_evicted: u64 = 0;
    step_counter: u64 = 0;

    /// Initializes a zero-alloc KV eviction manager. Returns `null` if `cfg` is invalid.
    static fn init(cfg: KvEvictionConfig): KvEvictionManager? {
        if (!cfg.isValid()) {
            return null;
        }
        return KvEvictionManager{
            cfg: cfg,
            slots: [KvSlotMeta{}; 256],
            occupied_count: 0,
            sink_count: 0,
            heavy_count: 0,
            recent_count: 0,
            total_admitted: 0,
            total_evicted: 0,
            step_counter: 0,
        };
    }

    /// Admits a new token at logical position `token_pos` mapped to physical `kv_block_idx`
    /// with initial attention score `init_score`.
    /// - Pins the first `sink_budget` tokens permanently as `attention_sink` (never evicted)
    /// - When full, promotes high-scoring tokens sliding out of `recent_window` into `heavy_hitter`
    ///   and evicts the lowest-score (LRU tie-broken) non-sink victim.
    fn admit_token(*_, token_pos: u64, kv_block_idx: u32, init_score: f32): KvEvictionResult {
        if (!isFiniteF32(init_score) || init_score < 0.0) {
            return KvEvictionResult{ admitted: false };
        }

        const cap: usize = _.cfg.totalCapacity();
        _.step_counter += 1;

        // 1. Free slot available: admit without eviction
        if (_.occupied_count < cap) {
            let free_slot: usize = 0;
            while (free_slot < cap) : (free_slot += 1) {
                if (!_.slots[free_slot].occupied) {
                    break;
                }
            }

            let assigned_role = KvTokenRole.recent_window;
            if (_.cfg.sink_budget > 0 && _.sink_count < _.cfg.sink_budget) {
                assigned_role = KvTokenRole.attention_sink;
                _.sink_count += 1;
            } else if (_.cfg.recent_budget > 0) {
                // If recent_window is already at budget while heavy_budget has room,
                // slide the oldest recent token into heavy_hitter to maintain zone budgets.
                if (_.recent_count >= _.cfg.recent_budget && _.heavy_count < _.cfg.heavy_budget) {
                    let oldest_r: usize? = null;
                    let oldest_pos: u64 = std.math.maxInt(u64);
                    let i: usize = 0;
                    while (i < cap) : (i += 1) {
                        if (_.slots[i].occupied && _.slots[i].role == KvTokenRole.recent_window) {
                            if (oldest_r == null || _.slots[i].token_pos < oldest_pos) {
                                oldest_r = i;
                                oldest_pos = _.slots[i].token_pos;
                            }
                        }
                    }
                    if (or_idx := oldest_r) {
                        _.slots[or_idx].role = KvTokenRole.heavy_hitter;
                        _.recent_count -= 1;
                        _.heavy_count += 1;
                    }
                }
                assigned_role = KvTokenRole.recent_window;
                _.recent_count += 1;
            } else {
                assigned_role = KvTokenRole.heavy_hitter;
                _.heavy_count += 1;
            }

            _.slots[free_slot] = KvSlotMeta{
                token_pos: token_pos,
                kv_block_idx: kv_block_idx,
                cum_attn_score: init_score,
                last_access_step: _.step_counter,
                role: assigned_role,
                occupied: true,
            };
            _.occupied_count += 1;
            _.total_admitted += 1;

            return KvEvictionResult{
                admitted: true,
                evicted: false,
                assigned_slot: free_slot,
            };
        }

        // 2. Cache full: select victim according to policy (never evicting attention_sink)
        let victim_opt: usize? = null;
        let promote_idx_opt: usize? = null;
        let new_role = KvTokenRole.recent_window;

        if (_.cfg.policy == KvEvictionPolicyKind.streaming_sink_window || _.cfg.heavy_budget == 0) {
            // Pure sliding window (with sinks pinned): evict oldest non-sink token_pos
            let oldest_pos: u64 = std.math.maxInt(u64);
            let i: usize = 0;
            while (i < cap) : (i += 1) {
                const s = _.slots[i];
                if (!s.occupied || s.role == KvTokenRole.attention_sink) {
                    continue;
                }
                if (victim_opt == null || s.token_pos < oldest_pos) {
                    victim_opt = i;
                    oldest_pos = s.token_pos;
                }
            }
            new_role = KvTokenRole.recent_window;
        } else if (_.cfg.recent_budget > 0) {
            // Hybrid Sink + H2O + Recent Window:
            // Find oldest token sliding out of recent_window and pit it against all heavy_hitters.
            let oldest_r_opt: usize? = null;
            let oldest_r_pos: u64 = std.math.maxInt(u64);
            let i: usize = 0;
            while (i < cap) : (i += 1) {
                const s = _.slots[i];
                if (s.occupied && s.role == KvTokenRole.recent_window) {
                    if (oldest_r_opt == null || s.token_pos < oldest_r_pos) {
                        oldest_r_opt = i;
                        oldest_r_pos = s.token_pos;
                    }
                }
            }

            // Start victim search with oldest_r (sliding out of recent_window)
            if (or_idx := oldest_r_opt) {
                victim_opt = or_idx;
            }
            // Compare against all heavy_hitter slots
            let h_idx: usize = 0;
            while (h_idx < cap) : (h_idx += 1) {
                const s = _.slots[h_idx];
                if (!s.occupied || s.role != KvTokenRole.heavy_hitter) {
                    continue;
                }
                if (v_idx := victim_opt) {
                    if (isWeakerVictim(s, _.slots[v_idx])) {
                        victim_opt = h_idx;
                    }
                } else {
                    victim_opt = h_idx;
                }
            }

            // If a heavy_hitter is weaker than oldest_r, promote oldest_r to heavy_hitter
            if (or_idx := oldest_r_opt) {
                if (v_idx := victim_opt) {
                    if (v_idx != or_idx) {
                        promote_idx_opt = or_idx;
                    }
                }
            }
            new_role = KvTokenRole.recent_window;
        } else {
            // Pure H2O (recent_budget == 0): evict weakest non-sink heavy_hitter directly
            let i: usize = 0;
            while (i < cap) : (i += 1) {
                const s = _.slots[i];
                if (!s.occupied || s.role == KvTokenRole.attention_sink) {
                    continue;
                }
                if (v_idx := victim_opt) {
                    if (isWeakerVictim(s, _.slots[v_idx])) {
                        victim_opt = i;
                    }
                } else {
                    victim_opt = i;
                }
            }
            new_role = KvTokenRole.heavy_hitter;
        }

        // If every single slot is an attention_sink, no eviction is permitted
        const victim_idx = victim_opt ?? return KvEvictionResult{ admitted: false };

        const evicted_pos: u64 = _.slots[victim_idx].token_pos;
        const evicted_blk: u32 = _.slots[victim_idx].kv_block_idx;
        const victim_role = _.slots[victim_idx].role;

        // Apply role count adjustments cleanly
        if (victim_role == KvTokenRole.heavy_hitter && _.heavy_count > 0) {
            _.heavy_count -= 1;
        } else if (victim_role == KvTokenRole.recent_window && _.recent_count > 0) {
            _.recent_count -= 1;
        }

        if (p_idx := promote_idx_opt) {
            _.slots[p_idx].role = KvTokenRole.heavy_hitter;
            if (_.recent_count > 0) {
                _.recent_count -= 1;
            }
            _.heavy_count += 1;
        }

        _.slots[victim_idx] = KvSlotMeta{
            token_pos: token_pos,
            kv_block_idx: kv_block_idx,
            cum_attn_score: init_score,
            last_access_step: _.step_counter,
            role: new_role,
            occupied: true,
        };
        if (new_role == KvTokenRole.heavy_hitter) {
            _.heavy_count += 1;
        } else {
            _.recent_count += 1;
        }

        _.total_admitted += 1;
        _.total_evicted += 1;

        return KvEvictionResult{
            admitted: true,
            evicted: true,
            assigned_slot: victim_idx,
            evicted_token_pos: evicted_pos,
            evicted_kv_block_idx: evicted_blk,
        };
    }

    /// Updates cumulative attention scores for all resident slots using hardware `@fma`:
    /// `score = @fma(score, cfg.score_decay_f32, step_scores[slot])`.
    /// Requires `step_scores.len >= cfg.totalCapacity()` and all entries finite & non-negative.
    fn update_attention_scores(*_, step_scores: []const f32): bool {
        const cap: usize = _.cfg.totalCapacity();
        if (step_scores.len < cap) {
            return false;
        }
        let i: usize = 0;
        while (i < cap) : (i += 1) {
            if (!isFiniteF32(step_scores[i]) || step_scores[i] < 0.0) {
                return false;
            }
        }

        _.step_counter += 1;
        const decay: f32 = _.cfg.score_decay_f32;
        i = 0;
        while (i < cap) : (i += 1) {
            if (_.slots[i].occupied) {
                const add_s: f32 = step_scores[i];
                _.slots[i].cum_attn_score = @fma(_.slots[i].cum_attn_score, decay, add_s);
                if (add_s > 0.0) {
                    _.slots[i].last_access_step = _.step_counter;
                }
            }
        }
        return true;
    }

    /// Looks up the slot index of a resident token by its logical `token_pos`.
    fn lookup_by_token_pos(token_pos: u64): usize? {
        const cap: usize = _.cfg.totalCapacity();
        let i: usize = 0;
        while (i < cap) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].token_pos == token_pos) {
                return i;
            }
        }
        return null;
    }

    /// Writes all resident logical `token_pos` values into `out_pos` sorted ascending
    /// (for contiguous RoPE position mapping) without heap allocation.
    fn collect_active_positions(out_pos: *[]u64): usize {
        if (out_pos.len == 0 || _.occupied_count == 0) {
            return 0;
        }
        const cap: usize = _.cfg.totalCapacity();
        const limit: usize = out_pos.len;
        let count: usize = 0;

        let i: usize = 0;
        while (i < cap) : (i += 1) {
            if (!_.slots[i].occupied) {
                continue;
            }
            const tp: u64 = _.slots[i].token_pos;
            if (count < limit) {
                let pos: usize = count;
                while (pos > 0 && tp < out_pos[pos - 1]) : (pos -= 1) {
                    out_pos[pos] = out_pos[pos - 1];
                }
                out_pos[pos] = tp;
                count += 1;
            } else if (tp < out_pos[limit - 1]) {
                let pos: usize = limit - 1;
                while (pos > 0 && tp < out_pos[pos - 1]) : (pos -= 1) {
                    out_pos[pos] = out_pos[pos - 1];
                }
                out_pos[pos] = tp;
            }
        }
        return count;
    }

    /// Returns aggregate admission, eviction, role counts, and KV compression ratio.
    fn stats(): KvEvictionStats {
        const ratio: f32 = if (_.occupied_count > 0) {
            (_.total_admitted as f32) / (_.occupied_count as f32)
        } else {
            1.0
        };
        return KvEvictionStats{
            total_admitted: _.total_admitted,
            total_evicted: _.total_evicted,
            sink_count: _.sink_count,
            heavy_count: _.heavy_count,
            recent_count: _.recent_count,
            compression_ratio_f32: ratio,
        };
    }

    /// Resets all slots and counters to empty state while preserving `cfg`.
    fn reset(*_) {
        const cap: usize = _.cfg.totalCapacity();
        let i: usize = 0;
        while (i < cap) : (i += 1) {
            _.slots[i] = KvSlotMeta{};
        }
        _.occupied_count = 0;
        _.sink_count = 0;
        _.heavy_count = 0;
        _.recent_count = 0;
        _.total_admitted = 0;
        _.total_evicted = 0;
        _.step_counter = 0;
    }
}
