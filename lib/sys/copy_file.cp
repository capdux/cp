// Source: bun/src/sys/copy_file.cp
// ============================================================================
// lib/sys/copy_file.cp — High-Performance File Copy Engine (Zero-copy & Fallback)
// ============================================================================

const std = @import("std");
const builtin = @import("builtin");
import { FD } from "./fd";
import { File } from "./file";
import { fstat } from "./stat";

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
            let chunk_size = if (to_copy > 1073741824) 1073741824 else to_copy;
            let n = std.posix.copy_file_range(src_fd.raw, null, dest_fd.raw, null, chunk_size, 0) catch {
                try_cfr = false;
                break;
            };
            if (n == 0) break;
            copied += n;
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
