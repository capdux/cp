// Origin: Pure-CP Stdlib (Zero-Alloc Caffeine/Ristretto W-TinyLFU Admission & SLRU Cache Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation W-TinyLFU (Window-TinyLFU) cache admission & segmented LRU (SLRU) engine:
//   1. 4-bit Count-Min Sketch (`TinyLfuSketch`):
//      4 independent hash rows x 256 4-bit saturating counters (0..15) packed into 4 x 16 u64 words
//      (512 bytes total), with SWAR parallel right-shift halving (`(w >> 1) & 0x7777777777777777`)
//      for periodic aging against stale hotspots.
//   2. Three-Segment SLRU Layout (`Window` + `Probation` + `Protected`):
//      New keys enter the small Window LRU; upon Window overflow, the evicted candidate duels the
//      Main Probation LRU victim via `sketch.estimate(candidate) >= sketch.estimate(victim)` to
//      prevent one-hit-wonder scan pollution. Second hits in Probation promote to Protected;
//      Protected overflow demotes to Probation.

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

pub const TINYLFU_SKETCH_ROWS: usize = 4;
pub const TINYLFU_SKETCH_WORDS_PER_ROW: usize = 16;
pub const TINYLFU_SKETCH_WIDTH: usize = 256;
pub const TINYLFU_MAX_COUNT: u8 = 15;
pub const TINYLFU_MAX_CAPACITY: usize = 256;
pub const TINYLFU_NO_NODE: u16 = 0xFFFF;

fn mixRowHash(key_hash: u64, row: usize): usize {
    const seeds: [4]u64 = [
        0x9E3779B97F4A7C15,
        0xBF58476D1CE4E5B9,
        0x94D049BB133111EB,
        0xD6E8FEB86659FD93,
    ];
    let z: u64 = (key_hash ^ seeds[row]) *% seeds[(row + 1) & 3];
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = z ^ (z >> 27);
    return (z as usize) & (TINYLFU_SKETCH_WIDTH - 1);
}

// ============================================================================
// 1. 4-bit Count-Min Sketch with SWAR Parallel Halving
// ============================================================================

/// Zero-heap-allocation 4-row x 256-column 4-bit Count-Min Sketch (`0..=15` saturating counters)
/// with periodic SWAR right-shift halving for temporal decay.
pub struct TinyLfuSketch {
    rows: [TINYLFU_SKETCH_ROWS][TINYLFU_SKETCH_WORDS_PER_ROW]u64 = [[0; TINYLFU_SKETCH_WORDS_PER_ROW]; TINYLFU_SKETCH_ROWS];
    sample_size: u64 = 0;
    reset_threshold: u64 = 1024;
    halve_count: u64 = 0;

    /// Initializes a 4-bit Count-Min Sketch that halves all counters every `reset_threshold` increments.
    /// Returns `null` if `reset_threshold == 0`.
    static fn init(reset_threshold: u64): TinyLfuSketch? {
        if (reset_threshold == 0) {
            return null;
        }
        return TinyLfuSketch {
            reset_threshold: reset_threshold,
        };
    }

    fn readCounter(row: usize, col: usize): u8 {
        const word_i: usize = col >> 4;
        const shift: u6 = ((col & 15) * 4) as u6;
        return ((_.rows[row][word_i] >> shift) & 0xF) as u8;
    }

    fn incrementCounter(*_, row: usize, col: usize) {
        const word_i: usize = col >> 4;
        const shift: u6 = ((col & 15) * 4) as u6;
        const cur: u64 = (_.rows[row][word_i] >> shift) & 0xF;
        if (cur < 15) {
            _.rows[row][word_i] += (1 as u64) << shift;
        }
    }

    /// Halves every 4-bit counter across all 4 rows in parallel using SWAR bitmasking
    /// (`(w >> 1) & 0x7777777777777777`) and halves `sample_size`.
    fn resetHalve(*_) {
        let r: usize = 0;
        while (r < TINYLFU_SKETCH_ROWS) : (r += 1) {
            let w: usize = 0;
            while (w < TINYLFU_SKETCH_WORDS_PER_ROW) : (w += 1) {
                _.rows[r][w] = (_.rows[r][w] >> 1) & 0x7777777777777777;
            }
        }
        _.sample_size = _.sample_size >> 1;
        _.halve_count += 1;
    }

