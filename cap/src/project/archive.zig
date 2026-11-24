const std = @import("std");
const builtin = @import("builtin");
const main_mod = @import("../main.zig");
const native = @import("native.zig");

const ZSTD_CONTENTSIZE_UNKNOWN: c_ulonglong = std.math.maxInt(c_ulonglong);
const ZSTD_CONTENTSIZE_ERROR: c_ulonglong = std.math.maxInt(c_ulonglong) - 1;

const ZSTD_inBuffer = extern struct {
    src: ?*const anyopaque,
    size: usize,
    pos: usize,
};

const ZSTD_outBuffer = extern struct {
    dst: ?*anyopaque,
    size: usize,
    pos: usize,
};

const ZSTD_DStream = opaque {};

extern "c" fn ZSTD_compress(dst: ?*anyopaque, dstCapacity: usize, src: ?*const anyopaque, srcSize: usize, compressionLevel: c_int) usize;
extern "c" fn ZSTD_decompress(dst: ?*anyopaque, dstCapacity: usize, src: ?*const anyopaque, srcSize: usize) usize;
extern "c" fn ZSTD_compressBound(srcSize: usize) usize;
extern "c" fn ZSTD_getFrameContentSize(src: ?*const anyopaque, srcSize: usize) c_ulonglong;
extern "c" fn ZSTD_isError(code: usize) c_uint;
extern "c" fn ZSTD_createDStream() ?*ZSTD_DStream;
extern "c" fn ZSTD_freeDStream(zds: ?*ZSTD_DStream) usize;
extern "c" fn ZSTD_initDStream(zds: *ZSTD_DStream) usize;
extern "c" fn ZSTD_decompressStream(zds: *ZSTD_DStream, output: *ZSTD_outBuffer, input: *ZSTD_inBuffer) usize;

const LibdeflateCompressor = opaque {};
const LibdeflateDecompressor = opaque {};
const LIBDEFLATE_SUCCESS: c_int = 0;
const LIBDEFLATE_INSUFFICIENT_SPACE: c_int = 3;

extern "c" fn libdeflate_alloc_compressor(compression_level: c_int) ?*LibdeflateCompressor;
extern "c" fn libdeflate_free_compressor(compressor: ?*LibdeflateCompressor) void;
extern "c" fn libdeflate_gzip_compress_bound(compressor: ?*LibdeflateCompressor, in_nbytes: usize) usize;
extern "c" fn libdeflate_gzip_compress(
    compressor: *LibdeflateCompressor,
    in: ?*const anyopaque,
    in_nbytes: usize,
    out: ?*anyopaque,
    out_nbytes_avail: usize,
) usize;

extern "c" fn libdeflate_alloc_decompressor() ?*LibdeflateDecompressor;
extern "c" fn libdeflate_free_decompressor(decompressor: ?*LibdeflateDecompressor) void;
extern "c" fn libdeflate_gzip_decompress(
    decompressor: *LibdeflateDecompressor,
    in: ?*const anyopaque,
    in_nbytes: usize,
    out: ?*anyopaque,
    out_nbytes_avail: usize,
    actual_out_nbytes_ret: ?*usize,
) c_int;

pub const ArchiveFormat = enum {
    zstd,
    gzip,
};

/// Detects archive compression format from magic bytes:
/// - `28 B5 2F FD` -> Zstd (.tar.zst)
/// - `1F 8B`       -> Gzip (.tgz)
pub fn detectArchiveFormat(bytes: []const u8) !ArchiveFormat {
    if (bytes.len >= 4 and bytes[0] == 0x28 and bytes[1] == 0xB5 and bytes[2] == 0x2F and bytes[3] == 0xFD) {
        return .zstd;
    }
    if (bytes.len >= 2 and bytes[0] == 0x1F and bytes[1] == 0x8B) {
        return .gzip;
    }
    return error.UnsupportedArchiveFormat;
}

