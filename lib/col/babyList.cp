// Source: Absorbed from /data/dev/bun/src/collections/baby_list.cp | Synced: 2026-11-23T21:09:11Z
// ============================================================================
// Absorbed from Bun: bun/src/collections/baby_list.cp
// Upstream Sync: Refer to bun/src/collections/baby_list.zig for future upstream updates.
// ============================================================================
// lib/col/baby_list.cp
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

    pub static fn fromSlice(values: []const T): *BabyList<T> {
        const initial_cap: u32 = if (values.len > 0) (values.len as u32) else 8;
        let list = BabyList<T>.initCapacity(initial_cap);
        list.appendSlice(values);
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

    pub fn unusedCapacity(*_): usize {
        return (_.cap - _.len) as usize;
    }

    pub fn slice(*_): []T {
        return _.items[0 .. _.len as usize];
    }

    pub fn constSlice(*_): []const T {
        return _.items[0 .. _.len as usize];
    }

    pub fn unusedCapacitySlice(*_): []T {
        return _.items[_.len as usize .. _.cap as usize];
    }

    pub fn allocatedSlice(*_): []T {
        return _.items[0 .. _.cap as usize];
    }

    pub fn memoryCost(*_): usize {
        return (_.cap as usize) * @sizeOf(T);
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

    pub fn ensureTotalCapacityPrecise(*_, needed: u32) {
        if (needed <= _.cap) {
            return;
        }
        let new_items = std.heap.page_allocator.alloc(T, needed as usize) catch unreachable;
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
        _.cap = needed;
    }

    pub fn ensureUnusedCapacity(*_, needed: u32) {
        _.ensureTotalCapacity(_.len + needed);
    }

    pub fn expandToCapacity(*_) {
        _.len = _.cap;
    }

    pub fn shrinkRetainingCapacity(*_, new_len: u32) {
        if (new_len < _.len) {
            _.len = new_len;
        }
    }

    pub fn shrinkAndFree(*_, new_len: u32) {
        if (new_len >= _.cap) {
            return;
        }
        const actual_len = if (new_len < _.len) new_len else _.len;
        const new_cap = if (new_len > 0) new_len else 8;
        let new_items = std.heap.page_allocator.alloc(T, new_cap as usize) catch unreachable;
        let i: usize = 0;
        while (i < (actual_len as usize)) {
            new_items[i] = _.items[i];
            i += 1;
        }
        if (_.cap > 0) {
            std.heap.page_allocator.free(_.items);
        }
        _.items = new_items;
        _.cap = new_cap;
        _.len = actual_len;
    }

    pub fn append(*_, value: T) {
        _.ensureTotalCapacity(_.len + 1);
        _.items[_.len as usize] = value;
        _.len += 1;
    }

    pub fn appendAssumeCapacity(*_, value: T) {
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

    pub fn appendSliceAssumeCapacity(*_, values: []const T) {
        const base = _.len as usize;
        let i: usize = 0;
        while (i < values.len) {
            _.items[base + i] = values[i];
            i += 1;
        }
        _.len += (values.len as u32);
    }

    pub fn at(*_, index: usize): T {
        return _.items[index];
    }

    pub fn get(*_, index: usize): T {
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

    pub fn orderedRemove(*_, index: usize): T {
        const val = _.items[index];
        let i = index;
        const n = (_.len as usize) - 1;
        while (i < n) {
            _.items[i] = _.items[i + 1];
            i += 1;
        }
        _.len -= 1;
        return val;
    }

    pub fn swapRemove(*_, index: usize): T {
        const val = _.items[index];
        _.len -= 1;
        if (index != (_.len as usize)) {
            _.items[index] = _.items[_.len as usize];
        }
        return val;
    }

    pub fn insert(*_, index: usize, val: T) {
        _.ensureTotalCapacity(_.len + 1);
        let j = _.len as usize;
        while (j > index) {
            _.items[j] = _.items[j - 1];
            j -= 1;
        }
        _.items[index] = val;
        _.len += 1;
    }

    pub fn insertSlice(*_, index: usize, vals: []const T) {
        if (vals.len == 0) {
            return;
        }
        const add_len = vals.len as u32;
        _.ensureTotalCapacity(_.len + add_len);
        const shift = vals.len;
        let j = (_.len as usize) + shift;
        while (j > index + shift) {
            _.items[j - 1] = _.items[j - 1 - shift];
            j -= 1;
        }
        let k: usize = 0;
        while (k < shift) {
            _.items[index + k] = vals[k];
            k += 1;
        }
        _.len += add_len;
    }

    pub fn contains(*_, item: T): bool {
        let i: usize = 0;
        const n = _.len as usize;
        while (i < n) {
            if (_.items[i] == item) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn findIndex(*_, item: T): usize? {
        let i: usize = 0;
        const n = _.len as usize;
        while (i < n) {
            if (_.items[i] == item) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    pub fn clone(*_): *BabyList<T> {
        let cloned = BabyList<T>.initCapacity(_.cap);
        cloned.appendSlice(_.slice());
        return cloned;
    }

    pub fn clearRetainingCapacity(*_) {
        _.len = 0;
    }

    pub fn clearAndFree(*_) {
        if (_.cap > 0) {
            std.heap.page_allocator.free(_.items);
            _.items = [];
            _.cap = 0;
            _.len = 0;
        }
    }

    pub fn deinit(*_) {
        if (_.cap > 0) {
            std.heap.page_allocator.free(_.items);
            _.cap = 0;
            _.len = 0;
        }
    }

    ~init() {
        _.deinit();
        std.heap.page_allocator.destroy(_);
    }
}

pub struct OffsetByteList {
    pub list: *BabyList<u8>,
    pub head: u32 = 0,

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

    pub fn len(*_): u32 {
        return _.remainingLen();
    }

    pub fn write(*_, bytes: []const u8) {
        _.list.appendSlice(bytes);
    }

    pub fn slice(*_): []u8 {
        return _.list.slice();
    }

    pub fn remaining(*_): []u8 {
        if (_.head >= _.list.len) {
            return [];
        }
        return _.list.items[(_.head as usize) .. (_.list.len as usize)];
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

    pub fn clear(*_) {
        _.reset();
    }

    pub fn clearAndFree(*_) {
        _.head = 0;
        _.list.clearAndFree();
    }

    pub fn deinit(*_) {
        _.list.deinit();
    }

    ~init() {
        _.list.~init();
        std.heap.page_allocator.destroy(_);
    }
}
