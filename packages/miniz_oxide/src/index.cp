// @cap/miniz_oxide — DEFLATE / Zlib / ZIP compression engine (miniz 3.0.2) for CP
// License: MIT (Upstream: Frommi/miniz_oxide & Rich Geldreich/miniz)

import std from "std";

pub @c {
    fn cp_miniz_version(): [*:0]const u8;
    fn cp_miniz_zip_create_single(
        entry_name: [*:0]const u8!,
        data: [*]const u8!,
        data_len: usize,
        out_zip_len: *usize!
    ): [*]u8?!;
    fn cp_miniz_zip_extract_entry(
        zip_bytes: [*]const u8!,
        zip_len: usize,
        entry_name: [*:0]const u8!,
        out_len: *usize!
    ): [*]u8?!;
    fn cp_miniz_free(ptr: *!?);

    // Native miniz 3.0.2 DEFLATE / Zlib / Checksum APIs
    fn tdefl_create_comp_flags_from_zip_params(level: i32, window_bits: i32, strategy: i32): u32;
    fn tdefl_compress_mem_to_heap(
        pSrc_buf: [*]const u8!,
        src_buf_len: usize,
        pOut_len: *usize!,
        flags: i32
    ): [*]u8?!;
    fn tdefl_compress_mem_to_mem(
        pOut_buf: [*]u8!,
        out_buf_len: usize,
        pSrc_buf: [*]const u8!,
        src_buf_len: usize,
        flags: i32
    ): usize;
    fn tinfl_decompress_mem_to_heap(
        pSrc_buf: [*]const u8!,
        src_buf_len: usize,
        pOut_len: *usize!,
        flags: i32
    ): [*]u8?!;
    fn tinfl_decompress_mem_to_mem(
        pOut_buf: [*]u8!,
        out_buf_len: usize,
        pSrc_buf: [*]const u8!,
        src_buf_len: usize,
        flags: i32
    ): usize;
    fn mz_compressBound(source_len: usize): usize;
    fn mz_crc32(crc: usize, ptr: [*]const u8!, buf_len: usize): usize;
    fn mz_adler32(adler: usize, ptr: [*]const u8!, buf_len: usize): usize;
}

/// Compression levels matching crates.io `miniz_oxide::deflate::CompressionLevel`.
pub const COMPRESSION_NO_COMPRESSION: u8 = 0;
pub const COMPRESSION_BEST_SPEED: u8 = 1;
pub const COMPRESSION_DEFAULT_LEVEL: u8 = 6;
pub const COMPRESSION_DEFAULT_COMPRESSION: u8 = 6;
pub const COMPRESSION_BEST_COMPRESSION: u8 = 9;
pub const COMPRESSION_UBER_COMPRESSION: u8 = 10;

pub const TDEFL_WRITE_ZLIB_HEADER: i32 = 0x1000;
pub const TINFL_FLAG_PARSE_ZLIB_HEADER: i32 = 1;
pub const TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF: i32 = 4;
pub const TINFL_FLAG_COMPUTE_ADLER32: i32 = 8;

pub err MinizError {
    ZipCreateFailed,
    ZipExtractFailed,
    OutOfMemory,
    CompressFailed,
    DecompressFailed,
    BufferTooSmall,
    InvalidLevel,
    OutputLimitExceeded,
}

pub fn versionString(): string {
    unsafe { return std.mem.span(cp_miniz_version()); }
}

/// Returns a conservative upper bound on the compressed zlib size for `source_len` bytes.
pub fn compressBound(source_len: usize): usize {
    unsafe { return mz_compressBound(source_len); }
}

/// Computes IEEE 802.3 CRC-32 checksum over `data`.
pub fn crc32(data: []const u8): u32 {
    return crc32Update(0, data);
}

/// Updates an incremental CRC-32 checksum with `data`.
pub fn crc32Update(prev: u32, data: []const u8): u32 {
    if (data.len == 0) { return prev; }
    unsafe {
        return (mz_crc32(prev as usize, data.ptr, data.len) & 0xFFFFFFFF) as u32;
    }
}

/// Computes RFC 1950 Adler-32 checksum over `data`.
pub fn adler32(data: []const u8): u32 {
    return adler32Update(1, data);
}

/// Updates an incremental Adler-32 checksum with `data`.
pub fn adler32Update(prev: u32, data: []const u8): u32 {
    if (data.len == 0) { return prev; }
    unsafe {
        return (mz_adler32(prev as usize, data.ptr, data.len) & 0xFFFFFFFF) as u32;
    }
}

