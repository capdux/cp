// Origin: Pure-CP Stdlib (Zero-Alloc Power-of-Two Bitmask Batch SPSC Ring & Hysteresis Backpressure Queue) | Created: 2026-11-23
// Pure CP zero-heap-allocation Single-Producer Single-Consumer (SPSC) bounded ring channel:
//   - Configurable power-of-two capacity (`4..=256`) using monotonic `u64` head/tail cursors with
//     `& mask` zero-modulo indexing, distinguishing full vs empty with zero wasted slots
//   - Single-item & batch zero-allocation operations: `tryPush`, `tryPop`, `peek`, `peekAt`,
//     `pushBatch`, `popBatch`, `skip`, and `pushOverwrite` (evicting & returning oldest item on full)
//   - Hysteresis high/low watermark backpressure controller (`isAboveHighWatermark`, `isBelowLowWatermark`,
//     `isBackpressured`, `fillPermille`) + full telemetry stats snapshot

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Minimum supported power-of-two ring capacity.
pub const SPSC_MIN_CAPACITY: usize = 4;

/// Maximum supported inline power-of-two ring capacity (zero dynamic heap allocation).
pub const SPSC_MAX_CAPACITY: usize = 256;

/// Default high watermark threshold in permille (800 = 80.0% full).
pub const SPSC_DEFAULT_HIGH_WATERMARK_PERMILLE: u16 = 800;

/// Default low watermark threshold in permille (250 = 25.0% full).
pub const SPSC_DEFAULT_LOW_WATERMARK_PERMILLE: u16 = 250;

/// Returns `true` if `cap` is a power of two within `SPSC_MIN_CAPACITY..=SPSC_MAX_CAPACITY`.
pub fn spscIsValidCapacity(cap: usize): bool {
    if (cap < SPSC_MIN_CAPACITY || cap > SPSC_MAX_CAPACITY) return false;
    return (cap & (cap - 1)) == 0;
}

/// Hysteresis high/low watermark configuration (in permille, `0..=1000`).
pub struct SpscWatermarkConfig {
    /// Fill level (permille, `1..=1000`) at or above which backpressure activates (`throttled`).
    high_permille: u16 = SPSC_DEFAULT_HIGH_WATERMARK_PERMILLE,
    /// Fill level (permille, `0..=high_permille`) at or below which backpressure clears (`normal`).
    low_permille: u16 = SPSC_DEFAULT_LOW_WATERMARK_PERMILLE,
}

/// Hysteresis backpressure state of the SPSC ring channel.
pub enum SpscBackpressureState {
    normal,
    throttled,
}

/// Telemetry snapshot for `SpscRingU64`.
pub struct SpscRingStats {
    capacity: usize = 0,
    len: usize = 0,
    available: usize = 0,
    fill_permille: u16 = 0,
    peak_len: usize = 0,
    backpressure: SpscBackpressureState = SpscBackpressureState.normal,
    total_pushed: u64 = 0,
    total_popped: u64 = 0,
    total_rejected_full: u64 = 0,
    total_overwritten: u64 = 0,
    backpressure_transitions: u64 = 0,
}

// ============================================================================
// 2. Core Zero-Alloc Power-of-Two SPSC Ring Channel
// ============================================================================

/// Zero-heap-allocation power-of-two bitmask SPSC ring channel with batch I/O & hysteresis backpressure.
pub struct SpscRingU64 {
    slots: [SPSC_MAX_CAPACITY]u64 = [0; SPSC_MAX_CAPACITY],
    cap: usize = 0,
    mask: u64 = 0,
    head: u64 = 0,
    tail: u64 = 0,
    high_permille: u16 = SPSC_DEFAULT_HIGH_WATERMARK_PERMILLE,
    low_permille: u16 = SPSC_DEFAULT_LOW_WATERMARK_PERMILLE,
    backpressure: SpscBackpressureState = SpscBackpressureState.normal,
    peak_len: usize = 0,
    total_pushed: u64 = 0,
    total_popped: u64 = 0,
    total_rejected_full: u64 = 0,
    total_overwritten: u64 = 0,
    backpressure_transitions: u64 = 0,

