// Origin: Pure-CP Stdlib (Zero-Alloc CLOCK Second-Chance Database Page Buffer Pool Manager) | Created: 2026-11-22
// Pure CP zero-heap-allocation CLOCK (Second-Chance) Page Buffer Pool for Database Storage Engines:
//   - Fixed inline frame array (`BUFFER_POOL_MAX_FRAMES = 128`), 0 dynamic heap allocation
//   - Pin reference counting with underflow protection & guaranteed protection of in-flight pinned pages
//   - CLOCK second-chance eviction: skips pinned frames, clears `ref_bit` on first pass, evicts cold frame
//   - Dirty-page writeback reporting (`evicted_was_dirty`, `evicted_page_id`) & batched `flush_dirty_pages`
//   - Safe `invalidate_page` (rejects pinned pages) & real-time `BufferPoolStats` telemetry

/// Maximum frame slots supported inline by `ClockPageBufferPool` (zero dynamic heap).
pub const BUFFER_POOL_MAX_FRAMES: usize = 128;

/// Sentinel invalid page identifier rejected by `fetch_page` / `unpin_page` / `invalidate_page`.
pub const BUFFER_POOL_INVALID_PAGE: u64 = 0xFFFFFFFFFFFFFFFF;

// ============================================================================
// 1. Public Metadata, Fetch Outcome & Telemetry Models
// ============================================================================

/// Metadata tracked per resident buffer pool frame.
pub struct PageFrameMeta {
    page_id: u64 = BUFFER_POOL_INVALID_PAGE;
    pin_count: u32 = 0;
    dirty: bool = false;
    ref_bit: bool = false;
    occupied: bool = false;
}

/// Outcome returned by `ClockPageBufferPool.fetch_page`.
pub struct PageFetchOutcome {
    ok: bool = false;
    frame_idx: usize = 0;
    cache_hit: bool = false;
    evicted: bool = false;
    evicted_page_id: u64 = BUFFER_POOL_INVALID_PAGE;
    evicted_was_dirty: bool = false;
    all_pinned: bool = false;
}

/// Real-time occupancy and cumulative hit/eviction telemetry for `ClockPageBufferPool`.
pub struct BufferPoolStats {
    capacity: usize = 0;
    occupied_frames: usize = 0;
    pinned_frames: usize = 0;
    dirty_frames: usize = 0;
    cache_hits: u64 = 0;
    cache_misses: u64 = 0;
    evictions: u64 = 0;
    dirty_evictions: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc CLOCK Second-Chance Page Buffer Pool
// ============================================================================

/// Fixed-capacity (up to 128 frames) zero-heap-allocation CLOCK page buffer pool manager.
pub struct ClockPageBufferPool {
    capacity: usize = 0;
    clock_hand: usize = 0;
    frames: [BUFFER_POOL_MAX_FRAMES]PageFrameMeta = [PageFrameMeta{}; BUFFER_POOL_MAX_FRAMES],
    cache_hits: u64 = 0;
    cache_misses: u64 = 0;
    evictions: u64 = 0;
    dirty_evictions: u64 = 0;

    /// Initializes a `ClockPageBufferPool` with `capacity` frames (`1 <= capacity <= BUFFER_POOL_MAX_FRAMES`).
    /// Returns `null` if `capacity` is 0 or exceeds `BUFFER_POOL_MAX_FRAMES`.
    static fn init(capacity: usize): ClockPageBufferPool? {
        if (capacity == 0 || capacity > BUFFER_POOL_MAX_FRAMES) {
            return null;
        }
        return ClockPageBufferPool{
            capacity: capacity,
            clock_hand: 0,
        };
    }

    /// Fetches and pins `page_id` in the buffer pool:
    ///   - Cache hit: increments `pin_count`, sets `ref_bit = true`, increments `cache_hits`.
    ///   - Cache miss with free frame: allocates free frame with `pin_count = 1, ref_bit = true, dirty = false`.
    ///   - Full pool: advances `clock_hand` up to 2 full rotations, skipping `pin_count > 0` frames,
    ///     clearing `ref_bit` on first pass (second chance), and evicting the first unpinned cold frame.
    ///   - If every frame is currently pinned, returns `ok = false, all_pinned = true` without mutating any frame.
    fn fetch_page(*_, page_id: u64): PageFetchOutcome {
        if (page_id == BUFFER_POOL_INVALID_PAGE) {
            return PageFetchOutcome{};
        }

        let found_free: bool = false;
        let free_idx: usize = 0;
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.frames[i].occupied) {
                if (_.frames[i].page_id == page_id) {
                    if (_.frames[i].pin_count < 0xFFFFFFFF) {
                        _.frames[i].pin_count += 1;
                    }
                    _.frames[i].ref_bit = true;
                    _.cache_hits += 1;
                    return PageFetchOutcome{
                        ok: true,
                        frame_idx: i,
                        cache_hit: true,
                        evicted: false,
                        evicted_page_id: BUFFER_POOL_INVALID_PAGE,
                        evicted_was_dirty: false,
                        all_pinned: false,
                    };
                }
            } else if (!found_free) {
                found_free = true;
                free_idx = i;
            }
        }

