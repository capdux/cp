// Source: Absorbed from /data/dev/bun/src/shell/states/Script.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation POSIX / Bun Shell Top-Level Script & Statement Sequence State Machine
// Aligned with Bun src/shell/states/{Script,Stmt}.cp & docs/runtime/shell.mdx:
// - Models all 3 script host contexts: `TopLevel` (Interpreter), `Subshell` (`(...)`), and `CommandSubstitution` (`$(...)`)
// - Dispatches all 8 shell statement expression kinds (`Cmd`, `Binary`, `Pipeline`, `Assign`, `If`, `CondExpr`, `Subshell`, `Async`)
// - Empty scripts (`len == 0`) immediately finish with exit code 0
// - Supports both Bun Shell's default fail-fast (`errexit` / `.throws(true)`) and POSIX continue-on-error (`nothrow`) execution modes
// - Respects POSIX `errexit` exemptions (`exempt_from_errexit = true` for guarded/conditional statements)
// - 100% zero heap allocation with step-by-step `nextStep()` / `onExprDone(exit_code)` driving.

/// 单个脚本状态机支持的最大表达式条目数
pub const SHELL_SCRIPT_MAX_EXPRS: usize = 64;

/// 脚本/语句顺序执行状态机错误集
pub err ShellScriptError {
    TooManyExprs,
    AlreadyStarted,
    ExprStillPending,
    NotAwaitingExpr,
}

/// 脚本宿主上下文（对齐 Bun `Script.ParentPtr`）
pub enum ScriptHostKind: u8 {
    TopLevel = 0,
    Subshell = 1,
    CommandSubstitution = 2,
}

/// 语句内部表达式节点类型（对齐 Bun `Stmt.ChildPtr`）
pub enum ShellExprKind: u8 {
    Cmd = 0,
    Binary = 1,
    Pipeline = 2,
    Assign = 3,
    If = 4,
    CondExpr = 5,
    Subshell = 6,
    Async = 7,
}

/// 步进调度动作类型
pub enum ScriptStepKind: u8 {
    /// 启动第 `expr_index` 项表达式，完成后调用 `onExprDone(exit_code)`
    RunExpr = 0,
    /// 脚本正常执行完所有语句（或为空脚本）
    Completed = 1,
    /// 命中 `errexit` / `.throws(true)` 提前中止脚本
    AbortedOnError = 2,
}

/// 单条待执行表达式描述符
pub struct ScriptExprItem {
    kind: ShellExprKind = .Cmd,
    expr_id: u32 = 0,
    /// 是否豁免 `errexit`（如属于 `if` 条件或 `&&`/`||` 左操作数）
    exempt_from_errexit: bool = false,
}

/// `nextStep()` 返回的单步结果
pub struct ScriptStepResult {
    kind: ScriptStepKind = .Completed,
    expr_index: usize = 0,
    expr_kind: ShellExprKind = .Cmd,
    expr_id: u32 = 0,
    exit_code: u8 = 0,
}

/// 判断脚本在指定宿主上下文中退出时是否允许向外层父 Shell 传播环境副作用
pub fn scriptPropagatesEnvToParent(host: ScriptHostKind): bool {
    return host == .TopLevel;
}

/// 零堆分配 POSIX / Bun Shell 脚本与语句序列状态机
pub struct ShellScriptStateMachine {
    host: ScriptHostKind = .TopLevel,
    /// 对齐 Bun Shell `.throws(true)` / POSIX `set -e`
    errexit: bool = false,
    exprs: [SHELL_SCRIPT_MAX_EXPRS]ScriptExprItem = [ScriptExprItem{}; SHELL_SCRIPT_MAX_EXPRS],
    expr_count: usize = 0,
    cursor: usize = 0,
    started: bool = false,
    awaiting_expr: bool = false,
    aborted: bool = false,
    done: bool = false,
    last_exit_code: u8 = 0,
    executed_count: usize = 0,

    static fn init(host: ScriptHostKind, errexit: bool): ShellScriptStateMachine {
        let m = ShellScriptStateMachine{};
        m.host = host;
        m.errexit = errexit;
        return m;
    }

    fn hostKind(): ScriptHostKind {
        return _.host;
    }

    fn isErrexitEnabled(): bool {
        return _.errexit;
    }

    fn exprCount(): usize {
        return _.expr_count;
    }

    fn executedCount(): usize {
        return _.executed_count;
    }

    fn isDone(): bool {
        return _.done;
    }

    fn isAborted(): bool {
        return _.aborted;
    }

    fn finalExitCode(): u8 {
        return _.last_exit_code;
    }

    /// 追加一条待执行的语句表达式
    fn pushExpr(*_, kind: ShellExprKind, expr_id: u32, exempt_from_errexit: bool): ShellScriptError!usize {
        if (_.started) return err.AlreadyStarted;
        if (_.expr_count >= SHELL_SCRIPT_MAX_EXPRS) return err.TooManyExprs;
        const idx = _.expr_count;
        _.exprs[idx] = ScriptExprItem{
            kind: kind,
            expr_id: expr_id,
            exempt_from_errexit: exempt_from_errexit,
        };
        _.expr_count += 1;
        return idx;
    }

    /// 当前表达式执行完毕回调（对齐 Bun `Script.childDone` + `Stmt.childDone`）
    fn onExprDone(*_, exit_code: u8): ShellScriptError!bool {
        if (!_.awaiting_expr) return err.NotAwaitingExpr;
        const finished_idx = _.cursor - 1;
        _.awaiting_expr = false;
        _.last_exit_code = exit_code;
        _.executed_count += 1;

        if (_.errexit && exit_code != 0 && !_.exprs[finished_idx].exempt_from_errexit) {
            _.aborted = true;
            _.done = true;
            return false;
        }
        return true;
    }

    /// 推进一步脚本执行状态机
    fn nextStep(*_): ShellScriptError!ScriptStepResult {
        if (_.awaiting_expr) return err.ExprStillPending;

        if (_.aborted) {
            return ScriptStepResult{
                kind: .AbortedOnError,
                expr_index: _.cursor,
                expr_kind: .Cmd,
                expr_id: 0,
                exit_code: _.last_exit_code,
            };
        }
        if (_.done) {
            return ScriptStepResult{
                kind: .Completed,
                expr_index: _.cursor,
                expr_kind: .Cmd,
                expr_id: 0,
                exit_code: _.last_exit_code,
            };
        }

        _.started = true;

        // 空脚本或全部语句执行完毕：返回最后退出码（空脚本为 0）
        if (_.cursor >= _.expr_count) {
            _.done = true;
            return ScriptStepResult{
                kind: .Completed,
                expr_index: _.cursor,
                expr_kind: .Cmd,
                expr_id: 0,
                exit_code: _.last_exit_code,
            };
        }

        const idx = _.cursor;
        const item = _.exprs[idx];
        _.cursor += 1;
        _.awaiting_expr = true;
        return ScriptStepResult{
            kind: .RunExpr,
            expr_index: idx,
            expr_kind: item.kind,
            expr_id: item.expr_id,
            exit_code: 0,
        };
    }
}
