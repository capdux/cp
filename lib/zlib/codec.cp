// Source: bun/src/zlib/zlib.cp -> cp/lib/zlib/codec.cp (Unified Zlib, Gzip & Raw Deflate Codec)

const std = @import("std");
const deflate = @import("cap:deflate");
import { ZlibFormat, ZlibLevel, ZlibOptions, ZlibError, mapDeflateError } from "./types.cp";
import { decompressDeflatePrefix, compressStoredDeflate } from "./deflateStream.cp";

/// Computes IEEE 802.3 CRC-32 checksum.
pub fn crc32(data: []const u8): u32 {
    return deflate.crc32(data);
}

/// Computes RFC 1950 Adler-32 checksum.
pub fn adler32(data: []const u8): u32 {
    return deflate.adler32(data);
}

fn readU16LE(buf: []const u8, pos: usize): u16 {
    return (buf[pos] as u16) | ((buf[pos + 1] as u16) << 8);
}

fn readU32LE(buf: []const u8, pos: usize): u32 {
    return (buf[pos] as u32) |
        ((buf[pos + 1] as u32) << 8) |
        ((buf[pos + 2] as u32) << 16) |
        ((buf[pos + 3] as u32) << 24);
}

fn readU32BE(buf: []const u8, pos: usize): u32 {
    return ((buf[pos] as u32) << 24) |
        ((buf[pos + 1] as u32) << 16) |
        ((buf[pos + 2] as u32) << 8) |
        (buf[pos + 3] as u32);
}

fn appendU16LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u16): void ! ZlibError {
    out.append(alloc, (v & 0xff) as u8) catch return ZlibError.OutOfMemory;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch return ZlibError.OutOfMemory;
}

fn appendU32LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32): void ! ZlibError {
    out.append(alloc, (v & 0xff) as u8) catch return ZlibError.OutOfMemory;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch return ZlibError.OutOfMemory;
    out.append(alloc, ((v >> 16) & 0xff) as u8) catch return ZlibError.OutOfMemory;
    out.append(alloc, ((v >> 24) & 0xff) as u8) catch return ZlibError.OutOfMemory;
}

fn appendU32BE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32): void ! ZlibError {
    out.append(alloc, ((v >> 24) & 0xff) as u8) catch return ZlibError.OutOfMemory;
    out.append(alloc, ((v >> 16) & 0xff) as u8) catch return ZlibError.OutOfMemory;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch return ZlibError.OutOfMemory;
    out.append(alloc, (v & 0xff) as u8) catch return ZlibError.OutOfMemory;
}

fn compressRawPayload(allocator: std.mem.Allocator, input: []const u8, level: ZlibLevel): ([]u8) ! ZlibError {
    if (level == ZlibLevel.None) {
        return compressStoredDeflate(allocator, input);
    }
    return deflate.compressDeflate(allocator, input) catch |e| return mapDeflateError(e);
}

/// Detects container format (`Gzip` vs `Zlib` vs `RawDeflate`) from magic bytes.
pub fn detectFormat(data: []const u8): ZlibFormat ! ZlibError {
    if (data.len >= 2 && data[0] == 0x1f && data[1] == 0x8b) {
        return ZlibFormat.Gzip;
    }
    if (data.len >= 2 && (data[0] & 0x0f) == 0x08 && (data[0] >> 4) <= 7) {
        const cmf_flg: u16 = ((data[0] as u16) << 8) | (data[1] as u16);
        if ((cmf_flg % 31) == 0) {
            return ZlibFormat.Zlib;
        }
    }
    if (data.len == 0) {
        return ZlibError.UnexpectedEof;
    }
    return ZlibFormat.RawDeflate;
}

/// Compresses `input` into raw RFC 1951 DEFLATE stream.
pub fn compressRawDeflate(allocator: std.mem.Allocator, input: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    return compressRawPayload(allocator, input, options.level);
}

/// Decompresses a raw RFC 1951 DEFLATE stream with `options.max_output_bytes` guard.
pub fn decompressRawDeflate(allocator: std.mem.Allocator, compressed: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    return deflate.decompressDeflateWithMaxSize(allocator, compressed, options.max_output_bytes) catch |e| return mapDeflateError(e);
}

/// Compresses `input` into an RFC 1950 ZLIB container.
pub fn compressZlib(allocator: std.mem.Allocator, input: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    const raw = compressRawPayload(allocator, input, options.level) ?;
    defer allocator.free(raw);

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    const cmf: u8 = 0x78;
    const flevel: u8 = switch (options.level) {
        ZlibLevel.None => 0,
        ZlibLevel.Fast => 1,
        ZlibLevel.Default => 2,
        ZlibLevel.Best => 3,
    };
    const base_hdr: u16 = ((cmf as u16) << 8) | ((flevel as u16) << 6);
    const fcheck: u8 = ((31 - (base_hdr % 31)) % 31) as u8;
    const flg: u8 = (flevel << 6) | fcheck;

    out.append(allocator, cmf) catch return ZlibError.OutOfMemory;
    out.append(allocator, flg) catch return ZlibError.OutOfMemory;
    out.appendSlice(allocator, raw) catch return ZlibError.OutOfMemory;
    appendU32BE(allocator, &out, adler32(input)) ?;

    return out.toOwnedSlice(allocator) catch return ZlibError.OutOfMemory;
}

