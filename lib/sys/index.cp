// Source: /data/dev/bun/src/sys/sys.zig & /data/dev/bun/src/sys/Error.cp
// ============================================================================
// lib/sys/index.cp — Cross-platform System Calls, Files, Stat & Process Controls
// ============================================================================

const std = @import("std");

export {
    SyscallTag,
    SysError,
    Maybe,
    EPERM,
    ENOENT,
    ESRCH,
    EINTR,
    EIO,
    ENXIO,
    E2BIG,
    ENOEXEC,
    EBADF,
    ECHILD,
    EAGAIN,
    ENOMEM,
    EACCES,
    EFAULT,
    EBUSY,
    EEXIST,
    EXDEV,
    ENODEV,
    ENOTDIR,
    EISDIR,
    EINVAL,
    ENFILE,
    EMFILE,
    ENOTTY,
    ETXTBSY,
    EFBIG,
    ENOSPC,
    ESPIPE,
    EROFS,
    EMLINK,
    EPIPE,
    ERANGE,
    ENAMETOOLONG,
    ENOSYS,
    ENOTEMPTY,
    ELOOP,
    EWOULDBLOCK,
    EOPNOTSUPP,
    ECONNRESET,
    ECONNREFUSED,
    ETIMEDOUT,
    ECANCELED,
    linuxSyscallErrno,
    cErrno,
    errnoToError,
} from "./error";

export {
    FD,
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT,
    O_EXCL,
    O_NOCTTY,
    O_TRUNC,
    O_APPEND,
    O_NONBLOCK,
    O_DSYNC,
    O_DIRECT,
    O_DIRECTORY,
    O_NOFOLLOW,
    O_CLOEXEC,
    O_SYNC,
    O_PATH,
    O_TMPFILE,
    F_OK,
    X_OK,
    W_OK,
    R_OK,
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
    AT_FDCWD,
    AT_SYMLINK_NOFOLLOW,
    AT_REMOVEDIR,
    AT_SYMLINK_FOLLOW,
    AT_EMPTY_PATH,
    LOCK_SH,
    LOCK_EX,
    LOCK_NB,
    LOCK_UN,
    F_DUPFD,
    F_GETFD,
    F_SETFD,
    F_GETFL,
    F_SETFL,
    F_DUPFD_CLOEXEC,
    FD_CLOEXEC,
} from "./fd";

export {
    FileKind,
    StatxField,
    kindFromMode,
    Stat,
    PosixStat,
    fstat,
    fstatMaybe,
} from "./fstat";

export {
    stat,
    lstat,
    fstatat,
    lstatat,
    statMaybe,
    lstatMaybe,
    fstatatMaybe,
    lstatatMaybe,
    statx,
} from "./stat";

export {
    File,
} from "./file";

export {
    DirEntryKind,
    DirEntry,
    DirEntryOpt,
    DirIterator,
    Dir,
} from "./dir";

export {
    CopyFileState,
    copyFile,
    copyFileByFd,
    copyFileWithState,
} from "./copy_file";

export {
    POLLIN,
    POLLPRI,
    POLLOUT,
    POLLERR,
    POLLHUP,
    POLLNVAL,
    PollFd,
    openat,
    openatMaybe,
    poll,
    copyFileRange,
    sendfile,
} from "./io_ops";

export {
    Signal,
    fromNumber,
    fromName,
} from "./signal";

export {
    getTempDir,
    makeTempFile,
    makeTempDir,
    removeFile,
    removeDir,
} from "./tmp";

export {
    RENAME_NOREPLACE,
    RENAME_EXCHANGE,
    EpollEvent,
    EPOLL_CTL_ADD,
    EPOLL_CTL_DEL,
    EPOLL_CTL_MOD,
    EPOLLIN,
    EPOLLOUT,
    EPOLLERR,
    EPOLLHUP,
    EPOLLRDHUP,
    EPOLLET,
    open,
    openMaybe,
    close,
    read,
    write,
    pread,
    pwrite,
    lseek,
    ftruncate,
    dupFd,
    dup2,
    mkdir,
    mkdirat,
    mkdiratMaybe,
    mkdirAll,
    mkdirAllAt,
    rmdir,
    rmdirat,
    rmdirRecursive,
    rmdirRecursiveAt,
    unlink,
    unlinkat,
    unlinkatMaybe,
    rename,
    renameat,
    renameat2,
    renameat2Maybe,
    symlink,
    symlinkat,
    readlink,
    readlinkat,
    link,
    linkat,
    chmod,
    fchmod,
    chown,
    fchown,
    fsync,
    fdatasync,
    flock,
    fcntl,
    ioctl,
    createPipe,
    pipe2,
    epollCreate,
    epollCtl,
    epollWait,
    access,
    exists,
    getcwd,
    chdir,
    readFile,
    writeFile,
} from "./fs_ops";

export {
    WNOHANG,
    WUNTRACED,
    WCONTINUED,
    UnameInfo,
    WaitPidResult,
    SpawnOptions,
    PosixSpawn,
    getpid,
    getppid,
    getuid,
    getgid,
    uname,
    pageSize,
    cpuCount,
    monotonicNs,
    realtimeMs,
    waitpid,
    waitpidMaybe,
    kill,
    killMaybe,
} from "./process";

export {
    GroupEntry,
    GroupMemberIter,
    EtcGroupIter,
    findGroupByName,
    findGroupByGid,
    isUserInGroup,
} from "./group";

