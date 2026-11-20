// Source: cp/lib/wasm/leb128.cp (WebAssembly Unsigned & Signed LEB128 Codec)
// Pure CP WebAssembly 1.0 LEB128 variable-length integer codec with strict
// 5-byte (32-bit) and 10-byte (64-bit) overflow, sign-bit, and EOF validation.

const std = @import("std");

/// Errors produced by WebAssembly LEB128 encoding/decoding and module parsing/building.
pub err WasmError {
    InvalidMagic,
    UnsupportedVersion,
    InvalidSectionOrder,
    UnexpectedEof,
    Leb128Overflow,
    InvalidValType,
    InvalidExportKind,
    MalformedSection,
    OutOfMemory,
}

/// WebAssembly 1.0 Unsigned and Signed LEB128 codec (`u32`, `u64`, `i32`, `i64`).
pub static class Leb128 {
    /// Appends an unsigned 32-bit integer as canonical LEB128 (1..5 bytes) into `out`.
    fn writeU32(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: u32): void ! WasmError {
        let v: u32 = value;
        while (v >= 0x80) {
            const byte: u8 = ((v & 0x7F) as u8) | 0x80;
            out.append(alloc, byte) catch return WasmError.OutOfMemory;
            v >>= 7;
        }
        out.append(alloc, (v & 0x7F) as u8) catch return WasmError.OutOfMemory;
    }