/// Decompresses an RFC 1950 ZLIB container with header, Adler-32, and `max_output_bytes` validation.
pub fn decompressZlib(allocator: std.mem.Allocator, zlib_bytes: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    if (zlib_bytes.len < 6) {
        return ZlibError.UnexpectedEof;
    }
    const cmf = zlib_bytes[0];
    const flg = zlib_bytes[1];
    if ((cmf & 0x0f) != 8 || (cmf >> 4) > 7) {
        return ZlibError.InvalidHeader;
    }
    const check_val: u16 = ((cmf as u16) << 8) | (flg as u16);
    if ((check_val % 31) != 0) {
        return ZlibError.InvalidHeader;
    }
    // Preset dictionary (FDICT) is not supported.
    if ((flg & 0x20) != 0) {
        return ZlibError.InvalidHeader;
    }

    const payload = zlib_bytes[2 .. zlib_bytes.len - 4];
    const out = deflate.decompressDeflateWithMaxSize(allocator, payload, options.max_output_bytes) catch |e| return mapDeflateError(e);

    const expected_adler = readU32BE(zlib_bytes, zlib_bytes.len - 4);
    const actual_adler = adler32(out);
    if (actual_adler != expected_adler) {
        allocator.free(out);
        return ZlibError.ChecksumMismatch;
    }

    return out;
}

/// Compresses `input` into an RFC 1952 GZIP container with optional `FNAME`, `FCOMMENT`, and `FHCRC`.
pub fn compressGzip(allocator: std.mem.Allocator, input: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    const raw = compressRawPayload(allocator, input, options.level) ?;
    defer allocator.free(raw);

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    let flg: u8 = 0;
    if (options.gzip_filename != null) {
        flg |= 0x08;
    }
    if (options.gzip_comment != null) {
        flg |= 0x10;
    }
    if (options.gzip_fhcrc) {
        flg |= 0x02;
    }

    const xfl: u8 = switch (options.level) {
        ZlibLevel.Best => 2,
        ZlibLevel.Fast => 4,
        else => 0,
    };

    out.append(allocator, 0x1f) catch return ZlibError.OutOfMemory;
    out.append(allocator, 0x8b) catch return ZlibError.OutOfMemory;
    out.append(allocator, 0x08) catch return ZlibError.OutOfMemory;
    out.append(allocator, flg) catch return ZlibError.OutOfMemory;
    appendU32LE(allocator, &out, options.gzip_mtime) ?;
    out.append(allocator, xfl) catch return ZlibError.OutOfMemory;
    out.append(allocator, options.gzip_os) catch return ZlibError.OutOfMemory;

    if (fname := options.gzip_filename) {
        out.appendSlice(allocator, fname) catch return ZlibError.OutOfMemory;
        out.append(allocator, 0) catch return ZlibError.OutOfMemory;
    }
    if (comment := options.gzip_comment) {
        out.appendSlice(allocator, comment) catch return ZlibError.OutOfMemory;
        out.append(allocator, 0) catch return ZlibError.OutOfMemory;
    }
    if (options.gzip_fhcrc) {
        const hcrc = (crc32(out.items) & 0xffff) as u16;
        appendU16LE(allocator, &out, hcrc) ?;
    }

    out.appendSlice(allocator, raw) catch return ZlibError.OutOfMemory;
    appendU32LE(allocator, &out, crc32(input)) ?;
    appendU32LE(allocator, &out, (input.len & 0xffffffff) as u32) ?;

    return out.toOwnedSlice(allocator) catch return ZlibError.OutOfMemory;
}

