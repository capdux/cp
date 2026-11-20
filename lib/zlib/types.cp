// Source: bun/src/zlib/zlib.cp -> cp/lib/zlib/types.cp (Unified Zlib/Gzip/Deflate Types, Options & Errors)

import { DeflateError } from "cap:deflate";

/// Compression container format for `compress` and `decompress`.
pub enum ZlibFormat {
    RawDeflate,
    Zlib,
    Gzip,
    AutoDetect,
}

/// Compression level presets (RFC 1950 / RFC 1951).
pub enum ZlibLevel {
    None,
    Fast,
    Default,
    Best,
}

/// Unified options for compression and decompression.
pub struct ZlibOptions {
    level: ZlibLevel = ZlibLevel.Default,
    /// Maximum allowed decompressed output size in bytes (default: 64 MiB anti-bomb guard).
    max_output_bytes: usize = 64 * 1024 * 1024,
    /// Optional RFC 1952 GZIP FNAME metadata (null-terminated filename).
    gzip_filename: ([]const u8)? = null,
    /// Optional RFC 1952 GZIP MTIME (Unix timestamp in seconds).
    gzip_mtime: u32 = 0,
    /// Optional RFC 1952 GZIP OS byte (255 = unknown).
    gzip_os: u8 = 255,
    /// Optional RFC 1952 GZIP FCOMMENT metadata.
    gzip_comment: ([]const u8)? = null,
    /// Whether to emit RFC 1952 GZIP FHCRC header checksum.
    gzip_fhcrc: bool = false,
}

/// Unified error set for `cap:zlib`.
pub err ZlibError {
    UnexpectedEof,
    InvalidHeader,
    InvalidFormat,
    InvalidBlockType,
    InvalidStoredLength,
    InvalidHuffmanCode,
    InvalidDistance,
    ChecksumMismatch,
    OutputTooLarge,
    TrailingBytes,
    OutOfMemory,
}

/// Maps `DeflateError` from `cap:deflate` into `ZlibError`.
pub fn mapDeflateError(e: DeflateError): ZlibError {
    return switch (e) {
        DeflateError.UnexpectedEof => ZlibError.UnexpectedEof,
        DeflateError.InvalidBlockType => ZlibError.InvalidBlockType,
        DeflateError.InvalidStoredLength => ZlibError.InvalidStoredLength,
        DeflateError.InvalidHuffmanCode => ZlibError.InvalidHuffmanCode,
        DeflateError.InvalidDistance => ZlibError.InvalidDistance,
        DeflateError.InvalidHeader => ZlibError.InvalidHeader,
        DeflateError.ChecksumMismatch => ZlibError.ChecksumMismatch,
        DeflateError.OutputTooLarge => ZlibError.OutputTooLarge,
        DeflateError.TrailingBytes => ZlibError.TrailingBytes,
        DeflateError.OutOfMemory => ZlibError.OutOfMemory,
    };
}
