// Source: Absorbed from /data/dev/bun/src/meta/traits.cp, /data/dev/bun/src/ptr/meta.cp
//! Comptime type introspection traits and pointer metadata helpers.

/// Returns true if `T` is a byte slice or pointer to a byte array that coerces to `[]const u8`.
export fn isByteString(comptime T: type): bool {
    const info = @typeInfo(T);
    if (info != .pointer) return false;
    const ptr = info.pointer;
    if (ptr.is_volatile || ptr.is_allowzero) return false;
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

export fn isSlice(comptime T: type): bool {
    const info = @typeInfo(T);
    return info == .pointer && info.pointer.size == .slice;
}

export fn isSingleItemPtr(comptime T: type): bool {
    const info = @typeInfo(T);
    return info == .pointer && info.pointer.size == .one;
}

export fn isConstPtr(comptime T: type): bool {
    const info = @typeInfo(T);
    return info == .pointer && info.pointer.is_const;
}

export fn isOptional(comptime T: type): bool {
    return @typeInfo(T) == .optional;
}

export fn isInteger(comptime T: type): bool {
    const info = @typeInfo(T);
    return info == .int || info == .comptime_int;
}

export fn isFloat(comptime T: type): bool {
    const info = @typeInfo(T);
    return info == .float || info == .comptime_float;
}

export fn isNumber(comptime T: type): bool {
    return isInteger(T) || isFloat(T);
}

export fn isStruct(comptime T: type): bool {
    return @typeInfo(T) == .@"struct";
}

export fn isEnum(comptime T: type): bool {
    return @typeInfo(T) == .@"enum";
}

export fn isUnion(comptime T: type): bool {
    return @typeInfo(T) == .@"union";
}

export fn isContainer(comptime T: type): bool {
    const info = @typeInfo(T);
    return info == .@"struct" || info == .@"enum" || info == .@"opaque" || info == .@"union";
}

export fn isExternContainer(comptime T: type): bool {
    const info = @typeInfo(T);
    if (info == .@"struct") return info.@"struct".layout == .@"extern";
    if (info == .@"union") return info.@"union".layout == .@"extern";
    return false;
}

export fn isPackedContainer(comptime T: type): bool {
    const info = @typeInfo(T);
    if (info == .@"struct") return info.@"struct".layout == .@"packed";
    if (info == .@"union") return info.@"union".layout == .@"packed";
    return false;
}

export fn isIndexable(comptime T: type): bool {
    const info = @typeInfo(T);
    if (info == .pointer) {
        const ptr = info.pointer;
        if (ptr.size == .one) {
            return @typeInfo(ptr.child) == .array;
        }
        return true;
    }
    if (info == .array || info == .vector) return true;
    if (info == .@"struct") return info.@"struct".is_tuple;
    return false;
}

export enum PointerKind {
    single,
    slice,
}

export struct PointerParseOptions {
    allow_const: bool = true,
    allow_slices: bool = true,
}

export struct PointerMeta {
    kind: PointerKind,
    is_optional: bool,
    is_const: bool,
    alignment: usize,
    child_size: usize,

    pub static fn inspect(comptime Pointer: type, comptime options: PointerParseOptions): PointerMeta {
        const info0 = @typeInfo(Pointer);
        const is_opt = info0 == .optional;
        const NonOptionalPointer = if (is_opt) info0.optional.child else Pointer;

        const info1 = @typeInfo(NonOptionalPointer);
        if (info1 != .pointer) {
            @compileError("PointerMeta.inspect: type must be a (possibly optional) pointer");
        }
        const ptr = info1.pointer;
        const Child = ptr.child;

        const kind: PointerKind = if (ptr.size == .one)
            .single
        else if (ptr.size == .slice)
            .slice
        else
            @compileError("PointerMeta.inspect: only single-item pointers and slices are supported");

        if (kind == .slice && !options.allow_slices) {
            @compileError("PointerMeta.inspect: slices not allowed by options");
        }
        if (ptr.is_const && !options.allow_const) {
            @compileError("PointerMeta.inspect: const pointers not allowed by options");
        }
        if (ptr.is_volatile) {
            @compileError("PointerMeta.inspect: volatile pointers not supported");
        }
        if (ptr.is_allowzero) {
            @compileError("PointerMeta.inspect: allowzero pointers not supported");
        }

        return PointerMeta{
            kind: kind,
            is_optional: is_opt,
            is_const: ptr.is_const,
            alignment: ptr.alignment,
            child_size: @sizeOf(Child),
        };
    }
}
