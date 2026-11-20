// Source: cp/lib/ringbuf/ring.cp (Bounded RingBuffer, ByteRing Stream Pipe & Dual-Trigger MicroBatcher)
// Pure CP zero-C-dependency bounded ring buffer, wrap-around byte stream pipe,
// and count/time dual-trigger micro-batch aggregator.

const std = @import("std");

/// Error set for ring buffer, byte ring, and micro-batcher operations.
pub err RingError {
    BufferFull,
    InvalidCapacity,
    InvalidConfig,
    OutOfMemory,
}

/// Overflow handling policy for bounded RingBuffer<T>.
pub enum OverflowPolicy {
    Reject,
    OverwriteOldest,
}

/// Generic bounded circular buffer with configurable overflow policy.
pub fn RingBuffer(comptime T: type): type {
    return struct {
        const Self = @This();

        alloc: std.mem.Allocator,
        buf: []T = [],
        cap: usize = 0,
        head: usize = 0,
        count: usize = 0,
        policy: OverflowPolicy = .Reject,
        overwritten_count: u64 = 0,

        /// Initialize a bounded ring buffer with exact capacity and overflow policy.
        pub fn init(alloc: std.mem.Allocator, cap_size: usize, policy: OverflowPolicy): Self ! RingError {
            if (cap_size == 0) return RingError.InvalidCapacity;
            const buf = alloc.alloc(T, cap_size) catch return RingError.OutOfMemory;
            return Self{
                alloc: alloc,
                buf: buf,
                cap: cap_size,
                head: 0,
                count: 0,
                policy: policy,
                overwritten_count: 0,
            };
        }

        /// Release backing storage.
        pub fn deinit(*_) {
            if (_.buf.len > 0) {
                _.alloc.free(_.buf);
                _.buf = [];
            }
            _.cap = 0;
            _.head = 0;
            _.count = 0;
        }

        /// Push an item into the ring buffer according to its overflow policy.
        pub fn push(*_, item: T): void ! RingError {
            if (_.cap == 0) return RingError.InvalidCapacity;
            if (_.count == _.cap) {
                if (_.policy == .Reject) {
                    return RingError.BufferFull;
                }
                _.buf[_.head] = item;
                _.head = (_.head + 1) % _.cap;
                _.overwritten_count += 1;
            } else {
                const tail = (_.head + _.count) % _.cap;
                _.buf[tail] = item;
                _.count += 1;
            }
        }

        /// Pop the oldest item from the ring buffer, or null if empty.
        pub fn pop(*_): T? {
            if (_.count == 0) return null;
            const val = _.buf[_.head];
            _.head = (_.head + 1) % _.cap;
            _.count -= 1;
            return val;
        }

        /// Peek at the oldest item without removing it.
        pub fn peek(): T? {
            if (_.count == 0) return null;
            return _.buf[_.head];
        }

        /// Peek at an item by logical index (0 = oldest, len()-1 = newest).
        pub fn peekAt(logical_idx: usize): T? {
            if (logical_idx >= _.count) return null;
            const phys_idx = (_.head + logical_idx) % _.cap;
            return _.buf[phys_idx];
        }

        /// Pop up to out_buf.len items into out_buf, handling wrap-around.
        pub fn popBatch(*_, out_buf: []T): usize {
            const n = if (_.count < out_buf.len) _.count else out_buf.len;
            if (n == 0) return 0;
            const tail_space = _.cap - _.head;
            const first = if (n < tail_space) n else tail_space;
            @memcpy(out_buf[0..first], _.buf[_.head..(_.head + first)]);
            if (n > first) {
                const second = n - first;
                @memcpy(out_buf[first..n], _.buf[0..second]);
            }
            _.head = (_.head + n) % _.cap;
            _.count -= n;
            return n;
        }

        /// Current number of stored elements.
        pub fn len(): usize {
            return _.count;
        }

        /// Total fixed capacity of the ring buffer.
        pub fn capacity(): usize {
            return _.cap;
        }

        /// True if no elements are stored.
        pub fn isEmpty(): bool {
            return _.count == 0;
        }

        /// True if len() == capacity().
        pub fn isFull(): bool {
            return _.count == _.cap;
        }

        /// Cumulative count of overwritten oldest items under OverwriteOldest policy.
        pub fn overwrittenCount(): u64 {
            return _.overwritten_count;
        }

        /// Reset ring buffer to empty without freeing backing memory.
        pub fn clear(*_) {
            _.head = 0;
            _.count = 0;
        }
    };
}

/// High-performance circular byte stream pipe with wrap-around two-segment memcpy.
pub struct ByteRing {
    alloc: std.mem.Allocator,
    buf: []u8 = [],
    cap: usize = 0,
    head: usize = 0,
    count: usize = 0,

    /// Initialize a byte ring with fixed capacity.
    pub fn init(alloc: std.mem.Allocator, cap_size: usize): ByteRing ! RingError {
        if (cap_size == 0) return RingError.InvalidCapacity;
        const buf = alloc.alloc(u8, cap_size) catch return RingError.OutOfMemory;
        return ByteRing{
            alloc: alloc,
            buf: buf,
            cap: cap_size,
            head: 0,
            count: 0,
        };
    }

    /// Release backing byte buffer.
    pub fn deinit(*_) {
        if (_.buf.len > 0) {
            _.alloc.free(_.buf);
            _.buf = [];
        }
        _.cap = 0;
        _.head = 0;
        _.count = 0;
    }

