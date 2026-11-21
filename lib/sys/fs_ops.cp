// Source: /data/dev/bun/src/sys/sys.cp
// ============================================================================
// lib/sys/fs_ops.cp — POSIX / Linux File System & Descriptor Syscall Primitives
// ============================================================================

const std = @import("std");
import {
    FD,
    AT_FDCWD,
    AT_REMOVEDIR,
    AT_SYMLINK_NOFOLLOW,
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT,
    O_EXCL,
    O_TRUNC,
    O_APPEND,
    O_NONBLOCK,
    O_CLOEXEC,
    O_DIRECTORY,
    O_NOFOLLOW,
    flagsToLinuxO,
} from "./fd";
import { File } from "./file";
import { DirIterator } from "./dir";
import { SysError, SyscallTag, Maybe, linuxSyscallErrno, cErrno, EINTR, EEXIST, ENOTDIR, ENAMETOOLONG } from "./error";
import { openatMaybe, openat } from "./io_ops";

pub const RENAME_NOREPLACE: u32 = 1;
pub const RENAME_EXCHANGE: u32 = 2;

pub type EpollEvent = std.os.linux.epoll_event;

pub const EPOLL_CTL_ADD: u32 = 1;
pub const EPOLL_CTL_DEL: u32 = 2;
pub const EPOLL_CTL_MOD: u32 = 3;
pub const EPOLLIN: u32 = 0x001;
pub const EPOLLOUT: u32 = 0x004;
pub const EPOLLERR: u32 = 0x008;
pub const EPOLLHUP: u32 = 0x010;
pub const EPOLLRDHUP: u32 = 0x2000;
pub const EPOLLET: u32 = 0x80000000;

fn toCStr(path: []const u8, buf: []u8): ![*:0]const u8 {
    if (path.len >= buf.len) return error.NameTooLong;
    @memcpy(buf[0 .. path.len], path);
    buf[path.len] = 0;
    return buf[0 .. path.len :0].ptr;
}

pub fn openMaybe(path: []const u8, flags: i32, mode: u32): Maybe<FD> {
    return openatMaybe(FD.cwd(), path, flags, mode);
}

pub fn open(path: []const u8, flags: i32, mode: u32): !FD {
    return openat(FD.cwd(), path, flags, mode);
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

pub fn mkdiratMaybe(dir_fd: FD, path: []const u8, mode: u32): Maybe<void> {
    let path_buf: [4096]u8 = undefined;
    if (path.len >= path_buf.len) {
        return Maybe<void>.initErr(SysError.fromCode(ENAMETOOLONG, .mkdirat).withPath(path));
    }
    @memcpy(path_buf[0 .. path.len], path);
    path_buf[path.len] = 0;
    const path_z: [*:0]const u8 = path_buf[0 .. path.len :0].ptr;
    const base_raw = if (dir_fd.isValid()) dir_fd.raw else AT_FDCWD;

    while (true) {
        const rc = std.os.linux.mkdirat(base_raw, path_z, @as(std.os.linux.mode_t, @intCast(mode)));
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) return Maybe<void>.success();
        if (err_code == EINTR) continue;
        return Maybe<void>.initErr(SysError.fromCode(err_code, .mkdirat).withPath(path));
    }
}

pub fn mkdirat(dir_fd: FD, path: []const u8, mode: u32): !void {
    return mkdiratMaybe(dir_fd, path, mode).unwrap();
}

pub fn mkdir(path: []const u8, mode: u32): !void {
    return mkdirat(FD.cwd(), path, mode);
}

pub fn mkdirAllAt(dir_fd: FD, path: []const u8, mode: u32): !void {
    if (path.len == 0) return;
    let buf: [4096]u8 = undefined;
    if (path.len >= buf.len) return error.NameTooLong;
    let i: usize = 0;
    if (path[0] == '/') i = 1;
    while (i <= path.len) {
        if (i == path.len || path[i] == '/') {
            if (i > 0) {
                const sub = path[0..i];
                const res = mkdiratMaybe(dir_fd, sub, mode);
                if (e := res.asErr()) {
                    if (e.errno != EEXIST) {
                        return e.toError();
                    }
                }
            }
        }
        i += 1;
    }
}

