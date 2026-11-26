// cap/test/testCapRedent.cp
// Comprehensive unit tests for @cap/redent (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    redent,
    redentWithIndent,
} from "../../packages/@cap/redent/src/index.cp";

fn main() {
    test("1. @cap/redent — strips common indent when count is 0", () => {
        let buf: [128]u8 = undefined;
        const src = "\n    foo\n      bar\n";
        const n = redent(src, 0, buf[0..128]);
        expect_eq(buf[0..n], "\nfoo\n  bar\n");
    });

    test("2. @cap/redent — re-indents multiline text by space count", () => {
        let buf: [128]u8 = undefined;
        const src = "\n    foo\n      bar\n    baz\n";
        const n = redent(src, 2, buf[0..128]);
        expect_eq(buf[0..n], "\n  foo\n    bar\n  baz\n");
    });

    test("3. @cap/redent — redentWithIndent uses custom indent unit (tab / prefix)", () => {
        let buf: [128]u8 = undefined;
        const src = "    one\n      two\n    three";
        const n1 = redentWithIndent(src, 1, "\t", false, buf[0..128]);
        expect_eq(buf[0..n1], "\tone\n\t  two\n\tthree");

        const n2 = redentWithIndent(src, 2, "> ", false, buf[0..128]);
        expect_eq(buf[0..n2], "> > one\n> >   two\n> > three");
    });

    test("4. @cap/redent — include_empty_lines controls blank line indentation", () => {
        let buf: [128]u8 = undefined;
        const src = "    alpha\n\n    beta";
        const nFalse = redentWithIndent(src, 2, " ", false, buf[0..128]);
        expect_eq(buf[0..nFalse], "  alpha\n\n  beta");

        const nTrue = redentWithIndent(src, 2, " ", true, buf[0..128]);
        expect_eq(buf[0..nTrue], "  alpha\n  \n  beta");
    });

    test("5. @cap/redent — handles tab-indented source and CRLF line endings", () => {
        let buf: [128]u8 = undefined;
        const src = "\t\tfirst\r\n\t\t\tsecond\r\n\r\n\t\tthird";
        const n = redent(src, 3, buf[0..128]);
        expect_eq(buf[0..n], "   first\r\n   \tsecond\r\n\r\n   third");
    });

    test("6. @cap/redent — empty string, empty indent unit, and bounded output buffer", () => {
        let buf: [64]u8 = undefined;
        expect_eq(redent("", 4, buf[0..64]), 0 as usize);
        expect_eq(redentWithIndent("", 2, "\t", true, buf[0..64]), 0 as usize);

        const nEmptyUnit = redentWithIndent("    hello\n      world", 3, "", false, buf[0..64]);
        expect_eq(buf[0..nEmptyUnit], "hello\n  world");

        let tiny: [6]u8 = undefined;
        const nTiny = redent("    abcdef", 2, tiny[0..6]);
        expect_eq(nTiny, 6 as usize);
        expect_eq(tiny[0..nTiny], "  abcd");
    });
}
