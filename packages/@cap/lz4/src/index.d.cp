// Type definitions for CP

import {  } from "std";
export fn LZ4_versionNumber(): i32;
export fn LZ4_compressBound(inputSize: i32): i32;
export fn LZ4_compress_default(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32): i32;
export fn LZ4_compress_fast(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, acceleration: i32): i32;
export fn LZ4_compress_HC(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, compressionLevel: i32): i32;
export fn LZ4_decompress_safe(src: [*]&u8!, dst: [*]u8!, compressedSize: i32, dstCapacity: i32): i32;
export error Lz4Error { InputTooLarge, OutputTooSmall, CompressFailed, DecompressFailed, OutOfMemory }

export fn versionNumber(): i32;
export fn compressBound(input_len: usize): !usize;
export fn compressInto(src: &[]u8, dst: []u8): !&[]u8;
export fn compressFastInto(src: &[]u8, dst: []u8, acceleration: i32): !&[]u8;
export fn compressHCInto(src: &[]u8, dst: []u8, level: i32): !&[]u8;
export fn decompressInto(src: &[]u8, dst: []u8): !&[]u8;
export class Lz4Buffer {
    data: []u8,
    allocator: std.mem.Allocator,
    fn compress(allocator: std.mem.Allocator, src: &[]u8): !Lz4Buffer;
    fn compressHC(allocator: std.mem.Allocator, src: &[]u8, level: i32): !Lz4Buffer;
    fn decompress(allocator: std.mem.Allocator, src: &[]u8, max_uncompressed: usize): !Lz4Buffer;
    fn slice(): &[]u8;
    fn __cp_deinit(_: &mut Lz4Buffer);
}