    /// Initializes a ring channel with power-of-two `cap_size` (`4..=256`) and default watermarks (800 / 250).
    /// Returns `null` if `cap_size` is not a valid power of two in range.
    pub fn init(cap_size: usize): SpscRingU64? {
        return SpscRingU64.initWithWatermarks(cap_size, SpscWatermarkConfig{});
    }

    /// Initializes a ring channel with custom hysteresis high/low watermarks (permille).
    /// Returns `null` if `cap_size` is invalid or watermark thresholds are out of range.
    pub static fn initWithWatermarks(cap_size: usize, wm: SpscWatermarkConfig): SpscRingU64? {
        if (!spscIsValidCapacity(cap_size)) return null;
        if (wm.high_permille == 0 || wm.high_permille > 1000) return null;
        if (wm.low_permille > wm.high_permille) return null;

        return SpscRingU64{
            cap: cap_size,
            mask: (cap_size - 1) as u64,
            high_permille: wm.high_permille,
            low_permille: wm.low_permille,
        };
    }

    // ---- Internal Helpers ----

    fn updateBackpressure(*_) {
        const fill = _.fillPermille();
        if (_.backpressure == SpscBackpressureState.normal) {
            if (fill >= _.high_permille) {
                _.backpressure = SpscBackpressureState.throttled;
                _.backpressure_transitions += 1;
            }
        } else {
            if (fill <= _.low_permille) {
                _.backpressure = SpscBackpressureState.normal;
                _.backpressure_transitions += 1;
            }
        }
    }

    // ---- Capacity & Watermark Queries ----

    /// Returns the configured power-of-two capacity.
    pub fn capacity(): usize {
        return _.cap;
    }

    /// Returns the current number of buffered elements (`0..=capacity`).
    pub fn len(): usize {
        return (_.tail -% _.head) as usize;
    }

    /// Returns the remaining free slots (`capacity - len`).
    pub fn available(): usize {
        return _.cap - _.len();
    }

    /// Returns `true` when the ring contains zero elements.
    pub fn isEmpty(): bool {
        return _.tail == _.head;
    }

    /// Returns `true` when all `capacity` slots are occupied.
    pub fn isFull(): bool {
        return _.len() == _.cap;
    }

    /// Returns current fill ratio in permille (`0..=1000`).
    pub fn fillPermille(): u16 {
        if (_.cap == 0) return 0;
        return ((_.len() * 1000) / _.cap) as u16;
    }

    /// Returns `true` if current fill ratio is at or above `high_permille`.
    pub fn isAboveHighWatermark(): bool {
        return _.fillPermille() >= _.high_permille;
    }

    /// Returns `true` if current fill ratio is at or below `low_permille`.
    pub fn isBelowLowWatermark(): bool {
        return _.fillPermille() <= _.low_permille;
    }

    /// Returns `true` when hysteresis backpressure is currently active (`throttled`).
    pub fn isBackpressured(): bool {
        return _.backpressure == SpscBackpressureState.throttled;
    }

    // ---- Single-Item Operations ----

    /// Attempts to push a single `item`. Returns `false` (and increments `total_rejected_full`) if full.
    pub fn tryPush(*_, item: u64): bool {
        const cur_len = _.len();
        if (cur_len >= _.cap) {
            _.total_rejected_full += 1;
            return false;
        }
        _.slots[(_.tail & _.mask) as usize] = item;
        _.tail +%= 1;
        _.total_pushed += 1;
        const new_len = cur_len + 1;
        if (new_len > _.peak_len) _.peak_len = new_len;
        _.updateBackpressure();
        return true;
    }

