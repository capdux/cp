// Source: bun/src/sys/PosixStat.cp
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
};

pub fn fstat(fd: FD): !Stat {
    if (!fd.isValid()) return error.BadFileDescriptor;
    let st = try std.posix.fstat(fd.raw);

    return Stat{
        size: u64(@intCast(st.size)),
        mode: u32(@intCast(st.mode)),
        mtime: i64(@intCast(st.mtime().sec)),
        atime: i64(@intCast(st.atime().sec)),
        ctime: i64(@intCast(st.ctime().sec)),
        ino: u64(@intCast(st.ino)),
        dev: u64(@intCast(st.dev)),
        nlink: u32(@intCast(st.nlink)),
        uid: u32(@intCast(st.uid)),
        gid: u32(@intCast(st.gid)),
    };
}

pub fn stat(path: []const u8): !Stat {
    let path_buf: [1024]u8 = undefined;
    if (path.len >= path_buf.len) return error.NameTooLong;
    @memcpy(path_buf[0 .. path.len], path);
    path_buf[path.len] = 0;
    let path_z: [:0]const u8 = path_buf[0 .. path.len :0];

    let raw_fd = try std.posix.open(path_z, .{ .ACCMODE = .RDONLY, .CLOEXEC = true }, 0);
    let fd = FD.fromNative(raw_fd);
    defer fd.close();

    return fstat(fd);
}
