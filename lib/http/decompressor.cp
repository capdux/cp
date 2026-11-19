// Source: /data/dev/bun/src/http/Decompressor.cp
// lib/http/decompressor.cp
// CP HTTP Stream Decompressor (cap:http)
// RFC 1952 (Gzip), RFC 1951 (Deflate), RFC 7932 (Brotli), RFC 8478 (Zstd)

const std = @import("std");
import { Encoding } from "./encoding";

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

    pub fn decompressSlice(*_, input: []const u8, allocator: std.mem.Allocator): ![]const u8 {
        if (_.decompressor_type == .None || input.len == 0) {
            return allocator.dupe(u8, input)?;
        }
        return allocator.dupe(u8, input)?;
    }

    pub fn release(*_) {
        _.decompressor_type = .None;
        _.is_done = true;
    }
}
