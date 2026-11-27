// Source: native-cp-stdlib (Roaring Bitmap Compressed Set)
const std = @import("std");

pub const ARRAY_MAX_SIZE: usize = 4096;
pub const BITSET_WORDS: usize = 1024;
const SERIAL_MAGIC: [4]u8 = [0x52, 0x4F, 0x41, 0x52];

pub const RoaringError = error{
    InvalidFormat,
    OutOfMemory,
};

pub const ArrayContainer = struct {
    data: std.ArrayList(u16),

    pub static fn init(): ArrayContainer {
        return .{ .data = std.ArrayList(u16).empty };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.data.deinit(alloc);
    }

    pub fn clone(alloc: std.mem.Allocator): !ArrayContainer {
        let out = ArrayContainer.init();
        errdefer out.deinit(alloc);
        try out.data.appendSlice(alloc, _.data.items);
        return out;
    }

    fn lowerBound(val: u16): usize {
        let lo: usize = 0;
        let hi: usize = _.data.items.len;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            if (_.data.items[mid] < val) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    fn upperBound(val: u16): usize {
        let lo: usize = 0;
        let hi: usize = _.data.items.len;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            if (_.data.items[mid] <= val) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    pub fn contains(val: u16): bool {
        const idx = _.lowerBound(val);
        return idx < _.data.items.len and _.data.items[idx] == val;
    }

    pub fn add(*_, alloc: std.mem.Allocator, val: u16): !bool {
        const idx = _.lowerBound(val);
        if (idx < _.data.items.len and _.data.items[idx] == val) {
            return false;
        }
        try _.data.insert(alloc, idx, val);
        return true;
    }

    pub fn remove(*_, val: u16): bool {
        const idx = _.lowerBound(val);
        if (idx >= _.data.items.len or _.data.items[idx] != val) {
            return false;
        }
        * = _.data.orderedRemove(idx);
        return true;
    }

    pub fn cardinality(): u32 {
        return @intCast(_.data.items.len);
    }

    pub fn rank(val: u16): u32 {
        return @intCast(_.upperBound(val));
    }

    pub fn minimum(): u16? {
        if (_.data.items.len == 0) return null;
        return _.data.items[0];
    }

    pub fn maximum(): u16? {
        if (_.data.items.len == 0) return null;
        return _.data.items[_.data.items.len - 1];
    }

    pub fn toBitset(alloc: std.mem.Allocator): !BitsetContainer {
        let bs = try BitsetContainer.init(alloc);
        errdefer bs.deinit(alloc);
        for (v of _.data.items) {
            * = bs.add(v);
        }
        return bs;
    }
};

pub const BitsetContainer = struct {
    words: *[BITSET_WORDS]u64,
    card: u32,

    pub static fn init(alloc: std.mem.Allocator): !BitsetContainer {
        const words = try alloc.create([BITSET_WORDS]u64);
        let i: usize = 0;
        while (i < BITSET_WORDS) {
            words[i] = 0;
            i += 1;
        }
        return .{ .words = words, .card = 0 };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.destroy(_.words);
    }

    pub fn clone(alloc: std.mem.Allocator): !BitsetContainer {
        const words = try alloc.create([BITSET_WORDS]u64);
        let i: usize = 0;
        while (i < BITSET_WORDS) {
            words[i] = _.words[i];
            i += 1;
        }
        return .{ .words = words, .card = _.card };
    }

    pub fn recomputeCardinality(*_) {
        let total: u32 = 0;
        let i: usize = 0;
        while (i < BITSET_WORDS) {
            const pc: u32 = @intCast(@popCount(_.words[i]));
            total += pc;
            i += 1;
        }
        _.card = total;
    }

    pub fn contains(val: u16): bool {
        const word_idx: usize = @intCast(val >> 6);
        const bit_idx: u6 = @intCast(val & 63);
        const mask: u64 = @as(u64, 1) << bit_idx;
        return (_.words[word_idx] & mask) != 0;
    }

    pub fn add(*_, val: u16): bool {
        const word_idx: usize = @intCast(val >> 6);
        const bit_idx: u6 = @intCast(val & 63);
        const mask: u64 = @as(u64, 1) << bit_idx;
        if ((_.words[word_idx] & mask) != 0) {
            return false;
        }
        _.words[word_idx] |= mask;
        _.card += 1;
        return true;
    }

    pub fn remove(*_, val: u16): bool {
        const word_idx: usize = @intCast(val >> 6);
        const bit_idx: u6 = @intCast(val & 63);
        const mask: u64 = @as(u64, 1) << bit_idx;
        if ((_.words[word_idx] & mask) == 0) {
            return false;
        }
        _.words[word_idx] &= ~mask;
        _.card -= 1;
        return true;
    }

    pub fn cardinality(): u32 {
        return _.card;
    }

    pub fn rank(val: u16): u32 {
        const word_idx: usize = @intCast(val >> 6);
        const bit_idx: u6 = @intCast(val & 63);
        let total: u32 = 0;
        let i: usize = 0;
        while (i < word_idx) {
            const pc: u32 = @intCast(@popCount(_.words[i]));
            total += pc;
            i += 1;
        }
        const mask: u64 = if (bit_idx == 63) ~@as(u64, 0) else (@as(u64, 2) << bit_idx) - 1;
        const last_pc: u32 = @intCast(@popCount(_.words[word_idx] & mask));
        return total + last_pc;
    }

    pub fn minimum(): u16? {
        let i: usize = 0;
        while (i < BITSET_WORDS) {
            const w = _.words[i];
            if (w != 0) {
                const tz: usize = @intCast(@ctz(w));
                return @intCast(i * 64 + tz);
            }
            i += 1;
        }
        return null;
    }

    pub fn maximum(): u16? {
        let i: usize = BITSET_WORDS;
        while (i > 0) {
            i -= 1;
            const w = _.words[i];
            if (w != 0) {
                const lz: usize = @intCast(@clz(w));
                const bit: usize = 63 - lz;
                return @intCast(i * 64 + bit);
            }
        }
        return null;
    }

    pub fn toArray(alloc: std.mem.Allocator): !ArrayContainer {
        let arr = ArrayContainer.init();
        errdefer arr.deinit(alloc);
        try arr.data.ensureTotalCapacity(alloc, _.card);
        let i: usize = 0;
        while (i < BITSET_WORDS) {
            let w = _.words[i];
            while (w != 0) {
                const tz: usize = @intCast(@ctz(w));
                const v: u16 = @intCast(i * 64 + tz);
                try arr.data.append(alloc, v);
                w &= w - 1;
            }
            i += 1;
        }
        return arr;
    }
};

pub const Container = union(enum) {
    array: ArrayContainer,
    bitset: BitsetContainer,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .array => |*arr| arr.deinit(alloc),
            .bitset => |*bs| bs.deinit(alloc),
        }
    }

    pub fn clone(alloc: std.mem.Allocator): !Container {
        switch (_) {
            .array => |arr| return .{ .array = try arr.clone(alloc) },
            .bitset => |bs| return .{ .bitset = try bs.clone(alloc) },
        }
    }

    pub fn cardinality(): u32 {
        switch (_) {
            .array => |arr| return arr.cardinality(),
            .bitset => |bs| return bs.cardinality(),
        }
    }

    pub fn contains(val: u16): bool {
        switch (_) {
            .array => |arr| return arr.contains(val),
            .bitset => |bs| return bs.contains(val),
        }
    }

    pub fn rank(val: u16): u32 {
        switch (_) {
            .array => |arr| return arr.rank(val),
            .bitset => |bs| return bs.rank(val),
        }
    }

    pub fn minimum(): u16? {
        switch (_) {
            .array => |arr| return arr.minimum(),
            .bitset => |bs| return bs.minimum(),
        }
    }

    pub fn maximum(): u16? {
        switch (_) {
            .array => |arr| return arr.maximum(),
            .bitset => |bs| return bs.maximum(),
        }
    }
};

