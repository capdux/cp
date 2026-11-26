// @cap/ringbuf — Zero-allocation power-of-two ring deque & streaming byte ring buffer for CP
// Clean-room MIT implementation inspired by Rust `ringbuf` and `arraydeque`.

pub const RING_DEQUE_CAP: usize = 64;
const RING_DEQUE_MASK: usize = 63;

pub const BYTE_RING_CAP: usize = 512;
const BYTE_RING_MASK: usize = 511;

// ---------------------------------------------------------------------------
// 1. RingDequeI64 (Fixed 64-element O(1) Double-Ended Ring Queue)
// ---------------------------------------------------------------------------

pub struct RingDequeI64 {
    pub data: [64]i64,
    pub head: usize,
    pub count: usize,

    pub static fn init() -> RingDequeI64 {
        let data: [64]i64 = undefined;
        let i: usize = 0;
        while (i < RING_DEQUE_CAP) {
            data[i] = 0;
            i += 1;
        }
        return RingDequeI64{
            data: data,
            head: 0,
            count: 0,
        };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn capacity() -> usize {
        return RING_DEQUE_CAP;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn isFull() -> bool {
        return _.count == RING_DEQUE_CAP;
    }

    pub fn clear(*_) {
        _.head = 0;
        _.count = 0;
    }

    pub fn pushBack(*_, v: i64) -> bool {
        if (_.count >= RING_DEQUE_CAP) {
            return false;
        }
        const idx = (_.head + _.count) & RING_DEQUE_MASK;
        _.data[idx] = v;
        _.count += 1;
        return true;
    }

    pub fn pushFront(*_, v: i64) -> bool {
        if (_.count >= RING_DEQUE_CAP) {
            return false;
        }
        _.head = (_.head + RING_DEQUE_MASK) & RING_DEQUE_MASK;
        _.data[_.head] = v;
        _.count += 1;
        return true;
    }

    /// Appends `v` to the back. If full, overwrites and returns the oldest front element.
    pub fn pushBackOverwrite(*_, v: i64) -> i64? {
        if (_.count < RING_DEQUE_CAP) {
            const idx = (_.head + _.count) & RING_DEQUE_MASK;
            _.data[idx] = v;
            _.count += 1;
            return null;
        }
        const evicted = _.data[_.head];
        _.data[_.head] = v;
        _.head = (_.head + 1) & RING_DEQUE_MASK;
        return evicted;
    }

    pub fn popFront(*_) -> i64? {
        if (_.count == 0) {
            return null;
        }
        const v = _.data[_.head];
        _.head = (_.head + 1) & RING_DEQUE_MASK;
        _.count -= 1;
        return v;
    }

    pub fn popBack(*_) -> i64? {
        if (_.count == 0) {
            return null;
        }
        _.count -= 1;
        const idx = (_.head + _.count) & RING_DEQUE_MASK;
        return _.data[idx];
    }

    pub fn front() -> i64? {
        if (_.count == 0) {
            return null;
        }
        return _.data[_.head];
    }

    pub fn back() -> i64? {
        if (_.count == 0) {
            return null;
        }
        const idx = (_.head + _.count - 1) & RING_DEQUE_MASK;
        return _.data[idx];
    }

    pub fn get(idx: usize) -> i64? {
        if (idx >= _.count) {
            return null;
        }
        return _.data[(_.head + idx) & RING_DEQUE_MASK];
    }
}

// ---------------------------------------------------------------------------
// 2. ByteRingBuf (Fixed 512-byte Streaming Ring Buffer)
// ---------------------------------------------------------------------------

pub struct ByteRingBuf {
    pub data: [512]u8,
    pub head: usize,
    pub count: usize,

    pub static fn init() -> ByteRingBuf {
        let data: [512]u8 = undefined;
        let i: usize = 0;
        while (i < BYTE_RING_CAP) {
            data[i] = 0;
            i += 1;
        }
        return ByteRingBuf{
            data: data,
            head: 0,
            count: 0,
        };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn freeSpace() -> usize {
        return BYTE_RING_CAP - _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn isFull() -> bool {
        return _.count == BYTE_RING_CAP;
    }

    pub fn clear(*_) {
        _.head = 0;
        _.count = 0;
    }

    pub fn writeByte(*_, b: u8) -> bool {
        if (_.count >= BYTE_RING_CAP) {
            return false;
        }
        const idx = (_.head + _.count) & BYTE_RING_MASK;
        _.data[idx] = b;
        _.count += 1;
        return true;
    }

    pub fn readByte(*_) -> u8? {
        if (_.count == 0) {
            return null;
        }
        const b = _.data[_.head];
        _.head = (_.head + 1) & BYTE_RING_MASK;
        _.count -= 1;
        return b;
    }

    pub fn peekByte(offset: usize) -> u8? {
        if (offset >= _.count) {
            return null;
        }
        return _.data[(_.head + offset) & BYTE_RING_MASK];
    }

    /// Writes as many bytes from `data` as fit into the ring buffer, returning bytes written.
    pub fn writeBytes(*_, data: string) -> usize {
        const avail = BYTE_RING_CAP - _.count;
        const to_write = if (data.len < avail) data.len else avail;
        let i: usize = 0;
        while (i < to_write) {
            const idx = (_.head + _.count + i) & BYTE_RING_MASK;
            _.data[idx] = data[i];
            i += 1;
        }
        _.count += to_write;
        return to_write;
    }

    /// Reads up to `out.len` bytes into `out` (wrapping seamlessly across buffer end) and returns the slice.
    pub fn readBytes(*_, out: []u8) -> string {
        const to_read = if (out.len < _.count) out.len else _.count;
        let i: usize = 0;
        while (i < to_read) {
            out[i] = _.data[(_.head + i) & BYTE_RING_MASK];
            i += 1;
        }
        _.head = (_.head + to_read) & BYTE_RING_MASK;
        _.count -= to_read;
        return out[0..to_read];
    }
}
