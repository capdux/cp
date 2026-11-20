// Source: /data/dev/bun/src/sys/PosixStat.cp
// ============================================================================
// lib/sys/stat.cp — Cross-platform File Metadata & Status Introspection
// ============================================================================

const std = @import("std");
import { FD } from "./fd";

pub const Stat = struct {
    size: u64 = 0,
    mode: u32 = 0,
    mtime: i64 = 0,
    atime: i64 = 0,
    ctime: i64 = 0,
    ino: u64 = 0,
    dev: u64 = 0,
    nlink: u32 = 0,
    uid: u32 = 0,
    gid: u32 = 0,

    pub fn isFile(self: Stat): bool {
        return (self.mode & 0o170000) == 0o100000;
    }

    pub fn isDir(self: Stat): bool {
        return (self.mode & 0o170000) == 0o040000;
    }

    pub fn isSymLink(self: Stat): bool {
        return (self.mode & 0o170000) == 0o120000;
    }

    pub fn isSocket(self: Stat): bool {
        return (self.mode & 0o170000) == 0o140000;
    }

    pub fn isFIFO(self: Stat): bool {
        return (self.mode & 0o170000) == 0o010000;
    }

    pub fn permissions(self: Stat): u32 {
        return self.mode & 0o7777;
    }
};

fn statxToStat(stx: *const std.os.linux.Statx): Stat {
    return Stat{
        size: stx.size,
        mode: @as(u32, stx.mode),
        mtime: stx.mtime.sec,
        atime: stx.atime.sec,
        ctime: stx.ctime.sec,
        ino: stx.ino,
        dev: (@as(u64, stx.dev_major) << 32) | @as(u64, stx.dev_minor),
        nlink: stx.nlink,
        uid: stx.uid,
        gid: stx.gid,
    };
}

fn defaultStatxMask(): std.os.linux.STATX {
    return std.os.linux.STATX{
        .TYPE = true,
        .MODE = true,
        .NLINK = true,
        .UID = true,
        .GID = true,
        .ATIME = true,
        .MTIME = true,
        .CTIME = true,
        .INO = true,
        .SIZE = true,
    };
}

pub fn fstat(fd: FD): !Stat {
    if (!fd.isValid()) return error.BadFileDescriptor;
    let stx: std.os.linux.Statx = undefined;
    const rc = std.c.statx(fd.raw, "", std.os.linux.AT.EMPTY_PATH, defaultStatxMask(), &stx);
    if (rc != 0) return error.StatFailed;
    return statxToStat(&stx);
}

pub fn stat(path: []const u8): !Stat {
    let path_buf: [1024]u8 = undefined;
    if (path.len >= path_buf.len) return error.NameTooLong;
    @memcpy(path_buf[0 .. path.len], path);
    path_buf[path.len] = 0;
    let path_z: [*:0]const u8 = path_buf[0 .. path.len :0].ptr;

    let stx: std.os.linux.Statx = undefined;
    const rc = std.c.statx(std.os.linux.AT.FDCWD, path_z, 0, defaultStatxMask(), &stx);
    if (rc != 0) return error.StatFailed;
    return statxToStat(&stx);
}

pub fn lstat(path: []const u8): !Stat {
    let path_buf: [1024]u8 = undefined;
    if (path.len >= path_buf.len) return error.NameTooLong;
    @memcpy(path_buf[0 .. path.len], path);
    path_buf[path.len] = 0;
    let path_z: [*:0]const u8 = path_buf[0 .. path.len :0].ptr;

    let stx: std.os.linux.Statx = undefined;
    const rc = std.c.statx(std.os.linux.AT.FDCWD, path_z, std.os.linux.AT.SYMLINK_NOFOLLOW, defaultStatxMask(), &stx);
    if (rc != 0) return error.StatFailed;
    return statxToStat(&stx);
}
