// cap/test/test_cli_filter_arg_production_deep.cp
// Production-deep regression tests for cap:cli filter_arg module
// Source: /data/dev/bun/src/cli/filter_arg.cp & /data/dev/bun/docs/pm/filter.mdx

import { test, expect } from "cap:test";
import {
    FILTER_MAX_PATTERNS,
    FilterArgError,
    FilterPatternKind,
    FilterSet,
    isFilterSkippedDir,
    formatWorkspacePackagePattern,
    normalizeFilterPath,
    matchFilterGlob,
    formatFilterScriptPrefix,
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

test("cli filter_arg: package name glob & match_all semantics") {
    const pats1: [1]([]const u8) = ["pkg-*"];
    const fs1 = FilterSet.fromPatterns(pats1[0..], "/repo") catch {
        expect(false);
        return;
    };
    expect(fs1.has_name_filters);
    expect(!fs1.match_all);
    expect(fs1.matches("/repo/packages/a", "pkg-a"));
    expect(fs1.matches("/repo/packages/b", "pkg-b"));
    expect(!fs1.matches("/repo/packages/other", "other"));

    // Scoped package name + character class + '?'
    const pats2: [2]([]const u8) = ["@acme/pkg-[a-c]", "tool-?"];
    const fs2 = FilterSet.fromPatterns(pats2[0..], "/repo") catch {
        expect(false);
        return;
    };
    expect(fs2.matches("/repo/a", "@acme/pkg-a"));
    expect(fs2.matches("/repo/c", "@acme/pkg-c"));
    expect(!fs2.matches("/repo/d", "@acme/pkg-d"));
    expect(fs2.matches("/repo/t1", "tool-1"));
    expect(!fs2.matches("/repo/t12", "tool-12"));

    // `--filter="*"` + `--filter="./bar"`: match_all requires non-empty name,
    // while unnamed packages can still match via explicit path filter.
    const pats3: [2]([]const u8) = ["*", "./bar"];
    const fs3 = FilterSet.fromPatterns(pats3[0..], "/repo") catch {
        expect(false);
        return;
    };
    expect(fs3.match_all);
    expect(fs3.matches("/repo/anywhere", "named-pkg"));
    expect(fs3.matches("/repo/bar", ""));
    expect(!fs3.matches("/repo/baz", ""));
}

test("cli filter_arg: path patterns with cwd normalization & windows separators") {
    let buf: [128]u8 = [0; 128];
    const n1 = normalizeFilterPath("/repo/apps/web", "../../packages//ui/", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n1], "/repo/packages/ui"));

    const n2 = normalizeFilterPath("C:\\workspace\\repo", ".\\packages\\*\\", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n2], "C:/workspace/repo/packages/*"));

    const pats: [2]([]const u8) = ["./packages/*", "./apps/**"];
    const fs = FilterSet.fromPatterns(pats[0..], "/repo") catch {
        expect(false);
        return;
    };
    expect(fs.has_path_filters);
    expect(!fs.has_name_filters);
    expect(fs.matches("/repo/packages/core", "core"));
    expect(!fs.matches("/repo/packages/core/sub", "sub"));
    expect(fs.matches("/repo/apps", "apps-root"));
    expect(fs.matches("/repo/apps/web/admin", "admin"));
    expect(fs.matches("/repo\\packages\\cli\\", "cli"));
    expect(!fs.matches("/repo/tools/lint", "lint"));
}

test("cli filter_arg: negation filters (!pkg-c, !./) and combined rules") {
    // Pure negation: `--filter '!pkg-c'` selects everything except `pkg-c`
    const pats1: [1]([]const u8) = ["!pkg-c"];
    const fs1 = FilterSet.fromPatterns(pats1[0..], "/repo") catch {
        expect(false);
        return;
    };
    expect(fs1.has_negated_filters);
    expect(!fs1.has_positive_filters);
    expect(fs1.matches("/repo/packages/a", "pkg-a"));
    expect(fs1.matches("/repo/packages/b", "pkg-b"));
    expect(!fs1.matches("/repo/packages/c", "pkg-c"));

    // Combined: `--filter '!./' --filter './packages/*'` (exclude root package.json)
    const pats2: [2]([]const u8) = ["!./", "./packages/*"];
    const fs2 = FilterSet.fromPatterns(pats2[0..], "/repo") catch {
        expect(false);
        return;
    };
    expect(!fs2.matches("/repo", "root"));
    expect(fs2.matches("/repo/packages/pkg-a", "pkg-a"));
    expect(!fs2.matches("/repo/other/pkg-x", "pkg-x"));

    // Negation overrides match_all (`--filter '*' --filter '!legacy-*'`)
    const pats3: [2]([]const u8) = ["*", "!legacy-*"];
    const fs3 = FilterSet.fromPatterns(pats3[0..], "/repo") catch {
        expect(false);
        return;
    };
    expect(fs3.matches("/repo/packages/app", "app"));
    expect(!fs3.matches("/repo/packages/old", "legacy-v1"));

    // Inspect compiled rule metadata
    if (r0 := fs2.ruleAt(0)) {
        expect(r0.isPath());
        expect(r0.isNegated());
        expect(strEq(r0.pattern(), "/repo"));
    } else {
        expect(false);
    }
    expect(fs2.count() == 2);
}

