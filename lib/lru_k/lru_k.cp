// Source: native-cp-stdlib (Scan-Resistant LRU-K Cache)
const std = @import("std");

pub const LruKError = error{
    InvalidCapacity,
    InvalidK,
};

pub fn LruKCache(comptime K: type, comptime V: type): type {
    return struct {
        const Self = @This();

        const Node = struct {
            key: K,
            value: V,
            access_count: usize,
            in_cache: bool,
            prev: usize?,
            next: usize?,
        };

        alloc: std.mem.Allocator,
        capacity: usize,
        history_capacity: usize,
        k_threshold: usize,
        map: std.AutoHashMap(K, usize),
        nodes: std.ArrayList(Node),
        free_list: std.ArrayList(usize),
        cache_head: usize?,
        cache_tail: usize?,
        history_head: usize?,
        history_tail: usize?,
        cache_len: usize,
        history_len: usize,
        hit_count: u64,
        miss_count: u64,

        pub fn init(
            alloc: std.mem.Allocator,
            capacity: usize,
            history_capacity: usize,
            k_threshold: usize,
        ) -> !Self {
            if (capacity == 0 or history_capacity == 0) {
                return LruKError.InvalidCapacity;
            }
            if (k_threshold == 0) {
                return LruKError.InvalidK;
            }
            return Self{
                alloc: alloc,
                capacity: capacity,
                history_capacity: history_capacity,
                k_threshold: k_threshold,
                map: std.AutoHashMap(K, usize).init(alloc),
                nodes: std.ArrayList(Node).empty,
                free_list: std.ArrayList(usize).empty,
                cache_head: null,
                cache_tail: null,
                history_head: null,
                history_tail: null,
                cache_len: 0,
                history_len: 0,
                hit_count: 0,
                miss_count: 0,
            };
        }

        pub fn deinit(self: *Self) {
            self.map.deinit();
            self.nodes.deinit(self.alloc);
            self.free_list.deinit(self.alloc);
        }

        fn allocNode(self: *Self, key: K, value: V, in_cache: bool) -> !usize {
            if (self.free_list.items.len > 0) {
                const idx = self.free_list.pop().?;
                self.nodes.items[idx] = Node{
                    key: key,
                    value: value,
                    access_count: 1,
                    in_cache: in_cache,
                    prev: null,
                    next: null,
                };
                return idx;
            }
            const idx = self.nodes.items.len;
            try self.nodes.append(self.alloc, Node{
                key: key,
                value: value,
                access_count: 1,
                in_cache: in_cache,
                prev: null,
                next: null,
            });
            return idx;
        }

        fn releaseNode(self: *Self, idx: usize) {
            self.free_list.append(self.alloc, idx) catch {};
        }

        fn attachCacheHead(self: *Self, idx: usize) {
            self.nodes.items[idx].prev = null;
            self.nodes.items[idx].next = self.cache_head;
            if (old_head := self.cache_head) {
                self.nodes.items[old_head].prev = idx;
            } else {
                self.cache_tail = idx;
            }
            self.cache_head = idx;
            self.cache_len += 1;
        }

        fn detachCache(self: *Self, idx: usize) {
            const p = self.nodes.items[idx].prev;
            const n = self.nodes.items[idx].next;
            if (prev_idx := p) {
                self.nodes.items[prev_idx].next = n;
            } else {
                self.cache_head = n;
            }
            if (next_idx := n) {
                self.nodes.items[next_idx].prev = p;
            } else {
                self.cache_tail = p;
            }
            self.nodes.items[idx].prev = null;
            self.nodes.items[idx].next = null;
            self.cache_len -= 1;
        }

        fn moveToCacheHead(self: *Self, idx: usize) {
            if (head_idx := self.cache_head) {
                if (head_idx == idx) {
                    return;
                }
            }
            self.detachCache(idx);
            self.attachCacheHead(idx);
        }

        fn evictCacheTail(self: *Self) {
            if (tail_idx := self.cache_tail) {
                const key = self.nodes.items[tail_idx].key;
                self.detachCache(tail_idx);
                * = self.map.remove(key);
                self.releaseNode(tail_idx);
            }
        }

        fn attachHistoryHead(self: *Self, idx: usize) {
            self.nodes.items[idx].prev = null;
            self.nodes.items[idx].next = self.history_head;
            if (old_head := self.history_head) {
                self.nodes.items[old_head].prev = idx;
            } else {
                self.history_tail = idx;
            }
            self.history_head = idx;
            self.history_len += 1;
        }

        fn detachHistory(self: *Self, idx: usize) {
            const p = self.nodes.items[idx].prev;
            const n = self.nodes.items[idx].next;
            if (prev_idx := p) {
                self.nodes.items[prev_idx].next = n;
            } else {
                self.history_head = n;
            }
            if (next_idx := n) {
                self.nodes.items[next_idx].prev = p;
            } else {
                self.history_tail = p;
            }
            self.nodes.items[idx].prev = null;
            self.nodes.items[idx].next = null;
            self.history_len -= 1;
        }

        fn moveToHistoryHead(self: *Self, idx: usize) {
            if (head_idx := self.history_head) {
                if (head_idx == idx) {
                    return;
                }
            }
            self.detachHistory(idx);
            self.attachHistoryHead(idx);
        }

        fn evictHistoryTail(self: *Self) {
            if (tail_idx := self.history_tail) {
                const key = self.nodes.items[tail_idx].key;
                self.detachHistory(tail_idx);
                * = self.map.remove(key);
                self.releaseNode(tail_idx);
            }
        }

        fn promoteFromHistory(self: *Self, idx: usize) {
            self.detachHistory(idx);
            if (self.cache_len >= self.capacity) {
                self.evictCacheTail();
            }
            self.nodes.items[idx].in_cache = true;
            self.attachCacheHead(idx);
        }

        pub fn put(self: *Self, key: K, value: V) -> !void {
            if (existing_idx := self.map.get(key)) {
                self.nodes.items[existing_idx].value = value;
                self.nodes.items[existing_idx].access_count += 1;
                if (self.nodes.items[existing_idx].in_cache) {
                    self.moveToCacheHead(existing_idx);
                } else {
                    if (self.nodes.items[existing_idx].access_count >= self.k_threshold) {
                        self.promoteFromHistory(existing_idx);
                    } else {
                        self.moveToHistoryHead(existing_idx);
                    }
                }
                return;
            }

            if (self.k_threshold == 1) {
                if (self.cache_len >= self.capacity) {
                    self.evictCacheTail();
                }
                const idx = try self.allocNode(key, value, true);
                try self.map.put(key, idx);
                self.attachCacheHead(idx);
                return;
            }

            if (self.history_len >= self.history_capacity) {
                self.evictHistoryTail();
            }
            const idx = try self.allocNode(key, value, false);
            try self.map.put(key, idx);
            self.attachHistoryHead(idx);
        }

        pub fn get(self: *Self, key: K) -> V? {
            if (idx := self.map.get(key)) {
                self.hit_count = self.hit_count +% 1;
                self.nodes.items[idx].access_count += 1;
                if (self.nodes.items[idx].in_cache) {
                    self.moveToCacheHead(idx);
                } else {
                    if (self.nodes.items[idx].access_count >= self.k_threshold) {
                        self.promoteFromHistory(idx);
                    } else {
                        self.moveToHistoryHead(idx);
                    }
                }
                return self.nodes.items[idx].value;
            }
            self.miss_count = self.miss_count +% 1;
            return null;
        }

        pub fn peek(self, key: K) -> V? {
            if (idx := self.map.get(key)) {
                return self.nodes.items[idx].value;
            }
            return null;
        }

        pub fn remove(self: *Self, key: K) -> V? {
            if (idx := self.map.get(key)) {
                const val = self.nodes.items[idx].value;
                if (self.nodes.items[idx].in_cache) {
                    self.detachCache(idx);
                } else {
                    self.detachHistory(idx);
                }
                * = self.map.remove(key);
                self.releaseNode(idx);
                return val;
            }
            return null;
        }

        pub fn clear(self: *Self) {
            self.map.clearRetainingCapacity();
            self.nodes.clearRetainingCapacity();
            self.free_list.clearRetainingCapacity();
            self.cache_head = null;
            self.cache_tail = null;
            self.history_head = null;
            self.history_tail = null;
            self.cache_len = 0;
            self.history_len = 0;
            self.hit_count = 0;
            self.miss_count = 0;
        }

        pub fn len(self) -> usize {
            return self.cache_len;
        }

        pub fn historyLen(self) -> usize {
            return self.history_len;
        }

        pub fn hits(self) -> u64 {
            return self.hit_count;
        }

        pub fn misses(self) -> u64 {
            return self.miss_count;
        }
    };
}
