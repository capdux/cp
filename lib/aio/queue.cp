// Source: Absorbed from /data/dev/bun/src/aio/posix_event_loop.zig | Synced: 2026-11-22T04:24:34Z
// lib/aio/queue.cp
// Zero-Heap Submission/Completion Ring Queue for Synchronous/Async-Style POSIX File I/O

const std = @import("std");

pub const AIO_O_RDONLY: i32 = 0;
pub const AIO_O_WRONLY: i32 = 1;
pub const AIO_O_RDWR: i32 = 2;
pub const AIO_O_CREAT: i32 = 64;
pub const AIO_O_EXCL: i32 = 128;
pub const AIO_O_TRUNC: i32 = 512;
pub const AIO_O_APPEND: i32 = 1024;
pub const AIO_O_NONBLOCK: i32 = 2048;
pub const AIO_O_CLOEXEC: i32 = 524288;

const ECANCELED_CODE: i32 = 125;
const EINVAL_CODE: i32 = 22;
const ENAMETOOLONG_CODE: i32 = 36;

pub enum AioOpKind: u8 {
    Nop = 0,
    Open = 1,
    Read = 2,
    Write = 3,
    Pread = 4,
    Pwrite = 5,
    Stat = 6,
    Fsync = 7,
    Close = 8,
    Unlink = 9,
    Mkdir = 10,
}

pub enum AioState: u8 {
    Pending = 0,
    Submitted = 1,
    Completed = 2,
    Cancelled = 3,
    Failed = 4,
}

pub struct AioOp {
    id: u64 = 0,
    kind: AioOpKind = AioOpKind.Nop,
    state: AioState = AioState.Pending,
    fd: i32 = -1,
    path: []const u8 = "",
    flags: i32 = 0,
    mode: u32 = 0o644,
    buf: ([]u8)? = null,
    data: ([]const u8)? = null,
    offset: u64 = 0,
    user_data: usize = 0,

    pub static fn nop(user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Nop,
            user_data: user_data,
        };
    }

    pub static fn open(path: []const u8, flags: i32, mode: u32 = 0o644, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Open,
            path: path,
            flags: flags,
            mode: mode,
            user_data: user_data,
        };
    }

    pub static fn read(fd: i32, buf: []u8, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Read,
            fd: fd,
            buf: buf,
            user_data: user_data,
        };
    }

    pub static fn write(fd: i32, data: []const u8, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Write,
            fd: fd,
            data: data,
            user_data: user_data,
        };
    }

    pub static fn pread(fd: i32, buf: []u8, offset: u64, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Pread,
            fd: fd,
            buf: buf,
            offset: offset,
            user_data: user_data,
        };
    }

    pub static fn pwrite(fd: i32, data: []const u8, offset: u64, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Pwrite,
            fd: fd,
            data: data,
            offset: offset,
            user_data: user_data,
        };
    }

    pub static fn stat(path: []const u8, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Stat,
            path: path,
            user_data: user_data,
        };
    }

    pub static fn fsync(fd: i32, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Fsync,
            fd: fd,
            user_data: user_data,
        };
    }

    pub static fn close(fd: i32, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Close,
            fd: fd,
            user_data: user_data,
        };
    }

    pub static fn unlink(path: []const u8, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Unlink,
            path: path,
            user_data: user_data,
        };
    }

    pub static fn mkdir(path: []const u8, mode: u32 = 0o755, user_data: usize = 0): AioOp {
        return AioOp {
            kind: AioOpKind.Mkdir,
            path: path,
            mode: mode,
            user_data: user_data,
        };
    }
}

pub struct AioCompletion {
    id: u64 = 0,
    kind: AioOpKind = AioOpKind.Nop,
    state: AioState = AioState.Completed,
    bytes_transferred: usize = 0,
    result_fd: i32 = -1,
    errno_code: i32 = 0,
    stat_size: u64 = 0,
    stat_mode: u32 = 0,
    stat_is_dir: bool = false,
    user_data: usize = 0,

    pub fn isOk(): bool {
        return _.state == AioState.Completed && _.errno_code == 0;
    }

