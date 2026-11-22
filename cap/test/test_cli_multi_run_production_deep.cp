// cap/test/test_cli_multi_run_production_deep.cp
// Production-deep regression tests for cap:cli multi_run module
// Source: /data/dev/bun/src/cli/multi_run.cp & /data/dev/bun/docs/snippets/cli/run.mdx

import { test, expect } from "cap:test";
import {
    MULTI_RUN_MAX_SCRIPTS,
    MULTI_RUN_MAX_EDGES,
    MULTI_RUN_LINE_BUF_CAP,
    MULTI_RUN_PALETTE_LEN,
    MultiRunError,
    MultiRunEdgeKind,
    MultiRunStatus,
    MultiRunTask,
    multiRunPaletteColor,
    formatMultiRunPrefix,
    formatMultiRunExitStatus,
    MultiRunLineBuffer,
    MultiRunScheduler,
} from "cap:cli";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("cli multi_run: 6-color palette cycling & Foreman prefix padding") {
    expect(MULTI_RUN_PALETTE_LEN == 6);
    expect(strEq(multiRunPaletteColor(0), "\x1b[36m")); // cyan
    expect(strEq(multiRunPaletteColor(1), "\x1b[33m")); // yellow
    expect(strEq(multiRunPaletteColor(5), "\x1b[31m")); // red
    expect(strEq(multiRunPaletteColor(6), "\x1b[36m")); // wraps back to cyan

    let buf: [128]u8 = [0; 128];
    const n1 = formatMultiRunPrefix("api", 8, 0, false, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "api      | "));

    const n2 = formatMultiRunPrefix("api", 5, 1, true, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "\x1b[33mapi  \x1b[0m | "));
}

test("cli multi_run: MultiRunLineBuffer streaming partial chunks & EOF flush") {
    let lb = MultiRunLineBuffer.init();
    let out: [256]u8 = [0; 256];

    // Chunk 1: one full line + partial second line
    const n1 = lb.feedChunk("line one\npart", "web", 5, 0, false, out[0..]) catch { expect(false); return; };
    expect(strEq(out[0..n1], "web   | line one\n"));
    expect(lb.len == 4);

    // Chunk 2: completes second line + starts third line without trailing \n
    const n2 = lb.feedChunk("ial two\ntail", "web", 5, 0, false, out[0..]) catch { expect(false); return; };
    expect(strEq(out[0..n2], "web   | partial two\n"));
    expect(lb.len == 4);

    // EOF flush auto-appends \n
    const n3 = lb.flushRemaining("web", 5, 0, false, out[0..]) catch { expect(false); return; };
    expect(strEq(out[0..n3], "web   | tail\n"));
    expect(lb.len == 0);

    // Second flush is a clean no-op
    const n4 = lb.flushRemaining("web", 5, 0, false, out[0..]) catch { expect(false); return; };
    expect(n4 == 0);
}

test("cli multi_run: formatMultiRunExitStatus duration & failure formats") {
    let buf: [128]u8 = [0; 128];

    const t_done = MultiRunTask{ label: "a", status: MultiRunStatus.Exited, exit_code: 0, duration_ns: 0 };
    const n0 = formatMultiRunExitStatus(t_done, 3, false, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n0], "a   | Done\n"));

    const t_ms = MultiRunTask{ label: "a", status: MultiRunStatus.Exited, exit_code: 0, duration_ns: 42_000_000 };
    const n1 = formatMultiRunExitStatus(t_ms, 3, false, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "a   | Done in 42ms\n"));

    const t_sec = MultiRunTask{ label: "a", status: MultiRunStatus.Exited, exit_code: 0, duration_ns: 1_250_000_000 };
    const n2 = formatMultiRunExitStatus(t_sec, 3, false, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "a   | Done in 1.25s\n"));

    const t_fail = MultiRunTask{ label: "a", status: MultiRunStatus.Exited, exit_code: 2, duration_ns: 10 };
    const n3 = formatMultiRunExitStatus(t_fail, 3, false, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n3], "a   | Exited with code 2\n"));

    const t_sig = MultiRunTask{ label: "a", status: MultiRunStatus.Signaled, signal_code: 15 };
    const n4 = formatMultiRunExitStatus(t_sig, 3, false, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n4], "a   | Signaled: 15\n"));
}

