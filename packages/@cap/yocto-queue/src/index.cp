// @cap/yocto-queue — Tiny O(1) FIFO queue data structure
// Clean-room MIT CP implementation of npm `yocto-queue` (200M+/wk, dependency of `p-limit` / `p-queue`)
// Zero-heap fixed-capacity ring-buffer queues (`YoctoQueueU64`, `YoctoQueueI64`, `YoctoQueueU32`).

pub const YOCTO_QUEUE_CAP: usize = 256;

pub struct PopU64Result {
    pub ok: bool,
    pub value: u64,
}

pub struct PopI64Result {
    pub ok: bool,
    pub value: i64,
}

/// Zero-heap O(1) FIFO queue for `u64` items (capacity 256).
pub struct YoctoQueueU64 {
    buf: [256]u64,
    head: usize,
    tail: usize,
    count: usize,

    pub static fn new(): YoctoQueueU64 {
        return YoctoQueueU64 {
            buf: undefined,
            head: 0,
            tail: 0,
            count: 0,
        };
    }

    pub fn size(): usize {
        return _.count;
    }

    pub fn capacity(): usize {
        return YOCTO_QUEUE_CAP;
    }

    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    pub fn isFull(): bool {
        return _.count >= YOCTO_QUEUE_CAP;
    }

    /// Enqueue a value at the tail in O(1). Returns `false` if full.
    pub fn enqueue(*_, value: u64): bool {
        if (_.count >= YOCTO_QUEUE_CAP) { return false; }
        _.buf[_.tail] = value;
        _.tail = (_.tail + 1) % YOCTO_QUEUE_CAP;
        _.count = _.count + 1;
        return true;
    }

    /// Dequeue the head value in O(1). Returns `{ ok: false, value: 0 }` if empty.
    pub fn dequeue(*_): PopU64Result {
        if (_.count == 0) {
            return PopU64Result { ok: false, value: 0 };
        }
        let v = _.buf[_.head];
        _.head = (_.head + 1) % YOCTO_QUEUE_CAP;
        _.count = _.count - 1;
        return PopU64Result { ok: true, value: v };
    }

    /// Peek at the head value without removing it.
    pub fn peek(): PopU64Result {
        if (_.count == 0) {
            return PopU64Result { ok: false, value: 0 };
        }
        return PopU64Result { ok: true, value: _.buf[_.head] };
    }

    /// Peek at the item at logical index `idx` from the front (0 = head).
    pub fn peekAt(idx: usize): PopU64Result {
        if (idx >= _.count) {
            return PopU64Result { ok: false, value: 0 };
        }
        let pos = (_.head + idx) % YOCTO_QUEUE_CAP;
        return PopU64Result { ok: true, value: _.buf[pos] };
    }

    /// Clear all items in O(1).
    pub fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
    }

    /// Drain up to `out.len` items in FIFO order into `out` and return drained count.
    pub fn drain(*_, out: []u64): usize {
        let n: usize = 0;
        while (n < out.len && _.count > 0) {
            out[n] = _.buf[_.head];
            _.head = (_.head + 1) % YOCTO_QUEUE_CAP;
            _.count = _.count - 1;
            n = n + 1;
        }
        return n;
    }
}

/// Zero-heap O(1) FIFO queue for signed `i64` items (capacity 256).
pub struct YoctoQueueI64 {
    buf: [256]i64,
    head: usize,
    tail: usize,
    count: usize,

    pub static fn new(): YoctoQueueI64 {
        return YoctoQueueI64 {
            buf: undefined,
            head: 0,
            tail: 0,
            count: 0,
        };
    }

    pub fn size(): usize { return _.count; }
    pub fn isEmpty(): bool { return _.count == 0; }
    pub fn isFull(): bool { return _.count >= YOCTO_QUEUE_CAP; }

    pub fn enqueue(*_, value: i64): bool {
        if (_.count >= YOCTO_QUEUE_CAP) { return false; }
        _.buf[_.tail] = value;
        _.tail = (_.tail + 1) % YOCTO_QUEUE_CAP;
        _.count = _.count + 1;
        return true;
    }

    pub fn dequeue(*_): PopI64Result {
        if (_.count == 0) {
            return PopI64Result { ok: false, value: 0 };
        }
        let v = _.buf[_.head];
        _.head = (_.head + 1) % YOCTO_QUEUE_CAP;
        _.count = _.count - 1;
        return PopI64Result { ok: true, value: v };
    }

    pub fn peek(): PopI64Result {
        if (_.count == 0) {
            return PopI64Result { ok: false, value: 0 };
        }
        return PopI64Result { ok: true, value: _.buf[_.head] };
    }

    pub fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
    }
}
