// Source: native-cp-stdlib (Bidirectional 1-to-1 Map)
import * as zstd from "zig:std";

type StrSlice = []const u8;

export const BiMapError = error {
    DuplicateLeft,
    DuplicateRight,
};

export fn BiMap(comptime L: type, comptime R: type): type {
    return struct {
        const Self = @This();

        alloc: zstd.mem.Allocator,
        left_to_right: zstd.AutoHashMap(L, R),
        right_to_left: zstd.AutoHashMap(R, L),

        pub fn init(alloc: zstd.mem.Allocator): Self {
            return Self{
                alloc: alloc,
                left_to_right: zstd.AutoHashMap(L, R).init(alloc),
                right_to_left: zstd.AutoHashMap(R, L).init(alloc),
            };
        }

        pub fn deinit(&self) {
            self.left_to_right.deinit();
            self.right_to_left.deinit();
        }

        pub fn insert(&self, left: L, right: R): !void {
            if (self.left_to_right.contains(left)) {
                return BiMapError.DuplicateLeft;
            }
            if (self.right_to_left.contains(right)) {
                return BiMapError.DuplicateRight;
            }
            try self.left_to_right.put(left, right);
            errdefer _ = self.left_to_right.remove(left);
            try self.right_to_left.put(right, left);
        }

        pub fn forceInsert(&self, left: L, right: R): !void {
            if (old_right_kv := self.left_to_right.fetchRemove(left)) {
                * = self.right_to_left.remove(old_right_kv.value);
            }
            if (old_left_kv := self.right_to_left.fetchRemove(right)) {
                * = self.left_to_right.remove(old_left_kv.value);
            }
            try self.left_to_right.put(left, right);
            errdefer _ = self.left_to_right.remove(left);
            try self.right_to_left.put(right, left);
        }

        pub fn getRight(self, left: L): R? {
            return self.left_to_right.get(left);
        }

        pub fn getLeft(self, right: R): L? {
            return self.right_to_left.get(right);
        }

        pub fn removeByLeft(&self, left: L): R? {
            if (kv := self.left_to_right.fetchRemove(left)) {
                * = self.right_to_left.remove(kv.value);
                return kv.value;
            }
            return null;
        }

        pub fn removeByRight(&self, right: R): L? {
            if (kv := self.right_to_left.fetchRemove(right)) {
                * = self.left_to_right.remove(kv.value);
                return kv.value;
            }
            return null;
        }

        pub fn containsLeft(self, left: L): bool {
            return self.left_to_right.contains(left);
        }

        pub fn containsRight(self, right: R): bool {
            return self.right_to_left.contains(right);
        }

        pub fn len(self): usize {
            return self.left_to_right.count();
        }

        pub fn clear(&self) {
            self.left_to_right.clearRetainingCapacity();
            self.right_to_left.clearRetainingCapacity();
        }
    };
}

export class StringBiMap {
    alloc: zstd.mem.Allocator;
    left_to_right: zstd.StringHashMap(StrSlice);
    right_to_left: zstd.StringHashMap(StrSlice);

    pub fn init(alloc: zstd.mem.Allocator): StringBiMap {
        return StringBiMap{
            alloc: alloc,
            left_to_right: zstd.StringHashMap(StrSlice).init(alloc),
            right_to_left: zstd.StringHashMap(StrSlice).init(alloc),
        };
    }

    pub fn deinit(*_) {
        let it = _.left_to_right.iterator();
        while (entry := it.next()) {
            _.alloc.free(entry.key_ptr.*);
            _.alloc.free(entry.value_ptr.*);
        }
        _.left_to_right.deinit();
        _.right_to_left.deinit();
    }

    pub fn insert(*_, left: []const u8, right: []const u8): !void {
        if (_.left_to_right.contains(left)) {
            return BiMapError.DuplicateLeft;
        }
        if (_.right_to_left.contains(right)) {
            return BiMapError.DuplicateRight;
        }
        const left_copy = try _.alloc.dupe(u8, left);
        errdefer _.alloc.free(left_copy);
        const right_copy = try _.alloc.dupe(u8, right);
        errdefer _.alloc.free(right_copy);

        try _.left_to_right.put(left_copy, right_copy);
        errdefer * = _.left_to_right.remove(left_copy);
        try _.right_to_left.put(right_copy, left_copy);
    }

    pub fn forceInsert(*_, left: []const u8, right: []const u8): !void {
        const left_copy = try _.alloc.dupe(u8, left);
        errdefer _.alloc.free(left_copy);
        const right_copy = try _.alloc.dupe(u8, right);
        errdefer _.alloc.free(right_copy);

        * = _.removeByLeft(left);
        * = _.removeByRight(right);

        try _.left_to_right.put(left_copy, right_copy);
        errdefer * = _.left_to_right.remove(left_copy);
        try _.right_to_left.put(right_copy, left_copy);
    }

    pub fn getRight(left: []const u8): ([]const u8)? {
        return _.left_to_right.get(left);
    }

    pub fn getLeft(right: []const u8): ([]const u8)? {
        return _.right_to_left.get(right);
    }

    pub fn removeByLeft(*_, left: []const u8): bool {
        if (kv := _.left_to_right.fetchRemove(left)) {
            * = _.right_to_left.remove(kv.value);
            _.alloc.free(kv.key);
            _.alloc.free(kv.value);
            return true;
        }
        return false;
    }

    pub fn removeByRight(*_, right: []const u8): bool {
        if (kv := _.right_to_left.fetchRemove(right)) {
            * = _.left_to_right.remove(kv.value);
            _.alloc.free(kv.key);
            _.alloc.free(kv.value);
            return true;
        }
        return false;
    }

    pub fn containsLeft(left: []const u8): bool {
        return _.left_to_right.contains(left);
    }

    pub fn containsRight(right: []const u8): bool {
        return _.right_to_left.contains(right);
    }

    pub fn len(): usize {
        return _.left_to_right.count();
    }

    pub fn clear(*_) {
        let it = _.left_to_right.iterator();
        while (entry := it.next()) {
            _.alloc.free(entry.key_ptr.*);
            _.alloc.free(entry.value_ptr.*);
        }
        _.left_to_right.clearRetainingCapacity();
        _.right_to_left.clearRetainingCapacity();
    }
}
