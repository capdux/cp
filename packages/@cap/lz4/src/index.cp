// @cap/lz4 — Extremely fast lossless compression (LZ4 & LZ4-HC v1.10.0) for CP
// License: BSD-2-Clause (Upstream: Yann Collet / lz4)

import std from "std";

pub @c {
    fn LZ4_versionNumber(): i32;
    fn LZ4_compressBound(inputSize: i32): i32;
    fn LZ4_compress_default(src: [*]const u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32): i32;
    fn LZ4_compress_fast(src: [*]const u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, acceleration: i32): i32;
    fn LZ4_compress_HC(src: [*]const u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, compressionLevel: i32): i32;
    fn LZ4_decompress_safe(src: [*]const u8!, dst: [*]u8!, compressedSize: i32, dstCapacity: i32): i32;
}

pub err Lz4Error {
    InputTooLarge,
    OutputTooSmall,
    CompressFailed,
    DecompressFailed,
    OutOfMemory,
}

/// Returns the linked LZ4 library version number (e.g. 11000 for 1.10.0).
pub fn versionNumber(): i32 {
    unsafe {
        return LZ4_versionNumber();
    }
}

/// Computes the worst-case compressed buffer size in bytes for an input of length `input_len`.
pub fn compressBound(input_len: usize): usize! {
    if (input_len > 0x7E000000) return Lz4Error.InputTooLarge;
    let bound: i32 = 0;
    unsafe {
        bound = LZ4_compressBound(@intCast(input_len));
    }
    if (bound <= 0) return Lz4Error.InputTooLarge;
    return @intCast(bound);
}

/// Compresses `src` into caller-provided `dst` using standard LZ4 block compression.
/// Returns the compressed slice view into `dst`.
pub fn compressInto(src: []const u8, dst: []u8): []const u8! {
    if (src.len > 0x7E000000) return Lz4Error.InputTooLarge;
    if (dst.len > 0x7FFFFFFF) return Lz4Error.OutputTooSmall;
    if (src.len == 0) return dst[0..0];
    let written: i32 = 0;
    unsafe {
        written = LZ4_compress_default(src.ptr, dst.ptr, @intCast(src.len), @intCast(dst.len));
    }
    if (written <= 0) return Lz4Error.CompressFailed;
    return dst[0..@intCast(written)];
}

/// Compresses `src` with tunable speed acceleration (`acceleration >= 1`, higher = faster).
pub fn compressFastInto(src: []const u8, dst: []u8, acceleration: i32): []const u8! {
    if (src.len > 0x7E000000) return Lz4Error.InputTooLarge;
    if (src.len == 0) return dst[0..0];
    const accel: i32 = if (acceleration < 1) 1 else acceleration;
    let written: i32 = 0;
    unsafe {
        written = LZ4_compress_fast(src.ptr, dst.ptr, @intCast(src.len), @intCast(dst.len), accel);
    }
    if (written <= 0) return Lz4Error.CompressFailed;
    return dst[0..@intCast(written)];
}

/// Compresses `src` using High-Compression LZ4-HC (`level` in 3..12, default 9).
pub fn compressHCInto(src: []const u8, dst: []u8, level: i32): []const u8! {
    if (src.len > 0x7E000000) return Lz4Error.InputTooLarge;
    if (src.len == 0) return dst[0..0];
    let written: i32 = 0;
    unsafe {
        written = LZ4_compress_HC(src.ptr, dst.ptr, @intCast(src.len), @intCast(dst.len), level);
    }
    if (written <= 0) return Lz4Error.CompressFailed;
    return dst[0..@intCast(written)];
}

/// Decompresses an LZ4 block `src` into `dst` (must be at least as large as original uncompressed size).
pub fn decompressInto(src: []const u8, dst: []u8): []const u8! {
    if (src.len == 0) return dst[0..0];
    if (src.len > 0x7FFFFFFF or dst.len > 0x7FFFFFFF) return Lz4Error.InputTooLarge;
    let decoded: i32 = 0;
    unsafe {
        decoded = LZ4_decompress_safe(src.ptr, dst.ptr, @intCast(src.len), @intCast(dst.len));
    }
    if (decoded < 0) return Lz4Error.DecompressFailed;
    return dst[0..@intCast(decoded)];
}

/// Owned RAII compressed/decompressed byte buffer backed by an explicit allocator.
pub struct Lz4Buffer {
    pub data: []u8,
    allocator: std.mem.Allocator,

    pub static fn compress(allocator: std.mem.Allocator, src: []const u8): Lz4Buffer! {
        const bound = try compressBound(src.len);
        const buf = allocator.alloc(u8, bound) catch return Lz4Error.OutOfMemory;
        const used = compressInto(src, buf) catch |e| {
            allocator.free(buf);
            return e;
        };
        const shrunk = allocator.realloc(buf, used.len) catch buf[0..used.len];
        return Lz4Buffer{ .data = shrunk, .allocator = allocator };
    }

    pub static fn compressHC(allocator: std.mem.Allocator, src: []const u8, level: i32): Lz4Buffer! {
        const bound = try compressBound(src.len);
        const buf = allocator.alloc(u8, bound) catch return Lz4Error.OutOfMemory;
        const used = compressHCInto(src, buf, level) catch |e| {
            allocator.free(buf);
            return e;
        };
        const shrunk = allocator.realloc(buf, used.len) catch buf[0..used.len];
        return Lz4Buffer{ .data = shrunk, .allocator = allocator };
    }

    pub static fn decompress(allocator: std.mem.Allocator, src: []const u8, max_uncompressed: usize): Lz4Buffer! {
        const buf = allocator.alloc(u8, max_uncompressed) catch return Lz4Error.OutOfMemory;
        const used = decompressInto(src, buf) catch |e| {
            allocator.free(buf);
            return e;
        };
        const shrunk = allocator.realloc(buf, used.len) catch buf[0..used.len];
        return Lz4Buffer{ .data = shrunk, .allocator = allocator };
    }

    pub fn slice(): []const u8 {
        return _.data;
    }

    ~init() {
        if (_.data.len > 0) {
            _.allocator.free(_.data);
            _.data = _.data[0..0];
        }
    }
}
