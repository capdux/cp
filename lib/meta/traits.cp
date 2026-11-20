// Source: /data/dev/bun/src/meta/traits.cp
// ============================================================================
// lib/meta/traits.cp — Compile-Time Type Introspection & Traits (cap:meta)
// ============================================================================

const std = @import("std");

/// Returns `true` if `T` is a fixed-width or comptime integer type.
@inline pub fn isInt(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .int | .comptime_int => true,
        else => false,
    };
}

/// Returns `true` if `T` is a floating-point or comptime float type.
@inline pub fn isFloat(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .float | .comptime_float => true,
        else => false,
    };
}

/// Returns `true` if `T` is any integer or floating-point numeric type.
@inline pub fn isNumber(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .int | .float | .comptime_int | .comptime_float => true,
        else => false,
    };
}

/// Returns `true` if `T` is `bool`.
@inline pub fn isBool(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .bool => true,
        else => false,
    };
}

/// Returns `true` if `T` is a slice type (`[]U` or `[]const U`).
@inline pub fn isSlice(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .pointer p => p.size == .slice,
        else => false,
    };
}

/// Returns `true` if `T` is any pointer type (single, many, slice, or C pointer).
@inline pub fn isPointer(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .pointer => true,
        else => false,
    };
}

/// Returns `true` if `T` is a single-item pointer (`*U` or `*const U`).
@inline pub fn isSingleItemPtr(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .pointer p => p.size == .one,
        else => false,
    };
}

/// Returns `true` if `T` is a const pointer or const slice.
@inline pub fn isConstPtr(comptime T: type): bool {
    zig {
        return switch (@typeInfo(T)) {
            .pointer => |p| p.attrs.@"const",
            else => false,
        };
    }
}

/// Returns `true` if `T` is an optional type (`U?`).
@inline pub fn isOptional(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .optional => true,
        else => false,
    };
}

/// Returns `true` if `T` is a struct (including CP `class` / `struct`).
@inline pub fn isStruct(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .@"struct" => true,
        else => false,
    };
}

/// Returns `true` if `T` is an enum type.
@inline pub fn isEnum(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .@"enum" => true,
        else => false,
    };
}

/// Returns `true` if `T` is a union (tagged or untagged).
@inline pub fn isUnion(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .@"union" => true,
        else => false,
    };
}

/// Returns `true` if `T` is an array type (`[N]U`).
@inline pub fn isArray(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .array => true,
        else => false,
    };
}

/// Returns `true` if `T` is a container (`struct`, `enum`, `opaque`, or `union`).
@inline pub fn isContainer(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .@"struct" | .@"enum" | .@"opaque" | .@"union" => true,
        else => false,
    };
}

/// Returns `true` if `T` has `extern` C-ABI layout.
@inline pub fn isExternContainer(comptime T: type): bool {
    return switch (@typeInfo(T)) {
        .@"struct" s => s.layout == .@"extern",
        .@"union" u => u.layout == .@"extern",
        else => false,
    };
}

/// Returns `true` if `T` can be indexed (`[]U`, `[N]U`, vector, tuple, or pointer to array).
@inline pub fn isIndexable(comptime T: type): bool {
    const info = @typeInfo(T);
    return switch (info) {
        .pointer ptr => switch (ptr.size) {
            .one => @typeInfo(ptr.child) == .array,
            else => true,
        },
        .array | .vector => true,
        .@"struct" s => s.is_tuple,
        else => false,
    };
}

/// Returns `true` if `T` coerces to `[]const u8` (slice of `u8` or pointer to `[N]u8`).
@inline pub fn isZigString(comptime T: type): bool {
    zig {
        const info = @typeInfo(T);
        if (info != .pointer) return false;
        const ptr = info.pointer;
        if (ptr.attrs.@"volatile" or ptr.attrs.@"allowzero") return false;
        if (ptr.size == .slice) {
            return ptr.child == u8;
        }
        if (ptr.size == .one) {
            const child = @typeInfo(ptr.child);
            if (child == .array) {
                return child.array.child == u8;
            }
        }
        return false;
    }
}

