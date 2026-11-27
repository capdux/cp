// @cap/zstd — Zstandard (libzstd) C-FFI & RAII safe wrapper for CP
// License: BSD-3-Clause (Upstream: Meta Platforms, Inc. / facebook/zstd)

import std from "std";

pub @opaque struct ZSTD_CCtx {}
pub @opaque struct ZSTD_DCtx {}

pub const ZSTD_CONTENTSIZE_UNKNOWN: u64 = 18446744073709551615;
pub const ZSTD_CONTENTSIZE_ERROR: u64 = 18446744073709551614;

pub @c {
    fn ZSTD_versionNumber(): u32;
    fn ZSTD_versionString(): [*:0]const u8;
    fn ZSTD_compressBound(srcSize: usize): usize;
    fn ZSTD_isError(code: usize): u32;
    fn ZSTD_getErrorName(code: usize): [*:0]const u8;
    fn ZSTD_getFrameContentSize(src: [*]const u8!, srcSize: usize): u64;

    fn ZSTD_createCCtx(): *ZSTD_CCtx?!;
    fn ZSTD_freeCCtx(cctx: *ZSTD_CCtx!): usize;
    fn ZSTD_compressCCtx(
        cctx: *ZSTD_CCtx!,
        dst: [*]u8!,
        dstCapacity: usize,
        src: [*]const u8!,
        srcSize: usize,
        compressionLevel: i32
    ): usize;

    fn ZSTD_createDCtx(): *ZSTD_DCtx?!;
    fn ZSTD_freeDCtx(dctx: *ZSTD_DCtx!): usize;
    fn ZSTD_decompressDCtx(
        dctx: *ZSTD_DCtx!,
        dst: [*]u8!,
        dstCapacity: usize,
        src: [*]const u8!,
        srcSize: usize
    ): usize;
}

pub err ZstdError {
    AllocContextFailed,
    InvalidContext,
    CompressFailed,
    DecompressFailed,
    UnknownContentSize,
    OutOfMemory,
}

pub fn versionNumber(): u32 {
    return ZSTD_versionNumber();
}

pub fn compressBound(src_size: usize): usize {
    return ZSTD_compressBound(src_size);
}

/// Reusable Zstandard compressor owning a native `ZSTD_CCtx` with deterministic `~init()` RAII release.
pub class Compressor {
    pub ctx: *ZSTD_CCtx?! = null,
    pub level: i32 = 3,

    pub fn init(*_, level: i32): void! {
        const handle = ZSTD_createCCtx() orelse return ZstdError.AllocContextFailed;
        _.ctx = handle;
        _.level = level;
    }

    pub fn compressInto(src: []const u8, dst: []u8): usize! {
        const handle = _.ctx orelse return ZstdError.InvalidContext;
        const written = ZSTD_compressCCtx(handle, dst.ptr, dst.len, src.ptr, src.len, _.level);
        if (ZSTD_isError(written) != 0) return ZstdError.CompressFailed;
        return written;
    }

    pub fn compressAlloc(allocator: std.mem.Allocator, src: []const u8): []u8! {
        const bound = ZSTD_compressBound(src.len);
        const out = allocator.alloc(u8, bound) catch return ZstdError.OutOfMemory;
        errdefer allocator.free(out);
        const written = try _.compressInto(src, out);
        return allocator.realloc(out, written) catch out[0..written];
    }

    ~init() {
        if (handle := _.ctx) {
            * = ZSTD_freeCCtx(handle);
            _.ctx = null;
        }
    }
}

/// Reusable Zstandard decompressor owning a native `ZSTD_DCtx` with deterministic `~init()` RAII release.
pub class Decompressor {
    pub ctx: *ZSTD_DCtx?! = null,

    pub fn init(*_): void! {
        const handle = ZSTD_createDCtx() orelse return ZstdError.AllocContextFailed;
        _.ctx = handle;
    }

    pub fn decompressInto(src: []const u8, dst: []u8): usize! {
        const handle = _.ctx orelse return ZstdError.InvalidContext;
        const written = ZSTD_decompressDCtx(handle, dst.ptr, dst.len, src.ptr, src.len);
        if (ZSTD_isError(written) != 0) return ZstdError.DecompressFailed;
        return written;
    }

    pub fn decompressAlloc(allocator: std.mem.Allocator, src: []const u8): []u8! {
        const expected = ZSTD_getFrameContentSize(src.ptr, src.len);
        if (expected == ZSTD_CONTENTSIZE_UNKNOWN || expected == ZSTD_CONTENTSIZE_ERROR) {
            return ZstdError.UnknownContentSize;
        }
        const cap = expected as usize;
        const out = allocator.alloc(u8, cap) catch return ZstdError.OutOfMemory;
        errdefer allocator.free(out);
        const written = try _.decompressInto(src, out);
        return out[0..written];
    }

    ~init() {
        if (handle := _.ctx) {
            * = ZSTD_freeDCtx(handle);
            _.ctx = null;
        }
    }
}
