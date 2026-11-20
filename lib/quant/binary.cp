// Source: cp/lib/quant/binary.cp (Binary 1-bit quantization and @popCount Hamming distance)

const std = @import("std");
import { QuantError, BinaryVector } from "./types.cp";

pub fn quantizeBinary(alloc: std.mem.Allocator, vec: []const f32): QuantError!BinaryVector {
    if (vec.len == 0) {
        return QuantError.EmptyInput;
    }

    const num_words: usize = (vec.len + 63) / 64;
    let bits = alloc.alloc(u64, num_words) catch return QuantError.OutOfMemory;
    let w: usize = 0;
    while (w < num_words) {
        bits[w] = 0;
        w += 1;
    }

    let i: usize = 0;
    while (i < vec.len) {
        if (vec[i] >= 0.0) {
            const word_idx = i / 64;
            const bit_idx: u6 = @intCast(i % 64);
            bits[word_idx] |= (@as(u64, 1) << bit_idx);
        }
        i += 1;
    }

    return BinaryVector{
        .bits = bits,
        .dim = vec.len,
    };
}

pub fn hammingDistance(a: BinaryVector, b: BinaryVector): QuantError!u32 {
    if (a.dim != b.dim) {
        return QuantError.DimensionMismatch;
    }
    if (a.dim == 0) {
        return QuantError.EmptyInput;
    }

    let dist: u32 = 0;
    let w: usize = 0;
    while (w < a.bits.len) {
        const diff = a.bits[w] ^ b.bits[w];
        dist += @as(u32, @popCount(diff));
        w += 1;
    }
    return dist;
}

pub fn binaryCosineApprox(a: BinaryVector, b: BinaryVector): QuantError!f32 {
    const dist = try hammingDistance(a, b);
    const ratio = @as(f32, @floatFromInt(dist)) / @as(f32, @floatFromInt(a.dim));
    return 1.0 - 2.0 * ratio;
}
