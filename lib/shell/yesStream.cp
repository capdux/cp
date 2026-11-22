// Source: Absorbed from /data/dev/bun/src/shell/builtin/yes.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation POSIX/Bun Shell `yes` Builtin Chunk Generator & Cooperative Yield State Machine
// Aligned with Bun src/shell/builtin/yes.cp & docs/runtime/shell.mdx:
// - Formats default "y\n" or space-joined arguments + "\n" into a single template line
// - Repeats whole lines into an 8KiB (BUFSIZ) slab without ever splitting a line across chunk boundaries
// - Uses doubling-style block replication for fast chunk preparation with zero heap allocation
// - Cooperatively yields back to the event loop every 4 chunks (32KiB) so synchronous pipelines never starve the main thread
// - Handles downstream pipe closure (EPIPE) and I/O write errors cleanly.

/// 默认输出块容量（对齐 Bun `yes.cp` `BUFSIZ = 8192`）
pub const YES_BUFSIZ: usize = 8192;

/// 触发 8KiB 块倍增填充的单行长度阈值（`BUFSIZ / 2 = 4096`）
pub const YES_HALF_BUFSIZ: usize = 4096;

/// 每轮事件循环 tick 最多连续写出的块数（对齐 Bun `writeNoIO` 4 × 8KiB = 32KiB 后让出）
pub const YES_CHUNKS_PER_TICK: usize = 4;

/// `yes` 块生成器与状态机错误集
pub err ShellYesError {
    BufferTooSmall,
    LineTooLong,
    LineSizeOverflow,
    InvalidState,
}

/// `yes` 内置命令执行状态（对齐 Bun `Yes.State`）
pub enum YesState: u8 {
    Idle = 0,
    Running = 1,
    Suspended = 2,
    WaitingWriteErr = 3,
    Done = 4,
    Failed = 5,
}

/// 单步协同调度动作
pub enum YesTickAction: u8 {
    /// 可立即写出 `chunkSlice()`
    WriteChunk = 0,
    /// 已达本轮 4 块配额，应让出至事件循环微任务队列
    YieldToEventLoop = 1,
    /// 下游已优雅结束（如 EPIPE）
    Done = 2,
    /// 写入失败，应向 stderr 输出错误并以非零退出码结束
    WriteError = 3,
}

/// 块填充布局计算结果
pub struct YesLayout {
    line_len: usize = 0,
    target_buf_len: usize = 0,
    copies_per_chunk: usize = 0,
    chunk_len: usize = 0,
}

/// 计算单行输出字节数（无参返回 2 `"y\n"`；多参按单空格分隔并追加 `'\n'`）
pub fn computeYesLineLen(args: []const ([]const u8)): ShellYesError!usize {
    if (args.len == 0) return 2;
    let total: usize = 0;
    let i: usize = 0;
    while (i < args.len) {
        const part_len = args[i].len;
        if (part_len > 0x3FFF_FFFF || total > 0x3FFF_FFFF - part_len) {
            return err.LineSizeOverflow;
        }
        total += part_len;
        if (i + 1 < args.len) {
            if (total >= 0x3FFF_FFFF) return err.LineSizeOverflow;
            total += 1;
        }
        i += 1;
    }
    if (total >= 0x3FFF_FFFF) return err.LineSizeOverflow;
    total += 1; // '\n'
    return total;
}

/// 计算给定期望缓冲容量 `buf_cap` 下的整行块布局（保证绝不截断半行）
pub fn computeYesLayout(args: []const ([]const u8), buf_cap: usize): ShellYesError!YesLayout {
    const line_len = computeYesLineLen(args)?;
    if (buf_cap < line_len) return err.BufferTooSmall;

    let target_len = line_len;
    if (line_len <= YES_HALF_BUFSIZ) {
        target_len = if (buf_cap < YES_BUFSIZ) buf_cap else YES_BUFSIZ;
    } else if (target_len > buf_cap) {
        return err.BufferTooSmall;
    }

    const copies = target_len / line_len;
    const chunk_len = copies * line_len;
    return YesLayout{
        line_len: line_len,
        target_buf_len: target_len,
        copies_per_chunk: copies,
        chunk_len: chunk_len,
    };
}

