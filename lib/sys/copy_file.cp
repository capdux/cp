// Source: /data/dev/bun/src/sys/copy_file.cp & /data/dev/bun/src/sys/sys.cp
// ============================================================================
// lib/sys/copy_file.cp — High-Performance File Copy Engine (Zero-copy & Fallback)
// ============================================================================

const std = @import("std");
const builtin = @import("builtin");
import { FD } from "./fd";
import { File } from "./file";
import { fstat } from "./stat";
import { SysError, Maybe, linuxSyscallErrno, cErrno, EINTR, EXDEV, EINVAL, EOPNOTSUPP, ENOSYS, EBADF } from "./error";

pub struct CopyFileState {
    has_seen_exdev: bool = false,
    has_ioctl_ficlone_failed: bool = false,
    has_copy_file_range_failed: bool = false,
    has_sendfile_failed: bool = false,
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

fn copyFileReadWriteChunk(src_fd: FD, dest_fd: FD, max_len: usize): !usize {
    let buf: [32768]u8 = undefined;
    const to_read = if (max_len < buf.len) max_len else buf.len;
    const bytes_read = try src_fd.read(buf[0..to_read]);
    if (bytes_read == 0) return 0;
    try dest_fd.writeAll(buf[0..bytes_read]);
    return bytes_read;
}

pub fn copyFileWithState(src_fd: FD, dest_fd: FD, state: *CopyFileState): !void {
    if (!src_fd.isValid() || !dest_fd.isValid()) {
        return error.BadFileDescriptor;
    }

    const max_chunk: usize = 0x7fff0000;

    // Tier 1: Linux copy_file_range (zero-copy in kernel / FS reflink)
    if (comptime builtin.os.tag == .linux) {
        while (!state.has_seen_exdev && !state.has_copy_file_range_failed) {
            const rc = std.os.linux.copy_file_range(src_fd.raw, null, dest_fd.raw, null, max_chunk, 0);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) {
                if (rc == 0) return;
                continue;
            }
            if (err_code == EINTR) continue;
            if (err_code == EXDEV) {
                state.has_seen_exdev = true;
                state.has_copy_file_range_failed = true;
                break;
            }
            if (err_code == EINVAL || err_code == EOPNOTSUPP || err_code == ENOSYS || err_code == EBADF) {
                state.has_copy_file_range_failed = true;
                break;
            }
            state.has_copy_file_range_failed = true;
            break;
        }

        // Tier 2: Linux sendfile (in-kernel page-cache copy)
        while (!state.has_sendfile_failed) {
            const rc = std.os.linux.sendfile(dest_fd.raw, src_fd.raw, null, max_chunk);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) {
                if (rc == 0) return;
                continue;
            }
            if (err_code == EINTR) continue;
            if (err_code == EXDEV) {
                state.has_seen_exdev = true;
            }
            state.has_sendfile_failed = true;
            break;
        }
    }

    // Tier 3: 32KB user-space read/write loop fallback
    while (true) {
        const n = try copyFileReadWriteChunk(src_fd, dest_fd, 32768);
        if (n == 0) break;
    }
}

pub fn copyFileByFd(src_fd: FD, dest_fd: FD): !void {
    let state = CopyFileState{};
    try copyFileWithState(src_fd, dest_fd, *state);
}

pub fn copyFile(src_path: []const u8, dest_path: []const u8): !void {
    let src = try File.openReadOnly(src_path);
    defer src.close();

    let dest = try File.create(dest_path);
    defer dest.close();

    try copyFileByFd(src.fd, dest.fd);
}
