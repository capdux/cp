// Source: Absorbed from /data/dev/bun/src/shell/states/Stmt.cp | Synced: 2026-11-22T20:42:08Z (Bun Shell Stmt child-expression dispatch state machine for Script & If parent contexts)
// Pure-CP zero-allocation shell statement execution state machine with sequential child dispatch and exit-code propagation.

/// Maximum number of sequential child expressions inside a single `StmtState`.
pub const SHELL_STMT_MAX_CHILDREN: usize = 32;

/// Parent container kind owning a `StmtState` (aligned with Bun `Stmt.ParentPtr`).
pub enum StmtParentKind: u8 {
    Script = 0,
    If = 1,
}

/// Child expression kind dispatched by `StmtState` (aligned with Bun `Stmt.ChildPtr`).
pub enum StmtChildKind: u8 {
    Async = 0,
    Binary = 1,
    Pipeline = 2,
    Cmd = 3,
    Assign = 4,
    If = 5,
    CondExpr = 6,
    Subshell = 7,
}

/// Error set for `StmtState` transitions.
pub err StmtError {
    TooManyChildren,
    ChildInFlight,
    NoChildInFlight,
    AlreadyFinished,
}

/// Scheduling action returned by `StmtState.next()`.
pub enum StmtAction: u8 {
    DispatchChild = 0,
    Finish = 1,
}

/// Step descriptor returned by `StmtState.next()`.
pub struct StmtStep {
    action: StmtAction = StmtAction.Finish,
    child_index: usize = 0,
    child_kind: StmtChildKind = StmtChildKind.Cmd,
    exit_code: u8 = 0,
}

/// Zero-allocation Bun Shell `Stmt` sequential child-expression state machine.
pub struct StmtState {
    parent: StmtParentKind = StmtParentKind.Script,
    children: [SHELL_STMT_MAX_CHILDREN]StmtChildKind = [StmtChildKind.Cmd; SHELL_STMT_MAX_CHILDREN],
    len: usize = 0,
    idx: usize = 0,
    in_flight: bool = false,
    finished: bool = false,
    last_exit_code: u8? = null,
    completed_count: usize = 0,

    /// Initializes a new empty `StmtState` for `parent` (`Script` or `If`).
    static fn init(parent: StmtParentKind): StmtState {
        return StmtState{
            parent: parent,
        };
    }

    /// Appends a child expression kind to the statement before execution starts.
    fn pushChild(*_, kind: StmtChildKind): usize ! StmtError {
        if (_.len >= SHELL_STMT_MAX_CHILDREN) return StmtError.TooManyChildren;
        const pos = _.len;
        _.children[pos] = kind;
        _.len += 1;
        return pos;
    }

    /// Returns the number of child expressions not yet completed.
    fn remaining(): usize {
        if (_.idx >= _.len) return 0;
        return _.len - _.idx;
    }

    /// Returns the final statement exit code (`last_exit_code ?? 0`, matching POSIX & Bun `Stmt`).
    fn finalExitCode(): u8 {
        if (c := _.last_exit_code) return c;
        return 0;
    }

    /// Advances the statement state machine (aligned with Bun `Stmt.next()`).
    /// Dispatches the next child expression or completes with `last_exit_code ?? 0`.
    fn next(*_): StmtStep ! StmtError {
        if (_.finished) return StmtError.AlreadyFinished;
        if (_.in_flight) return StmtError.ChildInFlight;

        if (_.idx >= _.len) {
            _.finished = true;
            return StmtStep{
                action: StmtAction.Finish,
                child_index: _.idx,
                child_kind: StmtChildKind.Cmd,
                exit_code: _.finalExitCode(),
            };
        }

        const child_idx = _.idx;
        const kind = _.children[child_idx];
        _.in_flight = true;
        return StmtStep{
            action: StmtAction.DispatchChild,
            child_index: child_idx,
            child_kind: kind,
            exit_code: 0,
        };
    }

    /// Marks the currently executing child expression as finished (aligned with Bun `Stmt.childDone()`).
    /// Returns the next child index.
    fn childDone(*_, exit_code: u8): usize ! StmtError {
        if (_.finished) return StmtError.AlreadyFinished;
        if (!_.in_flight) return StmtError.NoChildInFlight;

        _.last_exit_code = exit_code;
        _.in_flight = false;
        _.idx += 1;
        _.completed_count += 1;
        return _.idx;
    }

    /// Resets execution cursor and status while preserving the registered child list.
    fn reset(*_) {
        _.idx = 0;
        _.in_flight = false;
        _.finished = false;
        _.last_exit_code = null;
        _.completed_count = 0;
    }
}