    /// Records an access for `key_hash` across all 4 rows (saturating at 15) and triggers
    /// automatic halving once `sample_size` reaches `reset_threshold`.
    fn increment(*_, key_hash: u64) {
        let r: usize = 0;
        while (r < TINYLFU_SKETCH_ROWS) : (r += 1) {
            const col: usize = mixRowHash(key_hash, r);
            _.incrementCounter(r, col);
        }
        _.sample_size += 1;
        if (_.sample_size >= _.reset_threshold) {
            _.resetHalve();
        }
    }

    /// Returns the Count-Min frequency estimate (`0..=15`) for `key_hash`.
    fn estimate(key_hash: u64): u8 {
        let min_val: u8 = TINYLFU_MAX_COUNT;
        let r: usize = 0;
        while (r < TINYLFU_SKETCH_ROWS) : (r += 1) {
            const col: usize = mixRowHash(key_hash, r);
            const c: u8 = _.readCounter(r, col);
            if (c < min_val) {
                min_val = c;
            }
        }
        return min_val;
    }

    /// Clears all counters and resets sampling state.
    fn clear(*_) {
        let r: usize = 0;
        while (r < TINYLFU_SKETCH_ROWS) : (r += 1) {
            let w: usize = 0;
            while (w < TINYLFU_SKETCH_WORDS_PER_ROW) : (w += 1) {
                _.rows[r][w] = 0;
            }
        }
        _.sample_size = 0;
        _.halve_count = 0;
    }
}

// ============================================================================
// 2. W-TinyLFU Cache Segment Models & Configuration
// ============================================================================

/// Segment residence state of a cache entry.
pub enum TinyLfuSegment {
    free,
    window,
    probation,
    protected,
}

/// Outcome returned by `WTinyLfuCache.put`.
pub enum TinyLfuPutOutcome {
    updated_existing,
    inserted_window,
    admitted_main,
    rejected_by_sketch,
}

/// Capacity and frequency-sketch configuration for `WTinyLfuCache`.
pub struct TinyLfuConfig {
    window_cap: usize = 4;
    probation_cap: usize = 12;
    protected_cap: usize = 48;
    sketch_reset_threshold: u64 = 640;

    fn defaults(): TinyLfuConfig {
        return TinyLfuConfig {};
    }

    fn totalCapacity(): usize {
        return _.window_cap + _.probation_cap + _.protected_cap;
    }

    /// Validates `window_cap >= 1`, `probation_cap >= 1`, `totalCapacity() <= 256`,
    /// and `sketch_reset_threshold > 0`.
    fn isValid(): bool {
        if (_.window_cap == 0 || _.probation_cap == 0) {
            return false;
        }
        const total: usize = _.totalCapacity();
        if (total == 0 || total > TINYLFU_MAX_CAPACITY) {
            return false;
        }
        if (_.sketch_reset_threshold == 0) {
            return false;
        }
        return true;
    }
}

/// Intrusive doubly-linked slot node stored inline inside `WTinyLfuCache`.
pub struct TinyLfuNode {
    key_hash: u64 = 0;
    value: u64 = 0;
    segment: TinyLfuSegment = TinyLfuSegment.free;
    prev: u16 = TINYLFU_NO_NODE;
    next: u16 = TINYLFU_NO_NODE;
}

struct TinyLfuList {
    head: u16 = TINYLFU_NO_NODE;
    tail: u16 = TINYLFU_NO_NODE;
    len: usize = 0;
}

/// Aggregate occupancy, hit-rate, admission/rejection, and SLRU promotion statistics.
pub struct TinyLfuStats {
    len: usize = 0;
    capacity: usize = 0;
    window_len: usize = 0;
    probation_len: usize = 0;
    protected_len: usize = 0;
    hits: u64 = 0;
    misses: u64 = 0;
    admitted_count: u64 = 0;
    rejected_count: u64 = 0;
    evicted_count: u64 = 0;
    promotions: u64 = 0;
    demotions: u64 = 0;
    sketch_halves: u64 = 0;
    hit_rate_bps: u32 = 0;
}

