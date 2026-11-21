// Source: native-cp-stdlib (Ordered SkipList Key-Value Engine)

const std = @import("std");

pub fn SkipList(comptime K: type, comptime V: type) -> type {
    return struct {
        const Self = @This();
        pub const MAX_LEVEL: usize = 16;

        pub const Entry = struct {
            key: K,
            value: V,
        };

        const Node = struct {
            key: K,
            value: V,
            level: usize,
            forward: [MAX_LEVEL](*Node)?,
        };

        alloc: std.mem.Allocator,
        head_forward: [MAX_LEVEL](*Node)?,
        level: usize,
        size: usize,
        prng: std.Random.DefaultPrng,

        fn emptyForward() -> [MAX_LEVEL](*Node)? {
            let arr: [MAX_LEVEL](*Node)? = undefined;
            let i: usize = 0;
            while (i < MAX_LEVEL) {
                arr[i] = null;
                i += 1;
            }
            return arr;
        }

        pub fn init(alloc: std.mem.Allocator) -> Self {
            return Self{
                .alloc = alloc,
                .head_forward = emptyForward(),
                .level = 1,
                .size = 0,
                .prng = std.Random.DefaultPrng.init(0x9e3779b97f4a7c15),
            };
        }

        pub fn deinit(*self) {
            let iter_opt = self.head_forward[0];
            while (node_ptr := iter_opt) {
                const next_ptr = node_ptr.forward[0];
                self.alloc.destroy(node_ptr);
                iter_opt = next_ptr;
            }
            self.head_forward = emptyForward();
            self.level = 1;
            self.size = 0;
        }

        pub fn clear(*self) {
            self.deinit();
        }

        pub fn len(self) -> usize {
            return self.size;
        }

        pub fn isEmpty(self) -> bool {
            return self.size == 0;
        }

        fn compareKeys(a: K, b: K) -> std.math.Order {
            switch (@typeInfo(K)) {
                .pointer => |ptr| {
                    if (ptr.size == .slice and ptr.child == u8) {
                        return std.mem.order(u8, a, b);
                    }
                    @compileError("Unsupported pointer key type for SkipList");
                },
                .int, .float, .comptime_int, .comptime_float => {
                    return std.math.order(a, b);
                },
                else => @compileError("Unsupported key type for SkipList"),
            }
        }

        fn randomLevel(*self) -> usize {
            let lvl: usize = 1;
            let rand = self.prng.random();
            while (lvl < MAX_LEVEL and (rand.int(u8) & 3) == 0) {
                lvl += 1;
            }
            return lvl;
        }

        fn nextAt(self, curr: (*Node)?, lvl_idx: usize) -> (*Node)? {
            if (c := curr) {
                return c.forward[lvl_idx];
            }
            return self.head_forward[lvl_idx];
        }

        fn findPredecessor(self, key: K) -> (*Node)? {
            let curr: (*Node)? = null;
            let lvl_idx = self.level;
            while (lvl_idx > 0) {
                lvl_idx -= 1;
                while (true) {
                    const next_opt = self.nextAt(curr, lvl_idx);
                    if (nxt := next_opt) {
                        if (compareKeys(nxt.key, key) == .lt) {
                            curr = nxt;
                            continue;
                        }
                    }
                    break;
                }
            }
            return curr;
        }

        pub fn insert(*self, key: K, val: V) -> !V? {
            let update: [MAX_LEVEL](*Node)? = emptyForward();
            let curr: (*Node)? = null;
            let lvl_idx = self.level;
            while (lvl_idx > 0) {
                lvl_idx -= 1;
                while (true) {
                    const next_opt = self.nextAt(curr, lvl_idx);
                    if (nxt := next_opt) {
                        if (compareKeys(nxt.key, key) == .lt) {
                            curr = nxt;
                            continue;
                        }
                    }
                    break;
                }
                update[lvl_idx] = curr;
            }

            const candidate_opt = self.nextAt(curr, 0);
            if (cand := candidate_opt) {
                if (compareKeys(cand.key, key) == .eq) {
                    const old_val = cand.value;
                    cand.value = val;
                    return old_val;
                }
            }

            const new_lvl = self.randomLevel();
            if (new_lvl > self.level) {
                let i = self.level;
                while (i < new_lvl) {
                    update[i] = null;
                    i += 1;
                }
                self.level = new_lvl;
            }

            let new_node = try self.alloc.create(Node);
            new_node.* = Node{
                .key = key,
                .value = val,
                .level = new_lvl,
                .forward = emptyForward(),
            };

            let j: usize = 0;
            while (j < new_lvl) {
                if (prev_node := update[j]) {
                    new_node.forward[j] = prev_node.forward[j];
                    prev_node.forward[j] = new_node;
                } else {
                    new_node.forward[j] = self.head_forward[j];
                    self.head_forward[j] = new_node;
                }
                j += 1;
            }

            self.size += 1;
            return null;
        }

        pub fn upsert(*self, key: K, val: V) -> !V? {
            return self.insert(key, val);
        }

        pub fn get(self, key: K) -> V? {
            const curr = self.findPredecessor(key);
            const candidate_opt = self.nextAt(curr, 0);
            if (cand := candidate_opt) {
                if (compareKeys(cand.key, key) == .eq) {
                    return cand.value;
                }
            }
            return null;
        }

        pub fn contains(self, key: K) -> bool {
            return self.get(key) != null;
        }

        pub fn remove(*self, key: K) -> V? {
            let update: [MAX_LEVEL](*Node)? = emptyForward();
            let curr: (*Node)? = null;
            let lvl_idx = self.level;
            while (lvl_idx > 0) {
                lvl_idx -= 1;
                while (true) {
                    const next_opt = self.nextAt(curr, lvl_idx);
                    if (nxt := next_opt) {
                        if (compareKeys(nxt.key, key) == .lt) {
                            curr = nxt;
                            continue;
                        }
                    }
                    break;
                }
                update[lvl_idx] = curr;
            }

            const candidate_opt = self.nextAt(curr, 0);
            if (cand := candidate_opt) {
                if (compareKeys(cand.key, key) == .eq) {
                    const removed_val = cand.value;
                    let i: usize = 0;
                    while (i < self.level) {
                        if (prev_node := update[i]) {
                            if (prev_node.forward[i] == cand) {
                                prev_node.forward[i] = cand.forward[i];
                            }
                        } else {
                            if (self.head_forward[i] == cand) {
                                self.head_forward[i] = cand.forward[i];
                            }
                        }
                        i += 1;
                    }

                    while (self.level > 1 and self.head_forward[self.level - 1] == null) {
                        self.level -= 1;
                    }

                    self.alloc.destroy(cand);
                    self.size -= 1;
                    return removed_val;
                }
            }
            return null;
        }

        pub fn lowerBound(self, key: K) -> Entry? {
            const curr = self.findPredecessor(key);
            const candidate_opt = self.nextAt(curr, 0);
            if (cand := candidate_opt) {
                return Entry{
                    .key = cand.key,
                    .value = cand.value,
                };
            }
            return null;
        }

        pub fn ceiling(self, key: K) -> Entry? {
            return self.lowerBound(key);
        }

        pub fn floor(self, key: K) -> Entry? {
            const curr = self.findPredecessor(key);
            const candidate_opt = self.nextAt(curr, 0);
            if (cand := candidate_opt) {
                if (compareKeys(cand.key, key) == .eq) {
                    return Entry{
                        .key = cand.key,
                        .value = cand.value,
                    };
                }
            }
            if (c := curr) {
                return Entry{
                    .key = c.key,
                    .value = c.value,
                };
            }
            return null;
        }

        pub fn rangeQuery(self, alloc: std.mem.Allocator, start_key: K, end_key: K) -> ![]Entry {
            let out = std.ArrayList(Entry).empty;
            errdefer out.deinit(alloc);

            if (compareKeys(start_key, end_key) == .gt) {
                return try out.toOwnedSlice(alloc);
            }

            const curr = self.findPredecessor(start_key);
            let iter_opt = self.nextAt(curr, 0);
            while (node_ptr := iter_opt) {
                if (compareKeys(node_ptr.key, end_key) == .gt) {
                    break;
                }
                try out.append(alloc, Entry{
                    .key = node_ptr.key,
                    .value = node_ptr.value,
                });
                iter_opt = node_ptr.forward[0];
            }
            return try out.toOwnedSlice(alloc);
        }

        pub fn items(self, alloc: std.mem.Allocator) -> ![]Entry {
            let out = std.ArrayList(Entry).empty;
            errdefer out.deinit(alloc);

            let iter_opt = self.head_forward[0];
            while (node_ptr := iter_opt) {
                try out.append(alloc, Entry{
                    .key = node_ptr.key,
                    .value = node_ptr.value,
                });
                iter_opt = node_ptr.forward[0];
            }
            return try out.toOwnedSlice(alloc);
        }
    };
}
