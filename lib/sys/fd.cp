// Source: Absorbed from /data/dev/bun/src/sys/fd.cp | Synced: 2026-11-22T11:32:03Z
// ============================================================================
// lib/sys/fd.cp — Cross-platform File Descriptor Abstraction & Flags
// ============================================================================

const std = @import("std");
import { SysError, SyscallTag, Maybe, linuxSyscallErrno, cErrno, EINTR, EAGAIN, EBADF } from "./error";

pub const O_RDONLY: i32 = 0;
pub const O_WRONLY: i32 = 1;
pub const O_RDWR: i32 = 2;
pub const O_CREAT: i32 = 64;
pub const O_EXCL: i32 = 128;
pub const O_NOCTTY: i32 = 256;
pub const O_TRUNC: i32 = 512;
pub const O_APPEND: i32 = 1024;
pub const O_NONBLOCK: i32 = 2048;
pub const O_DSYNC: i32 = 4096;
pub const O_DIRECT: i32 = 16384;
pub const O_DIRECTORY: i32 = 65536;
pub const O_NOFOLLOW: i32 = 131072;
pub const O_CLOEXEC: i32 = 524288;
pub const O_SYNC: i32 = 1052672;
pub const O_PATH: i32 = 2097152;
pub const O_TMPFILE: i32 = 4259840;

pub const F_OK: i32 = 0;
pub const X_OK: i32 = 1;
pub const W_OK: i32 = 2;
pub const R_OK: i32 = 4;

pub const SEEK_SET: i32 = 0;
pub const SEEK_CUR: i32 = 1;
pub const SEEK_END: i32 = 2;

pub const AT_FDCWD: i32 = -100;
pub const AT_SYMLINK_NOFOLLOW: u32 = 0x100;
pub const AT_REMOVEDIR: u32 = 0x200;
pub const AT_SYMLINK_FOLLOW: u32 = 0x400;
pub const AT_EMPTY_PATH: u32 = 0x1000;

pub const LOCK_SH: i32 = 1;
pub const LOCK_EX: i32 = 2;
pub const LOCK_NB: i32 = 4;
pub const LOCK_UN: i32 = 8;

pub const F_DUPFD: i32 = 0;
pub const F_GETFD: i32 = 1;
pub const F_SETFD: i32 = 2;
pub const F_GETFL: i32 = 3;
pub const F_SETFL: i32 = 4;
pub const F_DUPFD_CLOEXEC: i32 = 1030;
pub const FD_CLOEXEC: i32 = 1;

pub struct FD {
    raw: i32 = -1,

    pub const invalid: FD = FD{ raw: -1 };
    pub const stdin: FD = FD{ raw: 0 };
    pub const stdout: FD = FD{ raw: 1 };
    pub const stderr: FD = FD{ raw: 2 };

    static fn fromNative(raw_fd: i32): FD {
        return FD{ raw: raw_fd };
    }

    static fn fromSystem(raw_fd: i32): FD {
        return FD{ raw: raw_fd };
    }

    static fn cwd(): FD {
        return FD{ raw: AT_FDCWD };
    }

    fn isValid(): bool {
        return _.raw >= 0;
    }

    fn unwrapValid(): FD? {
        if (_.isValid()) return _;
        return null;
    }

    fn isStandardStream(): bool {
        return _.raw >= 0 && _.raw <= 2;
    }

    fn native(): i32 {
        return _.raw;
    }

    fn cast(): i32 {
        return _.raw;
    }

    fn isTty(): bool {
        if (!_.isValid()) return false;
        return std.c.isatty(_.raw) != 0;
    }