fn makeDeflateFlags(level: u8, write_zlib_header: bool): i32! {
    if (level > COMPRESSION_UBER_COMPRESSION) {
        return MinizError.InvalidLevel;
    }
    const win_bits: i32 = if (write_zlib_header) { 15 } else { -15 };
    unsafe {
        const flags = tdefl_create_comp_flags_from_zip_params(level as i32, win_bits, 0);
        return flags as i32;
    }
}

/// Owned RAII byte buffer allocated by miniz.
pub struct MinizBuf {
    raw: [*]u8?! = null,
    pub len: usize = 0,

    pub static fn zipSingleFile(entry_name: string, payload: []const u8): MinizBuf! {
        const alloc = std.heap.page_allocator;
        const name_z = alloc.allocSentinel(u8, entry_name.len, 0) catch return MinizError.OutOfMemory;
        defer alloc.free(name_z);
        @memcpy(name_z[0..entry_name.len], entry_name);

        let sz: usize = 0;
        let ptr: [*]u8?! = null;
        unsafe {
            ptr = cp_miniz_zip_create_single(name_z.ptr, payload.ptr, payload.len, &sz);
        }
        if (ptr == null or sz == 0) return MinizError.ZipCreateFailed;
        return MinizBuf{ .raw = ptr, .len = sz };
    }

    pub static fn extractFile(zip_bytes: []const u8, entry_name: string): MinizBuf! {
        const alloc = std.heap.page_allocator;
        const name_z = alloc.allocSentinel(u8, entry_name.len, 0) catch return MinizError.OutOfMemory;
        defer alloc.free(name_z);
        @memcpy(name_z[0..entry_name.len], entry_name);

        let sz: usize = 0;
        let ptr: [*]u8?! = null;
        unsafe {
            ptr = cp_miniz_zip_extract_entry(zip_bytes.ptr, zip_bytes.len, name_z.ptr, &sz);
        }
        if (ptr == null) return MinizError.ZipExtractFailed;
        return MinizBuf{ .raw = ptr, .len = sz };
    }

    pub fn slice(): []const u8 {
        if (p := _.raw) return p[0.._.len];
        return "";
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            _.len = 0;
            unsafe { cp_miniz_free(@ptrCast(p)); }
        }
    }
}

/// Compresses `input` into a raw DEFLATE stream (RFC 1951) at `level` (0..=10).
/// Equivalent to crates.io `miniz_oxide::deflate::compress_to_vec`.
pub fn compressToVec(input: []const u8, level: u8): MinizBuf! {
    const flags = makeDeflateFlags(level, false)?;
    let dummy: [1]u8 = [0];
    const src_ptr = if (input.len == 0) { dummy[0..1].ptr } else { input.ptr };
    let out_len: usize = 0;
    let ptr: [*]u8?! = null;
    unsafe {
        ptr = tdefl_compress_mem_to_heap(src_ptr, input.len, &out_len, flags);
    }
    if (ptr == null) return MinizError.CompressFailed;
    return MinizBuf{ .raw = ptr, .len = out_len };
}

/// Compresses `input` into a Zlib-wrapped DEFLATE stream (RFC 1950) at `level` (0..=10).
/// Equivalent to crates.io `miniz_oxide::deflate::compress_to_vec_zlib`.
pub fn compressToVecZlib(input: []const u8, level: u8): MinizBuf! {
    const flags = makeDeflateFlags(level, true)?;
    let dummy: [1]u8 = [0];
    const src_ptr = if (input.len == 0) { dummy[0..1].ptr } else { input.ptr };
    let out_len: usize = 0;
    let ptr: [*]u8?! = null;
    unsafe {
        ptr = tdefl_compress_mem_to_heap(src_ptr, input.len, &out_len, flags);
    }
    if (ptr == null) return MinizError.CompressFailed;
    return MinizBuf{ .raw = ptr, .len = out_len };
}

/// Decompresses a raw DEFLATE stream (RFC 1951) into an owned `MinizBuf`.
/// Equivalent to crates.io `miniz_oxide::inflate::decompress_to_vec`.
pub fn decompressToVec(input: []const u8): MinizBuf! {
    if (input.len == 0) return MinizError.DecompressFailed;
    let out_len: usize = 0;
    let ptr: [*]u8?! = null;
    unsafe {
        ptr = tinfl_decompress_mem_to_heap(input.ptr, input.len, &out_len, 0);
    }
    if (ptr == null) return MinizError.DecompressFailed;
    return MinizBuf{ .raw = ptr, .len = out_len };
}

