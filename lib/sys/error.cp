// Source: Absorbed from /data/dev/bun/src/sys/Error.cp | Synced: 2026-11-22T11:12:46Z
// ============================================================================
// lib/sys/error.cp — Structured Syscall Errors, SyscallTag & Maybe<T> Result
// ============================================================================

const std = @import("std");
import { SystemErrno } from "../errno/index.cp";

pub enum SyscallTag: u8 {
    TODO,
    access,
    chdir,
    chmod,
    chown,
    clonefile,
    clonefileat,
    close,
    connect,
    copy_file_range,
    copyfile,
    dup,
    dup2,
    dup3,
    epoll_create1,
    epoll_ctl,
    epoll_wait,
    faccessat,
    fallocate,
    fchdir,
    fchmod,
    fchmodat,
    fchown,
    fcntl,
    fcopyfile,
    fdatasync,
    flock,
    fork,
    fstat,
    fstatat,
    fsync,
    ftruncate,
    futimens,
    getdents64,
    getcwd,
    getenv,
    ioctl,
    kill,
    lchmod,
    lchown,
    link,
    linkat,
    lseek,
    lstat,
    lstatat,
    memfd_create,
    mkdir,
    mkdirat,
    mkdtemp,
    mmap,
    munmap,
    open,
    openat,
    pidfd_open,
    pipe,
    pipe2,
    poll,
    posix_spawn,
    ppoll,
    pread,
    preadv,
    pwrite,
    pwritev,
    read,
    readlink,
    readlinkat,
    readv,
    recv,
    rename,
    renameat,
    renameat2,
    rmdir,
    rmdirat,
    send,
    sendfile,
    socket,
    socketpair,
    spawn,
    stat,
    statfs,
    statx,
    symlink,
    symlinkat,
    umask,
    uname,
    unlink,
    unlinkat,
    utime,
    utimensat,
    waitpid,
    write,
    writev,

    fn tagName(): []const u8 {
        return @tagName(_);
    }
}

pub const EPERM: i32 = 1;
pub const ENOENT: i32 = 2;
pub const ESRCH: i32 = 3;
pub const EINTR: i32 = 4;
pub const EIO: i32 = 5;
pub const ENXIO: i32 = 6;
pub const E2BIG: i32 = 7;
pub const ENOEXEC: i32 = 8;
pub const EBADF: i32 = 9;
pub const ECHILD: i32 = 10;
pub const EAGAIN: i32 = 11;
pub const ENOMEM: i32 = 12;
pub const EACCES: i32 = 13;
pub const EFAULT: i32 = 14;
pub const EBUSY: i32 = 16;
pub const EEXIST: i32 = 17;
pub const EXDEV: i32 = 18;
pub const ENODEV: i32 = 19;
pub const ENOTDIR: i32 = 20;
pub const EISDIR: i32 = 21;
pub const EINVAL: i32 = 22;
pub const ENFILE: i32 = 23;
pub const EMFILE: i32 = 24;
pub const ENOTTY: i32 = 25;
pub const ETXTBSY: i32 = 26;
pub const EFBIG: i32 = 27;
pub const ENOSPC: i32 = 28;
pub const ESPIPE: i32 = 29;
pub const EROFS: i32 = 30;
pub const EMLINK: i32 = 31;
pub const EPIPE: i32 = 32;
pub const ERANGE: i32 = 34;
pub const ENAMETOOLONG: i32 = 36;
pub const ENOSYS: i32 = 38;
pub const ENOTEMPTY: i32 = 39;
pub const ELOOP: i32 = 40;
pub const EWOULDBLOCK: i32 = 11;
pub const EOPNOTSUPP: i32 = 95;
pub const ECONNRESET: i32 = 104;
pub const ECONNREFUSED: i32 = 111;
pub const ETIMEDOUT: i32 = 110;
pub const ECANCELED: i32 = 125;

/// Extracts a positive POSIX errno from a raw Linux syscall return (`-4095..-1`), or `0` on success.
pub fn linuxSyscallErrno(rc: usize): i32 {
    const signed_rc: isize = @bitCast(rc);
    if (signed_rc < 0 && signed_rc > -4096) {
        return @intCast(-signed_rc);
    }
    return 0;
}

