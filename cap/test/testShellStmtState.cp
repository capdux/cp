// cap/test/testShellStmtState.cp
// Production-grade deep tests for cap:shell stmtState.cp
// (Bun Shell Stmt sequential child-expression dispatch state machine)

import { test, assert, assertEq } from "cap:test";
import {
    SHELL_STMT_MAX_CHILDREN,
    StmtParentKind,
    StmtChildKind,
    StmtError,
    StmtAction,
    StmtState,
} from "cap:shell";

test("StmtState: empty statement finishes immediately with exit code 0", () => {
    let st = StmtState.init(StmtParentKind.Script);
    assertEq(st.parent, StmtParentKind.Script);
    assertEq(st.remaining(), 0);
    assertEq(st.finalExitCode(), 0);

    const step = st.next() catch {
        assert(false);
        return;
    };
    assertEq(step.action, StmtAction.Finish);
    assertEq(step.exit_code, 0);
    assert(st.finished);

    let saw_finished = false;
    * = st.next() catch e => {
        if (e == StmtError.AlreadyFinished) saw_finished = true;
    };
    assert(saw_finished);
});

test("StmtState: sequential dispatch of all 8 StmtChildKind variants & last exit code propagation", () => {
    let st = StmtState.init(StmtParentKind.Script);
    const all_kinds: [8]StmtChildKind = [
        StmtChildKind.Async,
        StmtChildKind.Binary,
        StmtChildKind.Pipeline,
        StmtChildKind.Cmd,
        StmtChildKind.Assign,
        StmtChildKind.If,
        StmtChildKind.CondExpr,
        StmtChildKind.Subshell,
    ];

    let i: usize = 0;
    while (i < all_kinds.len) {
        const pos = st.pushChild(all_kinds[i]) catch {
            assert(false);
            return;
        };
        assertEq(pos, i);
        i += 1;
    }
    assertEq(st.remaining(), 8);

    let idx: usize = 0;
    while (idx < all_kinds.len) {
        const s = st.next() catch {
            assert(false);
            return;
        };
        assertEq(s.action, StmtAction.DispatchChild);
        assertEq(s.child_index, idx);
        assertEq(s.child_kind, all_kinds[idx]);
        assert(st.in_flight);

        const next_idx = st.childDone(@as(u8, @intCast(idx + 10))) catch {
            assert(false);
            return;
        };
        assertEq(next_idx, idx + 1);
        assert(!st.in_flight);
        idx += 1;
    }

    assertEq(st.completed_count, 8);
    assertEq(st.remaining(), 0);

    const done = st.next() catch {
        assert(false);
        return;
    };
    assertEq(done.action, StmtAction.Finish);
    assertEq(done.exit_code, 17); // 7 + 10
    assertEq(st.finalExitCode(), 17);
});

test("StmtState: If parent context & POSIX last-child exit code override", () => {
    let st = StmtState.init(StmtParentKind.If);
    assertEq(st.parent, StmtParentKind.If);
    * = st.pushChild(StmtChildKind.Cmd) catch {
        assert(false);
        return;
    };
    * = st.pushChild(StmtChildKind.CondExpr) catch {
        assert(false);
        return;
    };

    * = st.next() catch {
        assert(false);
        return;
    };
    * = st.childDone(1) catch {
        assert(false);
        return;
    };
    assertEq(st.finalExitCode(), 1);

    * = st.next() catch {
        assert(false);
        return;
    };
    * = st.childDone(0) catch {
        assert(false);
        return;
    };

    const fin = st.next() catch {
        assert(false);
        return;
    };
    assertEq(fin.action, StmtAction.Finish);
    assertEq(fin.exit_code, 0);
});

test("StmtState: state transition guards (ChildInFlight, NoChildInFlight, AlreadyFinished)", () => {
    let st = StmtState.init(StmtParentKind.Script);
    * = st.pushChild(StmtChildKind.Cmd) catch {
        assert(false);
        return;
    };

    // childDone before next -> NoChildInFlight
    let saw_no_child = false;
    * = st.childDone(0) catch e => {
        if (e == StmtError.NoChildInFlight) saw_no_child = true;
    };
    assert(saw_no_child);

    * = st.next() catch {
        assert(false);
        return;
    };

    // second next while in_flight -> ChildInFlight
    let saw_in_flight = false;
    * = st.next() catch e => {
        if (e == StmtError.ChildInFlight) saw_in_flight = true;
    };
    assert(saw_in_flight);

    * = st.childDone(42) catch {
        assert(false);
        return;
    };
    * = st.next() catch {
        assert(false);
        return;
    };

    // childDone after finished -> AlreadyFinished
    let saw_fin = false;
    * = st.childDone(0) catch e => {
        if (e == StmtError.AlreadyFinished) saw_fin = true;
    };
    assert(saw_fin);
});

test("StmtState: reset() preserves children list and clears execution state for reuse", () => {
    let st = StmtState.init(StmtParentKind.Script);
    * = st.pushChild(StmtChildKind.Assign) catch {
        assert(false);
        return;
    };
    * = st.pushChild(StmtChildKind.Pipeline) catch {
        assert(false);
        return;
    };

    * = st.next() catch {
        assert(false);
        return;
    };
    * = st.childDone(0) catch {
        assert(false);
        return;
    };
    * = st.next() catch {
        assert(false);
        return;
    };
    * = st.childDone(7) catch {
        assert(false);
        return;
    };
    const fin1 = st.next() catch {
        assert(false);
        return;
    };
    assertEq(fin1.exit_code, 7);

    // Reset and execute a second time with different exit codes
    st.reset();
    assertEq(st.remaining(), 2);
    assert(!st.finished);
    assert(!st.in_flight);
    assertEq(st.completed_count, 0);
    assertEq(st.finalExitCode(), 0);

    const s0 = st.next() catch {
        assert(false);
        return;
    };
    assertEq(s0.child_kind, StmtChildKind.Assign);
    * = st.childDone(0) catch {
        assert(false);
        return;
    };
    const s1 = st.next() catch {
        assert(false);
        return;
    };
    assertEq(s1.child_kind, StmtChildKind.Pipeline);
    * = st.childDone(0) catch {
        assert(false);
        return;
    };
    const fin2 = st.next() catch {
        assert(false);
        return;
    };
    assertEq(fin2.action, StmtAction.Finish);
    assertEq(fin2.exit_code, 0);
});

test("StmtState: capacity limit guard (SHELL_STMT_MAX_CHILDREN = 32)", () => {
    let st = StmtState.init(StmtParentKind.Script);
    let i: usize = 0;
    while (i < SHELL_STMT_MAX_CHILDREN) {
        * = st.pushChild(StmtChildKind.Cmd) catch {
            assert(false);
            return;
        };
        i += 1;
    }
    assertEq(st.remaining(), SHELL_STMT_MAX_CHILDREN);

    let saw_full = false;
    * = st.pushChild(StmtChildKind.Cmd) catch e => {
        if (e == StmtError.TooManyChildren) saw_full = true;
    };
    assert(saw_full);
});
