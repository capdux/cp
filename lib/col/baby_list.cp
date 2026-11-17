// Source: Bun src/collections/baby_list.cp
// lib/collections/baby_list.cp
// Ultra-compact u32-indexed dynamic list & OffsetByteList
// Absorbed from Bun src/collections/baby_list.cp


pub class BabyList<T> {
    pub items: []T = [];
    pub len: u32 = 0;
    pub cap: u32 = 0;

    pub static fn init(): *BabyList<T> {
        let list = std.heap.page_allocator.create(@This()) catch unreachable;
        list.items = (std.heap.page_allocator.alloc(T, 8) catch unreachable);
        list.len = 0;
        list.cap = 8;
        return list;
    }

    pub static fn initCapacity(initial_cap: u32): *BabyList<T> {
        const actual_cap: u32 = if (initial_cap > 0) initial_cap else 8;
        let list = std.heap.page_allocator.create(@This()) catch unreachable;
        list.items = (std.heap.page_allocator.alloc(T, actual_cap as usize) catch unreachable);
        list.len = 0;
        list.cap = actual_cap;
        return list;
    }

    pub fn isEmpty(*_): bool {
        return _.len == 0;
    }

    pub fn count(*_): usize {
        return _.len as usize;
    }

    pub fn capacity(*_): usize {
        return _.cap as usize;
    }

    pub fn ensureTotalCapacity(*_, needed: u32) {
        if (needed <= _.cap) {
            return;
        }
        let new_cap: u32 = if (_.cap > 0) _.cap else 8;
        while (new_cap < needed) {
            new_cap = new_cap * 2;
        }
        let new_items = std.heap.page_allocator.alloc(T, new_cap as usize) catch unreachable;
        let i: usize = 0;
        const n = _.len as usize;
        while (i < n) {
            new_items[i] = _.items[i];
            i += 1;
        }
        if (_.cap > 0) {
            std.heap.page_allocator.free(_.items);
        }
        _.items = new_items;
        _.cap = new_cap;
    }

    pub fn append(*_, value: T) {
        _.ensureTotalCapacity(_.len + 1);
        _.items[_.len as usize] = value;
        _.len += 1;
    }

    pub fn appendSlice(*_, values: []const T) {
        const add_len = values.len as u32;
        if (add_len == 0) {
            return;
        }
        _.ensureTotalCapacity(_.len + add_len);
        const base = _.len as usize;
        let i: usize = 0;
        while (i < values.len) {
            _.items[base + i] = values[i];
            i += 1;
        }
        _.len += add_len;
    }

    pub fn at(*_, index: usize): T {
        return _.items[index];
    }

    pub fn set(*_, index: usize, value: T) {
        if (index < (_.len as usize)) {
            _.items[index] = value;
        }
    }

    pub fn first(*_): T? {
        if (_.len == 0) {
            return null;
        }
        return _.items[0];
    }

    pub fn last(*_): T? {
        if (_.len == 0) {
            return null;
        }
        return _.items[(_.len - 1) as usize];
    }

    pub fn pop(*_): T? {
        if (_.len == 0) {
            return null;
        }
        _.len -= 1;
        return _.items[_.len as usize];
    }

    pub fn clearRetainingCapacity(*_) {
        _.len = 0;
    }

    pub fn deinit(*_) {
        if (_.cap > 0) {
            std.heap.page_allocator.free(_.items);
            _.cap = 0;
            _.len = 0;
        }
    }

    pub fn release(*_) {
        _.deinit();
        std.heap.page_allocator.destroy(_);
    }
}

// Absorbed from Bun OffsetByteList: BabyList<u8> with a read head offset
pub class OffsetByteList {
    pub list: *BabyList<u8>;
    pub head: u32 = 0;

    pub static fn init(initial_cap: u32): *OffsetByteList {
        let obl = std.heap.page_allocator.create(OffsetByteList) catch unreachable;
        obl.list = BabyList<u8>.initCapacity(initial_cap);
        obl.head = 0;
        return obl;
    }

    pub fn remainingLen(*_): u32 {
        if (_.head >= _.list.len) {
            return 0;
        }
        return _.list.len - _.head;
    }

    pub fn write(*_, bytes: []const u8) {
        _.list.appendSlice(bytes);
    }

    pub fn consume(*_, n: u32): u32 {
        const rem = _.remainingLen();
        const step = if (n < rem) n else rem;
        _.head += step;
        if (_.head == _.list.len) {
            _.head = 0;
            _.list.clearRetainingCapacity();
        }
        return step;
    }

    pub fn reset(*_) {
        _.head = 0;
        _.list.clearRetainingCapacity();
    }

    pub fn release(*_) {
        _.list.release();
        std.heap.page_allocator.destroy(_);
    }
}
