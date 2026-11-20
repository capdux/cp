// Source: /data/dev/bun/src/sys/copy_file.cp
// ============================================================================
// lib/sys/copy_file.cp — High-Performance File Copy Engine (Zero-copy & Fallback)
// ============================================================================

const std = @import("std");
const builtin = @import("builtin");
import { FD } from "./fd";
import { File } from "./file";
import { fstat } from "./stat";

pub fn copyFileRange(src_fd: FD, src_off: (*u64)?, dest_fd: FD, dest_off: (*u64)?, len: usize): !usize {
    if (!src_fd.isValid() or !dest_fd.isValid()) {
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

    let n = std.c.copy_file_range(src_fd.raw, s_ptr, dest_fd.raw, d_ptr, len, 0);
    if (n < 0) return error.CopyFileRangeFailed;

    if (sp := src_off) {
        sp.* = @intCast(s_off_val);
    }
    if (dp := dest_off) {
        dp.* = @intCast(d_off_val);
    }
    return @intCast(n);
}

pub fn copyFileByFd(src_fd: FD, dest_fd: FD): !void {
    if (!src_fd.isValid() or !dest_fd.isValid()) {
        return error.BadFileDescriptor;
    }

    let st = try fstat(src_fd);
    let total_size = st.size;

    // 1. Try Linux copy_file_range for kernel-level zero-copy
    if (comptime builtin.os.tag == .linux) {
        let copied: u64 = 0;
        let try_cfr = true;
        while (copied < total_size and try_cfr) {
            let to_copy = total_size - copied;
            let chunk_size: usize = if (to_copy > 1073741824) 1073741824 else @intCast(to_copy);
            let n = std.c.copy_file_range(src_fd.raw, null, dest_fd.raw, null, chunk_size, 0);
            if (n < 0) {
                try_cfr = false;
                break;
            }
            if (n == 0) break;
            copied += @as(u64, @intCast(n));
        }
        if (copied >= total_size) {
            return;
        }
    }

    // 2. Fallback: chunked user-space buffer copy
    let buf: [16384]u8 = undefined;
    while (true) {
        let bytes_read = try src_fd.read(&buf);
        if (bytes_read == 0) break;
        try dest_fd.writeAll(buf[0 .. bytes_read]);
    }
}

pub fn copyFile(src_path: []const u8, dest_path: []const u8): !void {
    let src = try File.openReadOnly(src_path);
    defer src.close();

    let dest = try File.create(dest_path);
    defer dest.close();

    try copyFileByFd(src.fd, dest.fd);
}
