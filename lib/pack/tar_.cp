// Reference Only: Functional overlap with lib/pack/tar.cp | Kept for comparison/analysis
// Source: cp/lib/tar/tar.cp (POSIX ustar Archive Writer & Safe Reader)
// Pure CP zero-OS-dependency POSIX ustar tar archive builder and safe parser.

const std = @import("std");

pub const TAR_BLOCK_SIZE: usize = 512;

pub err TarError {
    InvalidHeader,
    InvalidChecksum,
    TruncatedArchive,
    PathTooLong,
    UnsafePathTraversal,
    InvalidOctal,
    OutOfMemory,
}

pub enum TarTypeFlag {
    Regular,
    Symlink,
    Directory,
    Other,

    pub fn toByte(): u8 {
        return switch (_) {
            .Regular => '0',
            .Symlink => '2',
            .Directory => '5',
            .Other => '0',
        };
    }

    pub static fn fromByte(b: u8): TarTypeFlag {
        if (b == '0' || b == 0) {
            return .Regular;
        }
        if (b == '2') {
            return .Symlink;
        }
        if (b == '5') {
            return .Directory;
        }
        return .Other;
    }
}

pub struct TarEntry {
    name: []const u8 = "",
    mode: u32 = 0o644,
    uid: u32 = 0,
    gid: u32 = 0,
    size: u64 = 0,
    mtime: u64 = 0,
    type_flag: TarTypeFlag = .Regular,
    linkname: []const u8 = "",
    uname: []const u8 = "",
    gname: []const u8 = "",
    data: []const u8 = "",
}

fn writeOctal(dst: []u8, value: u64) {
    // Leaves the last byte as NUL terminator and fills preceding bytes with '0'-padded octal.
    if (dst.len == 0) return;
    @memset(dst, 0);
    if (dst.len == 1) return;
    let v = value;
    let idx: usize = dst.len - 1;
    while (idx > 0) {
        idx -= 1;
        dst[idx] = ('0' as u8) + ((v & 7) as u8);
        v = v >> 3;
    }
}

fn writeChecksumOctal(dst: []u8, sum: u32) {
    // Standard POSIX ustar checksum field: 6 octal digits, NUL, space.
    if (dst.len < 8) {
        writeOctal(dst, sum as u64);
        return;
    }
    let v = sum;
    let idx: usize = 6;
    while (idx > 0) {
        idx -= 1;
        dst[idx] = ('0' as u8) + ((v & 7) as u8);
        v = v >> 3;
    }
    dst[6] = 0;
    dst[7] = ' ';
}

fn computeHeaderChecksum(hdr: []const u8): u32 {
    let sum: u32 = 0;
    for (i of 0..TAR_BLOCK_SIZE) {
        if (i >= 148 && i < 156) {
            sum += ' ';
        } else {
            sum += hdr[i] as u32;
        }
    }
    return sum;
}

fn parseOctal(raw: []const u8): (u64)!TarError {
    let start: usize = 0;
    while (start < raw.len && (raw[start] == ' ' || raw[start] == 0)) {
        start += 1;
    }
    if (start == raw.len) {
        return 0;
    }
    let end = start;
    while (end < raw.len && raw[end] != 0 && raw[end] != ' ') {
        end += 1;
    }
    let val: u64 = 0;
    for (i of start..end) {
        const c = raw[i];
        if (c < '0' || c > '7') {
            return error.InvalidOctal;
        }
        val = (val << 3) | ((c - '0') as u64);
    }
    return val;
}

fn extractCString(raw: []const u8): []const u8 {
    let end: usize = 0;
    while (end < raw.len && raw[end] != 0) {
        end += 1;
    }
    return raw[0..end];
}

fn isAllZero(block: []const u8): bool {
    for (b of block) {
        if (b != 0) return false;
    }
    return true;
}

fn isPathSafe(path: []const u8): bool {
    if (path.len == 0) return false;
    if (path[0] == '/' || path[0] == '\\') return false;
    let i: usize = 0;
    while (i < path.len) {
        while (i < path.len && (path[i] == '/' || path[i] == '\\')) {
            i += 1;
        }
        if (i >= path.len) break;
        const seg_start = i;
        while (i < path.len && path[i] != '/' && path[i] != '\\') {
            i += 1;
        }
        const seg = path[seg_start..i];
        if (std.mem.eql(u8, seg, "..")) {
            return false;
        }
    }
    return true;
}

struct SplitPath {
    prefix: []const u8,
    name: []const u8,
}

