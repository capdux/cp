// Source: native-cp-stdlib (Google Maglev Consistent Hashing)

const std = @import("std");

pub const DEFAULT_TABLE_SIZE: usize = 65537;

const EMPTY_SLOT: usize = std.math.maxInt(usize);

export error MaglevError {
    EmptyBackends,
    InvalidTableSize,
    BackendNotFound,
    DuplicateBackend,
    OutOfMemory,
}

export struct Backend {
    name: []const u8,
    weight: usize,
}

fn isPrime(n: usize) -> bool {
    if (n < 2) {
        return false;
    }
    if (n == 2 || n == 3) {
        return true;
    }
    if (n % 2 == 0 || n % 3 == 0) {
        return false;
    }
    let i: usize = 5;
    while (i * i <= n) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
        i += 6;
    }
    return true;
}

fn mix64(val: u64) -> u64 {
    let z: u64 = val;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    z = z ^ (z >> 31);
    return z;
}

fn hash1(data: []const u8) -> u64 {
    let h: u64 = 14695981039346656037;
    for (b of data) {
        h = h ^ @as(u64, b);
        h = h *% 1099511628211;
    }
    return mix64(h);
}

fn hash2(data: []const u8) -> u64 {
    let h: u64 = 0x9e3779b97f4a7c15;
    for (b of data) {
        h = h *% 1315423911;
        h = h ^ (@as(u64, b) +% 0x517cc1b727220a95);
    }
    return mix64(h ^ @as(u64, data.len));
}

export class MaglevRouter {
    alloc: std.mem.Allocator,
    table_size: usize,
    backends: std.ArrayList(Backend),
    table: []usize,

    pub static fn init(alloc: std.mem.Allocator, table_size: usize) -> MaglevError!MaglevRouter {
        if (!isPrime(table_size)) {
            return MaglevError.InvalidTableSize;
        }
        let tbl = alloc.alloc(usize, table_size) catch return MaglevError.OutOfMemory;
        for (i of 0..table_size) {
            tbl[i] = EMPTY_SLOT;
        }
        return MaglevRouter{
            alloc: alloc,
            table_size: table_size,
            backends: std.ArrayList(Backend).empty,
            table: tbl,
        };
    }

    pub fn deinit(*_) {
        for (b of _.backends.items) {
            _.alloc.free(b.name);
        }
        _.backends.deinit(_.alloc);
        _.alloc.free(_.table);
    }

    pub fn backendCount() -> usize {
        return _.backends.items.len;
    }

    pub fn tableSize() -> usize {
        return _.table_size;
    }

    pub fn addBackend(*_, name: []const u8, weight: usize) -> MaglevError!void {
        for (b of _.backends.items) {
            if (std.mem.eql(u8, b.name, name)) {
                return MaglevError.DuplicateBackend;
            }
        }
        const owned_name = _.alloc.dupe(u8, name) catch return MaglevError.OutOfMemory;
        const w: usize = if (weight == 0) 1 else weight;
        _.backends.append(_.alloc, Backend{
            name: owned_name,
            weight: w,
        }) catch {
            _.alloc.free(owned_name);
            return MaglevError.OutOfMemory;
        };
        try _.rebuildInternal();
    }

    pub fn removeBackend(*_, name: []const u8) -> MaglevError!void {
        let found_idx: usize? = null;
        for (i of 0.._.backends.items.len) {
            if (std.mem.eql(u8, _.backends.items[i].name, name)) {
                found_idx = i;
                break;
            }
        }
        if (idx := found_idx) {
            const removed = _.backends.orderedRemove(idx);
            _.alloc.free(removed.name);
            if (_.backends.items.len == 0) {
                for (i of 0.._.table_size) {
                    _.table[i] = EMPTY_SLOT;
                }
                return;
            }
            try _.rebuildInternal();
        } else {
            return MaglevError.BackendNotFound;
        }
    }

    pub fn rebuild(*_) -> MaglevError!void {
        if (_.backends.items.len == 0) {
            return MaglevError.EmptyBackends;
        }
        try _.rebuildInternal();
    }

    fn rebuildInternal(*_) -> MaglevError!void {
        const n = _.backends.items.len;
        if (n == 0) {
            return MaglevError.EmptyBackends;
        }
        const m = _.table_size;
        for (i of 0..m) {
            _.table[i] = EMPTY_SLOT;
        }

        let offsets = _.alloc.alloc(usize, n) catch return MaglevError.OutOfMemory;
        defer _.alloc.free(offsets);
        let skips = _.alloc.alloc(usize, n) catch return MaglevError.OutOfMemory;
        defer _.alloc.free(skips);
        let next_pos = _.alloc.alloc(usize, n) catch return MaglevError.OutOfMemory;
        defer _.alloc.free(next_pos);

        const m_u64: u64 = @intCast(m);
        const m_minus_1_u64: u64 = @intCast(m - 1);

        for (i of 0..n) {
            const b_name = _.backends.items[i].name;
            const off_idx: usize = @intCast(hash1(b_name) % m_u64);
            const skip_idx: usize = @intCast((hash2(b_name) % m_minus_1_u64) + 1);
            offsets[i] = off_idx;
            skips[i] = skip_idx;
            next_pos[i] = 0;
        }

        let filled: usize = 0;
        while (filled < m) {
            for (i of 0..n) {
                const w = _.backends.items[i].weight;
                let turn: usize = 0;
                while (turn < w && filled < m) {
                    let cur_j = next_pos[i];
                    let cand = (offsets[i] + cur_j * skips[i]) % m;
                    while (_.table[cand] != EMPTY_SLOT) {
                        cur_j += 1;
                        cand = (offsets[i] + cur_j * skips[i]) % m;
                    }
                    _.table[cand] = i;
                    next_pos[i] = cur_j + 1;
                    filled += 1;
                    turn += 1;
                }
                if (filled >= m) {
                    break;
                }
            }
        }
    }

    pub fn lookup(key: []const u8) -> MaglevError![]const u8 {
        if (_.backends.items.len == 0 || _.table.len == 0) {
            return MaglevError.EmptyBackends;
        }
        const m_u64: u64 = @intCast(_.table_size);
        const slot_idx: usize = @intCast(hash1(key) % m_u64);
        const backend_idx = _.table[slot_idx];
        if (backend_idx >= _.backends.items.len) {
            return MaglevError.EmptyBackends;
        }
        return _.backends.items[backend_idx].name;
    }
}
