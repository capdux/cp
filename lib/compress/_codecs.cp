// ============================================================================
// lib/compress/_codecs.cp — Zlib / Brotli / Zstd 底层 C-FFI 声明与状态枚举
// 吸收自 Bun src/zlib/zlib.cp, src/brotli/brotli.cp, src/zstd/zstd.cp
// ============================================================================

pub @c struct z_stream_s {
    next_in: [*c]const u8,
    avail_in: u32,
    total_in: usize,
    next_out: [*c]u8,
    avail_out: u32,
    total_out: usize,
    msg: [*c]const u8,
    state: *!?,
    zalloc: *!?,
    zfree: *!?,
    opaque_ctx: *!?,
    data_type: i32,
    adler: usize,
    reserved: usize,
}

pub @c struct BrotliDecoderStateStruct {
    _opaque: i32,
}

pub @c struct BrotliEncoderStateStruct {
    _opaque: i32,
}

pub @c {
    // Zlib C-FFI
    fn zlibVersion(): [*c]const u8;
    fn compress(dest: [*c]u8, destLen: *usize, source: [*c]const u8, sourceLen: usize): i32;
    fn compress2(dest: [*c]u8, destLen: *usize, source: [*c]const u8, sourceLen: usize, level: i32): i32;
    fn compressBound(sourceLen: usize): usize;
    fn uncompress(dest: [*c]u8, destLen: *usize, source: [*c]const u8, sourceLen: usize): i32;

    // Zstd C-FFI
    fn ZSTD_compress(dst: [*c]u8, dstCapacity: usize, src: [*c]const u8, srcSize: usize, compressionLevel: i32): usize;
    fn ZSTD_decompress(dst: [*c]u8, dstCapacity: usize, src: [*c]const u8, compressedSize: usize): usize;
    fn ZSTD_compressBound(srcSize: usize): usize;
    fn ZSTD_isError(code: usize): u32;
    fn ZSTD_getErrorName(code: usize): [*c]const u8;
    fn ZSTD_defaultCLevel(): i32;
    fn ZSTD_findDecompressedSize(src: *const !?, srcSize: usize): u64;

    // Brotli C-FFI
    fn BrotliDecoderCreateInstance(alloc_func: *!?, free_func: *!?, opaque_ctx: *!?): (*BrotliDecoderStateStruct)?;
    fn BrotliDecoderDestroyInstance(state: *BrotliDecoderStateStruct);
    fn BrotliEncoderMaxCompressedSize(input_size: usize): usize;
}

pub const Z_MIN_WBITS: i32 = 8;
pub const Z_MAX_WBITS: i32 = 15;
pub const Z_DEFAULT_COMPRESSION: i32 = -1;
pub const Z_NO_COMPRESSION: i32 = 0;
pub const Z_BEST_SPEED: i32 = 1;
pub const Z_BEST_COMPRESSION: i32 = 9;

pub enum ZlibFlush: i32 {
    NoFlush = 0,
    PartialFlush = 1,
    SyncFlush = 2,
    FullFlush = 3,
    Finish = 4,
    Block = 5,
    Trees = 6,
}

pub enum ZlibStatus: i32 {
    Ok = 0,
    StreamEnd = 1,
    NeedDict = 2,
    Errno = -1,
    StreamError = -2,
    DataError = -3,
    MemError = -4,
    BufError = -5,
    VersionError = -6,

    static fn isSuccess(code: i32): bool {
        return code == 0 || code == 1;
    }
}

pub enum BrotliOperation: i32 {
    Process = 0,
    Flush = 1,
    Finish = 2,
    EmitMetadata = 3,
}

pub enum CompressionFormat: u8 {
    RawDeflate = 0,
    Zlib = 1,
    Gzip = 2,
    Brotli = 3,
    Zstd = 4,

    static fn contentEncodingName(fmt: CompressionFormat): []const u8 {
        if (fmt == CompressionFormat.Gzip) { return "gzip"; }
        if (fmt == CompressionFormat.Zlib) { return "deflate"; }
        if (fmt == CompressionFormat.Brotli) { return "br"; }
        if (fmt == CompressionFormat.Zstd) { return "zstd"; }
        return "identity";
    }

    static fn fromAcceptEncoding(header: []const u8): CompressionFormat {
        // 优先级：zstd > br > gzip > deflate
        if (containsToken(header, "zstd")) { return CompressionFormat.Zstd; }
        if (containsToken(header, "br")) { return CompressionFormat.Brotli; }
        if (containsToken(header, "gzip")) { return CompressionFormat.Gzip; }
        if (containsToken(header, "deflate")) { return CompressionFormat.Zlib; }
        return CompressionFormat.RawDeflate;
    }
}

fn containsToken(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0 || haystack.len < needle.len) { return false; }
    let limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        if (haystack[i .. i + needle.len] == needle) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 纯算术计算 Zlib / Zstd 压缩上界估计（无需链接底层 C 库即可预分配缓冲区）
pub struct CompressBoundEstimator {
    pub static fn zlibBound(source_len: usize): usize {
        return source_len + (source_len >> 12) + (source_len >> 14) + (source_len >> 25) + 13;
    }

    pub static fn zstdBound(source_len: usize): usize {
        let margin: usize = if (source_len < 131072) (((131072 as usize) - source_len) >> 11) else (0 as usize);
        return source_len + (source_len >> 8) + margin + 64;
    }
}
