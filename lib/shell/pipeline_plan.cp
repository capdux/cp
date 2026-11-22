// Source: Absorbed from /data/dev/bun/src/shell/states/Pipeline.cp | Synced: 2026-11-22T09:33:43Z
// Ported from Bun: /data/dev/bun/src/shell/states/Pipeline.cp & /data/dev/bun/docs/runtime/shell.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation Shell Pipeline Topology Planner & State Machine:
// - Filters non-runnable assignment-only items and wires N runnable stages across N-1 pipes (readPipe / writePipe)
// - Drives deterministic StartingCmds -> Pending -> Done lifecycle with out-of-order child completion tracking
// - Computes exact FD cleanup ranges on mid-pipeline spawn failure to prevent pipe descriptor leaks
// - Evaluates POSIX last-stage exit status, Bash `pipefail` semantics, and `!` pipeline negation.

/// 单条管道支持的最大可运行阶段数量
pub const SHELL_PIPELINE_MAX_STAGES: usize = 32;

/// 未连接管道端点的哨兵索引
pub const SHELL_PIPELINE_NO_PIPE: u32 = 0xFFFFFFFF;

/// Shell 管道规划与状态机错误集
pub err ShellPipelineError {
    TooManyStages,
    InvalidStageIndex,
    StageNotStarted,
    DuplicateStageCompletion,
    AlreadyCompleted,
    BufferTooSmall,
}

