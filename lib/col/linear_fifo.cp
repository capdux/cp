// Source: Absorbed from /data/dev/bun/src/collections/linear_fifo.cp | Synced: 2026-11-21T12:36:33Z
// ============================================================================
// Absorbed from Bun: bun/src/collections/linear_fifo.cp
// Upstream Sync: Refer to bun/src/collections/linear_fifo.zig for future upstream updates.
// ============================================================================
// lib/col/linear_fifo.cp
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

    pub fn capacity(): usize {
        return _.cap;
    }

    pub fn readableLength(): usize {
        return _.count;
    }

    pub fn writableLength(): usize {
        return _.cap - _.count;
    }

    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    pub fn isFull(): bool {
        return _.count >= _.cap;
    }

    pub fn realign(*_) {
        if (_.head == 0 || _.count == 0) {
            _.head = 0;
            return;
        }
        let tmp = std.heap.page_allocator.alloc(T, _.count) catch unreachable;
        const mask = _.cap - 1;
        let i: usize = 0;
        while (i < _.count) {
            tmp[i] = _.buf[(_.head + i) & mask];
            i += 1;
        }
        i = 0;
        while (i < _.count) {
            _.buf[i] = tmp[i];
            i += 1;
        }
        std.heap.page_allocator.free(tmp);
        _.head = 0;
    }

    pub fn ensureTotalCapacity(*_, needed: usize) {
        if (needed <= _.cap) {
            return;
        }
        let new_cap = if (_.cap > 0) _.cap * 2 else 16;
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
        if (_.cap > 0) {
            std.heap.page_allocator.free(_.buf);
        }
        _.buf = new_buf;
        _.cap = new_cap;
        _.head = 0;
    }

    pub fn ensureCapacity(*_, additional: usize) {
        const needed = _.count + additional;
        if (needed <= _.cap) {
            return;
        }
        _.ensureTotalCapacity(needed);
    }

    pub fn ensureUnusedCapacity(*_, additional: usize) {
        _.ensureCapacity(additional);
    }

    pub fn shrink(*_, size: usize) {
        if (size < _.count) {
            return;
        }
        let target_cap: usize = 16;
        while (target_cap < size) {
            target_cap = target_cap * 2;
        }
        if (target_cap >= _.cap) {
            return;
        }
        _.realign();
        let new_buf = std.heap.page_allocator.alloc(T, target_cap) catch unreachable;
        let i: usize = 0;
        while (i < _.count) {
            new_buf[i] = _.buf[i];
            i += 1;
        }
        std.heap.page_allocator.free(_.buf);
        _.buf = new_buf;
        _.cap = target_cap;
        _.head = 0;
    }

    pub fn readableSlice(*_, offset: usize): []const T {
        if (offset >= _.count) {
            return [];
        }
        const start = (_.head + offset) & (_.cap - 1);
        const remaining = _.count - offset;
        const contig = _.cap - start;
        const len = if (remaining < contig) remaining else contig;
        return _.buf[start .. start + len];
    }

    pub fn writableSlice(*_, offset: usize): []T {
        const wlen = _.writableLength();
        if (offset >= wlen) {
            return [];
        }
        const tail = (_.head + _.count + offset) & (_.cap - 1);
        const remaining = wlen - offset;
        const contig = _.cap - tail;
        const len = if (remaining < contig) remaining else contig;
        return _.buf[tail .. tail + len];
    }

    pub fn update(*_, count: usize) {
        _.count += count;
    }

    pub fn writeItem(*_, item: T) {
        _.ensureCapacity(1);
        const mask = _.cap - 1;
        const write_idx = (_.head + _.count) & mask;
        _.buf[write_idx] = item;
        _.count += 1;
    }

    pub fn writeItemAssumeCapacity(*_, item: T) {
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

    pub fn writeAssumeCapacity(*_, src: []const T) {
        const mask = _.cap - 1;
        let i: usize = 0;
        while (i < src.len) {
            const write_idx = (_.head + _.count + i) & mask;
            _.buf[write_idx] = src[i];
            i += 1;
        }
        _.count += src.len;
    }

    pub fn write(*_, src: []const T) {
        _.writeSlice(src);
    }

    pub fn peekItem(*_, offset: usize): T? {
        if (offset >= _.count) {
            return null;
        }
        const mask = _.cap - 1;
        return _.buf[(_.head + offset) & mask];
    }

    pub fn first(*_): T? {
        return _.peekItem(0);
    }

    pub fn last(*_): T? {
        if (_.count == 0) {
            return null;
        }
        return _.peekItem(_.count - 1);
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

    pub fn read(*_, dst: []T): usize {
        return _.readSlice(dst);
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

    pub fn unget(*_, src: []const T) {
        if (src.len == 0) {
            return;
        }
        _.ensureCapacity(src.len);
        const mask = _.cap - 1;
        _.head = (_.head + _.cap - (src.len & mask)) & mask;
        let i: usize = 0;
        while (i < src.len) {
            _.buf[(_.head + i) & mask] = src[i];
            i += 1;
        }
        _.count += src.len;
    }

    pub fn orderedRemoveItem(*_, offset: usize): bool {
        if (offset >= _.count) {
            return false;
        }
        if (offset == 0) {
            * = _.readItem();
            return true;
        }
        const mask = _.cap - 1;
        let i = offset;
        while (i + 1 < _.count) {
            const cur_idx = (_.head + i) & mask;
            const next_idx = (_.head + i + 1) & mask;
            _.buf[cur_idx] = _.buf[next_idx];
            i += 1;
        }
        _.count -= 1;
        return true;
    }

    pub fn clear(*_) {
        _.head = 0;
        _.count = 0;
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

    ~init() {
        _.deinit();
        std.heap.page_allocator.destroy(_);
    }
}