pub const PackResult = struct {
    sha256: []const u8,
    raw_size: u64,
    compressed_size: u64,
    file_count: usize,

    pub fn deinit(self: *PackResult, allocator: std.mem.Allocator) void {
        allocator.free(self.sha256);
    }
};

pub const TgzPackResult = struct {
    tgz_bytes: []u8,
    shasum: []const u8, // 40-char lowercase SHA-1 hex (npm dist.shasum)
    integrity: []const u8, // "sha512-<base64>" SRI (npm dist.integrity)
    sha256: []const u8, // "sha256-<64hex>"
    raw_size: u64,
    compressed_size: u64,
    file_count: usize,

    pub fn deinit(self: *TgzPackResult, allocator: std.mem.Allocator) void {
        allocator.free(self.tgz_bytes);
        allocator.free(self.shasum);
        allocator.free(self.integrity);
        allocator.free(self.sha256);
    }
};

/// Computes "sha256-<64 lowercase hex>" for an arbitrary byte buffer.
pub fn computeSha256Hex(allocator: std.mem.Allocator, data: []const u8) ![]const u8 {
    var hasher = std.crypto.hash.sha2.Sha256.init(.{});
    hasher.update(data);
    var digest: [32]u8 = undefined;
    hasher.final(&digest);

    const hex_chars = "0123456789abcdef";
    var out = try allocator.alloc(u8, 7 + 64);
    @memcpy(out[0..7], "sha256-");
    for (digest, 0..) |b, i| {
        out[7 + i * 2] = hex_chars[b >> 4];
        out[7 + i * 2 + 1] = hex_chars[b & 0x0f];
    }
    return out;
}

/// Computes 40-char lowercase SHA-1 hex (npm `dist.shasum`).
pub fn computeSha1Hex(allocator: std.mem.Allocator, data: []const u8) ![]const u8 {
    var hasher = std.crypto.hash.Sha1.init(.{});
    hasher.update(data);
    var digest: [20]u8 = undefined;
    hasher.final(&digest);

    const hex_chars = "0123456789abcdef";
    var out = try allocator.alloc(u8, 40);
    for (digest, 0..) |b, i| {
        out[i * 2] = hex_chars[b >> 4];
        out[i * 2 + 1] = hex_chars[b & 0x0f];
    }
    return out;
}

/// Computes npm SRI `"sha512-<base64>"` (`dist.integrity`).
pub fn computeSha512Sri(allocator: std.mem.Allocator, data: []const u8) ![]const u8 {
    var hasher = std.crypto.hash.sha2.Sha512.init(.{});
    hasher.update(data);
    var digest: [64]u8 = undefined;
    hasher.final(&digest);

    const b64_len = std.base64.standard.Encoder.calcSize(digest.len);
    var out = try allocator.alloc(u8, 7 + b64_len);
    @memcpy(out[0..7], "sha512-");
    _ = std.base64.standard.Encoder.encode(out[7..], &digest);
    return out;
}

/// Computes SRI `"sha256-<base64>"`.
pub fn computeSha256Sri(allocator: std.mem.Allocator, data: []const u8) ![]const u8 {
    var hasher = std.crypto.hash.sha2.Sha256.init(.{});
    hasher.update(data);
    var digest: [32]u8 = undefined;
    hasher.final(&digest);

    const b64_len = std.base64.standard.Encoder.calcSize(digest.len);
    var out = try allocator.alloc(u8, 7 + b64_len);
    @memcpy(out[0..7], "sha256-");
    _ = std.base64.standard.Encoder.encode(out[7..], &digest);
    return out;
}

