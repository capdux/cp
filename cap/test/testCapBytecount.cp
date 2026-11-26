import { test, expect_eq } from "cap:test";
import {
    count, countFast, numChars, countLines, countAsciiWhitespace, countInRange
} from "../../packages/@cap/bytecount/src/index.cp";

fn main() {
    test("1. @cap/bytecount — count and countFast agree across aligned & unaligned slices", () => {
        let buf: [11]u8 = [97, 98, 97, 99, 97, 100, 97, 101, 97, 102, 97]; // 6 x 'a'
        expect_eq(count(buf[0..11], 97), 6);
        expect_eq(countFast(buf[0..11], 97), 6);
        expect_eq(count(buf[0..11], 122), 0);
    });

    test("2. @cap/bytecount — numChars counts UTF-8 scalar values accurately", () => {
        // "A" (1B) + "é" (2B: C3 A9) + "中" (3B: E4 B8 AD) + "😀" (4B: F0 9F 98 80) = 10 bytes, 4 chars
        let utf: [10]u8 = [65, 195, 169, 228, 184, 173, 240, 159, 152, 128];
        expect_eq(numChars(utf[0..10]), 4);
        expect_eq(numChars(utf[0..0]), 0);
    });

    test("3. @cap/bytecount — countLines handles trailing newline and non-trailing newline", () => {
        let a: [6]u8 = [97, 10, 98, 10, 99, 10]; // "a\nb\nc\n" -> 3 lines
        let b: [5]u8 = [97, 10, 98, 10, 99];     // "a\nb\nc" -> 3 lines
        expect_eq(countLines(a[0..6]), 3);
        expect_eq(countLines(b[0..5]), 3);
        expect_eq(countLines(a[0..0]), 0);
    });

    test("4. @cap/bytecount — countAsciiWhitespace counts space/tab/CR/LF/FF", () => {
        let s: [8]u8 = [97, 32, 9, 98, 13, 10, 12, 99];
        expect_eq(countAsciiWhitespace(s[0..8]), 5);
    });

    test("5. @cap/bytecount — countInRange counts ASCII digits and rejects inverted bounds", () => {
        let s: [7]u8 = [97, 48, 57, 98, 53, 47, 58]; // 'a','0','9','b','5','/',':' -> 3 digits
        expect_eq(countInRange(s[0..7], 48, 57), 3);
        expect_eq(countInRange(s[0..7], 60, 50), 0);
    });

    test("6. @cap/bytecount — empty slice returns 0 for all counters", () => {
        let empty: [1]u8 = [0];
        expect_eq(count(empty[0..0], 0), 0);
        expect_eq(countFast(empty[0..0], 0), 0);
        expect_eq(countAsciiWhitespace(empty[0..0]), 0);
        expect_eq(countInRange(empty[0..0], 0, 255), 0);
    });
}