pub fn mkdirAll(path: []const u8, mode: u32): !void {
    return mkdirAllAt(FD.cwd(), path, mode);
}

pub fn unlinkatMaybe(dir_fd: FD, path: []const u8, flags: u32): Maybe<void> {
    let path_buf: [4096]u8 = undefined;
    if (path.len >= path_buf.len) {
        return Maybe<void>.initErr(SysError.fromCode(ENAMETOOLONG, .unlinkat).withPath(path));
    }
    @memcpy(path_buf[0 .. path.len], path);
    path_buf[path.len] = 0;
    const path_z: [*:0]const u8 = path_buf[0 .. path.len :0].ptr;
    const base_raw = if (dir_fd.isValid()) dir_fd.raw else AT_FDCWD;

    while (true) {
        const rc = std.os.linux.unlinkat(base_raw, path_z, flags);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) return Maybe<void>.success();
        if (err_code == EINTR) continue;
        return Maybe<void>.initErr(SysError.fromCode(err_code, .unlinkat).withPath(path));
    }
}

pub fn unlinkat(dir_fd: FD, path: []const u8, flags: u32): !void {
    return unlinkatMaybe(dir_fd, path, flags).unwrap();
}

pub fn rmdirat(dir_fd: FD, path: []const u8): !void {
    return unlinkat(dir_fd, path, AT_REMOVEDIR);
}

pub fn rmdir(path: []const u8): !void {
    return rmdirat(FD.cwd(), path);
}

pub fn unlink(path: []const u8): !void {
    return unlinkat(FD.cwd(), path, 0);
}

pub fn rmdirRecursiveAt(dir_fd: FD, path: []const u8): !void {
    let iter = try DirIterator.openAt(dir_fd, path);
    let closed = false;
    defer {
        if (!closed) iter.close();
    }

    while (entry := try iter.next()) {
        if (entry.kind == .directory) {
            try rmdirRecursiveAt(iter.dir_fd, entry.name);
        } else {
            // Note: entry.name is a slice into iter.buf; unlinkat copies it to stack before any next().
            try unlinkat(iter.dir_fd, entry.name, 0);
        }
    }
    iter.close();
    closed = true;
    try rmdirat(dir_fd, path);
}

pub fn rmdirRecursive(path: []const u8): !void {
    return rmdirRecursiveAt(FD.cwd(), path);
}

pub fn renameat2Maybe(old_dir: FD, old_path: []const u8, new_dir: FD, new_path: []const u8, flags: u32): Maybe<void> {
    let old_buf: [4096]u8 = undefined;
    let new_buf: [4096]u8 = undefined;
    if (old_path.len >= old_buf.len || new_path.len >= new_buf.len) {
        return Maybe<void>.initErr(SysError.fromCode(ENAMETOOLONG, .renameat2).withPathDest(old_path, new_path));
    }
    @memcpy(old_buf[0 .. old_path.len], old_path);
    old_buf[old_path.len] = 0;
    @memcpy(new_buf[0 .. new_path.len], new_path);
    new_buf[new_path.len] = 0;

    const old_z: [*:0]const u8 = old_buf[0 .. old_path.len :0].ptr;
    const new_z: [*:0]const u8 = new_buf[0 .. new_path.len :0].ptr;
    const old_raw = if (old_dir.isValid()) old_dir.raw else AT_FDCWD;
    const new_raw = if (new_dir.isValid()) new_dir.raw else AT_FDCWD;

    while (true) {
        const rflags: std.os.linux.RENAME = @bitCast(flags);
        const rc = std.os.linux.renameat2(old_raw, old_z, new_raw, new_z, rflags);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) return Maybe<void>.success();
        if (err_code == EINTR) continue;
        return Maybe<void>.initErr(SysError.fromCode(err_code, .renameat2).withPathDest(old_path, new_path));
    }
}

