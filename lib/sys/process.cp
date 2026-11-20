// Source: /data/dev/bun/src/platform/linux.cp
// ============================================================================
// lib/sys/process.cp — Process Identity, System Info, Clock & Hardware Primitives
// ============================================================================

const std = @import("std");

pub const UnameInfo = struct {
    sysname: [65]u8 = [0; 65],
    nodename: [65]u8 = [0; 65],
    release: [65]u8 = [0; 65],
    version: [65]u8 = [0; 65],
    machine: [65]u8 = [0; 65],

    fn cstrSlice(arr: *const [65]u8): []const u8 {
        let len: usize = 0;
        while (len < 65 && arr.*[len] != 0) {
            len += 1;
        }
        return arr.*[0..len];
    }

    pub fn sysnameStr(*_): []const u8 {
        return cstrSlice(&_.sysname);
    }

    pub fn nodenameStr(*_): []const u8 {
        return cstrSlice(&_.nodename);
    }

    pub fn releaseStr(*_): []const u8 {
        return cstrSlice(&_.release);
    }

    pub fn versionStr(*_): []const u8 {
        return cstrSlice(&_.version);
    }

    pub fn machineStr(*_): []const u8 {
        return cstrSlice(&_.machine);
    }
};

pub fn getpid(): i32 {
    return @intCast(std.c.getpid());
}

pub fn getppid(): i32 {
    return @intCast(std.c.getppid());
}

pub fn getuid(): u32 {
    return @intCast(std.c.getuid());
}

pub fn getgid(): u32 {
    return @intCast(std.c.getgid());
}

pub fn uname(): !UnameInfo {
    let raw: std.c.utsname = undefined;
    if (std.c.uname(&raw) != 0) {
        return error.UnameFailed;
    }
    let info = UnameInfo{};
    @memcpy(&info.sysname, raw.sysname[0..65]);
    @memcpy(&info.nodename, raw.nodename[0..65]);
    @memcpy(&info.release, raw.release[0..65]);
    @memcpy(&info.version, raw.version[0..65]);
    @memcpy(&info.machine, raw.machine[0..65]);
    return info;
}

pub fn pageSize(): usize {
    return std.heap.pageSize();
}

pub fn cpuCount(): usize {
    return std.Thread.getCpuCount() catch 1;
}

pub fn monotonicNs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
    let sec: u64 = if (ts.sec > 0) @intCast(ts.sec) else 0;
    let nsec: u64 = if (ts.nsec > 0) @intCast(ts.nsec) else 0;
    return (sec * 1000000000) + nsec;
}

pub fn realtimeMs(): i64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    * = std.os.linux.clock_gettime(.REALTIME, &ts);
    return (@as(i64, @intCast(ts.sec)) * 1000) + @divTrunc(@as(i64, @intCast(ts.nsec)), 1000000);
}
