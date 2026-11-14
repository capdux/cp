// lib/collections/linear_fifo.cp
// High-throughput power-of-two ring-buffer FIFO queue
// Absorbed from Bun src/collections/linear_fifo.cp


pub class LinearFifo<T> {
    pub buf: []T = [];
    pub cap: usize = 0;
    pub head: usize = 0;
    pub count: usize = 0;

    pub static fn init(initial_power_of_two_cap: usize): *LinearFifo<T> {
        let c: usize = 16;
        while (c < initial_power_of_two_cap) {
            c = c * 2;
        }
        let fifo = std.heap.page_allocator.create(@This()) catch unreachable;
        fifo.buf = (std.heap.page_allocator.alloc(T, c) catch unreachable);
        fifo.cap = c;
        fifo.head = 0;
        fifo.count = 0;
        return fifo;
    }

    pub fn readableLength(*_): usize {
        return _.count;
    }

    pub fn writableLength(*_): usize {
        return _.cap - _.count;
    }

    pub fn isEmpty(*_): bool {
        return _.count == 0;
    }

    pub fn ensureCapacity(*_, additional: usize) {
        const needed = _.count + additional;
        if (needed <= _.cap) {
            return;
        }
        let new_cap = _.cap * 2;
        while (new_cap < needed) {
            new_cap = new_cap * 2;
        }
        let new_buf = std.heap.page_allocator.alloc(T, new_cap) catch unreachable;
        const mask = _.cap - 1;
        let i: usize = 0;
        while (i < _.count) {
            new_buf[i] = _.buf[(_.head + i) & mask];
            i += 1;
        }
        std.heap.page_allocator.free(_.buf);
        _.buf = new_buf;
        _.cap = new_cap;
        _.head = 0;
    }

    pub fn writeItem(*_, item: T) {
        _.ensureCapacity(1);
        const mask = _.cap - 1;
        const write_idx = (_.head + _.count) & mask;
        _.buf[write_idx] = item;
        _.count += 1;
    }

    pub fn writeSlice(*_, items: []const T) {
        if (items.len == 0) {
            return;
        }
        _.ensureCapacity(items.len);
        const mask = _.cap - 1;
        let i: usize = 0;
        while (i < items.len) {
            const write_idx = (_.head + _.count + i) & mask;
            _.buf[write_idx] = items[i];
            i += 1;
        }
        _.count += items.len;
    }

    pub fn peekItem(*_, offset: usize): T? {
        if (offset >= _.count) {
            return null;
        }
        const mask = _.cap - 1;
        return _.buf[(_.head + offset) & mask];
    }

    pub fn readItem(*_): T? {
        if (_.count == 0) {
            return null;
        }
        const mask = _.cap - 1;
        const val = _.buf[_.head];
        _.head = (_.head + 1) & mask;
        _.count -= 1;
        if (_.count == 0) {
            _.head = 0;
        }
        return val;
    }

    pub fn readSlice(*_, out: []T): usize {
        const to_read = if (out.len < _.count) out.len else _.count;
        const mask = _.cap - 1;
        let i: usize = 0;
        while (i < to_read) {
            out[i] = _.buf[(_.head + i) & mask];
            i += 1;
        }
        _.discard(to_read);
        return to_read;
    }

    pub fn discard(*_, n: usize) {
        if (n >= _.count) {
            _.head = 0;
            _.count = 0;
            return;
        }
        const mask = _.cap - 1;
        _.head = (_.head + n) & mask;
        _.count -= n;
    }

    pub fn rewind(*_) {
        _.head = 0;
        _.count = 0;
    }

    pub fn deinit(*_) {
        if (_.cap > 0) {
            std.heap.page_allocator.free(_.buf);
            _.cap = 0;
            _.head = 0;
            _.count = 0;
        }
    }

    pub fn release(*_) {
        _.deinit();
        std.heap.page_allocator.destroy(_);
    }
}