pub fn renameat2(old_dir: FD, old_path: []const u8, new_dir: FD, new_path: []const u8, flags: u32): !void {
    return renameat2Maybe(old_dir, old_path, new_dir, new_path, flags).unwrap();
}

pub fn renameat(old_dir: FD, old_path: []const u8, new_dir: FD, new_path: []const u8): !void {
    return renameat2(old_dir, old_path, new_dir, new_path, 0);
}

pub fn rename(old_path: []const u8, new_path: []const u8): !void {
    return renameat(FD.cwd(), old_path, FD.cwd(), new_path);
}

pub fn symlinkat(target: []const u8, dir_fd: FD, link_path: []const u8): !void {
    let tgt_buf: [4096]u8 = undefined;
    let lnk_buf: [4096]u8 = undefined;
    const tgt_z = try toCStr(target, &tgt_buf);
    const lnk_z = try toCStr(link_path, &lnk_buf);
    const base_raw = if (dir_fd.isValid()) dir_fd.raw else AT_FDCWD;
    while (true) {
        const rc = std.os.linux.symlinkat(tgt_z, base_raw, lnk_z);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) return;
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .symlinkat).withPathDest(target, link_path).toError();
    }
}

pub fn symlink(target: []const u8, link_path: []const u8): !void {
    return symlinkat(target, FD.cwd(), link_path);
}

pub fn readlinkat(dir_fd: FD, path: []const u8, buf: []u8): ![]const u8 {
    if (buf.len == 0) return error.BufferTooSmall;
    let path_buf: [4096]u8 = undefined;
    const path_z = try toCStr(path, &path_buf);
    const base_raw = if (dir_fd.isValid()) dir_fd.raw else AT_FDCWD;
    while (true) {
        const rc = std.os.linux.readlinkat(base_raw, path_z, buf.ptr, buf.len);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) {
            return buf[0..rc];
        }
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .readlinkat).withPath(path).toError();
    }
}

pub fn readlink(path: []const u8, buf: []u8): ![]const u8 {
    return readlinkat(FD.cwd(), path, buf);
}

pub fn linkat(src_dir: FD, src_path: []const u8, dest_dir: FD, dest_path: []const u8, flags: u32): !void {
    let src_buf: [4096]u8 = undefined;
    let dst_buf: [4096]u8 = undefined;
    const src_z = try toCStr(src_path, &src_buf);
    const dst_z = try toCStr(dest_path, &dst_buf);
    const s_raw = if (src_dir.isValid()) src_dir.raw else AT_FDCWD;
    const d_raw = if (dest_dir.isValid()) dest_dir.raw else AT_FDCWD;
    while (true) {
        const rc = std.os.linux.linkat(s_raw, src_z, d_raw, dst_z, flags);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) return;
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .linkat).withPathDest(src_path, dest_path).toError();
    }
}

pub fn link(src_path: []const u8, dest_path: []const u8): !void {
    return linkat(FD.cwd(), src_path, FD.cwd(), dest_path, 0);
}

pub fn chmod(path: []const u8, mode: u32): !void {
    let path_buf: [4096]u8 = undefined;
    const path_z = try toCStr(path, &path_buf);
    while (true) {
        if (std.c.chmod(path_z, @as(std.c.mode_t, @intCast(mode))) == 0) return;
        const err_code = cErrno();
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .chmod).withPath(path).toError();
    }
}

pub fn fchmod(fd: FD, mode: u32): !void {
    return fd.fchmod(mode);
}

pub fn chown(path: []const u8, uid: u32, gid: u32): !void {
    let path_buf: [4096]u8 = undefined;
    const path_z = try toCStr(path, &path_buf);
    while (true) {
        if (std.c.chown(path_z, @intCast(uid), @intCast(gid)) == 0) return;
        const err_code = cErrno();
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .chown).withPath(path).toError();
    }
}

pub fn fchown(fd: FD, uid: u32, gid: u32): !void {
    return fd.fchown(uid, gid);
}

pub fn fsync(fd: FD): !void {
    return fd.sync();
}

