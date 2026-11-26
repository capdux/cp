// @cap/ring-queue — Zero-heap bounded ring queue & wrap-around byte ring buffer in pure CP
// MIT License — Clean-room implementation inspired by Rust `crossbeam-queue` ArrayQueue

/// Bounded O(1) FIFO ring queue of `u64` operating over a caller-owned `[]u64` backing slice.
pub struct RingQueueU64 {
    pub head: usize,
    pub tail: usize,
    pub count: usize,
    pub capacity: usize,

    /// Create an empty ring queue with logical capacity `cap`.
    pub static fn new(cap: usize): RingQueueU64 {
        return RingQueueU64 {
            head: 0,
            tail: 0,
            count: 0,
            capacity: cap,
        };
    }

    /// Return the current number of stored elements.
    pub fn len(): usize {
        return _.count;
    }

    /// Return true if the queue has 0 elements.
    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    /// Return true if the queue has reached capacity.
    pub fn isFull(): bool {
        return _.count >= _.capacity;
    }

    /// Return remaining free slots before full.
    pub fn remaining(): usize {
        if (_.count >= _.capacity) {
            return 0;
        }
        return _.capacity - _.count;
    }

    /// Reset queue indices and count to empty in O(1).
    pub fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
    }

    /// Push `val` onto the back of the queue. Returns `false` if full or 0-capacity.
    pub fn push(*_, buf: []u64, val: u64): bool {
        const eff_cap: usize = if (buf.len < _.capacity) buf.len else _.capacity;
        if (eff_cap == 0 || _.count >= eff_cap) {
            return false;
        }
        buf[_.tail] = val;
        _.tail += 1;
        if (_.tail >= eff_cap) {
            _.tail = 0;
        }
        _.count += 1;
        return true;
    }

    /// Push `val` unconditionally: if full, overwrites the oldest element and returns `true`
    /// to indicate eviction; returns `false` if inserted without eviction (or 0-capacity).
    pub fn forcePush(*_, buf: []u64, val: u64): bool {
        const eff_cap: usize = if (buf.len < _.capacity) buf.len else _.capacity;
        if (eff_cap == 0) {
            return false;
        }
        if (_.count >= eff_cap) {
            buf[_.tail] = val;
            _.tail += 1;
            if (_.tail >= eff_cap) {
                _.tail = 0;
            }
            _.head = _.tail;
            return true;
        }
        buf[_.tail] = val;
        _.tail += 1;
        if (_.tail >= eff_cap) {
            _.tail = 0;
        }
        _.count += 1;
        return false;
    }

    /// Pop the oldest element into `out`. Returns `false` if empty.
    pub fn pop(*_, buf: []const u64, out: *u64): bool {
        const eff_cap: usize = if (buf.len < _.capacity) buf.len else _.capacity;
        if (eff_cap == 0 || _.count == 0) {
            return false;
        }
        out.* = buf[_.head];
        _.head += 1;
        if (_.head >= eff_cap) {
            _.head = 0;
        }
        _.count -= 1;
        return true;
    }

    /// Peek at the oldest element without removing it. Returns `false` if empty.
    pub fn peek(buf: []const u64, out: *u64): bool {
        const eff_cap: usize = if (buf.len < _.capacity) buf.len else _.capacity;
        if (eff_cap == 0 || _.count == 0) {
            return false;
        }
        out.* = buf[_.head];
        return true;
    }
}

/// High-performance zero-heap byte ring buffer with two-segment wrap-around bulk copy.
pub struct ByteRing {
    pub head: usize,
    pub tail: usize,
    pub count: usize,
    pub capacity: usize,

    /// Create an empty byte ring with logical capacity `cap`.
    pub static fn new(cap: usize): ByteRing {
        return ByteRing {
            head: 0,
            tail: 0,
            count: 0,
            capacity: cap,
        };
    }

    /// Return the number of readable bytes currently buffered.
    pub fn len(): usize {
        return _.count;
    }

    /// Return true if no bytes are buffered.
    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    /// Return true if the byte ring is completely full.
    pub fn isFull(): bool {
        return _.count >= _.capacity;
    }

    /// Return the number of free writable bytes remaining.
    pub fn remaining(): usize {
        if (_.count >= _.capacity) {
            return 0;
        }
        return _.capacity - _.count;
    }

    /// Reset byte ring to empty in O(1).
    pub fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
    }

    /// Write as many bytes from `src` as fit into `buf` (with automatic two-part wrap-around).
    /// Returns the number of bytes actually written.
    pub fn writeBytes(*_, buf: []u8, src: []const u8): usize {
        const eff_cap: usize = if (buf.len < _.capacity) buf.len else _.capacity;
        if (eff_cap == 0 || src.len == 0 || _.count >= eff_cap) {
            return 0;
        }
        const avail: usize = eff_cap - _.count;
        const to_write: usize = if (src.len < avail) src.len else avail;
        const space_to_end: usize = eff_cap - _.tail;
        const first: usize = if (space_to_end < to_write) space_to_end else to_write;

        let i: usize = 0;
        while (i < first) {
            buf[_.tail + i] = src[i];
            i += 1;
        }
        const second: usize = to_write - first;
        let j: usize = 0;
        while (j < second) {
            buf[j] = src[first + j];
            j += 1;
        }

        _.tail = (_.tail + to_write) % eff_cap;
        _.count += to_write;
        return to_write;
    }

    /// Read up to `dst.len` bytes from `buf` into `dst` (with automatic two-part wrap-around).
    /// Returns the number of bytes actually read.
    pub fn readBytes(*_, buf: []const u8, dst: []u8): usize {
        const eff_cap: usize = if (buf.len < _.capacity) buf.len else _.capacity;
        if (eff_cap == 0 || dst.len == 0 || _.count == 0) {
            return 0;
        }
        const to_read: usize = if (dst.len < _.count) dst.len else _.count;
        const data_to_end: usize = eff_cap - _.head;
        const first: usize = if (data_to_end < to_read) data_to_end else to_read;

        let i: usize = 0;
        while (i < first) {
            dst[i] = buf[_.head + i];
            i += 1;
        }
        const second: usize = to_read - first;
        let j: usize = 0;
        while (j < second) {
            dst[first + j] = buf[j];
            j += 1;
        }

        _.head = (_.head + to_read) % eff_cap;
        _.count -= to_read;
        return to_read;
    }
}
