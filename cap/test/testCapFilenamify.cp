// cap/test/testCapFilenamify.cp
// Comprehensive unit tests for @cap/filenamify (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    filenamify,
    filenamifyWithOptions,
    isReservedFilename,
} from "../../packages/@cap/filenamify/src/index.cp";

fn main() {
    test("1. @cap/filenamify — replaces illegal path/Windows characters & control bytes", () => {
        let buf: [128]u8 = undefined;
        const n1 = filenamify("foo/bar:baz*qux?\"<>|end", buf[0..128]);
        expect_eq(buf[0..n1], "foo!bar!baz!qux!end");

        const n2 = filenamify("hello\x00\x07\x1Fworld\x7Ftest", buf[0..128]);
        expect_eq(buf[0..n2], "hello!world!test");

        const n3 = filenamify("normal_file-2026.txt", buf[0..128]);
        expect_eq(buf[0..n3], "normal_file-2026.txt");
    });

    test("2. @cap/filenamify — collapses repeated replacements & strips outer replacements/dots/spaces", () => {
        let buf: [128]u8 = undefined;
        const n1 = filenamify("///foo///bar///", buf[0..128]);
        expect_eq(buf[0..n1], "foo!bar");

        const n2 = filenamify("!!hello!!!world!!", buf[0..128]);
        expect_eq(buf[0..n2], "hello!world");

        const n3 = filenamify("report.pdf...   ", buf[0..128]);
        expect_eq(buf[0..n3], "report.pdf");

        const n4 = filenamify("foo..bar", buf[0..128]);
        expect_eq(buf[0..n4], "foo!bar");

        const n5 = filenamify(".gitignore", buf[0..128]);
        expect_eq(buf[0..n5], ".gitignore");
    });

    test("3. @cap/filenamify — detects and neutralizes Windows reserved device names", () => {
        expect_eq(isReservedFilename("CON"), true);
        expect_eq(isReservedFilename("prn"), true);
        expect_eq(isReservedFilename("Aux.txt"), true);
        expect_eq(isReservedFilename("nul.tar.gz"), true);
        expect_eq(isReservedFilename("COM1"), true);
        expect_eq(isReservedFilename("com9.log"), true);
        expect_eq(isReservedFilename("LPT1"), true);
        expect_eq(isReservedFilename("lpt9.bin"), true);
        expect_eq(isReservedFilename("COM0"), false);
        expect_eq(isReservedFilename("LPT0"), false);
        expect_eq(isReservedFilename("console"), false);
        expect_eq(isReservedFilename("null"), false);

        let buf: [64]u8 = undefined;
        const n1 = filenamify("CON", buf[0..64]);
        expect_eq(buf[0..n1], "CON!");

        const n2 = filenamify("con.txt", buf[0..64]);
        expect_eq(buf[0..n2], "con.txt!");

        const n3 = filenamifyWithOptions("LPT5", "_", 100, buf[0..64]);
        expect_eq(buf[0..n3], "LPT5_");
    });

    test("4. @cap/filenamify — custom replacement string & empty/dot-only fallback", () => {
        let buf: [64]u8 = undefined;
        const n1 = filenamifyWithOptions("a:b/c*d", "-", 100, buf[0..64]);
        expect_eq(buf[0..n1], "a-b-c-d");

        const n2 = filenamifyWithOptions("a::b", "__", 100, buf[0..64]);
        expect_eq(buf[0..n2], "a__b");

        expect_eq(filenamify("", buf[0..64]), 0 as usize);

        const nDot = filenamify(".", buf[0..64]);
        expect_eq(buf[0..nDot], "!");

        const nDots = filenamify("..", buf[0..64]);
        expect_eq(buf[0..nDots], "!");

        const nAllBad = filenamify("<>/?*", buf[0..64]);
        expect_eq(buf[0..nAllBad], "!");
    });

    test("5. @cap/filenamify — max_len truncation with UTF-8 multi-byte safety", () => {
        let buf: [128]u8 = undefined;
        const n1 = filenamifyWithOptions("abcdefghijklmno", "!", 10, buf[0..128]);
        expect_eq(buf[0..n1], "abcdefghij");

        // Each CJK char is 3 UTF-8 bytes: "你好世界" = 12 bytes; max_len = 7 must cut cleanly at 6 ("你好")
        const n2 = filenamifyWithOptions("你好世界", "!", 7, buf[0..128]);
        expect_eq(buf[0..n2], "你好");
    });

    test("6. @cap/filenamify — invalid replacement fallback & bounded output buffer", () => {
        let buf: [64]u8 = undefined;
        // Invalid replacement "/" falls back to "!"
        const n1 = filenamifyWithOptions("a:b", "/", 100, buf[0..64]);
        expect_eq(buf[0..n1], "a!b");

        // Output buffer smaller than sanitized length truncates safely
        let tiny: [4]u8 = undefined;
        const n2 = filenamify("hello_world", tiny[0..4]);
        expect_eq(n2, 4 as usize);
        expect_eq(tiny[0..n2], "hell");
    });
}
