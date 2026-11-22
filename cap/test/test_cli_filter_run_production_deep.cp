// cap/test/test_cli_filter_run_production_deep.cp
// Production-deep regression tests for cap:cli filter_run module
// Source: /data/dev/bun/src/cli/filter_run.cp & /data/dev/bun/docs/pm/filter.mdx

import { test, expect } from "cap:test";
import {
    FILTER_RUN_MAX_SCRIPTS,
    FILTER_RUN_MAX_EDGES,
    FILTER_RUN_DEFAULT_ELIDE_LINES,
    FILTER_RUN_SIGINT,
    FilterRunError,
    FilterScriptPhase,
    FilterRunMode,
    FilterScriptStatus,
    FilterElideResult,
    FilterScriptNode,
    elideScriptOutput,
    formatLifecycleScriptName,
    formatFilterDuration,
    formatFilterStatusLine,
    formatFilterOutputBlock,
    FilterRunGraph,
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

test("cli filter_run: topological dependency ordering & pre/main/post lifecycle chaining") {
    let g = FilterRunGraph.init(FilterRunMode.Topological, true);
    const core_pre = g.addScript("@ws/core", "prebuild", "rm -rf dist", FilterScriptPhase.Pre) catch {
        expect(false);
        return;
    };
    const core_main = g.addScript("@ws/core", "build", "tsc -b", FilterScriptPhase.Main) catch {
        expect(false);
        return;
    };
    const core_post = g.addScript("@ws/core", "postbuild", "cp README.md dist/", FilterScriptPhase.Post) catch {
        expect(false);
        return;
    };
    const app_main = g.addScript("@ws/app", "build", "vite build", FilterScriptPhase.Main) catch {
        expect(false);
        return;
    };

    const added = g.addPackageDependency("@ws/core", "@ws/app") catch {
        expect(false);
        return;
    };
    expect(added == 3);

    const has_cycle = g.resolveDependencies() catch {
        expect(false);
        return;
    };
    expect(!has_cycle);
    expect(!g.cycle_detected);
    expect(!g.isDone());

    let ready: [8]usize = [0; 8];
    // Wave 1: only core_pre is ready
    expect(g.collectReady(ready[0..]) == 1);
    expect(ready[0] == core_pre);
    expect(g.nodes[app_main].remaining_deps == 3);

    g.markStarted(core_pre) catch { expect(false); return; };
    expect(g.runningCount() == 1);
    g.markExited(core_pre, 0, 50_000_000) catch { expect(false); return; };

    // Wave 2: core_main is now ready (app_main still needs core_main & core_post)
    expect(g.collectReady(ready[0..]) == 1);
    expect(ready[0] == core_main);
    expect(g.nodes[app_main].remaining_deps == 2);

    g.markStarted(core_main) catch { expect(false); return; };
    g.markExited(core_main, 0, 200_000_000) catch { expect(false); return; };

    // Wave 3: core_post is ready
    expect(g.collectReady(ready[0..]) == 1);
    expect(ready[0] == core_post);
    expect(g.nodes[app_main].remaining_deps == 1);

    g.markStarted(core_post) catch { expect(false); return; };
    g.markExited(core_post, 0, 30_000_000) catch { expect(false); return; };

    // Wave 4: app_main is finally ready
    expect(g.collectReady(ready[0..]) == 1);
    expect(ready[0] == app_main);
    expect(g.nodes[app_main].remaining_deps == 0);

    g.markStarted(app_main) catch { expect(false); return; };
    g.markExited(app_main, 0, 1_250_000_000) catch { expect(false); return; };

    expect(g.isDone());
    expect(g.completedCount() == 4);
    expect(g.finalizeExitCode() == 0);
}

test("cli filter_run: cycle detection & cycle-break fallback preserving lifecycle order") {
    // 3-node cross-package cycle: pkg-a -> pkg-b -> pkg-c -> pkg-a
    let g = FilterRunGraph.init(FilterRunMode.Topological, true);
    const a_pre = g.addScript("pkg-a", "pretest", "echo pre", FilterScriptPhase.Pre) catch { expect(false); return; };
    const a_main = g.addScript("pkg-a", "test", "echo a", FilterScriptPhase.Main) catch { expect(false); return; };
    const b_main = g.addScript("pkg-b", "test", "echo b", FilterScriptPhase.Main) catch { expect(false); return; };
    const c_main = g.addScript("pkg-c", "test", "echo c", FilterScriptPhase.Main) catch { expect(false); return; };

    * = g.addPackageDependency("pkg-a", "pkg-b") catch { expect(false); return; };
    * = g.addPackageDependency("pkg-b", "pkg-c") catch { expect(false); return; };
    * = g.addPackageDependency("pkg-c", "pkg-a") catch { expect(false); return; };

    expect(g.hasCycle());
    const had_cycle = g.resolveDependencies() catch { expect(false); return; };
    expect(had_cycle);
    expect(g.cycle_detected);

    // Cross-package edges cleared, but pkg-a's pre -> main is still chained!
    let ready: [8]usize = [0; 8];
    const n = g.collectReady(ready[0..]);
    expect(n == 3);
    expect(ready[0] == a_pre);
    expect(ready[1] == b_main);
    expect(ready[2] == c_main);
    expect(g.nodes[a_main].remaining_deps == 1);

    // Self-loop detection
    let g_self = FilterRunGraph.init(FilterRunMode.Topological, true);
    const s0 = g_self.addScript("self-pkg", "build", "echo self", FilterScriptPhase.Main) catch { expect(false); return; };
    g_self.addDependency(s0, s0) catch { expect(false); return; };
    expect(g_self.hasCycle());
    const self_cycle = g_self.resolveDependencies() catch { expect(false); return; };
    expect(self_cycle);
    expect(g_self.nodes[s0].remaining_deps == 0);
}

test("cli filter_run: parallel vs sequential execution mode gating") {
    // Parallel mode ignores cross-package dependencies
    let gp = FilterRunGraph.init(FilterRunMode.Parallel, true);
    const p0 = gp.addScript("pkg-a", "build", "echo a", FilterScriptPhase.Main) catch { expect(false); return; };
    const p1 = gp.addScript("pkg-b", "build", "echo b", FilterScriptPhase.Main) catch { expect(false); return; };
    * = gp.addPackageDependency("pkg-a", "pkg-b") catch { expect(false); return; };
    const p_cycle = gp.resolveDependencies() catch { expect(false); return; };
    expect(!p_cycle);

    let ready: [8]usize = [0; 8];
    expect(gp.collectReady(ready[0..]) == 2);
    expect(ready[0] == p0);
    expect(ready[1] == p1);

    // Sequential mode enforces strict 1-at-a-time execution
    let gs = FilterRunGraph.init(FilterRunMode.Sequential, true);
    const s0 = gs.addScript("pkg-a", "lint", "eslint", FilterScriptPhase.Main) catch { expect(false); return; };
    const s1 = gs.addScript("pkg-b", "lint", "eslint", FilterScriptPhase.Main) catch { expect(false); return; };
    const s2 = gs.addScript("pkg-c", "lint", "eslint", FilterScriptPhase.Main) catch { expect(false); return; };
    * = gs.resolveDependencies() catch { expect(false); return; };

    expect(gs.collectReady(ready[0..]) == 1);
    expect(ready[0] == s0);
    gs.markStarted(s0) catch { expect(false); return; };
    // While s0 is running, nothing else may start
    expect(gs.collectReady(ready[0..]) == 0);
    gs.markExited(s0, 0, 10_000_000) catch { expect(false); return; };

    expect(gs.collectReady(ready[0..]) == 1);
    expect(ready[0] == s1);
    gs.markStarted(s1) catch { expect(false); return; };
    gs.markExited(s1, 0, 10_000_000) catch { expect(false); return; };

    expect(gs.collectReady(ready[0..]) == 1);
    expect(ready[0] == s2);
    gs.markStarted(s2) catch { expect(false); return; };
    gs.markExited(s2, 0, 10_000_000) catch { expect(false); return; };
    expect(gs.isDone());
}

test("cli filter_run: exit_on_error abort vs no-exit-on-error & signal exit codes") {
    // 1. exit_on_error = true: non-zero exit aborts all pending scripts
    let g1 = FilterRunGraph.init(FilterRunMode.Topological, true);
    const a = g1.addScript("a", "test", "fail", FilterScriptPhase.Main) catch { expect(false); return; };
    const b = g1.addScript("b", "test", "ok", FilterScriptPhase.Main) catch { expect(false); return; };
    * = g1.addPackageDependency("a", "b") catch { expect(false); return; };
    * = g1.resolveDependencies() catch { expect(false); return; };

    g1.markStarted(a) catch { expect(false); return; };
    g1.markExited(a, 42, 5_000_000) catch { expect(false); return; };
    expect(g1.aborted);
    expect(g1.nodes[b].status == FilterScriptStatus.Aborted);
    expect(g1.isDone());
    expect(g1.finalizeExitCode() == 42);

    // 2. exit_on_error = false (--no-exit-on-error): continues remaining scripts
    let g2 = FilterRunGraph.init(FilterRunMode.Parallel, false);
    const c = g2.addScript("c", "test", "fail", FilterScriptPhase.Main) catch { expect(false); return; };
    const d = g2.addScript("d", "test", "ok", FilterScriptPhase.Main) catch { expect(false); return; };
    * = g2.resolveDependencies() catch { expect(false); return; };

    g2.markStarted(c) catch { expect(false); return; };
    g2.markExited(c, 7, 1_000_000) catch { expect(false); return; };
    expect(!g2.aborted);
    expect(g2.nodes[d].status == FilterScriptStatus.Pending);

    g2.markStarted(d) catch { expect(false); return; };
    g2.markExited(d, 0, 2_000_000) catch { expect(false); return; };
    expect(g2.isDone());
    expect(g2.finalizeExitCode() == 7);

    // 3. Signaled (SIGINT -> 130, SIGTERM=15 -> 143) & Errored (-> 1)
    let g3 = FilterRunGraph.init(FilterRunMode.Parallel, true);
    const s = g3.addScript("s", "dev", "sleep 99", FilterScriptPhase.Main) catch { expect(false); return; };
    g3.markStarted(s) catch { expect(false); return; };
    g3.markSignaled(s, FILTER_RUN_SIGINT) catch { expect(false); return; };
    expect(g3.finalizeExitCode() == 130);

    let g4 = FilterRunGraph.init(FilterRunMode.Parallel, true);
    const e = g4.addScript("e", "dev", "bad-bin", FilterScriptPhase.Main) catch { expect(false); return; };
    g4.markStarted(e) catch { expect(false); return; };
    g4.markErrored(e) catch { expect(false); return; };
    expect(g4.finalizeExitCode() == 1);
}

test("cli filter_run: elideScriptOutput exact boundary & trailing newline semantics") {
    // Empty input
    const r0 = elideScriptOutput("", 10);
    expect(strEq(r0.content, ""));
    expect(r0.elided_count == 0);

    // Trailing newline stripped, no elision when max_lines == null or 0
    const r1 = elideScriptOutput("line1\nline2\nline3\n", null);
    expect(strEq(r1.content, "line1\nline2\nline3"));
    expect(r1.elided_count == 0);

    const r2 = elideScriptOutput("line1\nline2\nline3\n", 0);
    expect(strEq(r2.content, "line1\nline2\nline3"));
    expect(r2.elided_count == 0);

    // Exact boundary: 3 lines with max_lines = 3 -> 0 elided
    const r3 = elideScriptOutput("a\nb\nc\n", 3);
    expect(strEq(r3.content, "a\nb\nc"));
    expect(r3.elided_count == 0);

    // Overflow: 5 lines with max_lines = 2 -> keep last 2 lines, 3 elided
    const r4 = elideScriptOutput("l1\nl2\nl3\nl4\nl5\n", 2);
    expect(strEq(r4.content, "l4\nl5"));
    expect(r4.elided_count == 3);

    // Without trailing newline: 4 lines with max_lines = 1 -> keep last 1 line, 3 elided
    const r5 = elideScriptOutput("a\nb\nc\nd", 1);
    expect(strEq(r5.content, "d"));
    expect(r5.elided_count == 3);

    // Single line no newline
    const r6 = elideScriptOutput("solo", 1);
    expect(strEq(r6.content, "solo"));
    expect(r6.elided_count == 0);
}

test("cli filter_run: lifecycle name, duration, status line & output block formatting") {
    let buf: [512]u8 = [0; 512];

    const n_pre = formatLifecycleScriptName("build", FilterScriptPhase.Pre, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n_pre], "prebuild"));
    const n_main = formatLifecycleScriptName("build", FilterScriptPhase.Main, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n_main], "build"));
    const n_post = formatLifecycleScriptName("build", FilterScriptPhase.Post, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n_post], "postbuild"));

    // Duration formatting
    const d0 = formatFilterDuration(0, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..d0], "Done"));
    const d1 = formatFilterDuration(42_000_000, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..d1], "Done in 42 ms"));
    const d2 = formatFilterDuration(2_345_000_000, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..d2], "Done in 2.35 s"));

    // Status lines
    let node = FilterScriptNode{
        package_name: "@ws/api",
        script_name: "build",
        script_content: "tsc",
        status: FilterScriptStatus.Pending,
        remaining_deps: 2,
        elide_lines: 2,
    };
    const s_pend = formatFilterStatusLine(node, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..s_pend], "Waiting for 2 other script(s)"));

    node.status = FilterScriptStatus.Signaled;
    node.signal_code = FILTER_RUN_SIGINT;
    const s_int = formatFilterStatusLine(node, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..s_int], "Interrupted"));

    // Output block with elision (is_abort = false) vs full expansion (is_abort = true)
    node.status = FilterScriptStatus.Exited;
    node.exit_code = 0;
    node.duration_ns = 150_000_000;
    const raw = "step 1\nstep 2\nstep 3\nstep 4\n";
    const bn1 = formatFilterOutputBlock(node, raw, false, buf[0..]) catch { expect(false); return; };
    const expected_elided =
        "@ws/api build $ tsc\n" ++
        "│ [2 lines elided]\n" ++
        "│ step 3\n" ++
        "│ step 4\n" ++
        "└─ Done in 150 ms\n";
    expect(strEq(buf[0..bn1], expected_elided));

    const bn2 = formatFilterOutputBlock(node, raw, true, buf[0..]) catch { expect(false); return; };
    const expected_full =
        "@ws/api build $ tsc\n" ++
        "│ step 1\n" ++
        "│ step 2\n" ++
        "│ step 3\n" ++
        "│ step 4\n" ++
        "└─ Done in 150 ms\n";
    expect(strEq(buf[0..bn2], expected_full));
}

