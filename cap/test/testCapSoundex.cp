import { test, expect_eq } from "cap:test";
import {
    DEFAULT_SOUNDEX_LEN,
    soundexCharCode,
    soundex,
    soundexCode,
    soundexPadded,
    soundsLike,
    soundsLikeWithLength,
    soundexDifference,
} from "../../packages/soundex-code/src/index.cp";

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

    test("7. @cap/soundex — soundexCode custom maxLength (npm soundex-code semantics)", () => {
        expect_eq(DEFAULT_SOUNDEX_LEN, 4 as usize);
        let buf: [16]u8 = undefined;

        // Default maxLength=0 falls back to 4
        let n0 = soundexCode("Robert", 0, buf[0..16]);
        expect_eq(n0, 4 as usize);
        expect_eq(eqStr(buf[0..n0], "R163"), true);

        // Long word with maxLength=6 produces up to 6 chars
        let n1 = soundexCode("Washington", 6, buf[0..16]);
        expect_eq(eqStr(buf[0..n1], "W25235"), true);

        // Short word with maxLength=6 still pads to min 4
        let n2 = soundexCode("a", 6, buf[0..16]);
        expect_eq(eqStr(buf[0..n2], "A000"), true);

        // maxLength=2 truncates to 2 chars
        let n3 = soundexCode("Ashcraft", 2, buf[0..16]);
        expect_eq(eqStr(buf[0..n3], "A2"), true);
    });

    test("8. @cap/soundex — soundexPadded, soundsLikeWithLength & soundexCharCode", () => {
        let buf: [8]u8 = undefined;
        let n = soundexPadded("Robert", 6, buf[0..8]);
        expect_eq(n, 6 as usize);
        expect_eq(eqStr(buf[0..n], "R16300"), true);

        expect_eq(soundsLikeWithLength("Robert", "Rupert", 4), true);
        expect_eq(soundsLikeWithLength("Washington", "Wasington", 6), true);
        expect_eq(soundexCharCode('b'), '1');
        expect_eq(soundexCharCode('H'), 'H');
        expect_eq(soundexCharCode('a'), '0');
    });

    test("9. @cap/soundex — leading non-alpha & mixed punctuation filtering", () => {
        let buf: [8]u8 = undefined;
        let n = soundexCode("  --123 O'Connor!", 4, buf[0..8]);
        expect_eq(eqStr(buf[0..n], "O256"), true);
    });
}
