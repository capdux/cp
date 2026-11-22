// Source: Absorbed from /data/dev/bun/src/shell/states/Subshell.cp | Synced: 2026-11-22T12:55:06Z
// Pure-CP zero-allocation Bun Shell Subshell state machine:
// Idle -> ExpandingRedirect -> Exec -> WaitWriteErr -> Done lifecycle,
// isolated environment & CWD snapshot semantics (never mutating parent shell),
// redirect target validation, and Bun Shell `.throws()` / `.nothrow()` / `.quiet()` exit-code policy.

/// 子 Shell 重定向路径固定缓冲区最大长度
pub const SUBSHELL_MAX_REDIRECT_PATH_LEN: usize = 512;

/// 子 Shell 隔离环境快照支持的最大环境变量条目数
pub const SUBSHELL_MAX_ENV_SNAPSHOT: usize = 32;

/// 子 Shell 状态机错误集
pub err SubshellError {
    InvalidTransition,
    InvalidRedirectPath,
    RedirectPathTooLong,
    InvalidEnvKey,
    EnvSnapshotFull,
    AlreadyFinished,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn hasControlChar(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] < 0x20 || s[i] == 0x7f) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 子 Shell 宿主父节点类型（对齐 Bun `Subshell.ParentPtr`）
pub enum SubshellParentKind {
    Pipeline,
    Binary,
    Stmt,
}

/// 子 Shell 执行阶段状态（对齐 Bun `Subshell.RunState`）
pub enum SubshellRunState {
    Idle,
    ExpandingRedirect,
    Exec,
    WaitWriteErr,
    Done,
}

/// 子 Shell 外层重定向类型
pub enum SubshellRedirectKind {
    None,
    WriteTrunc,
    WriteAppend,
    Read,
    ErrTrunc,
    ErrAppend,
    BothTrunc,
}

/// 子 Shell 执行与隔离策略（对齐 Bun Shell `.throws()` / `.nothrow()` / `.quiet()` 及子 Shell 隔离语义）
pub struct SubshellExecPolicy {
    throw_on_nonzero: bool = true,
    quiet: bool = false,
    isolate_env: bool = true,
    isolate_cwd: bool = true,
}

/// 子 Shell 隔离环境变量条目
pub struct SubshellEnvEntry {
    key: []const u8 = "",
    value: []const u8 = "",
}

/// 单步推进的动作指令
pub enum SubshellStepKind {
    ExpandRedirect,
    ExecuteScript,
    WriteError,
    Completed,
}

/// 状态机单步推进结果
pub struct SubshellStepAction {
    kind: SubshellStepKind = SubshellStepKind.Completed,
    state: SubshellRunState = SubshellRunState.Done,
    redirect_kind: SubshellRedirectKind = SubshellRedirectKind.None,
    exit_code: i32 = 0,
    should_throw: bool = false,
}

/// 零堆分配 Bun Shell 子 Shell 状态机（对齐 `src/shell/states/Subshell.cp`）
pub struct SubshellStateMachine {
    parent: SubshellParentKind = SubshellParentKind.Stmt,
    state: SubshellRunState = SubshellRunState.Idle,
    redirect_kind: SubshellRedirectKind = SubshellRedirectKind.None,
    policy: SubshellExecPolicy = SubshellExecPolicy{},
    redirect_path_buf: [SUBSHELL_MAX_REDIRECT_PATH_LEN]u8 = [0; SUBSHELL_MAX_REDIRECT_PATH_LEN],
    redirect_path_len: usize = 0,
    redirect_expanded: bool = false,
    env_snapshot: [SUBSHELL_MAX_ENV_SNAPSHOT]SubshellEnvEntry = [SubshellEnvEntry{}; SUBSHELL_MAX_ENV_SNAPSHOT],
    env_len: usize = 0,
    local_cwd: []const u8 = "",
    local_env_mutated: bool = false,
    local_cwd_mutated: bool = false,
    exit_code: i32 = 0,

    static fn init(
        parent: SubshellParentKind,
        redirect_kind: SubshellRedirectKind,
        policy: SubshellExecPolicy,
    ): SubshellStateMachine {
        return SubshellStateMachine{
            parent: parent,
            state: SubshellRunState.Idle,
            redirect_kind: redirect_kind,
            policy: policy,
        };
    }