test("cli filter_run: malformed input & capacity boundary guards") {
    let g = FilterRunGraph.init(FilterRunMode.Topological, true);

    // Empty script name rejected
    let empty_err = false;
    * = g.addScript("pkg", "", "echo", FilterScriptPhase.Main) catch {
        empty_err = true;
    };
    expect(empty_err);

    // Lifecycle formatter empty script name rejected
    let tiny: [4]u8 = [0; 4];
    let fmt_empty_err = false;
    * = formatLifecycleScriptName("", FilterScriptPhase.Pre, tiny[0..]) catch {
        fmt_empty_err = true;
    };
    expect(fmt_empty_err);

    // BufferTooSmall guard
    let small_err = false;
    * = formatLifecycleScriptName("build", FilterScriptPhase.Post, tiny[0..]) catch {
        small_err = true;
    };
    expect(small_err);

    // Invalid script index & invalid state transition guards
    let idx_err = false;
    g.markStarted(99) catch { idx_err = true; };
    expect(idx_err);

    const s0 = g.addScript("a", "build", "echo a", FilterScriptPhase.Main) catch { expect(false); return; };
    const s1 = g.addScript("b", "build", "echo b", FilterScriptPhase.Main) catch { expect(false); return; };
    g.addDependency(s0, s1) catch { expect(false); return; };
    // Duplicate edge is idempotent (remaining_deps stays 1)
    g.addDependency(s0, s1) catch { expect(false); return; };
    expect(g.nodes[s1].remaining_deps == 1);
    expect(g.edge_count == 1);

    // Cannot start s1 while remaining_deps > 0
    let trans_err1 = false;
    g.markStarted(s1) catch { trans_err1 = true; };
    expect(trans_err1);

    // Cannot exit s0 before starting it
    let trans_err2 = false;
    g.markExited(s0, 0, 1) catch { trans_err2 = true; };
    expect(trans_err2);

    // Capacity overflow guard (FILTER_RUN_MAX_SCRIPTS = 64)
    let g_cap = FilterRunGraph.init(FilterRunMode.Parallel, true);
    let i: usize = 0;
    while (i < FILTER_RUN_MAX_SCRIPTS) {
        * = g_cap.addScript("p", "t", "c", FilterScriptPhase.Main) catch { expect(false); return; };
        i += 1;
    }
    let cap_err = false;
    * = g_cap.addScript("p", "t", "c", FilterScriptPhase.Main) catch {
        cap_err = true;
    };
    expect(cap_err);
    expect(FILTER_RUN_MAX_EDGES == 256);
    expect(FILTER_RUN_DEFAULT_ELIDE_LINES == 10);
}

