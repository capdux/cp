// cap/test/testCapIban.cp
// Comprehensive unit tests for @cap/iban (ISO 13616 / ISO 7064 Mod-97-10)

import { test, expect_eq } from "cap:test";
import {
    countryLength,
    electronicFormat,
    friendlyFormat,
    mod97,
    isValid,
    computeCheckDigits,
} from "../../packages/iban_validate/src/index.cp";

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
    test("1. @cap/iban — valid DE, GB, FR, NL IBANs & countryLength table", () => {
        expect_eq(countryLength('D' as u8, 'E' as u8), 22 as usize);
        expect_eq(countryLength('G' as u8, 'B' as u8), 22 as usize);
        expect_eq(countryLength('F' as u8, 'R' as u8), 27 as usize);
        expect_eq(countryLength('N' as u8, 'L' as u8), 18 as usize);
        expect_eq(countryLength('Z' as u8, 'Z' as u8), 0 as usize);

        expect_eq(isValid("DE89370400440532013000"), true);
        expect_eq(isValid("GB82WEST12345698765432"), true);
        expect_eq(isValid("FR1420041010050500013M02606"), true);
        expect_eq(isValid("NL91ABNA0417164300"), true);
    });

    test("2. @cap/iban — lowercase & spaced/hyphenated friendly input validation", () => {
        expect_eq(isValid("de89 3704 0044 0532 0130 00"), true);
        expect_eq(isValid("gb82-west-1234-5698-7654-32"), true);
        expect_eq(isValid("nl91 abna 0417 1643 00"), true);
    });

    test("3. @cap/iban — rejects modified digits, wrong length & unknown country", () => {
        // Single digit tampered in DE IBAN
        expect_eq(isValid("DE89370400440532013001"), false);
        // Wrong length (missing last char)
        expect_eq(isValid("DE8937040044053201300"), false);
        // Unknown country ZZ
        expect_eq(isValid("ZZ89370400440532013000"), false);
        // Non-digit check digits
        expect_eq(isValid("DEAB370400440532013000"), false);
        // Invalid symbol
        expect_eq(isValid("DE8937040044053201300!"), false);
    });

    test("4. @cap/iban — electronicFormat & friendlyFormat", () => {
        let out: [64]u8 = undefined;
        const n1: usize = electronicFormat("de89 3704-0044 0532 0130 00", out[0..64]);
        expect_eq(n1, 22 as usize);
        expect_eq(sliceEq(out[0..n1], "DE89370400440532013000"), true);

        const n2: usize = friendlyFormat("de89370400440532013000", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "DE89 3704 0044 0532 0130 00"), true);
    });

    test("5. @cap/iban — computeCheckDigits generates valid check digits & verifies round-trip", () => {
        let d0: u8 = 0;
        let d1: u8 = 0;
        const ok: bool = computeCheckDigits('D' as u8, 'E' as u8, "370400440532013000", &d0, &d1);
        expect_eq(ok, true);
        expect_eq(d0, '8' as u8);
        expect_eq(d1, '9' as u8);

        // Build full IBAN and verify with isValid
        let full: [22]u8 = undefined;
        full[0] = 'D' as u8;
        full[1] = 'E' as u8;
        full[2] = d0;
        full[3] = d1;
        const bban = "370400440532013000";
        let i: usize = 0;
        while (i < bban.len) {
            full[4 + i] = bban[i];
            i += 1;
        }
        expect_eq(isValid(full[0..22]), true);

        // Wrong BBAN length returns false
        expect_eq(computeCheckDigits('D' as u8, 'E' as u8, "1234", &d0, &d1), false);
    });

    test("6. @cap/iban — mod97 edge cases", () => {
        expect_eq(mod97("DE89370400440532013000"), 1 as u32);
        expect_eq(mod97("GB82WEST12345698765432"), 1 as u32);
        expect_eq(mod97("DE89370400440532013001") != 1, true);
        expect_eq(mod97("AB"), 0 as u32);
        expect_eq(mod97("DE89!0000"), 0 as u32);
    });
}
