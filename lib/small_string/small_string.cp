// Source: absorbed from bun/src/install_types/{SemverString,ExternalString,SlicedString}.cp (cap:small_string)
const std = @import("std");

pub const MAX_INLINE_LEN: usize = 8;

pub const SmallString = extern struct {
    bytes: [8]u8,

    pub fn empty(): SmallString {
        let out: [8]u8 = undefined;
        @memset(&out, 0);
        return SmallString{ .bytes = out };
    }

    pub fn canInline(s: []const u8): bool {
        if (s.len > MAX_INLINE_LEN) { return false; }
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            if (s[i] == 0) { return false; }
        }
        return true;
    }

    pub fn initInline(s: []const u8): SmallString {
        let out: [8]u8 = undefined;
        @memset(&out, 0);
        const n = @min(s.len, MAX_INLINE_LEN);
        @memcpy(out[0..n], s[0..n]);
        return SmallString{ .bytes = out };
    }

    pub fn initOffset(offset: u32, length: u32): SmallString {
        let out: [8]u8 = undefined;
        std.mem.writeInt(u32, out[0..4], offset, .little);
        // High bit of u32 length marks external/arena-backed mode
        std.mem.writeInt(u32, out[4..8], length | 0x8000_0000, .little);
        return SmallString{ .bytes = out };
    }

    pub fn isInline(self: SmallString): bool {
        return (self.bytes[7] & 0x80) == 0;
    }

    pub fn len(self: SmallString): usize {
        if (self.isInline()) {
            let i: usize = 0;
            while (i < 8) : (i += 1) {
                if (self.bytes[i] == 0) { return i; }
            }
            return 8;
        }
        const raw_len = std.mem.readInt(u32, self.bytes[4..8], .little) & 0x7FFF_FFFF;
        return @intCast(raw_len);
    }

    pub fn slice(self: *const SmallString, arena_buf: []const u8): []const u8 {
        if (self.isInline()) {
            return self.bytes[0..self.len()];
        }
        const off: usize = @intCast(std.mem.readInt(u32, self.bytes[0..4], .little));
        const l = self.len();
        return arena_buf[off .. off + l];
    }

    pub fn order(self: *const SmallString, other: *const SmallString, lhs_buf: []const u8, rhs_buf: []const u8): std.math.Order {
        return std.mem.order(u8, self.slice(lhs_buf), other.slice(rhs_buf));
    }

    pub fn eql(self: *const SmallString, other: *const SmallString, lhs_buf: []const u8, rhs_buf: []const u8): bool {
        return std.mem.eql(u8, self.slice(lhs_buf), other.slice(rhs_buf));
    }
};

pub const ExternalString = struct {
    value: SmallString,
    hash: u64,

    pub fn slice(self: *const ExternalString, arena_buf: []const u8): []const u8 {
        return self.value.slice(arena_buf);
    }

    pub fn eql(self: *const ExternalString, other: *const ExternalString, lhs_buf: []const u8, rhs_buf: []const u8): bool {
        if (self.hash != other.hash) { return false; }
        return self.value.eql(&other.value, lhs_buf, rhs_buf);
    }
};

pub const StringInternPool = struct {
    arena: std.ArrayList(u8),
    pool: std.AutoHashMap(u64, SmallString),
    alloc: std.mem.Allocator,

    pub fn init(alloc: std.mem.Allocator): StringInternPool {
        return StringInternPool{
            .arena = std.ArrayList(u8).empty,
            .pool = std.AutoHashMap(u64, SmallString).init(alloc),
            .alloc = alloc,
        };
    }

    pub fn deinit(self: *StringInternPool) {
        self.arena.deinit(self.alloc);
        self.pool.deinit();
    }

    pub fn buffer(self: StringInternPool): []const u8 {
        return self.arena.items;
    }

    pub fn intern(self: *StringInternPool, s: []const u8): !ExternalString {
        const h = std.hash.Wyhash.hash(0, s);
        if (SmallString.canInline(s)) {
            return ExternalString{
                .value = SmallString.initInline(s),
                .hash = h,
            };
        }
        const maybe_existing = self.pool.get(h);
        if (maybe_existing != null) {
            const existing = maybe_existing.?;
            if (std.mem.eql(u8, existing.slice(self.arena.items), s)) {
                return ExternalString{ .value = existing, .hash = h };
            }
        }
        const off: u32 = @intCast(self.arena.items.len);
        const slen: u32 = @intCast(s.len);
        try self.arena.appendSlice(self.alloc, s);
        const val = SmallString.initOffset(off, slen);
        try self.pool.put(h, val);
        return ExternalString{ .value = val, .hash = h };
    }
};