/// Verifies archive integrity supporting:
/// - CP native `"sha256-<64hex>"`
/// - SRI `"sha256-<base64>"`
/// - npm SRI `"sha512-<base64>"`
/// - Legacy npm SHA-1 `<40hex>`
pub fn verifyIntegrity(allocator: std.mem.Allocator, data: []const u8, expected: []const u8) !void {
    if (expected.len == 0) return;

    if (std.mem.startsWith(u8, expected, "sha512-")) {
        const actual = try computeSha512Sri(allocator, data);
        defer allocator.free(actual);
        if (!std.mem.eql(u8, actual, expected)) {
            std.debug.print("Error: IntegrityMismatch (expected {s}, got {s})\n", .{ expected, actual });
            return error.IntegrityMismatch;
        }
        return;
    }

    if (std.mem.startsWith(u8, expected, "sha256-")) {
        const rest = expected["sha256-".len..];
        const actual = if (rest.len == 64)
            try computeSha256Hex(allocator, data)
        else
            try computeSha256Sri(allocator, data);
        defer allocator.free(actual);
        if (!std.mem.eql(u8, actual, expected)) {
            std.debug.print("Error: IntegrityMismatch (expected {s}, got {s})\n", .{ expected, actual });
            return error.IntegrityMismatch;
        }
        return;
    }

    if (expected.len == 40) {
        const actual = try computeSha1Hex(allocator, data);
        defer allocator.free(actual);
        if (!std.ascii.eqlIgnoreCase(actual, expected)) {
            std.debug.print("Error: IntegrityMismatch (expected sha1 {s}, got {s})\n", .{ expected, actual });
            return error.IntegrityMismatch;
        }
        return;
    }

    // Fallback: compare against sha256-<hex>
    const actual = try computeSha256Hex(allocator, data);
    defer allocator.free(actual);
    if (!std.mem.eql(u8, actual, expected)) {
        std.debug.print("Error: IntegrityMismatch (expected {s}, got {s})\n", .{ expected, actual });
        return error.IntegrityMismatch;
    }
}

/// Compresses raw bytes into RFC 1952 Gzip using linked libdeflate (default level 6).
pub fn gzipCompress(allocator: std.mem.Allocator, src: []const u8, level: c_int) ![]u8 {
    const comp = libdeflate_alloc_compressor(level) orelse return error.GzipCompressFailed;
    defer libdeflate_free_compressor(comp);

    const bound = libdeflate_gzip_compress_bound(comp, src.len);
    const dst = try allocator.alloc(u8, bound);
    errdefer allocator.free(dst);

    const src_ptr: ?*const anyopaque = if (src.len > 0) @ptrCast(src.ptr) else null;
    const written = libdeflate_gzip_compress(comp, src_ptr, src.len, @ptrCast(dst.ptr), dst.len);
    if (written == 0) return error.GzipCompressFailed;

    return try allocator.realloc(dst, written);
}

/// Decompresses an RFC 1952 Gzip buffer using linked libdeflate.
pub fn gzipDecompress(allocator: std.mem.Allocator, compressed: []const u8) ![]u8 {
    if (compressed.len < 18) return error.InvalidGzipFrame;
    if (compressed[0] != 0x1F or compressed[1] != 0x8B) return error.InvalidGzipFrame;

    const decomp = libdeflate_alloc_decompressor() orelse return error.GzipDecompressFailed;
    defer libdeflate_free_decompressor(decomp);

    // Gzip trailer stores uncompressed size mod 2^32 in little-endian at the last 4 bytes.
    const isize_hint: u32 = std.mem.readInt(u32, compressed[compressed.len - 4 ..][0..4], .little);
    const max_cap: usize = 512 * 1024 * 1024;
    var cap: usize = if (isize_hint > 0 and isize_hint <= max_cap)
        @as(usize, isize_hint)
    else
        @max(compressed.len * 4, 64 * 1024);
    if (cap > max_cap) cap = max_cap;

    while (true) {
        const dst = try allocator.alloc(u8, cap);
        var actual_out: usize = 0;
        const rc = libdeflate_gzip_decompress(
            decomp,
            @ptrCast(compressed.ptr),
            compressed.len,
            @ptrCast(dst.ptr),
            dst.len,
            &actual_out,
        );
        if (rc == LIBDEFLATE_SUCCESS) {
            if (actual_out != dst.len) {
                return try allocator.realloc(dst, actual_out);
            }
            return dst;
        }
        allocator.free(dst);
        if (rc == LIBDEFLATE_INSUFFICIENT_SPACE and cap < max_cap) {
            cap = @min(cap * 2, max_cap);
            continue;
        }
        return error.GzipDecompressFailed;
    }
}

