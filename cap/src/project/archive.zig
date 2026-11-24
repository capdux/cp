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

pub const PackResult = struct {
    sha256: []const u8,
    raw_size: u64,
    compressed_size: u64,
    file_count: usize,

    pub fn deinit(self: *PackResult, allocator: std.mem.Allocator) void {
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

/// Packs `rel_paths` under `root_dir` into a deterministic POSIX USTAR TAR compressed with Zstd (level 6).
/// Writes the resulting `.tar.zst` to `out_path` and returns PackResult containing `"sha256-<64hex>"`.
pub fn packDirToTarZst(
    allocator: std.mem.Allocator,
    root_dir: []const u8,
    rel_paths: []const []const u8,
    out_path: []const u8,
) !PackResult {
    // Sort a copy of rel_paths for deterministic tar output
    const sorted = try allocator.alloc([]const u8, rel_paths.len);
    defer allocator.free(sorted);
    @memcpy(sorted, rel_paths);
    std.mem.sort([]const u8, sorted, {}, strLessThan);

    var tar_buf: std.ArrayList(u8) = .empty;
    defer tar_buf.deinit(allocator);

    for (sorted) |rel_raw| {
        const norm_rel = try normalizeTarPath(allocator, rel_raw);
        defer allocator.free(norm_rel);
        if (norm_rel.len == 0) continue;

        const full_path = if (std.mem.eql(u8, root_dir, ".") or root_dir.len == 0)
            try allocator.dupe(u8, norm_rel)
        else
            try std.Io.Dir.path.join(allocator, &[_][]const u8{ root_dir, norm_rel });
        defer allocator.free(full_path);

        const file_data = try readFileBytesAny(allocator, full_path);
        defer allocator.free(file_data);

        const file_mode = statFileModeAny(full_path);

        var header: [512]u8 = @splat(0);

        // Split path into USTAR name (<=100) and prefix (<=155) if needed
        if (norm_rel.len <= 100) {
            @memcpy(header[0..norm_rel.len], norm_rel);
        } else {
            var split_pos: ?usize = null;
            var idx: usize = norm_rel.len;
            while (idx > 0) {
                idx -= 1;
                if (norm_rel[idx] == '/') {
                    const prefix_len = idx;
                    const name_len = norm_rel.len - idx - 1;
                    if (prefix_len <= 155 and name_len <= 100 and name_len > 0) {
                        split_pos = idx;
                        break;
                    }
                }
            }
            const sp = split_pos orelse return error.PathTooLong;
            const prefix = norm_rel[0..sp];
            const name_part = norm_rel[sp + 1 ..];
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

    // Two 512-byte zero blocks mark end-of-archive
    const eof_blocks: [1024]u8 = @splat(0);
    try tar_buf.appendSlice(allocator, &eof_blocks);

    const compressed = try zstdCompress(allocator, tar_buf.items, 6);
    defer allocator.free(compressed);

    const sha256_str = try computeSha256Hex(allocator, compressed);
    errdefer allocator.free(sha256_str);

    try writeFileBytesAny(out_path, compressed, false);

    return PackResult{
        .sha256 = sha256_str,
        .raw_size = tar_buf.items.len,
        .compressed_size = compressed.len,
        .file_count = sorted.len,
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

/// Verifies SHA-256 integrity (if expected_integrity is non-empty), decompresses Zstd,
/// validates every TAR entry path against traversal, and extracts files into `dest_dir`.
pub fn extractTarZstWithVerify(
    allocator: std.mem.Allocator,
    archive_bytes: []const u8,
    dest_dir: []const u8,
    expected_integrity: ?[]const u8,
) !void {
    if (expected_integrity) |expected| {
        if (expected.len > 0) {
            const actual = try computeSha256Hex(allocator, archive_bytes);
            defer allocator.free(actual);
            if (!std.mem.eql(u8, actual, expected)) {
                std.debug.print("Error: IntegrityMismatch (expected {s}, got {s})\n", .{ expected, actual });
                return error.IntegrityMismatch;
            }
        }
    }

    const tar_bytes = try zstdDecompress(allocator, archive_bytes);
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

        // Strip trailing slash for directory entries before path validation
        var clean_rel = raw_rel;
        while (clean_rel.len > 0 and clean_rel[clean_rel.len - 1] == '/') {
            clean_rel = clean_rel[0 .. clean_rel.len - 1];
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
