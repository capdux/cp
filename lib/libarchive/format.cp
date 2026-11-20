// Source: cp/lib/libarchive/format.cp (pure CP archive format detection & safety)

const std = @import("std");

pub enum ArchiveFormat {
    Ar,
    CpioNewc,
    CpioCrc,
    Tar,
    Zip,
    Gzip,
    Zstd,
    Unknown,
}

pub error ArchiveError {
    TruncatedHeader,
    InvalidMagic,
    InvalidHeader,
    InvalidChecksum,
    PathTraversal,
    TooManyEntries,
    SizeLimitExceeded,
    UnsupportedFormat,
    OutOfMemory,
}

pub enum ArchiveEntryKind {
    File,
    Directory,
    Symlink,
    Other,
}

pub struct ArchiveEntry {
    name: []const u8,
    data: []const u8,
    mode: u32 = 0o100644,
    uid: u32 = 0,
    gid: u32 = 0,
    mtime: u64 = 0,
    kind: ArchiveEntryKind = .File,
    link_name: []const u8 = "",
}

pub struct ArchiveLimits {
    max_entries: usize = 100000,
    max_total_bytes: usize = 512 * 1024 * 1024,
    max_entry_bytes: usize = 256 * 1024 * 1024,
    allow_absolute_paths: bool = false,
}

pub fn detectArchiveFormat(data: []const u8): ArchiveFormat {
    if (data.len >= 8 && std.mem.eql(u8, data[0..8], "!<arch>\n")) {
        return .Ar;
    }
    if (data.len >= 6 && std.mem.eql(u8, data[0..6], "070701")) {
        return .CpioNewc;
    }
    if (data.len >= 6 && std.mem.eql(u8, data[0..6], "070702")) {
        return .CpioCrc;
    }
    if (data.len >= 4 && data[0] == 'P' && data[1] == 'K' &&
        ((data[2] == 0x03 && data[3] == 0x04) ||
         (data[2] == 0x05 && data[3] == 0x06) ||
         (data[2] == 0x01 && data[3] == 0x02)))
    {
        return .Zip;
    }
    if (data.len >= 2 && data[0] == 0x1f && data[1] == 0x8b) {
        return .Gzip;
    }
    if (data.len >= 4 && data[0] == 0x28 && data[1] == 0xb5 && data[2] == 0x2f && data[3] == 0xfd) {
        return .Zstd;
    }
    if (data.len >= 263 && std.mem.eql(u8, data[257..262], "ustar")) {
        return .Tar;
    }
    return .Unknown;
}

pub fn validateArchivePath(path: []const u8, allow_abs: bool): ArchiveError!void {
    if (path.len == 0) {
        return error.PathTraversal;
    }
    if (!allow_abs && (path[0] == '/' || path[0] == '\\')) {
        return error.PathTraversal;
    }
    let i: usize = 0;
    let seg_start: usize = 0;
    while (i <= path.len) {
        if (i < path.len && path[i] == 0) {
            return error.PathTraversal;
        }
        const at_end = (i == path.len);
        const is_sep = (!at_end && (path[i] == '/' || path[i] == '\\'));
        if (at_end || is_sep) {
            const seg = path[seg_start..i];
            if (std.mem.eql(u8, seg, "..")) {
                return error.PathTraversal;
            }
            seg_start = i + 1;
        }
        i += 1;
    }
}

pub fn freeEntries(alloc: std.mem.Allocator, entries: []ArchiveEntry) {
    for (entry of entries) {
        if (entry.name.len > 0) {
            alloc.free(entry.name);
        }
        if (entry.data.len > 0) {
            alloc.free(entry.data);
        }
        if (entry.link_name.len > 0) {
            alloc.free(entry.link_name);
        }
    }
    alloc.free(entries);
}