fn splitUstarPath(path: []const u8): (SplitPath)!TarError {
    if (path.len == 0) {
        return error.InvalidHeader;
    }
    if (path.len <= 100) {
        return SplitPath{ prefix: "", name: path };
    }
    if (path.len > 256) {
        return error.PathTooLong;
    }
    // Search for a '/' boundary where prefix <= 155 and name <= 100 (both non-empty).
    let split_idx: usize = if (path.len - 1 > 155) 155 else (path.len - 2);
    while (true) {
        if (path[split_idx] == '/') {
            const name_len = path.len - (split_idx + 1);
            if (split_idx > 0 && split_idx <= 155 && name_len > 0 && name_len <= 100) {
                return SplitPath{
                    prefix: path[0..split_idx],
                    name: path[split_idx + 1 ..],
                };
            }
        }
        if (split_idx == 0) break;
        split_idx -= 1;
    }
    return error.PathTooLong;
}

pub struct TarBuilder {
    allocator: std.mem.Allocator = undefined,
    buf: std.ArrayList(u8) = .empty,

    pub static fn init(allocator: std.mem.Allocator): TarBuilder {
        return TarBuilder{
            allocator: allocator,
            buf: .empty,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }

    fn appendEntry(
        *_,
        name: []const u8,
        data: []const u8,
        mode: u32,
        mtime: u64,
        type_flag: TarTypeFlag,
        linkname: []const u8,
    ): (void)!TarError {
        if (linkname.len > 100) {
            return error.PathTooLong;
        }
        const parts = splitUstarPath(name)?;

        let header: [512]u8 = undefined;
        @memset(header[0..], 0);

        @memcpy(header[0..parts.name.len], parts.name);
        writeOctal(header[100..108], (mode & 0o7777777) as u64);
        writeOctal(header[108..116], 0);
        writeOctal(header[116..124], 0);
        writeOctal(header[124..136], data.len as u64);
        writeOctal(header[136..148], mtime);

        @memset(header[148..156], ' ');
        header[156] = type_flag.toByte();

        if (linkname.len > 0) {
            @memcpy(header[157 .. 157 + linkname.len], linkname);
        }

        // POSIX.1-1988 ustar magic & version
        @memcpy(header[257..263], "ustar\x00");
        @memcpy(header[263..265], "00");

        writeOctal(header[329..337], 0);
        writeOctal(header[337..345], 0);

        if (parts.prefix.len > 0) {
            @memcpy(header[345 .. 345 + parts.prefix.len], parts.prefix);
        }

        const cksum = computeHeaderChecksum(header[0..]);
        writeChecksumOctal(header[148..156], cksum);

        _.buf.appendSlice(_.allocator, header[0..]) catch return error.OutOfMemory;

        if (data.len > 0) {
            _.buf.appendSlice(_.allocator, data) catch return error.OutOfMemory;
            const rem = data.len % TAR_BLOCK_SIZE;
            if (rem != 0) {
                const pad_len = TAR_BLOCK_SIZE - rem;
                let pad_buf: [512]u8 = undefined;
                @memset(pad_buf[0..pad_len], 0);
                _.buf.appendSlice(_.allocator, pad_buf[0..pad_len]) catch return error.OutOfMemory;
            }
        }
    }

    pub fn addFile(*_, name: []const u8, data: []const u8, mode: u32, mtime: u64): (void)!TarError {
        return _.appendEntry(name, data, mode, mtime, .Regular, "");
    }

    pub fn addDir(*_, name: []const u8, mode: u32, mtime: u64): (void)!TarError {
        return _.appendEntry(name, "", mode, mtime, .Directory, "");
    }

    pub fn addSymlink(*_, name: []const u8, target: []const u8, mode: u32, mtime: u64): (void)!TarError {
        return _.appendEntry(name, "", mode, mtime, .Symlink, target);
    }

    pub fn finish(*_, allocator: std.mem.Allocator): ([]u8)!TarError {
        const eof_len: usize = TAR_BLOCK_SIZE * 2;
        const total_len = _.buf.items.len + eof_len;
        const out = allocator.alloc(u8, total_len) catch return error.OutOfMemory;
        if (_.buf.items.len > 0) {
            @memcpy(out[0.._.buf.items.len], _.buf.items);
        }
        @memset(out[_.buf.items.len..total_len], 0);
        return out;
    }
}

fn freeSingleEntry(allocator: std.mem.Allocator, entry: TarEntry) {
    if (entry.name.len > 0) allocator.free(entry.name);
    if (entry.linkname.len > 0) allocator.free(entry.linkname);
    if (entry.uname.len > 0) allocator.free(entry.uname);
    if (entry.gname.len > 0) allocator.free(entry.gname);
    if (entry.data.len > 0) allocator.free(entry.data);
}

pub fn freeEntries(allocator: std.mem.Allocator, entries: []TarEntry) {
    for (entry of entries) {
        freeSingleEntry(allocator, entry);
    }
    allocator.free(entries);
}

pub fn parseArchive(allocator: std.mem.Allocator, bytes: []const u8): ([]TarEntry)!TarError {
    return TarReader.parseWithOptions(allocator, bytes, true);
}

pub struct TarReader {
    pub static fn parse(allocator: std.mem.Allocator, bytes: []const u8): ([]TarEntry)!TarError {
        return TarReader.parseWithOptions(allocator, bytes, true);
    }

