// Source: /data/dev/bun/src/sys/sys.cp
// ============================================================================
// lib/sys/fs_ops.cp — POSIX / Linux File System & Descriptor Syscall Primitives
// ============================================================================

const std = @import("std");
import { FD, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_EXCL, O_TRUNC, O_APPEND, O_NONBLOCK, O_CLOEXEC } from "./fd";

fn toCStr(path: []const u8, buf: []u8): ![*:0]const u8 {
    if (path.len >= buf.len) return error.NameTooLong;
    @memcpy(buf[0 .. path.len], path);
    buf[path.len] = 0;
    return buf[0 .. path.len :0].ptr;
}

pub fn open(path: []const u8, flags: i32, mode: u32): !FD {
    let path_buf: [1024]u8 = undefined;
    let path_z = try toCStr(path, &path_buf);

    let z_flags = std.c.O{
        .CREAT = (flags & O_CREAT) != 0,
        .EXCL = (flags & O_EXCL) != 0,
        .TRUNC = (flags & O_TRUNC) != 0,
        .APPEND = (flags & O_APPEND) != 0,
        .NONBLOCK = (flags & O_NONBLOCK) != 0,
        .CLOEXEC = (flags & O_CLOEXEC) != 0,
    };
    if ((flags & O_RDWR) != 0) {
        z_flags.ACCMODE = .RDWR;
    } else if ((flags & O_WRONLY) != 0) {
        z_flags.ACCMODE = .WRONLY;
    }

    let raw = std.c.open(path_z, z_flags, @as(std.c.mode_t, @intCast(mode)));
    if (raw < 0) return error.OpenFailed;
    return FD.fromNative(raw);
}

pub fn close(fd: FD) {
    fd.close();
}

pub fn read(fd: FD, buf: []u8): !usize {
    return fd.read(buf);
}

pub fn write(fd: FD, data: []const u8): !usize {
    return fd.write(data);
}

pub fn pread(fd: FD, buf: []u8, offset: u64): !usize {
    return fd.pread(buf, offset);
}

pub fn pwrite(fd: FD, data: []const u8, offset: u64): !usize {
    return fd.pwrite(data, offset);
}

pub fn lseek(fd: FD, offset: i64, whence: i32): !u64 {
    return fd.lseek(offset, whence);
}

pub fn ftruncate(fd: FD, length: u64): !void {
    return fd.ftruncate(length);
}

pub fn dupFd(fd: FD): !FD {
    return fd.dup();
}

pub fn dup2(old_fd: FD, new_fd: FD): !FD {
    return old_fd.dup2(new_fd);
}

pub fn mkdir(path: []const u8, mode: u32): !void {
    let path_buf: [1024]u8 = undefined;
    let path_z = try toCStr(path, &path_buf);
    if (std.c.mkdir(path_z, @as(std.c.mode_t, @intCast(mode))) != 0) {
        return error.MkdirFailed;
    }
}

pub fn rmdir(path: []const u8): !void {
    let path_buf: [1024]u8 = undefined;
    let path_z = try toCStr(path, &path_buf);
    if (std.c.rmdir(path_z) != 0) {
        return error.RmdirFailed;
    }
}

pub fn unlink(path: []const u8): !void {
    let path_buf: [1024]u8 = undefined;
    let path_z = try toCStr(path, &path_buf);
    if (std.c.unlink(path_z) != 0) {
        return error.UnlinkFailed;
    }
}

pub fn rename(old_path: []const u8, new_path: []const u8): !void {
    let old_buf: [1024]u8 = undefined;
    let new_buf: [1024]u8 = undefined;
    let old_z = try toCStr(old_path, &old_buf);
    let new_z = try toCStr(new_path, &new_buf);
    if (std.c.rename(old_z, new_z) != 0) {
        return error.RenameFailed;
    }
}

pub fn symlink(target: []const u8, link_path: []const u8): !void {
    let tgt_buf: [1024]u8 = undefined;
    let lnk_buf: [1024]u8 = undefined;
    let tgt_z = try toCStr(target, &tgt_buf);
    let lnk_z = try toCStr(link_path, &lnk_buf);
    if (std.c.symlink(tgt_z, lnk_z) != 0) {
        return error.SymlinkFailed;
    }
}

pub fn readlink(path: []const u8, buf: []u8): ![]const u8 {
    if (buf.len == 0) return error.BufferTooSmall;
    let path_buf: [1024]u8 = undefined;
    let path_z = try toCStr(path, &path_buf);
    let n = std.c.readlink(path_z, buf.ptr, buf.len);
    if (n < 0) return error.ReadlinkFailed;
    let len: usize = @intCast(n);
    return buf[0 .. len];
}

pub fn chmod(path: []const u8, mode: u32): !void {
    let path_buf: [1024]u8 = undefined;
    let path_z = try toCStr(path, &path_buf);
    if (std.c.chmod(path_z, @as(std.c.mode_t, @intCast(mode))) != 0) {
        return error.ChmodFailed;
    }
}

pub fn access(path: []const u8, mode: i32): !void {
    let path_buf: [1024]u8 = undefined;
    let path_z = try toCStr(path, &path_buf);
    if (std.c.access(path_z, @intCast(mode)) != 0) {
        return error.AccessDenied;
    }
}

pub fn getcwd(buf: []u8): ![]const u8 {
    if (buf.len == 0) return error.BufferTooSmall;
    let ptr = std.c.getcwd(buf.ptr, buf.len);
    if (ptr == null) return error.GetcwdFailed;
    let c_ptr: [*:0]const u8 = @ptrCast(buf.ptr);
    return std.mem.span(c_ptr);
}

pub fn chdir(path: []const u8): !void {
    let path_buf: [1024]u8 = undefined;
    let path_z = try toCStr(path, &path_buf);
    if (std.c.chdir(path_z) != 0) {
        return error.ChdirFailed;
    }
}
