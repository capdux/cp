import { test, assert, assertEq } from "cap:test";
import {
    ScaffoldError,
    ScaffoldTemplateKind,
    ScaffoldConflictPolicy,
    ScaffoldFileAction,
    ScaffoldVarBinding,
    ScaffoldRenderOptions,
    ScaffoldRenderDiag,
    CapManifestDep,
    CapManifestScript,
    ScaffoldProjectOptions,
    ScaffoldPlannedFile,
    isReservedProjectName,
    normalizeProjectName,
    parseScaffoldTemplateKind,
    scaffoldTemplateName,
    scaffoldTemplateDescription,
    renderScaffoldTemplate,
    formatCapManifestJson,
    resolveScaffoldConflict,
    planProjectScaffold,
    formatScaffoldPlanReport,
} from "cap:cli";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) return true;
        i += 1;
    }
    return false;
}

test("Project name normalization, reserved name checks, and path traversal guards", () => {
    let buf: [128]u8 = [0; 128];

    const n1 = normalizeProjectName("My_Awesome App 2!", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n1], "my-awesome-app-2"));

    const n2 = normalizeProjectName("@Acme/Core_SDK", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n2], "@acme/core-sdk"));

    assert(isReservedProjectName("node_modules"));
    assert(isReservedProjectName("cap_modules"));

    let res_err = false;
    * = normalizeProjectName("node_modules", buf[0..]) catch |e| {
        res_err = (e == ScaffoldError.ReservedProjectName);
    };
    assert(res_err);

    let trav_err = false;
    * = normalizeProjectName("../escape", buf[0..]) catch |e| {
        trav_err = (e == ScaffoldError.UnsafePathTraversal);
    };
    assert(trav_err);

    let slash_err = false;
    * = normalizeProjectName("a/b", buf[0..]) catch |e| {
        slash_err = (e == ScaffoldError.UnsafePathTraversal);
    };
    assert(slash_err);
});

test("Six built-in project scaffold template kinds and metadata lookup", () => {
    assert(parseScaffoldTemplateKind("minimal").? == ScaffoldTemplateKind.MinimalApp);
    assert(parseScaffoldTemplateKind("lib").? == ScaffoldTemplateKind.StdLibrary);
    assert(parseScaffoldTemplateKind("ai-agent").? == ScaffoldTemplateKind.AiAgentService);
    assert(parseScaffoldTemplateKind("http").? == ScaffoldTemplateKind.HttpApiServer);
    assert(parseScaffoldTemplateKind("media").? == ScaffoldTemplateKind.RealtimeMediaApp);
    assert(parseScaffoldTemplateKind("monorepo").? == ScaffoldTemplateKind.WorkspaceMonorepo);
    assert(parseScaffoldTemplateKind("unknown") == null);

    assert(strEql(scaffoldTemplateName(ScaffoldTemplateKind.AiAgentService), "ai-agent"));
    assert(scaffoldTemplateDescription(ScaffoldTemplateKind.RealtimeMediaApp).len > 0);
});

test("Zero-alloc {{key}} and {{#if}}...{{else}}...{{/if}} template interpolation with strict/lenient modes", () => {
    const vars: [3]ScaffoldVarBinding = [
        ScaffoldVarBinding{ key: "name", value: "demo-app" },
        ScaffoldVarBinding{ key: "author", value: "Alice" },
        ScaffoldVarBinding{ key: "private", value: "false" },
    ];
    const tpl =
        "pkg={{ name }}\n" ++
        "{{#if author}}by={{author}}{{else}}anon{{/if}}\n" ++
        "{{#if private}}priv{{else}}pub{{/if}}\n" ++
        "missing={{unknown_var}}";

    let out: [256]u8 = [0; 256];
    let diag = ScaffoldRenderDiag.empty();

    // Lenient mode succeeds and records undefined variable diagnostic
    const len1 = renderScaffoldTemplate(tpl, vars[0..], ScaffoldRenderOptions.defaults(), out[0..], &diag) catch unreachable;
    assert(strEql(out[0..len1], "pkg=demo-app\nby=Alice\npub\nmissing="));
    assertEq(diag.undefined_count, @as(u32, 1));
    assert(strEql(diag.first_undefined_key, "unknown_var"));

    // Strict mode fails with UndefinedTemplateVar
    let strict_err = false;
    * = renderScaffoldTemplate(tpl, vars[0..], ScaffoldRenderOptions.strict(), out[0..], &diag) catch |e| {
        strict_err = (e == ScaffoldError.UndefinedTemplateVar);
    };
    assert(strict_err);

    // Unclosed tag rejected
    let unclosed_err = false;
    * = renderScaffoldTemplate("hello {{name", vars[0..], ScaffoldRenderOptions.defaults(), out[0..], &diag) catch |e| {
        unclosed_err = (e == ScaffoldError.UnclosedTemplateTag);
    };
    assert(unclosed_err);

    // Unmatched #if block rejected
    let unmatched_err = false;
    * = renderScaffoldTemplate("{{#if author}}open", vars[0..], ScaffoldRenderOptions.defaults(), out[0..], &diag) catch |e| {
        unmatched_err = (e == ScaffoldError.UnmatchedIfBlock);
    };
    assert(unmatched_err);
});

