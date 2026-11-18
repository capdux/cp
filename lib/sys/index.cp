// Source: bun/src/sys/sys.cp
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
} from "./fd";

export {
    Stat,
    stat,
    fstat,
} from "./stat";

export {
    File,
} from "./file";

export {
    copyFile,
    copyFileByFd,
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

const mod_fd = @import("./fd");
const mod_file = @import("./file");
const mod_stat = @import("./stat");
const mod_copy_file = @import("./copy_file");
const mod_signal = @import("./signal");
const mod_tmp = @import("./tmp");

pub struct Sys {
    pub const FD = mod_fd.FD;
    pub const File = mod_file.File;
    pub const Stat = mod_stat.Stat;
    pub const stat = mod_stat.stat;
    pub const fstat = mod_stat.fstat;
    pub const copyFile = mod_copy_file.copyFile;
    pub const copyFileByFd = mod_copy_file.copyFileByFd;
    pub const Signal = mod_signal.Signal;
    pub const getTempDir = mod_tmp.getTempDir;
    pub const makeTempFile = mod_tmp.makeTempFile;
    pub const makeTempDir = mod_tmp.makeTempDir;
    pub const removeFile = mod_tmp.removeFile;
    pub const removeDir = mod_tmp.removeDir;

    /// Bind the current thread to a specific CPU core.
    pub static fn setAffinity(core_id: usize): !void {
        zig {
            const posix = std.posix;
            var cpu_set = posix.cpu_set_t{};
            posix.CPU_ZERO(&cpu_set);
            posix.CPU_SET(core_id, &cpu_set);
            
            const tid = posix.gettid();
            try posix.sched_setaffinity(tid, @sizeOf(posix.cpu_set_t), &cpu_set);
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
