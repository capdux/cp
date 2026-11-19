// Source: /data/dev/bun/src/ptr/owned.cp
// Unique single-ownership pointer and slice abstraction (Owned<T>, OwnedIn<T, Allocator>, Dynamic<T>)
import std from "std";
import { PointerInfo, PointerKind, AddConst } from "./meta.cp";

const AllocError = std.mem.Allocator.Error;

pub fn Owned(comptime Pointer: type): type {
    return OwnedIn(Pointer, std.mem.Allocator);
}

pub fn Dynamic(comptime Pointer: type): type {
    return OwnedIn(Pointer, std.mem.Allocator);
}

pub fn OwnedIn(comptime Pointer: type, comptime Allocator: type): type {
    return struct {
        const Self = @This();
        const parse_opts = PointerInfo.ParseOptions{ allow_const: true, allow_slices: true };
        const info = PointerInfo.parse(Pointer, parse_opts);
        const NonOptionalPointer = info.NonOptionalPointer;
        const Child = info.Child;
        const ConstPointer = AddConst(Pointer);

        ptr: Pointer,
        allocator: Allocator,

        static fn allocSingle(value: Child): !Self {
            return Self.allocInSingle(value, std.heap.page_allocator);
        }

        static fn allocSlice(count: usize, elem: Child): !Self {
            return Self.allocInSlice(count, elem, std.heap.page_allocator);
        }

        pub const alloc = if (info.kind() == .single) allocSingle else allocSlice;

        pub static fn allocInSingle(value: Child, alloc_inst: Allocator): !Self {
            let data = try alloc_inst.create(Child);
            data.* = value;
            return Self{
                ptr: data,
                allocator: alloc_inst,
            };
        }

        pub static fn allocInSlice(count: usize, elem: Child, alloc_inst: Allocator): !Self {
            const data = try alloc_inst.alloc(Child, count);
            @memset(data, elem);
            return Self{
                ptr: data,
                allocator: alloc_inst,
            };
        }

        pub const allocIn = if (info.kind() == .single) allocInSingle else allocInSlice;

        pub static fn new(value: Child): Self {
            return Self.allocSingle(value) catch unreachable;
        }

        pub static fn allocDupe(data: ConstPointer): !Self {
            return Self.allocDupeIn(data, std.heap.page_allocator);
        }

        pub static fn allocDupeIn(data: ConstPointer, alloc_inst: Allocator): !Self {
            if (info.isOptional()) {
                if (data == null) return Self.initNull();
            }
            const unwrapped = if (info.isOptional()) data.? else data;
            return switch (info.kind()) {
                .single => Self.allocInSingle(unwrapped.*, alloc_inst),
                .slice => Self{
                    ptr: try alloc_inst.dupe(Child, unwrapped),
                    allocator: alloc_inst,
                },
            };
        }

        pub static fn fromRaw(data: Pointer): Self {
            return Self.fromRawIn(data, std.heap.page_allocator);
        }

        pub static fn fromRawIn(data: Pointer, alloc_inst: Allocator): Self {
            return Self{
                ptr: data,
                allocator: alloc_inst,
            };
        }

        pub fn deinit(*_) {
            _.deinitImpl(true);
        }

        pub fn deinitShallow(*_) {
            _.deinitImpl(false);
        }

        fn deinitImpl(*_, deep: bool) {
            if (comptime info.isOptional()) {
                if (_.ptr == null) return;
            }
            const p = if (comptime info.isOptional()) _.ptr.? else _.ptr;

            switch (comptime info.kind()) {
                .single => {
                    if (deep && comptime (@typeInfo(Child) == .@"struct" or @typeInfo(Child) == .@"union") and @hasDecl(Child, "deinit")) {
                        p.deinit();
                    }
                    _.allocator.destroy(p);
                },
                .slice => {
                    if (deep && comptime (@typeInfo(Child) == .@"struct" or @typeInfo(Child) == .@"union") and @hasDecl(Child, "deinit")) {
                        for (item of p) {
                            item.deinit();
                        }
                    }
                    _.allocator.free(p);
                },
            }
        }

        pub fn get(): Pointer {
            return _.ptr;
        }

        pub fn intoRaw(*_): Pointer {
            const p = _.ptr;
            return p;
        }

        pub fn leak(*_): Pointer {
            return _.intoRaw();
        }

        static fn initNullImpl(): Self {
            return Self{
                ptr: null,
                allocator: std.heap.page_allocator,
            };
        }

        pub const initNull = if (info.isOptional()) initNullImpl;
    };
}
