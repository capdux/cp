import { test, expect_eq } from "cap:test";
import {
    formatLine,
    formatDump,
    encodeHexSpaced,
    firstDiffOffset,
} from "../../packages/hexdump-nodejs/src/index.cp";

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
    test("1. @cap/hex-dump — Full 16-byte canonical hexdump -C line", () => {
        const data = "0123456789abcdef";
        let buf: [128]u8 = undefined;
        const n = formatLine(data, 0, buf[0..128]);
        const expected = "00000000  30 31 32 33 34 35 36 37  38 39 61 62 63 64 65 66  |0123456789abcdef|";
        expect_eq(n, 78 as usize);
        expect_eq(testEq(buf[0..n], expected), true);
    });

    test("2. @cap/hex-dump — Partial < 16-byte line alignment & non-printable '.'", () => {
        const data = "Hello World!\n";
        let buf: [128]u8 = undefined;
        const n = formatLine(data, 0, buf[0..128]);
        const expected = "00000000  48 65 6c 6c 6f 20 57 6f  72 6c 64 21 0a           |Hello World!.|";
        expect_eq(n, expected.len);
        expect_eq(testEq(buf[0..n], expected), true);
    });

    test("3. @cap/hex-dump — Multi-line formatDump with 00000000 & 00000010 offsets", () => {
        const data = "0123456789abcdefXYZ";
        let buf: [256]u8 = undefined;
        const n = formatDump(data, buf[0..256]);
        expect_eq(n > 78, true);
        expect_eq(buf[78], 10 as u8); // '\n'
        // Second line starts with "00000010  58 59 5a"
        expect_eq(testEq(buf[79..97], "00000010  58 59 5a"), true);
        expect_eq(testEq(buf[n - 5..n], "|XYZ|"), true);
    });

    test("4. @cap/hex-dump — encodeHexSpaced lowercase & uppercase", () => {
        const raw: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
        let buf: [32]u8 = undefined;

        const n1 = encodeHexSpaced(raw[0..4], false, buf[0..32]);
        expect_eq(testEq(buf[0..n1], "de ad be ef"), true);

        const n2 = encodeHexSpaced(raw[0..4], true, buf[0..32]);
        expect_eq(testEq(buf[0..n2], "DE AD BE EF"), true);
    });

    test("5. @cap/hex-dump — firstDiffOffset equal, differing byte & length mismatch", () => {
        expect_eq(firstDiffOffset("abcdef", "abcdef"), -1 as i64);
        expect_eq(firstDiffOffset("abcdef", "Xbcdef"), 0 as i64);
        expect_eq(firstDiffOffset("abcdef", "abcXef"), 3 as i64);
        expect_eq(firstDiffOffset("abc", "abcdef"), 3 as i64);
        expect_eq(firstDiffOffset("", ""), -1 as i64);
    });

    test("6. @cap/hex-dump — Empty slice & undersized buffer protection", () => {
        const empty = "";
        let buf: [128]u8 = undefined;
        expect_eq(formatLine(empty, 0, buf[0..128]), 0 as usize);
        expect_eq(formatDump(empty, buf[0..128]), 0 as usize);
        expect_eq(encodeHexSpaced(empty, false, buf[0..128]), 0 as usize);

        let tiny: [10]u8 = undefined;
        expect_eq(formatLine("hello", 0, tiny[0..10]), 0 as usize);
        expect_eq(formatDump("hello", tiny[0..10]), 0 as usize);
        expect_eq(encodeHexSpaced("hello", false, tiny[0..10]), 0 as usize);
    });
}
