// Source: Absorbed from /data/dev/bun/src/ptr/CowSlice.cp | Synced: 2026-11-22T12:49:17Z
// Copy-on-Write slice primitive with pointer-packed ownership flag
import std from "std";

pub type CowSliceLen = u63;

@packed(usize) struct CowSliceFlags {
    len: CowSliceLen,
    is_owned: bool,
}

pub fn CowSlice(comptime T: type): type {
    return CowSliceZ(T, null);
}

pub fn CowSliceZ(comptime T: type, comptime sentinel: T?): type {
    return struct {
        ptr: [*]T,
        flags: CowSliceFlags,

        const Self = @This();
        pub const Slice = if (sentinel != null) [:sentinel.?]const T else []const T;
        pub const SliceMut = if (sentinel != null) [:sentinel.?]T else []T;

        pub const empty: Self = initStatic("");

        pub static fn initOwned(data: []T, allocator: std.mem.Allocator): Self {
            return Self{
                ptr: data.ptr,
                flags: CowSliceFlags{
                    is_owned: true,
                    len: @intCast(data.len),
                },
            };
        }

        pub static fn initDupe(data: Slice, allocator: std.mem.Allocator): !Self {
            const bytes: Slice = if (comptime sentinel != null)
                try allocator.dupeZ(T, data)
            else
                try allocator.dupe(T, data);

            return initOwned(unsafe { @constCast(bytes) }, allocator);
        }

        pub static fn initStatic(comptime data: Slice): Self {
            return Self{
                ptr: unsafe { @constCast(data.ptr) },
                flags: CowSliceFlags{
                    is_owned: false,
                    len: @intCast(data.len),
                },
            };
        }

        pub fn isOwned(): bool {
            return _.flags.is_owned;
        }

        pub fn slice(): Slice {
            return unsafe { _.ptr[0.._.flags.len] };
        }

        pub fn length(): usize {
            return _.flags.len as usize;
        }

        pub fn len(): usize {
            return _.length();
        }

        pub fn sliceMut(*_, allocator: std.mem.Allocator): !SliceMut {
            if (!_.isOwned()) {
                try _.intoOwned(allocator);
            }
            return unsafe { _.ptr[0.._.flags.len] };
        }

        pub fn sliceMutUnsafe(*_): SliceMut {
            if (!_.isOwned()) {
                @panic("CowSlice.sliceMutUnsafe called on borrowed slice");
            }
            return unsafe { _.ptr[0.._.flags.len] };
        }

        pub fn takeSlice(*_, allocator: std.mem.Allocator): !SliceMut {
            if (!_.isOwned()) {
                try _.intoOwned(allocator);
            }
            const res = unsafe { _.ptr[0.._.flags.len] };
            _.* = Self.empty;
            return res;
        }

        pub fn borrow(): Self {
            return Self{
                ptr: _.ptr,
                flags: CowSliceFlags{
                    is_owned: false,
                    len: _.flags.len,
                },
            };
        }

        pub fn borrowSubslice(start: usize, end: usize?): Self {
            const end_ = end ?? (_.flags.len as usize);
            if (start > end_ or end_ > (_.flags.len as usize)) {
                @panic("CowSlice.borrowSubslice: index out of bounds");
            }
            const subrange: Slice = if (comptime sentinel != null)
                unsafe { _.ptr[start..end_ :sentinel.?] }
            else
                unsafe { _.ptr[start..end_] };

            let result = _.borrow();
            result.ptr = unsafe { @constCast(subrange.ptr) };
            result.flags.len = @intCast(end_ - start);
            return result;
        }

        pub fn toOwned(*_, allocator: std.mem.Allocator): !void {
            if (!_.isOwned()) {
                try _.intoOwned(allocator);
            }
        }

        fn intoOwned(*_, allocator: std.mem.Allocator): !void {
            if (_.isOwned()) {
                @panic("CowSlice.intoOwned called on already owned slice");
            }

            const current = _.slice();
            const bytes: Slice = if (comptime sentinel != null)
                try allocator.dupeZ(T, current)
            else
                try allocator.dupe(T, current);

            _.ptr = unsafe { @constCast(bytes.ptr) };
            _.flags.is_owned = true;
        }

        pub fn deinit(*_, allocator: std.mem.Allocator) {
            if (_.flags.is_owned) {
                allocator.free(_.slice());
                _.flags.is_owned = false;
                _.flags.len = 0;
            }
        }

        pub static fn initUnchecked(data: Slice, is_owned: bool): Self {
            return Self{
                ptr: unsafe { @constCast(data.ptr) },
                flags: CowSliceFlags{
                    is_owned: is_owned,
                    len: @intCast(data.len),
                },
            };
        }
    };
}

pub type CowString = CowSlice(u8);