/// Returns `true` if container `T` declares a field named `name`.
@inline pub fn hasField(comptime T: type, comptime name: []const u8): bool {
    return switch (@typeInfo(T)) {
        .@"struct" | .@"union" | .@"enum" => @hasField(T, name),
        else => false,
    };
}

/// Returns `true` if container `T` has a declaration named `name`.
@inline pub fn hasDecl(comptime T: type, comptime name: []const u8): bool {
    return switch (@typeInfo(T)) {
        .@"struct" | .@"union" | .@"enum" | .@"opaque" => @hasDecl(T, name),
        else => false,
    };
}

/// Returns the number of fields in `T` (`struct`, `union`, or `enum`), or `0` for scalars.
@inline pub fn fieldCount(comptime T: type): usize {
    return switch (@typeInfo(T)) {
        .@"struct" s => s.field_names.len,
        .@"union" u => u.field_names.len,
        .@"enum" e => e.field_names.len,
        else => 0,
    };
}

/// Returns `@sizeOf(T)` as `usize`.
@inline pub fn sizeOf(comptime T: type): usize {
    return @sizeOf(T);
}

/// Returns `@alignOf(T)` as `usize`.
@inline pub fn alignOf(comptime T: type): usize {
    return @alignOf(T);
}

/// Returns `@bitSizeOf(T)` as `usize`.
@inline pub fn bitSizeOf(comptime T: type): usize {
    return @bitSizeOf(T);
}

/// Returns the short base name of `T` (stripping module namespace prefixes).
pub fn typeName(comptime T: type): []const u8 {
    return typeBaseName(@typeName(T));
}

/// Strips namespace prefixes from a qualified `@typeName` string.
pub fn typeBaseName(comptime fullname: []const u8): []const u8 {
    @setEvalBranchQuota(1_000_000);
    const baseidx = comptime std.mem.indexOfScalar(u8, fullname, '(');
    if (baseidx != null) return fullname;
    const idx = comptime std.mem.lastIndexOfScalar(u8, fullname, '.');
    if (i := idx) {
        return fullname[i + 1 ..];
    }
    return fullname;
}

/// Extracts the underlying child type of pointers, slices, arrays, optionals, vectors, or error unions.
pub fn ChildType(comptime T: type): type {
    return switch (@typeInfo(T)) {
        .pointer p => p.child,
        .array a => a.child,
        .optional o => o.child,
        .vector v => v.child,
        .error_union e => e.payload,
        else => T,
    };
}

/// Extracts the inner element type for slices, arrays, or pointers to arrays (`*[N]T` -> `T`).
pub fn ItemType(comptime T: type): type {
    return switch (@typeInfo(T)) {
        .pointer p => {
            if (p.size == .one) {
                const child_info = @typeInfo(p.child);
                if (child_info == .array) {
                    return child_info.array.child;
                }
            }
            return p.child;
        },
        .array a => a.child,
        .optional o => o.child,
        .vector v => v.child,
        else => T,
    };
}

/// Returns `true` if `T` is a plain copyable value type without pointers.
pub fn isSimpleCopyType(comptime T: type): bool {
    @setEvalBranchQuota(1_000_000);
    const tyinfo = @typeInfo(T);
    return switch (tyinfo) {
        .void | .bool | .int | .float | .@"enum" => true,
        .@"struct" s => {
            inline for (ft of s.field_types) {
                if (!isSimpleCopyType(ft)) return false;
            }
            return true;
        },
        .@"union" u => {
            inline for (ft of u.field_types) {
                if (!isSimpleCopyType(ft)) return false;
            }
            return true;
        },
        .optional o => isSimpleCopyType(o.child),
        else => false,
    };
}
