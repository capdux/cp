// Source: cp/lib/quant/scalar.cp (Scalar Int8 symmetric and affine quantization)

const std = @import("std");
import { QuantError, ScalarQuantParams, Int8Quantized, AffineQuantized } from "./types.cp";

pub fn quantizeInt8Symmetric(alloc: std.mem.Allocator, vec: []const f32): QuantError!Int8Quantized {
    if (vec.len == 0) {
        return QuantError.EmptyInput;
    }

    let max_abs: f32 = 0.0;
    let i: usize = 0;
    while (i < vec.len) {
        const v = @abs(vec[i]);
        if (v > max_abs) {
            max_abs = v;
        }
        i += 1;
    }

    const scale: f32 = if (max_abs == 0.0) 1.0 else max_abs / 127.0;
    const inv_scale: f32 = 1.0 / scale;

    let out = alloc.alloc(i8, vec.len) catch return QuantError.OutOfMemory;
    i = 0;
    while (i < vec.len) {
        if (max_abs == 0.0) {
            out[i] = 0;
        } else {
            let q = @round(vec[i] * inv_scale);
            if (q > 127.0) {
                q = 127.0;
            }
            if (q < -127.0) {
                q = -127.0;
            }
            out[i] = @as(i8, @intFromFloat(q));
        }
        i += 1;
    }

    return Int8Quantized{
        .data = out,
        .params = ScalarQuantParams{
            .scale = scale,
            .zero_point = 0,
        },
    };
}

pub fn quantizeInt8Affine(alloc: std.mem.Allocator, vec: []const f32): QuantError!AffineQuantized {
    if (vec.len == 0) {
        return QuantError.EmptyInput;
    }

    let min_val: f32 = vec[0];
    let max_val: f32 = vec[0];
    let i: usize = 1;
    while (i < vec.len) {
        if (vec[i] < min_val) {
            min_val = vec[i];
        }
        if (vec[i] > max_val) {
            max_val = vec[i];
        }
        i += 1;
    }

    const range = max_val - min_val;
    const scale: f32 = if (range == 0.0) 1.0 else range / 255.0;
    const zp_f: f32 = if (range == 0.0) 0.0 else @round(-min_val / scale);
    let zp_clamped = zp_f;
    if (zp_clamped < 0.0) {
        zp_clamped = 0.0;
    }
    if (zp_clamped > 255.0) {
        zp_clamped = 255.0;
    }
    const zero_point: i32 = @as(i32, @intFromFloat(zp_clamped));
    const inv_scale: f32 = 1.0 / scale;

    let out = alloc.alloc(u8, vec.len) catch return QuantError.OutOfMemory;
    i = 0;
    while (i < vec.len) {
        if (range == 0.0) {
            out[i] = 0;
        } else {
            let q = @round(vec[i] * inv_scale) + @as(f32, @floatFromInt(zero_point));
            if (q < 0.0) {
                q = 0.0;
            }
            if (q > 255.0) {
                q = 255.0;
            }
            out[i] = @as(u8, @intFromFloat(q));
        }
        i += 1;
    }

    return AffineQuantized{
        .data = out,
        .params = ScalarQuantParams{
            .scale = scale,
            .zero_point = zero_point,
        },
    };
}

pub fn dequantizeInt8(alloc: std.mem.Allocator, data: []const i8, params: ScalarQuantParams): QuantError![]f32 {
    if (data.len == 0) {
        return QuantError.EmptyInput;
    }

    let out = alloc.alloc(f32, data.len) catch return QuantError.OutOfMemory;
    let i: usize = 0;
    while (i < data.len) {
        const shifted = @as(i32, data[i]) - params.zero_point;
        out[i] = @as(f32, @floatFromInt(shifted)) * params.scale;
        i += 1;
    }
    return out;
}

pub fn dequantizeAffine(alloc: std.mem.Allocator, data: []const u8, params: ScalarQuantParams): QuantError![]f32 {
    if (data.len == 0) {
        return QuantError.EmptyInput;
    }

    let out = alloc.alloc(f32, data.len) catch return QuantError.OutOfMemory;
    let i: usize = 0;
    while (i < data.len) {
        const shifted = @as(i32, data[i]) - params.zero_point;
        out[i] = @as(f32, @floatFromInt(shifted)) * params.scale;
        i += 1;
    }
    return out;
}

pub fn dotInt8(a: []const i8, scale_a: f32, b: []const i8, scale_b: f32): QuantError!f32 {
    if (a.len != b.len) {
        return QuantError.DimensionMismatch;
    }
    if (a.len == 0) {
        return QuantError.EmptyInput;
    }

    let acc: i32 = 0;
    let i: usize = 0;
    while (i < a.len) {
        acc += @as(i32, a[i]) * @as(i32, b[i]);
        i += 1;
    }
    return @as(f32, @floatFromInt(acc)) * scale_a * scale_b;
}
