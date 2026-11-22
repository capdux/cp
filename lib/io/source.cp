// Source: Absorbed from /data/dev/bun/src/io/source.cp | Synced: 2026-11-22T12:49:17Z
/**
 * CP I/O Source, FileType & Non-blocking Poll State (cap:io)
 * 对应 Bun src/io/source.zig 与 src/io/pipes.zig 中的流源探测与 Poll 抽象
 */

const std = @import("std");
import { FD, O_NONBLOCK } from "../sys/fd";
import { fstatMaybe, FileKind } from "../sys/fstat";
import { PollFd, POLLIN, POLLOUT, POLLERR, POLLHUP, poll } from "../sys/ioOps";

pub enum IoStatus: u8 {
    Ok,
    WouldBlock,
    EndOfFile,
    BrokenPipe,
    Closed,
    BufferFull,
    Error,
}

pub enum ReadState: u8 {
    Ready,
    Eof,
    Again,
    Error,
}

pub enum WriteStatus: u8 {
    EndOfFile,
    Drained,
    Pending,
}

pub enum PollState: u8 {
    Ready,
    NotReady,
    Hup,
    Error,
}

pub enum FileType: u8 {
    File,
    Pipe,
    NonblockingPipe,
    Socket,
    Tty,

    pub fn isBlocking(): bool {
        return _ == FileType.Pipe || _ == FileType.Tty;
    }

    pub fn isPollable(): bool {
        return _ != FileType.File;
    }
}

pub struct PollResult {
    readable: bool = false,
    writable: bool = false,
    hup: bool = false,
    err: bool = false,
    revents: i16 = 0,
}

pub class IoSource {
    raw_fd: FD;
    file_type: FileType;
    nonblocking: bool;
    closed: bool;

    pub fn init(fd_in: i32) {
        _.raw_fd = FD.fromNative(fd_in);
        _.closed = fd_in < 0;
        _.nonblocking = if (fd_in >= 0) (_.raw_fd.isNonblocking() catch false) else false;
        _.file_type = IoSource.detectFileType(fd_in);
    }

    pub static fn fromFd(fd_in: FD): IoSource {
        return new IoSource(fd_in.raw);
    }

    pub static fn detectFileType(fd_num: i32): FileType {
        if (fd_num < 0) return FileType.File;
        const fd_obj = FD.fromNative(fd_num);
        if (fd_obj.isTty()) {
            return FileType.Tty;
        }
        const st_m = fstatMaybe(fd_obj);
        if (st_m.isOk()) {
            const st = st_m.asValue().?;
            const k = st.kind();
            if (k == FileKind.unix_domain_socket) {
                return FileType.Socket;
            }
            if (k == FileKind.named_pipe) {
                if (fd_obj.isNonblocking() catch false) {
                    return FileType.NonblockingPipe;
                }
                return FileType.Pipe;
            }
        }
        return FileType.File;
    }

    pub fn setNonblocking(*_, enable: bool): !void {
        if (_.closed || !_.raw_fd.isValid()) return error.Closed;
        try _.raw_fd.setNonblocking(enable);
        _.nonblocking = enable;
        if (_.file_type == FileType.Pipe && enable) {
            _.file_type = FileType.NonblockingPipe;
        } else if (_.file_type == FileType.NonblockingPipe && !enable) {
            _.file_type = FileType.Pipe;
        }
    }

    pub fn isNonblocking(): bool {
        return _.nonblocking;
    }

    pub fn getFileType(): FileType {
        return _.file_type;
    }

    pub fn getFd(): i32 {
        return _.raw_fd.raw;
    }

    pub fn asFd(): FD {
        return _.raw_fd;
    }

    pub fn isClosed(): bool {
        return _.closed;
    }

    pub fn pollReadable(*_, timeout_ms: i32): PollResult {
        if (_.closed || !_.raw_fd.isValid()) {
            return PollResult{ err: true };
        }
        let pfds: [1]PollFd = [PollFd{ .fd = _.raw_fd.raw, .events = POLLIN, .revents = 0 }];
        const n = poll(&pfds, timeout_ms) catch {
            return PollResult{ err: true };
        };
        if (n == 0) {
            return PollResult{};
        }
        const rev = pfds[0].revents;
        return PollResult{
            readable: (rev & POLLIN) != 0,
            hup: (rev & POLLHUP) != 0,
            err: (rev & POLLERR) != 0,
            revents: rev,
        };
    }

    pub fn pollWritable(*_, timeout_ms: i32): PollResult {
        if (_.closed || !_.raw_fd.isValid()) {
            return PollResult{ err: true };
        }
        let pfds: [1]PollFd = [PollFd{ .fd = _.raw_fd.raw, .events = POLLOUT, .revents = 0 }];
        const n = poll(&pfds, timeout_ms) catch {
            return PollResult{ err: true };
        };
        if (n == 0) {
            return PollResult{};
        }
        const rev = pfds[0].revents;
        return PollResult{
            writable: (rev & POLLOUT) != 0,
            hup: (rev & POLLHUP) != 0,
            err: (rev & POLLERR) != 0,
            revents: rev,
        };
    }

    pub fn isReadable(*_): PollState {
        const res = _.pollReadable(0);
        if (res.err) return PollState.Error;
        if (res.readable) return PollState.Ready;
        if (res.hup) return PollState.Hup;
        return PollState.NotReady;
    }

    pub fn isWritable(*_): PollState {
        const res = _.pollWritable(0);
        if (res.err) return PollState.Error;
        if (res.writable) return PollState.Ready;
        if (res.hup) return PollState.Hup;
        return PollState.NotReady;
    }

    pub fn close(*_) {
        if (!_.closed && _.raw_fd.isValid()) {
            _.raw_fd.close();
            _.closed = true;
            _.raw_fd = FD.invalid;
        }
    }
}
