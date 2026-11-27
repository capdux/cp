// Type definitions for CP

import {  } from "std";
export class ZSTD_CCtx {
}

export class ZSTD_DCtx {
}

export fn ZSTD_versionNumber(): u32;
export fn ZSTD_versionString(): [*:0]&u8!;
export fn ZSTD_compressBound(srcSize: usize): usize;
export fn ZSTD_isError(code: usize): u32;
export fn ZSTD_getErrorName(code: usize): [*:0]&u8!;
export fn ZSTD_getFrameContentSize(src: [*]&u8!, srcSize: usize): u64;
export fn ZSTD_createCCtx(): *ZSTD_CCtx!?;
export fn ZSTD_freeCCtx(cctx: *ZSTD_CCtx!): usize;
export fn ZSTD_compressCCtx(cctx: *ZSTD_CCtx!, dst: [*]u8!, dstCapacity: usize, src: [*]&u8!, srcSize: usize, compressionLevel: i32): usize;
export fn ZSTD_createDCtx(): *ZSTD_DCtx!?;
export fn ZSTD_freeDCtx(dctx: *ZSTD_DCtx!): usize;
export fn ZSTD_decompressDCtx(dctx: *ZSTD_DCtx!, dst: [*]u8!, dstCapacity: usize, src: [*]&u8!, srcSize: usize): usize;
export error ZstdError { AllocContextFailed, InvalidContext, CompressFailed, DecompressFailed, UnknownContentSize, OutOfMemory }

export fn versionNumber(): u32;
export fn compressBound(src_size: usize): usize;
export class Compressor {
    ctx: *ZSTD_CCtx!?,
    level: i32,
    fn init(_: auto, level: i32): !void;
    fn compressInto(src: &[]u8, dst: []u8): !usize;
    fn compressAlloc(allocator: std.mem.Allocator, src: &[]u8): ![]u8;
    fn __cp_deinit(_: &mut Compressor);
}

export class Decompressor {
    ctx: *ZSTD_DCtx!?,
    fn init(_: auto): !void;
    fn decompressInto(src: &[]u8, dst: []u8): !usize;
    fn decompressAlloc(allocator: std.mem.Allocator, src: &[]u8): ![]u8;
    fn __cp_deinit(_: &mut Decompressor);
}