pub const Bucket = struct {
    key: u16,
    container: Container,
};

fn unionContainers(alloc: std.mem.Allocator, a: Container, b: Container): !Container {
    switch (a) {
        .array => |arr_a| {
            switch (b) {
                .array => |arr_b| {
                    let out = ArrayContainer.init();
                    errdefer out.deinit(alloc);
                    const items_a = arr_a.data.items;
                    const items_b = arr_b.data.items;
                    let ia: usize = 0;
                    let ib: usize = 0;
                    while (ia < items_a.len and ib < items_b.len) {
                        const va = items_a[ia];
                        const vb = items_b[ib];
                        if (va < vb) {
                            try out.data.append(alloc, va);
                            ia += 1;
                        } else if (vb < va) {
                            try out.data.append(alloc, vb);
                            ib += 1;
                        } else {
                            try out.data.append(alloc, va);
                            ia += 1;
                            ib += 1;
                        }
                    }
                    while (ia < items_a.len) {
                        try out.data.append(alloc, items_a[ia]);
                        ia += 1;
                    }
                    while (ib < items_b.len) {
                        try out.data.append(alloc, items_b[ib]);
                        ib += 1;
                    }
                    if (out.data.items.len > ARRAY_MAX_SIZE) {
                        const bs = try out.toBitset(alloc);
                        out.deinit(alloc);
                        return .{ .bitset = bs };
                    }
                    return .{ .array = out };
                },
                .bitset => |bs_b| {
                    let bs = try bs_b.clone(alloc);
                    errdefer bs.deinit(alloc);
                    for (v of arr_a.data.items) {
                        * = bs.add(v);
                    }
                    return .{ .bitset = bs };
                },
            }
        },
        .bitset => |bs_a| {
            switch (b) {
                .array => |arr_b| {
                    let bs = try bs_a.clone(alloc);
                    errdefer bs.deinit(alloc);
                    for (v of arr_b.data.items) {
                        * = bs.add(v);
                    }
                    return .{ .bitset = bs };
                },
                .bitset => |bs_b| {
                    let bs = try BitsetContainer.init(alloc);
                    errdefer bs.deinit(alloc);
                    let i: usize = 0;
                    while (i < BITSET_WORDS) {
                        bs.words[i] = bs_a.words[i] | bs_b.words[i];
                        i += 1;
                    }
                    bs.recomputeCardinality();
                    return .{ .bitset = bs };
                },
            }
        },
    }
}

