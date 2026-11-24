// @cap/snappy — Google Snappy 1.2.1 fast compression library for CP
// License: BSD-3-Clause (Upstream: google/snappy)

pub @c {
    fn snappy_compress(input: [*]const u8!, input_length: usize, compressed: [*]u8!, compressed_length: *usize!): i32;
    fn snappy_uncompress(compressed: [*]const u8!, compressed_length: usize, uncompressed: [*]u8!, uncompressed_length: *usize!): i32;
    fn snappy_max_compressed_length(source_length: usize): usize;
    fn snappy_uncompressed_length(compressed: [*]const u8!, compressed_length: usize, result: *usize!): i32;
    fn snappy_validate_compressed_buffer(compressed: [*]const u8!, compressed_length: usize): i32;
}

pub err SnappyError {
    InvalidInput,
    BufferTooSmall,
}

/// Returns the maximal size of the compressed representation for `source_len` bytes.
pub fn maxCompressedLength(source_len: usize): usize {
    unsafe {
        return snappy_max_compressed_length(source_len);
    }
}

/// Returns the uncompressed length recorded in a Snappy-compressed buffer in O(1) time.
pub fn uncompressedLength(compressed: []const u8): usize! {
    let out_len: usize = 0;
    let status: i32 = 0;
    unsafe {
        status = snappy_uncompressed_length(compressed.ptr, compressed.len, &out_len);
    }
    if (status != 0) return SnappyError.InvalidInput;
    return out_len;
}

/// Compresses `src` into caller-provided `dst`.
/// `dst.len` must be at least `maxCompressedLength(src.len)`.
/// Returns the compressed slice view into `dst`.
pub fn compressInto(src: []const u8, dst: []u8): []const u8! {
    let out_len: usize = dst.len;
    let status: i32 = 0;
    unsafe {
        status = snappy_compress(src.ptr, src.len, dst.ptr, &out_len);
    }
    if (status == 2) return SnappyError.BufferTooSmall;
    if (status != 0) return SnappyError.InvalidInput;
    return dst[0..out_len];
}

/// Decompresses a Snappy-compressed buffer `compressed` into caller-provided `dst`.
/// Returns the decompressed slice view into `dst`.
pub fn uncompressInto(compressed: []const u8, dst: []u8): []const u8! {
    let out_len: usize = dst.len;
    let status: i32 = 0;
    unsafe {
        status = snappy_uncompress(compressed.ptr, compressed.len, dst.ptr, &out_len);
    }
    if (status == 2) return SnappyError.BufferTooSmall;
    if (status != 0) return SnappyError.InvalidInput;
    return dst[0..out_len];
}

/// Validates whether `compressed` is a well-formed Snappy stream without decompressing it.
pub fn validateCompressedBuffer(compressed: []const u8): bool {
    let status: i32 = 0;
    unsafe {
        status = snappy_validate_compressed_buffer(compressed.ptr, compressed.len);
    }
    return status == 0;
}