    /// Appends an unsigned 64-bit integer as canonical LEB128 (1..10 bytes) into `out`.
    fn writeU64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: u64): void ! WasmError {
        let v: u64 = value;
        while (v >= 0x80) {
            const byte: u8 = ((v & 0x7F) as u8) | 0x80;
            out.append(alloc, byte) catch return WasmError.OutOfMemory;
            v >>= 7;
        }
        out.append(alloc, (v & 0x7F) as u8) catch return WasmError.OutOfMemory;
    }

    /// Appends a signed 32-bit integer as canonical two's-complement LEB128 (1..5 bytes) into `out`.
    fn writeI32(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: i32): void ! WasmError {
        let v: i32 = value;
        while (true) {
            const raw: u32 = @bitCast(v);
            let byte: u8 = (raw & 0x7F) as u8;
            const sign_bit = (byte & 0x40) != 0;
            v = v >> 7;
            if ((v == 0 && !sign_bit) || (v == -1 && sign_bit)) {
                out.append(alloc, byte) catch return WasmError.OutOfMemory;
                break;
            }
            byte |= 0x80;
            out.append(alloc, byte) catch return WasmError.OutOfMemory;
        }
    }

    /// Appends a signed 64-bit integer as canonical two's-complement LEB128 (1..10 bytes) into `out`.
    fn writeI64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: i64): void ! WasmError {
        let v: i64 = value;
        while (true) {
            const raw: u64 = @bitCast(v);
            let byte: u8 = (raw & 0x7F) as u8;
            const sign_bit = (byte & 0x40) != 0;
            v = v >> 7;
            if ((v == 0 && !sign_bit) || (v == -1 && sign_bit)) {
                out.append(alloc, byte) catch return WasmError.OutOfMemory;
                break;
            }
            byte |= 0x80;
            out.append(alloc, byte) catch return WasmError.OutOfMemory;
        }
    }

    /// Encodes a `u32` into an owned LEB128 byte slice. Caller owns the slice.
    fn encodeU32(alloc: std.mem.Allocator, value: u32): ([]u8) ! WasmError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        Leb128.writeU32(&out, alloc, value)?;
        return out.toOwnedSlice(alloc) catch return WasmError.OutOfMemory;
    }

    /// Encodes a `u64` into an owned LEB128 byte slice. Caller owns the slice.
    fn encodeU64(alloc: std.mem.Allocator, value: u64): ([]u8) ! WasmError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        Leb128.writeU64(&out, alloc, value)?;
        return out.toOwnedSlice(alloc) catch return WasmError.OutOfMemory;
    }

    /// Encodes an `i32` into an owned signed LEB128 byte slice. Caller owns the slice.
    fn encodeI32(alloc: std.mem.Allocator, value: i32): ([]u8) ! WasmError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        Leb128.writeI32(&out, alloc, value)?;
        return out.toOwnedSlice(alloc) catch return WasmError.OutOfMemory;
    }

    /// Encodes an `i64` into an owned signed LEB128 byte slice. Caller owns the slice.
    fn encodeI64(alloc: std.mem.Allocator, value: i64): ([]u8) ! WasmError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        Leb128.writeI64(&out, alloc, value)?;
        return out.toOwnedSlice(alloc) catch return WasmError.OutOfMemory;
    }

    /// Decodes an unsigned 32-bit LEB128 integer at `bytes[pos.*..]`, advancing `pos.*`.
    /// Rejects encodings exceeding 5 bytes or carrying non-zero unused upper bits (> 32 bits).
    fn decodeU32(bytes: []const u8, pos: *usize): u32 ! WasmError {
        let result: u32 = 0;
        let shift: u5 = 0;
        let i: usize = 0;
        while (i < 5) : (i += 1) {
            if (pos.* >= bytes.len) {
                return WasmError.UnexpectedEof;
            }
            const b: u8 = bytes[pos.*];
            pos.* += 1;

            if (i == 4) {
                if ((b & 0x80) != 0 || b >= 0x10) {
                    return WasmError.Leb128Overflow;
                }
                result |= (b as u32) << shift;
                return result;
            }

            result |= ((b & 0x7F) as u32) << shift;
            if ((b & 0x80) == 0) {
                return result;
            }
            shift += 7;
        }
        return WasmError.Leb128Overflow;
    }

    /// Decodes an unsigned 64-bit LEB128 integer at `bytes[pos.*..]`, advancing `pos.*`.
    /// Rejects encodings exceeding 10 bytes or carrying non-zero unused upper bits (> 64 bits).
    fn decodeU64(bytes: []const u8, pos: *usize): u64 ! WasmError {
        let result: u64 = 0;
        let shift: u6 = 0;
        let i: usize = 0;
        while (i < 10) : (i += 1) {
            if (pos.* >= bytes.len) {
                return WasmError.UnexpectedEof;
            }
            const b: u8 = bytes[pos.*];
            pos.* += 1;

            if (i == 9) {
                if ((b & 0x80) != 0 || b > 1) {
                    return WasmError.Leb128Overflow;
                }
                result |= (b as u64) << shift;
                return result;
            }

            result |= ((b & 0x7F) as u64) << shift;
            if ((b & 0x80) == 0) {
                return result;
            }
            shift += 7;
        }
        return WasmError.Leb128Overflow;
    }

    /// Decodes a signed 32-bit LEB128 integer at `bytes[pos.*..]`, advancing `pos.*`.
    /// Rejects encodings exceeding 5 bytes or invalid sign-extension bits in the 5th byte.
    fn decodeI32(bytes: []const u8, pos: *usize): i32 ! WasmError {
        let result: u32 = 0;
        let shift: u5 = 0;
        let i: usize = 0;
        while (i < 5) : (i += 1) {
            if (pos.* >= bytes.len) {
                return WasmError.UnexpectedEof;
            }
            const b: u8 = bytes[pos.*];
            pos.* += 1;

            if (i == 4) {
                if ((b & 0x80) != 0) {
                    return WasmError.Leb128Overflow;
                }
                if ((b & 0x08) != 0) {
                    if ((b & 0x70) != 0x70) {
                        return WasmError.Leb128Overflow;
                    }
                } else {
                    if ((b & 0x70) != 0) {
                        return WasmError.Leb128Overflow;
                    }
                }
                result |= ((b & 0x0F) as u32) << shift;
                return @bitCast(result);
            }

            result |= ((b & 0x7F) as u32) << shift;
            if ((b & 0x80) == 0) {
                if ((b & 0x40) != 0) {
                    const sign_shift: u5 = shift + 7;
                    result |= (0xFFFFFFFF as u32) << sign_shift;
                }
                return @bitCast(result);
            }
            shift += 7;
        }
        return WasmError.Leb128Overflow;
    }

    /// Decodes a signed 64-bit LEB128 integer at `bytes[pos.*..]`, advancing `pos.*`.
    /// Rejects encodings exceeding 10 bytes or invalid sign-extension bits in the 10th byte.
    fn decodeI64(bytes: []const u8, pos: *usize): i64 ! WasmError {
        let result: u64 = 0;
        let shift: u6 = 0;
        let i: usize = 0;
        while (i < 10) : (i += 1) {
            if (pos.* >= bytes.len) {
                return WasmError.UnexpectedEof;
            }
            const b: u8 = bytes[pos.*];
            pos.* += 1;

            if (i == 9) {
                if ((b & 0x80) != 0) {
                    return WasmError.Leb128Overflow;
                }
                if ((b & 0x01) != 0) {
                    if ((b & 0x7E) != 0x7E) {
                        return WasmError.Leb128Overflow;
                    }
                } else {
                    if ((b & 0x7E) != 0) {
                        return WasmError.Leb128Overflow;
                    }
                }
                result |= ((b & 0x01) as u64) << shift;
                return @bitCast(result);
            }

            result |= ((b & 0x7F) as u64) << shift;
            if ((b & 0x80) == 0) {
                if ((b & 0x40) != 0) {
                    const sign_shift: u6 = shift + 7;
                    result |= (0xFFFFFFFFFFFFFFFF as u64) << sign_shift;
                }
                return @bitCast(result);
            }
            shift += 7;
        }
        return WasmError.Leb128Overflow;
    }
}
