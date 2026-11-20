// Source: cp/cap/test/test_gguf_production_deep.cp (Production deep tests for cap:gguf & cap.gguf)
const std = @import("std");
const testing = std.testing;
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GGUF_MAGIC,
    GGUF_VERSION_3,
    GgufError,
    GgmlType,
    GgufBuilder,
    parseGguf,
    dequantizeQ8_0Block,
    dequantizeQ4_0Block,
    quantizeQ8_0Block,
    quantizeQ4_0Block,
} from "cap:gguf";

test("gguf_builder_and_parse_roundtrip") {
    const alloc = testing.allocator;
    let builder = GgufBuilder.init(alloc);
    defer builder.deinit();

    try builder.addString("general.architecture", "llama");
    try builder.addU32("llama.context_length", 8192);
    try builder.addF32("llama.rope.freq_base", 500000.0);
    try builder.addBool("general.quantized", true);

    const tok_arr = [ "<bos>", "hello", "world", "<eos>" ];
    try builder.addStringArray("tokenizer.ggml.tokens", &tok_arr);

    // Tensor 1: F32 [4] -> 16 bytes
    let f32_bytes: [16]u8 = undefined;
    const f32_vals: [4]f32 = [ 1.25, -2.5, 3.75, 0.0 ];
    let idx: usize = 0;
    while (idx < 4) : (idx += 1) {
        const bits: u32 = @bitCast(f32_vals[idx]);
        std.mem.writeInt(u32, f32_bytes[idx * 4 .. idx * 4 + 4][0..4], bits, .little);
    }
    const f32_dims: [1]u64 = [ 4 ];
    try builder.addTensor("blk.0.attn_norm.weight", &f32_dims, GgmlType.F32, &f32_bytes);

    // Tensor 2: Q8_0 [32] -> 34 bytes
    let q8_src: [32]f32 = undefined;
    let j: usize = 0;
    while (j < 32) : (j += 1) {
        q8_src[j] = @as(f32, @floatFromInt(j)) * 0.25 - 4.0;
    }
    const q8_block = quantizeQ8_0Block(&q8_src);
    const q8_dims: [1]u64 = [ 32 ];
    try builder.addTensor("blk.0.attn_q.weight", &q8_dims, GgmlType.Q8_0, &q8_block);

    const raw = try builder.build(alloc);
    defer alloc.free(raw);

    let parsed = try parseGguf(alloc, raw);
    defer parsed.deinit(alloc);

    try expect_eq(parsed.version, GGUF_VERSION_3);
    try expect_eq(parsed.alignment, @as(u64, 32));
    try expect_eq(parsed.kvs.len, @as(usize, 5));
    try expect_eq(parsed.tensors.len, @as(usize, 2));

    const arch = parsed.getString("general.architecture").?;
    try testing.expect(std.mem.eql(u8, arch, "llama"));
    try expect_eq(parsed.getU32("llama.context_length").?, @as(u32, 8192));
    try expect_eq(parsed.getF32("llama.rope.freq_base").?, @as(f32, 500000.0));
    try expect_eq(parsed.getBool("general.quantized").?, true);

    const tok_kv = parsed.getKv("tokenizer.ggml.tokens").?;
    try expect_eq(tok_kv.array_val.items.len, @as(usize, 4));
    try testing.expect(std.mem.eql(u8, tok_kv.array_val.items[2].string_val, "world"));

    const t1 = parsed.getTensor("blk.0.attn_norm.weight").?;
    const t1_data = try parsed.tensorData(t1);
    try testing.expect(std.mem.eql(u8, t1_data, &f32_bytes));

    const t2 = parsed.getTensor("blk.0.attn_q.weight").?;
    const t2_data = try parsed.tensorData(t2);
    try testing.expect(std.mem.eql(u8, t2_data, &q8_block));
}

test("gguf_custom_alignment_64_zero_copy") {
    const alloc = testing.allocator;
    let builder = GgufBuilder.init(alloc);
    defer builder.deinit();

    try builder.setAlignment(64);
    try builder.addString("general.name", "align-test");

    let t1_bytes: [8]u8 = [ 1, 2, 3, 4, 5, 6, 7, 8 ];
    const t1_dims: [1]u64 = [ 2 ];
    try builder.addTensor("t1", &t1_dims, GgmlType.F32, &t1_bytes);

    let t2_bytes: [4]u8 = [ 9, 10, 11, 12 ];
    const t2_dims: [1]u64 = [ 1 ];
    try builder.addTensor("t2", &t2_dims, GgmlType.F32, &t2_bytes);

    const raw = try builder.build(alloc);
    defer alloc.free(raw);

    let parsed = try parseGguf(alloc, raw);
    defer parsed.deinit(alloc);

    try expect_eq(parsed.alignment, @as(u64, 64));
    try expect_eq(parsed.data_offset % 64, @as(u64, 0));

    const t1 = parsed.getTensor("t1").?;
    const t2 = parsed.getTensor("t2").?;
    try expect_eq(t1.offset, @as(u64, 0));
    try expect_eq(t2.offset, @as(u64, 64));

    const s1 = try parsed.tensorData(t1);
    const s2 = try parsed.tensorData(t2);
    try testing.expect(std.mem.eql(u8, s1, &t1_bytes));
    try testing.expect(std.mem.eql(u8, s2, &t2_bytes));
}