/// Compresses raw bytes using linked libzstd at the specified level (e.g. 6).
pub fn zstdCompress(allocator: std.mem.Allocator, src: []const u8, level: c_int) ![]u8 {
    const bound = ZSTD_compressBound(src.len);
    if (ZSTD_isError(bound) != 0) return error.ZstdCompressFailed;

    const dst = try allocator.alloc(u8, bound);
    errdefer allocator.free(dst);

    const src_ptr: ?*const anyopaque = if (src.len > 0) @ptrCast(src.ptr) else null;
    const written = ZSTD_compress(@ptrCast(dst.ptr), dst.len, src_ptr, src.len, level);
    if (ZSTD_isError(written) != 0) return error.ZstdCompressFailed;

    return try allocator.realloc(dst, written);
}

/// Decompresses a Zstd frame; supports both known-size single-shot and streaming fallback.
pub fn zstdDecompress(allocator: std.mem.Allocator, compressed: []const u8) ![]u8 {
    if (compressed.len == 0) return error.InvalidZstdFrame;

    const content_size = ZSTD_getFrameContentSize(@ptrCast(compressed.ptr), compressed.len);
    if (content_size == ZSTD_CONTENTSIZE_ERROR) {
        return error.InvalidZstdFrame;
    }

    const max_single_shot: u64 = 512 * 1024 * 1024;
    if (content_size != ZSTD_CONTENTSIZE_UNKNOWN and content_size <= max_single_shot) {
        const expected_len: usize = @intCast(content_size);
        const dst = try allocator.alloc(u8, expected_len);
        errdefer allocator.free(dst);

        if (expected_len == 0) return dst;
        const actual = ZSTD_decompress(@ptrCast(dst.ptr), dst.len, @ptrCast(compressed.ptr), compressed.len);
        if (ZSTD_isError(actual) != 0) return error.ZstdDecompressFailed;
        if (actual != expected_len) {
            return try allocator.realloc(dst, actual);
        }
        return dst;
    }

    // Streaming fallback for frames without content size in header
    const dstream = ZSTD_createDStream() orelse return error.ZstdDecompressFailed;
    defer _ = ZSTD_freeDStream(dstream);

    if (ZSTD_isError(ZSTD_initDStream(dstream)) != 0) return error.ZstdDecompressFailed;

    var out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    var in_buf = ZSTD_inBuffer{
        .src = @ptrCast(compressed.ptr),
        .size = compressed.len,
        .pos = 0,
    };
    var chunk: [64 * 1024]u8 = undefined;

    while (in_buf.pos < in_buf.size) {
        var out_buf = ZSTD_outBuffer{
            .dst = @ptrCast(&chunk),
            .size = chunk.len,
            .pos = 0,
        };
        const rc = ZSTD_decompressStream(dstream, &out_buf, &in_buf);
        if (ZSTD_isError(rc) != 0) return error.ZstdDecompressFailed;
        if (out_buf.pos > 0) {
            try out.appendSlice(allocator, chunk[0..out_buf.pos]);
        }
        if (rc == 0 and in_buf.pos == in_buf.size) break;
    }
    return try out.toOwnedSlice(allocator);
}

fn normalizeTarPath(allocator: std.mem.Allocator, raw: []const u8) ![]u8 {
    var out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    var s = raw;
    while (std.mem.startsWith(u8, s, "./") or std.mem.startsWith(u8, s, ".\\")) {
        s = s[2..];
    }
    while (s.len > 0 and (s[0] == '/' or s[0] == '\\')) {
        s = s[1..];
    }
    for (s) |c| {
        if (c == '\\') {
            try out.append(allocator, '/');
        } else {
            try out.append(allocator, c);
        }
    }
    return try out.toOwnedSlice(allocator);
}

