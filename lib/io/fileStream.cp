// Source: Absorbed from /data/dev/bun/src/io/openForWriting.cp | Synced: 2026-11-20T11:41:00Z
/**
 * CP Streaming File Reader / Writer (cap:io)
 * 支持偏移 pread/pwrite、零拷贝 copy_file_range/sendfile 直通、openForWriting 策略
 */

const std = @import("std");
import { IOError, SeekOrigin, Reader, Writer, Closer, Seeker } from "./traits";
import {
    FD,
    AT_FDCWD,
    O_RDONLY,
    O_WRONLY,
    O_CREAT,
    O_TRUNC,
    O_APPEND,
    O_CLOEXEC,
    O_NONBLOCK,
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
} from "../sys/fd";
import { fstat } from "../sys/fstat";
import { openat, copyFileRange, sendfile } from "../sys/ioOps";

pub class FileReader implements Reader, Seeker, Closer {
    fd: FD;
    owns_fd: bool;
    offset: u64;
    closed: bool;

    pub fn init(fd: FD, owns_fd: bool) {
        _.fd = fd;
        _.owns_fd = owns_fd;
        _.offset = 0;
        _.closed = !fd.isValid();
    }

    pub static fn open(path: []const u8): FileReader! {
        const fd = try openat(FD.fromNative(AT_FDCWD), path, O_RDONLY | O_CLOEXEC, 0);
        return new FileReader(fd, true);
    }

    pub static fn fromFd(fd: FD): FileReader {
        return new FileReader(fd, false);
    }

    pub static fn fromFdOwned(fd: FD): FileReader {
        return new FileReader(fd, true);
    }

    pub fn read(*_, buf: *[]u8): usize! {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        if (buf.len == 0) return 0;
        const n = _.fd.pread(buf, _.offset) catch {
            return err.EndOfStream;
        };
        _.offset += n as u64;
        return n;
    }

    pub fn pread(*_, buf: *[]u8, pos: u64): usize! {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        if (buf.len == 0) return 0;
        return _.fd.pread(buf, pos) catch {
            return err.EndOfStream;
        };
    }

    pub fn seek(*_, offset: i64, origin: SeekOrigin): u64! {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        let whence: i32 = SEEK_SET;
        match (origin) {
            SeekOrigin.Start => { whence = SEEK_SET; },
            SeekOrigin.Current => { whence = SEEK_CUR; },
            SeekOrigin.End => { whence = SEEK_END; },
        }
        const pos = _.fd.lseek(offset, whence) catch {
            return err.InvalidSeek;
        };
        if (pos < 0) return err.InvalidSeek;
        _.offset = @intCast(pos);
        return _.offset;
    }

    pub fn size(): u64! {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        const st = fstat(_.fd) catch {
            return err.InvalidSeek;
        };
        return st.size;
    }

    pub fn remaining(): u64! {
        const total = try _.size();
        if (_.offset >= total) return 0;
        return total - _.offset;
    }

    pub fn position(): u64 {
        return _.offset;
    }

    pub fn readToEnd(*_, allocator: std.mem.Allocator): ([]u8)! {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        let list: std.ArrayListUnmanaged(u8) = .empty;
        errdefer list.deinit(allocator);

        let chunk: [8192]u8 = undefined;
        while (true) {
            const n = try _.read(chunk[0..chunk.len]);
            if (n == 0) break;
            try list.appendSlice(allocator, chunk[0..n]);
        }
        return try list.toOwnedSlice(allocator);
    }

    pub fn copyFileRangeTo(*_, dest_fd: FD, len: usize): usize! {
        if (_.closed || !_.fd.isValid() || !dest_fd.isValid()) return err.Closed;
        let src_off: u64 = _.offset;
        const n = copyFileRange(_.fd, &src_off, dest_fd, null, len) catch {
            return err.ShortWrite;
        };
        _.offset = src_off;
        return n;
    }

    pub fn sendfileTo(*_, dest_fd: FD, len: usize): usize! {
        if (_.closed || !_.fd.isValid() || !dest_fd.isValid()) return err.Closed;
        let src_off: i64 = @intCast(_.offset);
        const n = sendfile(_.fd, dest_fd, &src_off, len) catch {
            return err.ShortWrite;
        };
        _.offset = @intCast(src_off);
        return n;
    }

