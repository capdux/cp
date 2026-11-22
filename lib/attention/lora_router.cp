// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Multi-LoRA Adapter Cache Router & SGMV Batch Planner) | Created: 2026-11-22
// Pure CP zero-heap-allocation S-LoRA / Punica / vLLM style multi-tenant LoRA adapter residency manager,
// reference-counted LRU eviction engine, stable SGMV batch group planner, and SIMD low-rank delta kernel:
//   - Fixed 32-slot inline adapter cache (`MultiLoraRouter`) with memory budget & pin/refcount guards
//   - Atomic OOM pre-flight check preventing partial eviction when all slots are pinned/in-flight
//   - Zero-aux-buffer stable SGMV batch grouping (`planLoraSgmvBatch`) isolating base-only (`id == 0`) requests
//   - Hardware SIMD `@dot` + `@fma` low-rank delta forward kernel (`applyLoraDeltaSimdF32`)

const std = @import("std");

// ============================================================================
// 1. Constants & Helpers
// ============================================================================

/// Maximum resident LoRA adapter slots in `MultiLoraRouter` (zero dynamic heap).
pub const LORA_ROUTER_MAX_ADAPTERS: usize = 32;
/// Recommended maximum distinct adapter groups per SGMV batch.
pub const LORA_MAX_BATCH_GROUPS: usize = 32;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

// ============================================================================
// 2. Public Models: Config, Slot, Acquire Outcome, Batch Group & Stats
// ============================================================================

/// Configuration for `MultiLoraRouter`.
pub struct LoraRouterConfig {
    max_loaded_adapters: usize = LORA_ROUTER_MAX_ADAPTERS;
    memory_budget_bytes: u64 = 0;
    max_rank: usize = 64;

    /// Validates slot limit (`1..=32`), non-zero memory budget, and non-zero max rank.
    fn isValid(): bool {
        if (_.max_loaded_adapters == 0 || _.max_loaded_adapters > LORA_ROUTER_MAX_ADAPTERS) {
            return false;
        }
        if (_.memory_budget_bytes == 0 || _.max_rank == 0) {
            return false;
        }
        return true;
    }
}

/// Inline metadata slot for a resident LoRA adapter in GPU/host HBM cache.
pub struct LoraAdapterSlot {
    adapter_id: u64 = 0;
    rank: usize = 0;
    alpha_f32: f32 = 0.0;
    scale_f32: f32 = 0.0;
    memory_bytes: u64 = 0;
    active_refs: u32 = 0;
    last_used_step: u64 = 0;
    pinned: bool = false;
    occupied: bool = false;
}

/// Result returned by `MultiLoraRouter.acquire_adapter`.
pub struct LoraAcquireOutcome {
    ok: bool = false;
    slot_idx: usize = 0;
    cache_hit: bool = false;
    evicted_count: usize = 0;
    oom_blocked: bool = false;
}

/// Contiguous request slice belonging to a single `adapter_id` (`0` denotes base-only requests).
pub struct LoraBatchGroup {
    adapter_id: u64 = 0;
    start_offset: usize = 0;
    req_count: usize = 0;
}

/// Operational cache & memory statistics snapshot for `MultiLoraRouter`.
pub struct LoraRouterStats {
    loaded_count: usize = 0;
    pinned_count: usize = 0;
    memory_used_bytes: u64 = 0;
    cache_hits: u64 = 0;
    cache_misses: u64 = 0;
    evictions: u64 = 0;
}

// ============================================================================
// 3. Zero-Alloc Multi-LoRA Residency Cache & LRU Eviction Router
// ============================================================================

/// Zero-heap-allocation multi-tenant LoRA adapter residency manager with refcount-safe LRU eviction.
pub struct MultiLoraRouter {
    cfg: LoraRouterConfig = LoraRouterConfig{};
    slots: [LORA_ROUTER_MAX_ADAPTERS]LoraAdapterSlot = [LoraAdapterSlot{}; LORA_ROUTER_MAX_ADAPTERS];
    loaded_count: usize = 0;
    memory_used_bytes: u64 = 0;
    cache_hits: u64 = 0;
    cache_misses: u64 = 0;
    evictions: u64 = 0;

