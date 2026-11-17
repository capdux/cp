// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
// lib/pack/native.cp
// Native decompression using Zig std.compress

import { Allocator } from "cap:mem";

zig {
    const std = @import("std");

    pub fn decompress(
        allocator: std.mem.Allocator,
        src: []const u8,
        comptime Decompressor: type,
    ) ![]u8 {
        var arena = std.heap.ArenaAllocator.init(allocator);
        defer arena.deinit();
        
        var stream = std.io.fixedBufferStream(src);
        var decompressor = try Decompressor.init(arena.allocator(), stream.reader());
        
        return try decompressor.reader().readAllAlloc(allocator, 1024 * 1024 * 10); // Limit 10MB
    }
}

pub fn decompressDeflateNative(allocator: *Allocator, src: []const u8) ![]u8 {
    zig {
        return decompress(allocator.*, src, std.compress.flate.Decompress);
    }
}

pub fn decompressZlibNative(allocator: Allocator, src: []const u8) ![]u8 {
    zig {
        return decompress(allocator, src, std.compress.flate.Decompress);
    }
}

pub fn decompressGzipNative(allocator: Allocator, src: []const u8) ![]u8 {
    zig {
        return decompress(allocator, src, std.compress.flate.Decompress);
    }
}

pub fn decompressZstdNative(allocator: Allocator, src: []const u8) ![]u8 {
    zig {
        return decompress(allocator, src, std.compress.zstd.Decompress);
    }
}
