// Source: Absorbed from /data/dev/bun/src/shell/states/Cmd.cp | Synced: 2026-11-23T01:30:14Z (Shell Cmd 7-phase execution state machine: assigns/redirect/args expansion, builtin/subprocess dispatch & BufferedIoClosed barrier)
// Pure-CP zero-allocation Shell `Cmd` execution state machine and piped stdio completion tracker.

/// Parent AST container of a `CmdStateMachine` (aligned with Bun `Cmd.ParentPtr`).
pub enum CmdParentKind: u8 {
    Stmt = 0,
    Binary = 1,
    Pipeline = 2,
    Async = 3,
}

/// Active execution backend of a `CmdStateMachine` (aligned with Bun `Cmd.Exec`).
pub enum CmdExecKind: u8 {
    None = 0,
    Builtin = 1,
    Subprocess = 2,
}

/// 7-phase lifecycle state of `CmdStateMachine` (aligned with Bun `Cmd.CmdState`).
pub enum CmdPhase: u8 {
    Idle = 0,
    ExpandingAssigns = 1,
    ExpandingRedirect = 2,
    ExpandingArgs = 3,
    Exec = 4,
    WaitingWriteErr = 5,
    Done = 6,
}

/// Standard I/O channel identifier (aligned with Bun `Cmd.IoKind`).
pub enum CmdIoChannel: u8 {
    Stdin = 0,
    Stdout = 1,
    Stderr = 2,
}

/// Action requested by `CmdStateMachine.next()`.
pub enum CmdStepAction: u8 {
    ExpandAssigns = 0,
    ExpandRedirect = 1,
    ExpandArg = 2,
    RunExec = 3,
    WaitExec = 4,
    WaitWriteErr = 5,
    Finish = 6,
}

/// Error set for `CmdStateMachine` transitions.
pub err CmdStateError {
    InvalidPhase,
    StepInFlight,
    NoStepInFlight,
    AlreadyDone,
}

/// Descriptor returned by `CmdStateMachine.next()`.
pub struct CmdStep {
    action: CmdStepAction = CmdStepAction.Finish,
    index: u32 = 0,
    exit_code: u8 = 0,
}

/// Tracks closure of piped stdin/stdout/stderr buffers before a subprocess command can finish
/// (aligned with Bun `Cmd.BufferedIoClosed`).
pub struct CmdBufferedIoTracker {
    stdin_piped: bool = false,
    stdin_closed: bool = false,
    stdout_piped: bool = false,
    stdout_closed: bool = false,
    stderr_piped: bool = false,
    stderr_closed: bool = false,

    static fn fromPiped(stdin_piped: bool, stdout_piped: bool, stderr_piped: bool): CmdBufferedIoTracker {
        return CmdBufferedIoTracker{
            stdin_piped: stdin_piped,
            stdout_piped: stdout_piped,
            stderr_piped: stderr_piped,
        };
    }

    fn isBuffered(ch: CmdIoChannel): bool {
        return switch (ch) {
            .Stdin => _.stdin_piped,
            .Stdout => _.stdout_piped,
            .Stderr => _.stderr_piped,
        };
    }

    fn closeChannel(*_, ch: CmdIoChannel) {
        switch (ch) {
            .Stdin => {
                if (_.stdin_piped) _.stdin_closed = true;
            },
            .Stdout => {
                if (_.stdout_piped) _.stdout_closed = true;
            },
            .Stderr => {
                if (_.stderr_piped) _.stderr_closed = true;
            },
        }
    }

    /// Returns true when all piped channels have closed (unpiped channels are trivially closed).
    fn allClosed(): bool {
        const in_ok = !_.stdin_piped || _.stdin_closed;
        const out_ok = !_.stdout_piped || _.stdout_closed;
        const err_ok = !_.stderr_piped || _.stderr_closed;
        return in_ok && out_ok && err_ok;
    }
}

/// Zero-allocation Shell `Cmd` execution state machine (aligned with Bun `shell/states/Cmd.cp`).
pub struct CmdStateMachine {
    parent_kind: CmdParentKind = CmdParentKind.Stmt,
    phase: CmdPhase = CmdPhase.Idle,
    exec_kind: CmdExecKind = CmdExecKind.None,
    has_assigns: bool = false,
    assigns_done: bool = false,
    redirect_count: u32 = 0,
    redirect_idx: u32 = 0,
    arg_count: u32 = 0,
    arg_idx: u32 = 0,
    in_flight: bool = false,
    subproc_exited: bool = false,
    io_tracker: CmdBufferedIoTracker = CmdBufferedIoTracker{},
    exit_code: u8 = 0,

    static fn init(
        parent_kind: CmdParentKind,
        has_assigns: bool,
        redirect_count: u32,
        arg_count: u32,
    ): CmdStateMachine {
        return CmdStateMachine{
            parent_kind: parent_kind,
            has_assigns: has_assigns,
            redirect_count: redirect_count,
            arg_count: arg_count,
        };
    }

    fn isDone(): bool {
        return _.phase == CmdPhase.Done;
    }

