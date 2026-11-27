// cap/test/testCapCardBin.cp
// Comprehensive unit tests for @cap/card-bin

import { test, expect_eq } from "cap:test";
import {
    CardNetwork,
    luhnValid,
    inspectCard,
    networkName,
    formatCardNumber,
    maskCardPci,
} from "../../packages/credit-card-type/src/index.cp";

fn main() {
    test("1. @cap/card-bin — Visa, Mastercard (51-55 & 2221-2720), Amex detection", () => {
        const visa = inspectCard("4111 1111 1111 1111");
        expect_eq(visa.network == CardNetwork.Visa, true);
        expect_eq(visa.digit_count, 16 as usize);
        expect_eq(visa.valid_length, true);
        expect_eq(visa.luhn_ok, true);
        expect_eq(visa.is_valid, true);
        expect_eq(visa.cvv_len, 3 as u8);
        expect_eq(networkName(visa.network), "Visa");

        const mcOld = inspectCard("5500-0000-0000-0004");
        expect_eq(mcOld.network == CardNetwork.Mastercard, true);
        expect_eq(mcOld.is_valid, true);
        expect_eq(mcOld.cvv_len, 3 as u8);
        expect_eq(networkName(mcOld.network), "Mastercard");

        const mcNew = inspectCard("2221 0000 0000 0009");
        expect_eq(mcNew.network == CardNetwork.Mastercard, true);
        expect_eq(mcNew.is_valid, true);

        const amex = inspectCard("3782-822463-10005");
        expect_eq(amex.network == CardNetwork.Amex, true);
        expect_eq(amex.digit_count, 15 as usize);
        expect_eq(amex.is_valid, true);
        expect_eq(amex.cvv_len, 4 as u8);
        expect_eq(networkName(amex.network), "American Express");
    });

    test("2. @cap/card-bin — UnionPay, JCB, Discover, DinersClub, Maestro detection", () => {
        const up = inspectCard("6200 0000 0000 0005");
        expect_eq(up.network == CardNetwork.UnionPay, true);
        expect_eq(up.is_valid, true);
        expect_eq(networkName(up.network), "UnionPay");

        const jcb = inspectCard("3528 0000 0000 0007");
        expect_eq(jcb.network == CardNetwork.Jcb, true);
        expect_eq(jcb.is_valid, true);
        expect_eq(networkName(jcb.network), "JCB");

        const disc = inspectCard("6011 1111 1111 1117");
        expect_eq(disc.network == CardNetwork.Discover, true);
        expect_eq(disc.is_valid, true);
        expect_eq(networkName(disc.network), "Discover");

        const diners = inspectCard("3056 9309 0259 04");
        expect_eq(diners.network == CardNetwork.DinersClub, true);
        expect_eq(diners.digit_count, 14 as usize);
        expect_eq(diners.is_valid, true);
        expect_eq(networkName(diners.network), "Diners Club");

        const maestro = inspectCard("5018 0000 0000 0009");
        expect_eq(maestro.network == CardNetwork.Maestro, true);
        expect_eq(maestro.is_valid, true);
        expect_eq(networkName(maestro.network), "Maestro");
    });

    test("3. @cap/card-bin — Luhn mod-10 validation & checksum failure", () => {
        expect_eq(luhnValid("4111-1111-1111-1111"), true);
        expect_eq(luhnValid("4111-1111-1111-1112"), false);
        expect_eq(luhnValid("3782 822463 10005"), true);
        expect_eq(luhnValid("3782 822463 10006"), false);
        expect_eq(luhnValid(""), false);
        expect_eq(luhnValid("4"), false);
        expect_eq(luhnValid("4111abcd11111111"), false);
    });

    test("4. @cap/card-bin — formatCardNumber (Amex 4-6-5 & standard 4-4-4-4)", () => {
        let out1: [32]u8 = undefined;
        expect_eq(formatCardNumber("378282246310005", out1[0..32]), "3782 822463 10005");

        let out2: [32]u8 = undefined;
        expect_eq(formatCardNumber("4111-1111-1111-1111", out2[0..32]), "4111 1111 1111 1111");

        let out3: [32]u8 = undefined;
        expect_eq(formatCardNumber("6200000000000005123", out3[0..32]), "6200 0000 0000 0005 123");

        let out4: [32]u8 = undefined;
        expect_eq(formatCardNumber("invalid!", out4[0..32]), "");
    });

    test("5. @cap/card-bin — maskCardPci PCI-DSS masking (16-digit, 15-digit & short cards)", () => {
        let m1: [32]u8 = undefined;
        expect_eq(maskCardPci("4111-1111-1111-1111", m1[0..32]), "411111******1111");

        let m2: [32]u8 = undefined;
        expect_eq(maskCardPci("3782 822463 10005", m2[0..32]), "378282*****0005");

        let m3: [32]u8 = undefined;
        expect_eq(maskCardPci("12345678", m3[0..32]), "****5678");

        let m4: [32]u8 = undefined;
        expect_eq(maskCardPci("1234", m4[0..32]), "1234");

        let m5: [32]u8 = undefined;
        expect_eq(maskCardPci("", m5[0..32]), "");
    });

    test("6. @cap/card-bin — Invalid length, unknown BIN, and malformed inputs", () => {
        const shortVisa = inspectCard("4111 1111 1111");
        expect_eq(shortVisa.network == CardNetwork.Visa, true);
        expect_eq(shortVisa.valid_length, false);
        expect_eq(shortVisa.is_valid, false);

        const unknownBin = inspectCard("9999 0000 0000 0000");
        expect_eq(unknownBin.network == CardNetwork.Unknown, true);
        expect_eq(unknownBin.is_valid, false);
        expect_eq(networkName(unknownBin.network), "Unknown");

        const badChars = inspectCard("4111 1111 1111 111X");
        expect_eq(badChars.network == CardNetwork.Unknown, true);
        expect_eq(badChars.digit_count, 0 as usize);
        expect_eq(badChars.is_valid, false);
    });
}
