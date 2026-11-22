// Source: Absorbed from /data/dev/bun/src/shell/states/Binary.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation POSIX / Bun Shell `&&` / `||` Short-Circuit Binary Logic State Machine
// Aligned with Bun src/shell/states/Binary.cp & docs/runtime/shell.mdx:
// - Implements POSIX AND-OR list evaluation with equal precedence and strict left-associativity:
//     * `&&` short-circuits (skips RHS) whenever previous exit code != 0
//     * `||` short-circuits (skips RHS) whenever previous exit code == 0
//     * Empty child nodes (`is_empty == true`, mirroring Bun `makeChild == null`) resolve immediately with exit code 0
// - Drives step-by-step via `nextStep()` / `onTermDone(exit_code)` with zero heap allocation and full skip telemetry.

/// 单条 AND-OR 逻辑链支持的最大项数
pub const SHELL_BINARY_MAX_TERMS: usize = 32;

/// `&&` / `||` 二元逻辑状态机错误集
pub err ShellBinaryError {
    TooManyTerms,
    EmptyChain,
    InvalidOperator,
    AlreadyStarted,
    TermStillPending,
    NotAwaitingTerm,
}

/// 二元逻辑连接符（对齐 Bun `ast.Binary.Op`）
pub enum BinaryLogicOp: u8 {
    /// 首项无前置连接符
    None = 0,
    /// `&&`：前一项退出码 == 0 时才执行本项
    And = 1,
    /// `||`：前一项退出码 != 0 时才执行本项
    Or = 2,
}

/// 单步调度动作类型
pub enum BinaryStepKind: u8 {
    /// 调度器应执行 `term_id` 对应子命令，完成后调用 `onTermDone(exit_code)`
    RunTerm = 0,
    /// 整个 AND-OR 逻辑链执行结束，`exit_code` 为最终退出码
    Completed = 1,
}

/// 逻辑链中的单个命令项
pub struct BinaryTerm {
    op_before: BinaryLogicOp = .None,
    term_id: u32 = 0,
    /// 对齐 Bun `Binary.makeChild == null`（空命令项视为立即以退出码 0 完成）
    is_empty: bool = false,
}

/// `nextStep()` 返回的单步调度结果
pub struct BinaryStepResult {
    kind: BinaryStepKind = .Completed,
    term_index: usize = 0,
    term_id: u32 = 0,
    exit_code: u8 = 0,
    /// 截至目前已被短路跳过的项数
    skipped_count: usize = 0,
}

/// 纯函数判定：根据连接符与前序退出码判断是否应短路跳过下一项
pub fn shouldShortCircuitBinary(op: BinaryLogicOp, prev_exit_code: u8): bool {
    return switch (op) {
        .And => prev_exit_code != 0,
        .Or => prev_exit_code == 0,
        .None => false,
    };
}

/// 零堆分配 POSIX / Bun Shell `&&` / `||` 短路二元逻辑链状态机
pub struct ShellBinaryStateMachine {
    terms: [SHELL_BINARY_MAX_TERMS]BinaryTerm = [BinaryTerm{}; SHELL_BINARY_MAX_TERMS],
    term_count: usize = 0,
    cursor: usize = 0,
    started: bool = false,
    done: bool = false,
    awaiting_term: bool = false,
    last_exit_code: u8 = 0,
    executed_count: usize = 0,
    skipped_count: usize = 0,

    static fn init(): ShellBinaryStateMachine {
        return ShellBinaryStateMachine{};
    }

    fn termCount(): usize {
        return _.term_count;
    }

    fn isDone(): bool {
        return _.done;
    }

    fn finalExitCode(): u8 {
        return _.last_exit_code;
    }

    fn executedCount(): usize {
        return _.executed_count;
    }

    fn skippedCount(): usize {
        return _.skipped_count;
    }

    /// 设置链条首项（无前置连接符）
    fn setFirstTerm(*_, term_id: u32, is_empty: bool): ShellBinaryError!usize {
        if (_.started) return err.AlreadyStarted;
        if (_.term_count > 0) return err.InvalidOperator;
        _.terms[0] = BinaryTerm{
            op_before: .None,
            term_id: term_id,
            is_empty: is_empty,
        };
        _.term_count = 1;
        return 0;
    }

    /// 按 POSIX 左结合规则追加 `&& term` 或 `|| term`
    fn pushTerm(*_, op: BinaryLogicOp, term_id: u32, is_empty: bool): ShellBinaryError!usize {
        if (_.started) return err.AlreadyStarted;
        if (_.term_count == 0 || op == .None) return err.InvalidOperator;
        if (_.term_count >= SHELL_BINARY_MAX_TERMS) return err.TooManyTerms;
        const idx = _.term_count;
        _.terms[idx] = BinaryTerm{
            op_before: op,
            term_id: term_id,
            is_empty: is_empty,
        };
        _.term_count += 1;
        return idx;
    }

    /// 子项执行完毕回调（对齐 Bun `Binary.childDone`）
    fn onTermDone(*_, exit_code: u8): ShellBinaryError!bool {
        if (!_.awaiting_term) return err.NotAwaitingTerm;
        _.awaiting_term = false;
        _.last_exit_code = exit_code;
        _.executed_count += 1;
        return true;
    }

    /// 推进一步状态机：左结合处理 `&&` / `||` 短路与空项，返回下一个需执行的项或 `Completed`
    fn nextStep(*_): ShellBinaryError!BinaryStepResult {
        if (_.term_count == 0) return err.EmptyChain;
        if (_.awaiting_term) return err.TermStillPending;

        _.started = true;

        while (_.cursor < _.term_count) {
            const idx = _.cursor;
            const term = _.terms[idx];
            _.cursor += 1;

            // 对齐 POSIX 左结合 AND-OR 短路规则
            if (idx > 0 && shouldShortCircuitBinary(term.op_before, _.last_exit_code)) {
                _.skipped_count += 1;
                continue;
            }

            // 对齐 Bun `Binary.makeChild == null`：空子节点视为退出码 0
            if (term.is_empty) {
                _.last_exit_code = 0;
                continue;
            }

            _.awaiting_term = true;
            return BinaryStepResult{
                kind: .RunTerm,
                term_index: idx,
                term_id: term.term_id,
                exit_code: 0,
                skipped_count: _.skipped_count,
            };
        }

        _.done = true;
        return BinaryStepResult{
            kind: .Completed,
            term_index: _.term_count,
            term_id: 0,
            exit_code: _.last_exit_code,
            skipped_count: _.skipped_count,
        };
    }
}
