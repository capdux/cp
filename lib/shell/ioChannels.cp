// Source: Absorbed from /data/dev/bun/src/shell/IO.cp | Synced: 2026-11-22T14:25:00Z
// Pure-CP zero-allocation Bun Shell Stdin/Stdout/Stderr Channel Descriptor,
// Reference-Counted Handle Table & Subprocess Stdio Router.
// Aligned with Bun `src/shell/IO.cp` and `docs/runtime/shell.mdx`:
// - Models `ShellInKind` (`Fd`, `Ignore`) and `ShellOutKind` (`Fd`, `Pipe` for `.quiet()`, `Ignore`).
// - Tracks shared reader/writer reference counts across pipelines, subshells, and forked commands
//   with double-close protection (`AlreadyClosed`).
// - Routes channels to subprocess stdio (`Fd`, `Capture`, `Inherit`, `Pipe`, `Ignore`):
//   when an output `Fd` carries a capture buffer it routes to `.Capture`; when a valid OS fd is
//   present it routes to `.Fd`; when the fd was moved to the event loop (e.g. Windows/libuv) it
//   falls back to `.Inherit`.

/// 句柄表支持的最大活跃 IO 句柄数
pub const SHELL_IO_MAX_HANDLES: usize = 64;
/// 无效句柄槽位哨兵值
pub const SHELL_IO_INVALID_HANDLE: u32 = 0xFFFF_FFFF;

/// Shell IO 通道与引用计数错误集
pub err ShellIoError {
    HandleTableFull,
    InvalidHandle,
    AlreadyClosed,
    BufferTooSmall,
}

/// 输入通道类型（对齐 Bun `IO.InKind`）
pub enum ShellInKind: u8 {
    Ignore = 0,
    Fd = 1,
}

/// 输出通道类型（对齐 Bun `IO.OutKind`）
pub enum ShellOutKind: u8 {
    Ignore = 0,
    /// `.quiet()` 缓冲管道模式
    Pipe = 1,
    /// 文件描述符输出（可附加同步捕获缓冲区）
    Fd = 2,
}

/// 子进程 Stdio 路由目标（对齐 Bun `subproc.Stdio`）
pub enum SubprocStdioKind: u8 {
    Ignore = 0,
    Pipe = 1,
    Inherit = 2,
    Fd = 3,
    Capture = 4,
}

/// 单个通道的子进程路由决策
pub struct SubprocStdioRoute {
    kind: SubprocStdioKind = .Ignore,
    os_fd: i32 = -1,
    capture_buf_id: u32 = 0,
    writer_handle: u32 = SHELL_IO_INVALID_HANDLE,
}

/// 子进程三通道 Stdio 路由计划（`[0]=stdin, [1]=stdout, [2]=stderr`）
pub struct ShellSubprocStdioPlan {
    stdin: SubprocStdioRoute = SubprocStdioRoute{},
    stdout: SubprocStdioRoute = SubprocStdioRoute{},
    stderr: SubprocStdioRoute = SubprocStdioRoute{},
}

/// 输入通道描述符
pub struct ShellInChannel {
    kind: ShellInKind = .Ignore,
    handle: u32 = SHELL_IO_INVALID_HANDLE,
}

/// 输出通道描述符
pub struct ShellOutChannel {
    kind: ShellOutKind = .Ignore,
    handle: u32 = SHELL_IO_INVALID_HANDLE,
    has_capture: bool = false,
    capture_buf_id: u32 = 0,
    capture_bytes_estimate: usize = 0,
}

/// 单个 Shell 状态节点的三通道 IO 描述符（对齐 Bun `IO`）
pub struct ShellIoTriplet {
    stdin: ShellInChannel = ShellInChannel{},
    stdout: ShellOutChannel = ShellOutChannel{},
    stderr: ShellOutChannel = ShellOutChannel{},

    /// 创建全部忽略的静默 IO 组合
    static fn ignored(): ShellIoTriplet {
        return ShellIoTriplet{};
    }

    /// 创建 `.quiet()` 管道捕获模式（stdout/stderr 均置 `.Pipe`，stdin 为 `.Ignore`）
    static fn quietPipe(): ShellIoTriplet {
        return ShellIoTriplet{
            stdin: ShellInChannel{ kind: .Ignore },
            stdout: ShellOutChannel{ kind: .Pipe },
            stderr: ShellOutChannel{ kind: .Pipe },
        };
    }
}

