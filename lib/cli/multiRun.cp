// Source: Absorbed from /data/dev/bun/src/cli/multi_run.cp | Synced: 2026-11-23T20:20:53Z
// Ported from Bun: /data/dev/bun/src/cli/multi_run.cp & /data/dev/bun/docs/snippets/cli/run.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation Foreman-style `--parallel` / `--sequential` multi-script runner:
// 6-color ANSI label prefixer with dynamic column padding, streaming line buffer (`feedChunk` /
// `flushRemaining`), and two-tier dependency cascade (`group_dependents` pre->main->post skip
// on failure vs `next_dependents` sequential continuation under `--no-exit-on-error`).

/// 单个 MultiRun 调度器支持的最大脚本数
pub const MULTI_RUN_MAX_SCRIPTS: usize = 64;

/// 单个 MultiRun 调度器支持的最大依赖边数（组内边 + 跨组顺序边）
pub const MULTI_RUN_MAX_EDGES: usize = 256;

/// 单个流式行缓冲区容量（字节）
pub const MULTI_RUN_LINE_BUF_CAP: usize = 1024;

/// Foreman 调色板颜色数（对齐 Bun `multi_run.cp` 6 色循环：cyan/yellow/magenta/green/blue/red）
pub const MULTI_RUN_PALETTE_LEN: usize = 6;

/// MultiRun 调度与流式格式化错误集
pub err MultiRunError {
    EmptyLabel,
    TooManyScripts,
    TooManyEdges,
    InvalidScriptIndex,
    InvalidStateTransition,
    LineBufferOverflow,
    BufferTooSmall,
}

/// 依赖边类别：组内生命周期边（失败即跳过下游）或跨组顺序边（`--no-exit-on-error` 下失败仍继续）
pub enum MultiRunEdgeKind: u8 {
    /// 同脚本组内依赖（`pre -> main -> post`）：前置失败则整组剩余步骤被跳过
    GroupChain = 0,
    /// 跨脚本组顺序依赖（`group N -> group N+1`）：开启 `no_exit_on_error` 时前置失败仍触发下一组
    NextGroup = 1,
}

/// MultiRun 脚本任务状态
pub enum MultiRunStatus: u8 {
    Pending = 0,
    Running = 1,
    Exited = 2,
    Signaled = 3,
    Errored = 4,
    Skipped = 5,
    Aborted = 6,
}

/// MultiRun 任务节点
pub struct MultiRunTask {
    label: []const u8 = "",
    color_idx: usize = 0,
    status: MultiRunStatus = MultiRunStatus.Pending,
    exit_code: u8 = 0,
    signal_code: u8 = 0,
    duration_ns: u64 = 0,
    remaining_deps: usize = 0,
}