    fn findEnvIndex(key: []const u8): usize? {
        let i: usize = 0;
        while (i < _.env_len) {
            if (bytesEql(_.env_snapshot[i].key, key)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 启动子 Shell 前复制父 Shell 环境变量快照（对齐 `dupeForSubshell`）
    fn captureParentEnv(*_, key: []const u8, value: []const u8): bool! {
        if (key.len == 0 || hasControlChar(key)) {
            return err.InvalidEnvKey;
        }
        if (idx := _.findEnvIndex(key)) {
            _.env_snapshot[idx].value = value;
            return true;
        }
        if (_.env_len >= SUBSHELL_MAX_ENV_SNAPSHOT) {
            return err.EnvSnapshotFull;
        }
        _.env_snapshot[_.env_len] = SubshellEnvEntry{ key: key, value: value };
        _.env_len += 1;
        return true;
    }

    /// 在子 Shell 内部修改环境变量（隔离生效，不污染父 Shell）
    fn setLocalEnv(*_, key: []const u8, value: []const u8): bool! {
        const ok = _.captureParentEnv(key, value)?;
        _.local_env_mutated = true;
        return ok;
    }

    fn getLocalEnv(key: []const u8): ([]const u8)? {
        const idx = _.findEnvIndex(key) ?? return null;
        return _.env_snapshot[idx].value;
    }

    /// 在子 Shell 内部切换工作目录（隔离生效，不污染父 Shell）
    fn setLocalCwd(*_, path: []const u8): bool! {
        if (path.len == 0 || hasControlChar(path)) {
            return err.InvalidRedirectPath;
        }
        _.local_cwd = path;
        _.local_cwd_mutated = true;
        return true;
    }

    /// 子 Shell 默认严格隔离环境变量与 CWD，不向父 Shell 传播副作用
    fn propagatesSideEffectsToParent(): bool {
        return !_.policy.isolate_env && !_.policy.isolate_cwd;
    }

    /// 按状态机规则推进一步：
    /// - `Idle`: 有重定向则迁入 `ExpandingRedirect` 返回 `ExpandRedirect`，否则直接迁入 `Exec` 返回 `ExecuteScript`；
    /// - `ExpandingRedirect`: 若已展开则迁入 `Exec`，否则继续等待 `onRedirectExpanded`；
    /// - `Exec`: 返回 `ExecuteScript`；
    /// - `WaitWriteErr`: 返回 `WriteError`；
    /// - `Done`: 返回 `Completed`（附带 `exit_code` 与 `should_throw`）。
    fn nextStep(*_): SubshellStepAction! {
        switch (_.state) {
            SubshellRunState.Idle => {
                if (_.redirect_kind != SubshellRedirectKind.None) {
                    _.state = SubshellRunState.ExpandingRedirect;
                    return SubshellStepAction{
                        kind: SubshellStepKind.ExpandRedirect,
                        state: _.state,
                        redirect_kind: _.redirect_kind,
                    };
                }
                _.state = SubshellRunState.Exec;
                return SubshellStepAction{
                    kind: SubshellStepKind.ExecuteScript,
                    state: _.state,
                    redirect_kind: _.redirect_kind,
                };
            },
            SubshellRunState.ExpandingRedirect => {
                if (_.redirect_expanded) {
                    _.state = SubshellRunState.Exec;
                    return SubshellStepAction{
                        kind: SubshellStepKind.ExecuteScript,
                        state: _.state,
                        redirect_kind: _.redirect_kind,
                    };
                }
                return SubshellStepAction{
                    kind: SubshellStepKind.ExpandRedirect,
                    state: _.state,
                    redirect_kind: _.redirect_kind,
                };
            },
            SubshellRunState.Exec => {
                return SubshellStepAction{
                    kind: SubshellStepKind.ExecuteScript,
                    state: _.state,
                    redirect_kind: _.redirect_kind,
                };
            },
            SubshellRunState.WaitWriteErr => {
                return SubshellStepAction{
                    kind: SubshellStepKind.WriteError,
                    state: _.state,
                    redirect_kind: _.redirect_kind,
                    exit_code: _.exit_code,
                    should_throw: _.policy.throw_on_nonzero && _.exit_code != 0,
                };
            },
            SubshellRunState.Done => {
                return SubshellStepAction{
                    kind: SubshellStepKind.Completed,
                    state: _.state,
                    redirect_kind: _.redirect_kind,
                    exit_code: _.exit_code,
                    should_throw: _.policy.throw_on_nonzero && _.exit_code != 0,
                };
            },
        }
    }

    /// 重定向目标词展开完成回调，校验路径并迁入 `Exec`
    fn onRedirectExpanded(*_, resolved_path: []const u8): bool! {
        if (_.state != SubshellRunState.ExpandingRedirect) {
            return err.InvalidTransition;
        }
        if (resolved_path.len == 0 || hasControlChar(resolved_path)) {
            return err.InvalidRedirectPath;
        }
        if (resolved_path.len > SUBSHELL_MAX_REDIRECT_PATH_LEN) {
            return err.RedirectPathTooLong;
        }
        let i: usize = 0;
        while (i < resolved_path.len) {
            _.redirect_path_buf[i] = resolved_path[i];
            i += 1;
        }
        _.redirect_path_len = resolved_path.len;
        _.redirect_expanded = true;
        _.state = SubshellRunState.Exec;
        return true;
    }

    /// 重定向展开或打开失败回调，记录非零退出码并迁入 `WaitWriteErr`（若开启 `.quiet()` 直接完成）
    fn onRedirectError(*_, err_exit_code: i32): bool! {
        if (_.state != SubshellRunState.ExpandingRedirect && _.state != SubshellRunState.Exec) {
            return err.InvalidTransition;
        }
        _.exit_code = if (err_exit_code == 0) 1 else err_exit_code;
        if (_.policy.quiet) {
            _.state = SubshellRunState.Done;
        } else {
            _.state = SubshellRunState.WaitWriteErr;
        }
        return true;
    }

    /// 错误诊断输出完毕回调，从 `WaitWriteErr` 迁入 `Done`
    fn onErrorFlushed(*_): bool! {
        if (_.state != SubshellRunState.WaitWriteErr) {
            return err.InvalidTransition;
        }
        _.state = SubshellRunState.Done;
        return true;
    }

    /// 子脚本执行结束回调，记录退出码并迁入 `Done`
    fn onScriptFinished(*_, exit_code: i32): bool! {
        if (_.state == SubshellRunState.Done) {
            return err.AlreadyFinished;
        }
        if (_.state != SubshellRunState.Exec) {
            return err.InvalidTransition;
        }
        _.exit_code = exit_code;
        _.state = SubshellRunState.Done;
        return true;
    }

    fn resolvedRedirectPath(): []const u8 {
        return _.redirect_path_buf[0.._.redirect_path_len];
    }

    fn isDone(): bool {
        return _.state == SubshellRunState.Done;
    }

    fn exitCode(): i32 {
        return _.exit_code;
    }

    fn shouldThrow(): bool {
        return _.policy.throw_on_nonzero && _.exit_code != 0;
    }
}
