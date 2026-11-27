// @cap/ringbuf — Zero-heap O(1) power-of-two circular ring buffers (`RingBufU8`, `RingBufI64`).
// Clean-room MIT port of Rust's `ringbuf` crate.

pub const RB_U8_CAP: usize = 256;
pub const RB_U8_MASK: usize = 255;
pub const RB_I64_CAP: usize = 64;
pub const RB_I64_MASK: usize = 63;

/// Fixed-capacity 256-byte circular FIFO ring buffer.
pub struct RingBufU8 {
    pub buf: [256]u8,
    pub head: usize,
    pub tail: usize,
    pub count: usize,

    pub static fn new(): RingBufU8 {
        let arr: [256]u8 = undefined;
        let i: usize = 0;
        while (i < RB_U8_CAP) { arr[i] = 0; i = i + 1; }
        return RingBufU8 { buf: arr, head: 0, tail: 0, count: 0 };
    }

    pub fn len(): usize { return _.count; }
    pub fn capacity(): usize { return RB_U8_CAP; }
    pub fn freeLen(): usize { return RB_U8_CAP - _.count; }
    pub fn isEmpty(): bool { return _.count == 0; }
    pub fn isFull(): bool { return _.count >= RB_U8_CAP; }

    pub fn push(*_, b: u8): bool {
        if (_.count >= RB_U8_CAP) { return false; }
        _.buf[_.tail] = b;
        _.tail = (_.tail + 1) & RB_U8_MASK;
        _.count = _.count + 1;
        return true;
    }

    /// Pushes `b`, overwriting the oldest byte if full; returns overwritten byte or `null`.
    pub fn pushOverwrite(*_, b: u8): u8? {
        if (_.count >= RB_U8_CAP) {
            let old = _.buf[_.head];
            _.buf[_.tail] = b;
            _.head = (_.head + 1) & RB_U8_MASK;
            _.tail = (_.tail + 1) & RB_U8_MASK;
            return old;
        }
        _.buf[_.tail] = b;
        _.tail = (_.tail + 1) & RB_U8_MASK;
        _.count = _.count + 1;
        return null;
    }

    pub fn pop(*_): u8? {
        if (_.count == 0) { return null; }
        let b = _.buf[_.head];
        _.head = (_.head + 1) & RB_U8_MASK;
        _.count = _.count - 1;
        return b;
    }

    pub fn peek(): u8? {
        if (_.count == 0) { return null; }
        return _.buf[_.head];
    }

    pub fn pushSlice(*_, src: []const u8): usize {
        let n: usize = 0;
        while (n < src.len && _.count < RB_U8_CAP) {
            _.buf[_.tail] = src[n];
            _.tail = (_.tail + 1) & RB_U8_MASK;
            _.count = _.count + 1;
            n = n + 1;
        }
        return n;
    }

    pub fn popSlice(*_, dst: []u8): usize {
        let n: usize = 0;
        while (n < dst.len && _.count > 0) {
            dst[n] = _.buf[_.head];
            _.head = (_.head + 1) & RB_U8_MASK;
            _.count = _.count - 1;
            n = n + 1;
        }
        return n;
    }

    pub fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
    }
}

/// Fixed-capacity 64-element `i64` circular FIFO ring buffer.
pub struct RingBufI64 {
    pub buf: [64]i64,
    pub head: usize,
    pub tail: usize,
    pub count: usize,

    pub static fn new(): RingBufI64 {
        let arr: [64]i64 = undefined;
        let i: usize = 0;
        while (i < RB_I64_CAP) { arr[i] = 0; i = i + 1; }
        return RingBufI64 { buf: arr, head: 0, tail: 0, count: 0 };
    }

    pub fn len(): usize { return _.count; }
    pub fn capacity(): usize { return RB_I64_CAP; }
    pub fn isEmpty(): bool { return _.count == 0; }
    pub fn isFull(): bool { return _.count >= RB_I64_CAP; }

    pub fn push(*_, val: i64): bool {
        if (_.count >= RB_I64_CAP) { return false; }
        _.buf[_.tail] = val;
        _.tail = (_.tail + 1) & RB_I64_MASK;
        _.count = _.count + 1;
        return true;
    }

    pub fn pushOverwrite(*_, val: i64): i64? {
        if (_.count >= RB_I64_CAP) {
            let old = _.buf[_.head];
            _.buf[_.tail] = val;
            _.head = (_.head + 1) & RB_I64_MASK;
            _.tail = (_.tail + 1) & RB_I64_MASK;
            return old;
        }
        _.buf[_.tail] = val;
        _.tail = (_.tail + 1) & RB_I64_MASK;
        _.count = _.count + 1;
        return null;
    }

    pub fn pop(*_): i64? {
        if (_.count == 0) { return null; }
        let v = _.buf[_.head];
        _.head = (_.head + 1) & RB_I64_MASK;
        _.count = _.count - 1;
        return v;
    }

    pub fn peek(): i64? {
        if (_.count == 0) { return null; }
        return _.buf[_.head];
    }

    pub fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
    }
}
