// Source: cp/lib/gguf/quant.cp (Q8_0 & Q4_0 block quantization and dequantization)
const std = @import("std");
const types = @import("./types.cp");
const GgufError = types.GgufError;

pub const QK8_0: usize = 32;
pub const QK4_0: usize = 32;
pub const Q8_0_BLOCK_BYTES: usize = 34;
pub const Q4_0_BLOCK_BYTES: usize = 18;

fn readF16Le(bytes: []const u8): f32 {
    const bits = std.mem.readInt(u16, bytes[0..2], .little);
    const half: f16 = @bitCast(bits);
    return @floatCast(half);
}

fn writeF16Le(out: *[2]u8, val: f32) {
    const half: f16 = @floatCast(val);
    const bits: u16 = @bitCast(half);
    std.mem.writeInt(u16, out, bits, .little);
}

pub fn dequantizeQ8_0Block(raw: []const u8): GgufError![32]f32 {
    if (raw.len < Q8_0_BLOCK_BYTES) {
        return GgufError.UnexpectedEof;
    }
    const d = readF16Le(raw[0..2]);
    let out: [32]f32 = undefined;
    let i: usize = 0;
    while (i < QK8_0) : (i += 1) {
        const q: i8 = @bitCast(raw[2 + i]);
        out[i] = d * @as(f32, @floatFromInt(q));
    }
    return out;
}

pub fn dequantizeQ4_0Block(raw: []const u8): GgufError![32]f32 {
    if (raw.len < Q4_0_BLOCK_BYTES) {
        return GgufError.UnexpectedEof;
    }
    const d = readF16Le(raw[0..2]);
    let out: [32]f32 = undefined;
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const packed_byte = raw[2 + i];
        const x0: i32 = @as(i32, packed_byte & 0x0F) - 8;
        const x1: i32 = @as(i32, packed_byte >> 4) - 8;
        out[i] = d * @as(f32, @floatFromInt(x0));
        out[i + 16] = d * @as(f32, @floatFromInt(x1));
    }
    return out;
}

pub fn quantizeQ8_0Block(src: *const [32]f32): [34]u8 {
    let amax: f32 = 0.0;
    let i: usize = 0;
    while (i < QK8_0) : (i += 1) {
        const v = @abs(src[i]);
        if (v > amax) {
            amax = v;
        }
    }
    const d: f32 = amax / 127.0;
    const id: f32 = if (d != 0.0) (1.0 / d) else 0.0;

    let out: [34]u8 = undefined;
    writeF16Le(out[0..2], d);
    i = 0;
    while (i < QK8_0) : (i += 1) {
        const scaled = @round(src[i] * id);
        const clamped = @max(-127.0, @min(127.0, scaled));
        const q: i8 = @intFromFloat(clamped);
        out[2 + i] = @bitCast(q);
    }
    return out;
}

pub fn quantizeQ4_0Block(src: *const [32]f32): [18]u8 {
    let amax: f32 = 0.0;
    let max_val: f32 = 0.0;
    let i: usize = 0;
    while (i < QK4_0) : (i += 1) {
        const v = src[i];
        const av = @abs(v);
        if (av > amax) {
            amax = av;
            max_val = v;
        }
    }
    const d: f32 = max_val / -8.0;
    const id: f32 = if (d != 0.0) (1.0 / d) else 0.0;

    let out: [18]u8 = undefined;
    writeF16Le(out[0..2], d);
    i = 0;
    while (i < 16) : (i += 1) {
        const x0_f = src[i] * id + 8.5;
        const x1_f = src[i + 16] * id + 8.5;
        const q0: u8 = @intFromFloat(@max(0.0, @min(15.0, @floor(x0_f))));
        const q1: u8 = @intFromFloat(@max(0.0, @min(15.0, @floor(x1_f))));
        out[2 + i] = (q0 & 0x0F) | ((q1 & 0x0F) << 4);
    }
    return out;
}
