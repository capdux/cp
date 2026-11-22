// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc ABA-Safe Generational Slab Handle Pool) | Created: 2026-11-22
// Pure CP zero-heap-allocation O(1) Generational Slab Object Handle Pool for Actor runtimes,
// connection handle tables, and GC-free resource registries:
//   - Fixed inline capacity up to 256 slots (`GEN_SLAB_MAX_SLOTS`) with 0 dynamic heap allocation
//   - O(1) LIFO free-stack reuse + bump cursor allocation with high-water-mark tracking
//   - 32-bit non-zero generation counters preventing ABA aliasing & double-free corruption
//   - Compact 64-bit lossless handle encoding (`GenHandle.toU64` / `GenHandle.fromU64`)
//   - Full invalidation on `clear()` keeping bumped generations intact across slot reuse

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum supported slot capacity in `GenSlabPool` (zero dynamic heap).
pub const GEN_SLAB_MAX_SLOTS: usize = 256;

/// Sentinel index indicating an empty/unused free-stack entry.
pub const GEN_SLAB_NO_FREE: u16 = 0xFFFF;

/// ABA-safe generational handle pairing a slot index with a non-zero 32-bit generation.
pub struct GenHandle {
    index: u16 = 0;
    generation: u32 = 0;

    /// Packs `(generation, index)` losslessly into a compact 64-bit integer:
    /// bits `[0..15]` = `index`, bits `[16..47]` = `generation`.
    fn toU64(): u64 {
        return ((_.generation as u64) << 16) | (_.index as u64);
    }

    /// Unpacks a `GenHandle` previously encoded via `toU64()`.
    static fn fromU64(raw: u64): GenHandle {
        return GenHandle{
            index: @intCast(raw & 0xFFFF),
            generation: @intCast((raw >> 16) & 0xFFFFFFFF),
        };
    }

    /// Returns `true` iff both `index` and `generation` match identically.
    fn eql(other: GenHandle): bool {
        return _.index == other.index && _.generation == other.generation;
    }
}

/// Telemetry & capacity snapshot returned by `GenSlabPool.stats()`.
pub struct GenSlabStats {
    capacity: usize = 0;
    live_count: usize = 0;
    high_water_mark: usize = 0;
    total_allocs: u64 = 0;
    total_frees: u64 = 0;
    stale_handle_rejects: u64 = 0;
}

// ============================================================================
// 2. Internal Generation Helper
// ============================================================================

/// Advances a 32-bit generation counter while skipping sentinel `0`.
fn nextGeneration(gen: u32): u32 {
    const next: u32 = gen +% 1;
    if (next == 0) return 1;
    return next;
}

// ============================================================================
// 3. Zero-Alloc Generational Slab Handle Pool
// ============================================================================

/// Zero-heap-allocation O(1) Generational Slab Pool storing `u64` payloads behind ABA-safe handles.
pub struct GenSlabPool {
    capacity: usize = 0;
    bump_cursor: usize = 0;
    free_stack: [GEN_SLAB_MAX_SLOTS]u16 = [GEN_SLAB_NO_FREE; GEN_SLAB_MAX_SLOTS];
    free_top: usize = 0;
    generations: [GEN_SLAB_MAX_SLOTS]u32 = [1; GEN_SLAB_MAX_SLOTS];
    occupied: [GEN_SLAB_MAX_SLOTS]bool = [false; GEN_SLAB_MAX_SLOTS];
    payloads: [GEN_SLAB_MAX_SLOTS]u64 = [0; GEN_SLAB_MAX_SLOTS];

    live_count: usize = 0;
    high_water_mark: usize = 0;
    total_allocs: u64 = 0;
    total_frees: u64 = 0;
    stale_handle_rejects: u64 = 0;

    /// Initializes a `GenSlabPool` with `capacity` slots (`1 <= capacity <= GEN_SLAB_MAX_SLOTS`).
    /// Returns `null` if `capacity` is 0 or exceeds `GEN_SLAB_MAX_SLOTS`.
    static fn init(capacity: usize): GenSlabPool? {
        if (capacity == 0 || capacity > GEN_SLAB_MAX_SLOTS) return null;
        return GenSlabPool{
            capacity: capacity,
            bump_cursor: 0,
            free_stack: [GEN_SLAB_NO_FREE; GEN_SLAB_MAX_SLOTS],
            free_top: 0,
            generations: [1; GEN_SLAB_MAX_SLOTS],
            occupied: [false; GEN_SLAB_MAX_SLOTS],
            payloads: [0; GEN_SLAB_MAX_SLOTS],
            live_count: 0,
            high_water_mark: 0,
            total_allocs: 0,
            total_frees: 0,
            stale_handle_rejects: 0,
        };
    }