    pub fn getFd(): FD {
        return _.fd;
    }

    pub fn isClosed(): bool {
        return _.closed;
    }

    pub fn close(*_) {
        if (!_.closed) {
            if (_.owns_fd && _.fd.isValid()) {
                _.fd.close();
            }
            _.closed = true;
            _.fd = FD.invalid;
        }
    }
}

pub class FileWriter implements Writer, Seeker, Closer {
    fd: FD;
    owns_fd: bool;
    closed: bool;

    pub fn init(fd: FD, owns_fd: bool) {
        _.fd = fd;
        _.owns_fd = owns_fd;
        _.closed = !fd.isValid();
    }

    pub static fn open(path: []const u8, flags: i32, mode: u32): FileWriter! {
        const fd = try openat(FD.fromNative(AT_FDCWD), path, flags | O_CLOEXEC, mode);
        return new FileWriter(fd, true);
    }

    pub static fn create(path: []const u8): FileWriter! {
        return FileWriter.open(path, O_WRONLY | O_CREAT | O_TRUNC, 0o644);
    }

    pub static fn openAppend(path: []const u8): FileWriter! {
        return FileWriter.open(path, O_WRONLY | O_CREAT | O_APPEND, 0o644);
    }

    /// 移植自 Bun openForWriting：打开可写目标文件并探测是否支持非阻塞写
    pub static fn openForWriting(path: []const u8, mode: u32, force_sync: bool, out_nonblocking: *bool): FileWriter! {
        const fd = try openat(FD.fromNative(AT_FDCWD), path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, mode);
        const st = fstat(fd) catch {
            fd.close();
            return err.Closed;
        };
        const kind = st.kind();
        const is_tty = fd.isTty();
        const pollable = is_tty || kind == .named_pipe || kind == .unix_domain_socket;
        if (force_sync || is_tty || !pollable) {
            * = fd.setNonblocking(false) catch {};
            out_nonblocking.* = false;
        } else {
            let nb = fd.isNonblocking() catch false;
            if (!nb) {
                fd.setNonblocking(true) catch {};
                nb = fd.isNonblocking() catch false;
            }
            out_nonblocking.* = nb && pollable;
        }
        return new FileWriter(fd, true);
    }

    pub static fn fromFd(fd: FD): FileWriter {
        return new FileWriter(fd, false);
    }

    pub static fn fromFdOwned(fd: FD): FileWriter {
        return new FileWriter(fd, true);
    }

    pub fn write(*_, data: []const u8): usize! {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        if (data.len == 0) return 0;
        return _.fd.write(data) catch {
            return err.ShortWrite;
        };
    }

    pub fn pwrite(*_, data: []const u8, pos: u64): usize! {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        if (data.len == 0) return 0;
        return _.fd.pwrite(data, pos) catch {
            return err.ShortWrite;
        };
    }

    pub fn writeAll(*_, data: []const u8): void! {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        _.fd.writeAll(data) catch {
            return err.ShortWrite;
        };
    }

    pub fn seek(*_, offset: i64, origin: SeekOrigin): u64! {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        let whence: i32 = SEEK_SET;
        match (origin) {
            SeekOrigin.Start => { whence = SEEK_SET; },
            SeekOrigin.Current => { whence = SEEK_CUR; },
            SeekOrigin.End => { whence = SEEK_END; },
        }
        const pos = _.fd.lseek(offset, whence) catch {
            return err.InvalidSeek;
        };
        if (pos < 0) return err.InvalidSeek;
        return @intCast(pos);
    }

    pub fn sync(*_): !void {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        try _.fd.sync();
    }

    pub fn fdatasync(*_): !void {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        try _.fd.fdatasync();
    }

    pub fn ftruncate(*_, len: u64): !void {
        if (_.closed || !_.fd.isValid()) return err.Closed;
        try _.fd.ftruncate(len);
    }

    pub fn getFd(): FD {
        return _.fd;
    }

    pub fn isClosed(): bool {
        return _.closed;
    }

    pub fn close(*_) {
        if (!_.closed) {
            if (_.owns_fd && _.fd.isValid()) {
                _.fd.close();
            }
            _.closed = true;
            _.fd = FD.invalid;
        }
    }
}