test("gguf_q8_0_and_q4_0_dequantize_exact") {
    // Q8_0 exact block: d = 0.5 (f16 = 0x3800), qs[i] = i - 16
    let q8_raw: [34]u8 = undefined;
    std.mem.writeInt(u16, q8_raw[0..2], @as(u16, 0x3800), .little);
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        const q: i8 = @as(i8, @intCast(i)) - 16;
        q8_raw[2 + i] = @bitCast(q);
    }
    const q8_out = try dequantizeQ8_0Block(&q8_raw);
    i = 0;
    while (i < 32) : (i += 1) {
        const expected = 0.5 * @as(f32, @floatFromInt(@as(i32, @intCast(i)) - 16));
        try testing.expectApproxEqAbs(expected, q8_out[i], 1e-6);
    }

    // Q4_0 exact block: d = 0.25 (f16 = 0x3400), low nibble = 10 (+2), high nibble = 4 (-4)
    let q4_raw: [18]u8 = undefined;
    std.mem.writeInt(u16, q4_raw[0..2], @as(u16, 0x3400), .little);
    i = 0;
    while (i < 16) : (i += 1) {
        q4_raw[2 + i] = (4 << 4) | 10;
    }
    const q4_out = try dequantizeQ4_0Block(&q4_raw);
    i = 0;
    while (i < 16) : (i += 1) {
        try testing.expectApproxEqAbs(@as(f32, 0.5), q4_out[i], 1e-6);
        try testing.expectApproxEqAbs(@as(f32, -1.0), q4_out[i + 16], 1e-6);
    }

    // Roundtrip quantizeQ4_0Block -> dequantizeQ4_0Block
    let q4_src: [32]f32 = undefined;
    i = 0;
    while (i < 32) : (i += 1) {
        q4_src[i] = (@as(f32, @floatFromInt(i)) - 16.0) * 0.5;
    }
    const q4_packed = quantizeQ4_0Block(&q4_src);
    const q4_roundtrip = try dequantizeQ4_0Block(&q4_packed);
    try testing.expectApproxEqAbs(q4_src[0], q4_roundtrip[0], 1.0);
}

test("gguf_malformed_and_bomb_protection") {
    const alloc = testing.allocator;

    // 1. Invalid magic
    let bad_magic: [24]u8 = std.mem.zeroes([24]u8);
    std.mem.writeInt(u32, bad_magic[0..4], @as(u32, 0x12345678), .little);
    try testing.expectError(GgufError.InvalidMagic, parseGguf(alloc, &bad_magic));

    // 2. Unsupported version
    let bad_ver: [24]u8 = std.mem.zeroes([24]u8);
    std.mem.writeInt(u32, bad_ver[0..4], GGUF_MAGIC, .little);
    std.mem.writeInt(u32, bad_ver[4..8], @as(u32, 99), .little);
    try testing.expectError(GgufError.UnsupportedVersion, parseGguf(alloc, &bad_ver));

    // 3. Oversized array count bomb
    let arr_bomb: [64]u8 = std.mem.zeroes([64]u8);
    std.mem.writeInt(u32, arr_bomb[0..4], GGUF_MAGIC, .little);
    std.mem.writeInt(u32, arr_bomb[4..8], GGUF_VERSION_3, .little);
    std.mem.writeInt(u64, arr_bomb[8..16], @as(u64, 0), .little); // tensor_count
    std.mem.writeInt(u64, arr_bomb[16..24], @as(u64, 1), .little); // kv_count = 1
    std.mem.writeInt(u64, arr_bomb[24..32], @as(u64, 1), .little); // key len = 1
    arr_bomb[32] = 'k';
    std.mem.writeInt(u32, arr_bomb[33..37], @as(u32, 9), .little); // ARRAY
    std.mem.writeInt(u32, arr_bomb[37..41], @as(u32, 4), .little); // elem_type U32
    std.mem.writeInt(u64, arr_bomb[41..49], @as(u64, 5_000_000), .little); // > GGUF_MAX_ARRAY_LEN
    try testing.expectError(GgufError.ArrayTooLarge, parseGguf(alloc, &arr_bomb));

    // 4. Tensor offset out of bounds
    let builder = GgufBuilder.init(alloc);
    defer builder.deinit();
    const dims: [1]u64 = [ 4 ];
    const data: [16]u8 = std.mem.zeroes([16]u8);
    try builder.addTensor("w", &dims, GgmlType.F32, &data);
    const valid = try builder.build(alloc);
    defer alloc.free(valid);

    // Truncate last 4 bytes so tensor payload exceeds file length
    try testing.expectError(GgufError.TensorOutOfBounds, parseGguf(alloc, valid[0 .. valid.len - 4]));

    // 5. Invalid alignment (non-power-of-two)
    try testing.expectError(GgufError.InvalidAlignment, builder.setAlignment(48));
}

test("gguf_cap_namespace_and_leak_free") {
    const alloc = testing.allocator;
    let builder = cap.gguf.GgufBuilder.init(alloc);
    defer builder.deinit();

    try builder.addU64("general.file_type", 2);
    const buf = try builder.build(alloc);
    defer alloc.free(buf);

    let file = try cap.gguf.parseGguf(alloc, buf);
    defer file.deinit(alloc);

    try expect_eq(file.getU64("general.file_type").?, @as(u64, 2));
}
