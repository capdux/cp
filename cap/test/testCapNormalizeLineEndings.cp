import { test, expect_eq } from "cap:test";
import {
    LineEnding,
    detectLineEnding,
    normalizeToLf,
    normalizeToCrlf,
} from "../../packages/normalize-line-endings/src/index.cp";

fn main() {
    test("1. @cap/normalize-line-endings — detects LF, CRLF, CR, None, and Mixed", () => {
        expect_eq(detectLineEnding("hello world") == .None, true);
        expect_eq(detectLineEnding("a\nb\nc") == .Lf, true);
        expect_eq(detectLineEnding("a\r\nb\r\n") == .Crlf, true);
        expect_eq(detectLineEnding("a\rb\r") == .Cr, true);
        expect_eq(detectLineEnding("a\r\nb\nc") == .Mixed, true);
    });

    test("2. @cap/normalize-line-endings — normalizes CRLF to LF", () => {
        let buf: [64]u8 = [0; 64];
        const n = normalizeToLf("line1\r\nline2\r\nline3", buf[0..64]);
        expect_eq(buf[0..n] as string, "line1\nline2\nline3");
    });

    test("3. @cap/normalize-line-endings — normalizes mixed CR, CRLF, and LF to LF", () => {
        let buf: [64]u8 = [0; 64];
        const n = normalizeToLf("a\rb\r\nc\nd\r", buf[0..64]);
        expect_eq(buf[0..n] as string, "a\nb\nc\nd\n");
    });

    test("4. @cap/normalize-line-endings — normalizes LF and CR to CRLF without doubling existing CRLF", () => {
        let buf: [64]u8 = [0; 64];
        const n = normalizeToCrlf("a\nb\r\nc\rd", buf[0..64]);
        expect_eq(buf[0..n] as string, "a\r\nb\r\nc\r\nd");
    });

    test("5. @cap/normalize-line-endings — preserves consecutive blank lines", () => {
        let buf: [64]u8 = [0; 64];
        const n = normalizeToLf("a\r\n\r\nb", buf[0..64]);
        expect_eq(buf[0..n] as string, "a\n\nb");
    });

    test("6. @cap/normalize-line-endings — handles empty string safely", () => {
        let buf: [16]u8 = [0; 16];
        expect_eq(normalizeToLf("", buf[0..16]), 0 as usize);
        expect_eq(normalizeToCrlf("", buf[0..16]), 0 as usize);
    });
}
