// Source: cp/cap/test/test_gitignore_match_production_deep.cp (Production deep tests for cap:gitignore_match)

import { test, expect_eq } from "cap:test";
import {
    GitignoreRule,
    parseGitignoreLine,
    ruleMatchesPath,
    isIgnored,
} from "cap:git";

test("gitignore_match: parseGitignoreLine comments, trailing spaces, negation, escapes and anchoring") {
    expect_eq(parseGitignoreLine("") == null, true);
    expect_eq(parseGitignoreLine("   ") == null, true);
    expect_eq(parseGitignoreLine("# comment line") == null, true);
    expect_eq(parseGitignoreLine("!") == null, true);

    // 剥离尾部未转义空格
    const r_trim = parseGitignoreLine("*.log   ").?;
    expect_eq(std.mem.eql(u8, r_trim.pattern, "*.log"), true);
    expect_eq(r_trim.negated, false);
    expect_eq(r_trim.dir_only, false);
    expect_eq(r_trim.anchored, false);

    // 保留尾部转义空格 '\ '
    const r_esc_sp = parseGitignoreLine("foo\\ ").?;
    expect_eq(std.mem.eql(u8, r_esc_sp.pattern, "foo\\ "), true);

    // 否定 + 仅目录 + 锚定
    const r_dir = parseGitignoreLine("build/").?;
    expect_eq(std.mem.eql(u8, r_dir.pattern, "build"), true);
    expect_eq(r_dir.dir_only, true);
    expect_eq(r_dir.anchored, false);

    const r_anc = parseGitignoreLine("/dist/out").?;
    expect_eq(std.mem.eql(u8, r_anc.pattern, "dist/out"), true);
    expect_eq(r_anc.anchored, true);

    const r_neg = parseGitignoreLine("!important.log").?;
    expect_eq(std.mem.eql(u8, r_neg.pattern, "important.log"), true);
    expect_eq(r_neg.negated, true);

    // 转义 \# 与 \! 不视为注释或否定
    const r_hash = parseGitignoreLine("\\#notes.md").?;
    expect_eq(r_hash.negated, false);
    expect_eq(ruleMatchesPath(r_hash, "#notes.md", false), true);

    const r_bang = parseGitignoreLine("\\!README").?;
    expect_eq(r_bang.negated, false);
    expect_eq(ruleMatchesPath(r_bang, "!README", false), true);
}

test("gitignore_match: unanchored basename matching vs anchored root matching") {
    const r_unanc = parseGitignoreLine("*.o").?;
    expect_eq(ruleMatchesPath(r_unanc, "main.o", false), true);
    expect_eq(ruleMatchesPath(r_unanc, "src/core/main.o", false), true);
    expect_eq(ruleMatchesPath(r_unanc, "src/core/main.c", false), false);

    // 前导 / 仅匹配根路径，不匹配深层子目录
    const r_root = parseGitignoreLine("/TODO").?;
    expect_eq(ruleMatchesPath(r_root, "TODO", false), true);
    expect_eq(ruleMatchesPath(r_root, "sub/TODO", false), false);

    // 中间含 / 也视为锚定
    const r_mid = parseGitignoreLine("doc/frotz").?;
    expect_eq(ruleMatchesPath(r_mid, "doc/frotz", false), true);
    expect_eq(ruleMatchesPath(r_mid, "a/doc/frotz", false), false);
}

test("gitignore_match: globstar **/, a/**/b, abc/** and single-segment ?/[a-z]") {
    const r_lead = parseGitignoreLine("**/foo").?;
    expect_eq(ruleMatchesPath(r_lead, "foo", false), true);
    expect_eq(ruleMatchesPath(r_lead, "a/foo", false), true);
    expect_eq(ruleMatchesPath(r_lead, "a/b/c/foo", false), true);

    const r_mid = parseGitignoreLine("a/**/b").?;
    expect_eq(ruleMatchesPath(r_mid, "a/b", false), true);
    expect_eq(ruleMatchesPath(r_mid, "a/x/b", false), true);
    expect_eq(ruleMatchesPath(r_mid, "a/x/y/b", false), true);
    expect_eq(ruleMatchesPath(r_mid, "a/x/c", false), false);

    const r_tail = parseGitignoreLine("logs/**").?;
    expect_eq(ruleMatchesPath(r_tail, "logs/a.txt", false), true);
    expect_eq(ruleMatchesPath(r_tail, "logs/2026/b.txt", false), true);
    expect_eq(ruleMatchesPath(r_tail, "logs", true), false);

    const r_cls = parseGitignoreLine("temp?[0-9].tmp").?;
    expect_eq(ruleMatchesPath(r_cls, "dir/tempA7.tmp", false), true);
    expect_eq(ruleMatchesPath(r_cls, "dir/tempAA.tmp", false), false);
}

test("gitignore_match: dir_only trailing slash distinction") {
    const r_dir = parseGitignoreLine("cache/").?;
    expect_eq(ruleMatchesPath(r_dir, "cache", true), true);
    expect_eq(ruleMatchesPath(r_dir, "src/cache", true), true);
    expect_eq(ruleMatchesPath(r_dir, "cache", false), false);
    expect_eq(ruleMatchesPath(r_dir, "src/cache", false), false);
}

test("gitignore_match: full .gitignore evaluation with last-rule-wins and ! re-inclusion") {
    const gi =
        "\xEF\xBB\xBF" ++
        "# Build & log artifacts\r\n" ++
        "*.log\r\n" ++
        "!important.log\r\n" ++
        "dist/\r\n" ++
        "src/**/*.gen.cp\r\n" ++
        "!src/keep/stub.gen.cp\r\n" ++
        "/root_only.txt\r\n";

    expect_eq(isIgnored(gi, "debug.log", false), true);
    expect_eq(isIgnored(gi, "logs/error.log", false), true);
    expect_eq(isIgnored(gi, "important.log", false), false);
    expect_eq(isIgnored(gi, "logs/important.log", false), false);

    expect_eq(isIgnored(gi, "dist", true), true);
    expect_eq(isIgnored(gi, "dist", false), false);

    expect_eq(isIgnored(gi, "src/a/b/foo.gen.cp", false), true);
    expect_eq(isIgnored(gi, "src/keep/stub.gen.cp", false), false);

    expect_eq(isIgnored(gi, "root_only.txt", false), true);
    expect_eq(isIgnored(gi, "sub/root_only.txt", false), false);
}
