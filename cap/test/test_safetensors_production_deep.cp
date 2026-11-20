// Source: cp/cap/test/test_safetensors_production_deep.cp (Deep production tests for cap:safetensors)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Dtype,
    dtypeSize,
    bf16ToF32,
    f32ToBf16,
    f16ToF32,
    f32ToF16,
    SafetensorsBuilder,
    parseSafetensors,
} from "cap:safetensors";

test("bf16 and f16 IEEE-754 precision, zero, negative, Inf and NaN") {
    // BF16 round-to-nearest-even & special values
    expect_eq(f32ToBf16(0.0), @as(u16, 0x0000));
    expect_eq(f32ToBf16(-0.0), @as(u16, 0x8000));
    expect_eq(f32ToBf16(1.0), @as(u16, 0x3F80));
    expect_eq(f32ToBf16(-2.0), @as(u16, 0xC000));
    expect_eq(bf16ToF32(0x3F80), @as(f32, 1.0));
    expect_eq(bf16ToF32(0xC000), @as(f32, -2.0));

    const inf_bf16 = f32ToBf16(std.math.inf(f32));
    expect_eq(inf_bf16, @as(u16, 0x7F80));
    expect_eq(std.math.isInf(bf16ToF32(inf_bf16)), true);

    const nan_bf16 = f32ToBf16(std.math.nan(f32));
    expect_eq(std.math.isNan(bf16ToF32(nan_bf16)), true);

    // FP16 normal, subnormal, negative, Inf, NaN
    expect_eq(f32ToF16(0.0), @as(u16, 0x0000));
    expect_eq(f32ToF16(-0.0), @as(u16, 0x8000));
    expect_eq(f32ToF16(1.0), @as(u16, 0x3C00));
    expect_eq(f32ToF16(-2.5), @as(u16, 0xC100));
    expect_eq(f16ToF32(0x3C00), @as(f32, 1.0));
    expect_eq(f16ToF32(0xC100), @as(f32, -2.5));

    // FP16 smallest subnormal: 0x0001 = 2^-24
    const sub_val = f16ToF32(0x0001);
    expect_eq(f32ToF16(sub_val), @as(u16, 0x0001));

    const inf_f16 = f32ToF16(std.math.inf(f32));
    expect_eq(inf_f16, @as(u16, 0x7C00));
    expect_eq(std.math.isInf(f16ToF32(inf_f16)), true);

    const nan_f16 = f32ToF16(std.math.nan(f32));
    expect_eq(std.math.isNan(f16ToF32(nan_f16)), true);

    expect_eq(dtypeSize(.BF16), @as(usize, 2));
    expect_eq(dtypeSize(.F32), @as(usize, 4));
    expect_eq(dtypeSize(.F64), @as(usize, 8));
}

test("SafetensorsBuilder multi-tensor roundtrip with metadata and zero-copy view") {
    const alloc = std.testing.allocator;
    let builder = SafetensorsBuilder.init(alloc);
    defer builder.deinit();

    try builder.addMetadata("format", "pt");
    try builder.addMetadata("model", "tiny-cp");

    const f32_data: [4]f32 = [1.5, -3.25, 0.0, 42.0];
    const bf16_data: [2]f32 = [1.0, -2.0];
    const i32_data: [3]i32 = [10, -20, 30];
    const shape_2x2: [2]usize = [2, 2];
    const shape_2: [1]usize = [2];
    const shape_3: [1]usize = [3];

    try builder.addTensorF32("proj.weight", &shape_2x2, &f32_data);
    try builder.addTensorBF16FromF32("norm.scale", &shape_2, &bf16_data);
    try builder.addTensorI32("step", &shape_3, &i32_data);

    const bytes = try builder.build(alloc);
    defer alloc.free(bytes);

    let sf = try parseSafetensors(alloc, bytes);
    defer sf.deinit();

    expect_eq(sf.tensorCount(), @as(usize, 3));
    const fmt_meta = sf.getMetadata("format") orelse return error.TestUnexpectedResult;
    expect_eq(std.mem.eql(u8, fmt_meta, "pt"), true);

    const proj = sf.getTensor("proj.weight") orelse return error.TestUnexpectedResult;
    expect_eq(proj.dtype, Dtype.F32);
    expect_eq(proj.shape.len, @as(usize, 2));
    expect_eq(proj.shape[0], @as(usize, 2));
    expect_eq(proj.shape[1], @as(usize, 2));
    expect_eq(proj.data.len, @as(usize, 16));

    const step = sf.getTensor("step") orelse return error.TestUnexpectedResult;
    expect_eq(step.dtype, Dtype.I32);
    expect_eq(step.data.len, @as(usize, 12));
    expect_eq(std.mem.readInt(i32, step.data[0..4], .little), @as(i32, 10));
    expect_eq(std.mem.readInt(i32, step.data[4..8], .little), @as(i32, -20));
}

