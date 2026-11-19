// Source: /data/dev/bun/src/ptr/Cow.cp
// Copy-On-Write value wrapper (borrowed or owned)
import std from "std";

fn cowCopy(comptime T: type, comptime VTable: type, ptr: &T, allocator: std.mem.Allocator): T {
    if (comptime !@hasDecl(VTable, "copy")) {
        @compileError(@typeName(VTable) ++ " needs `copy()` function");
    }
    return VTable.copy(ptr, allocator);
}

fn cowDeinit(comptime T: type, comptime VTable: type, ptr: *T, allocator: std.mem.Allocator) {
    if (comptime !@hasDecl(VTable, "deinit")) {
        @compileError(@typeName(VTable) ++ " needs `deinit()` function");
    }
    VTable.deinit(ptr, allocator);
}

pub fn DefaultVTable(comptime T: type): type {
    return struct {
        pub static fn copy(ptr: &T, allocator: std.mem.Allocator): T {
            if (comptime (@typeInfo(T) == .@"struct" or @typeInfo(T) == .@"union") and @hasDecl(T, "clone")) {
                return ptr.clone();
            }
            return ptr.*;
        }

        pub static fn deinit(ptr: *T, allocator: std.mem.Allocator) {
            if (comptime (@typeInfo(T) == .@"struct" or @typeInfo(T) == .@"union") and @hasDecl(T, "deinit")) {
                ptr.deinit();
            }
        }
    };
}

pub fn Cow(comptime T: type): type {
    return CowWithVTable(T, DefaultVTable(T));
}

pub fn CowWithVTable(comptime T: type, comptime VTable: type): type {
    const info = @typeInfo(T);
    if (info == .pointer && info.pointer.size == .slice) {
        @compileError("Cow should not be used with slice types. Use CowSlice instead.");
    }

    return union(enum) {
        const Self = @This();

        borrowed: &T,
        owned: T,

        pub static fn borrow(val: &T): Self {
            return Self{ borrowed: val };
        }

        pub static fn own(val: T): Self {
            return Self{ owned: val };
        }

        pub fn replace(*_, allocator: std.mem.Allocator, newval: T) {
            if (_.* == .owned) {
                _.deinit(allocator);
            }
            _.* = Self{ owned: newval };
        }

        pub fn inner(*_): &T {
            return switch (_.*) {
                .borrowed => _.borrowed,
                .owned => &_.owned,
            };
        }

        pub fn innerMut(*_): (*T)? {
            return switch (_.*) {
                .borrowed => null,
                .owned => &_.owned,
            };
        }

        pub fn toOwned(*_, allocator: std.mem.Allocator): *T {
            switch (_.*) {
                .borrowed => |b| {
                    const copied = cowCopy(T, VTable, b, allocator);
                    _.* = Self{
                        owned: copied,
                    };
                },
                .owned => {},
            }
            return &_.owned;
        }

        pub fn isOwned(): bool {
            return switch (_) {
                .borrowed => false,
                .owned => true,
            };
        }

        pub fn deinit(*_, allocator: std.mem.Allocator) {
            if (_.* == .owned) {
                cowDeinit(T, VTable, &_.owned, allocator);
            }
        }
    };
}
