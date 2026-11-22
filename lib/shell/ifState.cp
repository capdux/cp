// Source: Absorbed from /data/dev/bun/src/shell/states/If.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation POSIX / Bun Shell `if / elif / else` Control-Flow State Machine
// Aligned with Bun src/shell/states/If.cp & docs/runtime/shell.mdx:
// - Uses Bun's flat `else_parts` layout:
//     * len == 0: no elif/else -> returns exit code 0 if condition fails (POSIX 2.9.4.4)
//     * len == 1: single `else` block
//     * len >= 2: even indices (0, 2, ...) are `elif` conditions, odd indices (1, 3, ...) are matching `then` bodies,
//                 and if `len` is odd the final slot (`len - 1`) is the trailing `else` block.
// - Evaluates multi-statement condition lists sequentially and branches strictly on the last statement's exit code
// - Drives step-by-step via `nextStep()` / `onStmtDone(exit_code)` with zero heap allocation.

/// 单个 `if` 状态机支持的最大总语句槽位数
pub const SHELL_IF_MAX_STMTS: usize = 32;

/// 最大 `else_parts` 分段数（支持最多 7 个 `elif` 对 + 1 个 `else`）
pub const SHELL_IF_MAX_ELSE_PARTS: usize = 16;

/// `if / elif / else` 控制流状态机错误集
pub err ShellIfError {
    TooManyStmts,
    TooManyElseParts,
    InvalidElsePartIndex,
    NotAwaitingStmt,
    StmtStillPending,
    AlreadyStarted,
}

/// 当前所处的分支执行阶段（对齐 Bun `If.ExecInnerState`）
pub enum IfBranchPhase: u8 {
    Idle = 0,
    Cond = 1,
    Then = 2,
    ElifCond = 3,
    ElseBranch = 4,
    Done = 5,
}

/// `nextStep()` 步进动作类型
pub enum IfStepKind: u8 {
    /// 调度器应启动 `stmt_id` 对应的子语句，完成后调用 `onStmtDone(exit_code)`
    RunStmt = 0,
    /// 整个 `if / elif / else` 控制流已结束，`exit_code` 为最终 POSIX 退出码
    Completed = 1,
}

/// 单步调度结果
pub struct IfStepResult {
    kind: IfStepKind = .Completed,
    phase: IfBranchPhase = .Done,
    stmt_id: u32 = 0,
    exit_code: u8 = 0,
}

/// 零堆分配语句列表槽位（指向 `ShellIfStateMachine.stmts` 内部连续区间或独立固定存储）
struct StmtList {
    ids: [SHELL_IF_MAX_STMTS]u32 = [0; SHELL_IF_MAX_STMTS],
    len: usize = 0,

    fn push(*_, stmt_id: u32): ShellIfError!usize {
        if (_.len >= SHELL_IF_MAX_STMTS) return err.TooManyStmts;
        _.ids[_.len] = stmt_id;
        _.len += 1;
        return _.len - 1;
    }
}

/// 零堆分配 POSIX / Bun Shell `if / elif / else` 控制流状态机
pub struct ShellIfStateMachine {
    cond_stmts: StmtList = StmtList{},
    then_stmts: StmtList = StmtList{},
    else_parts: [SHELL_IF_MAX_ELSE_PARTS]StmtList = [StmtList{}; SHELL_IF_MAX_ELSE_PARTS],
    else_part_count: usize = 0,

    phase: IfBranchPhase = .Idle,
    /// 当前正在执行的 `else_parts` 索引（仅在 `ElifCond` / `ElseBranch` / 命中的 `elif` `Then` 时使用）
    active_else_part: usize = 0,
    /// 标识当前 `Then` 阶段是否来自 `else_parts[active_else_part]`（否则来自主 `then_stmts`）
    then_from_else_part: bool = false,
    stmt_idx: usize = 0,
    last_exit_code: u8 = 0,
    final_exit_code: u8 = 0,
    awaiting_child: bool = false,
    executed_count: usize = 0,

    static fn init(): ShellIfStateMachine {
        return ShellIfStateMachine{};
    }

    fn currentPhase(): IfBranchPhase {
        return _.phase;
    }

    fn isDone(): bool {
        return _.phase == .Done;
    }

    fn finalExitCode(): u8 {
        return _.final_exit_code;
    }

    fn executedStmtCount(): usize {
        return _.executed_count;
    }

    fn elsePartCount(): usize {
        return _.else_part_count;
    }

    /// 向主 `if` 条件列表追加语句 ID
    fn addCondStmt(*_, stmt_id: u32): ShellIfError!usize {
        if (_.phase != .Idle) return err.AlreadyStarted;
        return _.cond_stmts.push(stmt_id);
    }

    /// 向主 `then` 分支追加语句 ID
    fn addThenStmt(*_, stmt_id: u32): ShellIfError!usize {
        if (_.phase != .Idle) return err.AlreadyStarted;
        return _.then_stmts.push(stmt_id);
    }