/// 句柄表中的单个 IO Reader/Writer 条目
pub struct ShellIoHandleEntry {
    in_use: bool = false,
    closed: bool = false,
    ref_count: u32 = 0,
    os_fd: i32 = -1,
    /// 若已移交底层事件循环（如 Windows libuv），子进程需回退 `.Inherit`
    moved_to_loop: bool = false,
    base_mem_bytes: usize = 64,
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) return err.BufferTooSmall;
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn copyByte(dst: []u8, offset: usize, b: u8): usize! {
    if (offset + 1 > dst.len) return err.BufferTooSmall;
    dst[offset] = b;
    return offset + 1;
}

fn copyI32(dst: []u8, offset: usize, val: i32): usize! {
    let pos = offset;
    let uval: u64 = 0;
    if (val < 0) {
        pos = copyByte(dst, pos, '-')?;
        uval = (0 - (val as i64)) as u64;
    } else {
        uval = val as u64;
    }
    if (uval == 0) {
        return copyByte(dst, pos, '0');
    }
    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    while (uval > 0) {
        tmp[n] = ((uval % 10) + 48) as u8;
        uval = uval / 10;
        n += 1;
    }
    while (n > 0) {
        n -= 1;
        pos = copyByte(dst, pos, tmp[n])?;
    }
    return pos;
}

/// 零堆分配 Shell IO 句柄引用计数表与子进程路由引擎
pub struct ShellIoHandleTable {
    entries: [SHELL_IO_MAX_HANDLES]ShellIoHandleEntry = [ShellIoHandleEntry{}; SHELL_IO_MAX_HANDLES],
    active_handles: usize = 0,

    static fn init(): ShellIoHandleTable {
        return ShellIoHandleTable{};
    }

    /// 分配一个初始引用计数为 1 的 IO 句柄
    fn openHandle(*_, os_fd: i32, moved_to_loop: bool, base_mem_bytes: usize): u32! {
        for (i of 0..SHELL_IO_MAX_HANDLES) {
            if (!_.entries[i].in_use) {
                _.entries[i] = ShellIoHandleEntry{
                    in_use: true,
                    closed: false,
                    ref_count: 1,
                    os_fd: os_fd,
                    moved_to_loop: moved_to_loop,
                    base_mem_bytes: base_mem_bytes,
                };
                _.active_handles += 1;
                return i as u32;
            }
        }
        return err.HandleTableFull;
    }

    /// 读取句柄状态
    fn getHandle(handle: u32): ShellIoHandleEntry! {
        const idx = handle as usize;
        if (handle == SHELL_IO_INVALID_HANDLE || idx >= SHELL_IO_MAX_HANDLES || !_.entries[idx].in_use) {
            return err.InvalidHandle;
        }
        return _.entries[idx];
    }

    /// 增加单个句柄引用计数
    fn refHandle(*_, handle: u32): u32! {
        const idx = handle as usize;
        if (handle == SHELL_IO_INVALID_HANDLE || idx >= SHELL_IO_MAX_HANDLES || !_.entries[idx].in_use) {
            return err.InvalidHandle;
        }
        if (_.entries[idx].closed) return err.AlreadyClosed;
        _.entries[idx].ref_count += 1;
        return _.entries[idx].ref_count;
    }

    /// 递减单个句柄引用计数，归零时标记为已关闭并减少 `active_handles`
    fn derefHandle(*_, handle: u32): u32! {
        const idx = handle as usize;
        if (handle == SHELL_IO_INVALID_HANDLE || idx >= SHELL_IO_MAX_HANDLES || !_.entries[idx].in_use) {
            return err.InvalidHandle;
        }
        if (_.entries[idx].closed || _.entries[idx].ref_count == 0) {
            return err.AlreadyClosed;
        }
        _.entries[idx].ref_count -= 1;
        if (_.entries[idx].ref_count == 0) {
            _.entries[idx].closed = true;
            if (_.active_handles > 0) {
                _.active_handles -= 1;
            }
        }
        return _.entries[idx].ref_count;
    }

    /// 复制三通道 IO（对所有 `.Fd` 通道递增引用计数，对齐 Bun `IO.copy()`）
    fn copyTriplet(*_, io: ShellIoTriplet): ShellIoTriplet! {
        if (io.stdin.kind == .Fd) {
            * = _.refHandle(io.stdin.handle)?;
        }
        if (io.stdout.kind == .Fd) {
            * = _.refHandle(io.stdout.handle)?;
        }
        if (io.stderr.kind == .Fd) {
            * = _.refHandle(io.stderr.handle)?;
        }
        return io;
    }

    /// 释放三通道 IO（对所有 `.Fd` 通道递减引用计数，对齐 Bun `IO.deinit()`）
    fn releaseTriplet(*_, io: ShellIoTriplet): usize! {
        let released: usize = 0;
        if (io.stdin.kind == .Fd) {
            * = _.derefHandle(io.stdin.handle)?;
            released += 1;
        }
        if (io.stdout.kind == .Fd) {
            * = _.derefHandle(io.stdout.handle)?;
            released += 1;
        }
        if (io.stderr.kind == .Fd) {
            * = _.derefHandle(io.stderr.handle)?;
            released += 1;
        }
        return released;
    }

