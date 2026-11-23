import { test, assert, assertEq } from "cap:test";
import {
    CreateTemplateError,
    CreateSourceKind,
    normalizeCreatePackageName,
    parseCreateTemplateSpec,
    CreateDestStatus,
    evaluateCreateDestination,
    CreateStepKind,
    CreateWorkflowStep,
    CreatePostMetadata,
    CreateWorkflowOptions,
    shouldSkipTemplateFile,
    resolveTemplateDestFilename,
    planCreateWorkflow,
    CreateSummaryInfo,
    formatCreateNextSteps,
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

test("parseCreateTemplateSpec classifies OfficialBuiltin and all GitHubRepo forms accurately", () => {
    let nbuf: [128]u8 = [0; 128];

    const b1 = parseCreateTemplateSpec("agent", nbuf[0..]) catch unreachable;
    assert(b1.kind == CreateSourceKind.OfficialBuiltin);
    assert(strEql(b1.builtin_name, "agent"));

    const g1 = parseCreateTemplateSpec("oven-sh/bun-starter#v2", nbuf[0..]) catch unreachable;
    assert(g1.kind == CreateSourceKind.GitHubRepo);
    assert(strEql(g1.github_owner, "oven-sh"));
    assert(strEql(g1.github_repo, "bun-starter"));
    assert(strEql(g1.github_ref, "v2"));
    assert(strEql(g1.default_project_name, "bun-starter"));

    const g2 = parseCreateTemplateSpec("github:cap-lang/templates/examples/web#main", nbuf[0..]) catch unreachable;
    assert(g2.kind == CreateSourceKind.GitHubRepo);
    assert(strEql(g2.github_owner, "cap-lang"));
    assert(strEql(g2.github_repo, "templates"));
    assert(strEql(g2.github_subpath, "examples/web"));
    assert(strEql(g2.github_ref, "main"));
    assert(strEql(g2.default_project_name, "web"));

    const g3 = parseCreateTemplateSpec("https://github.com/cap-lang/starters/tree/release-1/media", nbuf[0..]) catch unreachable;
    assert(g3.kind == CreateSourceKind.GitHubRepo);
    assert(strEql(g3.github_owner, "cap-lang"));
    assert(strEql(g3.github_repo, "starters"));
    assert(strEql(g3.github_ref, "release-1"));
    assert(strEql(g3.github_subpath, "media"));
});

test("parseCreateTemplateSpec handles NpmOrCapPackage create- prefixing, LocalPath, and TarballUrl", () => {
    let nbuf: [128]u8 = [0; 128];

    const p1 = parseCreateTemplateSpec("vite", nbuf[0..]) catch unreachable;
    assert(p1.kind == CreateSourceKind.NpmOrCapPackage);
    assert(strEql(p1.package_name, "create-vite"));

    const p2 = parseCreateTemplateSpec("create-vite@5.4.0", nbuf[0..]) catch unreachable;
    assert(p2.kind == CreateSourceKind.NpmOrCapPackage);
    assert(strEql(p2.package_name, "create-vite"));
    assert(strEql(p2.package_version, "5.4.0"));

    const p3 = parseCreateTemplateSpec("@acme/app@1.0.0", nbuf[0..]) catch unreachable;
    assert(p3.kind == CreateSourceKind.NpmOrCapPackage);
    assert(strEql(p3.package_name, "@acme/create-app"));
    assert(strEql(p3.package_version, "1.0.0"));

    const p4 = normalizeCreatePackageName("@acme", nbuf[0..]) catch unreachable;
    assert(strEql(nbuf[0..p4], "@acme/create"));

    const l1 = parseCreateTemplateSpec("./templates/my-starter", nbuf[0..]) catch unreachable;
    assert(l1.kind == CreateSourceKind.LocalPath);
    assert(strEql(l1.local_path, "./templates/my-starter"));
    assert(strEql(l1.default_project_name, "my-starter"));

    const t1 = parseCreateTemplateSpec("https://registry.cap.dev/templates/starter-v1.tgz", nbuf[0..]) catch unreachable;
    assert(t1.kind == CreateSourceKind.TarballUrl);
    assert(strEql(t1.default_project_name, "starter-v1"));
});

test("parseCreateTemplateSpec blocks shell injection, non-HTTPS tarballs, bad hostnames, and path traversal", () => {
    let nbuf: [128]u8 = [0; 128];

    // Shell metacharacter injection
    let shell_err = false;
    * = parseCreateTemplateSpec("owner/repo;echo pwned", nbuf[0..]) catch |e| {
        shell_err = (e == CreateTemplateError.UnsafeShellMetachar);
    };
    assert(shell_err);

    // Non-HTTPS tarball URL rejected
    let http_err = false;
    * = parseCreateTemplateSpec("http://example.com/tpl.tgz", nbuf[0..]) catch |e| {
        http_err = (e == CreateTemplateError.InvalidTarballUrl);
    };
    assert(http_err);

    // Invalid hostname rejected
    let host_err = false;
    * = parseCreateTemplateSpec("https://bad..host/tpl.tgz", nbuf[0..]) catch |e| {
        host_err = (e == CreateTemplateError.InvalidTarballUrl);
    };
    assert(host_err);

    // Path traversal in GitHub subpath rejected
    let trav_err = false;
    * = parseCreateTemplateSpec("owner/repo/../../etc", nbuf[0..]) catch |e| {
        trav_err = (e == CreateTemplateError.PathTraversalDetected);
    };
    assert(trav_err);
});

test("evaluateCreateDestination distinguishes empty, safe-ignored, conflicting, --force, and unsafe paths", () => {
    const empty_entries: [0][]const u8 = [];
    const d1 = evaluateCreateDestination("my-app", empty_entries[0..], false);
    assert(d1.allowed && d1.status == CreateDestStatus.CleanEmpty);

    const ignored_entries: [4][]const u8 = [".git", ".DS_Store", "LICENSE", "README.md"];
    const d2 = evaluateCreateDestination("my-app", ignored_entries[0..], false);
    assert(d2.allowed && d2.status == CreateDestStatus.CleanOnlyIgnored);
    assertEq(d2.ignored_entry_count, @as(u32, 4));

    const conflict_entries: [3][]const u8 = [".git", "cap.json", "src"];
    const d3 = evaluateCreateDestination("my-app", conflict_entries[0..], false);
    assert(!d3.allowed && d3.status == CreateDestStatus.BlockedNonEmpty);
    assertEq(d3.conflict_entry_count, @as(u32, 2));
    assert(strEql(d3.first_conflict_entry, "cap.json"));

    const d4 = evaluateCreateDestination("my-app", conflict_entries[0..], true);
    assert(d4.allowed && d4.status == CreateDestStatus.ForceOverwriteAllowed);

    const d5 = evaluateCreateDestination("../outside", empty_entries[0..], true);
    assert(!d5.allowed && d5.status == CreateDestStatus.BlockedUnsafePath);
});

test("shouldSkipTemplateFile filters VCS/modules/lockfiles and resolveTemplateDestFilename renames dotfiles & .tmpl", () => {
    assert(shouldSkipTemplateFile(".git/HEAD"));
    assert(shouldSkipTemplateFile("node_modules/zod/index.js"));
    assert(shouldSkipTemplateFile("cap_modules/std/index.cp"));
    assert(shouldSkipTemplateFile("package-lock.json"));
    assert(shouldSkipTemplateFile("yarn.lock"));
    assert(shouldSkipTemplateFile("pnpm-lock.yaml"));
    assert(shouldSkipTemplateFile("cap.lock"));
    assert(!shouldSkipTemplateFile("src/main.cp"));
    assert(!shouldSkipTemplateFile("cap.json"));

    let out: [128]u8 = [0; 128];
    const r1 = resolveTemplateDestFilename("gitignore", out[0..]) catch unreachable;
    assert(strEql(out[0..r1], ".gitignore"));

    const r2 = resolveTemplateDestFilename("packages/app/_gitignore", out[0..]) catch unreachable;
    assert(strEql(out[0..r2], "packages/app/.gitignore"));

    const r3 = resolveTemplateDestFilename("_npmignore", out[0..]) catch unreachable;
    assert(strEql(out[0..r3], ".npmignore"));

    const r4 = resolveTemplateDestFilename("cap.json.tmpl", out[0..]) catch unreachable;
    assert(strEql(out[0..r4], "cap.json"));

    let trav_err = false;
    * = resolveTemplateDestFilename("../evil.txt", out[0..]) catch |e| {
        trav_err = (e == CreateTemplateError.PathTraversalDetected);
    };
    assert(trav_err);
});

test("planCreateWorkflow plans ordered steps for remote vs builtin templates and honors --no-install/--no-git", () => {
    let nbuf: [64]u8 = [0; 64];
    const gh = parseCreateTemplateSpec("cap-lang/agent-starter", nbuf[0..]) catch unreachable;
    let steps: [8]CreateWorkflowStep = [CreateWorkflowStep.empty(); 8];

    const n1 = planCreateWorkflow(
        gh,
        CreatePostMetadata{ install_deps: true, git_init: true, post_create_script: "cap run setup" },
        CreateWorkflowOptions.defaults(),
        steps[0..],
    ) catch unreachable;
    assertEq(n1, @as(usize, 5));
    assert(steps[0].kind == CreateStepKind.FetchTemplate);
    assert(steps[1].kind == CreateStepKind.CopyAndTransformFiles);
    assert(steps[2].kind == CreateStepKind.InstallDependencies);
    assert(steps[3].kind == CreateStepKind.InitGitRepository);
    assert(steps[4].kind == CreateStepKind.RunPostCreateScript);

    // Builtin template skips FetchTemplate, and --no-install / --no-git prune those steps
    const bi = parseCreateTemplateSpec("app", nbuf[0..]) catch unreachable;
    const n2 = planCreateWorkflow(
        bi,
        CreatePostMetadata.defaults(),
        CreateWorkflowOptions{ no_install: true, no_git: true },
        steps[0..],
    ) catch unreachable;
    assertEq(n2, @as(usize, 1));
    assert(steps[0].kind == CreateStepKind.CopyAndTransformFiles);
});

test("formatCreateNextSteps renders actionable onboarding guidance and omits redundant steps", () => {
    let buf: [512]u8 = [0; 512];
    const n1 = formatCreateNextSteps(CreateSummaryInfo{
        project_name: "my-app",
        dest_dir: "my-app",
        template_label: "cap-lang/agent-starter",
        files_copied: 14,
        deps_installed: false,
        git_initialized: true,
        dev_script: "dev",
        is_current_dir: false,
    }, false, buf[0..]) catch unreachable;
    const s1 = buf[0..n1];
    assert(strContains(s1, "Created my-app from cap-lang/agent-starter (14 file(s), git initialized)"));
    assert(strContains(s1, "  cd my-app\n"));
    assert(strContains(s1, "  cap install\n"));
    assert(strContains(s1, "  cap run dev\n"));

    // Current dir + already installed -> omits `cd` and `cap install`
    const n2 = formatCreateNextSteps(CreateSummaryInfo{
        project_name: "in-place",
        dest_dir: ".",
        template_label: "app",
        files_copied: 5,
        deps_installed: true,
        git_initialized: false,
        dev_script: "start",
        is_current_dir: true,
    }, false, buf[0..]) catch unreachable;
    const s2 = buf[0..n2];
    assert(!strContains(s2, "cd "));
    assert(!strContains(s2, "cap install"));
    assert(strContains(s2, "  cap run start\n"));
});