pub fn fdatasync(fd: FD): !void {
    return fd.fdatasync();
}

pub fn flock(fd: FD, operation: i32): !void {
    return fd.flock(operation);
}

pub fn fcntl(fd: FD, cmd: i32, arg: usize): !usize {
    return fd.fcntl(cmd, arg);
}

pub fn ioctl(fd: FD, request: u32, arg: usize): !i32 {
    if (!fd.isValid()) return error.BadFileDescriptor;
    while (true) {
        const rc = std.os.linux.ioctl(fd.raw, request, arg);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) return @intCast(rc);
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .ioctl).withFd(fd.raw).toError();
    }
}

pub fn pipe2(flags: i32): ![2]FD {
    let fds: [2]i32 = undefined;
    const o_flags = flagsToLinuxO(flags);
    while (true) {
        const rc = std.os.linux.pipe2(&fds, o_flags);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) {
            return [FD.fromNative(fds[0]), FD.fromNative(fds[1])];
        }
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .pipe2).toError();
    }
}

pub fn createPipe(): ![2]FD {
    return pipe2(O_CLOEXEC);
}

pub fn epollCreate(flags: i32): !FD {
    const rc = std.os.linux.epoll_create1(@intCast(flags));
    const err_code = linuxSyscallErrno(rc);
    if (err_code == 0) return FD.fromNative(@intCast(rc));
    return SysError.fromCode(err_code, .epoll_create1).toError();
}

pub fn epollCtl(epfd: FD, op: u32, fd: FD, ev: (*EpollEvent)?): !void {
    if (!epfd.isValid() || !fd.isValid()) return error.BadFileDescriptor;
    const rc = std.os.linux.epoll_ctl(epfd.raw, op, fd.raw, ev);
    const err_code = linuxSyscallErrno(rc);
    if (err_code == 0) return;
    return SysError.fromCode(err_code, .epoll_ctl).withFd(fd.raw).toError();
}

pub fn epollWait(epfd: FD, events: []EpollEvent, timeout_ms: i32): !usize {
    if (!epfd.isValid()) return error.BadFileDescriptor;
    if (events.len == 0) return 0;
    while (true) {
        const rc = std.os.linux.epoll_wait(epfd.raw, events.ptr, @intCast(events.len), timeout_ms);
        const err_code = linuxSyscallErrno(rc);
        if (err_code == 0) return rc;
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .epoll_wait).withFd(epfd.raw).toError();
    }
}

pub fn access(path: []const u8, mode: i32): !void {
    let path_buf: [4096]u8 = undefined;
    const path_z = try toCStr(path, &path_buf);
    while (true) {
        if (std.c.access(path_z, @intCast(mode)) == 0) return;
        const err_code = cErrno();
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .access).withPath(path).toError();
    }
}

pub fn exists(path: []const u8): bool {
    access(path, 0) catch return false;
    return true;
}

pub fn getcwd(buf: []u8): ![]const u8 {
    if (buf.len == 0) return error.BufferTooSmall;
    const ptr = std.c.getcwd(buf.ptr, buf.len);
    if (ptr == null) return SysError.fromCode(cErrno(), .getcwd).toError();
    const c_ptr: [*:0]const u8 = unsafe { @ptrCast(buf.ptr) };
    return std.mem.span(c_ptr);
}

pub fn chdir(path: []const u8): !void {
    let path_buf: [4096]u8 = undefined;
    const path_z = try toCStr(path, &path_buf);
    while (true) {
        if (std.c.chdir(path_z) == 0) return;
        const err_code = cErrno();
        if (err_code == EINTR) continue;
        return SysError.fromCode(err_code, .chdir).withPath(path).toError();
    }
}

pub fn readFile(allocator: std.mem.Allocator, path: []const u8): ![]u8 {
    let f = try File.openReadOnly(path);
    defer f.close();
    return f.readToEndAlloc(allocator);
}

pub fn writeFile(path: []const u8, data: []const u8, mode: u32): !void {
    let f = try File.open(path, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, mode);
    defer f.close();
    try f.writeAll(data);
}
