// Source: cp/lib/wasm_fmt/leb128.cp (Pure CP LEB128 unsigned & signed 32-bit codec)
const std = @import("std");
import { WasmError } from "./types.cp";

pub fn readUleb128(data: []const u8, offset: *usize): WasmError!u32 {
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

pub fn writeUleb128(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: u32): WasmError!void {
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

pub fn readSleb128(data: []const u8, offset: *usize): WasmError!i32 {
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

pub fn writeSleb128(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: i32): WasmError!void {
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
