// Source: cp/lib/wasm_fmt/leb128.cp (Pure CP LEB128 unsigned & signed 32-bit/64-bit codec)
// Reference-hardened against bytecodealliance/wasm-tools (Apache-2.0)
const std = @import("std");
import { WasmError } from "./wasmTypes.cp";

pub fn readUleb128(data: []const u8, offset: *usize): u32 ! WasmError {
    let result: u32 = 0;
    let shift: u6 = 0;
    let count: usize = 0;

    while (true) {
        if (offset.* >= data.len) {
            return WasmError.UnexpectedEof;
        }
        const byte = data[offset.*];
        offset.* += 1;
        count += 1;

        if (count > 5) {
            return WasmError.Leb128Overflow;
        }
        if (count == 5) {
            if ((byte & 0x80) != 0 or (byte & 0xF0) != 0) {
                return WasmError.Leb128Overflow;
            }
        }

        const low: u32 = @as(u32, byte & 0x7F);
        result |= low << @as(u5, @intCast(shift));

        if ((byte & 0x80) == 0) {
            return result;
        }
        shift += 7;
    }
}

pub fn writeUleb128(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: u32): ! {
    let v = value;
    while (true) {
        const byte_val: u8 = @intCast(v & 0x7F);
        v = v >> 7;
        if (v == 0) {
            try out.append(alloc, byte_val);
            return;
        }
        try out.append(alloc, byte_val | 0x80);
    }
}

pub fn readSleb128(data: []const u8, offset: *usize): i32 ! WasmError {
    let result: u32 = 0;
    let shift: u6 = 0;
    let count: usize = 0;

    while (true) {
        if (offset.* >= data.len) {
            return WasmError.UnexpectedEof;
        }
        const byte = data[offset.*];
        offset.* += 1;
        count += 1;

        if (count > 5) {
            return WasmError.Leb128Overflow;
        }
        if (count == 5) {
            if ((byte & 0x80) != 0) {
                return WasmError.Leb128Overflow;
            }
            const sign_is_neg = (byte & 0x40) != 0;
            if (sign_is_neg) {
                if ((byte & 0x78) != 0x78) {
                    return WasmError.Leb128Overflow;
                }
            } else {
                if ((byte & 0x78) != 0x00) {
                    return WasmError.Leb128Overflow;
                }
            }
            const top: u32 = @as(u32, byte & 0x0F);
            result |= top << 28;
            return @bitCast(result);
        }

        const low: u32 = @as(u32, byte & 0x7F);
        result |= low << @as(u5, @intCast(shift));
        shift += 7;

        if ((byte & 0x80) == 0) {
            if ((byte & 0x40) != 0) {
                const ones: u32 = ~@as(u32, 0);
                result |= ones << @as(u5, @intCast(shift));
            }
            return @bitCast(result);
        }
    }
}

pub fn writeSleb128(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: i32): ! {
    let v = value;
    while (true) {
        const bits: u32 = @bitCast(v);
        const byte_val: u8 = @intCast(bits & 0x7F);
        v = v >> 7;
        const sign_bit = (byte_val & 0x40) != 0;
        if ((v == 0 and !sign_bit) or (v == -1 and sign_bit)) {
            try out.append(alloc, byte_val);
            return;
        }
        try out.append(alloc, byte_val | 0x80);
    }
}

/// Decodes a canonical WebAssembly unsigned 64-bit LEB128 integer (max 10 bytes, 10th byte <= 0x01).
pub fn readUleb128U64(data: []const u8, offset: *usize): u64 ! WasmError {
    let result: u64 = 0;
    let shift: u7 = 0;
    let count: usize = 0;

    while (true) {
        if (offset.* >= data.len) {
            return WasmError.UnexpectedEof;
        }
        const byte = data[offset.*];
        offset.* += 1;
        count += 1;

        if (count > 10) {
            return WasmError.Leb128Overflow;
        }
        if (count == 10) {
            if ((byte & 0x80) != 0 or byte > 0x01) {
                return WasmError.Leb128Overflow;
            }
            result |= @as(u64, byte & 0x01) << 63;
            return result;
        }

        const low: u64 = @as(u64, byte & 0x7F);
        result |= low << @as(u6, @intCast(shift));

        if ((byte & 0x80) == 0) {
            return result;
        }
        shift += 7;
    }
}

/// Encodes an unsigned 64-bit integer as canonical LEB128.
pub fn writeUleb128U64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: u64): ! {
    let v = value;
    while (true) {
        const byte_val: u8 = @intCast(v & 0x7F);
        v = v >> 7;
        if (v == 0) {
            try out.append(alloc, byte_val);
            return;
        }
        try out.append(alloc, byte_val | 0x80);
    }
}

/// Decodes a canonical WebAssembly signed 64-bit LEB128 integer (max 10 bytes, 10th byte sign-extended).
pub fn readSleb128I64(data: []const u8, offset: *usize): i64 ! WasmError {
    let result: u64 = 0;
    let shift: u7 = 0;
    let count: usize = 0;

    while (true) {
        if (offset.* >= data.len) {
            return WasmError.UnexpectedEof;
        }
        const byte = data[offset.*];
        offset.* += 1;
        count += 1;

        if (count > 10) {
            return WasmError.Leb128Overflow;
        }
        if (count == 10) {
            if ((byte & 0x80) != 0) {
                return WasmError.Leb128Overflow;
            }
            const sign_is_neg = (byte & 0x40) != 0;
            if (sign_is_neg) {
                if (byte != 0x7F) {
                    return WasmError.Leb128Overflow;
                }
            } else {
                if (byte != 0x00) {
                    return WasmError.Leb128Overflow;
                }
            }
            const top: u64 = @as(u64, byte & 0x01);
            result |= top << 63;
            return @bitCast(result);
        }

        const low: u64 = @as(u64, byte & 0x7F);
        result |= low << @as(u6, @intCast(shift));
        shift += 7;

        if ((byte & 0x80) == 0) {
            if ((byte & 0x40) != 0) {
                const ones: u64 = ~@as(u64, 0);
                result |= ones << @as(u6, @intCast(shift));
            }
            return @bitCast(result);
        }
    }
}

/// Encodes a signed 64-bit integer as canonical LEB128.
pub fn writeSleb128I64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: i64): ! {
    let v = value;
    while (true) {
        const bits: u64 = @bitCast(v);
        const byte_val: u8 = @intCast(bits & 0x7F);
        v = v >> 7;
        const sign_bit = (byte_val & 0x40) != 0;
        if ((v == 0 and !sign_bit) or (v == -1 and sign_bit)) {
            try out.append(alloc, byte_val);
            return;
        }
        try out.append(alloc, byte_val | 0x80);
    }
}
