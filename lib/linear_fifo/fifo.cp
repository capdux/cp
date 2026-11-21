// Source: Absorbed from /data/dev/bun/src/collections/linear_fifo.cp
// Pure-CP zero-allocation generic ring-buffer FIFO (`StaticLinearFifo`)
// supporting item/slice read/write, peek, discard, unget, and wrap-around.

/// Fixed-capacity zero-allocation ring buffer FIFO.
pub fn StaticLinearFifo(comptime T: type, comptime capacity: usize): type {
    return struct {
        buf: [capacity]T,
        head: usize,
        count: usize,

        const Self = @This();

        pub static fn init(): Self {
            return Self{
                buf: undefined,
                head: 0,
                count: 0,
            };
        }

        pub fn readableLength(): usize {
            return _.count;
        }

        pub fn writableLength(): usize {
            return capacity - _.count;
        }

        pub fn isEmpty(): bool {
            return _.count == 0;
        }

        pub fn isFull(): bool {
            return _.count == capacity;
        }

        pub fn clear(*_) {
            _.head = 0;
            _.count = 0;
        }

        /// Push a single item to the tail. Returns false if the buffer is full.
        pub fn writeItem(*_, item: T): bool {
            if (_.count >= capacity) return false;
            const tail = (_.head + _.count) % capacity;
            _.buf[tail] = item;
            _.count += 1;
            return true;
        }

        /// Pop a single item from the head. Returns null if empty.
        pub fn readItem(*_): T? {
            if (_.count == 0) return null;
            const item = _.buf[_.head];
            _.head = (_.head + 1) % capacity;
            _.count -= 1;
            return item;
        }

        /// Peek item at offset `idx` from the head without removing it.
        pub fn peekItem(idx: usize): T? {
            if (idx >= _.count) return null;
            return _.buf[(_.head + idx) % capacity];
        }

        /// Push an item back to the front of the FIFO (unread). Returns false if full.
        pub fn unget(*_, item: T): bool {
            if (_.count >= capacity) return false;
            _.head = if (_.head == 0) capacity - 1 else _.head - 1;
            _.buf[_.head] = item;
            _.count += 1;
            return true;
        }

        /// Discard up to `n` items from the head. Returns actual number discarded.
        pub fn discard(*_, n: usize): usize {
            const actual = @min(n, _.count);
            _.head = (_.head + actual) % capacity;
            _.count -= actual;
            return actual;
        }

        /// Write as many items from `src` as fit. Returns number of items written.
        pub fn writeSlice(*_, src: []const T): usize {
            const avail = capacity - _.count;
            const n = @min(src.len, avail);
            let i: usize = 0;
            while (i < n) : (i += 1) {
                const tail = (_.head + _.count) % capacity;
                _.buf[tail] = src[i];
                _.count += 1;
            }
            return n;
        }

        /// Read up to `dst.len` items into `dst`. Returns number of items read.
        pub fn readSlice(*_, dst: []T): usize {
            const n = @min(dst.len, _.count);
            let i: usize = 0;
            while (i < n) : (i += 1) {
                dst[i] = _.buf[_.head];
                _.head = (_.head + 1) % capacity;
                _.count -= 1;
            }
            return n;
        }
    };
}
