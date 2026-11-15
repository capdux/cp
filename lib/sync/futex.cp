// lib/sync/futex.cp
// Futex thread synchronization primitive in CP
// Ported from Bun.js core threading architecture with pure CP semantics

pub struct FutexLinux {
    pub static fn wait(ptr: *const std.atomic.Value(u32), expect: u32, timeout: u64?): !void {
        zig {
            var timeout_timespec: std.os.linux.timespec = undefined;
            var timeout_ptr: ?*const std.os.linux.timespec = null;

            if (timeout) |timeout_ns| {
                timeout_timespec = .{
                    .sec = @intCast(timeout_ns / std.time.ns_per_s),
                    .nsec = @intCast(timeout_ns % std.time.ns_per_s),
                };
                timeout_ptr = &timeout_timespec;
            }

            const rc = std.os.linux.futex_4arg(
                &ptr.raw,
                .{ .cmd = .WAIT, .private = true },
                expect,
                timeout_ptr,
            );

            switch (std.posix.errno(rc)) {
                .SUCCESS => {},
                .INVAL => {},
                .TIMEDOUT => {
                    return error.Timeout;
                },
                .INTR => {},
                .AGAIN => {},
                .FAULT => @panic("futex_wait() returned EFAULT unexpectedly"),
                else => @panic("Unexpected futex_wait() return code"),
            }
        }
    }

    pub static fn wake(ptr: *const std.atomic.Value(u32), max_waiters: u32) {
        zig {
            const rc = std.os.linux.futex_3arg(
                &ptr.raw,
                .{ .cmd = .WAKE, .private = true },
                @bitCast(std.math.cast(i32, max_waiters) orelse std.math.maxInt(i32)),
            );

            switch (std.posix.errno(rc)) {
                .SUCCESS => {},
                .INVAL => {},
                .FAULT => @panic("futex_wake() returned EFAULT unexpectedly"),
                else => @panic("Unexpected futex_wake() return code"),
            }
        }
    }
}

pub struct Futex {
    pub static fn wait(ptr: *const std.atomic.Value(u32), expect: u32, timeout_ns: u64?): !void {
        if (t := timeout_ns) {
            if (t == 0) {
                if (ptr.load(.seq_cst) != expect) return;
                return error.Timeout;
            }
        }
        return FutexLinux.wait(ptr, expect, timeout_ns);
    }

    pub static fn waitForever(ptr: *const std.atomic.Value(u32), expect: u32) {
        zig {
            FutexLinux.wait(ptr, expect, null) catch unreachable;
        }
    }

    pub static fn wake(ptr: *const std.atomic.Value(u32), max_waiters: u32) {
        if (max_waiters == 0) return;
        FutexLinux.wake(ptr, max_waiters);
    }
}

pub struct Deadline {
    timeout: u64? = null,
    started_ns: u64 = 0,

    pub static fn init(expires_in_ns: u64?): Deadline {
        let start: u64 = 0;
        if (expires_in_ns != null) {
            zig {
                var ts: std.os.linux.timespec = undefined;
                _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
                start = @as(u64, @intCast(ts.sec)) * std.time.ns_per_s + @as(u64, @intCast(ts.nsec));
            }
        }
        return Deadline {
            timeout: expires_in_ns,
            started_ns: start,
        };
    }

    pub fn wait(*_, ptr: *const std.atomic.Value(u32), expect: u32): !void {
        if (_.timeout == null) {
            Futex.waitForever(ptr, expect);
            return;
        }
        const timeout_ns = _.timeout.?;
        let now: u64 = 0;
        zig {
            var ts: std.os.linux.timespec = undefined;
            _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
            now = @as(u64, @intCast(ts.sec)) * std.time.ns_per_s + @as(u64, @intCast(ts.nsec));
        }
        const elapsed_ns = if (now > _.started_ns) now - _.started_ns else 0;
        const until_timeout_ns = if (timeout_ns > elapsed_ns) timeout_ns - elapsed_ns else 0;
        return Futex.wait(ptr, expect, until_timeout_ns);
    }
}