fn intersectContainers(alloc: std.mem.Allocator, a: Container, b: Container): !Container {
    switch (a) {
        .array => |arr_a| {
            switch (b) {
                .array => |arr_b| {
                    let out = ArrayContainer.init();
                    errdefer out.deinit(alloc);
                    const items_a = arr_a.data.items;
                    const items_b = arr_b.data.items;
                    let ia: usize = 0;
                    let ib: usize = 0;
                    while (ia < items_a.len and ib < items_b.len) {
                        const va = items_a[ia];
                        const vb = items_b[ib];
                        if (va < vb) {
                            ia += 1;
                        } else if (vb < va) {
                            ib += 1;
                        } else {
                            try out.data.append(alloc, va);
                            ia += 1;
                            ib += 1;
                        }
                    }
                    return .{ .array = out };
                },
                .bitset => |bs_b| {
                    let out = ArrayContainer.init();
                    errdefer out.deinit(alloc);
                    for (v of arr_a.data.items) {
                        if (bs_b.contains(v)) {
                            try out.data.append(alloc, v);
                        }
                    }
                    return .{ .array = out };
                },
            }
        },
        .bitset => |bs_a| {
            switch (b) {
                .array => |arr_b| {
                    let out = ArrayContainer.init();
                    errdefer out.deinit(alloc);
                    for (v of arr_b.data.items) {
                        if (bs_a.contains(v)) {
                            try out.data.append(alloc, v);
                        }
                    }
                    return .{ .array = out };
                },
                .bitset => |bs_b| {
                    let bs = try BitsetContainer.init(alloc);
                    errdefer bs.deinit(alloc);
                    let i: usize = 0;
                    while (i < BITSET_WORDS) {
                        bs.words[i] = bs_a.words[i] & bs_b.words[i];
                        i += 1;
                    }
                    bs.recomputeCardinality();
                    if (bs.card == 0) {
                        bs.deinit(alloc);
                        return .{ .array = ArrayContainer.init() };
                    }
                    if (bs.card <= ARRAY_MAX_SIZE) {
                        const arr = try bs.toArray(alloc);
                        bs.deinit(alloc);
                        return .{ .array = arr };
                    }
                    return .{ .bitset = bs };
                },
            }
        },
    }
}

