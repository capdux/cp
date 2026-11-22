import { test, assert, assertEq } from "cap:test";
import {
    SCRIPT_RUNNER_TIMEOUT_EXIT_CODE,
    ScriptRunnerError,
    formatScriptDurationMs,
    ScriptHookStage,
    ScriptManifestEntry,
    ScriptPackageHooks,
    PlannedScriptStep,
    LifecyclePlanOptions,
    planLifecycleSequence,
    expandCustomScriptHooks,
    ScriptTrustReason,
    ScriptTrustPolicy,
    ScriptTrustDecision,
    evaluateScriptTrust,
    evaluateBatchScriptTrust,
    formatUntrustedScriptsReport,
    ScriptTaskStatus,
    ScriptFailurePolicy,
    ScriptDepEdge,
    ScriptTaskNode,
    ScriptDispatchItem,
    PkgScriptScheduler,
    ScriptEnvEntry,
    ScriptEnvOptions,
    isSensitiveEnvKey,
    buildScriptEnvVars,
    ScriptAnsiColor,
    ScriptLinePrefixConfig,
    ScriptLineBuffer,
    formatScriptRunSummary,
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

test("planLifecycleSequence orders install/prepare hooks, supports implicit node-gyp, and expandCustomScriptHooks expands pre/post", () => {
    const pkg = ScriptPackageHooks{
        package_name: "sharp",
        package_version: "0.33.5",
        package_dir: "cap_modules/sharp",
        has_binding_gyp: true,
    };
    const scripts: [6]ScriptManifestEntry = [
        ScriptManifestEntry{ name: "postprepare", command: "echo postprep" },
        ScriptManifestEntry{ name: "postinstall", command: "node install/libvips.js" },
        ScriptManifestEntry{ name: "preinstall", command: "echo pre" },
        ScriptManifestEntry{ name: "prepare", command: "cap build" },
        ScriptManifestEntry{ name: "preprepare", command: "echo preprep" },
        ScriptManifestEntry{ name: "install", command: "node-gyp rebuild" },
    ];
    let steps: [8]PlannedScriptStep = [PlannedScriptStep.empty(); 8];
    const n = planLifecycleSequence(
        pkg,
        scripts[0..],
        LifecyclePlanOptions{ include_prepare: true },
        steps[0..],
    ) catch unreachable;
    assertEq(n, @as(usize, 6));
    assert(steps[0].stage == ScriptHookStage.Preinstall);
    assert(steps[1].stage == ScriptHookStage.Install);
    assert(steps[2].stage == ScriptHookStage.Postinstall);
    assert(steps[3].stage == ScriptHookStage.Preprepare);
    assert(steps[4].stage == ScriptHookStage.Prepare);
    assert(steps[5].stage == ScriptHookStage.Postprepare);
    assert(!steps[1].is_implicit_gyp);

    // Implicit node-gyp rebuild when binding.gyp exists and no preinstall/install defined
    const native_scripts: [1]ScriptManifestEntry = [
        ScriptManifestEntry{ name: "postinstall", command: "echo verify" },
    ];
    const gyp_n = planLifecycleSequence(
        pkg,
        native_scripts[0..],
        LifecyclePlanOptions.defaults(),
        steps[0..],
    ) catch unreachable;
    assertEq(gyp_n, @as(usize, 2));
    assert(steps[0].stage == ScriptHookStage.Install);
    assert(steps[0].is_implicit_gyp);
    assert(strEql(steps[0].command, "node-gyp rebuild"));
    assert(steps[1].stage == ScriptHookStage.Postinstall);

    // --ignore-scripts yields 0 steps
    const ign_n = planLifecycleSequence(
        pkg,
        scripts[0..],
        LifecyclePlanOptions{ ignore_scripts: true },
        steps[0..],
    ) catch unreachable;
    assertEq(ign_n, @as(usize, 0));

    // Custom pre<script> -> <script> -> post<script> expansion
    const custom_scripts: [4]ScriptManifestEntry = [
        ScriptManifestEntry{ name: "prebuild", command: "rm -rf dist" },
        ScriptManifestEntry{ name: "build", command: "cap build src/index.cp" },
        ScriptManifestEntry{ name: "postbuild", command: "echo done" },
        ScriptManifestEntry{ name: "premissing", command: "echo orphan" },
    ];
    const cn = expandCustomScriptHooks(pkg, "build", custom_scripts[0..], false, steps[0..]) catch unreachable;
    assertEq(cn, @as(usize, 3));
    assert(steps[0].stage == ScriptHookStage.PreCustom);
    assert(strEql(steps[0].event_name, "prebuild"));
    assert(steps[1].stage == ScriptHookStage.Custom);
    assert(strEql(steps[1].event_name, "build"));
    assert(steps[2].stage == ScriptHookStage.PostCustom);
    assert(strEql(steps[2].event_name, "postbuild"));

    // Orphan premissing without missing script returns 0
    const missing_n = expandCustomScriptHooks(pkg, "missing", custom_scripts[0..], false, steps[0..]) catch unreachable;
    assertEq(missing_n, @as(usize, 0));

    // Empty script name returns EmptyScriptName
    let empty_err = false;
    * = expandCustomScriptHooks(pkg, "", custom_scripts[0..], false, steps[0..]) catch |e| {
        empty_err = (e == ScriptRunnerError.EmptyScriptName);
    };
    assert(empty_err);
});

test("evaluateScriptTrust enforces zero-trust allowlist, wildcards, deny-list, workspace auto-trust, and formats untrusted report", () => {
    const trusted: [2][]const u8 = ["esbuild", "@prisma/*"];
    const denied: [1][]const u8 = ["@prisma/banned"];
    const policy = ScriptTrustPolicy{
        trusted_dependencies: trusted[0..],
        blocked_packages: denied[0..],
    };

    const pkgs: [6]ScriptPackageHooks = [
        ScriptPackageHooks{ package_name: "my-monorepo", package_version: "1.0.0", is_root: true },
        ScriptPackageHooks{ package_name: "@acme/ui", package_version: "0.1.0", is_workspace: true },
        ScriptPackageHooks{ package_name: "esbuild", package_version: "0.24.0" },
        ScriptPackageHooks{ package_name: "@prisma/engines", package_version: "5.20.0" },
        ScriptPackageHooks{ package_name: "@prisma/banned", package_version: "1.0.0" },
        ScriptPackageHooks{ package_name: "untrusted-native", package_version: "2.3.4" },
    ];
    const counts: [6]u32 = [1, 2, 1, 1, 1, 2];
    let decisions: [6]ScriptTrustDecision = [ScriptTrustDecision.empty(); 6];

    const sum = evaluateBatchScriptTrust(pkgs[0..], counts[0..], policy, decisions[0..]) catch unreachable;
    assertEq(sum.total_evaluated, @as(u32, 6));
    assertEq(sum.allowed_count, @as(u32, 4));
    assertEq(sum.blocked_denied_count, @as(u32, 1));
    assertEq(sum.blocked_untrusted_count, @as(u32, 1));

    assert(decisions[0].allowed && decisions[0].reason == ScriptTrustReason.AllowedRoot);
    assert(decisions[1].allowed && decisions[1].reason == ScriptTrustReason.AllowedWorkspace);
    assert(decisions[2].allowed && decisions[2].reason == ScriptTrustReason.AllowedTrustedDependency);
    assert(decisions[3].allowed && decisions[3].reason == ScriptTrustReason.AllowedTrustedDependency);
    assert(!decisions[4].allowed && decisions[4].reason == ScriptTrustReason.BlockedDeniedPackage);
    assert(!decisions[5].allowed && decisions[5].reason == ScriptTrustReason.BlockedUntrustedThirdParty);

    // Format actionable untrusted script report
    let report: [512]u8 = [0; 512];
    const rlen = formatUntrustedScriptsReport(decisions[0..], report[0..]) catch unreachable;
    const text = report[0..rlen];
    assert(strContains(text, "Blocked 2 postinstall script(s) from 1 untrusted package(s)"));
    assert(strContains(text, "untrusted-native@2.3.4"));
    assert(strContains(text, "cap pm trust untrusted-native"));

    // --ignore-scripts blocks even root & workspace
    const ignore_dec = evaluateScriptTrust(pkgs[0], 1, ScriptTrustPolicy{ ignore_scripts: true });
    assert(!ignore_dec.allowed);
    assert(ignore_dec.reason == ScriptTrustReason.BlockedIgnoreScripts);
});

test("PkgScriptScheduler executes diamond Monorepo DAG in topological order with concurrency limit and priority", () => {
    let nodes: [4]ScriptTaskNode = [ScriptTaskNode.empty(); 4];
    let edges: [4]ScriptDepEdge = [ScriptDepEdge.empty(); 4];
    let sched = PkgScriptScheduler.init(nodes[0..], edges[0..], 2, 30000, ScriptFailurePolicy.FailFast);

    // Diamond: core -> (utils [prio 10], ui [prio 50]) -> app
    const core = sched.addTask(ScriptTaskNode{ task_id: 1, package_name: "@acme/core", script_name: "build", command: "cap build" }) catch unreachable;
    const utils = sched.addTask(ScriptTaskNode{ task_id: 2, package_name: "@acme/utils", script_name: "build", command: "cap build", priority: 10 }) catch unreachable;
    const ui = sched.addTask(ScriptTaskNode{ task_id: 3, package_name: "@acme/ui", script_name: "build", command: "cap build", priority: 50 }) catch unreachable;
    const app = sched.addTask(ScriptTaskNode{ task_id: 4, package_name: "@acme/app", script_name: "build", command: "cap build" }) catch unreachable;

    sched.addDependency(core, utils) catch unreachable;
    sched.addDependency(core, ui) catch unreachable;
    sched.addDependency(utils, app) catch unreachable;
    sched.addDependency(ui, app) catch unreachable;

    let in_deg: [4]u32 = [0; 4];
    let queue: [4]u32 = [0; 4];
    sched.finalizeGraph(in_deg[0..], queue[0..]) catch unreachable;

    // Wave 1: only core is ready
    let batch: [4]ScriptDispatchItem = [ScriptDispatchItem.empty(); 4];
    const b1 = sched.dispatchReadyBatch(100, batch[0..]) catch unreachable;
    assertEq(b1, @as(usize, 1));
    assertEq(batch[0].task_index, core);

    sched.completeTask(core, 0, 250, 5) catch unreachable;

    // Wave 2: ui (prio 50) dispatches before utils (prio 10), both fit concurrency=2
    const b2 = sched.dispatchReadyBatch(250, batch[0..]) catch unreachable;
    assertEq(b2, @as(usize, 2));
    assertEq(batch[0].task_index, ui);
    assertEq(batch[1].task_index, utils);

    // Concurrency full -> next dispatch yields 0
    assertEq(sched.dispatchReadyBatch(260, batch[0..]) catch unreachable, @as(usize, 0));

    sched.completeTask(ui, 0, 400, 3) catch unreachable;
    // app still waiting on utils
    assertEq(sched.dispatchReadyBatch(400, batch[0..]) catch unreachable, @as(usize, 0));

    sched.completeTask(utils, 0, 500, 2) catch unreachable;

    // Wave 3: app now ready
    const b3 = sched.dispatchReadyBatch(500, batch[0..]) catch unreachable;
    assertEq(b3, @as(usize, 1));
    assertEq(batch[0].task_index, app);
    sched.completeTask(app, 0, 700, 8) catch unreachable;

    assert(sched.isComplete());
    const st = sched.stats();
    assertEq(st.succeeded, @as(u32, 4));
    assertEq(st.failed, @as(u32, 0));
    assertEq(st.aggregate_exit_code, @as(i32, 0));
    assertEq(st.total_duration_ms, @as(u64, 150 + 150 + 250 + 200));
    assertEq(st.max_task_duration_ms, @as(u64, 250));
});

test("PkgScriptScheduler detects dependency cycles, self-edges, duplicate task IDs, and unfinalized dispatch", () => {
    let nodes: [4]ScriptTaskNode = [ScriptTaskNode.empty(); 4];
    let edges: [4]ScriptDepEdge = [ScriptDepEdge.empty(); 4];
    let sched = PkgScriptScheduler.init(nodes[0..], edges[0..], 2, 10000, ScriptFailurePolicy.FailFast);

    const a = sched.addTask(ScriptTaskNode{ task_id: 10, package_name: "a", script_name: "build" }) catch unreachable;
    const b = sched.addTask(ScriptTaskNode{ task_id: 20, package_name: "b", script_name: "build" }) catch unreachable;
    const c = sched.addTask(ScriptTaskNode{ task_id: 30, package_name: "c", script_name: "build" }) catch unreachable;

    // Duplicate task_id rejected
    let dup_err = false;
    * = sched.addTask(ScriptTaskNode{ task_id: 10, package_name: "dup", script_name: "build" }) catch |e| {
        dup_err = (e == ScriptRunnerError.DuplicateTaskId);
    };
    assert(dup_err);

    // Self-edge immediately rejected
    let self_err = false;
    sched.addDependency(a, a) catch |e| {
        self_err = (e == ScriptRunnerError.DependencyCycleDetected);
    };
    assert(self_err);

    // Unfinalized dispatch rejected
    let batch: [2]ScriptDispatchItem = [ScriptDispatchItem.empty(); 2];
    let unfin_err = false;
    * = sched.dispatchReadyBatch(0, batch[0..]) catch |e| {
        unfin_err = (e == ScriptRunnerError.GraphNotFinalized);
    };
    assert(unfin_err);

    // 3-node cycle a -> b -> c -> a caught during finalizeGraph
    sched.addDependency(a, b) catch unreachable;
    sched.addDependency(b, c) catch unreachable;
    sched.addDependency(c, a) catch unreachable;

    let in_deg: [4]u32 = [0; 4];
    let queue: [4]u32 = [0; 4];
    let cycle_err = false;
    sched.finalizeGraph(in_deg[0..], queue[0..]) catch |e| {
        cycle_err = (e == ScriptRunnerError.DependencyCycleDetected);
    };
    assert(cycle_err);
});

test("PkgScriptScheduler handles FailFast vs ContinueOnError and optional package degradation", () => {
    // 1. FailFast: branch A fails -> A_child skipped, unrelated B cancelled
    let nodes1: [3]ScriptTaskNode = [ScriptTaskNode.empty(); 3];
    let edges1: [2]ScriptDepEdge = [ScriptDepEdge.empty(); 2];
    let s1 = PkgScriptScheduler.init(nodes1[0..], edges1[0..], 1, 10000, ScriptFailurePolicy.FailFast);

    const a1 = s1.addTask(ScriptTaskNode{ task_id: 1, package_name: "pkg-a", script_name: "build", priority: 10 }) catch unreachable;
    const a1_child = s1.addTask(ScriptTaskNode{ task_id: 2, package_name: "pkg-a-child", script_name: "build" }) catch unreachable;
    const b1 = s1.addTask(ScriptTaskNode{ task_id: 3, package_name: "pkg-b", script_name: "build", priority: 1 }) catch unreachable;
    s1.addDependency(a1, a1_child) catch unreachable;

    let d1: [3]u32 = [0; 3];
    let q1: [3]u32 = [0; 3];
    s1.finalizeGraph(d1[0..], q1[0..]) catch unreachable;

    let batch: [2]ScriptDispatchItem = [ScriptDispatchItem.empty(); 2];
    assertEq(s1.dispatchReadyBatch(0, batch[0..]) catch unreachable, @as(usize, 1));
    assertEq(batch[0].task_index, a1);
    s1.completeTask(a1, 2, 50, 1) catch unreachable;

    assert(s1.isComplete());
    assert(nodes1[a1_child].status == ScriptTaskStatus.SkippedDependencyFailed);
    assert(nodes1[b1].status == ScriptTaskStatus.Cancelled);
    const st1 = s1.stats();
    assertEq(st1.aggregate_exit_code, @as(i32, 2));
    assertEq(st1.skipped_dep_failed, @as(u32, 1));
    assertEq(st1.cancelled, @as(u32, 1));

    // 2. ContinueOnError + optional failure degradation
    let nodes2: [4]ScriptTaskNode = [ScriptTaskNode.empty(); 4];
    let edges2: [3]ScriptDepEdge = [ScriptDepEdge.empty(); 3];
    let s2 = PkgScriptScheduler.init(nodes2[0..], edges2[0..], 4, 10000, ScriptFailurePolicy.ContinueOnError);

    const req_fail = s2.addTask(ScriptTaskNode{ task_id: 1, package_name: "req-fail", script_name: "build" }) catch unreachable;
    const req_child = s2.addTask(ScriptTaskNode{ task_id: 2, package_name: "req-child", script_name: "build" }) catch unreachable;
    const opt_fail = s2.addTask(ScriptTaskNode{ task_id: 3, package_name: "opt-native", script_name: "postinstall", is_optional: true }) catch unreachable;
    const opt_child = s2.addTask(ScriptTaskNode{ task_id: 4, package_name: "indep-app", script_name: "build" }) catch unreachable;
    s2.addDependency(req_fail, req_child) catch unreachable;
    s2.addDependency(opt_fail, opt_child) catch unreachable;

    let d2: [4]u32 = [0; 4];
    let q2: [4]u32 = [0; 4];
    s2.finalizeGraph(d2[0..], q2[0..]) catch unreachable;

    let batch2: [4]ScriptDispatchItem = [ScriptDispatchItem.empty(); 4];
    assertEq(s2.dispatchReadyBatch(0, batch2[0..]) catch unreachable, @as(usize, 2));
    s2.completeTask(req_fail, 1, 30, 1) catch unreachable;
    s2.completeTask(opt_fail, 1, 40, 1) catch unreachable;

    // req_child skipped, but opt_child became Ready despite optional parent failure!
    assert(nodes2[req_child].status == ScriptTaskStatus.SkippedDependencyFailed);
    assert(nodes2[opt_child].status == ScriptTaskStatus.Ready);

    assertEq(s2.dispatchReadyBatch(40, batch2[0..]) catch unreachable, @as(usize, 1));
    assertEq(batch2[0].task_index, opt_child);
    s2.completeTask(opt_child, 0, 90, 2) catch unreachable;

    assert(s2.isComplete());
    const st2 = s2.stats();
    assertEq(st2.succeeded, @as(u32, 1));
    assertEq(st2.failed, @as(u32, 1));
    assertEq(st2.optional_failed, @as(u32, 1));
    assertEq(st2.skipped_dep_failed, @as(u32, 1));
    assertEq(st2.aggregate_exit_code, @as(i32, 1));
});

test("PkgScriptScheduler enforces per-task timeouts with exit code 124 and handles BlockedUntrusted tasks", () => {
    let nodes: [3]ScriptTaskNode = [ScriptTaskNode.empty(); 3];
    let edges: [2]ScriptDepEdge = [ScriptDepEdge.empty(); 2];
    let sched = PkgScriptScheduler.init(nodes[0..], edges[0..], 2, 5000, ScriptFailurePolicy.ContinueOnError);

    const untrusted = sched.addTask(ScriptTaskNode{ task_id: 1, package_name: "untrusted-dep", script_name: "postinstall" }) catch unreachable;
    const slow = sched.addTask(ScriptTaskNode{ task_id: 2, package_name: "slow-pkg", script_name: "build", timeout_ms: 500 }) catch unreachable;
    const downstream = sched.addTask(ScriptTaskNode{ task_id: 3, package_name: "web", script_name: "build" }) catch unreachable;

    sched.markTaskBlockedUntrusted(untrusted) catch unreachable;
    sched.addDependency(untrusted, slow) catch unreachable;
    sched.addDependency(slow, downstream) catch unreachable;

    let d: [3]u32 = [0; 3];
    let q: [3]u32 = [0; 3];
    sched.finalizeGraph(d[0..], q[0..]) catch unreachable;

    // slow is Ready because untrusted dep is non-blocking
    let batch: [2]ScriptDispatchItem = [ScriptDispatchItem.empty(); 2];
    const n = sched.dispatchReadyBatch(1000, batch[0..]) catch unreachable;
    assertEq(n, @as(usize, 1));
    assertEq(batch[0].task_index, slow);
    assertEq(batch[0].effective_timeout_ms, @as(u64, 500));

    // At 1499ms -> not timed out yet
    let timed: [2]u32 = [0; 2];
    assertEq(sched.checkTimeouts(1499, timed[0..]) catch unreachable, @as(usize, 0));

    // At 1500ms -> timed out!
    assertEq(sched.checkTimeouts(1500, timed[0..]) catch unreachable, @as(usize, 1));
    assertEq(timed[0], slow);
    assert(nodes[slow].status == ScriptTaskStatus.TimedOut);
    assertEq(nodes[slow].exit_code, SCRIPT_RUNNER_TIMEOUT_EXIT_CODE);
    assert(nodes[downstream].status == ScriptTaskStatus.SkippedDependencyFailed);

    assert(sched.isComplete());
    const st = sched.stats();
    assertEq(st.blocked_untrusted, @as(u32, 1));
    assertEq(st.timed_out, @as(u32, 1));
    assertEq(st.skipped_dep_failed, @as(u32, 1));
    assertEq(st.aggregate_exit_code, SCRIPT_RUNNER_TIMEOUT_EXIT_CODE);
});

test("buildScriptEnvVars prepends .bin PATH, injects npm/cap lifecycle vars, and filters sensitive credential keys", () => {
    assert(isSensitiveEnvKey("NPM_TOKEN"));
    assert(isSensitiveEnvKey("GITHUB_TOKEN"));
    assert(isSensitiveEnvKey("AWS_SECRET_ACCESS_KEY"));
    assert(isSensitiveEnvKey("STRIPE_API_KEY"));
    assert(isSensitiveEnvKey("DB_PASSWORD"));
    assert(!isSensitiveEnvKey("HOME"));
    assert(!isSensitiveEnvKey("NODE_ENV"));
    assert(!isSensitiveEnvKey("CI"));

    const inherited: [7]ScriptEnvEntry = [
        ScriptEnvEntry{ key: "HOME", value: "/root" },
        ScriptEnvEntry{ key: "PATH", value: "/usr/local/bin:/usr/bin" },
        ScriptEnvEntry{ key: "NPM_TOKEN", value: "npm_secret_123" },
        ScriptEnvEntry{ key: "GITHUB_TOKEN", value: "ghp_secret_456" },
        ScriptEnvEntry{ key: "MY_APP_API_KEY", value: "sk_live_789" },
        ScriptEnvEntry{ key: "CI", value: "true" },
        ScriptEnvEntry{ key: "npm_lifecycle_event", value: "stale_event" },
    ];

    const opts = ScriptEnvOptions{
        package_name: "@acme/core",
        package_version: "1.4.2",
        package_json_path: "/repo/packages/core/package.json",
        lifecycle_event: "postinstall",
        lifecycle_script: "cap build",
        init_cwd: "/repo",
        cap_exec_path: "/usr/local/bin/cap",
        local_bin_dir: "/repo/packages/core/cap_modules/.bin",
        workspace_root_bin_dir: "/repo/cap_modules/.bin",
    };

    let out_env: [16]ScriptEnvEntry = [ScriptEnvEntry.empty(); 16];
    let path_buf: [256]u8 = [0; 256];
    const res = buildScriptEnvVars(inherited[0..], opts, out_env[0..], path_buf[0..]) catch unreachable;

    assertEq(res.filtered_sensitive_count, @as(u32, 3));
    assertEq(res.injected_standard_count, @as(u32, 9));
    // 2 kept inherited (HOME, CI) + 9 injected = 11
    assertEq(res.entry_count, @as(usize, 11));

    let found_path = false;
    let found_event = false;
    let found_cap = false;
    let found_secret = false;
    let i: usize = 0;
    while (i < res.entry_count) {
        const e = out_env[i];
        if (strEql(e.key, "PATH")) {
            found_path = strEql(e.value, "/repo/packages/core/cap_modules/.bin:/repo/cap_modules/.bin:/usr/local/bin:/usr/bin");
        }
        if (strEql(e.key, "npm_lifecycle_event")) {
            found_event = strEql(e.value, "postinstall");
        }
        if (strEql(e.key, "CAP_EXEC_PATH")) {
            found_cap = strEql(e.value, "/usr/local/bin/cap");
        }
        if (strEql(e.key, "NPM_TOKEN") || strEql(e.key, "GITHUB_TOKEN") || strEql(e.key, "MY_APP_API_KEY")) {
            found_secret = true;
        }
        i += 1;
    }
    assert(found_path);
    assert(found_event);
    assert(found_cap);
    assert(!found_secret);
});

test("ScriptLineBuffer prefixes chunked/CRLF lines with color & padding, and formatScriptRunSummary renders timing breakdown", () => {
    let pending: [64]u8 = [0; 64];
    let lbuf = ScriptLineBuffer.init(
        pending[0..],
        "@acme/ui",
        "build",
        ScriptAnsiColor.pickForIndex(0),
        ScriptLinePrefixConfig{ use_color: true, include_script_name: true, pad_width: 16 },
    );

    let out: [512]u8 = [0; 512];
    let pos: usize = 0;
    const c1 = lbuf.pushChunk("Compiling src/", out[0..], &pos) catch unreachable;
    assertEq(c1, @as(u32, 0));
    const c2 = lbuf.pushChunk("button.cp\r\nDone!\npartial tail", out[0..], &pos) catch unreachable;
    assertEq(c2, @as(u32, 2));
    const flushed = lbuf.flush(out[0..], &pos) catch unreachable;
    assert(flushed);
    assertEq(lbuf.lines_flushed, @as(u32, 3));

    const rendered = out[0..pos];
    assert(strContains(rendered, "\x1b[36m[@acme/ui:build]\x1b[0m   Compiling src/button.cp\n"));
    assert(strContains(rendered, "Done!\n"));
    assert(strContains(rendered, "partial tail\n"));

    // Duration formatting across ms, s, and m s
    let dbuf: [64]u8 = [0; 64];
    let dpos: usize = 0;
    formatScriptDurationMs(42, dbuf[0..], &dpos) catch unreachable;
    assert(strEql(dbuf[0..dpos], "42ms"));
    dpos = 0;
    formatScriptDurationMs(1250, dbuf[0..], &dpos) catch unreachable;
    assert(strEql(dbuf[0..dpos], "1.25s"));
    dpos = 0;
    formatScriptDurationMs(125000, dbuf[0..], &dpos) catch unreachable;
    assert(strEql(dbuf[0..dpos], "2m 05s"));

    // Summary report formatting
    let nodes: [2]ScriptTaskNode = [
        ScriptTaskNode{
            task_id: 1,
            package_name: "@acme/core",
            script_name: "build",
            status: ScriptTaskStatus.Succeeded,
            started_at_ms: 0,
            finished_at_ms: 420,
            exit_code: 0,
        },
        ScriptTaskNode{
            task_id: 2,
            package_name: "@acme/web",
            script_name: "test",
            status: ScriptTaskStatus.Failed,
            started_at_ms: 420,
            finished_at_ms: 1920,
            exit_code: 1,
        },
    ];
    let edges: [1]ScriptDepEdge = [ScriptDepEdge.empty(); 1];
    let sched = PkgScriptScheduler.init(nodes[0..], edges[0..], 2, 10000, ScriptFailurePolicy.FailFast);
    sched.node_count = 2;
    sched.first_failure_exit_code = 1;
    const st = sched.stats();

    let sbuf: [1024]u8 = [0; 1024];
    const slen = formatScriptRunSummary(nodes[0..], st, false, sbuf[0..]) catch unreachable;
    const summary = sbuf[0..slen];
    assert(strContains(summary, "=== Script Run Summary: FAIL (exit 1) ==="));
    assert(strContains(summary, "Tasks: 2 total, 1 passed, 1 failed"));
    assert(strContains(summary, "[OK] @acme/core:build (420ms)"));
    assert(strContains(summary, "[FAIL] @acme/web:test (1.50s, exit 1)"));
    assert(strContains(summary, "Cumulative time: 1.92s (slowest: 1.50s)"));
});