test("tensorAsF32 decodes F32, BF16 and F16 tensors") {
    const alloc = std.testing.allocator;
    let builder = SafetensorsBuilder.init(alloc);
    defer builder.deinit();

    const vals: [4]f32 = [1.0, -2.5, 3.25, 0.5];
    const shape_4: [1]usize = [4];
    try builder.addTensorBF16FromF32("w_bf16", &shape_4, &vals);
    try builder.addTensorF16FromF32("w_f16", &shape_4, &vals);

    const bytes = try builder.build(alloc);
    defer alloc.free(bytes);

    let sf = try parseSafetensors(alloc, bytes);
    defer sf.deinit();

    const bf_out = try sf.tensorAsF32(alloc, "w_bf16");
    defer alloc.free(bf_out);
    expect_eq(bf_out.len, @as(usize, 4));
    expect_eq(bf_out[0], @as(f32, 1.0));
    expect_eq(bf_out[1], @as(f32, -2.5));
    expect_eq(bf_out[2], @as(f32, 3.25));
    expect_eq(bf_out[3], @as(f32, 0.5));

    const f16_out = try sf.tensorAsF32(alloc, "w_f16");
    defer alloc.free(f16_out);
    expect_eq(f16_out.len, @as(usize, 4));
    expect_eq(f16_out[0], @as(f32, 1.0));
    expect_eq(f16_out[1], @as(f32, -2.5));
    expect_eq(f16_out[2], @as(f32, 3.25));
    expect_eq(f16_out[3], @as(f32, 0.5));
}

fn makeRawSafetensors(alloc: std.mem.Allocator, header_json: []const u8, payload_len: usize): ![]u8 {
    let buf = try alloc.alloc(u8, 8 + header_json.len + payload_len);
    std.mem.writeInt(u64, buf[0..8], @as(u64, header_json.len), .little);
    @memcpy(buf[8 .. 8 + header_json.len], header_json);
    @memset(buf[8 + header_json.len .. buf.len], 0);
    return buf;
}

test("security validation: oversized header, out-of-bounds offsets, shape mismatch, overlap") {
    const alloc = std.testing.allocator;

    // 1. Oversized header (> 100 MiB)
    let huge_hdr: [16]u8 = undefined;
    std.mem.writeInt(u64, huge_hdr[0..8], @as(u64, 200 * 1024 * 1024), .little);
    @memset(huge_hdr[8..16], ' ');
    let got_huge = false;
    * = parseSafetensors(alloc, &huge_hdr) catch |e| {
        if (e == error.HeaderTooLarge) { got_huge = true; }
    };
    expect_eq(got_huge, true);

    // 2. Out-of-bounds data_offsets
    const oob_json = "{\"a\":{\"dtype\":\"F32\",\"shape\":[2],\"data_offsets\":[0,8]}}";
    const oob_buf = try makeRawSafetensors(alloc, oob_json, 4);
    defer alloc.free(oob_buf);
    let got_oob = false;
    * = parseSafetensors(alloc, oob_buf) catch |e| {
        if (e == error.OutOfBounds) { got_oob = true; }
    };
    expect_eq(got_oob, true);

    // 3. Shape product vs byte span mismatch
    const mis_json = "{\"a\":{\"dtype\":\"F32\",\"shape\":[3],\"data_offsets\":[0,8]}}";
    const mis_buf = try makeRawSafetensors(alloc, mis_json, 8);
    defer alloc.free(mis_buf);
    let got_mis = false;
    * = parseSafetensors(alloc, mis_buf) catch |e| {
        if (e == error.ShapeSizeMismatch) { got_mis = true; }
    };
    expect_eq(got_mis, true);

    // 4. Overlapping tensor intervals
    const ovl_json = "{\"a\":{\"dtype\":\"F32\",\"shape\":[2],\"data_offsets\":[0,8]},\"b\":{\"dtype\":\"F32\",\"shape\":[2],\"data_offsets\":[4,12]}}";
    const ovl_buf = try makeRawSafetensors(alloc, ovl_json, 12);
    defer alloc.free(ovl_buf);
    let got_ovl = false;
    * = parseSafetensors(alloc, ovl_buf) catch |e| {
        if (e == error.OverlappingTensors) { got_ovl = true; }
    };
    expect_eq(got_ovl, true);
}

test("cap.safetensors dynamic mount and zero-leak allocator check") {
    const alloc = std.testing.allocator;
    let builder = cap.safetensors.SafetensorsBuilder.init(alloc);
    defer builder.deinit();

    const shape_2: [1]usize = [2];
    const vec_vals: [2]f32 = [7.0, -9.0];
    try builder.addTensorF32("vec", &shape_2, &vec_vals);
    const bytes = try builder.build(alloc);
    defer alloc.free(bytes);

    let sf = try cap.safetensors.parseSafetensors(alloc, bytes);
    defer sf.deinit();

    const vals = try sf.tensorAsF32(alloc, "vec");
    defer alloc.free(vals);
    expect_eq(vals[0], @as(f32, 7.0));
    expect_eq(vals[1], @as(f32, -9.0));
}