    /// Advances the command state machine to the next required step:
    /// `Assigns -> Redirects[0..N] -> Args[0..M] -> Exec -> Done`.
    fn next(*_): CmdStep ! CmdStateError {
        if (_.phase == CmdPhase.Done) {
            return CmdStep{ action: CmdStepAction.Finish, exit_code: _.exit_code };
        }
        if (_.phase == CmdPhase.WaitingWriteErr) {
            return CmdStep{ action: CmdStepAction.WaitWriteErr, exit_code: _.exit_code };
        }
        if (_.phase == CmdPhase.Exec) {
            return CmdStep{ action: CmdStepAction.WaitExec, exit_code: _.exit_code };
        }
        if (_.in_flight) return CmdStateError.StepInFlight;

        if (_.has_assigns && !_.assigns_done) {
            _.phase = CmdPhase.ExpandingAssigns;
            _.in_flight = true;
            return CmdStep{ action: CmdStepAction.ExpandAssigns, index: 0 };
        }

        if (_.redirect_idx < _.redirect_count) {
            _.phase = CmdPhase.ExpandingRedirect;
            _.in_flight = true;
            return CmdStep{ action: CmdStepAction.ExpandRedirect, index: _.redirect_idx };
        }

        if (_.arg_idx < _.arg_count) {
            _.phase = CmdPhase.ExpandingArgs;
            _.in_flight = true;
            return CmdStep{ action: CmdStepAction.ExpandArg, index: _.arg_idx };
        }

        _.phase = CmdPhase.Exec;
        return CmdStep{ action: CmdStepAction.RunExec };
    }

    fn onAssignsDone(*_): bool ! CmdStateError {
        if (_.phase != CmdPhase.ExpandingAssigns || !_.in_flight) return CmdStateError.InvalidPhase;
        _.assigns_done = true;
        _.in_flight = false;
        return true;
    }

    fn onRedirectDone(*_): u32 ! CmdStateError {
        if (_.phase != CmdPhase.ExpandingRedirect || !_.in_flight) return CmdStateError.InvalidPhase;
        _.redirect_idx += 1;
        _.in_flight = false;
        return _.redirect_idx;
    }

    fn onArgDone(*_): u32 ! CmdStateError {
        if (_.phase != CmdPhase.ExpandingArgs || !_.in_flight) return CmdStateError.InvalidPhase;
        _.arg_idx += 1;
        _.in_flight = false;
        return _.arg_idx;
    }

    /// Begins executing as a native shell builtin.
    fn startBuiltin(*_): bool ! CmdStateError {
        if (_.phase != CmdPhase.Exec) return CmdStateError.InvalidPhase;
        _.exec_kind = CmdExecKind.Builtin;
        _.in_flight = true;
        return true;
    }

    /// Completes a running builtin with `exit_code` and transitions to `Done`.
    fn onBuiltinDone(*_, exit_code: u8): u8 ! CmdStateError {
        if (_.phase != CmdPhase.Exec || _.exec_kind != CmdExecKind.Builtin) return CmdStateError.InvalidPhase;
        _.exit_code = exit_code;
        _.in_flight = false;
        _.phase = CmdPhase.Done;
        return _.exit_code;
    }

    /// Begins executing as an external subprocess with the specified piped stdio channels.
    fn startSubprocess(
        *_,
        stdin_piped: bool,
        stdout_piped: bool,
        stderr_piped: bool,
    ): bool ! CmdStateError {
        if (_.phase != CmdPhase.Exec) return CmdStateError.InvalidPhase;
        _.exec_kind = CmdExecKind.Subprocess;
        _.subproc_exited = false;
        _.io_tracker = CmdBufferedIoTracker.fromPiped(stdin_piped, stdout_piped, stderr_piped);
        _.in_flight = true;
        return true;
    }

    fn tryFinishSubprocess(*_): bool {
        if (_.subproc_exited && _.io_tracker.allClosed()) {
            _.in_flight = false;
            _.phase = CmdPhase.Done;
            return true;
        }
        return false;
    }

    /// Records subprocess exit; transitions to `Done` only once all piped stdio channels have closed.
    fn onSubprocessExited(*_, exit_code: u8): bool ! CmdStateError {
        if (_.phase != CmdPhase.Exec || _.exec_kind != CmdExecKind.Subprocess) return CmdStateError.InvalidPhase;
        _.exit_code = exit_code;
        _.subproc_exited = true;
        return _.tryFinishSubprocess();
    }

    /// Records closure of one piped stdio channel; transitions to `Done` if the process has already exited
    /// and all remaining piped channels are closed.
    fn onSubprocessIoClosed(*_, ch: CmdIoChannel): bool ! CmdStateError {
        if (_.phase != CmdPhase.Exec || _.exec_kind != CmdExecKind.Subprocess) return CmdStateError.InvalidPhase;
        _.io_tracker.closeChannel(ch);
        return _.tryFinishSubprocess();
    }

    /// Enters error path (e.g. command not found -> 127 or redirect failure -> 1).
    /// When `needs_async_err_write` is true, waits in `WaitingWriteErr` until `onErrorWriteDone()`.
    fn failWithError(*_, exit_code: u8, needs_async_err_write: bool): bool ! CmdStateError {
        if (_.phase == CmdPhase.Done) return CmdStateError.AlreadyDone;
        _.exit_code = exit_code;
        _.in_flight = needs_async_err_write;
        _.phase = if (needs_async_err_write) CmdPhase.WaitingWriteErr else CmdPhase.Done;
        return _.phase == CmdPhase.Done;
    }

    /// Completes pending async stderr diagnostic write and transitions to `Done`.
    fn onErrorWriteDone(*_): u8 ! CmdStateError {
        if (_.phase != CmdPhase.WaitingWriteErr) return CmdStateError.InvalidPhase;
        _.in_flight = false;
        _.phase = CmdPhase.Done;
        return _.exit_code;
    }
}
