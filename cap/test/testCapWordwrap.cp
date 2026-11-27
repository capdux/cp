import { test, expect_eq } from "cap:test";
import { wrapText } from "../../packages/@cap/wordwrap/src/index.cp";

fn main() {
    test("1. @cap/wordwrap — wraps words cleanly at column boundary", () => {
        let buf: [128]u8 = [0; 128];
        const n = wrapText("hello world foo bar", 0, 11, false, buf[0..128]);
        expect_eq(buf[0..n] as string, "hello world\nfoo bar");
    });

    test("2. @cap/wordwrap — applies left indentation start_col on every wrapped line", () => {
        let buf: [128]u8 = [0; 128];
        const n = wrapText("hello world", 4, 10, false, buf[0..128]);
        expect_eq(buf[0..n] as string, "    hello\n    world");
    });

    test("3. @cap/wordwrap — soft mode preserves overlong words intact", () => {
        let buf: [128]u8 = [0; 128];
        const n = wrapText("hi superlongword ok", 0, 8, false, buf[0..128]);
        expect_eq(buf[0..n] as string, "hi\nsuperlongword\nok");
    });

    test("4. @cap/wordwrap — hard mode breaks overlong words across lines", () => {
        let buf: [128]u8 = [0; 128];
        const n = wrapText("abcdefghij", 0, 4, true, buf[0..128]);
        expect_eq(buf[0..n] as string, "abcd\nefgh\nij");

        const n2 = wrapText("abcdef", 2, 5, true, buf[0..128]);
        expect_eq(buf[0..n2] as string, "  abc\n  def");
    });

    test("5. @cap/wordwrap — preserves explicit newlines and paragraph breaks", () => {
        let buf: [128]u8 = [0; 128];
        const n = wrapText("line one\n\nline two", 2, 20, false, buf[0..128]);
        expect_eq(buf[0..n] as string, "  line one\n\n  line two");
    });

    test("6. @cap/wordwrap — handles empty input and tight column bounds safely", () => {
        let buf: [64]u8 = [0; 64];
        expect_eq(wrapText("", 0, 10, false, buf[0..64]), 0 as usize);
        const n = wrapText("ab", 5, 5, true, buf[0..64]);
        expect_eq(buf[0..n] as string, "     a\n     b");
    });
}
