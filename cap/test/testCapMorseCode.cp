// cap/test/testCapMorseCode.cp
// Comprehensive unit tests for @cap/morse-code (ITU-R M.1677)

import { test, expect_eq } from "cap:test";
import {
    encodeChar,
    decodeSymbol,
    encodeText,
    decodeText,
} from "../../packages/@cap/morse-code/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/morse-code — classic 'SOS' -> '... --- ...'", () => {
        expect_eq(sliceEq(encodeChar('S' as u8), "..."), true);
        expect_eq(sliceEq(encodeChar('o' as u8), "---"), true);
        expect_eq(decodeSymbol("..."), 'S' as u8);
        expect_eq(decodeSymbol("---"), 'O' as u8);

        let out: [32]u8 = undefined;
        const n: usize = encodeText("sos", out[0..32]);
        expect_eq(sliceEq(out[0..n], "... --- ..."), true);
    });

    test("2. @cap/morse-code — 'HELLO WORLD' with '/' word separator round-trip", () => {
        let enc: [128]u8 = undefined;
        let dec_buf: [64]u8 = undefined;
        const n: usize = encodeText("  hello   world  ", enc[0..128]);
        expect_eq(
            sliceEq(enc[0..n], ".... . .-.. .-.. --- / .-- --- .-. .-.. -.."),
            true,
        );

        const dn: usize = decodeText(enc[0..n], dec_buf[0..64]);
        expect_eq(sliceEq(dec_buf[0..dn], "HELLO WORLD"), true);
    });

    test("3. @cap/morse-code — digits '0123456789' full round-trip", () => {
        let enc: [128]u8 = undefined;
        let dec_buf: [32]u8 = undefined;
        const n: usize = encodeText("0123456789", enc[0..128]);
        expect_eq(n > 0, true);
        const dn: usize = decodeText(enc[0..n], dec_buf[0..32]);
        expect_eq(sliceEq(dec_buf[0..dn], "0123456789"), true);
    });

    test("4. @cap/morse-code — punctuation . , ? / - ( ) @ round-trip", () => {
        let enc: [128]u8 = undefined;
        let dec_buf: [32]u8 = undefined;
        const src = "A.B,C?D@E";
        const n: usize = encodeText(src, enc[0..128]);
        expect_eq(n > 0, true);
        const dn: usize = decodeText(enc[0..n], dec_buf[0..32]);
        expect_eq(sliceEq(dec_buf[0..dn], src), true);
    });

    test("5. @cap/morse-code — rejects unknown characters & invalid symbols", () => {
        expect_eq(encodeChar('#' as u8).len, 0 as usize);
        expect_eq(decodeSymbol("........"), 0 as u8);
        expect_eq(decodeSymbol(""), 0 as u8);

        let buf: [64]u8 = undefined;
        expect_eq(encodeText("HI#THERE", buf[0..64]), 0 as usize);
        expect_eq(decodeText("... ........ ---", buf[0..64]), 0 as usize);
    });

    test("6. @cap/morse-code — empty input & tiny buffer protection", () => {
        let buf: [32]u8 = undefined;
        expect_eq(encodeText("", buf[0..32]), 0 as usize);
        expect_eq(encodeText("   ", buf[0..32]), 0 as usize);
        expect_eq(decodeText("", buf[0..32]), 0 as usize);

        let tiny: [4]u8 = undefined;
        expect_eq(encodeText("SOS", tiny[0..4]), 0 as usize);
        expect_eq(decodeText(".... . .-.. .-.. ---", tiny[0..4]), 0 as usize);
    });
}
