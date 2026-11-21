// Source: native-cp-stdlib (State-Based CRDTs: GCounter, PNCounter, GSet, LWW)
const std = @import("std");

pub struct CounterEntry {
    node_id: u64,
    count: u64,
}

/// Grow-Only Counter (State-Based CRDT).
/// Supports monotonic increments per replica and associative, commutative,
/// idempotent join-semilattice merge via element-wise maximum.
pub struct GCounter {
    alloc: std.mem.Allocator,
    entries: std.ArrayList(CounterEntry),

    pub fn init(alloc: std.mem.Allocator): GCounter {
        return GCounter{
            .alloc = alloc,
            .entries = std.ArrayList(CounterEntry).empty,
        };
    }

    pub fn deinit(*_) {
        _.entries.deinit(_.alloc);
    }

    pub fn increment(*_, node_id: u64, delta: u64): !void {
        if (delta == 0) return;
        let i: usize = 0;
        while (i < _.entries.items.len) {
            if (_.entries.items[i].node_id == node_id) {
                _.entries.items[i].count = _.entries.items[i].count +% delta;
                return;
            }
            i = i + 1;
        }
        try _.entries.append(_.alloc, CounterEntry{
            .node_id = node_id,
            .count = delta,
        });
    }

    pub fn value(): u64 {
        let total: u64 = 0;
        for (entry of _.entries.items) {
            total = total +% entry.count;
        }
        return total;
    }

    pub fn merge(*_, other: GCounter): !void {
        for (other_entry of other.entries.items) {
            let found = false;
            let i: usize = 0;
            while (i < _.entries.items.len) {
                if (_.entries.items[i].node_id == other_entry.node_id) {
                    if (other_entry.count > _.entries.items[i].count) {
                        _.entries.items[i].count = other_entry.count;
                    }
                    found = true;
                    break;
                }
                i = i + 1;
            }
            if (!found) {
                try _.entries.append(_.alloc, other_entry);
            }
        }
    }
}

/// Positive-Negative Counter (State-Based CRDT).
/// Implemented as a pair of GCounters (pos, neg) supporting both increments
/// and decrements with eventual consistency across partitions.
pub struct PNCounter {
    pos: GCounter,
    neg: GCounter,

    pub fn init(alloc: std.mem.Allocator): PNCounter {
        return PNCounter{
            .pos = GCounter.init(alloc),
            .neg = GCounter.init(alloc),
        };
    }

    pub fn deinit(*_) {
        _.pos.deinit();
        _.neg.deinit();
    }

    pub fn increment(*_, node_id: u64, delta: u64): !void {
        try _.pos.increment(node_id, delta);
    }

    pub fn decrement(*_, node_id: u64, delta: u64): !void {
        try _.neg.increment(node_id, delta);
    }

    pub fn value(): i64 {
        const p: i64 = @intCast(_.pos.value());
        const n: i64 = @intCast(_.neg.value());
        return p - n;
    }

    pub fn merge(*_, other: PNCounter): !void {
        try _.pos.merge(other.pos);
        try _.neg.merge(other.neg);
    }
}

/// Last-Writer-Wins Register (State-Based CRDT).
/// Resolves concurrent writes deterministically using total order (ts, node_id).
pub fn LWWRegister(comptime T: type): type {
    return struct {
        val: T,
        ts: i64,
        node_id: u64,

        const Self = @This();

        pub fn init(val: T, ts: i64, node_id: u64): Self {
            return Self{
                .val = val,
                .ts = ts,
                .node_id = node_id,
            };
        }

        pub fn set(*_, val: T, ts: i64, node_id: u64) {
            if (ts > _.ts or (ts == _.ts and node_id > _.node_id)) {
                _.val = val;
                _.ts = ts;
                _.node_id = node_id;
            }
        }

        pub fn get(): T {
            return _.val;
        }

        pub fn merge(*_, other: Self) {
            if (other.ts > _.ts or (other.ts == _.ts and other.node_id > _.node_id)) {
                _.val = other.val;
                _.ts = other.ts;
                _.node_id = other.node_id;
            }
        }
    };
}

/// Grow-Only Set (State-Based CRDT).
/// Converges by set union without duplicates.
pub fn GSet(comptime T: type): type {
    return struct {
        alloc: std.mem.Allocator,
        items: std.ArrayList(T),

        const Self = @This();

        pub fn init(alloc: std.mem.Allocator): Self {
            return Self{
                .alloc = alloc,
                .items = std.ArrayList(T).empty,
            };
        }

        pub fn deinit(*_) {
            _.items.deinit(_.alloc);
        }

        pub fn contains(item: T): bool {
            for (elem of _.items.items) {
                if (std.meta.eql(elem, item)) {
                    return true;
                }
            }
            return false;
        }

        pub fn add(*_, item: T): !void {
            if (!_.contains(item)) {
                try _.items.append(_.alloc, item);
            }
        }

        pub fn len(): usize {
            return _.items.items.len;
        }

        pub fn merge(*_, other: Self): !void {
            for (elem of other.items.items) {
                try _.add(elem);
            }
        }
    };
}
