import { test, assert, assertEq } from "cap:test";
import {
    PackRulesError,
    normalizePackRelPath,
    formatTarballEntryPath,
    formatTarballFileName,
    matchPackGlob,
    isAlwaysIncludedPackFile,
    isHardExcludedPackPath,
    isDefaultIgnoredPackPath,
    PackRuleKind,
    PackRulePattern,
    PackFileCandidate,
    PackDecisionReason,
    PackPlannedFile,
    PackPlanner,
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

test("Path normalization, traversal guards, tarball entry path & tarball filename", () => {
    let buf: [128]u8 = [0; 128];

    const n1 = normalizePackRelPath("./src//utils/./index.cp", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n1], "src/utils/index.cp"));

    const e1 = formatTarballEntryPath("./dist/cli.js", buf[0..]) catch unreachable;
    assert(strEql(buf[0..e1], "package/dist/cli.js"));

    // Scoped & unscoped tarball filenames
    const t1 = formatTarballFileName("@scope/ui-kit", "2.4.0", buf[0..]) catch unreachable;
    assert(strEql(buf[0..t1], "scope-ui-kit-2.4.0.tgz"));

    const t2 = formatTarballFileName("lodash", "4.17.21", buf[0..]) catch unreachable;
    assert(strEql(buf[0..t2], "lodash-4.17.21.tgz"));

    // Path traversal, absolute path, backslash, empty rejected
    let err_trav = false;
    * = normalizePackRelPath("src/../../etc/passwd", buf[0..]) catch |e| {
        err_trav = (e == PackRulesError.UnsafePath);
    };
    assert(err_trav);

    let err_abs = false;
    * = normalizePackRelPath("/abs/path.js", buf[0..]) catch |e| {
        err_abs = (e == PackRulesError.UnsafePath);
    };
    assert(err_abs);

    let err_empty = false;
    * = normalizePackRelPath("././", buf[0..]) catch |e| {
        err_empty = (e == PackRulesError.EmptyPath);
    };
    assert(err_empty);
});

test("matchPackGlob supports *, ?, [a-z], [!0-9], and ** multi-segment matching", () => {
    assert(matchPackGlob("dist/**/*.js", "dist/index.js"));
    assert(matchPackGlob("dist/**/*.js", "dist/a/b/c/mod.js"));
    assert(!matchPackGlob("dist/**/*.js", "dist/a/mod.ts"));
    assert(matchPackGlob("**/*.test.cp", "foo.test.cp"));
    assert(matchPackGlob("**/*.test.cp", "a/b/foo.test.cp"));
    assert(matchPackGlob("src/file-?.cp", "src/file-a.cp"));
    assert(!matchPackGlob("src/file-?.cp", "src/file-ab.cp"));
    assert(matchPackGlob("v[0-9].json", "v7.json"));
    assert(!matchPackGlob("v[0-9].json", "va.json"));
    assert(matchPackGlob("v[!0-9].json", "va.json"));
    assert(matchPackGlob("dist/**", "dist/a/b/c"));
});

test("Always-included, hard-excluded, and default-ignored classification", () => {
    assert(isAlwaysIncludedPackFile("package.json"));
    assert(isAlwaysIncludedPackFile("cap.json"));
    assert(isAlwaysIncludedPackFile("README.md"));
    assert(isAlwaysIncludedPackFile("readme.txt"));
    assert(isAlwaysIncludedPackFile("LICENSE"));
    assert(isAlwaysIncludedPackFile("LICENCE.md"));
    assert(isAlwaysIncludedPackFile("CHANGELOG.md"));
    assert(!isAlwaysIncludedPackFile("docs/README.md"));

    assert(isHardExcludedPackPath(".git/config", false));
    assert(isHardExcludedPackPath("a/b/.npmrc", false));
    assert(isHardExcludedPackPath("node_modules/pkg/index.js", false));
    assert(isHardExcludedPackPath("cap_modules/pkg/index.cp", false));
    // Bundled dep permits node_modules but still blocks .git
    assert(!isHardExcludedPackPath("node_modules/pkg/index.js", true));
    assert(isHardExcludedPackPath("node_modules/pkg/.git/HEAD", true));

    assert(isDefaultIgnoredPackPath("bun.lock"));
    assert(isDefaultIgnoredPackPath("package-lock.json"));
    assert(isDefaultIgnoredPackPath("src/.DS_Store"));
    assert(isDefaultIgnoredPackPath("src/._index.cp"));
    assert(isDefaultIgnoredPackPath(".env.production"));
    assert(isDefaultIgnoredPackPath("bunfig.toml"));
    assert(!isDefaultIgnoredPackPath("src/index.cp"));
});