test("Structured cap.json manifest generator with escaping, scripts, workspaces, and deps", () => {
    const opts = ScaffoldProjectOptions{
        name: "@acme/api",
        version: "1.2.0",
        description: "Fast \"CP\" API",
        author: "Cap Team",
        license: "Apache-2.0",
        entry: "src/server.cp",
        template: ScaffoldTemplateKind.HttpApiServer,
    };
    const deps: [1]CapManifestDep = [
        CapManifestDep{ name: "@acme/core", spec: "^1.0.0" },
    ];
    const scripts: [2]CapManifestScript = [
        CapManifestScript{ name: "dev", command: "cap run src/server.cp" },
        CapManifestScript{ name: "test", command: "cap test" },
    ];
    const empty_ws: [0][]const u8 = [];

    let buf: [1024]u8 = [0; 1024];
    const n = formatCapManifestJson(opts, deps[0..], scripts[0..], empty_ws[0..], buf[0..]) catch unreachable;
    const json = buf[0..n];

    assert(containsSubstr(json, "\"name\": \"@acme/api\""));
    assert(containsSubstr(json, "\"description\": \"Fast \\\"CP\\\" API\""));
    assert(containsSubstr(json, "\"type\": \"app\""));
    assert(containsSubstr(json, "\"entry\": \"src/server.cp\""));
    assert(containsSubstr(json, "\"license\": \"Apache-2.0\""));
    assert(containsSubstr(json, "\"dev\": \"cap run src/server.cp\""));
    assert(containsSubstr(json, "\"@acme/core\": \"^1.0.0\""));
});

test("Conflict adjudication matrix for SkipExisting, Overwrite, MergeJson, and FailOnConflict", () => {
    assert(resolveScaffoldConflict(false, false, ScaffoldConflictPolicy.FailOnConflict) == ScaffoldFileAction.Create);
    assert(resolveScaffoldConflict(true, true, ScaffoldConflictPolicy.MergeJson) == ScaffoldFileAction.Merge);
    assert(resolveScaffoldConflict(false, true, ScaffoldConflictPolicy.MergeJson) == ScaffoldFileAction.Skip);
    assert(resolveScaffoldConflict(false, true, ScaffoldConflictPolicy.Overwrite) == ScaffoldFileAction.Overwrite);
    assert(resolveScaffoldConflict(false, true, ScaffoldConflictPolicy.SkipExisting) == ScaffoldFileAction.Skip);
    assert(resolveScaffoldConflict(false, true, ScaffoldConflictPolicy.FailOnConflict) == ScaffoldFileAction.ConflictError);
});

test("End-to-end planProjectScaffold with MergeJson policy and formatted report", () => {
    const opts = ScaffoldProjectOptions{
        name: "My Agent Service",
        description: "Autonomous BEAM actor agent",
        author: "CP Labs",
        template: ScaffoldTemplateKind.AiAgentService,
        conflict_policy: ScaffoldConflictPolicy.MergeJson,
    };
    const existing: [2][]const u8 = ["cap.json", ".gitignore"];
    const extra: [0]ScaffoldVarBinding = [];
    let planned: [16]ScaffoldPlannedFile = [ScaffoldPlannedFile.empty(); 16];
    let arena: [8192]u8 = [0; 8192];
    let apos: usize = 0;

    const summary = planProjectScaffold(opts, existing[0..], extra[0..], planned[0..], arena[0..], &apos) catch unreachable;
    assert(strEql(summary.project_name, "my-agent-service"));
    assertEq(summary.total_files, @as(u32, 6));
    assertEq(summary.merge_count, @as(u32, 1)); // cap.json
    assertEq(summary.skip_count, @as(u32, 1)); // .gitignore
    assertEq(summary.create_count, @as(u32, 4)); // src/main.cp, test/mainTest.cp, README.md, LICENSE

    assert(planned[0].action == ScaffoldFileAction.Merge);
    assert(containsSubstr(planned[0].rendered_content, "\"name\": \"my-agent-service\""));
    assert(containsSubstr(planned[1].rendered_content, "[agent] my-agent-service ready"));

    let rep_buf: [1024]u8 = [0; 1024];
    const rlen = formatScaffoldPlanReport(summary, planned[0..@as(usize, summary.total_files)], rep_buf[0..]) catch unreachable;
    const rep = rep_buf[0..rlen];
    assert(containsSubstr(rep, "scaffold my-agent-service (template=ai-agent files=6 create=4 skip=1 merge=1)"));
    assert(containsSubstr(rep, "  ~ merge cap.json\n"));
    assert(containsSubstr(rep, "  - skip .gitignore\n"));
    assert(containsSubstr(rep, "  + create src/main.cp\n"));
});

test("WorkspaceMonorepo template planning and FailOnConflict guard", () => {
    const ws_opts = ScaffoldProjectOptions{
        name: "acme-monorepo",
        template: ScaffoldTemplateKind.WorkspaceMonorepo,
        conflict_policy: ScaffoldConflictPolicy.FailOnConflict,
    };
    const empty_existing: [0][]const u8 = [];
    const extra: [0]ScaffoldVarBinding = [];
    let planned: [16]ScaffoldPlannedFile = [ScaffoldPlannedFile.empty(); 16];
    let arena: [8192]u8 = [0; 8192];
    let apos: usize = 0;

    const summary = planProjectScaffold(ws_opts, empty_existing[0..], extra[0..], planned[0..], arena[0..], &apos) catch unreachable;
    assert(containsSubstr(planned[0].rendered_content, "\"type\": \"workspace\""));
    assert(containsSubstr(planned[0].rendered_content, "\"private\": true"));
    assert(containsSubstr(planned[0].rendered_content, "\"workspaces\": [\"packages/*\"]"));
    assert(strEql(planned[1].rel_path, "packages/core/src/index.cp"));

    // FailOnConflict aborts when cap.json already exists
    const conflict_existing: [1][]const u8 = ["cap.json"];
    let conflict_err = false;
    * = planProjectScaffold(ws_opts, conflict_existing[0..], extra[0..], planned[0..], arena[0..], &apos) catch |e| {
        conflict_err = (e == ScaffoldError.ConflictDetected);
    };
    assert(conflict_err);
});