fn appendBytes(out: []u8, pos: *usize, src: []const u8): void! {
    if (pos.* + src.len > out.len) {
        return MultiRunError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn appendByte(out: []u8, pos: *usize, b: u8): void! {
    if (pos.* + 1 > out.len) {
        return MultiRunError.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

fn appendU64(out: []u8, pos: *usize, val_in: u64): void! {
    if (val_in == 0) {
        appendByte(out, pos, '0')?;
        return;
    }
    let rev: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val_in;
    while (v > 0) {
        rev[n] = ('0' as u8) + ((v % 10) as u8);
        v /= 10;
        n += 1;
    }
    if (pos.* + n > out.len) {
        return MultiRunError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = rev[n - 1 - i];
        i += 1;
    }
    pos.* += n;
}

/// 返回第 `color_idx` 个脚本对应的 ANSI 前景色转义序列（6 色循环：cyan/yellow/magenta/green/blue/red）
pub fn multiRunPaletteColor(color_idx: usize): []const u8 {
    return switch (color_idx % MULTI_RUN_PALETTE_LEN) {
        0 => "\x1b[36m",
        1 => "\x1b[33m",
        2 => "\x1b[35m",
        3 => "\x1b[32m",
        4 => "\x1b[34m",
        else => "\x1b[31m",
    };
}

/// 渲染对齐的 Foreman 行前缀：`[ANSI]<label><padding>[RESET] | `
pub fn formatMultiRunPrefix(
    label: []const u8,
    max_label_len: usize,
    color_idx: usize,
    use_colors: bool,
    out: []u8,
): usize! {
    let pos: usize = 0;
    if (use_colors) {
        appendBytes(out, &pos, multiRunPaletteColor(color_idx))?;
    }
    appendBytes(out, &pos, label)?;
    let pad: usize = if (max_label_len > label.len) max_label_len - label.len else 0;
    while (pad > 0) {
        appendByte(out, &pos, ' ')?;
        pad -= 1;
    }
    if (use_colors) {
        appendBytes(out, &pos, "\x1b[0m")?;
    }
    appendBytes(out, &pos, " | ")?;
    return pos;
}

/// 渲染脚本退出状态行（对齐 Bun `multi_run.cp`：`Done in 1.25s\n` / `Done in 42ms\n` / `Exited with code N\n`）
pub fn formatMultiRunExitStatus(
    task: MultiRunTask,
    max_label_len: usize,
    use_colors: bool,
    out: []u8,
): usize! {
    let pos: usize = formatMultiRunPrefix(task.label, max_label_len, task.color_idx, use_colors, out)?;
    switch (task.status) {
        MultiRunStatus.Exited => {
            if (task.exit_code != 0) {
                appendBytes(out, &pos, "Exited with code ")?;
                appendU64(out, &pos, task.exit_code as u64)?;
                appendByte(out, &pos, '\n')?;
            } else if (task.duration_ns == 0) {
                appendBytes(out, &pos, "Done\n")?;
            } else if (task.duration_ns > 1_000_000_000) {
                appendBytes(out, &pos, "Done in ")?;
                const centis = (task.duration_ns + 5_000_000) / 10_000_000;
                const secs = centis / 100;
                const frac = centis % 100;
                appendU64(out, &pos, secs)?;
                appendByte(out, &pos, '.')?;
                appendByte(out, &pos, ('0' as u8) + ((frac / 10) as u8))?;
                appendByte(out, &pos, ('0' as u8) + ((frac % 10) as u8))?;
                appendBytes(out, &pos, "s\n")?;
            } else {
                appendBytes(out, &pos, "Done in ")?;
                const ms = (task.duration_ns + 500_000) / 1_000_000;
                appendU64(out, &pos, ms)?;
                appendBytes(out, &pos, "ms\n")?;
            }
        },
        MultiRunStatus.Signaled => {
            appendBytes(out, &pos, "Signaled: ")?;
            appendU64(out, &pos, task.signal_code as u64)?;
            appendByte(out, &pos, '\n')?;
        },
        MultiRunStatus.Errored => {
            appendBytes(out, &pos, "Error\n")?;
        },
        MultiRunStatus.Skipped => {
            appendBytes(out, &pos, "Skipped\n")?;
        },
        MultiRunStatus.Aborted => {
            appendBytes(out, &pos, "Aborted\n")?;
        },
        MultiRunStatus.Pending => {
            appendBytes(out, &pos, "Pending\n")?;
        },
        MultiRunStatus.Running => {
            appendBytes(out, &pos, "Running\n")?;
        },
    }
    return pos;
}

/// 零堆分配流式行前缀缓冲器（对齐 Bun `multi_run.PipeReader` + `readChunk` / `flushPipeBuffer`）
pub struct MultiRunLineBuffer {
    buf: [MULTI_RUN_LINE_BUF_CAP]u8 = [0; MULTI_RUN_LINE_BUF_CAP],
    len: usize = 0,

    static fn init(): MultiRunLineBuffer {
        return MultiRunLineBuffer{};
    }

    /// 喂入任意字节分片：将所有以 `\n` 结尾的完整行加上 Foreman 前缀写入 `out`，
    /// 末尾尚未换行的半行数据暂存于内部缓冲等待后续分片或 `flushRemaining`。
    fn feedChunk(
        *_,
        chunk: []const u8,
        label: []const u8,
        max_label_len: usize,
        color_idx: usize,
        use_colors: bool,
        out: []u8,
    ): usize! {
        let out_pos: usize = 0;
        let i: usize = 0;
        while (i < chunk.len) {
            const b = chunk[i];
            if (_.len >= MULTI_RUN_LINE_BUF_CAP) {
                return MultiRunError.LineBufferOverflow;
            }
            _.buf[_.len] = b;
            _.len += 1;
            if (b == '\n') {
                const pn = formatMultiRunPrefix(label, max_label_len, color_idx, use_colors, out[out_pos..])?;
                out_pos += pn;
                appendBytes(out, &out_pos, _.buf[0.._.len])?;
                _.len = 0;
            }
            i += 1;
        }
        return out_pos;
    }

    /// 进程退出时刷出末尾未换行残留数据，自动补齐 `\n`
    fn flushRemaining(
        *_,
        label: []const u8,
        max_label_len: usize,
        color_idx: usize,
        use_colors: bool,
        out: []u8,
    ): usize! {
        if (_.len == 0) {
            return 0;
        }
        let out_pos: usize = formatMultiRunPrefix(label, max_label_len, color_idx, use_colors, out)?;
        appendBytes(out, &out_pos, _.buf[0.._.len])?;
        if (_.buf[_.len - 1] != '\n') {
            appendByte(out, &out_pos, '\n')?;
        }
        _.len = 0;
        return out_pos;
    }
}

/// 零堆分配双层依赖级联多脚本调度器（对齐 Bun `multi_run.State`）
pub struct MultiRunScheduler {
    tasks: [MULTI_RUN_MAX_SCRIPTS]MultiRunTask = [MultiRunTask{}; MULTI_RUN_MAX_SCRIPTS],
    task_count: usize = 0,
    edge_from: [MULTI_RUN_MAX_EDGES]u16 = [0; MULTI_RUN_MAX_EDGES],
    edge_to: [MULTI_RUN_MAX_EDGES]u16 = [0; MULTI_RUN_MAX_EDGES],
    edge_kind: [MULTI_RUN_MAX_EDGES]MultiRunEdgeKind = [MultiRunEdgeKind.GroupChain; MULTI_RUN_MAX_EDGES],
    edge_count: usize = 0,
    max_label_len: usize = 0,
    no_exit_on_error: bool = false,
    aborted: bool = false,

    static fn init(no_exit_on_error: bool): MultiRunScheduler {
        return MultiRunScheduler{ no_exit_on_error: no_exit_on_error };
    }

    /// 注册一个脚本任务，自动更新 `max_label_len` 并分配循环调色板下标
    fn addTask(*_, label: []const u8): usize! {
        if (label.len == 0) {
            return MultiRunError.EmptyLabel;
        }
        if (_.task_count >= MULTI_RUN_MAX_SCRIPTS) {
            return MultiRunError.TooManyScripts;
        }
        const idx = _.task_count;
        _.tasks[idx] = MultiRunTask{
            label: label,
            color_idx: idx,
            status: MultiRunStatus.Pending,
        };
        if (label.len > _.max_label_len) {
            _.max_label_len = label.len;
        }
        _.task_count += 1;
        return idx;
    }

    /// 添加有向依赖边：`from_idx -> to_idx`（区分 `GroupChain` 与 `NextGroup`）
    fn addEdge(*_, from_idx: usize, to_idx: usize, kind: MultiRunEdgeKind): void! {
        if (from_idx >= _.task_count || to_idx >= _.task_count) {
            return MultiRunError.InvalidScriptIndex;
        }
        if (_.edge_count >= MULTI_RUN_MAX_EDGES) {
            return MultiRunError.TooManyEdges;
        }
        _.edge_from[_.edge_count] = from_idx as u16;
        _.edge_to[_.edge_count] = to_idx as u16;
        _.edge_kind[_.edge_count] = kind;
        _.edge_count += 1;
        _.tasks[to_idx].remaining_deps += 1;
    }

    /// 收集当前所有满足 `Pending && remaining_deps == 0` 的就绪任务下标
    fn collectReady(out_indices: []usize): usize {
        if (_.aborted) {
            return 0;
        }
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.task_count) {
            if (_.tasks[i].status == MultiRunStatus.Pending && _.tasks[i].remaining_deps == 0) {
                if (n < out_indices.len) {
                    out_indices[n] = i;
                    n += 1;
                }
            }
            i += 1;
        }
        return n;
    }

    fn markStarted(*_, idx: usize): void! {
        if (idx >= _.task_count) {
            return MultiRunError.InvalidScriptIndex;
        }
        if (_.tasks[idx].status != MultiRunStatus.Pending || _.tasks[idx].remaining_deps != 0) {
            return MultiRunError.InvalidStateTransition;
        }
        _.tasks[idx].status = MultiRunStatus.Running;
    }

    fn releaseEdgesOfKind(*_, src_idx: usize, kind: MultiRunEdgeKind) {
        let i: usize = 0;
        while (i < _.edge_count) {
            if ((_.edge_from[i] as usize) == src_idx && _.edge_kind[i] == kind) {
                const to_idx = _.edge_to[i] as usize;
                if (_.tasks[to_idx].remaining_deps > 0) {
                    _.tasks[to_idx].remaining_deps -= 1;
                }
            }
            i += 1;
        }
    }

    /// 迭代式跳过失败节点的全部组内下游（`GroupChain`），同时在未中止时继续释放其跨组顺序下游（`NextGroup`）
    fn skipGroupDependentsIterative(*_, failed_idx: usize) {
        let stack: [MULTI_RUN_MAX_SCRIPTS]u16 = [0; MULTI_RUN_MAX_SCRIPTS];
        let top: usize = 0;

        // 将 failed_idx 的直接 GroupChain 下游入栈
        let e0: usize = 0;
        while (e0 < _.edge_count) {
            if ((_.edge_from[e0] as usize) == failed_idx && _.edge_kind[e0] == MultiRunEdgeKind.GroupChain) {
                const dep = _.edge_to[e0] as usize;
                if (_.tasks[dep].remaining_deps > 0) {
                    _.tasks[dep].remaining_deps -= 1;
                }
                if (_.tasks[dep].remaining_deps == 0 && _.tasks[dep].status == MultiRunStatus.Pending) {
                    _.tasks[dep].status = MultiRunStatus.Skipped;
                    if (top < MULTI_RUN_MAX_SCRIPTS) {
                        stack[top] = dep as u16;
                        top += 1;
                    }
                }
            }
            e0 += 1;
        }

        while (top > 0) {
            top -= 1;
            const cur = stack[top] as usize;
            // 释放被跳过节点指向下一组的 NextGroup 边（确保 sequential --no-exit-on-error 继续）
            if (!_.aborted) {
                _.releaseEdgesOfKind(cur, MultiRunEdgeKind.NextGroup);
            }
            // 继续跳过 cur 的 GroupChain 下游
            let ei: usize = 0;
            while (ei < _.edge_count) {
                if ((_.edge_from[ei] as usize) == cur && _.edge_kind[ei] == MultiRunEdgeKind.GroupChain) {
                    const dep = _.edge_to[ei] as usize;
                    if (_.tasks[dep].remaining_deps > 0) {
                        _.tasks[dep].remaining_deps -= 1;
                    }
                    if (_.tasks[dep].remaining_deps == 0 && _.tasks[dep].status == MultiRunStatus.Pending) {
                        _.tasks[dep].status = MultiRunStatus.Skipped;
                        if (top < MULTI_RUN_MAX_SCRIPTS) {
                            stack[top] = dep as u16;
                            top += 1;
                        }
                    }
                }
                ei += 1;
            }
        }
    }

    fn abortPending(*_) {
        _.aborted = true;
        let i: usize = 0;
        while (i < _.task_count) {
            if (_.tasks[i].status == MultiRunStatus.Pending) {
                _.tasks[i].status = MultiRunStatus.Aborted;
            }
            i += 1;
        }
    }

    /// 标记任务完成并触发双层依赖级联（严格对齐 Bun `multi_run.State.processExit`）
    fn markExited(*_, idx: usize, exit_code: u8, duration_ns: u64): void! {
        if (idx >= _.task_count) {
            return MultiRunError.InvalidScriptIndex;
        }
        if (_.tasks[idx].status != MultiRunStatus.Running) {
            return MultiRunError.InvalidStateTransition;
        }
        _.tasks[idx].status = MultiRunStatus.Exited;
        _.tasks[idx].exit_code = exit_code;
        _.tasks[idx].duration_ns = duration_ns;

        if (exit_code != 0) {
            if (!_.no_exit_on_error) {
                _.abortPending();
                return;
            }
            _.skipGroupDependentsIterative(idx);
            if (!_.aborted) {
                _.releaseEdgesOfKind(idx, MultiRunEdgeKind.NextGroup);
            }
            return;
        }

        if (!_.aborted) {
            _.releaseEdgesOfKind(idx, MultiRunEdgeKind.GroupChain);
            _.releaseEdgesOfKind(idx, MultiRunEdgeKind.NextGroup);
        }
    }

    /// 标记任务被信号终止
    fn markSignaled(*_, idx: usize, signal_code: u8): void! {
        if (idx >= _.task_count) {
            return MultiRunError.InvalidScriptIndex;
        }
        if (_.tasks[idx].status != MultiRunStatus.Running) {
            return MultiRunError.InvalidStateTransition;
        }
        _.tasks[idx].status = MultiRunStatus.Signaled;
        _.tasks[idx].signal_code = signal_code;
        if (!_.no_exit_on_error) {
            _.abortPending();
            return;
        }
        _.skipGroupDependentsIterative(idx);
        if (!_.aborted) {
            _.releaseEdgesOfKind(idx, MultiRunEdgeKind.NextGroup);
        }
    }

    fn isDone(): bool {
        let i: usize = 0;
        while (i < _.task_count) {
            const st = _.tasks[i].status;
            if (st == MultiRunStatus.Running || (st == MultiRunStatus.Pending && !_.aborted)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 聚合最终退出码（首个非零退出码优先，信号映射为 `128 + signal`，错误映射为 `1`）
    fn finalizeExitCode(): u8 {
        let i: usize = 0;
        while (i < _.task_count) {
            const t = _.tasks[i];
            switch (t.status) {
                MultiRunStatus.Exited => {
                    if (t.exit_code != 0) {
                        return t.exit_code;
                    }
                },
                MultiRunStatus.Signaled => {
                    const code: u16 = (128 as u16) + (t.signal_code as u16);
                    return if (code > 255) 255 else (code as u8);
                },
                MultiRunStatus.Errored => {
                    return 1;
                },
                MultiRunStatus.Pending => {},
                MultiRunStatus.Running => {},
                MultiRunStatus.Skipped => {},
                MultiRunStatus.Aborted => {},
            }
            i += 1;
        }
        return 0;
    }
}