// ============================================================================
// 3. Zero-Alloc W-TinyLFU Cache Engine (Window + Probation + Protected)
// ============================================================================

/// Zero-heap-allocation W-TinyLFU cache backed by `TINYLFU_MAX_CAPACITY` (256) inline slots,
/// three O(1) intrusive LRU segments, and a 4-bit Count-Min Sketch admission filter.
pub struct WTinyLfuCache {
    nodes: [TINYLFU_MAX_CAPACITY]TinyLfuNode = [TinyLfuNode {}; TINYLFU_MAX_CAPACITY];
    cfg: TinyLfuConfig = TinyLfuConfig {};
    sketch: TinyLfuSketch = TinyLfuSketch {};
    window_list: TinyLfuList = TinyLfuList {};
    probation_list: TinyLfuList = TinyLfuList {};
    protected_list: TinyLfuList = TinyLfuList {};
    total_len: usize = 0;
    hits: u64 = 0;
    misses: u64 = 0;
    admitted_count: u64 = 0;
    rejected_count: u64 = 0;
    evicted_count: u64 = 0;
    promotions: u64 = 0;
    demotions: u64 = 0;

    /// Initializes a W-TinyLFU cache with validated segment capacities and sketch decay threshold.
    fn init(cfg: TinyLfuConfig): WTinyLfuCache? {
        if (!cfg.isValid()) {
            return null;
        }
        const sk = TinyLfuSketch.init(cfg.sketch_reset_threshold) ?? return null;
        return WTinyLfuCache {
            cfg: cfg,
            sketch: sk,
        };
    }

    fn findSlot(key_hash: u64): usize? {
        const cap: usize = _.cfg.totalCapacity();
        let i: usize = 0;
        while (i < cap) : (i += 1) {
            if (_.nodes[i].segment != TinyLfuSegment.free && _.nodes[i].key_hash == key_hash) {
                return i;
            }
        }
        return null;
    }

    fn allocFreeSlot(): usize? {
        const cap: usize = _.cfg.totalCapacity();
        let i: usize = 0;
        while (i < cap) : (i += 1) {
            if (_.nodes[i].segment == TinyLfuSegment.free) {
                return i;
            }
        }
        return null;
    }

    fn listPushFront(*_, seg: TinyLfuSegment, idx: u16) {
        let list = if (seg == TinyLfuSegment.window) {
            *_.window_list
        } else if (seg == TinyLfuSegment.probation) {
            *_.probation_list
        } else {
            *_.protected_list
        };

        _.nodes[idx as usize].segment = seg;
        _.nodes[idx as usize].prev = TINYLFU_NO_NODE;
        _.nodes[idx as usize].next = list.head;

        if (list.head != TINYLFU_NO_NODE) {
            _.nodes[list.head as usize].prev = idx;
        } else {
            list.tail = idx;
        }
        list.head = idx;
        list.len += 1;
    }

    fn listUnlink(*_, seg: TinyLfuSegment, idx: u16) {
        let list = if (seg == TinyLfuSegment.window) {
            *_.window_list
        } else if (seg == TinyLfuSegment.probation) {
            *_.probation_list
        } else {
            *_.protected_list
        };

        const p: u16 = _.nodes[idx as usize].prev;
        const n: u16 = _.nodes[idx as usize].next;

        if (p != TINYLFU_NO_NODE) {
            _.nodes[p as usize].next = n;
        } else {
            list.head = n;
        }
        if (n != TINYLFU_NO_NODE) {
            _.nodes[n as usize].prev = p;
        } else {
            list.tail = p;
        }

        _.nodes[idx as usize].prev = TINYLFU_NO_NODE;
        _.nodes[idx as usize].next = TINYLFU_NO_NODE;
        if (list.len > 0) {
            list.len -= 1;
        }
    }

    fn listPopBack(*_, seg: TinyLfuSegment): u16 {
        const tail: u16 = if (seg == TinyLfuSegment.window) {
            _.window_list.tail
        } else if (seg == TinyLfuSegment.probation) {
            _.probation_list.tail
        } else {
            _.protected_list.tail
        };
        if (tail == TINYLFU_NO_NODE) {
            return TINYLFU_NO_NODE;
        }
        _.listUnlink(seg, tail);
        return tail;
    }

