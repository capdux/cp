// Production Deep Test for cap:shell (subshellState.cp)
// Covers: Idle -> ExpandingRedirect -> Exec -> Done lifecycle,
// direct Idle -> Exec without redirect, WaitWriteErr -> Done & .quiet() fast-path,
// isolated environment & CWD snapshot semantics, .throws() vs .nothrow() policy,
// and invalid transition / malformed redirect path guards.

import { test, assert, assertEq } from "cap:test";
import {
    SubshellParentKind,
    SubshellRunState,
    SubshellRedirectKind,
    SubshellExecPolicy,
    SubshellStepKind,
    SubshellStateMachine,
} from "cap:shell";

fn strEql(a: []const u8, b: []const u8): bool {
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

test("subshellState: redirect expansion -> exec -> done lifecycle", () => {
    let sub = SubshellStateMachine.init(
        SubshellParentKind.Pipeline,
        SubshellRedirectKind.WriteTrunc,
        SubshellExecPolicy{},
    );

    const s1 = sub.nextStep() catch {
        assert(false);
        return;
    };
    assert(s1.kind == SubshellStepKind.ExpandRedirect);
    assert(s1.state == SubshellRunState.ExpandingRedirect);

    // Calling nextStep before onRedirectExpanded stays in ExpandingRedirect
    const s1_wait = sub.nextStep() catch {
        assert(false);
        return;
    };
    assert(s1_wait.kind == SubshellStepKind.ExpandRedirect);

    * = sub.onRedirectExpanded("/tmp/subshell_out.log") catch {
        assert(false);
        return;
    };
    assert(strEql(sub.resolvedRedirectPath(), "/tmp/subshell_out.log"));

    const s2 = sub.nextStep() catch {
        assert(false);
        return;
    };
    assert(s2.kind == SubshellStepKind.ExecuteScript);
    assert(s2.state == SubshellRunState.Exec);

    * = sub.onScriptFinished(0) catch {
        assert(false);
        return;
    };
    assert(sub.isDone());
    assertEq(sub.exitCode(), 0);
    assert(!sub.shouldThrow());

    const s3 = sub.nextStep() catch {
        assert(false);
        return;
    };
    assert(s3.kind == SubshellStepKind.Completed);
    assert(!s3.should_throw);
});

test("subshellState: no-redirect direct exec and throws vs nothrow policy", () => {
    // Default throws=true on non-zero exit
    let sub_throw = SubshellStateMachine.init(
        SubshellParentKind.Binary,
        SubshellRedirectKind.None,
        SubshellExecPolicy{ throw_on_nonzero: true },
    );
    const step = sub_throw.nextStep() catch {
        assert(false);
        return;
    };
    assert(step.kind == SubshellStepKind.ExecuteScript);
    * = sub_throw.onScriptFinished(2) catch {
        assert(false);
        return;
    };
    assertEq(sub_throw.exitCode(), 2);
    assert(sub_throw.shouldThrow());

    // .nothrow() suppresses throw while preserving exit code
    let sub_nothrow = SubshellStateMachine.init(
        SubshellParentKind.Stmt,
        SubshellRedirectKind.None,
        SubshellExecPolicy{ throw_on_nonzero: false },
    );
    * = sub_nothrow.nextStep() catch {
        assert(false);
        return;
    };
    * = sub_nothrow.onScriptFinished(127) catch {
        assert(false);
        return;
    };
    assertEq(sub_nothrow.exitCode(), 127);
    assert(!sub_nothrow.shouldThrow());
});

test("subshellState: redirect error WaitWriteErr path vs quiet fast-path", () => {
    // Non-quiet enters WaitWriteErr before Done
    let sub = SubshellStateMachine.init(
        SubshellParentKind.Stmt,
        SubshellRedirectKind.WriteAppend,
        SubshellExecPolicy{ quiet: false, throw_on_nonzero: true },
    );
    * = sub.nextStep() catch {
        assert(false);
        return;
    };
    * = sub.onRedirectError(1) catch {
        assert(false);
        return;
    };
    const err_step = sub.nextStep() catch {
        assert(false);
        return;
    };
    assert(err_step.kind == SubshellStepKind.WriteError);
    assert(err_step.state == SubshellRunState.WaitWriteErr);
    assert(err_step.should_throw);

    * = sub.onErrorFlushed() catch {
        assert(false);
        return;
    };
    assert(sub.isDone());
    assertEq(sub.exitCode(), 1);

    // Quiet mode skips WaitWriteErr directly to Done (and normalizes 0 -> 1)
    let sub_q = SubshellStateMachine.init(
        SubshellParentKind.Stmt,
        SubshellRedirectKind.Read,
        SubshellExecPolicy{ quiet: true, throw_on_nonzero: false },
    );
    * = sub_q.nextStep() catch {
        assert(false);
        return;
    };
    * = sub_q.onRedirectError(0) catch {
        assert(false);
        return;
    };
    assert(sub_q.isDone());
    assertEq(sub_q.exitCode(), 1);
    assert(!sub_q.shouldThrow());
});

test("subshellState: isolated env and CWD snapshot semantics", () => {
    let sub = SubshellStateMachine.init(
        SubshellParentKind.Stmt,
        SubshellRedirectKind.None,
        SubshellExecPolicy{},
    );
    assert(!sub.propagatesSideEffectsToParent());

    * = sub.captureParentEnv("HOME", "/home/alice") catch {
        assert(false);
        return;
    };
    * = sub.setLocalEnv("HOME", "/tmp/sandbox") catch {
        assert(false);
        return;
    };
    * = sub.setLocalEnv("LOCAL_ONLY", "42") catch {
        assert(false);
        return;
    };
    * = sub.setLocalCwd("/tmp/sandbox") catch {
        assert(false);
        return;
    };

    assert(sub.local_env_mutated);
    assert(sub.local_cwd_mutated);
    if (h := sub.getLocalEnv("HOME")) {
        assert(strEql(h, "/tmp/sandbox"));
    } else {
        assert(false);
    }
    if (v := sub.getLocalEnv("LOCAL_ONLY")) {
        assert(strEql(v, "42"));
    } else {
        assert(false);
    }
    assert(sub.getLocalEnv("MISSING") == null);
});

test("subshellState: invalid transition and malformed path guards", () => {
    let sub = SubshellStateMachine.init(
        SubshellParentKind.Stmt,
        SubshellRedirectKind.WriteTrunc,
        SubshellExecPolicy{},
    );

    // Cannot finish script while Idle
    const bad1 = sub.onScriptFinished(0) catch false;
    assert(!bad1);

    // Cannot expand redirect while Idle
    const bad2 = sub.onRedirectExpanded("/tmp/a") catch false;
    assert(!bad2);

    * = sub.nextStep() catch {
        assert(false);
        return;
    };

    // Empty or control-char redirect path rejected
    const bad3 = sub.onRedirectExpanded("") catch false;
    assert(!bad3);
    const bad4 = sub.onRedirectExpanded("/tmp/bad\npath") catch false;
    assert(!bad4);

    // Empty env key rejected
    const bad5 = sub.setLocalEnv("", "val") catch false;
    assert(!bad5);
});
