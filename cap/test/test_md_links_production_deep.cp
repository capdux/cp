// Source: cp/cap/test/test_md_links_production_deep.cp (Production deep tests for cap:markdown links)

import { test, expect_eq } from "cap:test";
import {
    LinkKind,
    ParsedLink,
    parseAutolink,
    parseInlineLinkOrImage,
    percentEncodeUrl,
} from "cap:markdown";

test("Inline link, image, nested image-in-link, escaped brackets and code span skip") {
    const s1 = "Check [CP Lang](https://cp-lang.org)!";
    const l1 = parseInlineLinkOrImage(s1, 6).?;
    expect_eq(l1.kind == .inline_link, true);
    expect_eq(std.mem.eql(u8, l1.text, "CP Lang"), true);
    expect_eq(std.mem.eql(u8, l1.dest, "https://cp-lang.org"), true);
    expect_eq(l1.title.len, @as(usize, 0));
    expect_eq(l1.start_pos, @as(usize, 6));
    expect_eq(l1.end_pos, @as(usize, s1.len - 1));

    const img = "![logo](/assets/logo.png \"CP Logo\")";
    const l2 = parseInlineLinkOrImage(img, 0).?;
    expect_eq(l2.kind == .image, true);
    expect_eq(l2.kind.isImage(), true);
    expect_eq(std.mem.eql(u8, l2.text, "logo"), true);
    expect_eq(std.mem.eql(u8, l2.dest, "/assets/logo.png"), true);
    expect_eq(std.mem.eql(u8, l2.title, "CP Logo"), true);

    // 嵌套图片链接与转义括号/反引号内 ']' 跳过
    const nested = "[![badge](icon.png)](https://cp-lang.org)";
    const l3 = parseInlineLinkOrImage(nested, 0).?;
    expect_eq(l3.kind == .inline_link, true);
    expect_eq(std.mem.eql(u8, l3.text, "![badge](icon.png)"), true);
    expect_eq(std.mem.eql(u8, l3.dest, "https://cp-lang.org"), true);

    const escaped = "[a\\]b `]` c](https://ex.com)";
    const l4 = parseInlineLinkOrImage(escaped, 0).?;
    expect_eq(std.mem.eql(u8, l4.text, "a\\]b `]` c"), true);
    expect_eq(std.mem.eql(u8, l4.dest, "https://ex.com"), true);
}

test("Balanced parentheses URL, angle-bracket destination and 3 title quote styles") {
    // 平衡括号 URL
    const wiki = "[Wiki](https://en.wikipedia.org/wiki/Foo_(bar))";
    const lw = parseInlineLinkOrImage(wiki, 0).?;
    expect_eq(std.mem.eql(u8, lw.dest, "https://en.wikipedia.org/wiki/Foo_(bar)"), true);

    // 尖括号含空格目标 + 双引号标题
    const angle = "[Doc](<https://example.com/my file.html> \"My Title\")";
    const la = parseInlineLinkOrImage(angle, 0).?;
    expect_eq(std.mem.eql(u8, la.dest, "https://example.com/my file.html"), true);
    expect_eq(std.mem.eql(u8, la.title, "My Title"), true);

    // 单引号标题
    const single_q = "[Single](https://ex.com 'Single Title')";
    const ls = parseInlineLinkOrImage(single_q, 0).?;
    expect_eq(std.mem.eql(u8, ls.title, "Single Title"), true);

    // 圆括号标题
    const paren_q = "[Paren](https://ex.com (Paren Title))";
    const lp = parseInlineLinkOrImage(paren_q, 0).?;
    expect_eq(std.mem.eql(u8, lp.title, "Paren Title"), true);
}

