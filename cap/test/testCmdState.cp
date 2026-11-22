// cap/test/testCmdState.cp
// Production-grade deep tests for cap:shell cmdState.cp
// (Shell Cmd 7-phase execution state machine & BufferedIoClosed barrier absorbed from Bun Cmd.cp)

import { test, assert, assertEq } from "cap:test";
import {
    CmdParentKind,
    CmdExecKind,
    CmdPhase,
    CmdIoChannel,
    CmdStepAction,
    CmdStateError,
    CmdStep,
    CmdBufferedIoTracker,
    CmdStateMachine,
} from "cap:shell";

test("CmdBufferedIoTracker: tracks piped vs unpiped stdio channels", () => {
    const none = CmdBufferedIoTracker.fromPiped(false, false, false);
    assert(none.allClosed());
    assert(!none.isBuffered(CmdIoChannel.Stdin));
    assert(!none.isBuffered(CmdIoChannel.Stdout));
    assert(!none.isBuffered(CmdIoChannel.Stderr));

    let tr = CmdBufferedIoTracker.fromPiped(true, true, false);
    assert(tr.isBuffered(CmdIoChannel.Stdin));
    assert(tr.isBuffered(CmdIoChannel.Stdout));
    assert(!tr.isBuffered(CmdIoChannel.Stderr));
    assert(!tr.allClosed());

    tr.closeChannel(CmdIoChannel.Stdin);
    assert(!tr.allClosed());
    tr.closeChannel(CmdIoChannel.Stdout);
    assert(tr.allClosed());
});

test("CmdStateMachine: full Assigns -> Redirects -> Args -> Builtin lifecycle", () => {
    let cmd = CmdStateMachine.init(CmdParentKind.Stmt, true, 2, 2);
    assertEq(cmd.phase, CmdPhase.Idle);

    // 1. Assigns
    const s0 = cmd.next() catch {
        assert(false);
        return;
    };
    assertEq(s0.action, CmdStepAction.ExpandAssigns);
    assertEq(cmd.phase, CmdPhase.ExpandingAssigns);
    * = cmd.onAssignsDone() catch {
        assert(false);
        return;
    };

    // 2. Redirects (2)
    const s1 = cmd.next() catch {
        assert(false);
        return;
    };
    assertEq(s1.action, CmdStepAction.ExpandRedirect);
    assertEq(s1.index, 0);
    * = cmd.onRedirectDone() catch {
        assert(false);
        return;
    };

    const s2 = cmd.next() catch {
        assert(false);
        return;
    };
    assertEq(s2.action, CmdStepAction.ExpandRedirect);
    assertEq(s2.index, 1);
    * = cmd.onRedirectDone() catch {
        assert(false);
        return;
    };

    // 3. Args (2)
    const s3 = cmd.next() catch {
        assert(false);
        return;
    };
    assertEq(s3.action, CmdStepAction.ExpandArg);
    assertEq(s3.index, 0);
    * = cmd.onArgDone() catch {
        assert(false);
        return;
    };

    const s4 = cmd.next() catch {
        assert(false);
        return;
    };
    assertEq(s4.action, CmdStepAction.ExpandArg);
    assertEq(s4.index, 1);
    * = cmd.onArgDone() catch {
        assert(false);
        return;
    };

    // 4. Exec as Builtin
    const s5 = cmd.next() catch {
        assert(false);
        return;
    };
    assertEq(s5.action, CmdStepAction.RunExec);
    assertEq(cmd.phase, CmdPhase.Exec);

    * = cmd.startBuiltin() catch {
        assert(false);
        return;
    };
    assertEq(cmd.exec_kind, CmdExecKind.Builtin);

    const code = cmd.onBuiltinDone(0) catch {
        assert(false);
        return;
    };
    assertEq(code, 0);
    assert(cmd.isDone());

    const fin = cmd.next() catch {
        assert(false);
        return;
    };
    assertEq(fin.action, CmdStepAction.Finish);
    assertEq(fin.exit_code, 0);
});

