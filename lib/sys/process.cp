// Source: /data/dev/bun/src/platform/linux.cp & /data/dev/bun/src/runtime/api/bun/spawn.cp
// ============================================================================
// lib/sys/process.cp — Process Identity, Clock, WaitPid, Kill & PosixSpawn
// ============================================================================

const std = @import("std");
import { FD, O_CLOEXEC } from "./fd";
import { SysError, Maybe, linuxSyscallErrno, EINTR, ENOENT, ENOMEM } from "./error";

pub const WNOHANG: i32 = 1;
pub const WUNTRACED: i32 = 2;
pub const WCONTINUED: i32 = 8;

pub struct UnameInfo {
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

    fn sysnameStr(*_): []const u8 {
        return cstrSlice(&_.sysname);
    }

    fn nodenameStr(*_): []const u8 {
        return cstrSlice(&_.nodename);
    }

    fn releaseStr(*_): []const u8 {
        return cstrSlice(&_.release);
    }

    fn versionStr(*_): []const u8 {
        return cstrSlice(&_.version);
    }

    fn machineStr(*_): []const u8 {
        return cstrSlice(&_.machine);
    }
}

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
    const sec: u64 = if (ts.sec > 0) @intCast(ts.sec) else 0;
    const nsec: u64 = if (ts.nsec > 0) @intCast(ts.nsec) else 0;
    return (sec * 1000000000) + nsec;
}

pub fn realtimeMs(): i64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    * = std.os.linux.clock_gettime(.REALTIME, &ts);
    return (@as(i64, @intCast(ts.sec)) * 1000) + @divTrunc(@as(i64, @intCast(ts.nsec)), 1000000);
}

pub struct WaitPidResult {
    pid: i32 = 0,
    status: u32 = 0,

    fn exited(): bool {
        return (_.status & 0x7f) == 0;
    }

    fn exitCode(): u8 {
        return @intCast((_.status & 0xff00) >> 8);
    }

    fn signaled(): bool {
        const term = _.status & 0x7f;
        return term != 0 && term != 0x7f;
    }

    fn termSig(): u8 {
        return @intCast(_.status & 0x7f);
    }

    fn stopped(): bool {
        return (_.status & 0xff) == 0x7f;
    }

    fn stopSig(): u8 {
        return @intCast((_.status & 0xff00) >> 8);
    }
}

pub fn waitpidMaybe(pid: i32, flags: i32): Maybe<WaitPidResult> {
    let status: i32 = 0;
    while (true) {
        const rc = std.os.linux.waitpid(pid, &status, @intCast(flags));
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) {
            return Maybe<WaitPidResult>.initResult(WaitPidResult{
                pid: @intCast(rc),
                status: @bitCast(status),
            });
        }
        if (err_code == EINTR) continue;
        return Maybe<WaitPidResult>.initErr(SysError.fromCode(err_code, .waitpid));
    }
}

pub fn waitpid(pid: i32, flags: i32): !WaitPidResult {
    return waitpidMaybe(pid, flags).unwrap();
}

pub fn killMaybe(pid: i32, sig: i32): Maybe<void> {
    const rc = std.os.linux.kill(pid, @enumFromInt(sig));
    const err_code = linuxSyscallErrno(rc);
    if (err_code == 0) return Maybe<void>.success();
    return Maybe<void>.initErr(SysError.fromCode(err_code, .kill));
}

pub fn kill(pid: i32, sig: i32): !void {
    return killMaybe(pid, sig).unwrap();
}

pub struct SpawnOptions {
    cwd: ([]const u8)? = null,
    envp: ([]const []const u8)? = null,
    stdin_fd: FD? = null,
    stdout_fd: FD? = null,
    stderr_fd: FD? = null,
    detached: bool = false,
    new_process_group: bool = false,
    pdeathsig: i32 = 0,
    search_path: bool = true,
}

fn findSlash(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') return true;
        i += 1;
    }
    return false;
}

type ProcessCStrPtr = [*:0]const u8;
type ProcessOptCStrPtr = ProcessCStrPtr?;