/// Extracts current thread-local libc errno.
pub fn cErrno(): i32 {
    return @intCast(std.c._errno().*);
}

pub fn errnoToError(code: u16): anyerror {
    return switch (code) {
        1 => error.PermissionDenied,     // EPERM
        2 => error.FileNotFound,         // ENOENT
        3 => error.ProcessNotFound,      // ESRCH
        4 => error.Interrupted,          // EINTR
        5 => error.InputOutput,          // EIO
        6 => error.NoDevice,             // ENXIO
        7 => error.TooBig,               // E2BIG
        8 => error.InvalidExe,           // ENOEXEC
        9 => error.BadFileDescriptor,    // EBADF
        10 => error.NoChildProcesses,    // ECHILD
        11 => error.WouldBlock,          // EAGAIN / EWOULDBLOCK
        12 => error.OutOfMemory,         // ENOMEM
        13 => error.AccessDenied,        // EACCES
        14 => error.BadAddress,          // EFAULT
        16 => error.DeviceBusy,          // EBUSY
        17 => error.PathAlreadyExists,   // EEXIST
        18 => error.RenameAcrossMountPoints, // EXDEV
        19 => error.NoDevice,            // ENODEV
        20 => error.NotDir,              // ENOTDIR
        21 => error.IsDir,               // EISDIR
        22 => error.InvalidArgument,     // EINVAL
        23 => error.SystemFdQuotaExceeded, // ENFILE
        24 => error.ProcessFdQuotaExceeded, // EMFILE
        25 => error.NotATerminal,        // ENOTTY
        26 => error.FileBusy,            // ETXTBSY
        27 => error.FileTooBig,          // EFBIG
        28 => error.NoSpaceLeft,         // ENOSPC
        29 => error.Unseekable,          // ESPIPE
        30 => error.ReadOnlyFileSystem,  // EROFS
        31 => error.LinkQuotaExceeded,   // EMLINK
        32 => error.BrokenPipe,          // EPIPE
        34 => error.ResultTooLarge,      // ERANGE
        36 => error.NameTooLong,         // ENAMETOOLONG
        38 => error.FunctionNotImplemented, // ENOSYS
        39 => error.DirNotEmpty,         // ENOTEMPTY
        40 => error.SymLinkLoop,         // ELOOP
        95 => error.OperationNotSupported, // EOPNOTSUPP
        104 => error.ConnectionResetByPeer, // ECONNRESET
        110 => error.ConnectionTimedOut, // ETIMEDOUT
        111 => error.ConnectionRefused,  // ECONNREFUSED
        125 => error.Canceled,           // ECANCELED
        else => error.Unexpected,
    };
}

pub struct SysError {
    errno: u16 = 0,
    syscall: SyscallTag = .TODO,
    path: []const u8 = "",
    dest: []const u8 = "",
    fd: i32 = -1,

    pub const retry: SysError = SysError{
        errno: 11,
        syscall: .read,
    };

    pub const oom: SysError = SysError{
        errno: 12,
        syscall: .read,
    };

    pub static fn fromCode(code: i32, syscall_tag: SyscallTag): SysError {
        const abs_code: i32 = if (code < 0) (-code) else code;
        const clamped: u16 = if (abs_code > 65535) 65535 else @intCast(abs_code);
        return SysError{
            errno: clamped,
            syscall: syscall_tag,
        };
    }

    pub static fn fromErrno(e: SystemErrno, syscall_tag: SyscallTag): SysError {
        return SysError{
            errno: @intFromEnum(e),
            syscall: syscall_tag,
        };
    }

    pub static fn fromLinuxReturn(rc: usize, syscall_tag: SyscallTag): SysError? {
        const code = linuxSyscallErrno(rc);
        if (code == 0) return null;
        return SysError.fromCode(code, syscall_tag);
    }

    pub static fn fromCReturn(rc: isize, syscall_tag: SyscallTag): SysError? {
        if (rc >= 0) return null;
        return SysError.fromCode(cErrno(), syscall_tag);
    }

    fn getErrno(): SystemErrno {
        return SystemErrno.fromCode(_.errno as i32);
    }

    fn codeName(): []const u8 {
        return _.getErrno().codeName();
    }

    fn name(): []const u8 {
        return _.codeName();
    }