        if (found_free) {
            _.cache_misses += 1;
            _.frames[free_idx] = PageFrameMeta{
                page_id: page_id,
                pin_count: 1,
                dirty: false,
                ref_bit: true,
                occupied: true,
            };
            return PageFetchOutcome{
                ok: true,
                frame_idx: free_idx,
                cache_hit: false,
                evicted: false,
                evicted_page_id: BUFFER_POOL_INVALID_PAGE,
                evicted_was_dirty: false,
                all_pinned: false,
            };
        }

        // Pool is full: CLOCK second-chance scan (at most 2 * capacity steps)
        const max_steps: usize = _.capacity * 2;
        let steps: usize = 0;
        while (steps < max_steps) : (steps += 1) {
            const idx: usize = _.clock_hand;
            _.clock_hand = (_.clock_hand + 1) % _.capacity;

            if (_.frames[idx].pin_count > 0) {
                continue;
            }
            if (_.frames[idx].ref_bit) {
                _.frames[idx].ref_bit = false;
                continue;
            }

            // Found cold unpinned victim frame
            const victim_page: u64 = _.frames[idx].page_id;
            const victim_dirty: bool = _.frames[idx].dirty;
            _.cache_misses += 1;
            _.evictions += 1;
            if (victim_dirty) {
                _.dirty_evictions += 1;
            }

            _.frames[idx] = PageFrameMeta{
                page_id: page_id,
                pin_count: 1,
                dirty: false,
                ref_bit: true,
                occupied: true,
            };
            return PageFetchOutcome{
                ok: true,
                frame_idx: idx,
                cache_hit: false,
                evicted: true,
                evicted_page_id: victim_page,
                evicted_was_dirty: victim_dirty,
                all_pinned: false,
            };
        }

        // Every frame is pinned -> refuse eviction safely
        _.cache_misses += 1;
        return PageFetchOutcome{
            ok: false,
            frame_idx: 0,
            cache_hit: false,
            evicted: false,
            evicted_page_id: BUFFER_POOL_INVALID_PAGE,
            evicted_was_dirty: false,
            all_pinned: true,
        };
    }

    /// Decrements the pin count of `page_id` (guarding against underflow) and optionally marks it dirty.
    /// Returns `false` if `page_id` is invalid, not resident, or already unpinned (`pin_count == 0`).
    fn unpin_page(*_, page_id: u64, mark_dirty: bool): bool {
        if (page_id == BUFFER_POOL_INVALID_PAGE) return false;
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.frames[i].occupied && _.frames[i].page_id == page_id) {
                if (_.frames[i].pin_count == 0) {
                    return false;
                }
                _.frames[i].pin_count -= 1;
                if (mark_dirty) {
                    _.frames[i].dirty = true;
                }
                return true;
            }
        }
        return false;
    }

    /// Collects up to `out_dirty_page_ids.len` dirty resident page IDs and clears their `dirty` bit.
    /// Remaining dirty pages stay marked dirty so callers can safely flush in small batches without losing pages.
    fn flush_dirty_pages(*_, out_dirty_page_ids: *[]u64): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.capacity && written < out_dirty_page_ids.len) : (i += 1) {
            if (_.frames[i].occupied && _.frames[i].dirty) {
                out_dirty_page_ids[written] = _.frames[i].page_id;
                _.frames[i].dirty = false;
                written += 1;
            }
        }
        return written;
    }

    /// Invalidates and frees the frame holding `page_id` iff its `pin_count == 0`.
    /// Returns `false` if `page_id` is invalid, not resident, or still pinned.
    fn invalidate_page(*_, page_id: u64): bool {
        if (page_id == BUFFER_POOL_INVALID_PAGE) return false;
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.frames[i].occupied && _.frames[i].page_id == page_id) {
                if (_.frames[i].pin_count > 0) {
                    return false;
                }
                _.frames[i] = PageFrameMeta{};
                return true;
            }
        }
        return false;
    }

    /// Returns a live occupancy and cumulative hit/eviction telemetry snapshot.
    fn stats(): BufferPoolStats {
        let occ: usize = 0;
        let pinned: usize = 0;
        let dirty: usize = 0;
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.frames[i].occupied) {
                occ += 1;
                if (_.frames[i].pin_count > 0) pinned += 1;
                if (_.frames[i].dirty) dirty += 1;
            }
        }
        return BufferPoolStats{
            capacity: _.capacity,
            occupied_frames: occ,
            pinned_frames: pinned,
            dirty_frames: dirty,
            cache_hits: _.cache_hits,
            cache_misses: _.cache_misses,
            evictions: _.evictions,
            dirty_evictions: _.dirty_evictions,
        };
    }
}