fn differenceContainers(alloc: std.mem.Allocator, a: Container, b: Container): !Container {
    switch (a) {
        .array => |arr_a| {
            switch (b) {
                .array => |arr_b| {
                    let out = ArrayContainer.init();
                    errdefer out.deinit(alloc);
                    const items_a = arr_a.data.items;
                    const items_b = arr_b.data.items;
                    let ia: usize = 0;
                    let ib: usize = 0;
                    while (ia < items_a.len and ib < items_b.len) {
                        const va = items_a[ia];
                        const vb = items_b[ib];
                        if (va < vb) {
                            try out.data.append(alloc, va);
                            ia += 1;
                        } else if (vb < va) {
                            ib += 1;
                        } else {
                            ia += 1;
                            ib += 1;
                        }
                    }
                    while (ia < items_a.len) {
                        try out.data.append(alloc, items_a[ia]);
                        ia += 1;
                    }
                    return .{ .array = out };
                },
                .bitset => |bs_b| {
                    let out = ArrayContainer.init();
                    errdefer out.deinit(alloc);
                    for (v of arr_a.data.items) {
                        if (!bs_b.contains(v)) {
                            try out.data.append(alloc, v);
                        }
                    }
                    return .{ .array = out };
                },
            }
        },
        .bitset => |bs_a| {
            switch (b) {
                .array => |arr_b| {
                    let bs = try bs_a.clone(alloc);
                    errdefer bs.deinit(alloc);
                    for (v of arr_b.data.items) {
                        * = bs.remove(v);
                    }
                    if (bs.card == 0) {
                        bs.deinit(alloc);
                        return .{ .array = ArrayContainer.init() };
                    }
                    if (bs.card <= ARRAY_MAX_SIZE) {
                        const arr = try bs.toArray(alloc);
                        bs.deinit(alloc);
                        return .{ .array = arr };
                    }
                    return .{ .bitset = bs };
                },
                .bitset => |bs_b| {
                    let bs = try BitsetContainer.init(alloc);
                    errdefer bs.deinit(alloc);
                    let i: usize = 0;
                    while (i < BITSET_WORDS) {
                        bs.words[i] = bs_a.words[i] & ~bs_b.words[i];
                        i += 1;
                    }
                    bs.recomputeCardinality();
                    if (bs.card == 0) {
                        bs.deinit(alloc);
                        return .{ .array = ArrayContainer.init() };
                    }
                    if (bs.card <= ARRAY_MAX_SIZE) {
                        const arr = try bs.toArray(alloc);
                        bs.deinit(alloc);
                        return .{ .array = arr };
                    }
                    return .{ .bitset = bs };
                },
            }
        },
    }
}

