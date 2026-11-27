// cap/test/testCapHumanNumber.cp
// Comprehensive unit tests for @cap/human-number

import { test, expect_eq } from "cap:test";
import {
    formatGrouped,
    formatCompactEn,
    formatCompactZh,
    formatPercent,
    toRoman,
    fromRoman,
    toEnglishWords,
} from "../../packages/numeral/src/index.cp";

fn main() {
    test("1. @cap/human-number — formatGrouped (positive, negative, zero, custom sep)", () => {
        let out: [128]u8 = undefined;
        expect_eq(formatGrouped(0, ',', out[0..128]), "0");
        expect_eq(formatGrouped(999, ',', out[0..128]), "999");
        expect_eq(formatGrouped(1000, ',', out[0..128]), "1,000");
        expect_eq(formatGrouped(1234567, ',', out[0..128]), "1,234,567");
        expect_eq(formatGrouped(-987654, ',', out[0..128]), "-987,654");
        expect_eq(formatGrouped(1234567890, '_', out[0..128]), "1_234_567_890");
    });

    test("2. @cap/human-number — formatCompactEn (K, M, B, T & negative)", () => {
        let out: [128]u8 = undefined;
        expect_eq(formatCompactEn(950, out[0..128]), "950");
        expect_eq(formatCompactEn(1000, out[0..128]), "1K");
        expect_eq(formatCompactEn(1200, out[0..128]), "1.2K");
        expect_eq(formatCompactEn(3500000, out[0..128]), "3.5M");
        expect_eq(formatCompactEn(2400000000, out[0..128]), "2.4B");
        expect_eq(formatCompactEn(1500000000000, out[0..128]), "1.5T");
        expect_eq(formatCompactEn(-4200, out[0..128]), "-4.2K");
    });

    test("3. @cap/human-number — formatCompactZh (万, 亿, 万亿)", () => {
        let out: [128]u8 = undefined;
        expect_eq(formatCompactZh(8888, out[0..128]), "8888");
        expect_eq(formatCompactZh(10000, out[0..128]), "1万");
        expect_eq(formatCompactZh(12500, out[0..128]), "1.3万");
        expect_eq(formatCompactZh(250000000, out[0..128]), "2.5亿");
        expect_eq(formatCompactZh(1500000000000, out[0..128]), "1.5万亿");
        expect_eq(formatCompactZh(-32000, out[0..128]), "-3.2万");
    });

    test("4. @cap/human-number — formatPercent (decimals, rounding, negative, zero)", () => {
        let out: [128]u8 = undefined;
        expect_eq(formatPercent(0.9995, 2, out[0..128]), "99.95%");
        expect_eq(formatPercent(0.5, 0, out[0..128]), "50%");
        expect_eq(formatPercent(0.05, 2, out[0..128]), "5.00%");
        expect_eq(formatPercent(1.0, 1, out[0..128]), "100.0%");
        expect_eq(formatPercent(-0.1234, 1, out[0..128]), "-12.3%");
        expect_eq(formatPercent(0.0, 2, out[0..128]), "0.00%");
    });

    test("5. @cap/human-number — Roman numerals round-trip & strict invalid rejection", () => {
        let out: [64]u8 = undefined;
        expect_eq(toRoman(1, out[0..64]), "I");
        expect_eq(toRoman(4, out[0..64]), "IV");
        expect_eq(toRoman(1994, out[0..64]), "MCMXCIV");
        expect_eq(toRoman(2026, out[0..64]), "MMXXVI");
        expect_eq(toRoman(3999, out[0..64]), "MMMCMXCIX");
        expect_eq(toRoman(0, out[0..64]), "");
        expect_eq(toRoman(4000, out[0..64]), "");

        expect_eq(fromRoman("MMXXVI") ?? 0, 2026 as u16);
        expect_eq(fromRoman("mcmxciv") ?? 0, 1994 as u16);
        expect_eq(fromRoman("MMMCMXCIX") ?? 0, 3999 as u16);

        // Strict invalid forms must return null
        expect_eq(fromRoman("") == null, true);
        expect_eq(fromRoman("IIII") == null, true);
        expect_eq(fromRoman("VV") == null, true);
        expect_eq(fromRoman("IC") == null, true);
        expect_eq(fromRoman("IIV") == null, true);
        expect_eq(fromRoman("MMMM") == null, true);
        expect_eq(fromRoman("XYZ") == null, true);
    });

    test("6. @cap/human-number — toEnglishWords (zero, negative, thousands, millions, billions)", () => {
        let out: [256]u8 = undefined;
        expect_eq(toEnglishWords(0, out[0..256]), "zero");
        expect_eq(toEnglishWords(19, out[0..256]), "nineteen");
        expect_eq(toEnglishWords(42, out[0..256]), "forty-two");
        expect_eq(toEnglishWords(-42, out[0..256]), "minus forty-two");
        expect_eq(
            toEnglishWords(1234, out[0..256]),
            "one thousand two hundred thirty-four",
        );
        expect_eq(
            toEnglishWords(1000000, out[0..256]),
            "one million",
        );
        expect_eq(
            toEnglishWords(2005012007, out[0..256]),
            "two billion five million twelve thousand seven",
        );
    });
}
