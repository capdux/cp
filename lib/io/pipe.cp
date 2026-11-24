// Source: Absorbed from /data/dev/bun/src/io/PipeReader.cp | Synced: 2026-11-23T21:45:49Z
/**
 * CP Native OS Pipe (cap:io)
 * 生产级非阻塞/阻塞管道状态机（支持 pipe2、EAGAIN 感知、部分写续传、BrokenPipe 与 MaxBuf 守卫）
 */

const std = @import("std");
import { Reader, Writer, Closer } from "./traits";
import { IoStatus, PollResult } from "./source";
import { MaxBuf } from "./maxBuf";
import { FD, O_CLOEXEC, O_NONBLOCK, flagsToLinuxO } from "../sys/fd";
import { EINTR, EAGAIN, EWOULDBLOCK, EPIPE, cErrno, linuxSyscallErrno } from "../sys/error";
import { PollFd, POLLIN, POLLOUT, POLLERR, POLLHUP, poll } from "../sys/ioOps";

pub struct PipeReadResult {
    status: IoStatus = IoStatus.Ok,
    bytes: usize = 0,
    errno: i32 = 0,
}

pub struct PipeWriteResult {
    status: IoStatus = IoStatus.Ok,
    bytes: usize = 0,
    errno: i32 = 0,
}

/**
 * 原生非阻塞/阻塞管道读取器
 */
pub class PipeReader implements Reader, Closer {
    fd: i32;
    closed: bool;
    nonblocking: bool;
    eof_reached: bool;
    total_read: usize;
    max_buf: (*MaxBuf)?;

    pub fn init(fd: i32) {
        _.fd = fd;
        _.closed = fd < 0;
        _.nonblocking = if (fd >= 0) (FD.fromNative(fd).isNonblocking() catch false) else false;
        _.eof_reached = false;
        _.total_read = 0;
        _.max_buf = null;
    }

    pub static fn createNonblocking(fd: i32): PipeReader {
        let r = new PipeReader(fd);
        if (fd >= 0 && !r.nonblocking) {
            * = r.setNonblocking(true) catch {};
        }
        return r;
    }

    pub fn setNonblocking(*_, enable: bool): !void {
        if (_.closed || _.fd < 0) return err.Closed;
        try FD.fromNative(_.fd).setNonblocking(enable);
        _.nonblocking = enable;
    }

    pub fn isNonblocking(): bool {
        return _.nonblocking;
    }

    pub fn isEof(): bool {
        return _.eof_reached;
    }

    pub fn bytesRead(): usize {
        return _.total_read;
    }

    pub fn attachMaxBuf(*_, mb: *MaxBuf) {
        _.max_buf = mb;
    }

    pub fn detachMaxBuf(*_) {
        _.max_buf = null;
    }

    /// 非阻塞/阻塞感知状态机单次读取（自动 EINTR 重试、EAGAIN -> WouldBlock、EOF 判定）
    pub fn attemptRead(*_, buf: *[]u8): PipeReadResult {
        if (_.closed || _.fd < 0) {
            return PipeReadResult{ status: IoStatus.Closed, bytes: 0, errno: 9 };
        }
        if (_.eof_reached) {
            return PipeReadResult{ status: IoStatus.EndOfFile, bytes: 0, errno: 0 };
        }
        if (buf.len == 0) {
            return PipeReadResult{ status: IoStatus.Ok, bytes: 0, errno: 0 };
        }

        while (true) {
            const rc = std.c.read(_.fd, buf.ptr, buf.len);
            if (rc > 0) {
                const n: usize = @intCast(rc);
                _.total_read += n;
                if (mb := _.max_buf) {
                    if (mb.onReadBytesStatus(n) == IoStatus.BufferFull) {
                        return PipeReadResult{ status: IoStatus.BufferFull, bytes: n, errno: 0 };
                    }
                }
                return PipeReadResult{ status: IoStatus.Ok, bytes: n, errno: 0 };
            }
            if (rc == 0) {
                _.eof_reached = true;
                return PipeReadResult{ status: IoStatus.EndOfFile, bytes: 0, errno: 0 };
            }
            const err_code = cErrno();
            if (err_code == EINTR) continue;
            if (err_code == EAGAIN || err_code == EWOULDBLOCK) {
                return PipeReadResult{ status: IoStatus.WouldBlock, bytes: 0, errno: err_code };
            }
            return PipeReadResult{ status: IoStatus.Error, bytes: 0, errno: err_code };
        }
    }