    /// Initializes a validated `MultiLoraRouter`. Returns `null` if `cfg.isValid()` is `false`.
    static fn init(cfg: LoraRouterConfig): MultiLoraRouter? {
        if (!cfg.isValid()) {
            return null;
        }
        return MultiLoraRouter{
            cfg: cfg,
            slots: [LoraAdapterSlot{}; LORA_ROUTER_MAX_ADAPTERS],
            loaded_count: 0,
            memory_used_bytes: 0,
            cache_hits: 0,
            cache_misses: 0,
            evictions: 0,
        };
    }

    fn findAdapterSlot(adapter_id: u64): usize? {
        if (adapter_id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.cfg.max_loaded_adapters) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].adapter_id == adapter_id) {
                return i;
            }
        }
        return null;
    }

    /// Acquires a reference to `adapter_id` at scheduler `step`.
    ///
    /// Behavior:
    /// - Cache hit: increments `active_refs`, refreshes `last_used_step`, optionally upgrades `pinned`.
    /// - Cache miss: pre-checks whether evicting cold (`active_refs == 0 && !pinned`) adapters can
    ///   satisfy both slot and `memory_budget_bytes` limits; if not, leaves cache untouched and returns
    ///   `oom_blocked = true`. Otherwise evicts strictly in LRU (`last_used_step` ascending) order and loads.
    fn acquire_adapter(
        *_,
        adapter_id: u64,
        rank: usize,
        alpha_f32: f32,
        mem_bytes: u64,
        step: u64,
        pin: bool,
    ): LoraAcquireOutcome {
        if (adapter_id == 0 || rank == 0 || rank > _.cfg.max_rank || mem_bytes == 0) {
            return LoraAcquireOutcome{};
        }
        if (!isFiniteF32(alpha_f32) || alpha_f32 <= 0.0) {
            return LoraAcquireOutcome{};
        }

        // 1. Check cache hit first
        if (hit_idx := _.findAdapterSlot(adapter_id)) {
            _.slots[hit_idx].active_refs += 1;
            _.slots[hit_idx].last_used_step = step;
            if (pin) {
                _.slots[hit_idx].pinned = true;
            }
            _.cache_hits += 1;
            return LoraAcquireOutcome{
                ok: true,
                slot_idx: hit_idx,
                cache_hit: true,
                evicted_count: 0,
                oom_blocked: false,
            };
        }

        // 2. Cache miss
        _.cache_misses += 1;
        if (mem_bytes > _.cfg.memory_budget_bytes) {
            return LoraAcquireOutcome{
                ok: false,
                slot_idx: 0,
                cache_hit: false,
                evicted_count: 0,
                oom_blocked: true,
            };
        }

        // Atomic pre-flight check: ensure enough cold capacity exists before evicting anything
        let reclaimable_slots: usize = 0;
        let reclaimable_bytes: u64 = 0;
        let i: usize = 0;
        while (i < _.cfg.max_loaded_adapters) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].active_refs == 0 && !_.slots[i].pinned) {
                reclaimable_slots += 1;
                reclaimable_bytes += _.slots[i].memory_bytes;
            }
        }

        const need_slot = _.loaded_count >= _.cfg.max_loaded_adapters;
        const locked_bytes = _.memory_used_bytes - reclaimable_bytes;
        if ((need_slot && reclaimable_slots == 0) || (locked_bytes + mem_bytes > _.cfg.memory_budget_bytes)) {
            return LoraAcquireOutcome{
                ok: false,
                slot_idx: 0,
                cache_hit: false,
                evicted_count: 0,
                oom_blocked: true,
            };
        }

        // 3. Evict cold adapters strictly in LRU order until both slot & memory fit
        let evicted: usize = 0;
        while (_.loaded_count >= _.cfg.max_loaded_adapters ||
            _.memory_used_bytes + mem_bytes > _.cfg.memory_budget_bytes)
        {
            let victim_opt: usize? = null;
            let best_step: u64 = 0xFFFFFFFFFFFFFFFF;
            let k: usize = 0;
            while (k < _.cfg.max_loaded_adapters) : (k += 1) {
                if (_.slots[k].occupied && _.slots[k].active_refs == 0 && !_.slots[k].pinned) {
                    if (victim_opt == null || _.slots[k].last_used_step < best_step) {
                        victim_opt = k;
                        best_step = _.slots[k].last_used_step;
                    }
                }
            }
            const victim = victim_opt ?? return LoraAcquireOutcome{
                ok: false,
                slot_idx: 0,
                cache_hit: false,
                evicted_count: evicted,
                oom_blocked: true,
            };

            _.memory_used_bytes -= _.slots[victim].memory_bytes;
            _.slots[victim] = LoraAdapterSlot{};
            _.loaded_count -= 1;
            evicted += 1;
            _.evictions += 1;
        }

        // 4. Insert into first free slot
        let target: usize = 0;
        let found_free = false;
        let f: usize = 0;
        while (f < _.cfg.max_loaded_adapters) : (f += 1) {
            if (!_.slots[f].occupied) {
                target = f;
                found_free = true;
                break;
            }
        }
        if (!found_free) {
            return LoraAcquireOutcome{
                ok: false,
                slot_idx: 0,
                cache_hit: false,
                evicted_count: evicted,
                oom_blocked: true,
            };
        }

        _.slots[target] = LoraAdapterSlot{
            adapter_id: adapter_id,
            rank: rank,
            alpha_f32: alpha_f32,
            scale_f32: alpha_f32 / (rank as f32),
            memory_bytes: mem_bytes,
            active_refs: 1,
            last_used_step: step,
            pinned: pin,
            occupied: true,
        };
        _.loaded_count += 1;
        _.memory_used_bytes += mem_bytes;

        return LoraAcquireOutcome{
            ok: true,
            slot_idx: target,
            cache_hit: false,
            evicted_count: evicted,
            oom_blocked: false,
        };
    }

    /// Decrements `active_refs` for `adapter_id` with underflow protection.
    fn release_adapter(*_, adapter_id: u64): bool {
        const idx = _.findAdapterSlot(adapter_id) ?? return false;
        if (_.slots[idx].active_refs == 0) {
            return false;
        }
        _.slots[idx].active_refs -= 1;
        return true;
    }

    /// Clears the `pinned` flag on `adapter_id`, making it eligible for LRU eviction when `active_refs == 0`.
    fn unpin_adapter(*_, adapter_id: u64): bool {
        const idx = _.findAdapterSlot(adapter_id) ?? return false;
        if (!_.slots[idx].pinned) {
            return false;
        }
        _.slots[idx].pinned = false;
        return true;
    }

    /// Returns a read-only copy of slot `slot_idx`, or `null` if out-of-range or unoccupied.
    fn get_slot(slot_idx: usize): LoraAdapterSlot? {
        if (slot_idx >= _.cfg.max_loaded_adapters || !_.slots[slot_idx].occupied) {
            return null;
        }
        return _.slots[slot_idx];
    }

    /// Returns an operational statistics snapshot.
    fn stats(): LoraRouterStats {
        let pinned: usize = 0;
        let i: usize = 0;
        while (i < _.cfg.max_loaded_adapters) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].pinned) {
                pinned += 1;
            }
        }
        return LoraRouterStats{
            loaded_count: _.loaded_count,
            pinned_count: pinned,
            memory_used_bytes: _.memory_used_bytes,
            cache_hits: _.cache_hits,
            cache_misses: _.cache_misses,
            evictions: _.evictions,
        };
    }
}