/// 将 `yes` 重复输出行以倍增复制填充到调用方提供的固定缓冲区 `out`，返回有效字节长度
pub fn fillYesChunk(args: []const ([]const u8), out: []u8): ShellYesError!usize {
    const layout = computeYesLayout(args, out.len)?;

    // 1. 写入首份模板行
    let pos: usize = 0;
    if (args.len == 0) {
        out[0] = 'y';
        out[1] = '\n';
        pos = 2;
    } else {
        let i: usize = 0;
        while (i < args.len) {
            const arg = args[i];
            let j: usize = 0;
            while (j < arg.len) {
                out[pos] = arg[j];
                pos += 1;
                j += 1;
            }
            if (i + 1 < args.len) {
                out[pos] = ' ';
                pos += 1;
            }
            i += 1;
        }
        out[pos] = '\n';
        pos += 1;
    }

    // 2. 倍增复制填满整个 chunk_len（保持整行对齐，不截断半行）
    let filled = pos;
    while (filled < layout.chunk_len) {
        const remaining = layout.chunk_len - filled;
        const to_copy = if (filled < remaining) filled else remaining;
        let k: usize = 0;
        while (k < to_copy) {
            out[filled + k] = out[k];
            k += 1;
        }
        filled += to_copy;
    }
    return filled;
}

/// 零堆分配 `yes` 内置命令流式块生成器与协同让出状态机
pub struct YesStreamState {
    buf: [YES_BUFSIZ]u8 = [0; YES_BUFSIZ],
    chunk_len: usize = 0,
    line_len: usize = 0,
    copies_per_chunk: usize = 0,
    state: YesState = .Idle,
    chunks_in_current_tick: usize = 0,
    total_chunks_emitted: u64 = 0,
    total_bytes_emitted: u64 = 0,
    total_lines_emitted: u64 = 0,
    exit_code: u8 = 0,

    static fn init(args: []const ([]const u8)): ShellYesError!YesStreamState {
        const line_len = computeYesLineLen(args)?;
        if (line_len > YES_BUFSIZ) return err.LineTooLong;

        let st = YesStreamState{};
        const layout = computeYesLayout(args, YES_BUFSIZ)?;
        const filled = fillYesChunk(args, st.buf[0..])?;
        st.chunk_len = filled;
        st.line_len = layout.line_len;
        st.copies_per_chunk = layout.copies_per_chunk;
        st.state = .Running;
        return st;
    }

    fn chunkSlice(): []const u8 {
        return _.buf[0.._.chunk_len];
    }

    fn lineLen(): usize {
        return _.line_len;
    }

    fn copiesPerChunk(): usize {
        return _.copies_per_chunk;
    }

    fn currentState(): YesState {
        return _.state;
    }

    fn exitCode(): u8 {
        return _.exit_code;
    }

    fn totalChunksEmitted(): u64 {
        return _.total_chunks_emitted;
    }

    fn totalBytesEmitted(): u64 {
        return _.total_bytes_emitted;
    }

    fn totalLinesEmitted(): u64 {
        return _.total_lines_emitted;
    }

    /// 推进一步协同调度：每轮最多连发 4 块（32KiB），随后返回 YieldToEventLoop 防止饥饿主线程
    fn nextStep(*_): YesTickAction {
        if (_.state == .Done) return .Done;
        if (_.state == .Failed || _.state == .WaitingWriteErr) return .WriteError;
        if (_.state == .Suspended) return .YieldToEventLoop;
        if (_.state == .Idle) {
            _.state = .Running;
        }
        if (_.chunks_in_current_tick >= YES_CHUNKS_PER_TICK) {
            _.state = .Suspended;
            return .YieldToEventLoop;
        }
        _.chunks_in_current_tick += 1;
        return .WriteChunk;
    }

    /// 记录一次写出完成的字节数
    fn onChunkWritten(*_, bytes_written: usize) {
        _.total_chunks_emitted += 1;
        _.total_bytes_emitted += (bytes_written as u64);
        if (_.line_len > 0) {
            _.total_lines_emitted += ((bytes_written / _.line_len) as u64);
        }
    }

    /// 事件循环重新唤醒后恢复下一轮 4 块写入配额
    fn resumeFromEventLoop(*_): bool {
        if (_.state != .Suspended) return false;
        _.chunks_in_current_tick = 0;
        _.state = .Running;
        return true;
    }

    /// 处理下游管道写入错误：`is_broken_pipe == true` 视为常规管道上游终止（`yes | head`），否则标记失败（退出码 1）
    fn onWriteError(*_, is_broken_pipe: bool): u8 {
        if (is_broken_pipe) {
            _.state = .Done;
            _.exit_code = 0;
            return 0;
        }
        _.state = .Failed;
        _.exit_code = 1;
        return 1;
    }
}