fn writeOctal(buf: []u8, value: u64) void {
    @memset(buf, '0');
    if (buf.len == 0) return;
    buf[buf.len - 1] = 0;
    var v = value;
    var i: usize = buf.len - 1;
    while (i > 0) {
        i -= 1;
        buf[i] = @intCast('0' + (v & 7));
        v >>= 3;
    }
}

fn parseOctal(field: []const u8) !u64 {
    var s = field;
    // Strip trailing NULs and spaces
    while (s.len > 0 and (s[s.len - 1] == 0 or s[s.len - 1] == ' ')) {
        s = s[0 .. s.len - 1];
    }
    // Strip leading spaces and NULs
    while (s.len > 0 and (s[0] == ' ' or s[0] == 0)) {
        s = s[1..];
    }
    if (s.len == 0) return 0;
    var val: u64 = 0;
    for (s) |c| {
        if (c < '0' or c > '7') return error.InvalidTarHeader;
        val = (val << 3) | @as(u64, c - '0');
    }
    return val;
}

fn readFileBytesAny(allocator: std.mem.Allocator, path: []const u8) ![]u8 {
    if (path.len > 0 and path[0] == '/') {
        var root_dir = try std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{});
        defer root_dir.close(main_mod.global_io);
        return try root_dir.readFileAlloc(main_mod.global_io, path[1..], allocator, @fromBackingInt(@intCast(256 * 1024 * 1024)));
    }
    return try std.Io.Dir.readFileAlloc(.cwd(), main_mod.global_io, path, allocator, @fromBackingInt(@intCast(256 * 1024 * 1024)));
}

fn statFileModeAny(path: []const u8) u32 {
    if (builtin.os.tag == .windows) return 0o644;
    const st = blk: {
        if (path.len > 0 and path[0] == '/') {
            var root_dir = std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{}) catch return 0o644;
            defer root_dir.close(main_mod.global_io);
            break :blk root_dir.statFile(main_mod.global_io, path[1..], .{}) catch return 0o644;
        }
        break :blk std.Io.Dir.statFile(.cwd(), main_mod.global_io, path, .{}) catch return 0o644;
    };
    const mode = st.permissions.toMode();
    return if ((mode & 0o111) != 0) 0o755 else 0o644;
}

fn writeFileBytesAny(path: []const u8, data: []const u8, is_exec: bool) !void {
    if (std.Io.Dir.path.dirname(path)) |parent| {
        try native.ensureDirPath(parent);
    }
    var out_file = blk: {
        if (path.len > 0 and path[0] == '/') {
            var root_dir = try std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{});
            defer root_dir.close(main_mod.global_io);
            break :blk try root_dir.createFile(main_mod.global_io, path[1..], .{});
        } else {
            break :blk try std.Io.Dir.createFile(.cwd(), main_mod.global_io, path, .{});
        }
    };
    defer out_file.close(main_mod.global_io);
    try out_file.writeStreamingAll(main_mod.global_io, data);

    if (builtin.os.tag != .windows and is_exec) {
        out_file.setPermissions(main_mod.global_io, .fromMode(0o755)) catch {};
    }
}

fn strLessThan(_: void, a: []const u8, b: []const u8) bool {
    return std.mem.order(u8, a, b) == .lt;
}

