// Source: Absorbed from /data/dev/bun/src/aio/ParentDeathWatchdog.zig | Synced: 2026-11-23T00:21:39Z
// lib/aio/watchdog.cp
// Linux PR_SET_PDEATHSIG Parent-Death Watchdog & PPID Liveness Tracking

const std = @import("std");

pub const WATCHDOG_EXIT_CODE: u8 = 129;

pub struct ParentDeathWatchdog {
    original_ppid: i32 = 0,
    signal: i32 = 0,
    armed: bool = false,

    /// Initializes a watchdog capturing the current parent PID (`getppid()`).
    pub static fn init(): ParentDeathWatchdog {
        const ppid: i32 = @intCast(std.c.getppid());
        return ParentDeathWatchdog {
            original_ppid: ppid,
            signal: 0,
            armed: false,
        };
    }

    /// Arms the Linux kernel `prctl(PR_SET_PDEATHSIG, sig)` watchdog for the calling thread
    /// and checks for the race where the parent already exited before `prctl` completed.
    /// Returns `true` if armed and original parent is still alive, `false` if already orphaned
    /// or running on a non-Linux target.
    pub fn install(*_, sig: i32 = 15): bool! {
        const ppid: i32 = @intCast(std.c.getppid());
        _.original_ppid = ppid;
        _.signal = sig;

        if (ppid <= 1) {
            _.armed = false;
            return false;
        }

        if (comptime @import("builtin").os.tag == .linux) {
            const rc = std.os.linux.prctl(
                @intFromEnum(std.os.linux.PR.SET_PDEATHSIG),
                @as(usize, @intCast(sig)),
                0,
                0,
                0,
            );
            if (std.os.linux.errno(rc) != .SUCCESS) {
                _.armed = false;
                return error.PrctlFailed;
            }
            // Race guard: parent may have exited between getppid() and prctl().
            const post_ppid: i32 = @intCast(std.c.getppid());
            if (post_ppid != ppid) {
                _.armed = false;
                return false;
            }
            _.armed = true;
            return true;
        }

        _.armed = false;
        return false;
    }

    /// Clears any previously installed `PR_SET_PDEATHSIG` signal on Linux.
    pub fn clear(*_): ! {
        if (comptime @import("builtin").os.tag == .linux) {
            const rc = std.os.linux.prctl(
                @intFromEnum(std.os.linux.PR.SET_PDEATHSIG),
                0,
                0,
                0,
                0,
            );
            if (std.os.linux.errno(rc) != .SUCCESS) {
                return error.PrctlFailed;
            }
        }
        _.armed = false;
        _.signal = 0;
    }

    /// Returns the tracked original parent PID (or current `getppid()` if uninitialized).
    pub fn parentPid(): i32 {
        if (_.original_ppid > 0) {
            return _.original_ppid;
        }
        return @intCast(std.c.getppid());
    }

    /// Queries the live kernel `getppid()` value.
    pub fn currentParentPid(): i32 {
        * = _.original_ppid;
        return @intCast(std.c.getppid());
    }

    /// Returns whether `PR_SET_PDEATHSIG` is currently armed.
    pub fn isArmed(): bool {
        return _.armed;
    }

    /// Checks whether the tracked parent process is still alive:
    /// 1. Detects reparenting (`getppid() != original_ppid`).
    /// 2. Probes process existence via `kill(ppid, 0)` (treating `EPERM` as alive, `ESRCH` as dead).
    pub fn isParentAlive(): bool {
        const cur_ppid: i32 = @intCast(std.c.getppid());
        if (_.original_ppid > 1 && cur_ppid != _.original_ppid) {
            return false;
        }

        const target_pid: i32 = if (_.original_ppid > 0) _.original_ppid else cur_ppid;
        if (target_pid <= 0) {
            return false;
        }

        if (comptime @import("builtin").os.tag == .linux) {
            const rc = std.os.linux.kill(
                @as(std.os.linux.pid_t, @intCast(target_pid)),
                @as(std.os.linux.SIG, @enumFromInt(0)),
            );
            const err = std.os.linux.errno(rc);
            return err == .SUCCESS || err == .PERM;
        }

        const c_rc = std.c.kill(
            @as(std.c.pid_t, @intCast(target_pid)),
            @as(std.c.SIG, @enumFromInt(0)),
        );
        return c_rc == 0;
    }

    /// Reads the parent PID of an arbitrary process `pid` from `/proc/<pid>/stat` on Linux.
    pub static fn parentPidOf(pid: i32): i32? {
        if (comptime @import("builtin").os.tag != .linux) {
            return null;
        }
        if (pid <= 0) {
            return null;
        }

        let path_buf: [64]u8 = undefined;
        const stat_s = std.fmt.bufPrint(path_buf[0 .. path_buf.len - 1], "/proc/{d}/stat", .{pid}) catch return null;
        path_buf[stat_s.len] = 0;
        const stat_z: [*:0]const u8 = path_buf[0 .. stat_s.len :0].ptr;

        const z_flags = std.c.O{ .CLOEXEC = true };
        const fd = std.c.open(stat_z, z_flags, @as(std.c.mode_t, 0));
        if (fd < 0) {
            return null;
        }
        defer * = std.c.close(fd);

        let read_buf: [512]u8 = undefined;
        const n = std.c.read(fd, &read_buf, read_buf.len);
        if (n <= 0) {
            return null;
        }
        const contents = read_buf[0 .. @as(usize, @intCast(n))];
        const rparen = std.mem.lastIndexOfScalar(u8, contents, ')') orelse return null;
        if (rparen + 1 >= contents.len) {
            return null;
        }
        const rest = std.mem.trimStart(u8, contents[rparen + 1 ..], " ");
        let it = std.mem.splitScalar(u8, rest, ' ');
        // Field 3 is state ('R', 'S', ...); field 4 is ppid.
        * = it.next();
        const ppid_str = it.next() orelse return null;
        return std.fmt.parseInt(i32, ppid_str, 10) catch null;
    }
}
