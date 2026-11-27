import { test, expect_eq } from "cap:test";
import {
    stripFinalNewline, stripAllTrailingNewlines,
    hasFinalNewline, hasFinalCrlf, ensureFinalNewline
} from "../../packages/strip-final-newline/src/index.cp";

fn eqStr(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn eqBuf(buf: []u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("strips single trailing LF or CRLF", () => {
        expect_eq(eqStr(stripFinalNewline("hello\n"), "hello"), true);
        expect_eq(eqStr(stripFinalNewline("hello\r\n"), "hello"), true);
        expect_eq(eqStr(stripFinalNewline("hello"), "hello"), true);
    });

    test("only strips ONE trailing newline matching npm spec", () => {
        expect_eq(eqStr(stripFinalNewline("foo\n\n"), "foo\n"), true);
        expect_eq(eqStr(stripFinalNewline("foo\r\n\r\n"), "foo\r\n"), true);
        expect_eq(eqStr(stripFinalNewline("\n"), ""), true);
        expect_eq(eqStr(stripFinalNewline("\r\n"), ""), true);
    });

    test("bare CR without LF is preserved by stripFinalNewline", () => {
        expect_eq(eqStr(stripFinalNewline("foo\r"), "foo\r"), true);
        expect_eq(hasFinalNewline("foo\r"), false);
        expect_eq(hasFinalCrlf("foo\r\n"), true);
    });

    test("stripAllTrailingNewlines removes all trailing CR/LF", () => {
        expect_eq(eqStr(stripAllTrailingNewlines("bar\r\n\n\r"), "bar"), true);
        expect_eq(eqStr(stripAllTrailingNewlines("\n\r\n"), ""), true);
    });

    test("ensureFinalNewline adds or normalizes cleanly", () => {
        let out: [32]u8 = [0; 32];
        let n1 = ensureFinalNewline("hello", false, out[0..32]);
        expect_eq(eqBuf(out[0..32], n1, "hello\n"), true);
        let n2 = ensureFinalNewline("hello\n", true, out[0..32]);
        expect_eq(eqBuf(out[0..32], n2, "hello\r\n"), true);
    });

    test("empty string & small buffer safety", () => {
        expect_eq(eqStr(stripFinalNewline(""), ""), true);
        expect_eq(hasFinalNewline(""), false);
        expect_eq(hasFinalCrlf(""), false);
        let tiny: [3]u8 = [0; 3];
        let n = ensureFinalNewline("abcdef", false, tiny[0..3]);
        expect_eq(n, 3 as usize);
    });
}
