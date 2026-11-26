import { test, expect_eq } from "cap:test";
import {
    fill,
    fillWithIndent,
    indent,
    dedent,
    unfill,
    lineDisplayWidth,
} from "../../packages/@cap/textwrap-crate/src/index.cp";

fn eqStr(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/textwrap-crate — fill word wrapping & paragraph blank line preservation", () => {
        let buf: [256]u8 = undefined;

        const n1 = fill("textwrap: a small library for wrapping text.", 18, buf[0..]);
        expect_eq(eqStr(buf[0..n1], "textwrap: a small\nlibrary for\nwrapping text."), true);

        const n2 = fill("hello world\n\nsecond paragraph here", 10, buf[0..]);
        expect_eq(eqStr(buf[0..n2], "hello\nworld\n\nsecond\nparagraph\nhere"), true);

        const n3 = fill("", 20, buf[0..]);
        expect_eq(n3, 0 as usize);
    });

    test("2. @cap/textwrap-crate — fillWithIndent initial & subsequent indents", () => {
        let buf: [256]u8 = undefined;

        const n1 = fillWithIndent("alpha beta gamma delta", 16, "- ", "  ", buf[0..]);
        expect_eq(eqStr(buf[0..n1], "- alpha beta\n  gamma delta"), true);

        const n2 = fillWithIndent("one two three four", 14, ">>> ", "    ", buf[0..]);
        expect_eq(eqStr(buf[0..n2], ">>> one two\n    three four"), true);
    });

    test("3. @cap/textwrap-crate — indent skips blank lines and preserves newlines", () => {
        let buf: [256]u8 = undefined;

        const n1 = indent("foo\n\nbar\nbaz\n", "> ", buf[0..]);
        expect_eq(eqStr(buf[0..n1], "> foo\n\n> bar\n> baz\n"), true);

        const n2 = indent("single line", "  ", buf[0..]);
        expect_eq(eqStr(buf[0..n2], "  single line"), true);

        const n3 = indent("a\n   \nb", "* ", buf[0..]);
        expect_eq(eqStr(buf[0..n3], "* a\n   \n* b"), true);
    });

    test("4. @cap/textwrap-crate — dedent strips common leading whitespace", () => {
        let buf: [256]u8 = undefined;

        const src1 = "    hello\n      world\n\n    end\n";
        const n1 = dedent(src1, buf[0..]);
        expect_eq(eqStr(buf[0..n1], "hello\n  world\n\nend\n"), true);

        const src2 = "\t\tline1\n\t\t  line2\n   \n\t\tline3";
        const n2 = dedent(src2, buf[0..]);
        expect_eq(eqStr(buf[0..n2], "line1\n  line2\n\nline3"), true);

        const src3 = "no_indent\n  indented";
        const n3 = dedent(src3, buf[0..]);
        expect_eq(eqStr(buf[0..n3], "no_indent\n  indented"), true);
    });

    test("5. @cap/textwrap-crate — unfill merges soft wraps & preserves paragraphs", () => {
        let buf: [256]u8 = undefined;

        const src1 = "Hello\nworld!\n\nSecond\nparagraph\nhere.";
        const n1 = unfill(src1, buf[0..]);
        expect_eq(eqStr(buf[0..n1], "Hello world!\n\nSecond paragraph here."), true);

        const src2 = "  first line  \n  second line  ";
        const n2 = unfill(src2, buf[0..]);
        expect_eq(eqStr(buf[0..n2], "first line second line"), true);
    });

    test("6. @cap/textwrap-crate — lineDisplayWidth & CJK-aware wrapping via @cap/unicode-width", () => {
        expect_eq(lineDisplayWidth("hello"), 5 as usize);
        expect_eq(lineDisplayWidth("你好"), 4 as usize);
        expect_eq(lineDisplayWidth("Hi 世界"), 7 as usize);

        let buf: [256]u8 = undefined;
        // "你好" (4) + " " (1) + "世界" (4) = 9 > 6 -> wraps between words
        const n1 = fill("你好 世界 CP", 6, buf[0..]);
        expect_eq(eqStr(buf[0..n1], "你好\n世界\nCP"), true);
    });
}
