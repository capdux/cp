// Source: Zig Standard Library: std.posix
// lib/sys/index.cp
// High-Level System Controls: CPU Affinity & Dynamic Library Loading
// Built upon Zig std.posix and std.DynLib

const std = @import("std");

pub struct Sys {
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

        pub static fn open(path: string): !Lib {
            let lib = try std.DynLib.open(path);
            return Lib{ .handle = lib };
        }

        pub fn deinit(*_) {
            _.handle.close();
        }

        pub fn lookup(*_, comptime T: type, symbol: string): T {
            return _.handle.lookup(T, symbol) orelse @panic("Failed to find symbol");
        }
    }
}
