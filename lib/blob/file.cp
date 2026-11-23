// Source: Absorbed from /data/dev/bun/src/runtime/webcore/Blob.cp | Synced: 2026-11-23T00:23:00Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/webcore/Blob.cp
// Upstream Sync: Refer to bun/src/runtime/webcore/Blob.zig for future upstream updates.
// ============================================================================

const std = @import("std");
import { Blob } from "./blob";

struct DiskMeta {
    size: usize = 0,
    mtime_ms: i64 = 0,
}

fn statPathC(path: string): DiskMeta? {
    if (path.len == 0 || path.len >= 4096) {
        return null;
    }
    let path_buf: [4096]u8 = undefined;
    zig {
        @memcpy(path_buf[0..path.len], path);
        path_buf[path.len] = 0;
    }
    let path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;
    let stx: std.os.linux.Statx = undefined;
    const mask = std.os.linux.STATX{ .SIZE = true, .MTIME = true };
    const rc = std.c.statx(std.os.linux.AT.FDCWD, path_z, 0, mask, &stx);
    if (rc != 0) {
        return null;
    }
    const mtime_ms = (stx.mtime.sec as i64) * 1000 + (((stx.mtime.nsec as u64) / 1000000) as i64);
    return DiskMeta{
        size: stx.size as usize,
        mtime_ms: mtime_ms,
    };
}

fn writePathC(path: string, data: []const u8): bool {
    if (path.len == 0 || path.len >= 4096) {
        return false;
    }
    let path_buf: [4096]u8 = undefined;
    zig {
        @memcpy(path_buf[0..path.len], path);
        path_buf[path.len] = 0;
    }
    let path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;

    let z_flags = std.c.O{
        .CREAT = true,
        .TRUNC = true,
        .CLOEXEC = true,
    };
    z_flags.ACCMODE = .WRONLY;

    let fd = std.c.open(path_z, z_flags, @as(std.c.mode_t, 0o644));
    if (fd < 0) {
        return false;
    }

    let written: usize = 0;
    let ok = true;
    while (written < data.len) {
        let rc = std.c.write(fd, data[written..data.len].ptr, data.len - written);
        if (rc <= 0) {
            ok = false;
            break;
        }
        written += (rc as usize);
    }
    * = std.c.close(fd);
    return ok;
}

fn accessPathC(path: string): bool {
    if (path.len == 0 || path.len >= 4096) {
        return false;
    }
    let path_buf: [4096]u8 = undefined;
    zig {
        @memcpy(path_buf[0..path.len], path);
        path_buf[path.len] = 0;
    }
    let path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;
    return std.c.access(path_z, 0) == 0;
}

fn unlinkPathC(path: string): bool {
    if (path.len == 0 || path.len >= 4096) {
        return false;
    }
    let path_buf: [4096]u8 = undefined;
    zig {
        @memcpy(path_buf[0..path.len], path);
        path_buf[path.len] = 0;
    }
    let path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;
    return std.c.unlink(path_z) == 0;
}

pub struct File {
    pub blob: Blob,
    pub name: string,
    pub path: string = "",
    pub lastModified: i64,
    pub size: usize,
    pub type: string,

    pub static fn init(data: []const u8, name: string, mime_type: string, last_modified: i64): File {
        let b = Blob.fromBytes(data, mime_type);
        return File{
            blob: b,
            name: name,
            path: "",
            lastModified: last_modified,
            size: b.size,
            type: b.type,
        };
    }

    pub static fn fromPath(path: string, mime_type: string): File? {
        if (b := Blob.fromFile(path, mime_type)) {
            let mtime: i64 = 0;
            if (fmeta := statPathC(path)) {
                mtime = fmeta.mtime_ms;
            }
            let filename = File.extractFilename(path);
            return File{
                blob: b,
                name: filename,
                path: path,
                lastModified: mtime,
                size: b.size,
                type: b.type,
            };
        }
        return null;
    }

    /// Creates a disk-backed File reference at `path` (populates metadata via `statx` if the file exists).
    pub static fn at(path: string, mime_type: string = ""): File {
        let resolved_mime = if (mime_type.len > 0) mime_type else Blob.guessMime(path);
        let filename = File.extractFilename(path);
        if (fmeta := statPathC(path)) {
            if (b := Blob.fromFile(path, resolved_mime)) {
                return File{
                    blob: b,
                    name: filename,
                    path: path,
                    lastModified: fmeta.mtime_ms,
                    size: b.size,
                    type: b.type,
                };
            }
        }
        return File{
            blob: Blob.fromBytes("", resolved_mime),
            name: filename,
            path: path,
            lastModified: 0,
            size: 0,
            type: resolved_mime,
        };
    }

    pub fn exists(): bool {
        if (_.path.len > 0) {
            return accessPathC(_.path);
        }
        return _.size > 0;
    }

    pub fn bytes(): []const u8 {
        return _.blob.bytes();
    }

    pub fn text(): string {
        return _.blob.text();
    }

    pub fn slice(start: usize, end: usize, content_type: string): File {
        let sliced_blob = _.blob.slice(start, end, content_type);
        return File{
            blob: sliced_blob,
            name: _.name,
            path: _.path,
            lastModified: _.lastModified,
            size: sliced_blob.size,
            type: sliced_blob.type,
        };
    }

    pub fn sliceSigned(start: isize, end: isize, content_type: string): File {
        let sliced_blob = _.blob.sliceSigned(start, end, content_type);
        return File{
            blob: sliced_blob,
            name: _.name,
            path: _.path,
            lastModified: _.lastModified,
            size: sliced_blob.size,
            type: sliced_blob.type,
        };
    }

    /// Writes `data` to the File's associated `path` via POSIX C I/O and updates in-memory metadata.
    pub fn write(*_, data: []const u8): bool {
        if (_.path.len == 0) {
            return false;
        }
        if (!writePathC(_.path, data)) {
            return false;
        }
        _.blob.~init();
        _.blob = Blob.fromBytes(data, _.type);
        _.size = data.len;
        if (fmeta := statPathC(_.path)) {
            _.lastModified = fmeta.mtime_ms;
        }
        return true;
    }

    /// Deletes the file on disk associated with `_.path`.
    pub fn delete(*_): bool {
        if (_.path.len == 0) {
            return false;
        }
        return unlinkPathC(_.path);
    }

    pub fn save(dest_path: string): bool {
        let raw = _.bytes();
        return writePathC(dest_path, raw);
    }

    pub fn writeTo(dest_path: string): bool {
        return _.save(dest_path);
    }

    ~init() {
        _.blob.~init();
    }

    pub fn deinit(*_) {
        _.~init();
    }

    static fn extractFilename(path: string): string {
        if (path.len == 0) {
            return "";
        }
        let mut_idx = path.len;
        while (mut_idx > 0) {
            mut_idx = mut_idx - 1;
            let ch = path[mut_idx];
            if (ch == '/' || ch == '\\') {
                return path[(mut_idx + 1)..];
            }
        }
        return path;
    }
}
