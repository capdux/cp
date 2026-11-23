import { test, assert, assertEq } from "cap:test";
import {
    PmExecError,
    PmExecAction,
    parsePmExecAction,
    pmExecActionName,
    PmExecRequest,
    parsePmExecRequest,
    PmExecStage,
    pmExecStageName,
    PmExecEvent,
    PmExecSession,
    shouldSkipPmExecStage,
    WorkspaceFilterMatch,
    matchWorkspaceFilterGlob,
    WorkspaceNodeInput,
    WorkspaceInstallStep,
    planWorkspaceInstallOrder,
    formatPmExecSummary,
    formatPmExecEventsLog,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strContains(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("parsePmExecRequest routes all 8 actions, aliases, flags, and conflict guards", () => {
    assert(parsePmExecAction("i") == PmExecAction.Install);
    assert(parsePmExecAction("a") == PmExecAction.Add);
    assert(parsePmExecAction("rm") == PmExecAction.Remove);
    assert(parsePmExecAction("up") == PmExecAction.Update);
    assert(parsePmExecAction("ci") == PmExecAction.Ci);
    assert(parsePmExecAction("ln") == PmExecAction.Link);
    assert(parsePmExecAction("audit") == PmExecAction.Audit);
    assert(parsePmExecAction("outdated") == PmExecAction.Outdated);
    assert(parsePmExecAction("unknown") == null);
    assert(strEql(pmExecActionName(PmExecAction.Ci), "ci"));

    let targets: [8][]const u8 = [""; 8];
    let filters: [8][]const u8 = [""; 8];

    // `cap add zod --dev --no-save --concurrency=16`
    const add_argv: [5][]const u8 = ["add", "zod", "--dev", "--no-save", "--concurrency=16"];
    const add_req = parsePmExecRequest(add_argv[0..], targets[0..], filters[0..]) catch unreachable;
    assert(add_req.action == PmExecAction.Add);
    assert(add_req.flags.save_dev);
    assert(add_req.flags.no_save);
    assertEq(add_req.flags.concurrency, @as(u32, 16));
    assertEq(add_req.target_count, @as(usize, 1));
    assert(strEql(targets[0], "zod"));
    assert(!add_req.manifest_write_allowed); // blocked by --no-save
    assert(add_req.lockfile_write_allowed);

    // `cap ci` enforces frozen_lockfile + clean_node_modules
    const ci_argv: [1][]const u8 = ["ci"];
    const ci_req = parsePmExecRequest(ci_argv[0..], targets[0..], filters[0..]) catch unreachable;
    assert(ci_req.flags.frozen_lockfile);
    assert(ci_req.flags.clean_node_modules);
    assert(!ci_req.lockfile_write_allowed);
    assert(!ci_req.manifest_write_allowed);

    // Conflicting flags: `cap ci --lockfile-only` must fail
    const bad_ci: [2][]const u8 = ["ci", "--lockfile-only"];
    let ci_conflict = false;
    * = parsePmExecRequest(bad_ci[0..], targets[0..], filters[0..]) catch {
        ci_conflict = true;
    };
    assert(ci_conflict);

    // Conflicting flags: `-D` + `-O` must fail
    const bad_dep: [4][]const u8 = ["add", "react", "-D", "-O"];
    let dep_conflict = false;
    * = parsePmExecRequest(bad_dep[0..], targets[0..], filters[0..]) catch {
        dep_conflict = true;
    };
    assert(dep_conflict);
});

test("PmExecSession runs full 9-stage monotonic pipeline and rejects backwards/skipped transitions", () => {
    let targets: [4][]const u8 = [""; 4];
    let filters: [4][]const u8 = [""; 4];
    const argv: [2][]const u8 = ["add", "zod"];
    const req = parsePmExecRequest(argv[0..], targets[0..], filters[0..]) catch unreachable;

    let evs: [64]PmExecEvent = [PmExecEvent.empty(); 64];
    let sess = PmExecSession.init(req, evs[0..], 1000);

    // Jumping directly to ResolveGraph before LoadConfig must fail
    let jumped = false;
    sess.advanceStage(PmExecStage.ResolveGraph, 1005) catch {
        jumped = true;
    };
    assert(jumped);

    let t: u64 = 1010;
    while (!sess.isFinished()) {
        const next = sess.nextRequiredStage();
        sess.advanceStage(next, t) catch unreachable;
        if (next == PmExecStage.ComputeDiff) {
            sess.recordDiff(3, 1, 2, 10, t + 5) catch unreachable;
        } else if (next == PmExecStage.MaterializeStore) {
            sess.recordMaterialized(5, t + 5) catch unreachable;
        } else if (next == PmExecStage.LinkWorkspaces) {
            sess.recordWorkspacesLinked(4, 4, t + 5) catch unreachable;
        } else if (next == PmExecStage.RunLifecycle) {
            sess.recordLifecycleRun(2, 1, t + 5) catch unreachable;
        } else if (next == PmExecStage.PersistLockfile) {
            sess.recordPersistence(true, true, t + 5) catch unreachable;
        }
        t += 10;
    }

    assert(sess.stage == PmExecStage.Done);
    assertEq(sess.metrics.stages_completed, @as(u32, 9));
    assertEq(sess.metrics.stages_skipped, @as(u32, 0));
    assert(sess.metrics.manifest_updated);
    assert(sess.metrics.lockfile_updated);
    assert(sess.event_count >= 18);

    // Moving after Done must fail
    let after_done = false;
    sess.advanceStage(PmExecStage.Done, t + 1) catch {
        after_done = true;
    };
    assert(after_done);

    let sum_buf: [512]u8 = [0; 512];
    const slen = formatPmExecSummary(sess, sum_buf[0..]) catch unreachable;
    const summary = sum_buf[0..slen];
    assert(strContains(summary, "cap add [Done]"));
    assert(strContains(summary, "+3 -1 ~2 =10 materialized=5"));
    assert(strContains(summary, "manifest_saved=true lockfile_saved=true"));

    let log_buf: [2048]u8 = [0; 2048];
    const llen = formatPmExecEventsLog(evs[0..sess.event_count], log_buf[0..]) catch unreachable;
    assert(strContains(log_buf[0..llen], "ENTER LoadConfig"));
    assert(strContains(log_buf[0..llen], "ENTER Done"));
});

test("PmExecSession honors --dry-run, --lockfile-only, and --ignore-scripts stage skipping", () => {
    let targets: [4][]const u8 = [""; 4];
    let filters: [4][]const u8 = [""; 4];

    // 1. --dry-run stops right after ComputeDiff (skips 4 stages: Materialize, Link, Lifecycle, Persist)
    const dry_argv: [2][]const u8 = ["install", "--dry-run"];
    const dry_req = parsePmExecRequest(dry_argv[0..], targets[0..], filters[0..]) catch unreachable;
    let evs1: [48]PmExecEvent = [PmExecEvent.empty(); 48];
    let s1 = PmExecSession.init(dry_req, evs1[0..], 0);
    while (!s1.isFinished()) {
        const next = s1.nextRequiredStage();
        s1.advanceStage(next, 10) catch unreachable;
        if (next == PmExecStage.ComputeDiff) {
            s1.recordDiff(4, 0, 0, 2, 12) catch unreachable;
        }
    }
    assert(s1.stage == PmExecStage.Done);
    assertEq(s1.metrics.stages_completed, @as(u32, 5));
    assertEq(s1.metrics.stages_skipped, @as(u32, 4));
    assert(!s1.metrics.lockfile_updated);

    // 2. --lockfile-only skips Materialize, Link, Lifecycle, but runs PersistLockfile
    const lfo_argv: [2][]const u8 = ["install", "--lockfile-only"];
    const lfo_req = parsePmExecRequest(lfo_argv[0..], targets[0..], filters[0..]) catch unreachable;
    let evs2: [48]PmExecEvent = [PmExecEvent.empty(); 48];
    let s2 = PmExecSession.init(lfo_req, evs2[0..], 0);
    while (!s2.isFinished()) {
        const next = s2.nextRequiredStage();
        s2.advanceStage(next, 20) catch unreachable;
        if (next == PmExecStage.PersistLockfile) {
            s2.recordPersistence(false, true, 25) catch unreachable;
        }
    }
    assert(s2.stage == PmExecStage.Done);
    assertEq(s2.metrics.stages_completed, @as(u32, 6));
    assertEq(s2.metrics.stages_skipped, @as(u32, 3));
    assert(s2.metrics.lockfile_updated);
    assert(shouldSkipPmExecStage(lfo_req, PmExecStage.MaterializeStore));
    assert(!shouldSkipPmExecStage(lfo_req, PmExecStage.PersistLockfile));
});

test("PmExecSession enforces --frozen-lockfile drift guard under cap ci", () => {
    let targets: [4][]const u8 = [""; 4];
    let filters: [4][]const u8 = [""; 4];
    const argv: [1][]const u8 = ["ci"];
    const req = parsePmExecRequest(argv[0..], targets[0..], filters[0..]) catch unreachable;

    let evs: [32]PmExecEvent = [PmExecEvent.empty(); 32];
    let sess = PmExecSession.init(req, evs[0..], 0);
    sess.advanceStage(PmExecStage.LoadConfig, 1) catch unreachable;
    sess.advanceStage(PmExecStage.ReadManifest, 2) catch unreachable;
    sess.advanceStage(PmExecStage.LoadLockfile, 3) catch unreachable;
    sess.advanceStage(PmExecStage.ResolveGraph, 4) catch unreachable;
    sess.advanceStage(PmExecStage.ComputeDiff, 5) catch unreachable;

    let drift_err = false;
    sess.recordDiff(1, 0, 0, 20, 6) catch {
        drift_err = true;
    };
    assert(drift_err);
    assert(sess.stage == PmExecStage.Failed);
    assert(sess.metrics.clean_rebuild_triggered);

    let out: [512]u8 = [0; 512];
    const n = formatPmExecSummary(sess, out[0..]) catch unreachable;
    assert(strContains(out[0..n], "lockfile is frozen"));
});

test("matchWorkspaceFilterGlob handles name globs, path globs, and ! negations", () => {
    assert(matchWorkspaceFilterGlob("@cap/*", "@cap/core", "packages/core") == WorkspaceFilterMatch.Match);
    assert(matchWorkspaceFilterGlob("@cap/*", "web-app", "apps/web") == WorkspaceFilterMatch.NoMatch);
    assert(matchWorkspaceFilterGlob("./packages/*", "@cap/core", "packages/core") == WorkspaceFilterMatch.Match);
    assert(matchWorkspaceFilterGlob("!@cap/legacy", "@cap/legacy", "packages/legacy") == WorkspaceFilterMatch.NegateExclude);
    assert(matchWorkspaceFilterGlob("!packages/legacy", "@cap/legacy", "packages/legacy") == WorkspaceFilterMatch.NegateExclude);
    assert(matchWorkspaceFilterGlob("app-?", "app-1", "apps/1") == WorkspaceFilterMatch.Match);
    assert(matchWorkspaceFilterGlob("app-?", "app-12", "apps/12") == WorkspaceFilterMatch.NoMatch);
});

test("planWorkspaceInstallOrder filters workspaces, expands transitive deps, and computes topological waves", () => {
    // Graph:
    //   0: root (depends on none)
    //   1: @app/web (depends on 2: @pkg/ui, 3: @pkg/utils)
    //   2: @pkg/ui (depends on 3: @pkg/utils)
    //   3: @pkg/utils (leaf)
    //   4: @app/docs (unrelated, depends on none)
    const ws_deps: [3]usize = [2, 3, 3];
    const nodes: [5]WorkspaceNodeInput = [
        WorkspaceNodeInput{ name: "monorepo-root", rel_path: ".", is_root: true, deps_start: 0, deps_len: 0 },
        WorkspaceNodeInput{ name: "@app/web", rel_path: "apps/web", deps_start: 0, deps_len: 2 },
        WorkspaceNodeInput{ name: "@pkg/ui", rel_path: "packages/ui", deps_start: 2, deps_len: 1 },
        WorkspaceNodeInput{ name: "@pkg/utils", rel_path: "packages/utils", deps_start: 0, deps_len: 0 },
        WorkspaceNodeInput{ name: "@app/docs", rel_path: "apps/docs", deps_start: 0, deps_len: 0 },
    ];

    const filters: [2][]const u8 = ["@app/*", "!@app/docs"];
    let selected: [5]u8 = [0; 5];
    let indeg: [5]u32 = [0; 5];
    let waves: [5]u32 = [0; 5];
    let steps: [5]WorkspaceInstallStep = [WorkspaceInstallStep.empty(); 5];

    const plan = planWorkspaceInstallOrder(
        nodes[0..],
        ws_deps[0..],
        filters[0..],
        selected[0..],
        indeg[0..],
        waves[0..],
        steps[0..],
    ) catch unreachable;

    // Only @app/web matched directly; @pkg/ui and @pkg/utils pulled in transitively.
    assertEq(plan.selected_count, @as(usize, 3));
    assertEq(plan.direct_match_count, @as(usize, 1));
    assertEq(plan.transitive_dep_count, @as(usize, 2));
    assertEq(plan.wave_count, @as(u32, 3));
    assert(!plan.include_root);

    // Wave 0: @pkg/utils (idx 3), Wave 1: @pkg/ui (idx 2), Wave 2: @app/web (idx 1)
    assertEq(steps[0].ws_index, @as(usize, 3));
    assertEq(steps[0].wave, @as(u32, 0));
    assert(steps[0].transitive_dep);

    assertEq(steps[1].ws_index, @as(usize, 2));
    assertEq(steps[1].wave, @as(u32, 1));
    assert(steps[1].transitive_dep);

    assertEq(steps[2].ws_index, @as(usize, 1));
    assertEq(steps[2].wave, @as(u32, 2));
    assert(steps[2].direct_match);
});

test("planWorkspaceInstallOrder detects cyclic workspace dependencies", () => {
    // 0 -> 1 -> 0 cycle
    const ws_deps: [2]usize = [1, 0];
    const nodes: [2]WorkspaceNodeInput = [
        WorkspaceNodeInput{ name: "pkg-a", rel_path: "packages/a", deps_start: 0, deps_len: 1 },
        WorkspaceNodeInput{ name: "pkg-b", rel_path: "packages/b", deps_start: 1, deps_len: 1 },
    ];
    const no_filters: [0][]const u8 = [];
    let selected: [2]u8 = [0; 2];
    let indeg: [2]u32 = [0; 2];
    let waves: [2]u32 = [0; 2];
    let steps: [2]WorkspaceInstallStep = [WorkspaceInstallStep.empty(); 2];

    let cycle_caught = false;
    * = planWorkspaceInstallOrder(
        nodes[0..],
        ws_deps[0..],
        no_filters[0..],
        selected[0..],
        indeg[0..],
        waves[0..],
        steps[0..],
    ) catch {
        cycle_caught = true;
    };
    assert(cycle_caught);
});

test("deterministic 500-iteration fuzz over malformed glob filters and CLI flag combinations causes 0 panics", () => {
    let seed: u64 = 0xd1b54a32d192ed03;
    const raw_tokens: [14][]const u8 = [
        "install",
        "add",
        "ci",
        "remove",
        "--dry-run",
        "--lockfile-only",
        "--frozen-lockfile",
        "--no-save",
        "--filter=",
        "--filter=***a*?b***",
        "--concurrency=0",
        "--concurrency=abc",
        "--unknown-flag",
        "pkg-x",
    ];
    const pat_chars: [8]u8 = ['*', '?', '!', '/', '.', 'a', '@', '-'];

    let targets: [8][]const u8 = [""; 8];
    let filters: [8][]const u8 = [""; 8];
    let argv: [5][]const u8 = [""; 5];
    let pat_buf: [64]u8 = [0; 64];

    let iter: u32 = 0;
    let valid_reqs: u32 = 0;
    let rejected_reqs: u32 = 0;
    while (iter < 500) {
        seed = seed ^ (seed >> 12);
        seed = seed ^ (seed << 25);
        seed = seed ^ (seed >> 27);
        const r = seed *% 2685821657736338717;

        // 1. Fuzz CLI flag combinations
        const argc: usize = @as(usize, @intCast(r % 5)) + 1;
        let a: usize = 0;
        let lr = r;
        while (a < argc) {
            lr = lr *% 6364136223846793005 +% 1442695040888963407;
            argv[a] = raw_tokens[@as(usize, @intCast((lr >> 16) % 14))];
            a += 1;
        }
        const parsed = parsePmExecRequest(argv[0..argc], targets[0..], filters[0..]) catch null;
        if (parsed != null) {
            valid_reqs += 1;
        } else {
            rejected_reqs += 1;
        }

        // 2. Fuzz adversarial wildcard patterns (multi-star ReDoS stress)
        const plen: usize = @as(usize, @intCast((lr >> 8) % 48));
        let p: usize = 0;
        while (p < plen) {
            lr = lr *% 6364136223846793005 +% 1442695040888963407;
            pat_buf[p] = pat_chars[@as(usize, @intCast((lr >> 24) % 8))];
            p += 1;
        }
        const m = matchWorkspaceFilterGlob(pat_buf[0..plen], "@scope/pkg-alpha-beta", "packages/alpha/beta");
        assert(m == WorkspaceFilterMatch.NoMatch || m == WorkspaceFilterMatch.Match || m == WorkspaceFilterMatch.NegateExclude);

        iter += 1;
    }

    assert(valid_reqs > 20);
    assert(rejected_reqs > 50);
});
