// Source: Absorbed from /data/dev/bun/src/http/Decompressor.cp | Synced: 2026-11-23T01:58:49Z
// lib/http/decompressor.cp
// CP HTTP Stream Decompressor Bridged to cap:pack (cap:http)
// RFC 1952 (Gzip), RFC 1950/1951 (Deflate/Inflate), Identity

const std = @import("std");
import { Encoding } from "./encoding";
import {
    gunzip,
    inflate,
    inflateRaw,
    GunzipStream,
    InflateStream,
    ZlibStreamDecompressor,
    WINDOW_BITS_RAW,
    WINDOW_BITS_AUTO,
} from "../pack/gzip.cp";

pub enum DecompressorType {
    None,
    Gzip,
    Deflate,
    Brotli,
    Zstd,
}

pub struct Decompressor {
    pub decompressor_type: DecompressorType = DecompressorType.None;
    pub is_done: bool = false;

    pub static fn init(encoding: Encoding): Decompressor {
        const dt = switch (encoding) {
            .Gzip => DecompressorType.Gzip,
            .Deflate => DecompressorType.Deflate,
            .Brotli => DecompressorType.Brotli,
            .Zstd => DecompressorType.Zstd,
            else => DecompressorType.None,
        };
        return Decompressor{
            decompressor_type: dt,
            is_done: false,
        };
    }

    /// Strict decompression via `cap:pack` for gzip/deflate/identity (returns error if payload is corrupt or unsupported).
    pub fn decompressStrict(*_, input: []const u8, allocator: std.mem.Allocator): ([]const u8)! {
        if (_.decompressor_type == .None || input.len == 0) {
            return allocator.dupe(u8, input)?;
        }
        switch (_.decompressor_type) {
            .Gzip => {
                const out = gunzip(input) ?? return error.DecompressionFailed;
                const copy = allocator.dupe(u8, out)?;
                _.is_done = true;
                return copy;
            },
            .Deflate => {
                if (out := inflate(input)) {
                    const copy = allocator.dupe(u8, out)?;
                    _.is_done = true;
                    return copy;
                }
                if (raw_out := inflateRaw(input)) {
                    const copy = allocator.dupe(u8, raw_out)?;
                    _.is_done = true;
                    return copy;
                }
                return error.DecompressionFailed;
            },
            .Brotli, .Zstd => {
                return error.UnsupportedEncoding;
            },
            .None => return allocator.dupe(u8, input)?,
        }
    }

    /// Decompresses gzip/deflate slices when valid headers are present, falling back to passthrough duping for unencoded inputs.
    pub fn decompressSlice(*_, input: []const u8, allocator: std.mem.Allocator): ([]const u8)! {
        if (_.decompressor_type == .None || input.len == 0) {
            return allocator.dupe(u8, input)?;
        }
        switch (_.decompressor_type) {
            .Gzip => {
                if (input.len >= 2 && input[0] == 0x1F && input[1] == 0x8B) {
                    if (out := gunzip(input)) {
                        _.is_done = true;
                        return allocator.dupe(u8, out)?;
                    }
                }
                return allocator.dupe(u8, input)?;
            },
            .Deflate => {
                if (out := inflate(input)) {
                    _.is_done = true;
                    return allocator.dupe(u8, out)?;
                }
                if (raw_out := inflateRaw(input)) {
                    _.is_done = true;
                    return allocator.dupe(u8, raw_out)?;
                }
                return allocator.dupe(u8, input)?;
            },
            else => return allocator.dupe(u8, input)?,
        }
    }

    ~init() {
        _.decompressor_type = .None;
        _.is_done = true;
    }
}

/// Incremental streaming HTTP Content-Encoding decompressor backed by `cap:pack` zlib streams.
pub struct StreamingDecompressor {
    pub encoding: Encoding = Encoding.Identity,
    pub stream: (*ZlibStreamDecompressor)? = null,
    pub is_finished: bool = false,

    pub static fn init(encoding: Encoding): StreamingDecompressor {
        let sd = StreamingDecompressor{
            encoding: encoding,
            stream: null,
            is_finished: false,
        };
        if (encoding == Encoding.Gzip) {
            sd.stream = GunzipStream.create();
        } else if (encoding == Encoding.Deflate) {
            sd.stream = InflateStream.create();
        }
        return sd;
    }

    pub static fn initRawDeflate(): StreamingDecompressor {
        return StreamingDecompressor{
            encoding: Encoding.Deflate,
            stream: ZlibStreamDecompressor.create(WINDOW_BITS_RAW),
            is_finished: false,
        };
    }

    pub fn feed(*_, chunk: []const u8, allocator: std.mem.Allocator): ([]const u8)! {
        if (_.encoding == Encoding.Identity || _.stream == null) {
            return allocator.dupe(u8, chunk)?;
        }
        let s = _.stream.?;
        const out = s.write(chunk);
        return allocator.dupe(u8, out)?;
    }

    pub fn finish(*_, allocator: std.mem.Allocator): ([]const u8)! {
        _.is_finished = true;
        if (_.stream == null) {
            return allocator.dupe(u8, "")?;
        }
        let s = _.stream.?;
        const out = s.finish();
        return allocator.dupe(u8, out)?;
    }

    ~init() {
        _.stream = null;
        _.is_finished = true;
    }
}
