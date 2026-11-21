// Source: Absorbed from /data/dev/bun/src/install/padding_checker.cp
// Compile-time struct layout & uninitialized padding verifier for safe binary serialization (sliceAsBytes).

/// Returns total implicit padding bytes inside struct `T` (comptime).
/// Returns 0 for packed structs, scalars, and structs with explicit padding fields.
pub fn paddingBytes(comptime T: type): usize {
    const info = @typeInfo(T);
    if (info != .@"struct") {
        return 0;
    }
    const s = info.@"struct";
    if (s.layout == .@"packed") {
        return 0;
    }
    let cursor: usize = 0;
    let total_pad: usize = 0;
    comptime for (field of s.fields) {
        const off = @offsetOf(T, field.name);
        if (off > cursor) {
            total_pad += off - cursor;
        }
        cursor = off + @sizeOf(field.@"type");
    }
    if (@sizeOf(T) > cursor) {
        total_pad += @sizeOf(T) - cursor;
    }
    return total_pad;
}

/// Returns true if struct `T` (or any nested field) contains implicit uninitialized padding bytes.
pub fn hasPadding(comptime T: type): bool {
    const info = @typeInfo(T);
    if (info == .@"struct") {
        if (paddingBytes(T) > 0) return true;
        comptime for (field of info.@"struct".fields) {
            if (hasPadding(field.@"type")) return true;
        }
        return false;
    }
    if (info == .array) {
        return hasPadding(info.array.child);
    }
    if (info == .optional) {
        return hasPadding(info.optional.child);
    }
    return false;
}

/// Returns true if `T` (or any nested struct/array/optional field) contains a pointer type,
/// which makes direct raw byte serialization (`std.mem.asBytes`) unsafe across processes.
pub fn hasPointers(comptime T: type): bool {
    const info = @typeInfo(T);
    if (info == .pointer) {
        return true;
    }
    if (info == .@"struct") {
        comptime for (field of info.@"struct".fields) {
            if (hasPointers(field.@"type")) return true;
        }
        return false;
    }
    if (info == .@"union") {
        comptime for (field of info.@"union".fields) {
            if (hasPointers(field.@"type")) return true;
        }
        return false;
    }
    if (info == .array) {
        return hasPointers(info.array.child);
    }
    if (info == .optional) {
        return hasPointers(info.optional.child);
    }
    return false;
}

/// Returns true if `T` is strictly binary-serializable (no pointers and no implicit padding).
pub fn isSafeBinarySerializable(comptime T: type): bool {
    return !hasPointers(T) && !hasPadding(T);
}

/// Compile-time assertion that `T` contains neither pointers nor uninitialized padding gaps.
pub fn assertNoUninitializedPadding(comptime T: type) {
    comptime {
        if (hasPointers(T)) {
            @compileError("Expected no pointer types in " ++ @typeName(T));
        }
        const pad = paddingBytes(T);
        if (pad > 0) {
            @compileError(std.fmt.comptimePrint(
                "Expected no uninitialized padding bytes in '{s}', found {d} implicit padding byte(s)",
                [ @typeName(T), pad ],
            ));
        }
        const info = @typeInfo(T);
        if (info == .@"struct") {
            comptime for (field of info.@"struct".fields) {
                assertNoUninitializedPadding(field.@"type");
            }
        } else if (info == .array) {
            assertNoUninitializedPadding(info.array.child);
        } else if (info == .optional) {
            assertNoUninitializedPadding(info.optional.child);
        }
    }
}
