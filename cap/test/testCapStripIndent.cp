// cap/test/testCapStripIndent.cp
// Comprehensive unit tests for @cap/strip-indent (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    minIndent,
    stripIndent,
    stripIndentBy,
} from "../../packages/strip-indent/src/index.cp";

fn main() {
    test("1. @cap/strip-indent — minIndent ignores empty and whitespace-only lines", () => {
        expect_eq(minIndent(""), 0 as usize);
        expect_eq(minIndent("   \n\t\t\n  "), 0 as usize);
        expect_eq(minIndent("foo\n  bar"), 0 as usize);
        expect_eq(minIndent("\n    foo\n      bar\n"), 4 as usize);
        expect_eq(minIndent("  \n\t\tfoo\n\t\t\tbar\n"), 2 as usize);
        expect_eq(minIndent("    a\r\n  b\r\n      c"), 2 as usize);
    });

    test("2. @cap/strip-indent — stripIndent removes shortest non-empty space indentation", () => {
        let buf: [128]u8 = undefined;
        const src = "\n    foo\n      bar\n    baz\n";
        const n = stripIndent(src, buf[0..128]);
        expect_eq(buf[0..n], "\nfoo\n  bar\nbaz\n");
    });

    test("3. @cap/strip-indent — stripIndent handles tabs and mixed spaces/tabs", () => {
        let buf: [128]u8 = undefined;
        const src = "\t\tfoo\n\t\t\tbar\n\n\t\tbaz";
        const n = stripIndent(src, buf[0..128]);
        expect_eq(buf[0..n], "foo\n\tbar\n\nbaz");

        const mixed = "  \talpha\n  \t  beta";
        const n2 = stripIndent(mixed, buf[0..128]);
        expect_eq(buf[0..n2], "alpha\n  beta");
    });

    test("4. @cap/strip-indent — preserves CRLF (\\r\\n) line endings and blank lines", () => {
        let buf: [128]u8 = undefined;
        const src = "    line1\r\n\r\n      line2\r\n    line3\r\n";
        const n = stripIndent(src, buf[0..128]);
        expect_eq(buf[0..n], "line1\r\n\r\n  line2\r\nline3\r\n");
    });

    test("5. @cap/strip-indent — stripIndentBy strips exact count & leaves under-indented lines intact", () => {
        let buf: [128]u8 = undefined;
        const src = "    a\n  b\n      c";
        const n1 = stripIndentBy(src, 2, buf[0..128]);
        expect_eq(buf[0..n1], "  a\nb\n    c");

        // n = 4: line "  b" has only 2 spaces (< 4), kept intact per /^[ \t]{4}/gm semantics
        const n2 = stripIndentBy(src, 4, buf[0..128]);
        expect_eq(buf[0..n2], "a\n  b\n  c");

        // n = 0: verbatim copy
        const n3 = stripIndentBy("  hi", 0, buf[0..128]);
        expect_eq(buf[0..n3], "  hi");
    });

    test("6. @cap/strip-indent — empty input, zero-indent input, and bounded output buffer", () => {
        let buf: [16]u8 = undefined;
        expect_eq(stripIndent("", buf[0..16]), 0 as usize);

        const noIndent = "hello\n  world";
        const n1 = stripIndent(noIndent, buf[0..16]);
        expect_eq(buf[0..n1], "hello\n  world");

        // Output buffer smaller than result truncates cleanly without overflow
        let tiny: [5]u8 = undefined;
        const n2 = stripIndent("    abcdef", tiny[0..5]);
        expect_eq(n2, 5 as usize);
        expect_eq(tiny[0..n2], "abcde");
    });
}
