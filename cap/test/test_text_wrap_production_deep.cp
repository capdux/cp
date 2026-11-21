import {
    wrapText,
    indentLines,
    commonIndent,
    dedentLines,
    truncateEllipsis,
} from "cap:text_wrap";
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

test("wrapText greedy word wrap and paragraph breaks", () => {
    let buf: [128]u8 = [0; 128];
    const wrapped = wrapText("the quick brown fox jumps over the lazy dog", 15, buf[0..]).?;
    expect(strEq(wrapped, "the quick brown\nfox jumps over\nthe lazy dog"));

    const para = wrapText("hello world\n\nfoo bar", 20, buf[0..]).?;
    expect(strEq(para, "hello world\n\nfoo bar"));
    expect(wrapText("abc", 0, buf[0..]) == null);
});

test("commonIndent, dedentLines, indentLines, and UTF-8 safe truncateEllipsis", () => {
    const block = "    line1\n      line2\n\n    line3";
    expect(commonIndent(block) == 4);

    let buf: [128]u8 = [0; 128];
    expect(strEq(dedentLines(block, buf[0..]).?, "line1\n  line2\n\nline3"));
    expect(strEq(indentLines("a\n\nb", "  ", buf[0..]).?, "  a\n\n  b"));

    expect(strEq(truncateEllipsis("hello world", 8, "...", buf[0..]).?, "hello..."));
    expect(strEq(truncateEllipsis("short", 10, "...", buf[0..]).?, "short"));
    // UTF-8 safe: "你好世界" is 12 bytes (3 bytes/char); max_len=8, ellipsis="..." (3 bytes) -> cut<=5 -> backs up to 3 ("你...")
    expect(strEq(truncateEllipsis("你好世界", 8, "...", buf[0..]).?, "你..."));
});