fn appendTarEntry(
    allocator: std.mem.Allocator,
    tar_buf: *std.ArrayList(u8),
    entry_path: []const u8,
    file_data: []const u8,
    file_mode: u32,
) !void {
    var header: [512]u8 = @splat(0);

    // Split path into USTAR name (<=100) and prefix (<=155) if needed
    if (entry_path.len <= 100) {
        @memcpy(header[0..entry_path.len], entry_path);
    } else {
        var split_pos: ?usize = null;
        var idx: usize = entry_path.len;
        while (idx > 0) {
            idx -= 1;
            if (entry_path[idx] == '/') {
                const prefix_len = idx;
                const name_len = entry_path.len - idx - 1;
                if (prefix_len <= 155 and name_len <= 100 and name_len > 0) {
                    split_pos = idx;
                    break;
                }
            }
        }
        const sp = split_pos orelse return error.PathTooLong;
        const prefix = entry_path[0..sp];
        const name_part = entry_path[sp + 1 ..];
        @memcpy(header[0..name_part.len], name_part);
        @memcpy(header[345 .. 345 + prefix.len], prefix);
    }

    writeOctal(header[100..108], file_mode);
    writeOctal(header[108..116], 0);
    writeOctal(header[116..124], 0);
    writeOctal(header[124..136], file_data.len);
    writeOctal(header[136..148], 0); // deterministic mtime
    @memset(header[148..156], ' ');
    header[156] = '0'; // regular file
    @memcpy(header[257..263], "ustar\x00");
    @memcpy(header[263..265], "00");

    var chksum: u64 = 0;
    for (header) |b| chksum += b;
    writeOctal(header[148..155], chksum);
    header[155] = ' ';

    try tar_buf.appendSlice(allocator, &header);
    if (file_data.len > 0) {
        try tar_buf.appendSlice(allocator, file_data);
        const rem = file_data.len % 512;
        if (rem != 0) {
            const pad = 512 - rem;
            const zeros: [512]u8 = @splat(0);
            try tar_buf.appendSlice(allocator, zeros[0..pad]);
        }
    }
}

fn buildTarBuffer(
    allocator: std.mem.Allocator,
    root_dir: []const u8,
    rel_paths: []const []const u8,
    path_prefix: ?[]const u8,
    extra_virtual_rel: ?[]const u8,
    extra_virtual_bytes: ?[]const u8,
) !struct { tar: []u8, file_count: usize } {
    const sorted = try allocator.alloc([]const u8, rel_paths.len);
    defer allocator.free(sorted);
    @memcpy(sorted, rel_paths);
    std.mem.sort([]const u8, sorted, {}, strLessThan);

    var tar_buf: std.ArrayList(u8) = .empty;
    errdefer tar_buf.deinit(allocator);

    var count: usize = 0;
    var has_extra = false;

    for (sorted) |rel_raw| {
        const norm_rel = try normalizeTarPath(allocator, rel_raw);
        defer allocator.free(norm_rel);
        if (norm_rel.len == 0) continue;

        if (extra_virtual_rel) |ev| {
            if (std.mem.eql(u8, norm_rel, ev)) has_extra = true;
        }

        const full_path = if (std.mem.eql(u8, root_dir, ".") or root_dir.len == 0)
            try allocator.dupe(u8, norm_rel)
        else
            try std.Io.Dir.path.join(allocator, &[_][]const u8{ root_dir, norm_rel });
        defer allocator.free(full_path);

        const file_data = try readFileBytesAny(allocator, full_path);
        defer allocator.free(file_data);

        const file_mode = statFileModeAny(full_path);

        const entry_path = if (path_prefix) |pfx|
            try std.fmt.allocPrint(allocator, "{s}{s}", .{ pfx, norm_rel })
        else
            try allocator.dupe(u8, norm_rel);
        defer allocator.free(entry_path);

        try appendTarEntry(allocator, &tar_buf, entry_path, file_data, file_mode);
        count += 1;
    }

    if (extra_virtual_rel) |ev| {
        if (!has_extra) {
            if (extra_virtual_bytes) |ev_bytes| {
                const entry_path = if (path_prefix) |pfx|
                    try std.fmt.allocPrint(allocator, "{s}{s}", .{ pfx, ev })
                else
                    try allocator.dupe(u8, ev);
                defer allocator.free(entry_path);
                try appendTarEntry(allocator, &tar_buf, entry_path, ev_bytes, 0o644);
                count += 1;
            }
        }
    }

    // Two 512-byte zero blocks mark end-of-archive
    const eof_blocks: [1024]u8 = @splat(0);
    try tar_buf.appendSlice(allocator, &eof_blocks);

    return .{
        .tar = try tar_buf.toOwnedSlice(allocator),
        .file_count = count,
    };
}