    pub fn read(*_, buf: *[]u8): usize! {
        if (_.closed || _.fd < 0) {
            return err.Closed;
        }
        if (buf.len == 0) {
            return 0;
        }
        const res = _.attemptRead(buf);
        match (res.status) {
            IoStatus.Ok => return res.bytes,
            IoStatus.EndOfFile => return 0,
            IoStatus.WouldBlock => return err.WouldBlock,
            IoStatus.BufferFull => return err.BufferFull,
            IoStatus.Closed => return err.Closed,
            else => return err.EndOfStream,
        }
    }

    pub fn pollReadable(*_, timeout_ms: i32): PollResult {
        if (_.closed || _.fd < 0) return PollResult{ err: true };
        let pfds: [1]PollFd = [PollFd{ .fd = _.fd, .events = POLLIN, .revents = 0 }];
        const n = poll(&pfds, timeout_ms) catch {
            return PollResult{ err: true };
        };
        if (n == 0) return PollResult{};
        const rev = pfds[0].revents;
        return PollResult{
            readable: (rev & POLLIN) != 0,
            hup: (rev & POLLHUP) != 0,
            err: (rev & POLLERR) != 0,
            revents: rev,
        };
    }

    pub fn readToEnd(*_, allocator: std.mem.Allocator): ([]u8)! {
        if (_.closed || _.fd < 0) return err.Closed;
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        let chunk: [4096]u8 = undefined;
        while (true) {
            const res = _.attemptRead(chunk[0..chunk.len]);
            match (res.status) {
                IoStatus.Ok => {
                    if (res.bytes > 0) {
                        try out.appendSlice(allocator, chunk[0..res.bytes]);
                    }
                },
                IoStatus.EndOfFile => break,
                IoStatus.WouldBlock => {
                    const pr = _.pollReadable(5000);
                    if (pr.err) return err.EndOfStream;
                    if (!pr.readable && pr.hup) {
                        // Drain remaining or hit EOF on next read
                        continue;
                    }
                },
                IoStatus.BufferFull => return err.BufferFull,
                IoStatus.Closed => return err.Closed,
                else => return err.EndOfStream,
            }
        }
        return try out.toOwnedSlice(allocator);
    }

    pub fn close(*_) {
        if (!_.closed && _.fd >= 0) {
            * = std.c.close(_.fd);
            _.closed = true;
            _.fd = -1;
        }
    }

    pub fn getFd(): i32 {
        return _.fd;
    }

    pub fn isClosed(): bool {
        return _.closed;
    }
}

/**
 * 原生非阻塞/阻塞管道写入器
 */
pub class PipeWriter implements Writer, Closer {
    fd: i32;
    closed: bool;
    nonblocking: bool;
    broken_pipe: bool;
    total_written: usize;

    pub fn init(fd: i32) {
        _.fd = fd;
        _.closed = fd < 0;
        _.nonblocking = if (fd >= 0) (FD.fromNative(fd).isNonblocking() catch false) else false;
        _.broken_pipe = false;
        _.total_written = 0;
    }

    pub static fn createNonblocking(fd: i32): PipeWriter {
        let w = new PipeWriter(fd);
        if (fd >= 0 && !w.nonblocking) {
            * = w.setNonblocking(true) catch {};
        }
        return w;
    }

    pub fn setNonblocking(*_, enable: bool): !void {
        if (_.closed || _.fd < 0) return err.Closed;
        try FD.fromNative(_.fd).setNonblocking(enable);
        _.nonblocking = enable;
    }

    pub fn isNonblocking(): bool {
        return _.nonblocking;
    }

    pub fn isBrokenPipe(): bool {
        return _.broken_pipe;
    }

    pub fn bytesWritten(): usize {
        return _.total_written;
    }

    /// 非阻塞/阻塞感知单次写入（支持部分写报告、EAGAIN -> WouldBlock、EPIPE -> BrokenPipe）
    pub fn attemptWrite(*_, data: []const u8): PipeWriteResult {
        if (_.closed || _.fd < 0) {
            return PipeWriteResult{ status: IoStatus.Closed, bytes: 0, errno: 9 };
        }
        if (_.broken_pipe) {
            return PipeWriteResult{ status: IoStatus.BrokenPipe, bytes: 0, errno: EPIPE };
        }
        if (data.len == 0) {
            return PipeWriteResult{ status: IoStatus.Ok, bytes: 0, errno: 0 };
        }

        let offset: usize = 0;
        while (offset < data.len) {
            // Use raw Linux write or C write; SIGPIPE on closed pipe returns EPIPE
            const rc = std.os.linux.write(_.fd, data[offset..].ptr, data.len - offset);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) {
                if (rc == 0) {
                    return PipeWriteResult{ status: IoStatus.Ok, bytes: offset, errno: 0 };
                }
                offset += rc;
                _.total_written += rc;
                // In nonblocking mode, try continuing until full or EAGAIN
                if (!_.nonblocking) break;
                continue;
            }
            if (err_code == EINTR) continue;
            if (err_code == EAGAIN || err_code == EWOULDBLOCK) {
                return PipeWriteResult{ status: IoStatus.WouldBlock, bytes: offset, errno: err_code };
            }
            if (err_code == EPIPE) {
                _.broken_pipe = true;
                return PipeWriteResult{ status: IoStatus.BrokenPipe, bytes: offset, errno: err_code };
            }
            return PipeWriteResult{ status: IoStatus.Error, bytes: offset, errno: err_code };
        }

