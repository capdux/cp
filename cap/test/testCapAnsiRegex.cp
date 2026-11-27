import { test, expect_eq } from "cap:test";
import {
    AnsiSpan, testAnsi, findFirstAnsi, findAllAnsi, countAnsiSequences, stripAllAnsi
} from "../../packages/ansi-regex/src/index.cp";

fn eqBytes(buf: []u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("testAnsi detects CSI & OSC sequences", () => {
        expect_eq(testAnsi("plain text"), false);
        expect_eq(testAnsi("\x1b[31mred\x1b[0m"), true);
        expect_eq(testAnsi("\x1b]8;;https://example.com\x07link\x1b]8;;\x07"), true);
    });

    test("findFirstAnsi locates exact byte span", () => {
        let s = "hi \x1b[1;32mgreen\x1b[0m!";
        let sp = findFirstAnsi(s);
        expect_eq(sp.found, true);
        expect_eq(sp.start, 3 as usize);
        expect_eq(sp.len, 7 as usize);
    });

    test("findFirstAnsi returns found=false on plain string", () => {
        let sp = findFirstAnsi("no escape here");
        expect_eq(sp.found, false);
    });

    test("findAllAnsi & countAnsiSequences", () => {
        let s = "\x1b[31mA\x1b[32mB\x1b[0m";
        let spans: [8]AnsiSpan = [AnsiSpan { start: 0, len: 0, found: false }; 8];
        let n = findAllAnsi(s, spans[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(countAnsiSequences(s), 3 as usize);
        expect_eq(spans[0].start, 0 as usize);
        expect_eq(spans[1].start, 6 as usize);
        expect_eq(spans[2].start, 12 as usize);
    });

    test("stripAllAnsi delegates to @cap/anstyle-parse cleanly", () => {
        let out: [64]u8 = [0; 64];
        let n = stripAllAnsi("\x1b[1;34mHello\x1b[0m \x1b[33mCP\x1b[0m!", out[0..64]);
        expect_eq(eqBytes(out[0..64], n, "Hello CP!"), true);
    });

    test("empty string & small buffer safety", () => {
        expect_eq(testAnsi(""), false);
        expect_eq(countAnsiSequences(""), 0 as usize);
        let spans: [1]AnsiSpan = [AnsiSpan { start: 0, len: 0, found: false }; 1];
        expect_eq(findAllAnsi("\x1b[31mA\x1b[0m", spans[0..1]), 1 as usize);
    });
}
