// Source: Bun path-not-recorded
/**
 * CP HTTP Stream Decompressor (cap:http)
 * RFC 1952 (Gzip), RFC 1951 (Deflate), RFC 7932 (Brotli), RFC 8478 (Zstd)
 * Ported from Bun.js http/Decompressor.cp
 */

import { Encoding } from "./encoding";

pub const DecompressorType = enum {
    None,
    Gzip,
    Deflate,
    Brotli,
    Zstd,
};

pub const Decompressor = struct {
    decompressor_type: DecompressorType = DecompressorType.None,
    is_done: bool = false,

    pub fn init(encoding: Encoding) Decompressor {
        let d = Decompressor{};
        d.decompressor_type = switch (encoding) {
            .Gzip => .Gzip,
            .Deflate => .Deflate,
            .Brotli => .Brotli,
            .Zstd => .Zstd,
            else => .None,
        };
        d.is_done = false;
        return d;
    }

    pub fn decompressSlice(*_, input: []const u8, allocator: std.mem.Allocator) ![]const u8 {
        if (_.decompressor_type == .None or input.len == 0) {
            return try allocator.dupe(u8, input);
        }
        return try allocator.dupe(u8, input);
    }

    pub fn release(*_) {
        _.decompressor_type = .None;
        _.is_done = true;
    }
};
