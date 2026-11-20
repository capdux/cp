// Source: /data/dev/bun/src/sys/File.cp
// ============================================================================
// lib/sys/file.cp — RAII File Handle Wrapper & High-level File Operations
// ============================================================================

const std = @import("std");
import { FD, AT_FDCWD, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_EXCL, O_TRUNC, O_APPEND, O_NONBLOCK, O_CLOEXEC, flagsToLinuxO } from "./fd";
import { Stat, fstat } from "./fstat";
import { SysError, Maybe, linuxSyscallErrno, EINTR, ENAMETOOLONG } from "./error";

fn ensureParentDirsAt(dir_fd: FD, path: []const u8) {
    let end: usize = path.len;
    while (end > 0 && path[end - 1] != '/') {
        end -= 1;
    }
    if (end <= 1) return;
    const parent = path[0 .. end - 1];
    let i: usize = 0;
    if (parent.len > 0 && parent[0] == '/') i = 1;
    const base_raw = if (dir_fd.isValid()) dir_fd.raw else AT_FDCWD;
    let buf: [4096]u8 = undefined;
    while (i <= parent.len) {
        if (i == parent.len || parent[i] == '/') {
            if (i > 0 && i < buf.len) {
                @memcpy(buf[0..i], parent[0..i]);
                buf[i] = 0;
                const p_z: [*:0]const u8 = buf[0 .. i :0].ptr;
                * = std.os.linux.mkdirat(base_raw, p_z, 0o755);
            }
        }
        i += 1;
    }
}

pub struct File {
    fd: FD = FD.invalid,

    static fn fromFd(fd: FD): File {
        return File{ fd: fd };
    }

    static fn openatMaybe(dir_fd: FD, path: []const u8, flags: i32, mode: u32): Maybe<File> {
        let path_buf: [4096]u8 = undefined;
        if (path.len >= path_buf.len) {
            return Maybe<File>.initErr(SysError.fromCode(ENAMETOOLONG, .openat).withPath(path));
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
                return Maybe<File>.initResult(File{ fd: FD.fromNative(@intCast(rc)) });
            }
            if (err_code == EINTR) continue;
            return Maybe<File>.initErr(SysError.fromCode(err_code, .openat).withPath(path));
        }
    }

    static fn openat(dir_fd: FD, path: []const u8, flags: i32, mode: u32): !File {
        return File.openatMaybe(dir_fd, path, flags, mode).unwrap();
    }

    static fn openMaybe(path: []const u8, flags: i32, mode: u32): Maybe<File> {
        return File.openatMaybe(FD.cwd(), path, flags, mode);
    }

    static fn open(path: []const u8, flags: i32, mode: u32): !File {
        return File.openat(FD.cwd(), path, flags, mode);
    }

    static fn makeOpenat(dir_fd: FD, path: []const u8, flags: i32, mode: u32): !File {
        const res = File.openatMaybe(dir_fd, path, flags, mode);
        if (res.isOk()) return res.unwrap();
        ensureParentDirsAt(dir_fd, path);
        return File.openat(dir_fd, path, flags, mode);
    }

    static fn makeOpen(path: []const u8, flags: i32, mode: u32): !File {
        return File.makeOpenat(FD.cwd(), path, flags, mode);
    }

    static fn create(path: []const u8): !File {
        return File.open(path, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, 0o644);
    }

    static fn openReadOnly(path: []const u8): !File {
        return File.open(path, O_RDONLY | O_CLOEXEC, 0);
    }

    fn close(*_) {
        _.fd.close();
        _.fd = FD.invalid;
    }

    fn read(*_, buf: []u8): !usize {
        return _.fd.read(buf);
    }

    fn readAll(*_, buf: []u8): !usize {
        return _.fd.readAll(buf);
    }

    fn write(*_, data: []const u8): !usize {
        return _.fd.write(data);
    }

    fn writeAll(*_, data: []const u8): !void {
        return _.fd.writeAll(data);
    }

    fn pread(*_, buf: []u8, offset: u64): !usize {
        return _.fd.pread(buf, offset);
    }

    fn pwrite(*_, data: []const u8, offset: u64): !usize {
        return _.fd.pwrite(data, offset);
    }

    fn pwriteAll(*_, data: []const u8, initial_offset: u64): !void {
        let pos: usize = 0;
        let off: u64 = initial_offset;
        while (pos < data.len) {
            const n = try _.fd.pwrite(data[pos..data.len], off);
            if (n == 0) return error.WriteZero;
            pos += n;
            off += (n as u64);
        }
    }

    fn lseek(*_, offset: i64, whence: i32): !u64 {
        return _.fd.lseek(offset, whence);
    }

    fn seekTo(*_, offset: i64): !u64 {
        return _.fd.seekTo(offset);
    }

    fn seekFromEnd(*_, offset: i64): !u64 {
        return _.fd.seekFromEnd(offset);
    }

    fn ftruncate(*_, length: u64): !void {
        return _.fd.ftruncate(length);
    }

    fn sync(*_): !void {
        return _.fd.sync();
    }

    fn fdatasync(*_): !void {
        return _.fd.fdatasync();
    }

    fn flock(*_, operation: i32): !void {
        return _.fd.flock(operation);
    }

    fn fchmod(*_, mode: u32): !void {
        return _.fd.fchmod(mode);
    }

    fn setNonblocking(*_, nonblocking: bool): !void {
        return _.fd.setNonblocking(nonblocking);
    }

    fn isTty(*_): bool {
        return _.fd.isTty();
    }

    fn getPath(*_, buf: []u8): ![]const u8 {
        return _.fd.getFdPath(buf);
    }

    fn dup(*_): !File {
        const new_fd = try _.fd.dup();
        return File{ fd: new_fd };
    }

    fn stat(*_): !Stat {
        return fstat(_.fd);
    }

    fn size(*_): !u64 {
        const st = try fstat(_.fd);
        return st.size;
    }

    fn readToEndAlloc(*_, allocator: std.mem.Allocator): ![]u8 {
        const s = try _.size();
        const buf = try allocator.alloc(u8, s);
        errdefer allocator.free(buf);
        let total: usize = 0;
        while (total < s) {
            const bytes_read = try _.pread(buf[total..buf.len], total as u64);
            if (bytes_read == 0) break;
            total += bytes_read;
        }
        if (total == s) return buf;
        const shrunk = allocator.realloc(buf, total) catch return buf[0..total];
        return shrunk;
    }
}