/// Decompresses a Zlib-wrapped DEFLATE stream (RFC 1950) with Adler-32 verification.
/// Equivalent to crates.io `miniz_oxide::inflate::decompress_to_vec_zlib`.
pub fn decompressToVecZlib(input: []const u8): MinizBuf! {
    if (input.len < 2) return MinizError.DecompressFailed;
    let out_len: usize = 0;
    let ptr: [*]u8?! = null;
    unsafe {
        ptr = tinfl_decompress_mem_to_heap(input.ptr, input.len, &out_len, TINFL_FLAG_PARSE_ZLIB_HEADER);
    }
    if (ptr == null) return MinizError.DecompressFailed;
    return MinizBuf{ .raw = ptr, .len = out_len };
}

/// Decompresses a raw DEFLATE stream enforcing a maximum output size `max_size`.
/// Equivalent to crates.io `miniz_oxide::inflate::decompress_to_vec_with_limit`.
pub fn decompressToVecWithLimit(input: []const u8, max_size: usize): MinizBuf! {
    let buf = decompressToVec(input)?;
    if (buf.len > max_size) {
        return MinizError.OutputLimitExceeded;
    }
    return buf;
}

/// Decompresses a Zlib stream enforcing a maximum output size `max_size`.
/// Equivalent to crates.io `miniz_oxide::inflate::decompress_to_vec_zlib_with_limit`.
pub fn decompressToVecZlibWithLimit(input: []const u8, max_size: usize): MinizBuf! {
    let buf = decompressToVecZlib(input)?;
    if (buf.len > max_size) {
        return MinizError.OutputLimitExceeded;
    }
    return buf;
}

/// Zero-heap raw DEFLATE compression directly into caller-provided `out` slice.
pub fn compressToSlice(input: []const u8, out: []u8, level: u8): usize! {
    if (out.len == 0) return MinizError.BufferTooSmall;
    const flags = makeDeflateFlags(level, false)?;
    let dummy: [1]u8 = [0];
    const src_ptr = if (input.len == 0) { dummy[0..1].ptr } else { input.ptr };
    unsafe {
        const written = tdefl_compress_mem_to_mem(out.ptr, out.len, src_ptr, input.len, flags);
        if (written == 0) return MinizError.BufferTooSmall;
        return written;
    }
}

/// Zero-heap Zlib compression directly into caller-provided `out` slice.
pub fn compressToSliceZlib(input: []const u8, out: []u8, level: u8): usize! {
    if (out.len == 0) return MinizError.BufferTooSmall;
    const flags = makeDeflateFlags(level, true)?;
    let dummy: [1]u8 = [0];
    const src_ptr = if (input.len == 0) { dummy[0..1].ptr } else { input.ptr };
    unsafe {
        const written = tdefl_compress_mem_to_mem(out.ptr, out.len, src_ptr, input.len, flags);
        if (written == 0) return MinizError.BufferTooSmall;
        return written;
    }
}

/// Zero-heap raw DEFLATE decompression directly into caller-provided `out` slice.
pub fn decompressToSlice(input: []const u8, out: []u8): usize! {
    if (input.len == 0) return MinizError.DecompressFailed;
    if (out.len == 0) return MinizError.BufferTooSmall;
    unsafe {
        const written = tinfl_decompress_mem_to_mem(out.ptr, out.len, input.ptr, input.len, 0);
        if (written == (~(0 as usize))) return MinizError.DecompressFailed;
        return written;
    }
}

/// Zero-heap Zlib decompression directly into caller-provided `out` slice.
pub fn decompressToSliceZlib(input: []const u8, out: []u8): usize! {
    if (input.len < 2) return MinizError.DecompressFailed;
    if (out.len == 0) return MinizError.BufferTooSmall;
    unsafe {
        const written = tinfl_decompress_mem_to_mem(
            out.ptr,
            out.len,
            input.ptr,
            input.len,
            TINFL_FLAG_PARSE_ZLIB_HEADER,
        );
        if (written == (~(0 as usize))) return MinizError.DecompressFailed;
        return written;
    }
}

/// Rust `miniz_oxide` snake_case compatibility aliases.
pub fn compress_to_vec(input: []const u8, level: u8): MinizBuf! {
    return compressToVec(input, level);
}

pub fn compress_to_vec_zlib(input: []const u8, level: u8): MinizBuf! {
    return compressToVecZlib(input, level);
}

pub fn decompress_to_vec(input: []const u8): MinizBuf! {
    return decompressToVec(input);
}

pub fn decompress_to_vec_zlib(input: []const u8): MinizBuf! {
    return decompressToVecZlib(input);
}