/// Decompresses an RFC 1952 GZIP stream (including multi-member concatenated streams),
/// validating `FEXTRA`, `FNAME`, `FCOMMENT`, `FHCRC`, `CRC32`, `ISIZE`, and `max_output_bytes`.
pub fn decompressGzip(allocator: std.mem.Allocator, gz_bytes: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    if (gz_bytes.len < 10) {
        return ZlibError.UnexpectedEof;
    }

    let total_out = std.ArrayList(u8).empty;
    errdefer total_out.deinit(allocator);

    let offset: usize = 0;
    while (offset < gz_bytes.len) {
        if (offset + 10 > gz_bytes.len) {
            return ZlibError.UnexpectedEof;
        }
        if (gz_bytes[offset] != 0x1f || gz_bytes[offset + 1] != 0x8b || gz_bytes[offset + 2] != 0x08) {
            return ZlibError.InvalidHeader;
        }
        const flg = gz_bytes[offset + 3];
        if ((flg & 0xe0) != 0) {
            return ZlibError.InvalidHeader;
        }

        let pos: usize = offset + 10;

        // FEXTRA (bit 2)
        if ((flg & 0x04) != 0) {
            if (pos + 2 > gz_bytes.len) {
                return ZlibError.UnexpectedEof;
            }
            const xlen = readU16LE(gz_bytes, pos) as usize;
            pos += 2;
            if (pos + xlen > gz_bytes.len) {
                return ZlibError.UnexpectedEof;
            }
            pos += xlen;
        }

        // FNAME (bit 3)
        if ((flg & 0x08) != 0) {
            while (pos < gz_bytes.len && gz_bytes[pos] != 0) {
                pos += 1;
            }
            if (pos >= gz_bytes.len) {
                return ZlibError.UnexpectedEof;
            }
            pos += 1;
        }

        // FCOMMENT (bit 4)
        if ((flg & 0x10) != 0) {
            while (pos < gz_bytes.len && gz_bytes[pos] != 0) {
                pos += 1;
            }
            if (pos >= gz_bytes.len) {
                return ZlibError.UnexpectedEof;
            }
            pos += 1;
        }

        // FHCRC (bit 1)
        if ((flg & 0x02) != 0) {
            if (pos + 2 > gz_bytes.len) {
                return ZlibError.UnexpectedEof;
            }
            const expected_hcrc = readU16LE(gz_bytes, pos);
            const actual_hcrc = (crc32(gz_bytes[offset..pos]) & 0xffff) as u16;
            if (actual_hcrc != expected_hcrc) {
                return ZlibError.ChecksumMismatch;
            }
            pos += 2;
        }

        if (pos + 8 > gz_bytes.len) {
            return ZlibError.UnexpectedEof;
        }

        const remaining_budget = if (options.max_output_bytes > total_out.items.len)
            options.max_output_bytes - total_out.items.len
        else
            0;

        const member_res = decompressDeflatePrefix(allocator, gz_bytes[pos..], remaining_budget) ?;
        defer allocator.free(member_res.data);

        const trailer_pos = pos + member_res.bytes_consumed;
        if (trailer_pos + 8 > gz_bytes.len) {
            return ZlibError.UnexpectedEof;
        }

        const expected_crc = readU32LE(gz_bytes, trailer_pos);
        const expected_isize = readU32LE(gz_bytes, trailer_pos + 4);
        const actual_crc = crc32(member_res.data);
        const actual_isize = (member_res.data.len & 0xffffffff) as u32;

        if (actual_crc != expected_crc || actual_isize != expected_isize) {
            return ZlibError.ChecksumMismatch;
        }

        total_out.appendSlice(allocator, member_res.data) catch return ZlibError.OutOfMemory;
        offset = trailer_pos + 8;
    }

    return total_out.toOwnedSlice(allocator) catch return ZlibError.OutOfMemory;
}

/// Unified compression entry point supporting `RawDeflate`, `Zlib`, and `Gzip`.
pub fn compress(
    allocator: std.mem.Allocator,
    input: []const u8,
    format: ZlibFormat,
    options: ZlibOptions,
): ([]u8) ! ZlibError {
    return switch (format) {
        ZlibFormat.RawDeflate => compressRawDeflate(allocator, input, options),
        ZlibFormat.Zlib => compressZlib(allocator, input, options),
        ZlibFormat.Gzip => compressGzip(allocator, input, options),
        ZlibFormat.AutoDetect => compressZlib(allocator, input, options),
    };
}

/// Unified decompression entry point supporting `RawDeflate`, `Zlib`, `Gzip`, and `AutoDetect`.
pub fn decompress(
    allocator: std.mem.Allocator,
    input: []const u8,
    format: ZlibFormat,
    options: ZlibOptions,
): ([]u8) ! ZlibError {
    const resolved = switch (format) {
        ZlibFormat.AutoDetect => detectFormat(input) ?,
        else => format,
    };
    return switch (resolved) {
        ZlibFormat.RawDeflate => decompressRawDeflate(allocator, input, options),
        ZlibFormat.Zlib => decompressZlib(allocator, input, options),
        ZlibFormat.Gzip => decompressGzip(allocator, input, options),
        ZlibFormat.AutoDetect => ZlibError.InvalidFormat,
    };
}

/// Convenience sync wrapper: compress into GZIP (RFC 1952).
pub fn gzipSync(allocator: std.mem.Allocator, input: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    return compressGzip(allocator, input, options);
}

/// Convenience sync wrapper: decompress GZIP (RFC 1952).
pub fn gunzipSync(allocator: std.mem.Allocator, input: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    return decompressGzip(allocator, input, options);
}

/// Convenience sync wrapper: compress into ZLIB (RFC 1950).
pub fn deflateSync(allocator: std.mem.Allocator, input: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    return compressZlib(allocator, input, options);
}

/// Convenience sync wrapper: decompress ZLIB (RFC 1950).
pub fn inflateSync(allocator: std.mem.Allocator, input: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    return decompressZlib(allocator, input, options);
}

/// Convenience sync wrapper: compress into Raw DEFLATE (RFC 1951).
pub fn deflateRawSync(allocator: std.mem.Allocator, input: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    return compressRawDeflate(allocator, input, options);
}

/// Convenience sync wrapper: decompress Raw DEFLATE (RFC 1951).
pub fn inflateRawSync(allocator: std.mem.Allocator, input: []const u8, options: ZlibOptions): ([]u8) ! ZlibError {
    return decompressRawDeflate(allocator, input, options);
}