    pub fn isCancelled(): bool {
        return _.state == AioState.Cancelled;
    }

    pub fn isFailed(): bool {
        return _.state == AioState.Failed;
    }
}

fn currentCErrno(): i32 {
    const code = std.c._errno().*;
    if (code <= 0) return EINVAL_CODE;
    return code;
}

fn copyPathToZ(path: []const u8, buf: []u8): [*:0]const u8? {
    if (path.len == 0 || path.len >= buf.len) {
        return null;
    }
    @memcpy(buf[0..path.len], path);
    buf[path.len] = 0;
    return buf[0..path.len :0].ptr;
}

fn toCOpenFlags(flags: i32): std.c.O {
    let z_flags = std.c.O{
        .CREAT = (flags & AIO_O_CREAT) != 0,
        .EXCL = (flags & AIO_O_EXCL) != 0,
        .TRUNC = (flags & AIO_O_TRUNC) != 0,
        .APPEND = (flags & AIO_O_APPEND) != 0,
        .NONBLOCK = (flags & AIO_O_NONBLOCK) != 0,
        .CLOEXEC = (flags & AIO_O_CLOEXEC) != 0,
    };
    if ((flags & AIO_O_RDWR) == AIO_O_RDWR) {
        z_flags.ACCMODE = .RDWR;
    } else if ((flags & AIO_O_WRONLY) != 0) {
        z_flags.ACCMODE = .WRONLY;
    } else {
        z_flags.ACCMODE = .RDONLY;
    }
    return z_flags;
}