test("PackPlanner files whitelist + negation + always-included + entrypoints + bundled deps", () => {
    let f_rules: [8]PackRulePattern = [PackRulePattern.empty(); 8];
    let i_rules: [8]PackRulePattern = [PackRulePattern.empty(); 8];
    let entries: [4][]const u8 = [""; 4];
    let planned: [16]PackPlannedFile = [PackPlannedFile.empty(); 16];
    let planner = PackPlanner.init(f_rules[0..], i_rules[0..], entries[0..], planned[0..]);

    planner.addFilesEntry("dist") catch unreachable;
    planner.addFilesEntry("!dist/**/*.map") catch unreachable;
    planner.addEntrypoint("bin/cli.js") catch unreachable;

    const candidates: [10]PackFileCandidate = [
        PackFileCandidate.file("src/internal.ts", 500),
        PackFileCandidate.file("dist/index.js", 1200),
        PackFileCandidate.file("dist/index.js.map", 3000),
        PackFileCandidate.binFile("bin/cli.js", 400),
        PackFileCandidate.file("package.json", 320),
        PackFileCandidate.file("README.md", 800),
        PackFileCandidate.file("LICENSE", 1024),
        PackFileCandidate.file(".git/HEAD", 24),
        PackFileCandidate.file("bun.lock", 4096),
        PackFileCandidate.bundled("node_modules/tiny-dep/index.js", 150),
    ];

    const summary = planner.planCandidates(candidates[0..]) catch unreachable;
    assertEq(summary.total_candidates, 10);
    assertEq(summary.packed_files, 6); // LICENSE, README.md, bin/cli.js, dist/index.js, node_modules/tiny-dep/index.js, package.json
    assertEq(summary.bundled_files, 1);
    assertEq(summary.ignored_files, 4);
    assertEq(summary.unpacked_bytes, 1200 + 400 + 320 + 800 + 1024 + 150);
    assert(summary.has_readme);
    assert(summary.has_license);
    assert(summary.manifest_hash64 != 0);

    // Deterministic lexicographic sorting verified
    assert(strEql(planned[0].rel_path, "LICENSE"));
    assert(strEql(planned[1].rel_path, "README.md"));
    assert(strEql(planned[2].rel_path, "bin/cli.js"));
    assert(strEql(planned[3].rel_path, "dist/index.js"));
    assert(strEql(planned[4].rel_path, "node_modules/tiny-dep/index.js"));
    assert(strEql(planned[5].rel_path, "package.json"));

    let rep: [512]u8 = [0; 512];
    const rn = planner.formatPackReport("@cap/demo", "1.0.0", rep[0..]) catch unreachable;
    assert(rn > 60);
});

test("PackPlanner .npmignore takes precedence over .gitignore in same directory & supports !reinclude", () => {
    let f_rules: [4]PackRulePattern = [PackRulePattern.empty(); 4];
    let i_rules: [16]PackRulePattern = [PackRulePattern.empty(); 16];
    let entries: [2][]const u8 = [""; 2];
    let planned: [16]PackPlannedFile = [PackPlannedFile.empty(); 16];
    let planner = PackPlanner.init(f_rules[0..], i_rules[0..], entries[0..], planned[0..]);

    // Root .npmignore ignores `*.log` and `tests/`, but re-includes `tests/fixtures/**`
    const added_npm = planner.addIgnoreFile(
        "# comment\n*.log\ntests/\n!tests/fixtures/**\n",
        "",
        0,
        PackRuleKind.npm_ignore,
    ) catch unreachable;
    assertEq(added_npm, 3);

    // Root .gitignore tries to ignore `dist/` -> MUST be skipped because root `.npmignore` exists!
    const added_git = planner.addIgnoreFile("dist/\n", "", 0, PackRuleKind.git_ignore) catch unreachable;
    assertEq(added_git, 0);

    // Subdirectory `src` has NO `.npmignore`, so `src/.gitignore` IS honored
    const added_sub = planner.addIgnoreFile("*.tmp\n", "src", 1, PackRuleKind.git_ignore) catch unreachable;
    assertEq(added_sub, 1);

    assert((planner.evaluateCandidate(PackFileCandidate.file("dist/app.js", 100)) catch unreachable) == PackDecisionReason.default_included);
    assert((planner.evaluateCandidate(PackFileCandidate.file("debug.log", 50)) catch unreachable) == PackDecisionReason.ignore_rule_excluded);
    assert((planner.evaluateCandidate(PackFileCandidate.file("tests/unit.test.cp", 50)) catch unreachable) == PackDecisionReason.ignore_rule_excluded);
    assert((planner.evaluateCandidate(PackFileCandidate.file("tests/fixtures/sample.json", 50)) catch unreachable) == PackDecisionReason.negated_reincluded);
    assert((planner.evaluateCandidate(PackFileCandidate.file("src/scratch.tmp", 10)) catch unreachable) == PackDecisionReason.ignore_rule_excluded);
    // `root.tmp` is outside `src/` so `src/.gitignore` does NOT match it
    assert((planner.evaluateCandidate(PackFileCandidate.file("root.tmp", 10)) catch unreachable) == PackDecisionReason.default_included);
});

test("PackPlanner buffer capacity and error guards", () => {
    let f_rules: [1]PackRulePattern = [PackRulePattern.empty(); 1];
    let i_rules: [1]PackRulePattern = [PackRulePattern.empty(); 1];
    let entries: [1][]const u8 = [""];
    let planned: [1]PackPlannedFile = [PackPlannedFile.empty(); 1];
    let planner = PackPlanner.init(f_rules[0..], i_rules[0..], entries[0..], planned[0..]);

    planner.addFilesEntry("dist") catch unreachable;
    let rule_full = false;
    * = planner.addFilesEntry("src") catch |e| { rule_full = (e == PackRulesError.RuleBufferFull); };
    assert(rule_full);

    planner.addEntrypoint("index.js") catch unreachable;
    let ep_full = false;
    * = planner.addEntrypoint("cli.js") catch |e| { ep_full = (e == PackRulesError.EntrypointBufferFull); };
    assert(ep_full);

    const cands: [2]PackFileCandidate = [
        PackFileCandidate.file("package.json", 10),
        PackFileCandidate.file("README.md", 10),
    ];
    let plan_full = false;
    * = planner.planCandidates(cands[0..]) catch |e| { plan_full = (e == PackRulesError.PlanBufferFull); };
    assert(plan_full);

    let tiny: [4]u8 = [0; 4];
    let buf_small = false;
    * = formatTarballFileName("pkg", "1.0.0", tiny[0..]) catch |e| { buf_small = (e == PackRulesError.BufferTooSmall); };
    assert(buf_small);
});