test("cli multi_run: two-tier cascade skips GroupChain on failure while advancing NextGroup") {
    // Group A: pre_a -> main_a -> post_a (GroupChain)
    // Group B: main_b depends on post_a via NextGroup (--sequential --no-exit-on-error)
    let sched = MultiRunScheduler.init(true);
    const pre_a = sched.addTask("pkg-a:prebuild") catch { expect(false); return; };
    const main_a = sched.addTask("pkg-a:build") catch { expect(false); return; };
    const post_a = sched.addTask("pkg-a:postbuild") catch { expect(false); return; };
    const main_b = sched.addTask("pkg-b:build") catch { expect(false); return; };

    expect(sched.max_label_len == 15); // "pkg-a:postbuild"
    sched.addEdge(pre_a, main_a, MultiRunEdgeKind.GroupChain) catch { expect(false); return; };
    sched.addEdge(main_a, post_a, MultiRunEdgeKind.GroupChain) catch { expect(false); return; };
    sched.addEdge(post_a, main_b, MultiRunEdgeKind.NextGroup) catch { expect(false); return; };

    let ready: [8]usize = [0; 8];
    expect(sched.collectReady(ready[0..]) == 1);
    expect(ready[0] == pre_a);

    // pre_a fails -> main_a & post_a MUST be Skipped, and main_b MUST become ready!
    sched.markStarted(pre_a) catch { expect(false); return; };
    sched.markExited(pre_a, 1, 10_000_000) catch { expect(false); return; };

    expect(!sched.aborted);
    expect(sched.tasks[main_a].status == MultiRunStatus.Skipped);
    expect(sched.tasks[post_a].status == MultiRunStatus.Skipped);
    expect(sched.tasks[main_b].status == MultiRunStatus.Pending);
    expect(sched.tasks[main_b].remaining_deps == 0);

    expect(sched.collectReady(ready[0..]) == 1);
    expect(ready[0] == main_b);

    sched.markStarted(main_b) catch { expect(false); return; };
    sched.markExited(main_b, 0, 50_000_000) catch { expect(false); return; };
    expect(sched.isDone());
    expect(sched.finalizeExitCode() == 1);
}

test("cli multi_run: fail-fast default aborts pending tasks & signal exit code") {
    let sched = MultiRunScheduler.init(false);
    const t0 = sched.addTask("lint") catch { expect(false); return; };
    const t1 = sched.addTask("test") catch { expect(false); return; };
    sched.addEdge(t0, t1, MultiRunEdgeKind.NextGroup) catch { expect(false); return; };

    sched.markStarted(t0) catch { expect(false); return; };
    sched.markExited(t0, 42, 5_000_000) catch { expect(false); return; };
    expect(sched.aborted);
    expect(sched.tasks[t1].status == MultiRunStatus.Aborted);
    expect(sched.isDone());
    expect(sched.finalizeExitCode() == 42);

    let s2 = MultiRunScheduler.init(false);
    const sig_t = s2.addTask("dev") catch { expect(false); return; };
    s2.markStarted(sig_t) catch { expect(false); return; };
    s2.markSignaled(sig_t, 15) catch { expect(false); return; };
    expect(s2.finalizeExitCode() == 143);
}

test("cli multi_run: happy-path parallel & sequential completion round-trip") {
    let sched = MultiRunScheduler.init(false);
    const a = sched.addTask("build:js") catch { expect(false); return; };
    const b = sched.addTask("build:css") catch { expect(false); return; };
    const c = sched.addTask("package") catch { expect(false); return; };
    sched.addEdge(a, c, MultiRunEdgeKind.NextGroup) catch { expect(false); return; };
    sched.addEdge(b, c, MultiRunEdgeKind.NextGroup) catch { expect(false); return; };

    let ready: [8]usize = [0; 8];
    expect(sched.collectReady(ready[0..]) == 2);
    sched.markStarted(a) catch { expect(false); return; };
    sched.markStarted(b) catch { expect(false); return; };
    sched.markExited(a, 0, 100_000_000) catch { expect(false); return; };
    expect(sched.collectReady(ready[0..]) == 0);
    sched.markExited(b, 0, 150_000_000) catch { expect(false); return; };

    expect(sched.collectReady(ready[0..]) == 1);
    expect(ready[0] == c);
    sched.markStarted(c) catch { expect(false); return; };
    sched.markExited(c, 0, 200_000_000) catch { expect(false); return; };

    expect(sched.isDone());
    expect(sched.finalizeExitCode() == 0);
}

test("cli multi_run: malformed inputs & capacity boundary guards") {
    let sched = MultiRunScheduler.init(false);

    // Empty label rejected
    let e1 = false;
    * = sched.addTask("") catch { e1 = true; };
    expect(e1);

    // Invalid script index rejected
    let e2 = false;
    sched.markStarted(99) catch { e2 = true; };
    expect(e2);

    // BufferTooSmall in formatMultiRunPrefix
    let tiny: [3]u8 = [0; 3];
    let e3 = false;
    * = formatMultiRunPrefix("long-label", 10, 0, false, tiny[0..]) catch { e3 = true; };
    expect(e3);

    // TooManyScripts guard (64)
    let i: usize = 0;
    while (i < MULTI_RUN_MAX_SCRIPTS) {
        * = sched.addTask("t") catch { expect(false); return; };
        i += 1;
    }
    let e4 = false;
    * = sched.addTask("overflow") catch { e4 = true; };
    expect(e4);
    expect(MULTI_RUN_MAX_EDGES == 256);

    // LineBufferOverflow guard (1024 bytes without newline)
    let lb = MultiRunLineBuffer.init();
    let big_line: [MULTI_RUN_LINE_BUF_CAP + 1]u8 = ['a'; MULTI_RUN_LINE_BUF_CAP + 1];
    let out: [32]u8 = [0; 32];
    let e5 = false;
    * = lb.feedChunk(big_line[0..], "x", 1, 0, false, out[0..]) catch { e5 = true; };
    expect(e5);
}
