import std from "std";
import { test, expect_eq } from "cap:test";
import {
    WorkspaceFilterError,
    WorkspaceFilterKind,
    WorkspaceFilterPattern,
    isIgnoredFilterDir,
    normalizeFilterPath,
    matchFilterGlob,
    WorkspaceFilter,
} from "cap:pkg";

test("1. isIgnoredFilterDir and normalizeFilterPath handle hidden dirs, Windows slashes, and parent segments", () => {
    const alloc = std.testing.allocator;

    expect_eq(isIgnoredFilterDir("."), true);
    expect_eq(isIgnoredFilterDir(".git"), true);
    expect_eq(isIgnoredFilterDir(".cache"), true);
    expect_eq(isIgnoredFilterDir("node_modules"), true);
    expect_eq(isIgnoredFilterDir("packages"), false);
    expect_eq(isIgnoredFilterDir(""), false);

    const p1 = normalizeFilterPath(alloc, "", "./packages\\core//") catch unreachable;
    defer alloc.free(p1);
    expect_eq(std.mem.eql(u8, p1, "packages/core"), true);

    const p2 = normalizeFilterPath(alloc, "", "./") catch unreachable;
    defer alloc.free(p2);
    expect_eq(std.mem.eql(u8, p2, "."), true);

    const p3 = normalizeFilterPath(alloc, "/repo/monorepo/", "./packages/../apps/web") catch unreachable;
    defer alloc.free(p3);
    expect_eq(std.mem.eql(u8, p3, "/repo/monorepo/apps/web"), true);
});

test("2. matchFilterGlob supports *, **, ?, path-segment isolation, and zero-depth /**/", () => {
    // 名称模式: * 可匹配作用域斜杠
    expect_eq(matchFilterGlob("@acme/*", "@acme/core", false), true);
    expect_eq(matchFilterGlob("pkg-?", "pkg-a", false), true);
    expect_eq(matchFilterGlob("pkg-?", "pkg-ab", false), false);

    // 路径模式: 单 * 不跨 /，** 可跨多层及零层
    expect_eq(matchFilterGlob("packages/*", "packages/core", true), true);
    expect_eq(matchFilterGlob("packages/*", "packages/core/sub", true), false);
    expect_eq(matchFilterGlob("packages/**", "packages/core/sub/deep", true), true);
    expect_eq(matchFilterGlob("packages/**/ui", "packages/ui", true), true);
    expect_eq(matchFilterGlob("packages/**/ui", "packages/a/b/ui", true), true);
    expect_eq(matchFilterGlob("packages/?/ui", "packages///ui", true), false);
});

test("3. WorkspaceFilter matches package names and allows all when empty", () => {
    const alloc = std.testing.allocator;

    const empty_rules: [0][]const u8 = [];
    let wf_all = WorkspaceFilter.init(alloc, empty_rules[0..], "") catch unreachable;
    defer wf_all.deinit();
    expect_eq(wf_all.patternCount(), usize(0));
    expect_eq(wf_all.matches("packages/any", "anything"), true);

    const rules: [2][]const u8 = ["pkg-*", "@acme/core"];
    let wf = WorkspaceFilter.init(alloc, rules[0..], "") catch unreachable;
    defer wf.deinit();

    expect_eq(wf.has_name_filters, true);
    expect_eq(wf.has_path_filters, false);
    expect_eq(wf.matches("packages/a", "pkg-a"), true);
    expect_eq(wf.matches("packages/core", "@acme/core"), true);
    expect_eq(wf.matches("packages/other", "other"), false);
});

test("4. WorkspaceFilter path patterns with ./ root and cwd resolution", () => {
    const alloc = std.testing.allocator;

    const root_only: [1][]const u8 = ["./"];
    let wf_root = WorkspaceFilter.init(alloc, root_only[0..], "") catch unreachable;
    defer wf_root.deinit();
    expect_eq(wf_root.has_path_filters, true);
    expect_eq(wf_root.matches("./", "root-pkg"), true);
    expect_eq(wf_root.matches(".", "root-pkg"), true);
    expect_eq(wf_root.matches("packages/a", "pkg-a"), false);

    const with_cwd: [1][]const u8 = ["./packages/**"];
    let wf_cwd = WorkspaceFilter.init(alloc, with_cwd[0..], "/ws/app") catch unreachable;
    defer wf_cwd.deinit();
    expect_eq(wf_cwd.matchesPath("/ws/app/packages/api"), true);
    expect_eq(wf_cwd.matchesPath("/ws/app/packages/api/v2"), true);
    expect_eq(wf_cwd.matchesPath("/ws/app/apps/web"), false);
});

test("5. WorkspaceFilter negation semantics: pure-negation and positive+negation combinations", () => {
    const alloc = std.testing.allocator;

    // 纯否定规则: 匹配除 pkg-c 之外的全部包
    const neg_only: [1][]const u8 = ["!pkg-c"];
    let wf_neg = WorkspaceFilter.init(alloc, neg_only[0..], "") catch unreachable;
    defer wf_neg.deinit();
    expect_eq(wf_neg.has_positive_filters, false);
    expect_eq(wf_neg.matches("packages/a", "pkg-a"), true);
    expect_eq(wf_neg.matches("packages/b", "pkg-b"), true);
    expect_eq(wf_neg.matches("packages/c", "pkg-c"), false);

    // 正负组合: 匹配 ./packages/* 但排除根包与 pkg-legacy
    const mixed: [3][]const u8 = ["!./", "./packages/*", "!pkg-legacy"];
    let wf_mix = WorkspaceFilter.init(alloc, mixed[0..], "") catch unreachable;
    defer wf_mix.deinit();
    expect_eq(wf_mix.has_positive_filters, true);
    expect_eq(wf_mix.matches(".", "root"), false);
    expect_eq(wf_mix.matches("packages/core", "pkg-core"), true);
    expect_eq(wf_mix.matches("packages/legacy", "pkg-legacy"), false);
    expect_eq(wf_mix.matches("apps/web", "web"), false);
});

test("6. WorkspaceFilter rejects empty patterns and normalizes backslash input paths", () => {
    const alloc = std.testing.allocator;

    const bad1: [1][]const u8 = [""];
    let got_empty1 = false;
    * = WorkspaceFilter.init(alloc, bad1[0..], "") catch |e| {
        if (e == WorkspaceFilterError.EmptyPattern) got_empty1 = true;
    };
    expect_eq(got_empty1, true);

    const bad2: [1][]const u8 = ["!"];
    let got_empty2 = false;
    * = WorkspaceFilter.init(alloc, bad2[0..], "") catch |e| {
        if (e == WorkspaceFilterError.EmptyPattern) got_empty2 = true;
    };
    expect_eq(got_empty2, true);

    const win_rule: [1][]const u8 = [".\\packages\\*"];
    let wf_win = WorkspaceFilter.init(alloc, win_rule[0..], "") catch unreachable;
    defer wf_win.deinit();
    expect_eq(wf_win.matches("packages\\cli", "cli"), true);
});
