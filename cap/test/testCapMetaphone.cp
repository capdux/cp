import { test, expect_eq } from "cap:test";
import { metaphone, soundsLike } from "../../packages/metaphone/src/index.cp";

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
    test("1. @cap/metaphone — Classic vectors Michael, Phone, Knight & Wright", () => {
        let buf: [16]u8 = undefined;
        expect_eq(testEq(buf[0..metaphone("Michael", 8, buf[0..16])], "MXL"), true);
        expect_eq(testEq(buf[0..metaphone("Phone", 8, buf[0..16])], "FN"), true);
        expect_eq(testEq(buf[0..metaphone("Knight", 8, buf[0..16])], "NT"), true);
        expect_eq(testEq(buf[0..metaphone("Wright", 8, buf[0..16])], "RT"), true);
    });

    test("2. @cap/metaphone — School, Smith & Smythe phonetic equivalence", () => {
        let buf: [16]u8 = undefined;
        expect_eq(testEq(buf[0..metaphone("School", 8, buf[0..16])], "SKL"), true);
        expect_eq(testEq(buf[0..metaphone("Smith", 8, buf[0..16])], "SM0"), true);
        expect_eq(testEq(buf[0..metaphone("Smythe", 8, buf[0..16])], "SM0"), true);
        expect_eq(soundsLike("Smith", "Smythe"), true);
    });

    test("3. @cap/metaphone — TH -> 0 & SH -> X rules", () => {
        let buf: [16]u8 = undefined;
        expect_eq(testEq(buf[0..metaphone("The", 8, buf[0..16])], "0"), true);
        expect_eq(testEq(buf[0..metaphone("Ship", 8, buf[0..16])], "XP"), true);
        expect_eq(testEq(buf[0..metaphone("Nation", 8, buf[0..16])], "NXN"), true);
    });

    test("4. @cap/metaphone — max_len truncation control", () => {
        let buf: [16]u8 = undefined;
        const n = metaphone("internationalization", 4, buf[0..16]);
        expect_eq(n, 4 as usize);
        expect_eq(testEq(buf[0..4], "INTR"), true);
    });

    test("5. @cap/metaphone — soundsLike positive & negative pairs", () => {
        expect_eq(soundsLike("Phone", "Fone"), true);
        expect_eq(soundsLike("Knight", "Night"), true);
        expect_eq(soundsLike("Write", "Right"), true);
        expect_eq(soundsLike("Apple", "Orange"), false);
    });

    test("6. @cap/metaphone — Empty string, digits-only & zero/small buffer guards", () => {
        let buf: [16]u8 = undefined;
        expect_eq(metaphone("", 4, buf[0..16]), 0 as usize);
        expect_eq(metaphone("12345!!!", 4, buf[0..16]), 0 as usize);
        expect_eq(metaphone("Phone", 0, buf[0..16]), 0 as usize);
        expect_eq(metaphone("Phone", 4, buf[0..0]), 0 as usize);
        expect_eq(soundsLike("", ""), false);

        // Buffer smaller than max_len safely caps output
        let tiny: [2]u8 = undefined;
        expect_eq(metaphone("School", 8, tiny[0..2]), 2 as usize);
        expect_eq(testEq(tiny[0..2], "SK"), true);
    });
}
