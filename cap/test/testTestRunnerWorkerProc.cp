// Deep production regression tests for cap:test_runner workerProc.cp
// Covers: atomic start/commit/abortStart rollback (EMFILE recursion guard),
// file dispatch/completion & stdout+stderr atomic capture with truncation accounting,
// two-barrier reap gating (exit_status + ipc_done in both orders), corrupt-frame KillAndReap,
// POSIX/Windows spawn isolation policies, and concurrentTestGlob pattern matching.

import { test, expect, expectEq } from "cap:test";
import {
    WORKER_PROC_CAPTURE_CAP,
    WorkerPipeRole,
    WorkerExitStatus,
    WorkerLifecyclePhase,
    WorkerIpcAction,
    WorkerSpawnPolicy,
    WorkerProcessSlot,
    matchesConcurrentTestGlob,
} from "../../lib/test_runner/workerProc";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("workerProc: full lifecycle with two-barrier reap (exit first, then IPC EOF)", fn () {
    let w = WorkerProcessSlot.init(2);
    expectEq(w.phase, WorkerLifecyclePhase.Idle);
    expect(!w.canReap());

    * = w.beginStart() catch unreachable;
    expectEq(w.phase, WorkerLifecyclePhase.Starting);
    * = w.commitStart(true) catch unreachable;
    expectEq(w.phase, WorkerLifecyclePhase.Running);
    expect(w.alive);

    const fidx = w.dispatchFile(11, 5000) catch unreachable;
    expectEq(fidx, @as(u32, 11));
    expectEq(w.inflight_file.?, @as(u32, 11));

    * = w.onPipeChunk(WorkerPipeRole.Stdout, "pass: a\n");
    * = w.onPipeChunk(WorkerPipeRole.Stderr, "warn: b\n");
    let cap_buf: [64]u8 = [0; 64];
    const cn = w.takeCaptured(cap_buf[0..]) catch unreachable;
    expect(strEq(cap_buf[0..cn], "pass: a\nwarn: b\n"));
    expectEq(w.capturedSlice().len, @as(usize, 0));

    const done_idx = w.completeInflight();
    expectEq(done_idx.?, @as(u32, 11));
    expectEq(w.completed_files, @as(u32, 1));

    * = w.requestShutdown() catch unreachable;
    expectEq(w.phase, WorkerLifecyclePhase.ShuttingDown);

    // Barrier 1: process exits, IPC still draining -> cannot reap yet
    const can1 = w.onProcessExit(WorkerExitStatus.exited(0));
    expect(!can1);
    expect(!w.reap());

    // Barrier 2: IPC closes cleanly -> ReadyToReap
    const act = w.onChannelDone(false);
    expectEq(act, WorkerIpcAction.ReadyToReap);
    expect(w.canReap());
    expect(w.reap());
    expectEq(w.phase, WorkerLifecyclePhase.Reaped);
});

test("workerProc: corrupt IPC frame while alive requests KillAndReap", fn () {
    let w = WorkerProcessSlot.init(0);
    * = w.beginStart() catch unreachable;
    * = w.commitStart(true) catch unreachable;
    * = w.dispatchFile(4, 100) catch unreachable;

    // still_attached == true indicates corrupt frame path
    const act = w.onChannelDone(true);
    expectEq(act, WorkerIpcAction.KillAndReap);
    expect(!w.canReap()); // Still waiting for SIGKILL exit notification

    const can_now = w.onProcessExit(WorkerExitStatus.signaled(9));
    expect(can_now);
    expect(!w.exit_status.?.isCleanSuccess());
    expect(w.reap());
});

test("workerProc: abortStart rolls back cleanly without setting exit_status", fn () {
    let w = WorkerProcessSlot.init(1);
    * = w.beginStart() catch unreachable;
    w.abortStart();

    expect(!w.alive);
    expectEq(w.phase, WorkerLifecyclePhase.Idle);
    expect(w.exit_status == null);
    expect(w.ipc_done);
    expect(w.stdout_done);
    expect(w.stderr_done);

    // Can cleanly restart after rollback
    * = w.beginStart() catch unreachable;
    * = w.commitStart(false) catch unreachable;
    expect(w.alive);
    expect(w.ipc_done);
});

test("workerProc: capture buffer truncates safely beyond capacity and tracks truncated_bytes", fn () {
    let w = WorkerProcessSlot.init(0);
    let block: [512]u8 = ['x'; 512];
    let i: usize = 0;
    while (i < 5) {
        * = w.onPipeChunk(.Stdout, block[0..]);
        i += 1;
    }
    expectEq(w.captured_len, WORKER_PROC_CAPTURE_CAP);
    expectEq(w.truncated_bytes, @as(usize, 512));

    let tiny: [16]u8 = [0; 16];
    let failed_small = false;
    * = w.takeCaptured(tiny[0..]) catch {
        failed_small = true;
    };
    expect(failed_small);
});

test("workerProc: POSIX/Windows spawn policies and concurrentTestGlob matching", fn () {
    const linux_pol = WorkerSpawnPolicy.forPosix(true);
    expect(linux_pol.new_process_group);
    expect(linux_pol.use_linux_pdeathsig);
    expect(linux_pol.use_unix_socketpair);
    expect(!linux_pol.use_windows_job_object);

    const win_pol = WorkerSpawnPolicy.forWindows();
    expect(win_pol.is_windows);
    expect(win_pol.use_windows_job_object);
    expect(!win_pol.use_unix_socketpair);

    // Bun docs/guides/test/concurrent-test-glob.mdx examples
    expect(matchesConcurrentTestGlob("**/concurrent-*.test.ts", "tests/integration/concurrent-api.test.ts"));
    expect(matchesConcurrentTestGlob("**/concurrent-*.test.ts", "concurrent-db.test.ts"));
    expect(!matchesConcurrentTestGlob("**/concurrent-*.test.ts", "tests/unit/math.test.ts"));
    expect(matchesConcurrentTestGlob("tests/unit/*.test.ts", "tests/unit/utils.test.ts"));
    expect(!matchesConcurrentTestGlob("tests/unit/*.test.ts", "tests/unit/deep/utils.test.ts"));
    expect(!matchesConcurrentTestGlob("", "any.test.ts"));
});

test("workerProc: guards double dispatch, dispatch while idle, and double start", fn () {
    let w = WorkerProcessSlot.init(0);

    let err_idle_dispatch = false;
    * = w.dispatchFile(0, 10) catch {
        err_idle_dispatch = true;
    };
    expect(err_idle_dispatch);

    * = w.beginStart() catch unreachable;
    * = w.commitStart(true) catch unreachable;
    * = w.dispatchFile(0, 10) catch unreachable;

    let err_double_dispatch = false;
    * = w.dispatchFile(1, 20) catch {
        err_double_dispatch = true;
    };
    expect(err_double_dispatch);

    let err_double_start = false;
    * = w.beginStart() catch {
        err_double_start = true;
    };
    expect(err_double_start);
});