test("cli filter_run: diamond monorepo DAG round-trip simulation") {
    // shared -> {api, web} -> e2e
    let g = FilterRunGraph.init(FilterRunMode.Topological, true);
    const shared = g.addScript("@mono/shared", "build", "tsc -p shared", FilterScriptPhase.Main) catch { expect(false); return; };
    const api = g.addScript("@mono/api", "build", "tsc -p api", FilterScriptPhase.Main) catch { expect(false); return; };
    const web = g.addScript("@mono/web", "build", "vite build", FilterScriptPhase.Main) catch { expect(false); return; };
    const e2e = g.addScript("@mono/e2e", "build", "playwright test", FilterScriptPhase.Main) catch { expect(false); return; };

    * = g.addPackageDependency("@mono/shared", "@mono/api") catch { expect(false); return; };
    * = g.addPackageDependency("@mono/shared", "@mono/web") catch { expect(false); return; };
    * = g.addPackageDependency("@mono/api", "@mono/e2e") catch { expect(false); return; };
    * = g.addPackageDependency("@mono/web", "@mono/e2e") catch { expect(false); return; };

    const cyc = g.resolveDependencies() catch { expect(false); return; };
    expect(!cyc);

    let ready: [8]usize = [0; 8];
    // Wave 1: shared
    expect(g.collectReady(ready[0..]) == 1);
    expect(ready[0] == shared);
    g.markStarted(shared) catch { expect(false); return; };
    g.markExited(shared, 0, 100_000_000) catch { expect(false); return; };

    // Wave 2: api & web in parallel
    expect(g.collectReady(ready[0..]) == 2);
    expect(ready[0] == api);
    expect(ready[1] == web);
    g.markStarted(api) catch { expect(false); return; };
    g.markStarted(web) catch { expect(false); return; };
    expect(g.runningCount() == 2);
    g.markExited(api, 0, 300_000_000) catch { expect(false); return; };
    // e2e still waiting for web
    expect(g.collectReady(ready[0..]) == 0);
    expect(g.nodes[e2e].remaining_deps == 1);

    g.markExited(web, 0, 450_000_000) catch { expect(false); return; };

    // Wave 3: e2e
    expect(g.collectReady(ready[0..]) == 1);
    expect(ready[0] == e2e);
    g.markStarted(e2e) catch { expect(false); return; };
    g.markExited(e2e, 0, 1_500_000_000) catch { expect(false); return; };

    expect(g.isDone());
    expect(g.completedCount() == 4);
    expect(g.finalizeExitCode() == 0);
}