pub const RoaringBitmap = struct {
    alloc: std.mem.Allocator,
    buckets: std.ArrayList(Bucket),

    pub static fn init(alloc: std.mem.Allocator): RoaringBitmap {
        return .{
            .alloc = alloc,
            .buckets = std.ArrayList(Bucket).empty,
        };
    }

    pub fn deinit(*_) {
        for (*b of _.buckets.items) {
            b.container.deinit(_.alloc);
        }
        _.buckets.deinit(_.alloc);
    }

    fn findBucket(key: u16): usize? {
        let lo: usize = 0;
        let hi: usize = _.buckets.items.len;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            const k = _.buckets.items[mid].key;
            if (k == key) {
                return mid;
            } else if (k < key) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return null;
    }

    fn lowerBoundBucket(key: u16): usize {
        let lo: usize = 0;
        let hi: usize = _.buckets.items.len;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            if (_.buckets.items[mid].key < key) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    pub fn add(*_, val: u32): !bool {
        const key: u16 = @intCast(val >> 16);
        const low: u16 = @intCast(val & 0xFFFF);
        const idx = _.lowerBoundBucket(key);
        if (idx < _.buckets.items.len and _.buckets.items[idx].key == key) {
            switch (_.buckets.items[idx].container) {
                .array => |*arr| {
                    const inserted = try arr.add(_.alloc, low);
                    if (inserted and arr.data.items.len > ARRAY_MAX_SIZE) {
                        const bs = try arr.toBitset(_.alloc);
                        arr.deinit(_.alloc);
                        _.buckets.items[idx].container = .{ .bitset = bs };
                    }
                    return inserted;
                },
                .bitset => |*bs| {
                    return bs.add(low);
                },
            }
        }
        let arr = ArrayContainer.init();
        errdefer arr.deinit(_.alloc);
        * = try arr.add(_.alloc, low);
        try _.buckets.insert(_.alloc, idx, .{
            .key = key,
            .container = .{ .array = arr },
        });
        return true;
    }

    pub fn remove(*_, val: u32): !bool {
        const key: u16 = @intCast(val >> 16);
        const low: u16 = @intCast(val & 0xFFFF);
        if (idx := _.findBucket(key)) {
            let should_remove_bucket = false;
            let removed = false;
            switch (_.buckets.items[idx].container) {
                .array => |*arr| {
                    removed = arr.remove(low);
                    if (removed and arr.data.items.len == 0) {
                        should_remove_bucket = true;
                    }
                },
                .bitset => |*bs| {
                    removed = bs.remove(low);
                    if (removed) {
                        if (bs.card == 0) {
                            should_remove_bucket = true;
                        } else if (bs.card <= ARRAY_MAX_SIZE) {
                            const arr = try bs.toArray(_.alloc);
                            bs.deinit(_.alloc);
                            _.buckets.items[idx].container = .{ .array = arr };
                        }
                    }
                },
            }
            if (should_remove_bucket) {
                let removed_bucket = _.buckets.orderedRemove(idx);
                removed_bucket.container.deinit(_.alloc);
            }
            return removed;
        }
        return false;
    }

    pub fn contains(val: u32): bool {
        const key: u16 = @intCast(val >> 16);
        const low: u16 = @intCast(val & 0xFFFF);
        if (idx := _.findBucket(key)) {
            return _.buckets.items[idx].container.contains(low);
        }
        return false;
    }

    pub fn cardinality(): u64 {
        let total: u64 = 0;
        for (b of _.buckets.items) {
            total += @as(u64, b.container.cardinality());
        }
        return total;
    }

    pub fn rank(val: u32): u64 {
        const key: u16 = @intCast(val >> 16);
        const low: u16 = @intCast(val & 0xFFFF);
        let total: u64 = 0;
        for (b of _.buckets.items) {
            if (b.key < key) {
                total += @as(u64, b.container.cardinality());
            } else if (b.key == key) {
                total += @as(u64, b.container.rank(low));
                break;
            } else {
                break;
            }
        }
        return total;
    }

    pub fn minimum(): u32? {
        if (_.buckets.items.len == 0) return null;
        const b = _.buckets.items[0];
        if (low := b.container.minimum()) {
            return (@as(u32, b.key) << 16) | @as(u32, low);
        }
        return null;
    }

    pub fn maximum(): u32? {
        if (_.buckets.items.len == 0) return null;
        const b = _.buckets.items[_.buckets.items.len - 1];
        if (low := b.container.maximum()) {
            return (@as(u32, b.key) << 16) | @as(u32, low);
        }
        return null;
    }

    pub fn toArray(alloc: std.mem.Allocator): ![]u32 {
        let out = std.ArrayList(u32).empty;
        errdefer out.deinit(alloc);
        const total = _.cardinality();
        try out.ensureTotalCapacity(alloc, @intCast(total));
        for (b of _.buckets.items) {
            const hi_shift: u32 = @as(u32, b.key) << 16;
            switch (b.container) {
                .array => |arr| {
                    for (low of arr.data.items) {
                        try out.append(alloc, hi_shift | @as(u32, low));
                    }
                },
                .bitset => |bs| {
                    let i: usize = 0;
                    while (i < BITSET_WORDS) {
                        let w = bs.words[i];
                        while (w != 0) {
                            const tz: usize = @intCast(@ctz(w));
                            const low: u32 = @intCast(i * 64 + tz);
                            try out.append(alloc, hi_shift | low);
                            w &= w - 1;
                        }
                        i += 1;
                    }
                },
            }
        }
        return try out.toOwnedSlice(alloc);
    }

    pub fn runOptimize(*_): !bool {
        let changed = false;
        let i: usize = 0;
        while (i < _.buckets.items.len) {
            const card = _.buckets.items[i].container.cardinality();
            if (card == 0) {
                let removed = _.buckets.orderedRemove(i);
                removed.container.deinit(_.alloc);
                changed = true;
                continue;
            }
            switch (_.buckets.items[i].container) {
                .array => |*arr| {
                    if (arr.data.items.len > ARRAY_MAX_SIZE) {
                        const bs = try arr.toBitset(_.alloc);
                        arr.deinit(_.alloc);
                        _.buckets.items[i].container = .{ .bitset = bs };
                        changed = true;
                    } else {
                        arr.data.shrinkAndFree(_.alloc, arr.data.items.len);
                    }
                },
                .bitset => |*bs| {
                    if (bs.card <= ARRAY_MAX_SIZE) {
                        const arr = try bs.toArray(_.alloc);
                        bs.deinit(_.alloc);
                        _.buckets.items[i].container = .{ .array = arr };
                        changed = true;
                    }
                },
            }
            i += 1;
        }
        return changed;
    }

    pub static fn unionWith(alloc: std.mem.Allocator, a: RoaringBitmap, b: RoaringBitmap): !RoaringBitmap {
        let res = RoaringBitmap.init(alloc);
        errdefer res.deinit();
        let ia: usize = 0;
        let ib: usize = 0;
        const ba = a.buckets.items;
        const bb = b.buckets.items;
        while (ia < ba.len and ib < bb.len) {
            const ka = ba[ia].key;
            const kb = bb[ib].key;
            if (ka < kb) {
                const c = try ba[ia].container.clone(alloc);
                try res.buckets.append(alloc, .{ .key = ka, .container = c });
                ia += 1;
            } else if (kb < ka) {
                const c = try bb[ib].container.clone(alloc);
                try res.buckets.append(alloc, .{ .key = kb, .container = c });
                ib += 1;
            } else {
                const c = try unionContainers(alloc, ba[ia].container, bb[ib].container);
                try res.buckets.append(alloc, .{ .key = ka, .container = c });
                ia += 1;
                ib += 1;
            }
        }
        while (ia < ba.len) {
            const c = try ba[ia].container.clone(alloc);
            try res.buckets.append(alloc, .{ .key = ba[ia].key, .container = c });
            ia += 1;
        }
        while (ib < bb.len) {
            const c = try bb[ib].container.clone(alloc);
            try res.buckets.append(alloc, .{ .key = bb[ib].key, .container = c });
            ib += 1;
        }
        return res;
    }

    pub static fn intersectWith(alloc: std.mem.Allocator, a: RoaringBitmap, b: RoaringBitmap): !RoaringBitmap {
        let res = RoaringBitmap.init(alloc);
        errdefer res.deinit();
        let ia: usize = 0;
        let ib: usize = 0;
        const ba = a.buckets.items;
        const bb = b.buckets.items;
        while (ia < ba.len and ib < bb.len) {
            const ka = ba[ia].key;
            const kb = bb[ib].key;
            if (ka < kb) {
                ia += 1;
            } else if (kb < ka) {
                ib += 1;
            } else {
                let c = try intersectContainers(alloc, ba[ia].container, bb[ib].container);
                if (c.cardinality() > 0) {
                    try res.buckets.append(alloc, .{ .key = ka, .container = c });
                } else {
                    c.deinit(alloc);
                }
                ia += 1;
                ib += 1;
            }
        }
        return res;
    }

    pub static fn differenceWith(alloc: std.mem.Allocator, a: RoaringBitmap, b: RoaringBitmap): !RoaringBitmap {
        let res = RoaringBitmap.init(alloc);
        errdefer res.deinit();
        let ia: usize = 0;
        let ib: usize = 0;
        const ba = a.buckets.items;
        const bb = b.buckets.items;
        while (ia < ba.len and ib < bb.len) {
            const ka = ba[ia].key;
            const kb = bb[ib].key;
            if (ka < kb) {
                const c = try ba[ia].container.clone(alloc);
                try res.buckets.append(alloc, .{ .key = ka, .container = c });
                ia += 1;
            } else if (kb < ka) {
                ib += 1;
            } else {
                let c = try differenceContainers(alloc, ba[ia].container, bb[ib].container);
                if (c.cardinality() > 0) {
                    try res.buckets.append(alloc, .{ .key = ka, .container = c });
                } else {
                    c.deinit(alloc);
                }
                ia += 1;
                ib += 1;
            }
        }
        while (ia < ba.len) {
            const c = try ba[ia].container.clone(alloc);
            try res.buckets.append(alloc, .{ .key = ba[ia].key, .container = c });
            ia += 1;
        }
        return res;
    }

    pub fn toOwnedSlice(alloc: std.mem.Allocator): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);
        try out.appendSlice(alloc, &SERIAL_MAGIC);
        let count_buf: [4]u8 = undefined;
        std.mem.writeInt(u32, &count_buf, @intCast(_.buckets.items.len), .little);
        try out.appendSlice(alloc, &count_buf);

        for (b of _.buckets.items) {
            let key_buf: [2]u8 = undefined;
            std.mem.writeInt(u16, &key_buf, b.key, .little);
            try out.appendSlice(alloc, &key_buf);
            switch (b.container) {
                .array => |arr| {
                    try out.append(alloc, 0);
                    let len_buf: [2]u8 = undefined;
                    std.mem.writeInt(u16, &len_buf, @intCast(arr.data.items.len), .little);
                    try out.appendSlice(alloc, &len_buf);
                    for (v of arr.data.items) {
                        let v_buf: [2]u8 = undefined;
                        std.mem.writeInt(u16, &v_buf, v, .little);
                        try out.appendSlice(alloc, &v_buf);
                    }
                },
                .bitset => |bs| {
                    try out.append(alloc, 1);
                    let card_buf: [4]u8 = undefined;
                    std.mem.writeInt(u32, &card_buf, bs.card, .little);
                    try out.appendSlice(alloc, &card_buf);
                    let wi: usize = 0;
                    while (wi < BITSET_WORDS) {
                        let w_buf: [8]u8 = undefined;
                        std.mem.writeInt(u64, &w_buf, bs.words[wi], .little);
                        try out.appendSlice(alloc, &w_buf);
                        wi += 1;
                    }
                },
            }
        }
        return try out.toOwnedSlice(alloc);
    }

    pub static fn fromSlice(alloc: std.mem.Allocator, data: []const u8): !RoaringBitmap {
        if (data.len < 8) return RoaringError.InvalidFormat;
        if (!std.mem.eql(u8, data[0..4], &SERIAL_MAGIC)) return RoaringError.InvalidFormat;
        const num_buckets = std.mem.readInt(u32, data[4..8], .little);
        let pos: usize = 8;

        let res = RoaringBitmap.init(alloc);
        errdefer res.deinit();
        try res.buckets.ensureTotalCapacity(alloc, @intCast(num_buckets));

        let bi: u32 = 0;
        while (bi < num_buckets) {
            if (pos + 3 > data.len) return RoaringError.InvalidFormat;
            const key = std.mem.readInt(u16, data[pos..][0..2], .little);
            const kind = data[pos + 2];
            pos += 3;

            if (kind == 0) {
                if (pos + 2 > data.len) return RoaringError.InvalidFormat;
                const count = std.mem.readInt(u16, data[pos..][0..2], .little);
                pos += 2;
                const byte_len: usize = @as(usize, count) * 2;
                if (pos + byte_len > data.len) return RoaringError.InvalidFormat;
                let arr = ArrayContainer.init();
                errdefer arr.deinit(alloc);
                try arr.data.ensureTotalCapacity(alloc, count);
                let ci: u16 = 0;
                while (ci < count) {
                    const v = std.mem.readInt(u16, data[pos..][0..2], .little);
                    try arr.data.append(alloc, v);
                    pos += 2;
                    ci += 1;
                }
                try res.buckets.append(alloc, .{ .key = key, .container = .{ .array = arr } });
            } else if (kind == 1) {
                if (pos + 4 + BITSET_WORDS * 8 > data.len) return RoaringError.InvalidFormat;
                const card = std.mem.readInt(u32, data[pos..][0..4], .little);
                pos += 4;
                let bs = try BitsetContainer.init(alloc);
                errdefer bs.deinit(alloc);
                let wi: usize = 0;
                while (wi < BITSET_WORDS) {
                    bs.words[wi] = std.mem.readInt(u64, data[pos..][0..8], .little);
                    pos += 8;
                    wi += 1;
                }
                bs.card = card;
                try res.buckets.append(alloc, .{ .key = key, .container = .{ .bitset = bs } });
            } else {
                return RoaringError.InvalidFormat;
            }
            bi += 1;
        }
        if (pos != data.len) return RoaringError.InvalidFormat;
        return res;
    }
};