fn appendSlice(buf: []u8, pos: *usize, s: []const u8): ShellPipelineError!bool {
    if (pos.* + s.len > buf.len) {
        return ShellPipelineError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        buf[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

/// 管道成员 AST 节点类别（对齐 Bun `Pipeline.ChildPtr`）
pub enum PipeStageKind: u8 {
    Cmd = 0,
    If = 1,
    CondExpr = 2,
    Subshell = 3,
    Assigns = 4,

    /// 是否为可运行管道阶段（纯赋值节点在管道中不创建子进程/管道槽位）
    fn isRunnable(): bool {
        return _ != .Assigns;
    }

    /// 阶段退出后是否需要由管道释放 duped subshell 状态（对齐 Bun `Pipeline.childDone`）
    fn needsSubshellDeinit(): bool {
        return switch (_) {
            .Cmd, .If, .CondExpr => true,
            .Subshell, .Assigns => false,
        };
    }
}

/// 阶段标准输入来源
pub enum PipeIoSource: u8 {
    InheritStdin = 0,
    PipeRead = 1,
}

/// 阶段标准输出去向
pub enum PipeIoSink: u8 {
    InheritStdout = 0,
    PipeWrite = 1,
}

/// 管道状态机阶段（对齐 Bun `PipelineState`）
pub enum PipeMachineState: u8 {
    StartingCmds = 0,
    Pending = 1,
    WaitingWriteErr = 2,
    Done = 3,
}

/// 管道退出码汇总策略
pub enum PipeExitPolicy: u8 {
    /// POSIX 默认：取最后一个可运行阶段的退出码
    LastStage = 0,
    /// Bash `set -o pipefail`：取最右侧非零退出码，全 0 则返回 0
    Pipefail = 1,
}

/// 相邻阶段间的单条管道通道描述符
pub struct PipeChannelSlot {
    pipe_index: u32 = 0,
    writer_stage: u32 = 0,
    reader_stage: u32 = 0,
}

/// 管道中单个可运行阶段的 I/O 连接与运行状态
pub struct PipeStagePlan {
    stage_index: u32 = 0,
    kind: PipeStageKind = .Cmd,
    label: []const u8 = "",
    stdin_source: PipeIoSource = .InheritStdin,
    stdin_pipe_index: u32 = SHELL_PIPELINE_NO_PIPE,
    stdout_sink: PipeIoSink = .InheritStdout,
    stdout_pipe_index: u32 = SHELL_PIPELINE_NO_PIPE,
    stderr_inherited: bool = true,
    started: bool = false,
    exited: bool = false,
    exit_code: u8 = 0,
}

/// 某阶段启动失败时需要关闭的残留管道端区间（半开区间 `[from, to)`，防 FD 泄漏）
pub struct PipeCleanupAction {
    close_read_from: usize = 0,
    close_read_to: usize = 0,
    close_write_from: usize = 0,
    close_write_to: usize = 0,

    fn readCount(): usize {
        return if (_.close_read_to > _.close_read_from) _.close_read_to - _.close_read_from else 0;
    }

    fn writeCount(): usize {
        return if (_.close_write_to > _.close_write_from) _.close_write_to - _.close_write_from else 0;
    }
}

/// 零堆分配 Shell 管道拓扑规划器与执行状态机
pub struct ShellPipelinePlanner {
    stage_count: usize = 0,
    stages: [SHELL_PIPELINE_MAX_STAGES]PipeStagePlan = [PipeStagePlan{}; SHELL_PIPELINE_MAX_STAGES],
    pipe_count: usize = 0,
    pipes: [SHELL_PIPELINE_MAX_STAGES]PipeChannelSlot = [PipeChannelSlot{}; SHELL_PIPELINE_MAX_STAGES],
    ignored_assign_count: usize = 0,
    negated: bool = false,
    exit_policy: PipeExitPolicy = .LastStage,
    state: PipeMachineState = .StartingCmds,
    next_start_idx: usize = 0,
    exited_count: usize = 0,
    final_exit_code: u8 = 0,

    static fn init(negated: bool, policy: PipeExitPolicy): ShellPipelinePlanner {
        return ShellPipelinePlanner{
            negated: negated,
            exit_policy: policy,
        };
    }

    fn stagesSlice(): []const PipeStagePlan {
        return _.stages[0.._.stage_count];
    }

    fn pipesSlice(): []const PipeChannelSlot {
        return _.pipes[0.._.pipe_count];
    }

    /// 追加管道项；纯赋值项按 Bun 语义忽略并返回 `false`，可运行项纳入拓扑并返回 `true`
    fn addItem(*_, kind: PipeStageKind, label: []const u8): ShellPipelineError!bool {
        if (!kind.isRunnable()) {
            _.ignored_assign_count += 1;
            return false;
        }
        if (_.stage_count >= SHELL_PIPELINE_MAX_STAGES) {
            return ShellPipelineError.TooManyStages;
        }
        const idx = _.stage_count;
        _.stages[idx] = PipeStagePlan{
            stage_index: @as(u32, @intCast(idx)),
            kind: kind,
            label: label,
        };
        _.stage_count += 1;
        _.rewirePipes();
        return true;
    }

    /// 按 Bun `readPipe` / `writePipe` 规则布线：首阶段读 stdin，末阶段写 stdout，中间阶段串联 N-1 条管道
    fn rewirePipes(*_) {
        if (_.stage_count <= 1) {
            _.pipe_count = 0;
            if (_.stage_count == 1) {
                _.stages[0].stdin_source = .InheritStdin;
                _.stages[0].stdin_pipe_index = SHELL_PIPELINE_NO_PIPE;
                _.stages[0].stdout_sink = .InheritStdout;
                _.stages[0].stdout_pipe_index = SHELL_PIPELINE_NO_PIPE;
            }
            return;
        }

        _.pipe_count = _.stage_count - 1;
        let pi: usize = 0;
        while (pi < _.pipe_count) {
            _.pipes[pi] = PipeChannelSlot{
                pipe_index: @as(u32, @intCast(pi)),
                writer_stage: @as(u32, @intCast(pi)),
                reader_stage: @as(u32, @intCast(pi + 1)),
            };
            pi += 1;
        }

        let si: usize = 0;
        while (si < _.stage_count) {
            if (si == 0) {
                _.stages[si].stdin_source = .InheritStdin;
                _.stages[si].stdin_pipe_index = SHELL_PIPELINE_NO_PIPE;
            } else {
                _.stages[si].stdin_source = .PipeRead;
                _.stages[si].stdin_pipe_index = @as(u32, @intCast(si - 1));
            }

            if (si == _.stage_count - 1) {
                _.stages[si].stdout_sink = .InheritStdout;
                _.stages[si].stdout_pipe_index = SHELL_PIPELINE_NO_PIPE;
            } else {
                _.stages[si].stdout_sink = .PipeWrite;
                _.stages[si].stdout_pipe_index = @as(u32, @intCast(si));
            }
            si += 1;
        }
    }

    /// 结束拓扑构建并初始化状态机；若无可运行阶段则直接进入 `Done`
    fn finalizeWiring(*_) {
        _.rewirePipes();
        if (_.stage_count == 0) {
            _.state = .Done;
            _.final_exit_code = if (_.negated) 1 else 0;
        } else {
            _.state = .StartingCmds;
            _.next_start_idx = 0;
            _.exited_count = 0;
            _.final_exit_code = 0;
        }
    }

    /// 推进状态机返回下一个待启动阶段的索引；所有阶段均已派发后转为 `Pending` 并返回 `null`
    fn nextStageToStart(*_): u32? {
        if (_.state != .StartingCmds) {
            return null;
        }
        if (_.next_start_idx >= _.stage_count) {
            _.state = .Pending;
            return null;
        }
        const idx = _.next_start_idx;
        _.stages[idx].started = true;
        _.next_start_idx += 1;
        if (_.next_start_idx >= _.stage_count) {
            _.state = .Pending;
        }
        return @as(u32, @intCast(idx));
    }

    /// 当第 `failed_stage_idx` 阶段创建失败时，计算尚未移交的管道读/写端清理区间（对齐 Bun `Pipeline.cp:163-167`）
    fn computeAbortCleanup(failed_stage_idx: u32): ShellPipelineError!PipeCleanupAction {
        const idx = @as(usize, failed_stage_idx);
        if (idx >= _.stage_count) {
            return ShellPipelineError.InvalidStageIndex;
        }
        if (_.pipe_count == 0) {
            return PipeCleanupAction{};
        }
        const r_from = if (idx < _.pipe_count) idx else _.pipe_count;
        const w_from = if (idx + 1 < _.pipe_count) idx + 1 else _.pipe_count;
        return PipeCleanupAction{
            close_read_from: r_from,
            close_read_to: _.pipe_count,
            close_write_from: w_from,
            close_write_to: _.pipe_count,
        };
    }

    /// 标记管道遇到致命 I/O 写入错误（对齐 Bun `waiting_write_err` -> exit code 1）
    fn enterWriteError(*_) {
        _.state = .WaitingWriteErr;
        _.final_exit_code = 1;
    }

    /// 计算退出码（支持 `LastStage` / `Pipefail` 与 `!` 取反）
    fn computeExitCode(): u8 {
        if (_.stage_count == 0) {
            return if (_.negated) 1 else 0;
        }
        let code: u8 = 0;
        switch (_.exit_policy) {
            .LastStage => {
                code = _.stages[_.stage_count - 1].exit_code;
            },
            .Pipefail => {
                let i: usize = _.stage_count;
                while (i > 0) {
                    i -= 1;
                    if (_.stages[i].exit_code != 0) {
                        code = _.stages[i].exit_code;
                        break;
                    }
                }
            },
        }
        if (_.negated) {
            return if (code == 0) 1 else 0;
        }
        return code;
    }

    /// 记录指定阶段退出（支持任意乱序完成）；全部阶段退出后自动进入 `Done` 并返回 `true`
    fn onStageDone(*_, stage_idx: u32, exit_code: u8): ShellPipelineError!bool {
        if (_.state == .Done) {
            return ShellPipelineError.AlreadyCompleted;
        }
        const idx = @as(usize, stage_idx);
        if (idx >= _.stage_count) {
            return ShellPipelineError.InvalidStageIndex;
        }
        if (!_.stages[idx].started) {
            return ShellPipelineError.StageNotStarted;
        }
        if (_.stages[idx].exited) {
            return ShellPipelineError.DuplicateStageCompletion;
        }

        _.stages[idx].exited = true;
        _.stages[idx].exit_code = exit_code;
        _.exited_count += 1;

        if (_.exited_count >= _.stage_count) {
            _.final_exit_code = _.computeExitCode();
            _.state = .Done;
            return true;
        }
        return false;
    }

    /// 格式化管道拓扑字符串（如 `"! echo | grep | wc"`）
    fn formatTopology(buf: []u8): ShellPipelineError!usize {
        let pos: usize = 0;
        if (_.negated) {
            * = appendSlice(buf, &pos, "! ")?;
        }
        let i: usize = 0;
        while (i < _.stage_count) {
            if (i > 0) {
                * = appendSlice(buf, &pos, " | ")?;
            }
            const lbl = if (_.stages[i].label.len > 0) _.stages[i].label else "<stage>";
            * = appendSlice(buf, &pos, lbl)?;
            i += 1;
        }
        return pos;
    }
}
