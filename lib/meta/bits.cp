// Source: Absorbed from /data/dev/bun/src/meta/bits.cp | Synced: 2026-11-20T07:21:53Z
// ============================================================================
// lib/meta/bits.cp — Branchless Bit Manipulation, Alignment & Integer Packing
// ============================================================================

const std = @import("std");

/// Returns `true` if `v` is a power of two (`v > 0 && (v & (v - 1)) == 0`).
pub fn isPowerOfTwo(v: auto): bool {
    zig {
        const T = @TypeOf(v);
        if (T == comptime_int) {
            return v > 0 and (v & (v - 1)) == 0;
        }
        return v > 0 and (v & (v - 1)) == 0;
    }
}

fn BitOutType(comptime T: type): type {
    zig {
        if (T == comptime_int) return u64;
        return T;
    }
}

/// Returns the smallest power of two >= `v` (returns `1` when `v <= 1`).
pub fn nextPowerOfTwo(v: auto): u64 {
    zig {
        const val: u64 = @intCast(v);
        if (val <= 1) return 1;
        return std.math.ceilPowerOfTwo(u64, val) catch std.math.maxInt(u64);
    }
}

/// Aligns `value` upward to the nearest multiple of `alignment` (must be power of two).
pub fn alignUp(value: usize, alignment: usize): usize {
    if (alignment <= 1) return value;
    const mask = alignment - 1;
    return (value + mask) & ~mask;
}

/// Aligns `value` downward to the nearest multiple of `alignment` (must be power of two).
pub fn alignDown(value: usize, alignment: usize): usize {
    if (alignment <= 1) return value;
    const mask = alignment - 1;
    return value & ~mask;
}

/// Returns `true` if `value` is aligned to `alignment` (must be power of two).
pub fn isAligned(value: usize, alignment: usize): bool {
    if (alignment <= 1) return true;
    return (value & (alignment - 1)) == 0;
}

/// Counts the number of set bits (`1` bits) in `v` (hardware POPCNT).
pub fn bitCount(v: auto): usize {
    zig {
        const T = @TypeOf(v);
        if (T == comptime_int) {
            return @as(usize, @popCount(@as(u64, @intCast(v))));
        }
        return @as(usize, @popCount(v));
    }
}

/// Counts the number of leading zero bits in `v` (hardware CLZ).
/// Supports integer types and packed bitfield structs.
pub fn leadingZeros(v: auto): usize {
    zig {
        const T = @TypeOf(v);
        if (T == comptime_int) {
            return @as(usize, @clz(@as(u64, @intCast(v))));
        }
        const info = @typeInfo(T);
        if (info == .@"struct") {
            const BackInt = info.@"struct".backing_integer.?;
            const bits: BackInt = @bitCast(v);
            return @as(usize, @clz(bits));
        }
        return @as(usize, @clz(v));
    }
}

/// Counts the number of trailing zero bits in `v` (hardware CTZ).
pub fn trailingZeros(v: auto): usize {
    zig {
        const T = @TypeOf(v);
        if (T == comptime_int) {
            return @as(usize, @ctz(@as(u64, @intCast(v))));
        }
        const info = @typeInfo(T);
        if (info == .@"struct") {
            const BackInt = info.@"struct".backing_integer.?;
            const bits: BackInt = @bitCast(v);
            return @as(usize, @ctz(bits));
        }
        return @as(usize, @ctz(v));
    }
}

/// Reverses the bit order of `v` (defaults `comptime_int` literals to `u64`).
pub fn reverseBits(v: auto): BitOutType(@TypeOf(v)) {
    zig {
        const T = @TypeOf(v);
        if (T == comptime_int) {
            return @bitReverse(@as(u64, @intCast(v)));
        }
        return @bitReverse(v);
    }
}

/// Rotates `v` left by `shift` bits.
pub fn rotateLeft(v: auto, shift: auto): BitOutType(@TypeOf(v)) {
    zig {
        const T = @TypeOf(v);
        const OutT = if (T == comptime_int) u64 else T;
        const val: OutT = @intCast(v);
        return std.math.rotl(OutT, val, shift);
    }
}

/// Rotates `v` right by `shift` bits.
pub fn rotateRight(v: auto, shift: auto): BitOutType(@TypeOf(v)) {
    zig {
        const T = @TypeOf(v);
        const OutT = if (T == comptime_int) u64 else T;
        const val: OutT = @intCast(v);
        return std.math.rotr(OutT, val, shift);
    }
}

/// Packs high byte `hi` and low byte `lo` into a `u16`: `(hi << 8) | lo`.
pub fn packU16(hi: u8, lo: u8): u16 {
    return ((hi as u16) << 8) | (lo as u16);
}

/// Unpacks a `u16` into `[hi, lo]` bytes.
pub fn unpackU16(v: u16): [u8, u8] {
    zig {
        return .{ @as(u8, @intCast((v >> 8) & 0xFF)), @as(u8, @intCast(v & 0xFF)) };
    }
}

/// Packs high `u16` and low `u16` into a `u32`: `(hi << 16) | lo`.
pub fn packU32(hi: u16, lo: u16): u32 {
    return ((hi as u32) << 16) | (lo as u32);
}

/// Unpacks a `u32` into `[hi, lo]` `u16` halves.
pub fn unpackU32(v: u32): [u16, u16] {
    zig {
        return .{ @as(u16, @intCast((v >> 16) & 0xFFFF)), @as(u16, @intCast(v & 0xFFFF)) };
    }
}

/// Packs high `u32` and low `u32` into a `u64`: `(hi << 32) | lo`.
pub fn packU64(hi: u32, lo: u32): u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

/// Unpacks a `u64` into `[hi, lo]` `u32` halves.
pub fn unpackU64(v: u64): [u32, u32] {
    zig {
        return .{ @as(u32, @intCast((v >> 32) & 0xFFFFFFFF)), @as(u32, @intCast(v & 0xFFFFFFFF)) };
    }
}

// ============================================================================
// Packed Bitfield Helpers (absorbed from Bun src/meta/bits.zig)
// ============================================================================

fn BackingInt(comptime T: type): type {
    return @typeInfo(T).@"struct".backing_integer.?;
}

@inline pub fn asInt(comptime T: type, value: T): BackingInt(T) {
    return @bitCast(value);
}

@inline pub fn fromInt(comptime T: type, bits: BackingInt(T)): T {
    return @bitCast(bits);
}

@inline pub fn bitfieldIntersects(comptime T: type, lhs: T, rhs: T): bool {
    return (asInt(T, lhs) & asInt(T, rhs)) != 0;
}

@inline pub fn bitfieldAnd(comptime T: type, lhs: T, rhs: T): T {
    return fromInt(T, asInt(T, lhs) & asInt(T, rhs));
}

@inline pub fn bitfieldOr(comptime T: type, lhs: T, rhs: T): T {
    return fromInt(T, asInt(T, lhs) | asInt(T, rhs));
}

@inline pub fn bitfieldInvert(comptime T: type, value: T): T {
    return fromInt(T, ~asInt(T, value));
}

@inline pub fn bitfieldInsert(comptime T: type, lhs: *T, rhs: T) {
    lhs.* = bitfieldOr(T, lhs.*, rhs);
}

@inline pub fn bitfieldRemove(comptime T: type, lhs: *T, rhs: T) {
    lhs.* = bitfieldAnd(T, lhs.*, bitfieldInvert(T, rhs));
}

@inline pub fn bitfieldContains(comptime T: type, lhs: T, rhs: T): bool {
    return (asInt(T, lhs) & asInt(T, rhs)) == asInt(T, rhs);
}
