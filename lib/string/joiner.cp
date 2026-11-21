// Source: /data/dev/bun/src/string/StringJoiner.cp
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/string/StringJoiner.cp
// Upstream Sync: Refer to /data/dev/bun/src/string/StringJoiner.zig for future upstream updates.
// ============================================================================
// lib/string/joiner.cp
// StringJoiner: High-performance string joiner and concatenator
// Absorbed from Bun src/string/StringJoiner.cp with pure CP semantics

pub struct StringJoiner {
    buf: [*]u8! = undefined,
    capacity: usize = 0,
    length: usize = 0,
    has_allocated: bool = false,

    pub static fn init(): StringJoiner {
        let cap: usize = 256;
        let p: [*]u8! = unsafe { @ptrFromInt(1) };
        zig {
            const mem = std.heap.page_allocator.alloc(u8, cap) catch unreachable;
            p = mem.ptr;
        }
        return StringJoiner {
            buf: p,
            capacity: cap,
            length: 0,
            has_allocated: true,
        };
    }

    pub static fn join(slices: []const []const u8, delimiter: []const u8): []u8 {
        if (slices.len == 0) {
            let empty: []u8 = "";
            return empty;
        }
        let total_len: usize = 0;
        let i: usize = 0;
        while (i < slices.len) {
            total_len += slices[i].len;
            if (i + 1 < slices.len) {
                total_len += delimiter.len;
            }
            i += 1;
        }
        let out_slice: []u8 = "";
        zig {
            const mem = std.heap.page_allocator.alloc(u8, total_len) catch unreachable;
            var offset: usize = 0;
            var idx: usize = 0;
            while (idx < slices.len) : (idx += 1) {
                const s = slices[idx];
                @memcpy(mem[offset .. offset + s.len], s);
                offset += s.len;
                if (idx + 1 < slices.len and delimiter.len > 0) {
                    @memcpy(mem[offset .. offset + delimiter.len], delimiter);
                    offset += delimiter.len;
                }
            }
            out_slice = mem;
        }
        return out_slice;
    }

    pub fn len(): usize {
        return _.length;
    }

    pub fn push(*_, s: []const u8) {
        if (s.len == 0) {
            return;
        }
        _.ensureCapacity(_.length + s.len);
        zig {
            @memcpy(self.buf[self.length .. self.length + s.len], s);
        }
        _.length += s.len;
    }

    pub fn pushDelimited(*_, s: []const u8, delimiter: []const u8) {
        if (_.length > 0 && delimiter.len > 0) {
            _.push(delimiter);
        }
        _.push(s);
    }

    pub fn slice(): []const u8 {
        if (_.length == 0) {
            return "";
        }
        let res: []const u8 = "";
        zig {
            res = self.buf[0..self.length];
        }
        return res;
    }

    pub fn ensureCapacity(*_, needed: usize) {
        if (needed <= _.capacity) {
            return;
        }
        let new_cap = _.capacity * 2;
        if (new_cap < needed) {
            new_cap = needed + 64;
        }
        let new_p: [*]u8! = unsafe { @ptrFromInt(1) };
        zig {
            const new_mem = std.heap.page_allocator.alloc(u8, new_cap) catch unreachable;
            if (self.length > 0) {
                @memcpy(new_mem[0..self.length], self.buf[0..self.length]);
            }
            if (self.has_allocated) {
                std.heap.page_allocator.free(self.buf[0..self.capacity]);
            }
            new_p = new_mem.ptr;
        }
        _.buf = new_p;
        _.capacity = new_cap;
        _.has_allocated = true;
    }

    ~init() {
        if (_.has_allocated && _.capacity > 0) {
            zig {
                std.heap.page_allocator.free(self.buf[0..self.capacity]);
            }
            _.has_allocated = false;
            _.capacity = 0;
            _.length = 0;
        }
    }
}
