// Source: /data/dev/bun/src/sys/sys.zig & /data/dev/bun/src/sys/Error.cp
// ============================================================================
// lib/sys/index.cp — Cross-platform System Calls, Files, Stat & Process Controls
// ============================================================================

const std = @import("std");

export {
    FD,
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT,
    O_EXCL,
    O_TRUNC,
    O_APPEND,
    O_NONBLOCK,
    O_CLOEXEC,
    F_OK,
    X_OK,
    W_OK,
    R_OK,
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
} from "./fd";

export {
    Stat,
    stat,
    lstat,
    fstat,
} from "./stat";

export {
    File,
} from "./file";

export {
    copyFile,
    copyFileByFd,
    copyFileRange,
} from "./copy_file";

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
    open,
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
    rmdir,
    unlink,
    rename,
    symlink,
    readlink,
    chmod,
    access,
    getcwd,
    chdir,
} from "./fs_ops";

export {
    UnameInfo,
    getpid,
    getppid,
    getuid,
    getgid,
    uname,
    pageSize,
    cpuCount,
    monotonicNs,
    realtimeMs,
} from "./process";

pub struct Sys {
    pub static fn dup(fd: FD): !FD {
        return fd.dup();
    }

    pub static fn dup2(old_fd: FD, new_fd: FD): !FD {
        return old_fd.dup2(new_fd);
    }

    /// Bind the current thread to a specific CPU core using Zig 0.17 std.os.linux.
    pub static fn setAffinity(core_id: usize): !void {
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

        pub static fn open(path: []const u8): !Lib {
            let lib = try std.DynLib.open(path);
            return Lib{ handle: lib };
        }

        pub fn deinit(*_) {
            _.handle.close();
        }

        pub fn lookup(*_, comptime T: type, symbol: []const u8): T {
            return _.handle.lookup(T, symbol) orelse @panic("Failed to find symbol");
        }
    }
}

pub const sys = Sys;