fn execWithOptionalPath(
    exe: [*:0]const u8,
    exe_slice: []const u8,
    argv_ptrs: [*:null]const ProcessOptCStrPtr,
    envp_ptrs: [*:null]const ProcessOptCStrPtr,
    search_path: bool,
): i32 {
    if (!search_path || findSlash(exe_slice)) {
        const rc = std.os.linux.execve(exe, argv_ptrs, envp_ptrs);
        const err_code = linuxSyscallErrno(rc);
        return if (err_code != 0) err_code else 2;
    }

    let path_env: []const u8 = "/usr/local/bin:/usr/bin:/bin";
    if (p := std.c.getenv("PATH")) {
        path_env = std.mem.span(p);
    }

    let cand_buf: [4096]u8 = undefined;
    let start: usize = 0;
    let last_err: i32 = ENOENT;

    while (start <= path_env.len) {
        let end = start;
        while (end < path_env.len && path_env[end] != ':') {
            end += 1;
        }
        const dir = path_env[start..end];
        const dir_str = if (dir.len == 0) "." else dir;
        if (dir_str.len + 1 + exe_slice.len < cand_buf.len) {
            @memcpy(cand_buf[0..dir_str.len], dir_str);
            cand_buf[dir_str.len] = '/';
            @memcpy(cand_buf[dir_str.len + 1 .. dir_str.len + 1 + exe_slice.len], exe_slice);
            cand_buf[dir_str.len + 1 + exe_slice.len] = 0;
            const cand_z: [*:0]const u8 = cand_buf[0 .. dir_str.len + 1 + exe_slice.len :0].ptr;
            const rc = std.os.linux.execve(cand_z, argv_ptrs, envp_ptrs);
            const err_code = linuxSyscallErrno(rc);
            if (err_code != ENOENT && err_code != 20) {
                last_err = err_code;
            }
        }
        if (end >= path_env.len) break;
        start = end + 1;
    }
    return last_err;
}

fn dupeCString(allocator: std.mem.Allocator, s: []const u8): ![:0]u8 {
    let out = try allocator.allocSentinel(u8, s.len, 0);
    if (s.len > 0) {
        @memcpy(out[0..s.len], s);
    }
    return out;
}

pub struct PosixSpawn {
    pub type WaitResult = WaitPidResult;

    static fn wait(pid: i32, flags: i32): !WaitPidResult {
        return waitpid(pid, flags);
    }

    static fn signal(pid: i32, sig: i32): !void {
        return kill(pid, sig);
    }

