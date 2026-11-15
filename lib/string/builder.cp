// lib/string/builder.cp
// Two-pass zero-reallocation StringBuilder
// Absorbed from bun/src/string/StringBuilder.zig with pure CP semantics

pub struct StringBuilder {
    ptr: [*]u8! = undefined,
    len: usize = 0,
    cap: usize = 0,
    allocated: bool = false,

    pub static fn init(): StringBuilder {
        return StringBuilder {
            ptr: undefined,
            len: 0,
            cap: 0,
            allocated: false,
        };
    }

    // Pass 1: accumulate required byte capacity
    pub fn count(*_, s: []const u8) {
        _.cap += s.len;
    }

    pub fn countByte(*_) {
        _.cap += 1;
    }

    // Allocate exact buffer once between Pass 1 and Pass 2
    pub fn allocate(*_) {
        const c = if (_.cap == 0) 64 else _.cap;
        _.cap = c;
        _.len = 0;
        _.allocated = true;
        zig {
            const buf = std.heap.page_allocator.alloc(u8, c) catch unreachable;
            self.ptr = buf.ptr;
        }
    }

    // Pass 2: append slices into pre-counted buffer without any reallocation
    pub fn append(*_, s: []const u8): []const u8 {
        if (!_.allocated) {
            _.cap += s.len + 64;
            _.allocate();
        }
        if (_.len + s.len > _.cap) {
            const new_cap = (_.cap + s.len) * 2;
            const old_len = _.len;
            const old_cap = _.cap;
            zig {
                const new_buf = std.heap.page_allocator.alloc(u8, new_cap) catch unreachable;
                if (old_len > 0) {
                    @memcpy(new_buf[0..old_len], self.ptr[0..old_len]);
                }
                if (old_cap > 0) {
                    std.heap.page_allocator.free(self.ptr[0..old_cap]);
                }
                self.ptr = new_buf.ptr;
                self.cap = new_cap;
            }
        }
        const start = _.len;
        const n = s.len;
        zig {
            @memcpy(self.ptr[start .. start + n], s);
        }
        _.len += n;
        let out: []const u8 = &[];
        zig {
            out = self.ptr[start .. start + n];
        }
        return out;
    }

    pub fn appendByte(*_, b: u8) {
        let single: [1]u8 = [b];
        * = _.append(single[0..1]);
    }

    pub fn slice(*_): []const u8 {
        if (!_.allocated || _.len == 0) {
            return &[];
        }
        let out: []const u8 = &[];
        zig {
            out = self.ptr[0..self.len];
        }
        return out;
    }

    pub fn reset(*_) {
        _.len = 0;
    }

    pub fn release(*_) {
        if (_.allocated && _.cap > 0) {
            zig {
                std.heap.page_allocator.free(self.ptr[0..self.cap]);
            }
        }
        _.len = 0;
        _.cap = 0;
        _.allocated = false;
    }
}