    /// 将单个输出通道映射为子进程 Stdio 路由（同时为 `.Fd` 复制一次 writer 引用）
    fn routeOutChannel(*_, ch: ShellOutChannel): SubprocStdioRoute! {
        return switch (ch.kind) {
            .Ignore => SubprocStdioRoute{ kind: .Ignore },
            .Pipe => SubprocStdioRoute{ kind: .Pipe },
            .Fd => {
                const ent = _.getHandle(ch.handle)?;
                * = _.refHandle(ch.handle)?;
                if (ch.has_capture) {
                    return SubprocStdioRoute{
                        kind: .Capture,
                        os_fd: ent.os_fd,
                        capture_buf_id: ch.capture_buf_id,
                        writer_handle: ch.handle,
                    };
                }
                if (ent.os_fd >= 0 && !ent.moved_to_loop) {
                    return SubprocStdioRoute{
                        kind: .Fd,
                        os_fd: ent.os_fd,
                        writer_handle: ch.handle,
                    };
                }
                return SubprocStdioRoute{
                    kind: .Inherit,
                    os_fd: ent.os_fd,
                    writer_handle: ch.handle,
                };
            },
        };
    }

    /// 将三通道 IO 映射为子进程 Stdio 路由计划（对齐 Bun `IO.to_subproc_stdio`）
    fn toSubprocStdio(*_, io: ShellIoTriplet): ShellSubprocStdioPlan! {
        let stdin_route = SubprocStdioRoute{ kind: .Ignore };
        if (io.stdin.kind == .Fd) {
            const ent = _.getHandle(io.stdin.handle)?;
            stdin_route = SubprocStdioRoute{
                kind: if (ent.os_fd >= 0 && !ent.moved_to_loop) .Fd else .Inherit,
                os_fd: ent.os_fd,
                writer_handle: io.stdin.handle,
            };
        }
        const stdout_route = _.routeOutChannel(io.stdout)?;
        const stderr_route = _.routeOutChannel(io.stderr)?;
        return ShellSubprocStdioPlan{
            stdin: stdin_route,
            stdout: stdout_route,
            stderr: stderr_route,
        };
    }

    /// 估算三通道 IO 的总内存占用（对齐 Bun `IO.memoryCost`）
    fn estimateMemoryCost(io: ShellIoTriplet): usize {
        let total: usize = 64; // base triplet struct size
        if (io.stdin.kind == .Fd) {
            if (ent := _.getHandle(io.stdin.handle)) {
                total += ent.base_mem_bytes;
            }
        }
        if (io.stdout.kind == .Fd) {
            if (ent := _.getHandle(io.stdout.handle)) {
                total += ent.base_mem_bytes;
            }
            if (io.stdout.has_capture) {
                total += io.stdout.capture_bytes_estimate;
            }
        }
        if (io.stderr.kind == .Fd) {
            if (ent := _.getHandle(io.stderr.handle)) {
                total += ent.base_mem_bytes;
            }
            if (io.stderr.has_capture) {
                total += io.stderr.capture_bytes_estimate;
            }
        }
        return total;
    }

    /// 格式化三通道 IO 诊断摘要（对齐 Bun `IO.format`）
    fn formatTriplet(io: ShellIoTriplet, out: []u8): usize! {
        let pos: usize = 0;
        pos = copyInto(out, pos, "stdin: ")?;
        if (io.stdin.kind == .Ignore) {
            pos = copyInto(out, pos, "ignore")?;
        } else {
            const ent = _.getHandle(io.stdin.handle)?;
            pos = copyInto(out, pos, "fd: ")?;
            pos = copyI32(out, pos, ent.os_fd)?;
        }

        pos = copyInto(out, pos, "\nstdout: ")?;
        pos = _.formatOutChannel(io.stdout, out, pos)?;

        pos = copyInto(out, pos, "\nstderr: ")?;
        pos = _.formatOutChannel(io.stderr, out, pos)?;
        return pos;
    }

    fn formatOutChannel(ch: ShellOutChannel, out: []u8, offset: usize): usize! {
        return switch (ch.kind) {
            .Ignore => copyInto(out, offset, "ignore"),
            .Pipe => copyInto(out, offset, "pipe"),
            .Fd => {
                const ent = _.getHandle(ch.handle)?;
                let pos = copyInto(out, offset, "fd: ")?;
                return copyI32(out, pos, ent.os_fd);
            },
        };
    }
}