    fn touchExisting(*_, idx: u16) {
        const seg = _.nodes[idx as usize].segment;
        if (seg == TinyLfuSegment.window) {
            _.listUnlink(TinyLfuSegment.window, idx);
            _.listPushFront(TinyLfuSegment.window, idx);
        } else if (seg == TinyLfuSegment.probation) {
            _.listUnlink(TinyLfuSegment.probation, idx);
            if (_.cfg.protected_cap > 0) {
                _.listPushFront(TinyLfuSegment.protected, idx);
                _.promotions += 1;
                if (_.protected_list.len > _.cfg.protected_cap) {
                    const demoted: u16 = _.listPopBack(TinyLfuSegment.protected);
                    if (demoted != TINYLFU_NO_NODE) {
                        _.listPushFront(TinyLfuSegment.probation, demoted);
                        _.demotions += 1;
                    }
                }
            } else {
                _.listPushFront(TinyLfuSegment.probation, idx);
            }
        } else if (seg == TinyLfuSegment.protected) {
            _.listUnlink(TinyLfuSegment.protected, idx);
            _.listPushFront(TinyLfuSegment.protected, idx);
        }
    }

    fn releaseSlot(*_, idx: u16) {
        _.nodes[idx as usize] = TinyLfuNode {};
        if (_.total_len > 0) {
            _.total_len -= 1;
        }
    }

    /// Looks up `key_hash`, records frequency in the 4-bit sketch, and updates SLRU recency/promotion.
    fn get(*_, key_hash: u64): u64? {
        _.sketch.increment(key_hash);
        if (idx := _.findSlot(key_hash)) {
            _.hits += 1;
            _.touchExisting(idx as u16);
            return _.nodes[idx].value;
        }
        _.misses += 1;
        return null;
    }

    /// Inserts or updates `(key_hash, value_u64)` using W-TinyLFU window admission and SLRU eviction.
    fn put(*_, key_hash: u64, value_u64: u64): TinyLfuPutOutcome {
        _.sketch.increment(key_hash);

        // 1. Existing key: update value in-place and promote/refresh recency.
        if (existing := _.findSlot(key_hash)) {
            _.nodes[existing].value = value_u64;
            _.touchExisting(existing as u16);
            return TinyLfuPutOutcome.updated_existing;
        }

        // 2. Window has room: insert directly into Window MRU head.
        if (_.window_list.len < _.cfg.window_cap) {
            if (free_idx := _.allocFreeSlot()) {
                _.nodes[free_idx] = TinyLfuNode {
                    key_hash: key_hash,
                    value: value_u64,
                    segment: TinyLfuSegment.window,
                };
                _.total_len += 1;
                _.listPushFront(TinyLfuSegment.window, free_idx as u16);
                return TinyLfuPutOutcome.inserted_window;
            }
        }

        // 3. Window is full: pop oldest Window entry as candidate for Main Probation.
        const cand_idx: u16 = _.listPopBack(TinyLfuSegment.window);

        // Case 3a: Total cache is not yet at full capacity -> admit candidate directly to Probation
        // and allocate a free slot for the newly arriving key in Window.
        if (_.total_len < _.cfg.totalCapacity()) {
            if (free_idx := _.allocFreeSlot()) {
                _.listPushFront(TinyLfuSegment.probation, cand_idx);
                _.admitted_count += 1;

                _.nodes[free_idx] = TinyLfuNode {
                    key_hash: key_hash,
                    value: value_u64,
                    segment: TinyLfuSegment.window,
                };
                _.total_len += 1;
                _.listPushFront(TinyLfuSegment.window, free_idx as u16);
                return TinyLfuPutOutcome.admitted_main;
            }
        }

        // Case 3b: Cache is full -> W-TinyLFU frequency duel between Window candidate and Main victim.
        const victim_seg: TinyLfuSegment = if (_.probation_list.len > 0) {
            TinyLfuSegment.probation
        } else {
            TinyLfuSegment.protected
        };
        const victim_idx: u16 = if (victim_seg == TinyLfuSegment.probation) {
            _.probation_list.tail
        } else {
            _.protected_list.tail
        };

        const cand_freq: u8 = _.sketch.estimate(_.nodes[cand_idx as usize].key_hash);
        const victim_freq: u8 = if (victim_idx != TINYLFU_NO_NODE) {
            _.sketch.estimate(_.nodes[victim_idx as usize].key_hash)
        } else {
            0
        };

        if (victim_idx != TINYLFU_NO_NODE && cand_freq >= victim_freq) {
            // Candidate wins: evict victim, admit candidate to Probation, reuse victim slot for new key.
            _.listUnlink(victim_seg, victim_idx);
            _.evicted_count += 1;
            _.admitted_count += 1;

            _.listPushFront(TinyLfuSegment.probation, cand_idx);

            _.nodes[victim_idx as usize] = TinyLfuNode {
                key_hash: key_hash,
                value: value_u64,
                segment: TinyLfuSegment.window,
            };
            _.listPushFront(TinyLfuSegment.window, victim_idx);
            return TinyLfuPutOutcome.admitted_main;
        }

        // Candidate loses: reject candidate from Main (anti-scan pollution) and reuse its slot for new key.
        _.evicted_count += 1;
        _.rejected_count += 1;
        _.nodes[cand_idx as usize] = TinyLfuNode {
            key_hash: key_hash,
            value: value_u64,
            segment: TinyLfuSegment.window,
        };
        _.listPushFront(TinyLfuSegment.window, cand_idx);
        return TinyLfuPutOutcome.rejected_by_sketch;
    }