test("Full, collapsed and shortcut reference links") {
    const full = "[Click Here][docs-ref]";
    const rf = parseInlineLinkOrImage(full, 0).?;
    expect_eq(rf.kind == .full_ref, true);
    expect_eq(rf.kind.isReference(), true);
    expect_eq(std.mem.eql(u8, rf.text, "Click Here"), true);
    expect_eq(std.mem.eql(u8, rf.dest, "docs-ref"), true);
    expect_eq(rf.end_pos, full.len);

    const collapsed = "[Guide][]";
    const rc = parseInlineLinkOrImage(collapsed, 0).?;
    expect_eq(rc.kind == .collapsed_ref, true);
    expect_eq(std.mem.eql(u8, rc.text, "Guide"), true);
    expect_eq(std.mem.eql(u8, rc.dest, "Guide"), true);
    expect_eq(rc.end_pos, collapsed.len);

    const shortcut = "[Shortcut] rest";
    const rs = parseInlineLinkOrImage(shortcut, 0).?;
    expect_eq(rs.kind == .shortcut_ref, true);
    expect_eq(std.mem.eql(u8, rs.text, "Shortcut"), true);
    expect_eq(std.mem.eql(u8, rs.dest, "Shortcut"), true);
    expect_eq(rs.end_pos, @as(usize, 10));
}

test("CommonMark §6.5 URI and Email autolinks and invalid autolink guards") {
    const u1 = parseAutolink("<https://cp-lang.org/docs?a=1&b=2>", 0).?;
    expect_eq(u1.kind == .autolink_uri, true);
    expect_eq(u1.kind.isAutolink(), true);
    expect_eq(std.mem.eql(u8, u1.dest, "https://cp-lang.org/docs?a=1&b=2"), true);
    expect_eq(u1.end_pos, @as(usize, 34));

    const e1 = parseAutolink("mail: <dev+cp@example.co.uk>!", 6).?;
    expect_eq(e1.kind == .autolink_email, true);
    expect_eq(std.mem.eql(u8, e1.dest, "dev+cp@example.co.uk"), true);
    expect_eq(e1.start_pos, @as(usize, 6));
    expect_eq(e1.end_pos, @as(usize, 28));

    // 非法自动链接拦截：单字母 scheme、含空格、空域标签、未闭合
    expect_eq(parseAutolink("<a:b>", 0) == null, true);
    expect_eq(parseAutolink("<https://a b.com>", 0) == null, true);
    expect_eq(parseAutolink("<user@a..b>", 0) == null, true);
    expect_eq(parseAutolink("<user@-bad.com>", 0) == null, true);
    expect_eq(parseAutolink("<https://unclosed", 0) == null, true);
}

test("percentEncodeUrl preserves valid %XX and encodes spaces, non-ASCII and controls") {
    let buf: [128]u8 = [0; 128];

    // 空格编码为 %20，合法 %20 与 %E4%B8%AD 原样保留，孤立 % 编码为 %25
    const enc1 = percentEncodeUrl("https://ex.com/a b%20c%25d%ZZ", buf[0..128]).?;
    expect_eq(std.mem.eql(u8, enc1, "https://ex.com/a%20b%20c%25d%25ZZ"), true);

    // 中文 UTF-8 "中" = E4 B8 AD
    const enc_zh = percentEncodeUrl("/doc/中", buf[0..128]).?;
    expect_eq(std.mem.eql(u8, enc_zh, "/doc/%E4%B8%AD"), true);

    // 控制字符 0x0A 编码为 %0A
    const enc_ctrl = percentEncodeUrl("a\nb", buf[0..128]).?;
    expect_eq(std.mem.eql(u8, enc_ctrl, "a%0Ab"), true);

    // 缓冲不足返回 null
    let tiny: [4]u8 = [0; 4];
    expect_eq(percentEncodeUrl("a b c", tiny[0..4]) == null, true);
}

test("Malformed link and boundary input guards") {
    expect_eq(parseInlineLinkOrImage("", 0) == null, true);
    expect_eq(parseInlineLinkOrImage("[unclosed](https://ex.com)", 0) == null, true);
    expect_eq(parseInlineLinkOrImage("![unclosed](img.png", 0) == null, true);
    expect_eq(parseInlineLinkOrImage("![img](<bad\nurl>)", 0) == null, true);
    expect_eq(parseInlineLinkOrImage("![img](url \"unclosed)", 0) == null, true);
    expect_eq(parseInlineLinkOrImage("   ", 0) == null, true);
    expect_eq(parseInlineLinkOrImage("[   ]", 0) == null, true);
    expect_eq(parseInlineLinkOrImage("abc", 10) == null, true);
}