    fn readMaybe(buf: []u8): Maybe<usize> {
        if (!_.isValid()) {
            return Maybe<usize>.initErr(SysError.fromCode(EBADF, .read).withFd(_.raw));
        }
        if (buf.len == 0) return Maybe<usize>.initResult(0);
        while (true) {
            const rc = std.os.linux.read(_.raw, buf.ptr, buf.len);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) return Maybe<usize>.initResult(rc);
            if (err_code == EINTR) continue;
            return Maybe<usize>.initErr(SysError.fromCode(err_code, .read).withFd(_.raw));
        }
    }

    fn read(buf: []u8): !usize {
        return _.readMaybe(buf).unwrap();
    }

    fn readAll(buf: []u8): !usize {
        if (!_.isValid()) return error.BadFileDescriptor;
        let total: usize = 0;
        while (total < buf.len) {
            const n = try _.read(buf[total..buf.len]);
            if (n == 0) break;
            total += n;
        }
        return total;
    }

    fn writeMaybe(data: []const u8): Maybe<usize> {
        if (!_.isValid()) {
            return Maybe<usize>.initErr(SysError.fromCode(EBADF, .write).withFd(_.raw));
        }
        if (data.len == 0) return Maybe<usize>.initResult(0);
        while (true) {
            const rc = std.os.linux.write(_.raw, data.ptr, data.len);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) return Maybe<usize>.initResult(rc);
            if (err_code == EINTR) continue;
            return Maybe<usize>.initErr(SysError.fromCode(err_code, .write).withFd(_.raw));
        }
    }

    fn write(data: []const u8): !usize {
        return _.writeMaybe(data).unwrap();
    }

    fn writeAll(data: []const u8): !void {
        if (!_.isValid()) return error.BadFileDescriptor;
        let remaining = data;
        while (remaining.len > 0) {
            const written = try _.write(remaining);
            if (written == 0) return error.WriteZero;
            remaining = remaining[written..];
        }
    }

    fn preadMaybe(buf: []u8, offset: u64): Maybe<usize> {
        if (!_.isValid()) {
            return Maybe<usize>.initErr(SysError.fromCode(EBADF, .pread).withFd(_.raw));
        }
        if (buf.len == 0) return Maybe<usize>.initResult(0);
        while (true) {
            const rc = std.c.pread(_.raw, buf.ptr, buf.len, @intCast(offset));
            if (rc >= 0) return Maybe<usize>.initResult(@intCast(rc));
            const err_code = cErrno();
            if (err_code == EINTR) continue;
            return Maybe<usize>.initErr(SysError.fromCode(err_code, .pread).withFd(_.raw));
        }
    }

    fn pread(buf: []u8, offset: u64): !usize {
        return _.preadMaybe(buf, offset).unwrap();
    }

    fn pwriteMaybe(data: []const u8, offset: u64): Maybe<usize> {
        if (!_.isValid()) {
            return Maybe<usize>.initErr(SysError.fromCode(EBADF, .pwrite).withFd(_.raw));
        }
        if (data.len == 0) return Maybe<usize>.initResult(0);
        while (true) {
            const rc = std.c.pwrite(_.raw, data.ptr, data.len, @intCast(offset));
            if (rc >= 0) return Maybe<usize>.initResult(@intCast(rc));
            const err_code = cErrno();
            if (err_code == EINTR) continue;
            return Maybe<usize>.initErr(SysError.fromCode(err_code, .pwrite).withFd(_.raw));
        }
    }

    fn pwrite(data: []const u8, offset: u64): !usize {
        return _.pwriteMaybe(data, offset).unwrap();
    }

    fn lseek(offset: i64, whence: i32): !u64 {
        if (!_.isValid()) return error.BadFileDescriptor;
        const rc = std.c.lseek(_.raw, @intCast(offset), @intCast(whence));
        if (rc < 0) return SysError.fromCode(cErrno(), .lseek).withFd(_.raw).toError();
        return @intCast(rc);
    }

    fn seekTo(offset: i64): !u64 {
        return _.lseek(offset, SEEK_SET);
    }

    fn seekBy(offset: i64): !u64 {
        return _.lseek(offset, SEEK_CUR);
    }

    fn seekFromEnd(offset: i64): !u64 {
        return _.lseek(offset, SEEK_END);
    }

    fn ftruncate(length: u64): !void {
        if (!_.isValid()) return error.BadFileDescriptor;
        while (true) {
            const rc = std.c.ftruncate(_.raw, @intCast(length));
            if (rc == 0) return;
            const err_code = cErrno();
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .ftruncate).withFd(_.raw).toError();
        }
    }

    fn dup(): !FD {
        if (!_.isValid()) return error.BadFileDescriptor;
        const new_raw = std.c.dup(_.raw);
        if (new_raw < 0) return SysError.fromCode(cErrno(), .dup).withFd(_.raw).toError();
        return FD.fromNative(new_raw);
    }

    fn dupCloexec(): !FD {
        if (!_.isValid()) return error.BadFileDescriptor;
        const new_raw = try _.fcntl(F_DUPFD_CLOEXEC, 0);
        return FD.fromNative(@intCast(new_raw));
    }

    fn dup2(new_fd: FD): !FD {
        if (!_.isValid() || new_fd.raw < 0) return error.BadFileDescriptor;
        while (true) {
            const rc = std.c.dup2(_.raw, new_fd.raw);
            if (rc >= 0) return FD.fromNative(rc);
            const err_code = cErrno();
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .dup2).withFd(_.raw).toError();
        }
    }

    fn fcntl(cmd: i32, arg: usize): !usize {
        if (!_.isValid() && _.raw != AT_FDCWD) return error.BadFileDescriptor;
        while (true) {
            const rc = std.os.linux.fcntl(_.raw, cmd, arg);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) return rc;
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .fcntl).withFd(_.raw).toError();
        }
    }

    fn getFlags(): !i32 {
        const flags = try _.fcntl(F_GETFL, 0);
        return @intCast(flags);
    }

    fn isNonblocking(): !bool {
        const flags = try _.getFlags();
        return (flags & O_NONBLOCK) != 0;
    }

    fn setNonblocking(nonblocking: bool): !void {
        const cur = try _.getFlags();
        const next: i32 = if (nonblocking) (cur | O_NONBLOCK) else (cur & (~O_NONBLOCK));
        if (next != cur) {
            * = try _.fcntl(F_SETFL, @intCast(next));
        }
    }

    fn setCloseOnExec(cloexec: bool): !void {
        const cur: i32 = @intCast(try _.fcntl(F_GETFD, 0));
        const next: i32 = if (cloexec) (cur | FD_CLOEXEC) else (cur & (~FD_CLOEXEC));
        if (next != cur) {
            * = try _.fcntl(F_SETFD, @intCast(next));
        }
    }

    fn flock(operation: i32): !void {
        if (!_.isValid()) return error.BadFileDescriptor;
        while (true) {
            const rc = std.os.linux.flock(_.raw, operation);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) return;
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .flock).withFd(_.raw).toError();
        }
    }

    fn fchmod(mode: u32): !void {
        if (!_.isValid()) return error.BadFileDescriptor;
        while (true) {
            const rc = std.c.fchmod(_.raw, @as(std.c.mode_t, @intCast(mode)));
            if (rc == 0) return;
            const err_code = cErrno();
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .fchmod).withFd(_.raw).toError();
        }
    }

    fn fchown(uid: u32, gid: u32): !void {
        if (!_.isValid()) return error.BadFileDescriptor;
        while (true) {
            const rc = std.c.fchown(_.raw, @intCast(uid), @intCast(gid));
            if (rc == 0) return;
            const err_code = cErrno();
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .fchown).withFd(_.raw).toError();
        }
    }

    fn sync(): !void {
        if (!_.isValid()) return error.BadFileDescriptor;
        while (true) {
            const rc = std.c.fsync(_.raw);
            if (rc == 0) return;
            const err_code = cErrno();
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .fsync).withFd(_.raw).toError();
        }
    }

    fn fdatasync(): !void {
        if (!_.isValid()) return error.BadFileDescriptor;
        while (true) {
            const rc = std.os.linux.fdatasync(_.raw);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) return;
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .fdatasync).withFd(_.raw).toError();
        }
    }

    fn getFdPath(buf: []u8): ![]const u8 {
        if (!_.isValid()) return error.BadFileDescriptor;
        if (buf.len == 0) return error.BufferTooSmall;
        let proc_buf: [64]u8 = undefined;
        const proc_z = std.fmt.bufPrintZ(&proc_buf, "/proc/self/fd/{d}", .{_.raw}) catch return error.NameTooLong;
        while (true) {
            const rc = std.os.linux.readlinkat(AT_FDCWD, proc_z.ptr, buf.ptr, buf.len);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) {
                return buf[0..rc];
            }
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .readlinkat).withFd(_.raw).toError();
        }
    }

    fn closeAllowingBadFileDescriptor(): SysError? {
        if (_.raw < 0 || _.isStandardStream()) return null;
        const rc = std.os.linux.close(_.raw);
        const err_code = linuxSyscallErrno(rc);
        if (err_code != 0) {
            return SysError.fromCode(err_code, .close).withFd(_.raw);
        }
        return null;
    }

    fn close() {
        if (_.isValid() && !_.isStandardStream()) {
            * = std.c.close(_.raw);
        }
    }
}

pub fn flagsToLinuxO(flags: i32): std.os.linux.O {
    let z_flags = std.os.linux.O{
        .CREAT = (flags & O_CREAT) != 0,
        .EXCL = (flags & O_EXCL) != 0,
        .TRUNC = (flags & O_TRUNC) != 0,
        .APPEND = (flags & O_APPEND) != 0,
        .NONBLOCK = (flags & O_NONBLOCK) != 0,
        .CLOEXEC = (flags & O_CLOEXEC) != 0,
        .DIRECTORY = (flags & O_DIRECTORY) != 0,
        .NOFOLLOW = (flags & O_NOFOLLOW) != 0,
    };
    if ((flags & O_RDWR) != 0) {
        z_flags.ACCMODE = .RDWR;
    } else if ((flags & O_WRONLY) != 0) {
        z_flags.ACCMODE = .WRONLY;
    }
    return z_flags;
}