test("CmdStateMachine: subprocess waits for both exit and all piped stdio closures", () => {
    // Case A: process exits first, then stdout & stderr pipes close
    let c1 = CmdStateMachine.init(CmdParentKind.Pipeline, false, 0, 0);
    const step = c1.next() catch {
        assert(false);
        return;
    };
    assertEq(step.action, CmdStepAction.RunExec);
    * = c1.startSubprocess(false, true, true) catch {
        assert(false);
        return;
    };

    const done1 = c1.onSubprocessExited(0) catch {
        assert(false);
        return;
    };
    assert(!done1);
    assert(!c1.isDone());

    const done2 = c1.onSubprocessIoClosed(CmdIoChannel.Stdout) catch {
        assert(false);
        return;
    };
    assert(!done2);

    const done3 = c1.onSubprocessIoClosed(CmdIoChannel.Stderr) catch {
        assert(false);
        return;
    };
    assert(done3);
    assert(c1.isDone());
    assertEq(c1.exit_code, 0);

    // Case B: stdout closes before process exits
    let c2 = CmdStateMachine.init(CmdParentKind.Async, false, 0, 0);
    * = c2.next() catch {
        assert(false);
        return;
    };
    * = c2.startSubprocess(false, true, false) catch {
        assert(false);
        return;
    };
    const b1 = c2.onSubprocessIoClosed(CmdIoChannel.Stdout) catch {
        assert(false);
        return;
    };
    assert(!b1);
    const b2 = c2.onSubprocessExited(42) catch {
        assert(false);
        return;
    };
    assert(b2);
    assert(c2.isDone());
    assertEq(c2.exit_code, 42);
});

test("CmdStateMachine: async WaitingWriteErr & synchronous failWithError", () => {
    let c1 = CmdStateMachine.init(CmdParentKind.Binary, false, 0, 1);
    * = c1.next() catch {
        assert(false);
        return;
    };
    const immediate = c1.failWithError(127, true) catch {
        assert(false);
        return;
    };
    assert(!immediate);
    assertEq(c1.phase, CmdPhase.WaitingWriteErr);

    const wait_step = c1.next() catch {
        assert(false);
        return;
    };
    assertEq(wait_step.action, CmdStepAction.WaitWriteErr);

    const code = c1.onErrorWriteDone() catch {
        assert(false);
        return;
    };
    assertEq(code, 127);
    assert(c1.isDone());

    // Synchronous error finishes immediately
    let c2 = CmdStateMachine.init(CmdParentKind.Stmt, false, 0, 0);
    const imm2 = c2.failWithError(1, false) catch {
        assert(false);
        return;
    };
    assert(imm2);
    assert(c2.isDone());
    assertEq(c2.exit_code, 1);
});

test("CmdStateMachine: StepInFlight, InvalidPhase & AlreadyDone guards", () => {
    let cmd = CmdStateMachine.init(CmdParentKind.Stmt, true, 0, 0);
    * = cmd.next() catch {
        assert(false);
        return;
    };

    // Calling next() while assigns step is in flight returns StepInFlight
    let got_inflight = false;
    * = cmd.next() catch |e| {
        assertEq(e, CmdStateError.StepInFlight);
        got_inflight = true;
        CmdStep{}
    };
    assert(got_inflight);

    // Wrong callback in ExpandingAssigns returns InvalidPhase
    let got_bad_phase = false;
    * = cmd.onArgDone() catch |e| {
        assertEq(e, CmdStateError.InvalidPhase);
        got_bad_phase = true;
        0
    };
    assert(got_bad_phase);

    // Calling failWithError after Done returns AlreadyDone
    * = cmd.failWithError(2, false) catch {
        assert(false);
        return;
    };
    let got_done_err = false;
    * = cmd.failWithError(3, false) catch |e| {
        assertEq(e, CmdStateError.AlreadyDone);
        got_done_err = true;
        false
    };
    assert(got_done_err);
});
