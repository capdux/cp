// Source: Absorbed from /data/dev/bun/src/sys/sys.cp | Synced: 2026-11-22T10:09:59Z
// ============================================================================
// lib/sys/io_ops.cp — Low-level Descriptor Open, Poll & Zero-Copy Syscall Ops
// (Isolated from high-level fs_ops/copy_file to avoid single-file collisions
//  with cap:fs when cap:io or cap:stream is imported alongside cap:fs.)
// ============================================================================

const std = @import("std");
import { FD, AT_FDCWD, flagsToLinuxO } from "./fd";
import { SysError, Maybe, linuxSyscallErrno, cErrno, EINTR, ENAMETOOLONG } from "./error";

pub const POLLIN: i16 = 0x001;
pub const POLLPRI: i16 = 0x002;
pub const POLLOUT: i16 = 0x004;
pub const POLLERR: i16 = 0x008;
pub const POLLHUP: i16 = 0x010;
pub const POLLNVAL: i16 = 0x020;

pub type PollFd = std.os.linux.pollfd;

pub fn openatMaybe(dir_fd: FD, path: []const u8, flags: i32, mode: u32): Maybe<FD> {
    let path_buf: [4096]u8 = undefined;
    if (path.len >= path_buf.len) {
        return Maybe<FD>.initErr(SysError.fromCode(ENAMETOOLONG, .openat).withPath(path));
    }
    @memcpy(path_buf[0 .. path.len], path);
    path_buf[path.len] = 0;
    const path_z: [*:0]const u8 = path_buf[0 .. path.len :0].ptr;
    const z_flags = flagsToLinuxO(flags);
    const base_raw = if (dir_fd.isValid()) dir_fd.raw else AT_FDCWD;

    while (true) {
        const rc = std.os.linux.openat(base_raw, path_z, z_flags, @as(std.os.linux.mode_t, @intCast(mode)));
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) {
            return Maybe<FD>.initResult(FD.fromNative(@intCast(rc)));
        }
        if (err_code == EINTR) continue;
        return Maybe<FD>.initErr(SysError.fromCode(err_code, .openat).withPath(path));
    }
}

pub fn openat(dir_fd: FD, path: []const u8, flags: i32, mode: u32): !FD {
    return openatMaybe(dir_fd, path, flags, mode).unwrap();
}

pub fn poll(fds: []PollFd, timeout_ms: i32): !usize {
    if (fds.len == 0) return 0;
    while (true) {
        const rc = std.os.linux.poll(fds.ptr, fds.len, timeout_ms);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) return rc;
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .poll).toError();
    }
}

pub fn copyFileRange(src_fd: FD, src_off: (*u64)?, dest_fd: FD, dest_off: (*u64)?, len: usize): !usize {
    if (!src_fd.isValid() || !dest_fd.isValid()) {
        return error.BadFileDescriptor;
    }
    let s_off_val: i64 = 0;
    let s_ptr: (*i64!)? = null;
    if (sp := src_off) {
        s_off_val = @intCast(sp.*);
        unsafe { s_ptr = *s_off_val!; }
    }
    let d_off_val: i64 = 0;
    let d_ptr: (*i64!)? = null;
    if (dp := dest_off) {
        d_off_val = @intCast(dp.*);
        unsafe { d_ptr = *d_off_val!; }
    }

    while (true) {
        const n = std.c.copy_file_range(src_fd.raw, s_ptr, dest_fd.raw, d_ptr, len, 0);
        if (n >= 0) {
            if (sp := src_off) {
                sp.* = @intCast(s_off_val);
            }
            if (dp := dest_off) {
                dp.* = @intCast(d_off_val);
            }
            return @intCast(n);
        }
        const err_code = cErrno();
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .copy_file_range).toError();
    }
}

pub fn sendfile(src_fd: FD, dest_fd: FD, offset: (*i64)?, len: usize): !usize {
    if (!src_fd.isValid() || !dest_fd.isValid()) {
        return error.BadFileDescriptor;
    }
    let off_val: i64 = 0;
    let off_ptr: (*i64!)? = null;
    if (op := offset) {
        off_val = op.*;
        unsafe { off_ptr = *off_val!; }
    }
    while (true) {
        const rc = std.os.linux.sendfile(dest_fd.raw, src_fd.raw, off_ptr, len);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) {
            if (op := offset) {
                op.* = off_val;
            }
            return rc;
        }
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .sendfile).toError();
    }
}
