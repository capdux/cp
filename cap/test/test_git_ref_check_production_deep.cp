// Source: cp/cap/test/test_git_ref_check_production_deep.cp (Production deep tests for cap:git_ref_check)

import { test, expect_eq } from "cap:test";
import {
    GitRefKind,
    ParsedGitRef,
    isValidGitRefName,
    isValidBranchName,
    parseFullGitRef,
    isValidGitOidHex,
    isValidShortOidHex,
} from "cap:git_ref_check";

test("git_ref_check: isValidGitRefName enforces all 10 git-check-ref-format rules") {
    // 合法引用名
    expect_eq(isValidGitRefName("main"), true);
    expect_eq(isValidGitRefName("refs/heads/feat/v1.2.3"), true);
    expect_eq(isValidGitRefName("refs/tags/v2.0.0-rc.1"), true);
    expect_eq(isValidGitRefName("user@domain/topic"), true);

    // 全部 10 条禁用规则
    expect_eq(isValidGitRefName(""), false);
    expect_eq(isValidGitRefName("@"), false);
    expect_eq(isValidGitRefName("/leading"), false);
    expect_eq(isValidGitRefName("trailing/"), false);
    expect_eq(isValidGitRefName("double//slash"), false);
    expect_eq(isValidGitRefName("trailing."), false);
    expect_eq(isValidGitRefName("a..b"), false);
    expect_eq(isValidGitRefName("a@{0}"), false);
    expect_eq(isValidGitRefName("refs/.hidden/branch"), false);
    expect_eq(isValidGitRefName("refs/heads/main.lock"), false);
    expect_eq(isValidGitRefName("refs/foo.lock/bar"), false);
    expect_eq(isValidGitRefName("has space"), false);
    expect_eq(isValidGitRefName("bad~1"), false);
    expect_eq(isValidGitRefName("bad^2"), false);
    expect_eq(isValidGitRefName("bad:ref"), false);
    expect_eq(isValidGitRefName("bad?ref"), false);
    expect_eq(isValidGitRefName("bad*ref"), false);
    expect_eq(isValidGitRefName("bad[ref"), false);
    expect_eq(isValidGitRefName("bad\\ref"), false);
    expect_eq(isValidGitRefName("ctrl\x1Fchar"), false);
    expect_eq(isValidGitRefName("del\x7Fchar"), false);
}

test("git_ref_check: isValidBranchName rejects leading dash and HEAD") {
    expect_eq(isValidBranchName("main"), true);
    expect_eq(isValidBranchName("feat/new-parser"), true);
    expect_eq(isValidBranchName("fix-123"), true);

    expect_eq(isValidBranchName("-leading-dash"), false);
    expect_eq(isValidBranchName("HEAD"), false);
    expect_eq(isValidBranchName("bad..branch"), false);
}

test("git_ref_check: parseFullGitRef classifies heads, tags, remotes, symbolic and other") {
    const b = parseFullGitRef("refs/heads/feat/router").?;
    expect_eq(b.kind == .branch, true);
    expect_eq(std.mem.eql(u8, b.short_name, "feat/router"), true);
    expect_eq(b.remote_name == null, true);

    const t = parseFullGitRef("refs/tags/v1.0.0").?;
    expect_eq(t.kind == .tag, true);
    expect_eq(std.mem.eql(u8, t.short_name, "v1.0.0"), true);
    expect_eq(t.remote_name == null, true);

    const r = parseFullGitRef("refs/remotes/origin/release/v2").?;
    expect_eq(r.kind == .remote, true);
    expect_eq(std.mem.eql(u8, r.remote_name.?, "origin"), true);
    expect_eq(std.mem.eql(u8, r.short_name, "release/v2"), true);
    expect_eq(std.mem.eql(u8, r.kind.asStr(), "remote"), true);

    const h = parseFullGitRef("HEAD").?;
    expect_eq(h.kind == .symbolic, true);
    expect_eq(std.mem.eql(u8, h.short_name, "HEAD"), true);

    const fh = parseFullGitRef("FETCH_HEAD").?;
    expect_eq(fh.kind == .symbolic, true);

    const notes = parseFullGitRef("refs/notes/commits").?;
    expect_eq(notes.kind == .other, true);

    // 畸形 remote（缺失分支段）或非法 ref 返回 null
    expect_eq(parseFullGitRef("refs/remotes/origin") == null, true);
    expect_eq(parseFullGitRef("refs/heads/bad..name") == null, true);
}

test("git_ref_check: SHA-1 40-char and SHA-256 64-char full OID validation") {
    const sha1 = "0123456789abcdefABCDEF0123456789abcdef01";
    const sha256 = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    expect_eq(isValidGitOidHex(sha1), true);
    expect_eq(isValidGitOidHex(sha256), true);

    // 长度不对或含非十六进制字符
    expect_eq(isValidGitOidHex("01234567"), false);
    expect_eq(isValidGitOidHex("g123456789abcdefABCDEF0123456789abcdef01"), false);
    expect_eq(isValidGitOidHex(""), false);
}

test("git_ref_check: isValidShortOidHex 4..=64 boundary validation") {
    expect_eq(isValidShortOidHex("a1b2"), true);
    expect_eq(isValidShortOidHex("deadbeef"), true);
    expect_eq(isValidShortOidHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"), true);

    expect_eq(isValidShortOidHex("abc"), false);
    expect_eq(isValidShortOidHex("abcdz"), false);
    expect_eq(isValidShortOidHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0"), false);
}
