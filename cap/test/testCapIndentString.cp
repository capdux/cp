import { test, expect_eq } from "cap:test";
import { indentString, indentStringWith } from "../../packages/@cap/indent-string/src/index.cp";

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
    test("basic space indentation across multiple lines", () => {
        let out: [64]u8 = [0; 64];
        let n = indentString("a\nb\nc", 2, out[0..64]);
        expect_eq(eqBuf(out[0..64], n, "  a\n  b\n  c"), true);
    });

    test("skips empty lines by default", () => {
        let out: [64]u8 = [0; 64];
        let n = indentString("a\n\nb", 4, out[0..64]);
        expect_eq(eqBuf(out[0..64], n, "    a\n\n    b"), true);
    });

    test("include_empty_lines=true indents blank lines too", () => {
        let out: [64]u8 = [0; 64];
        let n = indentStringWith("a\n\nb", 2, " ", true, out[0..64]);
        expect_eq(eqBuf(out[0..64], n, "  a\n  \n  b"), true);
    });

    test("custom indent unit (tabs or prefix)", () => {
        let out: [64]u8 = [0; 64];
        let n = indentStringWith("foo\nbar", 1, "> ", false, out[0..64]);
        expect_eq(eqBuf(out[0..64], n, "> foo\n> bar"), true);
    });

    test("preserves CRLF & count=0 passthrough", () => {
        let out: [64]u8 = [0; 64];
        let n1 = indentString("x\r\ny", 1, out[0..64]);
        expect_eq(eqBuf(out[0..64], n1, " x\r\n y"), true);
        let n2 = indentString("hello", 0, out[0..64]);
        expect_eq(eqBuf(out[0..64], n2, "hello"), true);
    });

    test("empty input & small buffer safety", () => {
        let out: [5]u8 = [0; 5];
        expect_eq(indentString("", 4, out[0..5]), 0 as usize);
        let n = indentString("abcdef", 2, out[0..5]);
        expect_eq(n, 5 as usize);
        expect_eq(eqBuf(out[0..5], n, "  abc"), true);
    });
}
