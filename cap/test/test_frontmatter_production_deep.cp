// Source: cp/cap/test/test_frontmatter_production_deep.cp (Production deep tests for cap:frontmatter)

import { test, expect_eq } from "cap:test";
import {
    FrontmatterKind,
    FrontmatterSplit,
    splitFrontmatter,
    getFrontmatterField,
} from "cap:frontmatter";

test("frontmatter: YAML --- ... --- extraction with BOM, CRLF and field lookup") {
    const doc =
        "\xEF\xBB\xBF" ++
        "---\r\n" ++
        "# post metadata\r\n" ++
        "title: \"CP Standard Library\"\r\n" ++
        "author: 'Alice'\r\n" ++
        "color: #ff00ff  # brand color\r\n" ++
        "draft: false\r\n" ++
        "---\r\n" ++
        "# Welcome\r\nBody text.\r\n";

    const sp = splitFrontmatter(doc);
    expect_eq(sp.kind == .yaml, true);
    expect_eq(std.mem.eql(u8, sp.kind.asStr(), "yaml"), true);
    expect_eq(std.mem.eql(u8, sp.body, "# Welcome\r\nBody text.\r\n"), true);

    expect_eq(std.mem.eql(u8, getFrontmatterField(sp.raw_meta, sp.kind, "title").?, "CP Standard Library"), true);
    expect_eq(std.mem.eql(u8, getFrontmatterField(sp.raw_meta, sp.kind, "author").?, "Alice"), true);
    expect_eq(std.mem.eql(u8, getFrontmatterField(sp.raw_meta, sp.kind, "color").?, "#ff00ff"), true);
    expect_eq(std.mem.eql(u8, getFrontmatterField(sp.raw_meta, sp.kind, "draft").?, "false"), true);
    expect_eq(getFrontmatterField(sp.raw_meta, sp.kind, "missing") == null, true);
}

test("frontmatter: YAML closed by ... ellipsis delimiter") {
    const doc =
        "---\n" ++
        "slug: intro-guide\n" ++
        "...\n" ++
        "Paragraph one.\n";

    const sp = splitFrontmatter(doc);
    expect_eq(sp.kind == .yaml, true);
    expect_eq(std.mem.eql(u8, getFrontmatterField(sp.raw_meta, sp.kind, "slug").?, "intro-guide"), true);
    expect_eq(std.mem.eql(u8, sp.body, "Paragraph one.\n"), true);
}

test("frontmatter: TOML +++ ... +++ extraction and field lookup") {
    const doc =
        "+++\n" ++
        "# Hugo config\n" ++
        "title = \"Fast Native Agents\" # inline comment\n" ++
        "weight = 10 # order\n" ++
        "theme = 'dark'\n" ++
        "+++\n" ++
        "Content here.";

    const sp = splitFrontmatter(doc);
    expect_eq(sp.kind == .toml, true);
    expect_eq(std.mem.eql(u8, sp.kind.asStr(), "toml"), true);
    expect_eq(std.mem.eql(u8, sp.body, "Content here."), true);

    expect_eq(std.mem.eql(u8, getFrontmatterField(sp.raw_meta, sp.kind, "title").?, "Fast Native Agents"), true);
    expect_eq(std.mem.eql(u8, getFrontmatterField(sp.raw_meta, sp.kind, "weight").?, "10"), true);
    expect_eq(std.mem.eql(u8, getFrontmatterField(sp.raw_meta, sp.kind, "theme").?, "dark"), true);
}

test("frontmatter: empty frontmatter block and no trailing body") {
    const doc = "---\n---\n";
    const sp = splitFrontmatter(doc);
    expect_eq(sp.kind == .yaml, true);
    expect_eq(sp.raw_meta.len, 0);
    expect_eq(sp.body.len, 0);

    const doc_eof = "+++\na = 1\n+++";
    const sp2 = splitFrontmatter(doc_eof);
    expect_eq(sp2.kind == .toml, true);
    expect_eq(std.mem.eql(u8, getFrontmatterField(sp2.raw_meta, sp2.kind, "a").?, "1"), true);
    expect_eq(sp2.body.len, 0);
}

test("frontmatter: unclosed fence, mismatched fence and plain markdown fallback to none") {
    const plain = "# Plain Markdown\nNo frontmatter here.\n";
    const sp1 = splitFrontmatter(plain);
    expect_eq(sp1.kind == .none, true);
    expect_eq(sp1.raw_meta.len, 0);
    expect_eq(std.mem.eql(u8, sp1.body, plain), true);

    // 未闭合 YAML 头块回退为 .none 且保留完整 doc
    const unclosed = "---\ntitle: Oops\nNo closing line";
    const sp2 = splitFrontmatter(unclosed);
    expect_eq(sp2.kind == .none, true);
    expect_eq(std.mem.eql(u8, sp2.body, unclosed), true);

    // 混合定界符 --- 与 +++ 不匹配回退为 .none
    const mismatch = "---\ntitle: Mix\n+++\nBody";
    const sp3 = splitFrontmatter(mismatch);
    expect_eq(sp3.kind == .none, true);
    expect_eq(std.mem.eql(u8, sp3.body, mismatch), true);

    // .none 查字段直接返回 null
    expect_eq(getFrontmatterField("title: a", .none, "title") == null, true);
}