export {
    ProtocolEntry,
    EtcProtocolsIter,
    findProtocolNumber,
    findProtocolByNumber,
} from "./protocols";

export {
    ServiceEntry,
    EtcServicesIter,
    findServicePort,
    findServiceByPort,
} from "./services";

export {
    parseShellLine,
    EtcShellsIter,
    isAllowedShell,
    findShellByBasename,
    countShells,
    isNologinShell,
} from "./shells";

export {
    NulArgIter,
    countArgs,
    argAt,
    ProcEnvEntry,
    ProcEnvIter,
    findProcEnv,
    formatCmdlineDisplay,
} from "./cmdline";

export {
    CpuBlock,
    CpuSummary,
    ProcCpuInfoIter,
    summarizeCpuInfo,
    hasCpuFlag,
} from "./cpuinfo";

export {
    ProcIoStats,
    parseProcIo,
} from "./procio";

export {
    LimitValue,
    ProcLimitEntry,
    ProcLimitsIter,
    parseLimitValue,
    findProcLimit,
    maxOpenFiles,
} from "./limits";

export {
    LoadAvg,
    SystemUptime,
} from "./loadavg";

export {
    MemInfoField,
    MemInfoIter,
    MemInfo,
} from "./meminfo";

export {
    MountEntry,
    ProcMountsIter,
    unescapeMountPath,
    findMountForPath,
} from "./mounts";

export {
    NetDevStats,
    ProcNetDevIter,
    findNetDev,
    sumNonLoopbackBytes,
} from "./netdev";

export {
    RTF_UP,
    RTF_GATEWAY,
    RTF_HOST,
    parseHexIpv4Le,
    RouteEntry,
    ProcRouteIter,
    findDefaultGateway,
} from "./route";

export {
    ProcStatus,
    parseProcStatus,
    findProcStatusField,
} from "./procstatus";

export {
    ProcState,
    parseProcState,
    ProcStat,
} from "./procstat";

export {
    ProcStatm,
    parseProcStatm,
} from "./statm";

export {
    ProcUptime,
    parseProcUptime,
} from "./uptime";

export {
    CgroupLimit,
    CpuMaxQuota,
    IoMaxEntry,
    parseCgroupMax,
    parseCpuMax,
    parseIoMaxLine,
    effectiveMemoryLimit,
} from "./cgroup";

export {
    SysctlEntry,
    SysctlIter,
    normalizeSysctlKey,
    sysctlKeysEqual,
    getSysctlValue,
} from "./sysctl";

export {
    FstabEntry,
    MountIter,
    unescapeFstabPath,
    findFstabForPath,
} from "./fstab";

export {
    OsReleaseField,
    OsReleaseIter,
    getOsReleaseField,
    isOsLike,
} from "./osrelease";

export {
    PasswdEntry,
    PasswdIter,
    lookupUserByName,
    lookupUserByUid,
    GroupEntry as PasswdGroupEntry,
    GroupIter,
    lookupGroupByName,
    lookupGroupByGid,
} from "./passwd";

export {
    NameserverIter,
    ResolvOptions,
    effectiveSearchRaw,
    firstNameserver,
} from "./resolv";

export {
    UnitEntry,
    UnitIter,
    getUnitValue,
    countSectionKey,
} from "./systemd";

export {
    DEFAULT_CONFIG_DIRS,
    DEFAULT_DATA_DIRS,
    XdgEnv,
    XdgDirIter,
    resolveConfigHome,
    resolveDataHome,
    resolveCacheHome,
    resolveStateHome,
    resolveRuntimeDir,
} from "./xdg";

import { pipe2 as sysPipe2, createPipe as sysCreatePipe } from "./fs_ops";

pub struct Sys {
    static fn dup(fd: FD): !FD {
        return fd.dup();
    }

    static fn dup2(old_fd: FD, new_fd: FD): !FD {
        return old_fd.dup2(new_fd);
    }

    static fn pipe(): ![2]FD {
        return sysCreatePipe();
    }

    static fn pipe2(flags: i32): ![2]FD {
        return sysPipe2(flags);
    }

    /// Bind the current thread to a specific CPU core using Zig 0.17 std.os.linux.
    static fn setAffinity(core_id: usize): !void {
        zig {
            var cpu_set = std.mem.zeroes(std.os.linux.cpu_set_t);
            const word_idx = core_id / @bitSizeOf(usize);
            const bit_idx: u6 = @intCast(core_id % @bitSizeOf(usize));
            if (word_idx < cpu_set.len) {
                cpu_set[word_idx] |= (@as(usize, 1) << bit_idx);
            }
            const tid: std.os.linux.pid_t = @intCast(std.os.linux.syscall0(.gettid));
            try std.os.linux.sched_setaffinity(tid, &cpu_set);
        }
    }

    /// Load a dynamic library (e.g., .so/.dylib).
    pub struct Lib {
        handle: std.DynLib,

        static fn open(path: []const u8): !Lib {
            const lib = try std.DynLib.open(path);
            return Lib{ handle: lib };
        }

        fn deinit(*_) {
            _.handle.close();
        }

        fn lookup(*_, comptime T: type, symbol: []const u8): T {
            return _.handle.lookup(T, symbol) orelse @panic("Failed to find symbol");
        }
    }
}

pub const sys = Sys;
