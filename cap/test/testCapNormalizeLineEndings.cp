import { test, expect_eq } from "cap:test";
import {
    LineEnding,
    lineEndingStr,
    detectLineEnding,
    countLineBreaks,
    countLines,
    needsNormalization,
    normalizedLen,
    normalizeToLf,
    normalizeToCrlf,
    normalizeToCr,
    normalizeLineEndings,
} from "../../packages/normalize-line-endings/src/index.cp";

fn main() {
    test("1. normalize-line-endings — detects LF, CRLF, CR, None, and Mixed", () => {
        expect_eq(detectLineEnding("hello world") == .None, true);
        expect_eq(detectLineEnding("a\nb\nc") == .Lf, true);
        expect_eq(detectLineEnding("a\r\nb\r\n") == .Crlf, true);
        expect_eq(detectLineEnding("a\rb\r") == .Cr, true);
        expect_eq(detectLineEnding("a\r\nb\nc") == .Mixed, true);
    });

    test("2. normalize-line-endings — normalizes CRLF to LF", () => {
        let buf: [64]u8 = undefined;
        const n = normalizeToLf("line1\r\nline2\r\nline3", buf[0..64]);
        expect_eq(buf[0..n] as string, "line1\nline2\nline3");
        expect_eq(n, normalizedLen("line1\r\nline2\r\nline3", .Lf));
    });

    test("3. normalize-line-endings — normalizes mixed CR, CRLF, and LF to LF", () => {
        let buf: [64]u8 = undefined;
        const n = normalizeToLf("a\rb\r\nc\nd\r", buf[0..64]);
        expect_eq(buf[0..n] as string, "a\nb\nc\nd\n");
    });

    test("4. normalize-line-endings — normalizes LF and CR to CRLF without doubling existing CRLF", () => {
        let buf: [64]u8 = undefined;
        const n = normalizeToCrlf("a\nb\r\nc\rd", buf[0..64]);
        expect_eq(buf[0..n] as string, "a\r\nb\r\nc\r\nd");
        expect_eq(n, normalizedLen("a\nb\r\nc\rd", .Crlf));
    });

    test("5. normalize-line-endings — preserves consecutive blank lines", () => {
        let buf: [64]u8 = undefined;
        const n = normalizeToLf("a\r\n\r\nb", buf[0..64]);
        expect_eq(buf[0..n] as string, "a\n\nb");
    });

    test("6. normalize-line-endings — handles empty string safely", () => {
        let buf: [16]u8 = undefined;
        expect_eq(normalizeToLf("", buf[0..16]), 0 as usize);
        expect_eq(normalizeToCrlf("", buf[0..16]), 0 as usize);
        expect_eq(normalizeToCr("", buf[0..16]), 0 as usize);
    });

    test("7. normalize-line-endings — normalizeToCr and generic normalizeLineEndings", () => {
        let buf: [64]u8 = undefined;
        const n1 = normalizeToCr("a\r\nb\nc\rd", buf[0..64]);
        expect_eq(buf[0..n1] as string, "a\rb\rc\rd");
        expect_eq(n1, normalizedLen("a\r\nb\nc\rd", .Cr));

        const n2 = normalizeLineEndings("x\ny\r\nz", .Crlf, buf[0..64]);
        expect_eq(buf[0..n2] as string, "x\r\ny\r\nz");

        const n3 = normalizeLineEndings("x\r\ny\nz", .None, buf[0..64]);
        expect_eq(buf[0..n3] as string, "xyz");
        expect_eq(n3, normalizedLen("x\r\ny\nz", .None));

        expect_eq(lineEndingStr(.Lf), "\n");
        expect_eq(lineEndingStr(.Crlf), "\r\n");
        expect_eq(lineEndingStr(.Cr), "\r");
        expect_eq(lineEndingStr(.None), "");
    });

    test("8. normalize-line-endings — countLines, countLineBreaks, and needsNormalization", () => {
        expect_eq(countLines(""), 0 as usize);
        expect_eq(countLines("single"), 1 as usize);
        expect_eq(countLines("a\nb\n"), 2 as usize);
        expect_eq(countLines("a\r\nb\rc"), 3 as usize);
        expect_eq(countLineBreaks("a\r\nb\nc\r"), 3 as usize);
        expect_eq(countLineBreaks("no-breaks"), 0 as usize);

        expect_eq(needsNormalization("no-breaks", .Lf), false);
        expect_eq(needsNormalization("a\nb", .Lf), false);
        expect_eq(needsNormalization("a\r\nb", .Lf), true);
        expect_eq(needsNormalization("a\nb\r\nc", .Crlf), true);
    });
}
