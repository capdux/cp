// Source: Absorbed from /data/dev/bun/src/bun_alloc/maybe_owned.cp | Synced: 2026-11-21T11:59:38Z
// lib/mem/cow.cp
// Copy-on-Write byte slice (absorbed from bun/src/ptr/CowSlice.cp & Cow.cp)

pub struct CowBytes {
    ptr: [*]u8! = undefined;
    length: usize = 0;
    is_owned: bool = false;

    pub static fn initBorrowed(data: []const u8): CowBytes {
        zig {
            return CowBytes{
                .ptr = @constCast(data.ptr),
                .length = data.len,
                .is_owned = false,
            };
        }
    }

    pub static fn initOwned(data: []const u8): CowBytes {
        zig {
            const buf = std.heap.page_allocator.dupe(u8, data) catch unreachable;
            return CowBytes{
                .ptr = buf.ptr,
                .length = data.len,
                .is_owned = true,
            };
        }
    }

    pub fn borrow(): CowBytes {
        return CowBytes{
            ptr: _.ptr,
            length: _.length,
            is_owned: false,
        };
    }

    pub fn isOwned(): bool {
        return _.is_owned;
    }

    pub fn len(): usize {
        return _.length;
    }

    pub fn slice(*_): []const u8 {
        if (_.length == 0) {
            return "";
        }
        let s: []const u8 = "";
        zig {
            s = self.ptr[0..self.length];
        }
        return s;
    }

    // Ensures the buffer is uniquely owned on the heap before mutation (Copy-on-Write)
    pub fn ensureOwned(*_) {
        if (!_.is_owned && _.length > 0) {
            zig {
                const duped = std.heap.page_allocator.dupe(u8, self.ptr[0..self.length]) catch unreachable;
                self.ptr = duped.ptr;
                self.is_owned = true;
            }
        } else if (!_.is_owned) {
            _.is_owned = true;
        }
    }

    pub fn setByte(*_, index: usize, val: u8): bool {
        if (index >= _.length) {
            return false;
        }
        _.ensureOwned();
        unsafe {
            _.ptr[index] = val;
        }
        return true;
    }

    ~init() {
        zig {
            if (self.is_owned and self.length > 0) {
                std.heap.page_allocator.free(self.ptr[0..self.length]);
            }
            self.length = 0;
            self.is_owned = false;
        }
    }
}

pub struct CowSlice {
    inner: CowBytes = CowBytes {};

    pub static fn initBorrowed(data: []const u8): CowSlice {
        return CowSlice {
            inner: CowBytes.initBorrowed(data),
        };
    }

    pub static fn initOwned(data: []const u8): CowSlice {
        return CowSlice {
            inner: CowBytes.initOwned(data),
        };
    }

    pub fn isOwned(): bool {
        return _.inner.isOwned();
    }

    pub fn len(): usize {
        return _.inner.len();
    }

    pub fn slice(*_): []const u8 {
        return _.inner.slice();
    }

    pub fn setByte(*_, index: usize, val: u8): bool {
        return _.inner.setByte(index, val);
    }

    ~init() {
        _.inner.~init();
    }
}

pub type Cow = CowSlice;
