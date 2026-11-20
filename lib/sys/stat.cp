// Source: /data/dev/bun/src/sys/PosixStat.cp & /data/dev/bun/src/sys/sys.cp
// ============================================================================
// lib/sys/stat.cp — Cross-platform File Metadata, statx & Status Introspection
// ============================================================================

const std = @import("std");
import { FD, AT_FDCWD, AT_SYMLINK_NOFOLLOW, AT_EMPTY_PATH } from "./fd";
import { SysError, SyscallTag, Maybe, linuxSyscallErrno, cErrno, EINTR } from "./error";

pub enum FileKind: u8 {
    file,
    directory,
    sym_link,
    block_device,
    character_device,
    named_pipe,
    unix_domain_socket,
    unknown,
}

pub enum StatxField: u32 {
    type_field = 0x00000001,
    mode = 0x00000002,
    nlink = 0x00000004,
    uid = 0x00000008,
    gid = 0x00000010,
    atime = 0x00000020,
    mtime = 0x00000040,
    ctime = 0x00000080,
    ino = 0x00000100,
    size = 0x00000200,
    blocks = 0x00000400,
    btime = 0x00000800,
}

pub fn kindFromMode(mode: u32): FileKind {
    const m = mode & 0o170000;
    if (m == 0o100000) return .file;
    if (m == 0o040000) return .directory;
    if (m == 0o120000) return .sym_link;
    if (m == 0o060000) return .block_device;
    if (m == 0o020000) return .character_device;
    if (m == 0o010000) return .named_pipe;
    if (m == 0o140000) return .unix_domain_socket;
    return .unknown;
}

pub struct Stat {
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
    rdev: u64 = 0,
    blksize: u32 = 0,
    blocks: u64 = 0,
    mtime_ns: i64 = 0,
    atime_ns: i64 = 0,
    ctime_ns: i64 = 0,
    btime: i64 = 0,
    btime_ns: i64 = 0,

    fn isFile(): bool {
        return (_.mode & 0o170000) == 0o100000;
    }

    fn isDir(): bool {
        return (_.mode & 0o170000) == 0o040000;
    }

    fn isSymLink(): bool {
        return (_.mode & 0o170000) == 0o120000;
    }

    fn isSymlink(): bool {
        return _.isSymLink();
    }

    fn isSocket(): bool {
        return (_.mode & 0o170000) == 0o140000;
    }

    fn isFIFO(): bool {
        return (_.mode & 0o170000) == 0o010000;
    }

    fn isBlockDevice(): bool {
        return (_.mode & 0o170000) == 0o060000;
    }

    fn isCharDevice(): bool {
        return (_.mode & 0o170000) == 0o020000;
    }

    fn kind(): FileKind {
        return kindFromMode(_.mode);
    }

    fn permissions(): u32 {
        return _.mode & 0o7777;
    }
}

pub type PosixStat = Stat;

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
        rdev: (@as(u64, stx.rdev_major) << 32) | @as(u64, stx.rdev_minor),
        blksize: stx.blksize,
        blocks: stx.blocks,
        mtime_ns: @as(i64, stx.mtime.nsec),
        atime_ns: @as(i64, stx.atime.nsec),
        ctime_ns: @as(i64, stx.ctime.nsec),
        btime: stx.btime.sec,
        btime_ns: @as(i64, stx.btime.nsec),
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
        .BLOCKS = true,
        .BTIME = true,
    };
}

fn maskFromU32(mask_bits: u32): std.os.linux.STATX {
    return std.os.linux.STATX{
        .TYPE = (mask_bits & 0x001) != 0,
        .MODE = (mask_bits & 0x002) != 0,
        .NLINK = (mask_bits & 0x004) != 0,
        .UID = (mask_bits & 0x008) != 0,
        .GID = (mask_bits & 0x010) != 0,
        .ATIME = (mask_bits & 0x020) != 0,
        .MTIME = (mask_bits & 0x040) != 0,
        .CTIME = (mask_bits & 0x080) != 0,
        .INO = (mask_bits & 0x100) != 0,
        .SIZE = (mask_bits & 0x200) != 0,
        .BLOCKS = (mask_bits & 0x400) != 0,
        .BTIME = (mask_bits & 0x800) != 0,
    };
}

fn statxRaw(dir_fd: i32, path_str: []const u8, flags: u32, mask: std.os.linux.STATX, tag: SyscallTag): Maybe<Stat> {
    let path_buf: [4096]u8 = undefined;
    if (path_str.len >= path_buf.len) {
        return Maybe<Stat>.initErr(SysError.fromCode(36, tag).withPath(path_str));
    }
    if (path_str.len > 0) {
        @memcpy(path_buf[0 .. path_str.len], path_str);
    }
    path_buf[path_str.len] = 0;
    const path_z: [*:0]const u8 = path_buf[0 .. path_str.len :0].ptr;
    let stx: std.os.linux.Statx = undefined;
    while (true) {
        const rc = std.os.linux.statx(dir_fd, path_z, flags, mask, &stx);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) {
            return Maybe<Stat>.initResult(statxToStat(&stx));
        }
        if (err_code == EINTR) continue;
        let se = SysError.fromCode(err_code, tag).withFd(dir_fd);
        if (path_str.len > 0) {
            se = se.withPath(path_str);
        }
        return Maybe<Stat>.initErr(se);
    }
}

pub fn fstatMaybe(fd: FD): Maybe<Stat> {
    if (!fd.isValid()) {
        return Maybe<Stat>.initErr(SysError.fromCode(9, .fstat).withFd(fd.raw));
    }
    return statxRaw(fd.raw, "", AT_EMPTY_PATH, defaultStatxMask(), .fstat);
}

pub fn fstat(fd: FD): !Stat {
    return fstatMaybe(fd).unwrap();
}

pub fn statMaybe(path: []const u8): Maybe<Stat> {
    return statxRaw(AT_FDCWD, path, 0, defaultStatxMask(), .stat);
}

pub fn stat(path: []const u8): !Stat {
    return statMaybe(path).unwrap();
}

pub fn lstatMaybe(path: []const u8): Maybe<Stat> {
    return statxRaw(AT_FDCWD, path, AT_SYMLINK_NOFOLLOW, defaultStatxMask(), .lstat);
}

pub fn lstat(path: []const u8): !Stat {
    return lstatMaybe(path).unwrap();
}

pub fn fstatatMaybe(dir_fd: FD, path: []const u8): Maybe<Stat> {
    const fd_raw = if (dir_fd.isValid()) dir_fd.raw else AT_FDCWD;
    return statxRaw(fd_raw, path, 0, defaultStatxMask(), .fstatat);
}

pub fn fstatat(dir_fd: FD, path: []const u8): !Stat {
    return fstatatMaybe(dir_fd, path).unwrap();
}

pub fn lstatatMaybe(dir_fd: FD, path: []const u8): Maybe<Stat> {
    const fd_raw = if (dir_fd.isValid()) dir_fd.raw else AT_FDCWD;
    return statxRaw(fd_raw, path, AT_SYMLINK_NOFOLLOW, defaultStatxMask(), .lstatat);
}

pub fn lstatat(dir_fd: FD, path: []const u8): !Stat {
    return lstatatMaybe(dir_fd, path).unwrap();
}

pub fn statx(dir_fd: FD, path: []const u8, flags: u32, mask_bits: u32): Maybe<Stat> {
    const fd_raw = if (dir_fd.isValid()) dir_fd.raw else AT_FDCWD;
    return statxRaw(fd_raw, path, flags, maskFromU32(mask_bits), .statx);
}