    /// Write as many bytes from src as fit into writable space; returns bytes written.
    pub fn write(*_, src: []const u8): usize {
        const avail = _.cap - _.count;
        const to_write = if (src.len < avail) src.len else avail;
        if (to_write == 0) return 0;

        const tail = (_.head + _.count) % _.cap;
        const tail_space = _.cap - tail;
        const first = if (to_write < tail_space) to_write else tail_space;
        @memcpy(_.buf[tail..(tail + first)], src[0..first]);
        if (to_write > first) {
            const second = to_write - first;
            @memcpy(_.buf[0..second], src[first..to_write]);
        }
        _.count += to_write;
        return to_write;
    }

    /// Peek up to dst.len bytes without consuming them; returns bytes copied.
    pub fn peek(dst: []u8): usize {
        const to_read = if (dst.len < _.count) dst.len else _.count;
        if (to_read == 0) return 0;

        const tail_space = _.cap - _.head;
        const first = if (to_read < tail_space) to_read else tail_space;
        @memcpy(dst[0..first], _.buf[_.head..(_.head + first)]);
        if (to_read > first) {
            const second = to_read - first;
            @memcpy(dst[first..to_read], _.buf[0..second]);
        }
        return to_read;
    }

    /// Read and consume up to dst.len bytes into dst; returns bytes read.
    pub fn read(*_, dst: []u8): usize {
        const n = _.peek(dst);
        if (n > 0) {
            _.head = (_.head + n) % _.cap;
            _.count -= n;
        }
        return n;
    }

    /// Discard up to n readable bytes; returns bytes skipped.
    pub fn skip(*_, n: usize): usize {
        const to_skip = if (n < _.count) n else _.count;
        if (to_skip > 0) {
            _.head = (_.head + to_skip) % _.cap;
            _.count -= to_skip;
        }
        return to_skip;
    }

    /// Number of bytes currently available to read.
    pub fn readableLen(): usize {
        return _.count;
    }

    /// Number of free bytes currently available to write.
    pub fn writableLen(): usize {
        return _.cap - _.count;
    }

    /// Total capacity in bytes.
    pub fn capacity(): usize {
        return _.cap;
    }

    /// True if no readable bytes remain.
    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    /// True if no writable space remains.
    pub fn isFull(): bool {
        return _.count == _.cap;
    }

    /// Reset stream state without freeing backing memory.
    pub fn clear(*_) {
        _.head = 0;
        _.count = 0;
    }

    /// Scan for delim byte; if found, consume and return an owned slice including delim.
    /// Caller owns the returned slice and must free it with alloc.free(...).
    pub fn readUntilByte(*_, alloc: std.mem.Allocator, delim: u8): ([]u8)?! {
        let found_idx: usize? = null;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const phys = (_.head + i) % _.cap;
            if (_.buf[phys] == delim) {
                found_idx = i;
                break;
            }
        }
        const idx = found_idx ?? return null;
        const frame_len = idx + 1;
        const out = alloc.alloc(u8, frame_len) catch return RingError.OutOfMemory;
        const read_n = _.read(out);
        if (read_n != frame_len) {
            alloc.free(out);
            return null;
        }
        return out;
    }
}

/// Count-and-time dual-trigger micro-batch aggregator.
pub fn MicroBatcher(comptime T: type): type {
    return struct {
        const Self = @This();

        alloc: std.mem.Allocator,
        items: []T = [],
        max_batch_size: usize = 0,
        max_wait_ms: i64 = 0,
        count: usize = 0,
        first_item_ms: i64 = 0,
        has_first: bool = false,

        /// Initialize micro-batcher with max batch count and max wait window in milliseconds.
        pub fn init(alloc: std.mem.Allocator, max_batch_size: usize, max_wait_ms: i64): Self ! RingError {
            if (max_batch_size == 0 || max_wait_ms < 0) {
                return RingError.InvalidConfig;
            }
            const buf = alloc.alloc(T, max_batch_size) catch return RingError.OutOfMemory;
            return Self{
                alloc: alloc,
                items: buf,
                max_batch_size: max_batch_size,
                max_wait_ms: max_wait_ms,
                count: 0,
                first_item_ms: 0,
                has_first: false,
            };
        }

        /// Release backing batch buffer.
        pub fn deinit(*_) {
            if (_.items.len > 0) {
                _.alloc.free(_.items);
                _.items = [];
            }
            _.count = 0;
            _.has_first = false;
            _.first_item_ms = 0;
        }

        /// Add an item with current timestamp; returns true when max_batch_size is reached.
        pub fn add(*_, item: T, now_ms: i64): bool ! RingError {
            if (_.count >= _.max_batch_size) {
                return RingError.BufferFull;
            }
            if (!_.has_first) {
                _.first_item_ms = now_ms;
                _.has_first = true;
            }
            _.items[_.count] = item;
            _.count += 1;
            return _.count >= _.max_batch_size;
        }

        /// True if non-empty and either count >= max_batch_size or wait window elapsed.
        pub fn shouldFlush(now_ms: i64): bool {
            if (_.count == 0) return false;
            if (_.count >= _.max_batch_size) return true;
            return (now_ms - _.first_item_ms) >= _.max_wait_ms;
        }

        /// Drain all buffered items into a newly allocated slice and reset timer state.
        /// Caller owns the returned slice and must free it with alloc.free(...).
        pub fn drain(*_, alloc: std.mem.Allocator): []T ! RingError {
            if (_.count == 0) {
                const empty = alloc.alloc(T, 0) catch return RingError.OutOfMemory;
                _.has_first = false;
                _.first_item_ms = 0;
                return empty;
            }
            const out = alloc.alloc(T, _.count) catch return RingError.OutOfMemory;
            @memcpy(out, _.items[0.._.count]);
            _.count = 0;
            _.has_first = false;
            _.first_item_ms = 0;
            return out;
        }

        /// Number of buffered items awaiting flush.
        pub fn len(): usize {
            return _.count;
        }

        /// True if no items are currently buffered.
        pub fn isEmpty(): bool {
            return _.count == 0;
        }
    };
}