fn executeAioOp(op: AioOp): AioCompletion {
    let comp = AioCompletion {
        id: op.id,
        kind: op.kind,
        state: AioState.Completed,
        bytes_transferred: 0,
        result_fd: -1,
        errno_code: 0,
        stat_size: 0,
        stat_mode: 0,
        stat_is_dir: false,
        user_data: op.user_data,
    };

    if (op.state == AioState.Cancelled) {
        comp.state = AioState.Cancelled;
        comp.errno_code = ECANCELED_CODE;
        return comp;
    }

    switch (op.kind) {
        AioOpKind.Nop => {
            return comp;
        },
        AioOpKind.Open => {
            let path_buf: [1024]u8 = undefined;
            if (path_z := copyPathToZ(op.path, &path_buf)) {
                const z_flags = toCOpenFlags(op.flags);
                const raw_fd = std.c.open(path_z, z_flags, @as(std.c.mode_t, @intCast(op.mode)));
                if (raw_fd < 0) {
                    comp.state = AioState.Failed;
                    comp.errno_code = currentCErrno();
                } else {
                    comp.result_fd = raw_fd;
                }
            } else {
                comp.state = AioState.Failed;
                comp.errno_code = if (op.path.len == 0) EINVAL_CODE else ENAMETOOLONG_CODE;
            }
            return comp;
        },
        AioOpKind.Read => {
            if (buf := op.buf) {
                if (buf.len == 0) {
                    return comp;
                }
                const rc = std.c.read(op.fd, buf.ptr, buf.len);
                if (rc < 0) {
                    comp.state = AioState.Failed;
                    comp.errno_code = currentCErrno();
                } else {
                    comp.bytes_transferred = @intCast(rc);
                }
            } else {
                comp.state = AioState.Failed;
                comp.errno_code = EINVAL_CODE;
            }
            return comp;
        },
        AioOpKind.Write => {
            if (data := op.data) {
                if (data.len == 0) {
                    return comp;
                }
                const rc = std.c.write(op.fd, data.ptr, data.len);
                if (rc < 0) {
                    comp.state = AioState.Failed;
                    comp.errno_code = currentCErrno();
                } else {
                    comp.bytes_transferred = @intCast(rc);
                }
            } else {
                comp.state = AioState.Failed;
                comp.errno_code = EINVAL_CODE;
            }
            return comp;
        },
        AioOpKind.Pread => {
            if (buf := op.buf) {
                if (buf.len == 0) {
                    return comp;
                }
                const rc = std.c.pread(op.fd, buf.ptr, buf.len, @as(std.c.off_t, @intCast(op.offset)));
                if (rc < 0) {
                    comp.state = AioState.Failed;
                    comp.errno_code = currentCErrno();
                } else {
                    comp.bytes_transferred = @intCast(rc);
                }
            } else {
                comp.state = AioState.Failed;
                comp.errno_code = EINVAL_CODE;
            }
            return comp;
        },
        AioOpKind.Pwrite => {
            if (data := op.data) {
                if (data.len == 0) {
                    return comp;
                }
                const rc = std.c.pwrite(op.fd, data.ptr, data.len, @as(std.c.off_t, @intCast(op.offset)));
                if (rc < 0) {
                    comp.state = AioState.Failed;
                    comp.errno_code = currentCErrno();
                } else {
                    comp.bytes_transferred = @intCast(rc);
                }
            } else {
                comp.state = AioState.Failed;
                comp.errno_code = EINVAL_CODE;
            }
            return comp;
        },
        AioOpKind.Stat => {
            let path_buf: [1024]u8 = undefined;
            if (path_z := copyPathToZ(op.path, &path_buf)) {
                if (comptime @import("builtin").os.tag == .linux) {
                    let stx: std.os.linux.Statx = undefined;
                    const mask = std.os.linux.STATX{ .TYPE = true, .MODE = true, .SIZE = true };
                    const rc = std.c.statx(std.os.linux.AT.FDCWD, path_z, 0, mask, &stx);
                    if (rc != 0) {
                        comp.state = AioState.Failed;
                        comp.errno_code = currentCErrno();
                    } else {
                        comp.stat_size = stx.size;
                        comp.stat_mode = stx.mode;
                        comp.stat_is_dir = (stx.mode & 0o170000) == 0o040000;
                    }
                } else {
                    comp.state = AioState.Failed;
                    comp.errno_code = EINVAL_CODE;
                }
            } else {
                comp.state = AioState.Failed;
                comp.errno_code = if (op.path.len == 0) EINVAL_CODE else ENAMETOOLONG_CODE;
            }
            return comp;
        },
        AioOpKind.Fsync => {
            const rc = std.c.fsync(op.fd);
            if (rc != 0) {
                comp.state = AioState.Failed;
                comp.errno_code = currentCErrno();
            }
            return comp;
        },
        AioOpKind.Close => {
            const rc = std.c.close(op.fd);
            if (rc != 0) {
                comp.state = AioState.Failed;
                comp.errno_code = currentCErrno();
            }
            return comp;
        },
        AioOpKind.Unlink => {
            let path_buf: [1024]u8 = undefined;
            if (path_z := copyPathToZ(op.path, &path_buf)) {
                const rc = std.c.unlink(path_z);
                if (rc != 0) {
                    comp.state = AioState.Failed;
                    comp.errno_code = currentCErrno();
                }
            } else {
                comp.state = AioState.Failed;
                comp.errno_code = if (op.path.len == 0) EINVAL_CODE else ENAMETOOLONG_CODE;
            }
            return comp;
        },
        AioOpKind.Mkdir => {
            let path_buf: [1024]u8 = undefined;
            if (path_z := copyPathToZ(op.path, &path_buf)) {
                const rc = std.c.mkdir(path_z, @as(std.c.mode_t, @intCast(op.mode)));
                if (rc != 0) {
                    comp.state = AioState.Failed;
                    comp.errno_code = currentCErrno();
                }
            } else {
                comp.state = AioState.Failed;
                comp.errno_code = if (op.path.len == 0) EINVAL_CODE else ENAMETOOLONG_CODE;
            }
            return comp;
        },
    }
}

/// Zero-heap-allocation fixed-capacity submission/completion ring queue for POSIX I/O operations.
pub struct AioQueue<const CAP: usize> {
    sq: [CAP]AioOp = undefined,
    sq_head: usize = 0,
    sq_tail: usize = 0,
    sq_count: usize = 0,
    cq: [CAP]AioCompletion = undefined,
    cq_head: usize = 0,
    cq_tail: usize = 0,
    cq_count: usize = 0,
    next_id: u64 = 1,
    submitted_total: u64 = 0,
    completed_total: u64 = 0,
    cancelled_total: u64 = 0,

