// Source: /data/dev/bun/src/sys/PosixStat.cp & /data/dev/bun/src/sys/sys.cp
// ============================================================================
// lib/sys/stat.cp — Path-based File Metadata, statx & Status Introspection
// ============================================================================

const std = @import("std");
import { FD, AT_FDCWD, AT_SYMLINK_NOFOLLOW } from "./fd";
import { Maybe } from "./error";
import { Stat, defaultStatxMask, maskFromU32, statxRaw } from "./fstat";

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
