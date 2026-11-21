import std from "std";
import { test, expect_eq } from "cap:test";
import {
    isValidXmlChar,
    escapeXmlText,
    escapeXmlAttr,
    wrapXmlCdata,
    unescapeXml,
} from "cap:xml_escape";

test("1. isValidXmlChar enforces XML 1.0 Fifth Edition section 2.2", () => {
    expect_eq(isValidXmlChar(0x09), true);
    expect_eq(isValidXmlChar(0x0A), true);
    expect_eq(isValidXmlChar(0x0D), true);
    expect_eq(isValidXmlChar(0x20), true);
    expect_eq(isValidXmlChar(0x4E2D), true); // '中'
    expect_eq(isValidXmlChar(0x1F680), true); // '🚀'

    expect_eq(isValidXmlChar(0x00), false);
    expect_eq(isValidXmlChar(0x08), false);
    expect_eq(isValidXmlChar(0x0B), false);
    expect_eq(isValidXmlChar(0x0C), false);
    expect_eq(isValidXmlChar(0x1F), false);
    expect_eq(isValidXmlChar(0xD800), false);
    expect_eq(isValidXmlChar(0xDFFF), false);
    expect_eq(isValidXmlChar(0xFFFE), false);
    expect_eq(isValidXmlChar(0xFFFF), false);
    expect_eq(isValidXmlChar(0x11_0000), false);
});

test("2. escapeXmlText and escapeXmlAttr escape special chars and round-trip with unescapeXml", () => {
    let esc_buf: [256]u8 = std.mem.zeroes([256]u8);
    let out_buf: [256]u8 = std.mem.zeroes([256]u8);

    const t = escapeXmlText("a < b & c ]]> d", &esc_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, t, "a &lt; b &amp; c ]]&gt; d"), true);
    expect_eq(std.mem.eql(u8, unescapeXml(t, &out_buf) ?? unreachable, "a < b & c ]]> d"), true);

    const attr_raw = "say \"hi\" & 'bye'\r\n\t<>";
    const a = escapeXmlAttr(attr_raw, &esc_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, a, "say &quot;hi&quot; &amp; &apos;bye&apos;&#xD;&#xA;&#x9;&lt;&gt;"), true);
    expect_eq(std.mem.eql(u8, unescapeXml(a, &out_buf) ?? unreachable, attr_raw), true);
});

test("3. wrapXmlCdata safely splits embedded ]]> sequences", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const c1 = wrapXmlCdata("plain text", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, c1, "<![CDATA[plain text]]>"), true);

    const c2 = wrapXmlCdata("a]]>b]]>c", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, c2, "<![CDATA[a]]]]><![CDATA[>b]]]]><![CDATA[>c]]>"), true);
});

test("4. unescapeXml decodes decimal/hex Unicode refs and rejects invalid XML chars & malformed entities", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // 十进制与十六进制 Unicode（中 = U+4E2D = 20013, 🚀 = U+1F680）
    const u = unescapeXml("&#20013;&#x1F680;&#65;", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, u, "中🚀A"), true);

    // 拒绝非法 XML 1.0 码点引用（NUL、代理项、0xFFFF）与未知/未闭合实体
    expect_eq(unescapeXml("&#0;", &buf) == null, true);
    expect_eq(unescapeXml("&#xD800;", &buf) == null, true);
    expect_eq(unescapeXml("&#xFFFF;", &buf) == null, true);
    expect_eq(unescapeXml("&unknown;", &buf) == null, true);
    expect_eq(unescapeXml("&amp", &buf) == null, true);
    expect_eq(unescapeXml("&;", &buf) == null, true);
});

test("5. buffer-too-small safety guards", () => {
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(escapeXmlText("&", &tiny) == null, true);
    expect_eq(escapeXmlAttr("\"", &tiny) == null, true);
    expect_eq(wrapXmlCdata("x", &tiny) == null, true);
    expect_eq(unescapeXml("hello", &tiny) == null, true);
});