    /// Removes `key_hash` from whichever segment holds it. Returns `false` if absent.
    fn remove(*_, key_hash: u64): bool {
        const idx = _.findSlot(key_hash) ?? return false;
        const seg = _.nodes[idx].segment;
        _.listUnlink(seg, idx as u16);
        _.releaseSlot(idx as u16);
        return true;
    }

    /// Side-effect-free membership check (does not alter sketch or LRU ordering).
    fn contains(key_hash: u64): bool {
        return _.findSlot(key_hash) != null;
    }

    /// Returns the current segment (`window`, `probation`, `protected`, or `free` if absent).
    fn segmentOf(key_hash: u64): TinyLfuSegment {
        if (idx := _.findSlot(key_hash)) {
            return _.nodes[idx].segment;
        }
        return TinyLfuSegment.free;
    }

    /// Returns the current 4-bit Count-Min Sketch frequency estimate (`0..=15`) for `key_hash`.
    fn estimateFreq(key_hash: u64): u8 {
        return _.sketch.estimate(key_hash);
    }

    fn len(): usize {
        return _.total_len;
    }

    fn capacity(): usize {
        return _.cfg.totalCapacity();
    }

    /// Returns detailed occupancy, hit rate (`hit_rate_bps`), and W-TinyLFU admission telemetry.
    fn stats(): TinyLfuStats {
        const total_gets: u64 = _.hits + _.misses;
        const rate_bps: u32 = if (total_gets == 0) {
            0
        } else {
            ((_.hits * 10000) / total_gets) as u32
        };
        return TinyLfuStats {
            len: _.total_len,
            capacity: _.cfg.totalCapacity(),
            window_len: _.window_list.len,
            probation_len: _.probation_list.len,
            protected_len: _.protected_list.len,
            hits: _.hits,
            misses: _.misses,
            admitted_count: _.admitted_count,
            rejected_count: _.rejected_count,
            evicted_count: _.evicted_count,
            promotions: _.promotions,
            demotions: _.demotions,
            sketch_halves: _.sketch.halve_count,
            hit_rate_bps: rate_bps,
        };
    }

    /// Clears all cached entries, segment lists, counters, and frequency sketch.
    fn clear(*_) {
        const cap: usize = _.cfg.totalCapacity();
        let i: usize = 0;
        while (i < cap) : (i += 1) {
            _.nodes[i] = TinyLfuNode {};
        }
        _.window_list = TinyLfuList {};
        _.probation_list = TinyLfuList {};
        _.protected_list = TinyLfuList {};
        _.total_len = 0;
        _.hits = 0;
        _.misses = 0;
        _.admitted_count = 0;
        _.rejected_count = 0;
        _.evicted_count = 0;
        _.promotions = 0;
        _.demotions = 0;
        _.sketch.clear();
    }
}
