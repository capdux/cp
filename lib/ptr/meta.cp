// Source: Absorbed from /data/dev/bun/src/ptr/meta.cp | Synced: 2026-11-19T20:38:58Z
// Pointer reflection, metadata extraction, and type transformation utilities
import std from "std";

pub enum PointerKind {
    single,
    slice,
}

pub struct PointerInfo {
    Pointer: type,
    NonOptionalPointer: type,
    Child: type,

    pub fn kind(): PointerKind {
        return switch (@typeInfo(_.NonOptionalPointer).pointer.size) {
            .one => .single,
            .slice => .slice,
            else => @compileError("unreachable"),
        };
    }

    pub fn isOptional(): bool {
        return @typeInfo(_.Pointer) == .optional;
    }

    pub fn isConst(): bool {
        const ptr = @typeInfo(_.NonOptionalPointer).pointer;
        if (comptime @hasField(@TypeOf(ptr), "attrs")) {
            return ptr.attrs.@"const";
        } else if (comptime @hasField(@TypeOf(ptr), "is_const")) {
            return ptr.is_const;
        }
        return false;
    }

    pub const ParseOptions = struct {
        allow_const: bool = true,
        allow_slices: bool = true,
    };

    pub static fn parse(comptime Pointer: type, comptime options: ParseOptions): PointerInfo {
        const NonOptionalPointer = switch (@typeInfo(Pointer)) {
            .optional => |opt| opt.child,
            else => Pointer,
        };

        const pointer_info = switch (@typeInfo(NonOptionalPointer)) {
            .pointer => |ptr| ptr,
            else => @compileError("type must be a (possibly optional) pointer"),
        };
        const Child = pointer_info.child;

        switch (pointer_info.size) {
            .one => {},
            .slice => {
                if (!options.allow_slices) {
                    @compileError("slices not supported");
                }
            },
            else => @compileError("pointer size not supported"),
        }

        return PointerInfo{
            Pointer: Pointer,
            NonOptionalPointer: NonOptionalPointer,
            Child: Child,
        };
    }
}

pub fn AddConst(comptime Pointer: type): type {
    const info = @typeInfo(Pointer);
    if (info == .optional) {
        return ?AddConst(info.optional.child);
    }
    if (info == .pointer) {
        if (info.pointer.size == .slice) {
            zig {
                return []const info.pointer.child;
            }
        } else {
            zig {
                return *const info.pointer.child;
            }
        }
    }
    return Pointer;
}
