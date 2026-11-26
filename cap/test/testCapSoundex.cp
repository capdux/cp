import { test, expect_eq } from "cap:test";
import {
    soundex,
    soundsLike,
    soundexDifference,
} from "../../packages/@cap/soundex/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/soundex — NARA classic reference vectors (Robert, Rupert, Rubin)", () => {
        let buf: [4]u8 = undefined;
        expect_eq(soundex("Robert", buf[0..]), 4);
        expect_eq(eqStr(buf[0..4], "R163"), true);

        expect_eq(soundex("Rupert", buf[0..]), 4);
        expect_eq(eqStr(buf[0..4], "R163"), true);

        expect_eq(soundex("Rubin", buf[0..]), 4);
        expect_eq(eqStr(buf[0..4], "R150"), true);
    });

    test("2. @cap/soundex — H/W adjacency collapse (Ashcraft, Ashcroft)", () => {
        let buf: [4]u8 = undefined;
        expect_eq(soundex("Ashcraft", buf[0..]), 4);
        expect_eq(eqStr(buf[0..4], "A261"), true);

        expect_eq(soundex("Ashcroft", buf[0..]), 4);
        expect_eq(eqStr(buf[0..4], "A261"), true);
    });

    test("3. @cap/soundex — vowel separation & Pfister / Tymczak / Honeyman", () => {
        let buf: [4]u8 = undefined;
        expect_eq(soundex("Tymczak", buf[0..]), 4);
        expect_eq(eqStr(buf[0..4], "T522"), true);

        expect_eq(soundex("Pfister", buf[0..]), 4);
        expect_eq(eqStr(buf[0..4], "P236"), true);

        expect_eq(soundex("Honeyman", buf[0..]), 4);
        expect_eq(eqStr(buf[0..4], "H555"), true);
    });

    test("4. @cap/soundex — soundsLike phonetic matching", () => {
        expect_eq(soundsLike("Robert", "Rupert"), true);
        expect_eq(soundsLike("Smith", "Smythe"), true);
        expect_eq(soundsLike("Catherine", "Kathryn"), false); // C265 vs K365
    });

    test("5. @cap/soundex — soundexDifference similarity score 0..4", () => {
        expect_eq(soundexDifference("Robert", "Rupert"), 4);
        expect_eq(soundexDifference("Robert", "Rubin"), 2);
        expect_eq(soundexDifference("", "Robert"), 0);
    });

    test("6. @cap/soundex — non-alpha, empty & small buffer guards", () => {
        let buf: [4]u8 = undefined;
        expect_eq(soundex("", buf[0..]), 0);
        expect_eq(soundex("12345", buf[0..]), 0);

        let tiny: [3]u8 = undefined;
        expect_eq(soundex("Robert", tiny[0..]), 0);
    });
}
