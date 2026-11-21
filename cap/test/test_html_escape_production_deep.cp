import {
    needsHtmlEscape,
    escapeHtml,
    unescapeHtml,
} from "cap:html_escape";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("escapeHtml and needsHtmlEscape", () => {
    expect(!needsHtmlEscape("plain text 123"));
    expect(needsHtmlEscape("<a href=\"x\">it's & ok</a>"));

    let buf: [128]u8 = [0; 128];
    const esc = escapeHtml("<a href=\"x\">it's & ok</a>", buf[0..]).?;
    expect(strEq(esc, "&lt;a href=&quot;x&quot;&gt;it&#x27;s &amp; ok&lt;/a&gt;"));

    let dec_buf: [128]u8 = [0; 128];
    expect(strEq(unescapeHtml(esc, dec_buf[0..]).?, "<a href=\"x\">it's & ok</a>"));
});

test("unescapeHtml named, decimal, hex, surrogate replacement, and verbatim unknown", () => {
    let buf: [128]u8 = [0; 128];
    expect(strEq(unescapeHtml("&#65;&#x42;&apos;&nbsp;", buf[0..]).?, "AB'\xc2\xa0"));
    // 4-byte UTF-8 emoji U+1F600
    expect(strEq(unescapeHtml("&#x1F600;", buf[0..]).?, "\xf0\x9f\x98\x80"));
    // Invalid surrogate/NUL replaced with U+FFFD
    expect(strEq(unescapeHtml("&#0;&#xD800;", buf[0..]).?, "\xef\xbf\xbd\xef\xbf\xbd"));
    // Unknown or unterminated entities preserved verbatim
    expect(strEq(unescapeHtml("a &unknown; & b", buf[0..]).?, "a &unknown; & b"));
});