    /// 新增一个扁平 `else_parts` 分段（遵循 Bun `If.cp` 布局规则）
    fn beginElsePart(*_): ShellIfError!usize {
        if (_.phase != .Idle) return err.AlreadyStarted;
        if (_.else_part_count >= SHELL_IF_MAX_ELSE_PARTS) return err.TooManyElseParts;
        const idx = _.else_part_count;
        _.else_parts[idx] = StmtList{};
        _.else_part_count += 1;
        return idx;
    }

    /// 向指定 `else_parts[part_idx]` 追加语句 ID
    fn addElsePartStmt(*_, part_idx: usize, stmt_id: u32): ShellIfError!usize {
        if (_.phase != .Idle) return err.AlreadyStarted;
        if (part_idx >= _.else_part_count) return err.InvalidElsePartIndex;
        return _.else_parts[part_idx].push(stmt_id);
    }

    fn currentListLen(): usize {
        return switch (_.phase) {
            .Cond => _.cond_stmts.len,
            .Then => if (_.then_from_else_part) _.else_parts[_.active_else_part].len else _.then_stmts.len,
            .ElifCond, .ElseBranch => _.else_parts[_.active_else_part].len,
            .Idle, .Done => 0,
        };
    }

    fn currentListStmt(idx: usize): u32 {
        return switch (_.phase) {
            .Cond => _.cond_stmts.ids[idx],
            .Then => if (_.then_from_else_part) _.else_parts[_.active_else_part].ids[idx] else _.then_stmts.ids[idx],
            .ElifCond, .ElseBranch => _.else_parts[_.active_else_part].ids[idx],
            .Idle, .Done => 0,
        };
    }

    /// 子语句执行完毕回调（对齐 Bun `If.childDone`）
    fn onStmtDone(*_, exit_code: u8): ShellIfError!bool {
        if (!_.awaiting_child) return err.NotAwaitingStmt;
        _.awaiting_child = false;
        _.last_exit_code = exit_code;
        _.executed_count += 1;
        return true;
    }

    /// 推进控制流状态机一步（对齐 Bun `If.next`）
    fn nextStep(*_): ShellIfError!IfStepResult {
        if (_.awaiting_child) return err.StmtStillPending;

        while (_.phase != .Done) {
            if (_.phase == .Idle) {
                _.phase = .Cond;
                _.stmt_idx = 0;
                _.last_exit_code = 0;
                _.then_from_else_part = false;
                continue;
            }

            const cur_len = _.currentListLen();
            if (_.stmt_idx >= cur_len) {
                switch (_.phase) {
                    .Cond => {
                        if (_.last_exit_code == 0) {
                            _.phase = .Then;
                            _.then_from_else_part = false;
                            _.stmt_idx = 0;
                            _.last_exit_code = 0;
                            continue;
                        }
                        if (_.else_part_count == 0) {
                            _.phase = .Done;
                            _.final_exit_code = 0;
                            return IfStepResult{ kind: .Completed, phase: .Done, stmt_id: 0, exit_code: 0 };
                        }
                        if (_.else_part_count == 1) {
                            _.phase = .ElseBranch;
                            _.active_else_part = 0;
                            _.stmt_idx = 0;
                            _.last_exit_code = 0;
                            continue;
                        }
                        _.phase = .ElifCond;
                        _.active_else_part = 0;
                        _.stmt_idx = 0;
                        _.last_exit_code = 0;
                        continue;
                    },
                    .Then, .ElseBranch => {
                        _.phase = .Done;
                        _.final_exit_code = _.last_exit_code;
                        return IfStepResult{
                            kind: .Completed,
                            phase: .Done,
                            stmt_id: 0,
                            exit_code: _.final_exit_code,
                        };
                    },
                    .ElifCond => {
                        if (_.last_exit_code == 0) {
                            _.phase = .Then;
                            _.active_else_part += 1;
                            _.then_from_else_part = true;
                            _.stmt_idx = 0;
                            _.last_exit_code = 0;
                            continue;
                        }
                        _.active_else_part += 2;
                        if (_.active_else_part >= _.else_part_count) {
                            _.phase = .Done;
                            _.final_exit_code = 0;
                            return IfStepResult{ kind: .Completed, phase: .Done, stmt_id: 0, exit_code: 0 };
                        }
                        if (_.active_else_part == _.else_part_count - 1) {
                            _.phase = .ElseBranch;
                            _.stmt_idx = 0;
                            _.last_exit_code = 0;
                            continue;
                        }
                        _.stmt_idx = 0;
                        _.last_exit_code = 0;
                        continue;
                    },
                    .Idle, .Done => {},
                }
            }

            const sid = _.currentListStmt(_.stmt_idx);
            _.stmt_idx += 1;
            _.awaiting_child = true;
            return IfStepResult{
                kind: .RunStmt,
                phase: _.phase,
                stmt_id: sid,
                exit_code: 0,
            };
        }

        return IfStepResult{
            kind: .Completed,
            phase: .Done,
            stmt_id: 0,
            exit_code: _.final_exit_code,
        };
    }
}