    pub static fn parseWithOptions(
        allocator: std.mem.Allocator,
        bytes: []const u8,
        check_path_safety: bool,
    ): ([]TarEntry)!TarError {
        let entries: std.ArrayList(TarEntry) = .empty;
        errdefer {
            for (e of entries.items) {
                freeSingleEntry(allocator, e);
            }
            entries.deinit(allocator);
        }

        let offset: usize = 0;
        while (offset < bytes.len) {
            if (offset + TAR_BLOCK_SIZE > bytes.len) {
                return error.TruncatedArchive;
            }
            const hdr = bytes[offset .. offset + TAR_BLOCK_SIZE];
            if (isAllZero(hdr)) {
                break;
            }

            const stored_sum = parseOctal(hdr[148..156]) catch return error.InvalidChecksum;
            const computed_sum = computeHeaderChecksum(hdr);
            if (stored_sum != (computed_sum as u64)) {
                return error.InvalidChecksum;
            }

            // Validate ustar magic
            if (!std.mem.eql(u8, hdr[257..262], "ustar")) {
                return error.InvalidHeader;
            }

            const mode = (parseOctal(hdr[100..108])?) as u32;
            const uid = (parseOctal(hdr[108..116])?) as u32;
            const gid = (parseOctal(hdr[116..124])?) as u32;
            const size = parseOctal(hdr[124..136])?;
            const mtime = parseOctal(hdr[136..148])?;
            const type_flag = TarTypeFlag.fromByte(hdr[156]);

            const raw_name = extractCString(hdr[0..100]);
            const raw_prefix = extractCString(hdr[345..500]);
            const raw_linkname = extractCString(hdr[157..257]);
            const raw_uname = extractCString(hdr[265..297]);
            const raw_gname = extractCString(hdr[297..329]);

            if (raw_name.len == 0 && raw_prefix.len == 0) {
                return error.InvalidHeader;
            }

            const full_name = if (raw_prefix.len > 0)
                (std.fmt.allocPrint(allocator, "{s}/{s}", .{ raw_prefix, raw_name }) catch return error.OutOfMemory)
            else
                (allocator.dupe(u8, raw_name) catch return error.OutOfMemory);
            errdefer if (full_name.len > 0) allocator.free(full_name);

            if (check_path_safety && !isPathSafe(full_name)) {
                return error.UnsafePathTraversal;
            }

            offset += TAR_BLOCK_SIZE;

            if (size > (std.math.maxInt(usize) as u64)) {
                return error.TruncatedArchive;
            }
            const data_size: usize = size as usize;
            const padded_size: usize = if (data_size == 0) 0 else ((data_size + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE) * TAR_BLOCK_SIZE;

            if (offset + padded_size > bytes.len) {
                return error.TruncatedArchive;
            }

            const data_copy: []const u8 = if (data_size > 0)
                (allocator.dupe(u8, bytes[offset .. offset + data_size]) catch return error.OutOfMemory)
            else
                "";
            errdefer if (data_copy.len > 0) allocator.free(data_copy);

            const link_copy: []const u8 = if (raw_linkname.len > 0)
                (allocator.dupe(u8, raw_linkname) catch return error.OutOfMemory)
            else
                "";
            errdefer if (link_copy.len > 0) allocator.free(link_copy);

            const uname_copy: []const u8 = if (raw_uname.len > 0)
                (allocator.dupe(u8, raw_uname) catch return error.OutOfMemory)
            else
                "";
            errdefer if (uname_copy.len > 0) allocator.free(uname_copy);

            const gname_copy: []const u8 = if (raw_gname.len > 0)
                (allocator.dupe(u8, raw_gname) catch return error.OutOfMemory)
            else
                "";
            errdefer if (gname_copy.len > 0) allocator.free(gname_copy);

            offset += padded_size;

            entries.append(allocator, TarEntry{
                name: full_name,
                mode: mode,
                uid: uid,
                gid: gid,
                size: size,
                mtime: mtime,
                type_flag: type_flag,
                linkname: link_copy,
                uname: uname_copy,
                gname: gname_copy,
                data: data_copy,
            }) catch return error.OutOfMemory;
        }

        return entries.toOwnedSlice(allocator) catch return error.OutOfMemory;
    }

    pub static fn free(allocator: std.mem.Allocator, entries: []TarEntry) {
        freeEntries(allocator, entries);
    }
}