    /// Allocates a slot in O(1) (preferring LIFO free-stack reuse, then bump cursor) and stores `payload`.
    /// Updates `high_water_mark` and returns `null` when the pool is full.
    fn alloc(*_, payload: u64): GenHandle? {
        let slot_idx: usize = 0;
        if (_.free_top > 0) {
            _.free_top -= 1;
            slot_idx = _.free_stack[_.free_top] as usize;
            _.free_stack[_.free_top] = GEN_SLAB_NO_FREE;
        } else if (_.bump_cursor < _.capacity) {
            slot_idx = _.bump_cursor;
            _.bump_cursor += 1;
        } else {
            return null;
        }

        if (_.generations[slot_idx] == 0) {
            _.generations[slot_idx] = 1;
        }
        _.occupied[slot_idx] = true;
        _.payloads[slot_idx] = payload;
        _.live_count += 1;
        if (_.live_count > _.high_water_mark) {
            _.high_water_mark = _.live_count;
        }
        _.total_allocs += 1;

        return GenHandle{
            index: @intCast(slot_idx),
            generation: _.generations[slot_idx],
        };
    }

    /// Returns `true` iff `handle` points to a currently occupied slot with an exact matching generation.
    fn is_alive(handle: GenHandle): bool {
        const idx: usize = handle.index as usize;
        if (idx >= _.capacity || handle.generation == 0) return false;
        return _.occupied[idx] && _.generations[idx] == handle.generation;
    }

    /// Retrieves the `u64` payload for `handle`, or `null` if stale/out-of-bounds/freed.
    fn get(handle: GenHandle): u64? {
        if (!_.is_alive(handle)) return null;
        return _.payloads[handle.index as usize];
    }

    /// Updates the `u64` payload for `handle`.
    /// Rejects stale/invalid handles cleanly, incrementing `stale_handle_rejects` and returning `false`.
    fn set(*_, handle: GenHandle, payload: u64): bool {
        if (!_.is_alive(handle)) {
            _.stale_handle_rejects += 1;
            return false;
        }
        _.payloads[handle.index as usize] = payload;
        return true;
    }

    /// Frees `handle` in O(1), advancing its slot generation (skipping 0), pushing the slot onto
    /// the free-stack for reuse, and returning the freed payload.
    /// Rejects stale/double-free/out-of-bounds handles cleanly (`stale_handle_rejects += 1`, returns `null`).
    fn free(*_, handle: GenHandle): u64? {
        if (!_.is_alive(handle)) {
            _.stale_handle_rejects += 1;
            return null;
        }

        const idx: usize = handle.index as usize;
        const old_payload: u64 = _.payloads[idx];

        _.occupied[idx] = false;
        _.payloads[idx] = 0;
        _.generations[idx] = nextGeneration(_.generations[idx]);

        _.free_stack[_.free_top] = handle.index;
        _.free_top += 1;
        _.live_count -= 1;
        _.total_frees += 1;

        return old_payload;
    }

    /// Releases all live slots and increments the generation of every occupied slot so all
    /// previously issued handles become immediately invalid while keeping full capacity reusable.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.bump_cursor) : (i += 1) {
            if (_.occupied[i]) {
                _.occupied[i] = false;
                _.payloads[i] = 0;
                _.generations[i] = nextGeneration(_.generations[i]);
                _.total_frees += 1;
            }
        }
        let f: usize = 0;
        while (f < _.free_top) : (f += 1) {
            _.free_stack[f] = GEN_SLAB_NO_FREE;
        }
        _.free_top = 0;
        _.bump_cursor = 0;
        _.live_count = 0;
    }

    /// Returns a cumulative telemetry & occupancy snapshot.
    fn stats(): GenSlabStats {
        return GenSlabStats{
            capacity: _.capacity,
            live_count: _.live_count,
            high_water_mark: _.high_water_mark,
            total_allocs: _.total_allocs,
            total_frees: _.total_frees,
            stale_handle_rejects: _.stale_handle_rejects,
        };
    }
}
