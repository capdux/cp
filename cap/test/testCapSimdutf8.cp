import { test, expect_eq } from "cap:test";
import {
    isAscii, basicIsValidUtf8, validateUtf8Compat, countCodepoints
} from "../../packages/@cap/simdutf8/src/index.cp";

fn main() {
    test("1. @cap/simdutf8 — ASCII fast-path & valid multi-byte UTF-8", () => {
        let asc: [12]u8 = [104, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 33];
        expect_eq(isAscii(asc[0..12]), true);
        expect_eq(basicIsValidUtf8(asc[0..12]), true);
        expect_eq(countCodepoints(asc[0..12]) ?? 0, 12);

        // "中" (E4 B8 AD) + "😀" (F0 9F 98 80) + "é" (C3 A9)
        let utf: [9]u8 = [228, 184, 173, 240, 159, 152, 128, 195, 169];
        expect_eq(isAscii(utf[0..9]), false);
        expect_eq(basicIsValidUtf8(utf[0..9]), true);
        expect_eq(countCodepoints(utf[0..9]) ?? 0, 3);
    });

    test("2. @cap/simdutf8 — rejects overlong encodings (C0 80, E0 80 80, F0 80 80 80)", () => {
        let ov2: [2]u8 = [192, 128];
        let r2 = validateUtf8Compat(ov2[0..2]);
        expect_eq(r2.valid, false);
        expect_eq(r2.valid_up_to, 0);
        expect_eq(r2.error_len, 1);

        let ov3: [3]u8 = [224, 128, 128];
        expect_eq(basicIsValidUtf8(ov3[0..3]), false);

        let ov4: [4]u8 = [240, 128, 128, 128];
        expect_eq(basicIsValidUtf8(ov4[0..4]), false);
    });

    test("3. @cap/simdutf8 — rejects UTF-16 surrogates U+D800..U+DFFF (ED A0 80)", () => {
        let sur: [3]u8 = [237, 160, 128];
        let r = validateUtf8Compat(sur[0..3]);
        expect_eq(r.valid, false);
        expect_eq(r.valid_up_to, 0);
        expect_eq(r.has_error_len, true);
        expect_eq(r.error_len, 1);
    });

    test("4. @cap/simdutf8 — rejects codepoints > U+10FFFF (F4 90 80 80)", () => {
        let hi: [4]u8 = [244, 144, 128, 128];
        expect_eq(basicIsValidUtf8(hi[0..4]), false);
        // U+10FFFF itself (F4 8F BF BF) is valid
        let max_cp: [4]u8 = [244, 143, 191, 191];
        expect_eq(basicIsValidUtf8(max_cp[0..4]), true);
    });

    test("5. @cap/simdutf8 — truncated sequence reports has_error_len=false & exact valid_up_to", () => {
        // "ab" + incomplete 3-byte char (E4 B8)
        let trunc: [4]u8 = [97, 98, 228, 184];
        let r = validateUtf8Compat(trunc[0..4]);
        expect_eq(r.valid, false);
        expect_eq(r.valid_up_to, 2);
        expect_eq(r.has_error_len, false);
    });

    test("6. @cap/simdutf8 — mid-stream bad continuation reports exact error_len", () => {
        // "a" + E4 B8 'x' (invalid 3rd byte)
        let bad: [4]u8 = [97, 228, 184, 120];
        let r = validateUtf8Compat(bad[0..4]);
        expect_eq(r.valid, false);
        expect_eq(r.valid_up_to, 1);
        expect_eq(r.has_error_len, true);
        expect_eq(r.error_len, 2);
        expect_eq(countCodepoints(bad[0..4]) == null, true);
    });
}