test("cli filter_arg: workspace package.json pattern & skipped dir guard") {
    expect(isFilterSkippedDir("."));
    expect(isFilterSkippedDir("node_modules"));
    expect(isFilterSkippedDir(".git"));
    expect(!isFilterSkippedDir(""));
    expect(!isFilterSkippedDir("packages"));
    expect(!isFilterSkippedDir(".github"));

    let out: [64]u8 = [0; 64];
    const n1 = formatWorkspacePackagePattern("packages/*", out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n1], "packages/*/package.json"));

    const n2 = formatWorkspacePackagePattern("apps\\web///", out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n2], "apps/web/package.json"));

    const n3 = formatWorkspacePackagePattern("/", out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n3], "package.json"));

    // Empty workspace pattern rejected
    let empty_err = false;
    * = formatWorkspacePackagePattern("", out[0..]) catch |e| {
        expect(e == FilterArgError.InvalidWorkspacePattern);
        empty_err = true;
    };
    expect(empty_err);

    // Output buffer too small rejected
    let tiny: [8]u8 = [0; 8];
    let small_err = false;
    * = formatWorkspacePackagePattern("packages/*", tiny[0..]) catch |e| {
        expect(e == FilterArgError.BufferTooSmall);
        small_err = true;
    };
    expect(small_err);
}

test("cli filter_arg: iterative glob edge cases & pathological multi-star defense") {
    // Zero-depth, single-depth, multi-depth `**`
    expect(matchFilterGlob("**/package.json", "package.json"));
    expect(matchFilterGlob("**/package.json", "a/package.json"));
    expect(matchFilterGlob("**/package.json", "a/b/c/package.json"));
    expect(matchFilterGlob("**/package.json", "/abs/a/b/package.json"));
    expect(matchFilterGlob("packages/**/src/*.cp", "packages/src/main.cp"));
    expect(matchFilterGlob("packages/**/src/*.cp", "packages/cli/sub/src/filter.cp"));
    expect(!matchFilterGlob("packages/**/src/*.cp", "packages/cli/src/a/b.cp"));

    // Negated char class & unclosed bracket literal fallback
    expect(matchFilterGlob("pkg-[!c]", "pkg-a"));
    expect(!matchFilterGlob("pkg-[!c]", "pkg-c"));
    expect(matchFilterGlob("file[1", "file[1"));
    expect(!matchFilterGlob("file[1", "file1"));

    // Absolute vs relative mismatch protection
    expect(!matchFilterGlob("/packages/*", "packages/a"));
    expect(!matchFilterGlob("packages/*", "/packages/a"));

    // Pathological multi-star input must finish fast in linear/bounded time
    expect(!matchFilterGlob("********************z", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    expect(!matchFilterGlob("**/**/**/**/**/target", "a/b/c/d/e/f/g/h/i/j/k/l/m/n/miss"));
    expect(matchFilterGlob("**/**/**/**/**/target", "a/b/c/d/e/f/g/h/target"));
}

test("cli filter_arg: malformed inputs, capacity limits & error handling") {
    let fs = FilterSet.init();

    // Empty pattern & bare negation rejected
    let got_empty1 = false;
    fs.addPattern("", "/repo") catch |e| {
        expect(e == FilterArgError.EmptyPattern);
        got_empty1 = true;
    };
    expect(got_empty1);

    let got_empty2 = false;
    fs.addPattern("!", "/repo") catch |e| {
        expect(e == FilterArgError.EmptyPattern);
        got_empty2 = true;
    };
    expect(got_empty2);

    // Fill up to FILTER_MAX_PATTERNS, then verify overflow returns TooManyPatterns
    let i: usize = 0;
    while (i < FILTER_MAX_PATTERNS) {
        fs.addPattern("pkg-*", "/repo") catch {
            expect(false);
            return;
        };
        i += 1;
    }
    expect(fs.count() == FILTER_MAX_PATTERNS);
    expect(fs.ruleAt(FILTER_MAX_PATTERNS) == null);

    let got_overflow = false;
    fs.addPattern("extra", "/repo") catch |e| {
        expect(e == FilterArgError.TooManyPatterns);
        got_overflow = true;
    };
    expect(got_overflow);

    // Tiny output buffer for normalizeFilterPath
    let tiny: [4]u8 = [0; 4];
    let got_small = false;
    * = normalizeFilterPath("/repo", "./packages/core", tiny[0..]) catch |e| {
        expect(e == FilterArgError.BufferTooSmall);
        got_small = true;
    };
    expect(got_small);
}

test("cli filter_arg: Foreman-style script prefix formatting & round-trip") {
    let buf: [64]u8 = [0; 64];

    // Named package
    const n1 = formatFilterScriptPrefix("pkg-a", "packages/a", "build", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n1], "pkg-a:build"));

    // Unnamed package falls back to relative path
    const n2 = formatFilterScriptPrefix("", "packages/anon", "test", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n2], "packages/anon:test"));

    // Both empty falls back to bare script name
    const n3 = formatFilterScriptPrefix("", "", "lint", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n3], "lint"));

    // Buffer too small protection
    let tiny: [4]u8 = [0; 4];
    let small_err = false;
    * = formatFilterScriptPrefix("pkg-a", "", "build", tiny[0..]) catch |e| {
        expect(e == FilterArgError.BufferTooSmall);
        small_err = true;
    };
    expect(small_err);
}

test("cli filter_arg: matchesPath standalone & path traversal clamping") {
    // Absolute path traversal above root clamps safely to `/`
    let buf: [64]u8 = [0; 64];
    const n = normalizeFilterPath("/repo", "../../../../etc/passwd", buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n], "/etc/passwd"));

    const pats: [2]([]const u8) = ["./packages/**", "!./packages/deprecated/**"];
    const fs = FilterSet.fromPatterns(pats[0..], "/repo") catch {
        expect(false);
        return;
    };
    expect(fs.matchesPath("/repo/packages/core"));
    expect(fs.matchesPath("/repo/packages/ui/button"));
    expect(!fs.matchesPath("/repo/packages/deprecated"));
    expect(!fs.matchesPath("/repo/packages/deprecated/old"));
    expect(!fs.matchesPath("/repo/apps/web"));
}
