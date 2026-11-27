import { test, expect_eq } from "cap:test";
import {
    offsetToLineCol,
    lineColToOffset,
    getLine,
    countLines,
} from "../../packages/line-column/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/line-col — Multi-line LF offsetToLineCol positioning", () => {
        const src = "fn main() {\n    let x = 42;\n}";
        const lc0 = offsetToLineCol(src, 0);
        expect_eq(lc0.valid, true);
        expect_eq(lc0.line, 1 as usize);
        expect_eq(lc0.col, 1 as usize);

        // Offset 12 is start of line 2 ("    let x = 42;")
        const lc1 = offsetToLineCol(src, 12);
        expect_eq(lc1.valid, true);
        expect_eq(lc1.line, 2 as usize);
        expect_eq(lc1.col, 1 as usize);

        // Offset 16 is 'l' in "let" (line 2, col 5)
        const lc2 = offsetToLineCol(src, 16);
        expect_eq(lc2.line, 2 as usize);
        expect_eq(lc2.col, 5 as usize);

        // End of string (offset == len)
        const lc_end = offsetToLineCol(src, src.len);
        expect_eq(lc_end.valid, true);
        expect_eq(lc_end.line, 3 as usize);
        expect_eq(lc_end.col, 2 as usize);
    });

    test("2. @cap/line-col — CRLF (\\r\\n) accurate handling", () => {
        const src = "abc\r\ndef\r\nghi";
        // 'd' is at index 5 -> line 2, col 1
        const lc1 = offsetToLineCol(src, 5);
        expect_eq(lc1.valid, true);
        expect_eq(lc1.line, 2 as usize);
        expect_eq(lc1.col, 1 as usize);

        // 'i' is at index 12 -> line 3, col 3
        const lc2 = offsetToLineCol(src, 12);
        expect_eq(lc2.line, 3 as usize);
        expect_eq(lc2.col, 3 as usize);

        expect_eq(testEq(getLine(src, 1), "abc"), true);
        expect_eq(testEq(getLine(src, 2), "def"), true);
        expect_eq(testEq(getLine(src, 3), "ghi"), true);
    });

    test("3. @cap/line-col — lineColToOffset <-> offsetToLineCol roundtrip", () => {
        const src = "first\nsecond line\r\nthird";
        const off1 = lineColToOffset(src, 2, 8); // 'l' in "line"
        expect_eq(off1, 13 as i64);
        const back1 = offsetToLineCol(src, off1 as usize);
        expect_eq(back1.line, 2 as usize);
        expect_eq(back1.col, 8 as usize);

        const off2 = lineColToOffset(src, 3, 1); // 't' in "third"
        expect_eq(off2, 19 as i64);
        const back2 = offsetToLineCol(src, off2 as usize);
        expect_eq(back2.line, 3 as usize);
        expect_eq(back2.col, 1 as usize);
    });

    test("4. @cap/line-col — getLine first, middle, last & out-of-bounds", () => {
        const src = "alpha\nbeta\ngamma\n";
        expect_eq(testEq(getLine(src, 1), "alpha"), true);
        expect_eq(testEq(getLine(src, 2), "beta"), true);
        expect_eq(testEq(getLine(src, 3), "gamma"), true);
        expect_eq(testEq(getLine(src, 0), ""), true);
        expect_eq(testEq(getLine(src, 4), ""), true);
    });

    test("5. @cap/line-col — countLines (empty, single line, trailing newline)", () => {
        expect_eq(countLines(""), 0 as usize);
        expect_eq(countLines("hello"), 1 as usize);
        expect_eq(countLines("hello\n"), 1 as usize);
        expect_eq(countLines("a\nb"), 2 as usize);
        expect_eq(countLines("a\nb\n"), 2 as usize);
        expect_eq(countLines("a\r\nb\r\nc\r\n"), 3 as usize);
    });

    test("6. @cap/line-col — Out-of-bounds offsets & 0-based line/col rejection", () => {
        const src = "ab\ncd";
        expect_eq(offsetToLineCol(src, src.len + 1).valid, false);
        expect_eq(lineColToOffset(src, 0, 1), -1 as i64);
        expect_eq(lineColToOffset(src, 1, 0), -1 as i64);
        expect_eq(lineColToOffset(src, 5, 1), -1 as i64);
        expect_eq(lineColToOffset(src, 1, 10), -1 as i64);
    });
}
