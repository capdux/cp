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

        pub static fn init(
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

        pub fn deinit(*_) {
            _.map.deinit();
            _.nodes.deinit(_.alloc);
            _.free_list.deinit(_.alloc);
        }

        fn allocNode(*_, key: K, value: V, in_cache: bool) -> !usize {
            if (_.free_list.items.len > 0) {
                const idx = _.free_list.pop().?;
                _.nodes.items[idx] = Node{
                    key: key,
                    value: value,
                    access_count: 1,
                    in_cache: in_cache,
                    prev: null,
                    next: null,
                };
                return idx;
            }
            const idx = _.nodes.items.len;
            try _.nodes.append(_.alloc, Node{
                key: key,
                value: value,
                access_count: 1,
                in_cache: in_cache,
                prev: null,
                next: null,
            });
            return idx;
        }

        fn releaseNode(*_, idx: usize) {
            _.free_list.append(_.alloc, idx) catch {};
        }

        fn attachCacheHead(*_, idx: usize) {
            _.nodes.items[idx].prev = null;
            _.nodes.items[idx].next = _.cache_head;
            if (old_head := _.cache_head) {
                _.nodes.items[old_head].prev = idx;
            } else {
                _.cache_tail = idx;
            }
            _.cache_head = idx;
            _.cache_len += 1;
        }

        fn detachCache(*_, idx: usize) {
            const p = _.nodes.items[idx].prev;
            const n = _.nodes.items[idx].next;
            if (prev_idx := p) {
                _.nodes.items[prev_idx].next = n;
            } else {
                _.cache_head = n;
            }
            if (next_idx := n) {
                _.nodes.items[next_idx].prev = p;
            } else {
                _.cache_tail = p;
            }
            _.nodes.items[idx].prev = null;
            _.nodes.items[idx].next = null;
            _.cache_len -= 1;
        }

        fn moveToCacheHead(*_, idx: usize) {
            if (head_idx := _.cache_head) {
                if (head_idx == idx) {
                    return;
                }
            }
            _.detachCache(idx);
            _.attachCacheHead(idx);
        }

        fn evictCacheTail(*_) {
            if (tail_idx := _.cache_tail) {
                const key = _.nodes.items[tail_idx].key;
                _.detachCache(tail_idx);
                * = _.map.remove(key);
                _.releaseNode(tail_idx);
            }
        }

        fn attachHistoryHead(*_, idx: usize) {
            _.nodes.items[idx].prev = null;
            _.nodes.items[idx].next = _.history_head;
            if (old_head := _.history_head) {
                _.nodes.items[old_head].prev = idx;
            } else {
                _.history_tail = idx;
            }
            _.history_head = idx;
            _.history_len += 1;
        }

        fn detachHistory(*_, idx: usize) {
            const p = _.nodes.items[idx].prev;
            const n = _.nodes.items[idx].next;
            if (prev_idx := p) {
                _.nodes.items[prev_idx].next = n;
            } else {
                _.history_head = n;
            }
            if (next_idx := n) {
                _.nodes.items[next_idx].prev = p;
            } else {
                _.history_tail = p;
            }
            _.nodes.items[idx].prev = null;
            _.nodes.items[idx].next = null;
            _.history_len -= 1;
        }

        fn moveToHistoryHead(*_, idx: usize) {
            if (head_idx := _.history_head) {
                if (head_idx == idx) {
                    return;
                }
            }
            _.detachHistory(idx);
            _.attachHistoryHead(idx);
        }

        fn evictHistoryTail(*_) {
            if (tail_idx := _.history_tail) {
                const key = _.nodes.items[tail_idx].key;
                _.detachHistory(tail_idx);
                * = _.map.remove(key);
                _.releaseNode(tail_idx);
            }
        }

        fn promoteFromHistory(*_, idx: usize) {
            _.detachHistory(idx);
            if (_.cache_len >= _.capacity) {
                _.evictCacheTail();
            }
            _.nodes.items[idx].in_cache = true;
            _.attachCacheHead(idx);
        }

        pub fn put(*_, key: K, value: V) -> !void {
            if (existing_idx := _.map.get(key)) {
                _.nodes.items[existing_idx].value = value;
                _.nodes.items[existing_idx].access_count += 1;
                if (_.nodes.items[existing_idx].in_cache) {
                    _.moveToCacheHead(existing_idx);
                } else {
                    if (_.nodes.items[existing_idx].access_count >= _.k_threshold) {
                        _.promoteFromHistory(existing_idx);
                    } else {
                        _.moveToHistoryHead(existing_idx);
                    }
                }
                return;
            }

            if (_.k_threshold == 1) {
                if (_.cache_len >= _.capacity) {
                    _.evictCacheTail();
                }
                const idx = try _.allocNode(key, value, true);
                try _.map.put(key, idx);
                _.attachCacheHead(idx);
                return;
            }

            if (_.history_len >= _.history_capacity) {
                _.evictHistoryTail();
            }
            const idx = try _.allocNode(key, value, false);
            try _.map.put(key, idx);
            _.attachHistoryHead(idx);
        }

        pub fn get(*_, key: K) -> V? {
            if (idx := _.map.get(key)) {
                _.hit_count = _.hit_count +% 1;
                _.nodes.items[idx].access_count += 1;
                if (_.nodes.items[idx].in_cache) {
                    _.moveToCacheHead(idx);
                } else {
                    if (_.nodes.items[idx].access_count >= _.k_threshold) {
                        _.promoteFromHistory(idx);
                    } else {
                        _.moveToHistoryHead(idx);
                    }
                }
                return _.nodes.items[idx].value;
            }
            _.miss_count = _.miss_count +% 1;
            return null;
        }

        pub fn peek(key: K) -> V? {
            if (idx := _.map.get(key)) {
                return _.nodes.items[idx].value;
            }
            return null;
        }

        pub fn remove(*_, key: K) -> V? {
            if (idx := _.map.get(key)) {
                const val = _.nodes.items[idx].value;
                if (_.nodes.items[idx].in_cache) {
                    _.detachCache(idx);
                } else {
                    _.detachHistory(idx);
                }
                * = _.map.remove(key);
                _.releaseNode(idx);
                return val;
            }
            return null;
        }

        pub fn clear(*_) {
            _.map.clearRetainingCapacity();
            _.nodes.clearRetainingCapacity();
            _.free_list.clearRetainingCapacity();
            _.cache_head = null;
            _.cache_tail = null;
            _.history_head = null;
            _.history_tail = null;
            _.cache_len = 0;
            _.history_len = 0;
            _.hit_count = 0;
            _.miss_count = 0;
        }

        pub fn len() -> usize {
            return _.cache_len;
        }

        pub fn historyLen() -> usize {
            return _.history_len;
        }

        pub fn hits() -> u64 {
            return _.hit_count;
        }

        pub fn misses() -> u64 {
            return _.miss_count;
        }
    };
}