    static fn spawnMaybe(allocator: std.mem.Allocator, argv: []const []const u8, opts: SpawnOptions): Maybe<i32> {
        if (argv.len == 0) {
            return Maybe<i32>.initErr(SysError.fromCode(22, .posix_spawn));
        }

        let argv_z = allocator.allocSentinel(ProcessOptCStrPtr, argv.len, null) catch {
            return Maybe<i32>.initErr(SysError.fromCode(ENOMEM, .posix_spawn));
        };
        defer {
            let k: usize = 0;
            while (k < argv.len) {
                if (ptr := argv_z[k]) {
                    allocator.free(std.mem.span(ptr));
                }
                k += 1;
            }
            allocator.free(argv_z);
        }
        let init_i: usize = 0;
        while (init_i < argv.len) {
            argv_z[init_i] = null;
            init_i += 1;
        }
        let i: usize = 0;
        while (i < argv.len) {
            const duped = dupeCString(allocator, argv[i]) catch {
                return Maybe<i32>.initErr(SysError.fromCode(ENOMEM, .posix_spawn));
            };
            argv_z[i] = duped.ptr;
            i += 1;
        }

        let custom_env_alloc: ([:null]ProcessOptCStrPtr)? = null;
        let env_count: usize = 0;
        if (ev := opts.envp) {
            env_count = ev.len;
            let env_z = allocator.allocSentinel(ProcessOptCStrPtr, ev.len, null) catch {
                return Maybe<i32>.initErr(SysError.fromCode(ENOMEM, .posix_spawn));
            };
            let j: usize = 0;
            while (j < ev.len) {
                env_z[j] = null;
                j += 1;
            }
            j = 0;
            while (j < ev.len) {
                const duped_e = dupeCString(allocator, ev[j]) catch {
                    let m: usize = 0;
                    while (m < ev.len) {
                        if (ep := env_z[m]) allocator.free(std.mem.span(ep));
                        m += 1;
                    }
                    allocator.free(env_z);
                    return Maybe<i32>.initErr(SysError.fromCode(ENOMEM, .posix_spawn));
                };
                env_z[j] = duped_e.ptr;
                j += 1;
            }
            custom_env_alloc = env_z;
        }
        defer {
            if (cz := custom_env_alloc) {
                let m: usize = 0;
                while (m < env_count) {
                    if (ep := cz[m]) allocator.free(std.mem.span(ep));
                    m += 1;
                }
                allocator.free(cz);
            }
        }

        let cwd_z: ([:0]u8)? = null;
        if (cwd := opts.cwd) {
            cwd_z = dupeCString(allocator, cwd) catch {
                return Maybe<i32>.initErr(SysError.fromCode(ENOMEM, .posix_spawn).withPath(cwd));
            };
        }
        defer {
            if (cz := cwd_z) allocator.free(cz);
        }

        // CLOEXEC pipe to report exact child exec/chdir errno back to parent
        let err_pipe: [2]i32 = undefined;
        const p_rc = std.os.linux.pipe2(&err_pipe, std.os.linux.O{ .CLOEXEC = true });
        const p_err = linuxSyscallErrno(p_rc);
        if (p_err != 0) {
            return Maybe<i32>.initErr(SysError.fromCode(p_err, .pipe2));
        }

        const fork_rc = std.os.linux.fork();
        const fork_err = linuxSyscallErrno(fork_rc);
        if (fork_err != 0) {
            * = std.os.linux.close(err_pipe[0]);
            * = std.os.linux.close(err_pipe[1]);
            return Maybe<i32>.initErr(SysError.fromCode(fork_err, .fork));
        }

        if (fork_rc == 0) {
            // Child process
            * = std.os.linux.close(err_pipe[0]);

            if (opts.detached) {
                * = std.os.linux.setsid();
            } else if (opts.new_process_group) {
                * = std.os.linux.setpgid(0, 0);
            }

            if (opts.pdeathsig > 0) {
                * = std.os.linux.prctl(@intFromEnum(std.os.linux.PR.SET_PDEATHSIG), @intCast(opts.pdeathsig), 0, 0, 0);
            }

            if (cz := cwd_z) {
                const cd_rc = std.os.linux.chdir(cz.ptr);
                const cd_err = linuxSyscallErrno(cd_rc);
                if (cd_err != 0) {
                    let err_bytes: [4]u8 = undefined;
                    std.mem.writeInt(i32, &err_bytes, cd_err, .little);
                    * = std.os.linux.write(err_pipe[1], &err_bytes, 4);
                    std.os.linux.exit_group(127);
                }
            }

            if (sfd := opts.stdin_fd) {
                if (sfd.raw != 0) {
                    * = std.os.linux.dup2(sfd.raw, 0);
                }
            }
            if (ofd := opts.stdout_fd) {
                if (ofd.raw != 1) {
                    * = std.os.linux.dup2(ofd.raw, 1);
                }
            }
            if (efd := opts.stderr_fd) {
                if (efd.raw != 2) {
                    * = std.os.linux.dup2(efd.raw, 2);
                }
            }

            let env_ptr: [*:null]const ProcessOptCStrPtr = undefined;
            if (cz := custom_env_alloc) {
                env_ptr = cz.ptr;
            } else {
                env_ptr = @ptrCast(std.c.environ);
            }

            const exec_err = execWithOptionalPath(
                argv_z[0].?,
                argv[0],
                argv_z.ptr,
                env_ptr,
                opts.search_path,
            );
            let err_bytes: [4]u8 = undefined;
            std.mem.writeInt(i32, &err_bytes, exec_err, .little);
            * = std.os.linux.write(err_pipe[1], &err_bytes, 4);
            std.os.linux.exit_group(127);
        }

        // Parent process
        const child_pid: i32 = @intCast(fork_rc);
        * = std.os.linux.close(err_pipe[1]);

        let err_buf: [4]u8 = [0; 4];
        let total_read: usize = 0;
        while (total_read < 4) {
            const r_rc = std.os.linux.read(err_pipe[0], err_buf[total_read..].ptr, 4 - total_read);
            const r_err = linuxSyscallErrno(r_rc);
            if (r_err == EINTR) continue;
            if (r_err != 0 || r_rc == 0) break;
            total_read += r_rc;
        }
        * = std.os.linux.close(err_pipe[0]);

        if (total_read == 4) {
            const child_errno = std.mem.readInt(i32, &err_buf, .little);
            * = waitpidMaybe(child_pid, 0);
            return Maybe<i32>.initErr(SysError.fromCode(child_errno, .posix_spawn).withPath(argv[0]));
        }

        return Maybe<i32>.initResult(child_pid);
    }

    static fn spawn(allocator: std.mem.Allocator, argv: []const []const u8, opts: SpawnOptions): !i32 {
        return PosixSpawn.spawnMaybe(allocator, argv, opts).unwrap();
    }
}