    /// Pushes `item` unconditionally: if full, overwrites and returns the oldest evicted element;
    /// otherwise pushes normally and returns `null`.
    pub fn pushOverwrite(*_, item: u64): u64? {
        const cur_len = _.len();
        if (cur_len >= _.cap) {
            const evicted = _.slots[(_.head & _.mask) as usize];
            _.head +%= 1;
            _.slots[(_.tail & _.mask) as usize] = item;
            _.tail +%= 1;
            _.total_pushed += 1;
            _.total_overwritten += 1;
            _.updateBackpressure();
            return evicted;
        }

        _.slots[(_.tail & _.mask) as usize] = item;
        _.tail +%= 1;
        _.total_pushed += 1;
        const new_len = cur_len + 1;
        if (new_len > _.peak_len) _.peak_len = new_len;
        _.updateBackpressure();
        return null;
    }

    /// Attempts to pop the oldest element. Returns `null` if empty.
    pub fn tryPop(*_): u64? {
        if (_.head == _.tail) return null;
        const item = _.slots[(_.head & _.mask) as usize];
        _.head +%= 1;
        _.total_popped += 1;
        _.updateBackpressure();
        return item;
    }

    /// Peeks at the oldest element without removing it, or `null` if empty.
    pub fn peek(): u64? {
        if (_.head == _.tail) return null;
        return _.slots[(_.head & _.mask) as usize];
    }

    /// Peeks at the element at logical `offset` (`0` = oldest, `len - 1` = newest), or `null` if OOB.
    pub fn peekAt(offset: usize): u64? {
        if (offset >= _.len()) return null;
        return _.slots[((_.head +% (offset as u64)) & _.mask) as usize];
    }

    // ---- Batch Operations ----

    /// Pushes up to `min(items.len, available())` elements in order.
    /// Any items that could not fit are counted toward `total_rejected_full`.
    /// Returns the number of elements actually written.
    pub fn pushBatch(*_, items: []const u64): usize {
        if (items.len == 0) return 0;
        const free = _.available();
        const to_write = if (items.len < free) items.len else free;

        if (items.len > to_write) {
            _.total_rejected_full += (items.len - to_write) as u64;
        }
        if (to_write == 0) return 0;

        let i: usize = 0;
        let t: u64 = _.tail;
        while (i < to_write) : (i += 1) {
            _.slots[(t & _.mask) as usize] = items[i];
            t +%= 1;
        }
        _.tail = t;
        _.total_pushed += to_write as u64;
        const new_len = _.len();
        if (new_len > _.peak_len) _.peak_len = new_len;
        _.updateBackpressure();
        return to_write;
    }

    /// Pops up to `min(len(), out_items.len)` elements into `out_items`.
    /// Returns the number of elements actually popped.
    pub fn popBatch(*_, out_items: *[]u64): usize {
        if (out_items.len == 0) return 0;
        const avail = _.len();
        const to_read = if (out_items.len < avail) out_items.len else avail;
        if (to_read == 0) return 0;

        let i: usize = 0;
        let h: u64 = _.head;
        while (i < to_read) : (i += 1) {
            out_items[i] = _.slots[(h & _.mask) as usize];
            h +%= 1;
        }
        _.head = h;
        _.total_popped += to_read as u64;
        _.updateBackpressure();
        return to_read;
    }

    /// Advances the consumer cursor by up to `count` buffered elements without copying.
    /// Returns the number of elements skipped.
    pub fn skip(*_, count: usize): usize {
        const avail = _.len();
        const to_skip = if (count < avail) count else avail;
        if (to_skip == 0) return 0;
        _.head +%= to_skip as u64;
        _.total_popped += to_skip as u64;
        _.updateBackpressure();
        return to_skip;
    }

    /// Drops all buffered elements while preserving cumulative telemetry counters.
    pub fn clear(*_) {
        _.head = _.tail;
        _.updateBackpressure();
    }

    /// Returns a complete telemetry & backpressure snapshot.
    pub fn stats(): SpscRingStats {
        const cur_len = _.len();
        return SpscRingStats{
            capacity: _.cap,
            len: cur_len,
            available: _.cap - cur_len,
            fill_permille: _.fillPermille(),
            peak_len: _.peak_len,
            backpressure: _.backpressure,
            total_pushed: _.total_pushed,
            total_popped: _.total_popped,
            total_rejected_full: _.total_rejected_full,
            total_overwritten: _.total_overwritten,
            backpressure_transitions: _.backpressure_transitions,
        };
    }
}