// ============================================================================
// 4. Zero-Alloc SGMV Batch Planner & SIMD Low-Rank Delta Kernel
// ============================================================================

/// Stably groups mixed multi-tenant requests by `adapter_id` into contiguous SGMV execution slices.
/// - Preserves first-appearance order of distinct adapters and original request order within each group.
/// - Base-only requests (`adapter_id == 0`) are grouped cleanly as their own group.
/// - Returns the number of groups written (`0` on empty input or insufficient output buffer capacity).
pub fn planLoraSgmvBatch(
    req_adapter_ids: []const u64,
    out_groups: *[]LoraBatchGroup,
    out_sorted_req_indices: *[]usize,
): usize {
    if (req_adapter_ids.len == 0 || out_groups.len == 0 || out_sorted_req_indices.len < req_adapter_ids.len) {
        return 0;
    }

    // Pass 1: discover distinct adapter IDs in stable first-appearance order
    let num_groups: usize = 0;
    let r: usize = 0;
    while (r < req_adapter_ids.len) : (r += 1) {
        const aid = req_adapter_ids[r];
        let found = false;
        let g: usize = 0;
        while (g < num_groups) : (g += 1) {
            if (out_groups[g].adapter_id == aid) {
                out_groups[g].req_count += 1;
                found = true;
                break;
            }
        }
        if (!found) {
            if (num_groups >= out_groups.len) {
                return 0;
            }
            out_groups[num_groups] = LoraBatchGroup{
                adapter_id: aid,
                start_offset: 0,
                req_count: 1,
            };
            num_groups += 1;
        }
    }

    // Pass 2: assign contiguous offsets and stably scatter request indices with zero auxiliary memory
    let cursor: usize = 0;
    let g: usize = 0;
    while (g < num_groups) : (g += 1) {
        out_groups[g].start_offset = cursor;
        const target_aid = out_groups[g].adapter_id;
        let idx: usize = 0;
        while (idx < req_adapter_ids.len) : (idx += 1) {
            if (req_adapter_ids[idx] == target_aid) {
                out_sorted_req_indices[cursor] = idx;
                cursor += 1;
            }
        }
    }

    return num_groups;
}

