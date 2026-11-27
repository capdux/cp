// @cap/bumpalo — Zero-allocation alignment-aware bump arena allocator for CP
// Clean-room MIT implementation inspired by Rust `bumpalo` v3 (850M+ downloads).
// 4096-byte fixed backing buffer, power-of-two alignment (1..=64), overflow-safe bounds,
// and O(1) checkpoint/rewind & reset.

pub const BUMP_CAPACITY: usize = 4096;

fn bmpIsValidAlign(alignment: usize) -> bool {
    if (alignment < (1 as usize) || alignment > (64 as usize)) {
        return false;
    }
    return (alignment & (alignment - (1 as usize))) == (0 as usize);
}

fn bmpAlignUp(offset: usize, alignment: usize) -> usize? {
    if (offset > BUMP_CAPACITY) {
        return null;
    }
    let rem = offset % alignment;
    if (rem == (0 as usize)) {
        return offset;
    }
    let pad = alignment - rem;
    if (pad > BUMP_CAPACITY - offset) {
        return null;
    }
    return offset + pad;
}

fn bmpHandleValid(handle: BumpSlice, cursor: usize) -> bool {
    if (handle.offset > cursor) {
        return false;
    }
    if (handle.len > cursor - handle.offset) {
        return false;
    }
    return true;
}

/// Handle referencing a contiguous region allocated inside a `BumpArena`.
pub struct BumpSlice {
    pub offset: usize,
    pub len: usize,

    pub static fn empty() -> BumpSlice {
        return BumpSlice{
            offset: 0 as usize,
            len: 0 as usize,
        };
    }

    pub fn isEmpty() -> bool {
        return _.len == (0 as usize);
    }

    pub fn end() -> usize {
        return _.offset + _.len;
    }
}

/// Zero-allocation linear bump allocator backed by a 4096-byte static array.
pub struct BumpArena {
    pub buf: [4096]u8,
    pub cursor: usize,
    pub alloc_count: usize,

    pub static fn new() -> BumpArena {
        let backing: [4096]u8 = undefined;
        let i: usize = 0;
        while (i < BUMP_CAPACITY) {
            backing[i] = 0 as u8;
            i += 1;
        }
        return BumpArena{
            buf: backing,
            cursor: 0 as usize,
            alloc_count: 0 as usize,
        };
    }

    pub fn capacity() -> usize {
        return BUMP_CAPACITY;
    }

    pub fn allocatedBytes() -> usize {
        return _.cursor;
    }

    pub fn remaining() -> usize {
        return BUMP_CAPACITY - _.cursor;
    }

    pub fn allocationCount() -> usize {
        return _.alloc_count;
    }

    pub fn reset(*_) {
        _.cursor = 0 as usize;
        _.alloc_count = 0 as usize;
    }

    pub fn allocAligned(*_, size: usize, alignment: usize) -> BumpSlice? {
        if (!bmpIsValidAlign(alignment)) {
            return null;
        }
        let aligned_opt = bmpAlignUp(_.cursor, alignment);
        if (aligned_opt == null) {
            return null;
        }
        let aligned = aligned_opt ?? (0 as usize);
        if (size > BUMP_CAPACITY - aligned) {
            return null;
        }
        let i: usize = 0;
        while (i < size) {
            _.buf[aligned + i] = 0 as u8;
            i += 1;
        }
        _.cursor = aligned + size;
        _.alloc_count += 1;
        return BumpSlice{
            offset: aligned,
            len: size,
        };
    }

    pub fn allocBytes(*_, size: usize) -> BumpSlice? {
        return _.allocAligned(size, 1 as usize);
    }

    pub fn allocSliceCopy(*_, src: []const u8) -> BumpSlice? {
        if (src.len > BUMP_CAPACITY - _.cursor) {
            return null;
        }
        let start = _.cursor;
        let i: usize = 0;
        while (i < src.len) {
            _.buf[start + i] = src[i];
            i += 1;
        }
        _.cursor = start + src.len;
        _.alloc_count += 1;
        return BumpSlice{
            offset: start,
            len: src.len,
        };
    }

    pub fn allocStr(*_, s: string) -> BumpSlice? {
        if (s.len > BUMP_CAPACITY - _.cursor) {
            return null;
        }
        let start = _.cursor;
        let i: usize = 0;
        while (i < s.len) {
            _.buf[start + i] = s[i];
            i += 1;
        }
        _.cursor = start + s.len;
        _.alloc_count += 1;
        return BumpSlice{
            offset: start,
            len: s.len,
        };
    }

    pub fn readByte(handle: BumpSlice, idx: usize) -> u8? {
        if (!bmpHandleValid(handle, _.cursor)) {
            return null;
        }
        if (idx >= handle.len) {
            return null;
        }
        return _.buf[handle.offset + idx];
    }

    pub fn writeByte(*_, handle: BumpSlice, idx: usize, val: u8) -> bool {
        if (!bmpHandleValid(handle, _.cursor)) {
            return false;
        }
        if (idx >= handle.len) {
            return false;
        }
        _.buf[handle.offset + idx] = val;
        return true;
    }

    pub fn matchesStr(handle: BumpSlice, expected: string) -> bool {
        if (!bmpHandleValid(handle, _.cursor)) {
            return false;
        }
        if (handle.len != expected.len) {
            return false;
        }
        let i: usize = 0;
        while (i < expected.len) {
            if (_.buf[handle.offset + i] != expected[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn checkpoint() -> usize {
        return _.cursor;
    }

    pub fn rewind(*_, mark: usize) -> bool {
        if (mark > _.cursor) {
            return false;
        }
        _.cursor = mark;
        if (mark == (0 as usize)) {
            _.alloc_count = 0 as usize;
        }
        return true;
    }
}