    fn description(): []const u8 {
        return _.getErrno().description();
    }

    fn msg(): []const u8 {
        return _.description();
    }

    fn isRetry(): bool {
        return _.errno == 11 || _.errno == 4 || _.errno == 41;
    }

    fn isNotFound(): bool {
        return _.errno == 2;
    }

    fn isAlreadyExists(): bool {
        return _.errno == 17;
    }

    fn isPermissionDenied(): bool {
        return _.errno == 1 || _.errno == 13 || _.errno == 30;
    }

    fn withFd(fd_raw: i32): SysError {
        return SysError{
            errno: _.errno,
            syscall: _.syscall,
            path: _.path,
            dest: _.dest,
            fd: fd_raw,
        };
    }

    fn withPath(path_val: []const u8): SysError {
        return SysError{
            errno: _.errno,
            syscall: _.syscall,
            path: path_val,
            dest: _.dest,
            fd: _.fd,
        };
    }

    fn withDest(dest_val: []const u8): SysError {
        return SysError{
            errno: _.errno,
            syscall: _.syscall,
            path: _.path,
            dest: dest_val,
            fd: _.fd,
        };
    }

    fn withPathDest(path_val: []const u8, dest_val: []const u8): SysError {
        return SysError{
            errno: _.errno,
            syscall: _.syscall,
            path: path_val,
            dest: dest_val,
            fd: _.fd,
        };
    }

    fn withSyscall(tag: SyscallTag): SysError {
        return SysError{
            errno: _.errno,
            syscall: tag,
            path: _.path,
            dest: _.dest,
            fd: _.fd,
        };
    }

    fn withoutPath(): SysError {
        return SysError{
            errno: _.errno,
            syscall: _.syscall,
            path: "",
            dest: "",
            fd: _.fd,
        };
    }

    fn toError(): anyerror {
        return errnoToError(_.errno);
    }

    fn format(buf: []u8): []const u8 {
        const code = _.codeName();
        const desc = _.description();
        const sc = _.syscall.tagName();
        if (_.path.len > 0 && _.dest.len > 0) {
            return std.fmt.bufPrint(buf, "{s}: {s}, {s} '{s}' -> '{s}'", .{ code, desc, sc, _.path, _.dest }) catch buf[0..0];
        }
        if (_.path.len > 0) {
            return std.fmt.bufPrint(buf, "{s}: {s}, {s} '{s}'", .{ code, desc, sc, _.path }) catch buf[0..0];
        }
        if (_.fd >= 0) {
            return std.fmt.bufPrint(buf, "{s}: {s}, {s} (fd={d})", .{ code, desc, sc, _.fd }) catch buf[0..0];
        }
        return std.fmt.bufPrint(buf, "{s}: {s}, {s}", .{ code, desc, sc }) catch buf[0..0];
    }
}

pub struct Maybe<T> {
    result: T = undefined,
    err: SysError? = null,

    static fn initResult(val: T): Maybe<T> {
        return Maybe<T>{ result: val, err: null };
    }

    static fn ok(val: T): Maybe<T> {
        return Maybe<T>{ result: val, err: null };
    }

    static fn success(): Maybe<T> {
        return Maybe<T>{ result: undefined, err: null };
    }

    static fn initErr(e: SysError): Maybe<T> {
        return Maybe<T>{ result: undefined, err: e };
    }

    static fn fail(e: SysError): Maybe<T> {
        return Maybe<T>{ result: undefined, err: e };
    }

    static fn fromCode(code: i32, tag: SyscallTag): Maybe<T> {
        return Maybe<T>{ result: undefined, err: SysError.fromCode(code, tag) };
    }

    fn isOk(): bool {
        return _.err == null;
    }

    fn isErr(): bool {
        return _.err != null;
    }

    fn errno(): u16 {
        if (e := _.err) {
            return e.errno;
        }
        return 0;
    }

    fn asValue(): T? {
        if (_.err != null) return null;
        return _.result;
    }

    fn asErr(): SysError? {
        return _.err;
    }

    fn unwrap(): !T {
        if (e := _.err) {
            return e.toError();
        }
        return _.result;
    }

    fn unwrapOr(default_val: T): T {
        if (_.err != null) return default_val;
        return _.result;
    }
}
