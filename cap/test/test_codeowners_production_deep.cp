// Source: cp/cap/test/test_codeowners_production_deep.cp (Production deep tests for cap:codeowners)

import { test, expect_eq } from "cap:test";
import {
    CodeownersRule,
    OwnerIter,
    CodeownersIter,
    resolveOwners,
    isOwnedBy,
} from "cap:codeowners";

test("codeowners: CodeownersIter skips comments, GitLab sections, tracks line_no and escaped spaces") {
    const content =
        "\xEF\xBB\xBF" ++
        "# global comment\n" ++
        "[Backend]\n" ++
        "^[Frontend]\n" ++
        "my\\ dir/*.cp @alice @org/compiler # inline comment\n" ++
        "/docs/ @docs-team\n";

    let it = CodeownersIter.init(content);

    const r1 = it.next().?;
    expect_eq(r1.line_no, 4);
    expect_eq(std.mem.eql(u8, r1.pattern, "my\\ dir/*.cp"), true);
    expect_eq(std.mem.eql(u8, r1.owners_raw, "@alice @org/compiler"), true);

    const r2 = it.next().?;
    expect_eq(r2.line_no, 5);
    expect_eq(std.mem.eql(u8, r2.pattern, "/docs/"), true);
    expect_eq(std.mem.eql(u8, r2.owners_raw, "@docs-team"), true);

    expect_eq(it.next() == null, true);
}

test("codeowners: OwnerIter splits users, teams and emails and stops at #") {
    let it = OwnerIter.init("  @alice   @org/runtime-team  dev@example.com # trailing note @ignored");

    expect_eq(std.mem.eql(u8, it.next().?, "@alice"), true);
    expect_eq(std.mem.eql(u8, it.next().?, "@org/runtime-team"), true);
    expect_eq(std.mem.eql(u8, it.next().?, "dev@example.com"), true);
    expect_eq(it.next() == null, true);
}

test("codeowners: GitHub last-match-wins resolution across *, *.cp, /docs/, docs/* and apps/**") {
    const co =
        "* @org/default\n" ++
        "*.cp @cp-maintainers\n" ++
        "/docs/ @docs-team\n" ++
        "docs/* @docs-root-only\n" ++
        "apps/** @app-team\n" ++
        "my\\ dir/file.txt @space-owner\n";

    // 默认兜底 *
    expect_eq(std.mem.eql(u8, resolveOwners(co, "README.md").?, "@org/default"), true);

    // *.cp 覆盖 *
    expect_eq(std.mem.eql(u8, resolveOwners(co, "lib/http/server.cp").?, "@cp-maintainers"), true);

    // docs/* 仅匹配直接子文件，覆盖 /docs/；更深子文件仍归 /docs/
    expect_eq(std.mem.eql(u8, resolveOwners(co, "docs/index.md").?, "@docs-root-only"), true);
    expect_eq(std.mem.eql(u8, resolveOwners(co, "docs/api/ref.md").?, "@docs-team"), true);

    // /docs/ 锚定根，不匹配 pkg/docs/a.md
    expect_eq(std.mem.eql(u8, resolveOwners(co, "pkg/docs/a.md").?, "@org/default"), true);

    // apps/** 匹配任意深度
    expect_eq(std.mem.eql(u8, resolveOwners(co, "apps/web/src/main.cp").?, "@app-team"), true);

    // 转义空格路径匹配
    expect_eq(std.mem.eql(u8, resolveOwners(co, "my dir/file.txt").?, "@space-owner"), true);
}

test("codeowners: unowned clearing rule resets previous ownership to null") {
    const co =
        "* @org/core\n" ++
        "generated/**\n";

    expect_eq(std.mem.eql(u8, resolveOwners(co, "src/main.cp").?, "@org/core"), true);
    expect_eq(resolveOwners(co, "generated/schema.cp") == null, true);
    expect_eq(isOwnedBy(co, "generated/schema.cp", "@org/core"), false);
}

test("codeowners: isOwnedBy case-insensitive matching and empty path guards") {
    const co =
        "lib/*.cp @Alice @Org/StdLib security@example.com\n";

    expect_eq(isOwnedBy(co, "lib/hash.cp", "@alice"), true);
    expect_eq(isOwnedBy(co, "lib/hash.cp", "@ORG/STDLIB"), true);
    expect_eq(isOwnedBy(co, "lib/hash.cp", "SECURITY@EXAMPLE.COM"), true);
    expect_eq(isOwnedBy(co, "lib/hash.cp", "@bob"), false);
    expect_eq(isOwnedBy(co, "lib/hash.cp", ""), false);
    expect_eq(resolveOwners(co, "") == null, true);
}
