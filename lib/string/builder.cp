// Source: /data/dev/bun/src/string/StringBuilder.cp
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/string/StringBuilder.cp
// Upstream Sync: Refer to /data/dev/bun/src/string/StringBuilder.zig for future upstream updates.
// ============================================================================
// lib/string/builder.cp
// Two-pass zero-reallocation StringBuilder (100% backward compatible + Bun parity)

const std = @import("std");
import { elementLengthUTF16IntoUTF8, copyUTF16IntoUTF8 } from "./utf.cp";

pub struct StringBuilder {
    ptr: [*]u8! = undefined,
    len: usize = 0,
    cap: usize = 0,
    allocated: bool = false,
    alloc_backing: std.mem.Allocator = std.heap.page_allocator,

    pub static fn init(): StringBuilder {
        return StringBuilder{
            ptr: undefined,
            len: 0,
            cap: 0,
            allocated: false,
            alloc_backing: std.heap.page_allocator,
        };
    }

    pub static fn initWithAllocator(allocator: std.mem.Allocator): StringBuilder {
        return StringBuilder{
            ptr: undefined,
            len: 0,
            cap: 0,
            allocated: false,
            alloc_backing: allocator,
        };
    }

    pub static fn initCapacity(allocator: std.mem.Allocator, init_cap: usize): !StringBuilder {
        const c = if (init_cap == 0) (64 as usize) else init_cap;
        const buf = try allocator.alloc(u8, c);
        return StringBuilder{
            ptr: buf.ptr,
            len: 0,
            cap: c,
            allocated: true,
            alloc_backing: allocator,
        };
    }

    // Pass 1: accumulate required byte capacity
    pub fn count(*_, s: []const u8) {
        _.cap += s.len;
    }

    pub fn countZ(*_, s: []const u8) {
        _.cap += s.len + 1;
    }

    pub fn countByte(*_) {
        _.cap += 1;
    }

    pub fn count16(*_, slice_u16: []const u16) {
        _.cap += elementLengthUTF16IntoUTF8(slice_u16);
    }

    pub fn count16Z(*_, slice_u16: []const u16) {
        _.cap += elementLengthUTF16IntoUTF8(slice_u16) + 1;
    }

    // Allocate exact buffer once between Pass 1 and Pass 2
    pub fn allocate(*_) {
        const c = if (_.cap == 0) (64 as usize) else _.cap;
        _.cap = c;
        _.len = 0;
        _.allocated = true;
        const alloc = _.alloc_backing;
        zig {
            const buf = alloc.alloc(u8, c) catch unreachable;
            self.ptr = buf.ptr;
        }
    }

    pub fn allocateIn(*_, allocator: std.mem.Allocator): !void {
        _.alloc_backing = allocator;
        const c = if (_.cap == 0) (64 as usize) else _.cap;
        const buf = try allocator.alloc(u8, c);
        _.ptr = buf.ptr;
        _.cap = c;
        _.len = 0;
        _.allocated = true;
    }

    fn ensureRoom(*_, needed: usize) {
        if (!_.allocated) {
            _.cap += needed + 64;
            _.allocate();
            return;
        }
        if (_.len + needed > _.cap) {
            const new_cap = (_.cap + needed) * 2;
            const old_len = _.len;
            const old_cap = _.cap;
            const alloc = _.alloc_backing;
            zig {
                const new_buf = alloc.alloc(u8, new_cap) catch unreachable;
                if (old_len > 0) {
                    @memcpy(new_buf[0..old_len], self.ptr[0..old_len]);
                }
                if (old_cap > 0) {
                    alloc.free(self.ptr[0..old_cap]);
                }
                self.ptr = new_buf.ptr;
                self.cap = new_cap;
            }
        }
    }

    // Pass 2: append slices into pre-counted buffer without any reallocation
    pub fn append(*_, s: []const u8): []const u8 {
        _.ensureRoom(s.len);
        const start = _.len;
        const n = s.len;
        zig {
            @memcpy(self.ptr[start .. start + n], s);
        }
        _.len += n;
        let out: []const u8 = "";
        zig {
            out = self.ptr[start .. start + n];
        }
        return out;
    }

    pub fn appendZ(*_, s: []const u8): [:0]const u8 {
        _.ensureRoom(s.len + 1);
        const start = _.len;
        const n = s.len;
        zig {
            @memcpy(self.ptr[start .. start + n], s);
            self.ptr[start + n] = 0;
        }
        _.len += n + 1;
        let out: [:0]const u8 = "";
        zig {
            out = self.ptr[start .. start + n :0];
        }
        return out;
    }

    pub fn append16(*_, slice_u16: []const u16): []const u8 {
        const needed = elementLengthUTF16IntoUTF8(slice_u16);
        _.ensureRoom(needed);
        const start = _.len;
        const buf = _.writable();
        const res = copyUTF16IntoUTF8(buf, slice_u16);
        _.len += res.written as usize;
        let out: []const u8 = "";
        const w = res.written as usize;
        zig {
            out = self.ptr[start .. start + w];
        }
        return out;
    }

    pub fn appendByte(*_, b: u8) {
        let single: [1]u8 = [b];
        * = _.append(single[0..1]);
    }

    pub fn appendFmt(*_, comptime fmt_str: []const u8, args: auto): []const u8 {
        if (!_.allocated) {
            _.allocate();
        }
        const rem = _.writable();
        const out = std.fmt.bufPrint(rem, fmt_str, args) catch {
            // Fallback: allocate temporary formatted string then append
            const tmp = std.fmt.allocPrint(_.alloc_backing, fmt_str, args) catch unreachable;
            defer _.alloc_backing.free(tmp);
            return _.append(tmp);
        };
        _.len += out.len;
        return out;
    }

    pub fn fmt(*_, comptime fmt_str: []const u8, args: auto): []const u8 {
        return _.appendFmt(fmt_str, args);
    }

    pub fn writable(*_): []u8 {
        if (!_.allocated || _.len >= _.cap) {
            let empty_buf: [0]u8 = undefined;
            return empty_buf[0..0];
        }
        let out: []u8 = undefined;
        zig {
            out = self.ptr[self.len..self.cap];
        }
        return out;
    }

    pub fn allocatedSlice(*_): []u8 {
        if (!_.allocated || _.cap == 0) {
            let empty_buf: [0]u8 = undefined;
            return empty_buf[0..0];
        }
        let out: []u8 = undefined;
        zig {
            out = self.ptr[0..self.cap];
        }
        return out;
    }

    pub fn slice(*_): []const u8 {
        if (!_.allocated || _.len == 0) {
            return "";
        }
        let out: []const u8 = "";
        zig {
            out = self.ptr[0..self.len];
        }
        return out;
    }

    pub fn length(): usize {
        return _.len;
    }

    pub fn capacity(): usize {
        return _.cap;
    }

    pub fn toOwnedSlice(*_, allocator: std.mem.Allocator): ![]u8 {
        const cur = _.slice();
        const duped = try allocator.dupe(u8, cur);
        _.~init();
        return duped;
    }

    pub fn reset(*_) {
        _.len = 0;
    }

    ~init() {
        if (_.allocated && _.cap > 0) {
            const alloc = _.alloc_backing;
            zig {
                alloc.free(self.ptr[0..self.cap]);
            }
        }
        _.len = 0;
        _.cap = 0;
        _.allocated = false;
    }
}