/// Packs `rel_paths` under `root_dir` into a deterministic POSIX USTAR TAR compressed with Zstd (level 6).
/// Writes the resulting `.tar.zst` to `out_path` and returns PackResult containing `"sha256-<64hex>"`.
pub fn packDirToTarZst(
    allocator: std.mem.Allocator,
    root_dir: []const u8,
    rel_paths: []const []const u8,
    out_path: []const u8,
) !PackResult {
    const built = try buildTarBuffer(allocator, root_dir, rel_paths, null, null, null);
    defer allocator.free(built.tar);

    const compressed = try zstdCompress(allocator, built.tar, 6);
    defer allocator.free(compressed);

    const sha256_str = try computeSha256Hex(allocator, compressed);
    errdefer allocator.free(sha256_str);

    try writeFileBytesAny(out_path, compressed, false);

    return PackResult{
        .sha256 = sha256_str,
        .raw_size = built.tar.len,
        .compressed_size = compressed.len,
        .file_count = built.file_count,
    };
}

/// Packs `rel_paths` under `root_dir` into an npm-compatible RFC 1952 Gzip `.tgz`
/// with `"package/"` prefix and auto-injected `package/package.json` (if not already present).
/// Optionally writes to `out_path` when non-null.
pub fn packDirToTgz(
    allocator: std.mem.Allocator,
    root_dir: []const u8,
    rel_paths: []const []const u8,
    synthetic_pkg_json: ?[]const u8,
    out_path: ?[]const u8,
) !TgzPackResult {
    const built = try buildTarBuffer(
        allocator,
        root_dir,
        rel_paths,
        "package/",
        if (synthetic_pkg_json != null) "package.json" else null,
        synthetic_pkg_json,
    );
    defer allocator.free(built.tar);

    const compressed = try gzipCompress(allocator, built.tar, 6);
    errdefer allocator.free(compressed);

    const shasum = try computeSha1Hex(allocator, compressed);
    errdefer allocator.free(shasum);
    const integrity = try computeSha512Sri(allocator, compressed);
    errdefer allocator.free(integrity);
    const sha256_str = try computeSha256Hex(allocator, compressed);
    errdefer allocator.free(sha256_str);

    if (out_path) |op| {
        try writeFileBytesAny(op, compressed, false);
    }

    return TgzPackResult{
        .tgz_bytes = compressed,
        .shasum = shasum,
        .integrity = integrity,
        .sha256 = sha256_str,
        .raw_size = built.tar.len,
        .compressed_size = compressed.len,
        .file_count = built.file_count,
    };
}

/// Validates a tar member path against path traversal, absolute paths, drive letters, and NUL bytes.
pub fn validateTarPath(path: []const u8) !void {
    if (path.len == 0) return error.UnsafeArchivePath;
    if (path[0] == '/' or path[0] == '\\') return error.UnsafeArchivePath;
    // Reject Windows drive-letter absolute paths like "C:/..." or "C:\..."
    if (path.len >= 2 and path[1] == ':') return error.UnsafeArchivePath;

    for (path) |c| {
        if (c == 0 or c == '\\') return error.UnsafeArchivePath;
    }

    var it = std.mem.splitScalar(u8, path, '/');
    while (it.next()) |seg| {
        if (std.mem.eql(u8, seg, "..")) return error.UnsafeArchivePath;
    }
}

fn cStrSlice(field: []const u8) []const u8 {
    for (field, 0..) |c, i| {
        if (c == 0) return field[0..i];
    }
    return field;
}

fn isAllZeroBlock(block: []const u8) bool {
    for (block) |b| {
        if (b != 0) return false;
    }
    return true;
}

