// ============================================================================
// Absorbed from Bun: bun/src/string/smol.cp
// Upstream Sync: Refer to bun/src/string/smol.zig for future upstream updates.
// ============================================================================
// lib/string/smol.cp
// SmolStr: Small String Optimization (SSO) container
// Stores <= 15 bytes inline on stack without heap allocation; falls back to heap for longer strings.
// Absorbed from Bun src/string/SmolStr.cp with pure CP semantics

pub const SMOL_MAX_INLINE: usize = 15;

pub struct SmolStr {
    inline_buf: [15]u8 = undefined,
    heap_ptr: [*]u8! = undefined,
    length: u32 = 0,
    inlined: bool = true,

    pub static fn init(s: []const u8): SmolStr {
        if (s.len <= SMOL_MAX_INLINE) {
            let res = SmolStr {
                inline_buf: undefined,
                heap_ptr: undefined,
                length: s.len as u32,
                inlined: true,
            };
            let i: usize = 0;
            while (i < s.len) {
                res.inline_buf[i] = s[i];
                i += 1;
            }
            return res;
        } else {
            let p: [*]u8! = unsafe { @ptrFromInt(1) };
            zig {
                const mem = std.heap.page_allocator.alloc(u8, s.len) catch unreachable;
                @memcpy(mem, s);
                p = mem.ptr;
            }
            return SmolStr {
                inline_buf: undefined,
                heap_ptr: p,
                length: s.len as u32,
                inlined: false,
            };
        }
    }

    pub fn len(): usize {
        return _.length as usize;
    }

    pub fn isInline(): bool {
        return _.inlined;
    }

    pub fn slice(*_): []const u8 {
        if (_.inlined) {
            return _.inline_buf[0..(_.length as usize)];
        } else {
            let res: []const u8 = "";
            zig {
                res = self.heap_ptr[0..self.length];
            }
            return res;
        }
    }

    pub fn eql(*_, other: []const u8): bool {
        const s = _.slice();
        if (s.len != other.len) {
            return false;
        }
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, s, other);
        }
        return eq;
    }

    pub fn release(*_) {
        if (!_.inlined && _.length > 0) {
            zig {
                std.heap.page_allocator.free(self.heap_ptr[0..self.length]);
            }
            _.inlined = true;
            _.length = 0;
        }
    }
}