/// Applies a LoRA low-rank delta `y += scale * (B * (A * x))` in place using CP's native
/// 8-lane SIMD `@dot` and hardware `@fma` intrinsics with zero heap allocations.
///
/// Matrix layouts (row-major):
/// - `x`: `[in_dim]`
/// - `a_mat`: `[rank, in_dim]`
/// - `b_mat`: `[out_dim, rank]`
/// - `scratch_mid`: caller-owned temporary buffer of length `>= rank`
/// - `y_inout`: `[out_dim]` accumulated in place
pub fn applyLoraDeltaSimdF32(
    x: []const f32,
    a_mat: []const f32,
    b_mat: []const f32,
    in_dim: usize,
    rank: usize,
    out_dim: usize,
    scale: f32,
    scratch_mid: *[]f32,
    y_inout: *[]f32,
): bool {
    if (in_dim == 0 || rank == 0 || out_dim == 0 || !isFiniteF32(scale)) {
        return false;
    }
    if (rank > 0xFFFFFFFFFFFFFFFF / in_dim || out_dim > 0xFFFFFFFFFFFFFFFF / rank) {
        return false;
    }
    if (x.len < in_dim || a_mat.len < rank * in_dim || b_mat.len < out_dim * rank) {
        return false;
    }
    if (scratch_mid.len < rank || y_inout.len < out_dim) {
        return false;
    }

    const x_vec = x[0..in_dim];
    let r: usize = 0;
    while (r < rank) : (r += 1) {
        const a_row = a_mat[r * in_dim .. (r + 1) * in_dim];
        scratch_mid[r] = @dot(a_row, x_vec);
    }

    const mid_vec: []const f32 = scratch_mid[0..rank];
    let o: usize = 0;
    while (o < out_dim) : (o += 1) {
        const b_row = b_mat[o * rank .. (o + 1) * rank];
        const delta: f32 = @dot(b_row, mid_vec);
        y_inout[o] = @fma(scale, delta, y_inout[o]);
    }

    return true;
}