/// Verifies integrity (supports `sha256-<hex>`, `sha256-<b64>`, `sha512-<b64>`, SHA-1 hex),
/// auto-detects Zstd (`28 B5 2F FD`) vs Gzip (`1F 8B`) by magic bytes, decompresses,
/// automatically strips npm's top-level `package/` prefix on `.tgz` archives,
/// validates every TAR entry path against traversal, and extracts files into `dest_dir`.
pub fn extractArchiveAuto(
    allocator: std.mem.Allocator,
    archive_bytes: []const u8,
    dest_dir: []const u8,
    expected_integrity: ?[]const u8,
) !void {
    if (expected_integrity) |expected| {
        if (expected.len > 0) {
            try verifyIntegrity(allocator, archive_bytes, expected);
        }
    }

    const fmt = try detectArchiveFormat(archive_bytes);
    const tar_bytes = switch (fmt) {
        .zstd => try zstdDecompress(allocator, archive_bytes),
        .gzip => try gzipDecompress(allocator, archive_bytes),
    };
    defer allocator.free(tar_bytes);

    try native.ensureDirPath(dest_dir);

    var offset: usize = 0;
    while (offset + 512 <= tar_bytes.len) {
        const hdr = tar_bytes[offset .. offset + 512];
        offset += 512;

        if (isAllZeroBlock(hdr)) break;

        // Verify header checksum
        const stored_sum = try parseOctal(hdr[148..156]);
        var calc_sum: u64 = 0;
        for (hdr, 0..) |b, i| {
            calc_sum += if (i >= 148 and i < 156) @as(u64, ' ') else @as(u64, b);
        }
        if (calc_sum != stored_sum) return error.InvalidTarHeader;

        const file_size = try parseOctal(hdr[124..136]);
        const file_mode = parseOctal(hdr[100..108]) catch 0o644;
        const typeflag = hdr[156];

        const name_part = cStrSlice(hdr[0..100]);
        const prefix_part = cStrSlice(hdr[345..500]);

        const padded_size: usize = std.mem.alignForward(usize, @intCast(file_size), 512);
        if (offset + padded_size > tar_bytes.len) return error.TruncatedTarArchive;
        const content = tar_bytes[offset .. offset + @as(usize, @intCast(file_size))];
        offset += padded_size;

        // Skip PAX extended headers ('x', 'g')
        if (typeflag == 'x' or typeflag == 'g') continue;

        const raw_rel = if (prefix_part.len > 0)
            try std.fmt.allocPrint(allocator, "{s}/{s}", .{ prefix_part, name_part })
        else
            try allocator.dupe(u8, name_part);
        defer allocator.free(raw_rel);

        // Strip leading "./" and trailing "/" before path checks
        var clean_rel: []const u8 = raw_rel;
        while (std.mem.startsWith(u8, clean_rel, "./")) {
            clean_rel = clean_rel[2..];
        }
        while (clean_rel.len > 0 and clean_rel[clean_rel.len - 1] == '/') {
            clean_rel = clean_rel[0 .. clean_rel.len - 1];
        }
        // For npm .tgz archives, automatically strip the top-level "package/" prefix
        if (fmt == .gzip) {
            if (std.mem.eql(u8, clean_rel, "package")) continue;
            if (std.mem.startsWith(u8, clean_rel, "package/")) {
                clean_rel = clean_rel["package/".len..];
            }
        }
        if (clean_rel.len == 0 or std.mem.eql(u8, clean_rel, ".")) continue;

        try validateTarPath(clean_rel);

        const out_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ dest_dir, clean_rel });
        defer allocator.free(out_path);

        if (typeflag == '5') {
            try native.ensureDirPath(out_path);
        } else if (typeflag == '0' or typeflag == 0) {
            const is_exec = (file_mode & 0o111) != 0;
            try writeFileBytesAny(out_path, content, is_exec);
        }
    }
}

/// Backward-compatible alias for `extractArchiveAuto`.
pub fn extractTarZstWithVerify(
    allocator: std.mem.Allocator,
    archive_bytes: []const u8,
    dest_dir: []const u8,
    expected_integrity: ?[]const u8,
) !void {
    return extractArchiveAuto(allocator, archive_bytes, dest_dir, expected_integrity);
}