    pub static fn init(): AioQueue<CAP> {
        return AioQueue<CAP> {
            sq: undefined,
            sq_head: 0,
            sq_tail: 0,
            sq_count: 0,
            cq: undefined,
            cq_head: 0,
            cq_tail: 0,
            cq_count: 0,
            next_id: 1,
            submitted_total: 0,
            completed_total: 0,
            cancelled_total: 0,
        };
    }

    pub fn capacity(): usize {
        return _.sq.len;
    }

    pub fn pendingCount(): usize {
        return _.sq_count;
    }

    pub fn completedCount(): usize {
        return _.cq_count;
    }

    pub fn isEmpty(): bool {
        return _.sq_count == 0 && _.cq_count == 0;
    }

    pub fn isFull(): bool {
        return _.sq_count >= CAP;
    }

    /// Enqueues a single `AioOp` into the submission ring. Auto-assigns a monotonic ID if `op.id == 0`.
    pub fn submit(*_, op: AioOp): u64! {
        if (CAP == 0 || _.sq_count >= CAP) {
            return error.QueueFull;
        }
        let assigned_id = op.id;
        if (assigned_id == 0) {
            assigned_id = _.next_id;
            _.next_id += 1;
        }
        _.sq[_.sq_tail] = op;
        _.sq[_.sq_tail].id = assigned_id;
        _.sq[_.sq_tail].state = AioState.Submitted;
        _.sq_tail = (_.sq_tail + 1) % CAP;
        _.sq_count += 1;
        _.submitted_total += 1;
        return assigned_id;
    }

    /// Atomically checks capacity and enqueues a batch of `AioOp` operations.
    pub fn submitBatch(*_, ops: []const AioOp): usize! {
        if (CAP == 0 || _.sq_count + ops.len > CAP) {
            return error.QueueFull;
        }
        let i: usize = 0;
        while (i < ops.len) {
            * = try _.submit(ops[i]);
            i += 1;
        }
        return ops.len;
    }

    /// Cancels a queued submission matching `op_id` before execution.
    pub fn cancel(*_, op_id: u64): bool {
        if (CAP == 0 || _.sq_count == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.sq_count) {
            const idx = (_.sq_head + i) % CAP;
            if (_.sq[idx].id == op_id && _.sq[idx].state == AioState.Submitted) {
                _.sq[idx].state = AioState.Cancelled;
                _.cancelled_total += 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Executes queued submission entries in FIFO order and pushes completions into the CQ ring.
    pub fn flush(*_): usize {
        if (CAP == 0) {
            return 0;
        }
        let executed: usize = 0;
        while (_.sq_count > 0 && _.cq_count < CAP) {
            const op = _.sq[_.sq_head];
            _.sq_head = (_.sq_head + 1) % CAP;
            _.sq_count -= 1;

            const comp = executeAioOp(op);
            _.cq[_.cq_tail] = comp;
            _.cq_tail = (_.cq_tail + 1) % CAP;
            _.cq_count += 1;
            _.completed_total += 1;
            executed += 1;
        }
        return executed;
    }

    /// Pops completed entries from the CQ ring into `out`, returning the number of completions copied.
    pub fn poll(*_, out: []AioCompletion): usize {
        if (CAP == 0 || out.len == 0 || _.cq_count == 0) {
            return 0;
        }
        let count: usize = 0;
        while (count < out.len && _.cq_count > 0) {
            out[count] = _.cq[_.cq_head];
            _.cq_head = (_.cq_head + 1) % CAP;
            _.cq_count -= 1;
            count += 1;
        }
        return count;
    }

    /// Executes pending submissions and drains resulting completions into `out`.
    pub fn drain(*_, out: []AioCompletion): usize {
        let total: usize = 0;
        while (total < out.len) {
            * = _.flush();
            const popped = _.poll(out[total..]);
            if (popped == 0) {
                break;
            }
            total += popped;
        }
        return total;
    }
}