        return PipeWriteResult{ status: IoStatus.Ok, bytes: offset, errno: 0 };
    }

    pub fn write(*_, data: []const u8): usize! {
        if (_.closed || _.fd < 0) {
            return err.Closed;
        }
        if (data.len == 0) {
            return 0;
        }
        const res = _.attemptWrite(data);
        match (res.status) {
            IoStatus.Ok => return res.bytes,
            IoStatus.WouldBlock => {
                if (res.bytes > 0) return res.bytes;
                return err.WouldBlock;
            },
            IoStatus.BrokenPipe => return err.BrokenPipe,
            IoStatus.Closed => return err.Closed,
            else => return err.ShortWrite,
        }
    }

    pub fn pollWritable(*_, timeout_ms: i32): PollResult {
        if (_.closed || _.fd < 0) return PollResult{ err: true };
        let pfds: [1]PollFd = [PollFd{ .fd = _.fd, .events = POLLOUT, .revents = 0 }];
        const n = poll(&pfds, timeout_ms) catch {
            return PollResult{ err: true };
        };
        if (n == 0) return PollResult{};
        const rev = pfds[0].revents;
        return PollResult{
            writable: (rev & POLLOUT) != 0,
            hup: (rev & POLLHUP) != 0,
            err: (rev & POLLERR) != 0,
            revents: rev,
        };
    }

    pub fn writeAll(*_, data: []const u8): usize! {
        if (_.closed || _.fd < 0) return err.Closed;
        let offset: usize = 0;
        while (offset < data.len) {
            const res = _.attemptWrite(data[offset..]);
            offset += res.bytes;
            match (res.status) {
                IoStatus.Ok => {
                    if (res.bytes == 0 && offset < data.len) {
                        return err.ShortWrite;
                    }
                },
                IoStatus.WouldBlock => {
                    const pr = _.pollWritable(5000);
                    if (pr.err) return err.ShortWrite;
                    if (!pr.writable && pr.hup) return err.BrokenPipe;
                },
                IoStatus.BrokenPipe => return err.BrokenPipe,
                IoStatus.Closed => return err.Closed,
                else => return err.ShortWrite,
            }
        }
        return offset;
    }

    pub fn close(*_) {
        if (!_.closed && _.fd >= 0) {
            * = std.c.close(_.fd);
            _.closed = true;
            _.fd = -1;
        }
    }

    pub fn getFd(): i32 {
        return _.fd;
    }

    pub fn isClosed(): bool {
        return _.closed;
    }
}

/**
 * 管道工厂：创建一对相互连通的 PipeReader 与 PipeWriter
 */
pub class Pipe {
    reader: PipeReader;
    writer: PipeWriter;

    pub fn init(reader: PipeReader, writer: PipeWriter) {
        _.reader = reader;
        _.writer = writer;
    }

    pub static fn create(): Pipe! {
        return Pipe.createWithFlags(O_CLOEXEC);
    }

    pub static fn createNonblocking(): Pipe! {
        return Pipe.createWithFlags(O_CLOEXEC | O_NONBLOCK);
    }

    pub static fn createWithFlags(flags: i32): Pipe! {
        let fds: [2]i32 = undefined;
        const o_flags = flagsToLinuxO(flags);
        while (true) {
            const rc = std.os.linux.pipe2(&fds, o_flags);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) {
                let r = new PipeReader(fds[0]);
                let w = new PipeWriter(fds[1]);
                if ((flags & O_NONBLOCK) != 0) {
                    r.nonblocking = true;
                    w.nonblocking = true;
                }
                return new Pipe(r, w);
            }
            if (err_code == EINTR) continue;
            return err.Closed;
        }
    }

    pub static fn fromFds(read_fd: i32, write_fd: i32): Pipe {
        let r = new PipeReader(read_fd);
        let w = new PipeWriter(write_fd);
        return new Pipe(r, w);
    }

    pub fn close(*_) {
        _.reader.close();
        _.writer.close();
    }
}
