// Source: cp/cap/test/test_iban_production_deep.cp (Production deep tests for cap:iban)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    IBAN_MIN_LEN,
    IBAN_MAX_LEN,
    IbanInfo,
    ibanCountryLength,
    ibanMod97,
    isValidIban,
    parseIban,
    computeIbanCheckDigits,
    formatIbanElectronic,
    formatIbanPrint,
} from "cap:iban";

fn bytesEq(a: []const u8, b: []const u8): bool {
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

test("1. Multi-country standard IBAN validation & parsing (DE, GB, FR, NL, CH)", () => {
    expect_eq(IBAN_MIN_LEN, 15 as usize);
    expect_eq(IBAN_MAX_LEN, 34 as usize);

    // Germany (22)
    const de = "DE89 3704 0044 0532 0130 00";
    expect_eq(isValidIban(de), true);
    expect_eq(ibanMod97(de) ?? unreachable, 1 as u8);
    const info_de = parseIban(de) ?? unreachable;
    expect_eq(info_de.country[0], 0x44 as u8); // 'D'
    expect_eq(info_de.country[1], 0x45 as u8); // 'E'
    expect_eq(info_de.check_digits[0], 0x38 as u8); // '8'
    expect_eq(info_de.check_digits[1], 0x39 as u8); // '9'
    expect_eq(info_de.totalLen(), 22 as usize);
    expect_eq(bytesEq(info_de.bbanSlice(), "370400440532013000"), true);

    // United Kingdom (22)
    const gb = "GB82 WEST 1234 5698 7654 32";
    expect_eq(isValidIban(gb), true);
    const info_gb = parseIban(gb) ?? unreachable;
    expect_eq(info_gb.totalLen(), 22 as usize);
    expect_eq(bytesEq(info_gb.bbanSlice(), "WEST12345698765432"), true);

    // France (27)
    const fr = "FR76 3000 6000 0112 3456 7890 189";
    expect_eq(isValidIban(fr), true);
    const info_fr = parseIban(fr) ?? unreachable;
    expect_eq(info_fr.totalLen(), 27 as usize);
    expect_eq(bytesEq(info_fr.bbanSlice(), "30006000011234567890189"), true);

    // Netherlands (18) — lowercase & hyphenated tolerance
    const nl = "nl91-abna-0417-1643-00";
    expect_eq(isValidIban(nl), true);
    const info_nl = parseIban(nl) ?? unreachable;
    expect_eq(info_nl.totalLen(), 18 as usize);
    expect_eq(bytesEq(info_nl.bbanSlice(), "ABNA0417164300"), true);

    // Switzerland (21)
    const ch = "CH93 0076 2011 6238 5295 7";
    expect_eq(isValidIban(ch), true);
    const info_ch = parseIban(ch) ?? unreachable;
    expect_eq(info_ch.totalLen(), 21 as usize);
    expect_eq(bytesEq(info_ch.bbanSlice(), "00762011623852957"), true);
});

test("2. Country length registry coverage & case-insensitive lookup", () => {
    expect_eq(ibanCountryLength(0x44, 0x45) ?? unreachable, 22 as u8); // DE
    expect_eq(ibanCountryLength(0x64, 0x65) ?? unreachable, 22 as u8); // de (lowercase)
    expect_eq(ibanCountryLength(0x4E, 0x4F) ?? unreachable, 15 as u8); // NO (min len 15)
    expect_eq(ibanCountryLength(0x4D, 0x54) ?? unreachable, 31 as u8); // MT
    expect_eq(ibanCountryLength(0x42, 0x52) ?? unreachable, 29 as u8); // BR
    expect_eq(ibanCountryLength(0x55, 0x41) ?? unreachable, 29 as u8); // UA
    expect_eq(ibanCountryLength(0x41, 0x45) ?? unreachable, 23 as u8); // AE
    expect_eq(ibanCountryLength(0x53, 0x41) ?? unreachable, 24 as u8); // SA
    expect_eq(ibanCountryLength(0x51, 0x41) ?? unreachable, 29 as u8); // QA
    expect_eq(ibanCountryLength(0x54, 0x52) ?? unreachable, 26 as u8); // TR
    expect_eq(ibanCountryLength(0x50, 0x4C) ?? unreachable, 28 as u8); // PL
    expect_eq(ibanCountryLength(0x42, 0x45) ?? unreachable, 16 as u8); // BE

    // Unknown or non-alpha country codes
    expect_eq(ibanCountryLength(0x55, 0x53) == null, true); // US (not IBAN)
    expect_eq(ibanCountryLength(0x5A, 0x5A) == null, true); // ZZ
    expect_eq(ibanCountryLength(0x31, 0x45) == null, true); // 1E
});

test("3. computeIbanCheckDigits recomputation including leading-zero check digits ('02')", () => {
    // DE89 3704 0044 0532 0130 00
    const cd_de = computeIbanCheckDigits([0x44, 0x45], "370400440532013000") ?? unreachable;
    expect_eq(cd_de[0], 0x38 as u8); // '8'
    expect_eq(cd_de[1], 0x39 as u8); // '9'

    // GB82 WEST 1234 5698 7654 32
    const cd_gb = computeIbanCheckDigits([0x47, 0x42], "WEST 1234 5698 7654 32") ?? unreachable;
    expect_eq(cd_gb[0], 0x38 as u8); // '8'
    expect_eq(cd_gb[1], 0x32 as u8); // '2'

    // Leading zero check digits (< 10): DE02 3704 0044 0532 0130 14
    const cd_02 = computeIbanCheckDigits([0x44, 0x45], "370400440532013014") ?? unreachable;
    expect_eq(cd_02[0], 0x30 as u8); // '0'
    expect_eq(cd_02[1], 0x32 as u8); // '2'
    expect_eq(isValidIban("DE02 3704 0044 0532 0130 14"), true);

    // Invalid country or mismatched BBAN length rejected
    expect_eq(computeIbanCheckDigits([0x55, 0x53], "123456789012345678") == null, true);
    expect_eq(computeIbanCheckDigits([0x44, 0x45], "37040044053201300") == null, true); // too short
});

test("4. Electronic & print bidirectional formatting", () => {
    let elec: [40]u8 = std.mem.zeroes([40]u8);
    let prnt: [48]u8 = std.mem.zeroes([48]u8);

    const raw = "de89 3704-0044 0532 0130 00";
    const elen = formatIbanElectronic(raw, elec[0..]) ?? unreachable;
    expect_eq(elen, 22 as usize);
    expect_eq(bytesEq(elec[0..elen], "DE89370400440532013000"), true);

    const plen = formatIbanPrint(elec[0..elen], prnt[0..]) ?? unreachable;
    expect_eq(plen, 27 as usize);
    expect_eq(bytesEq(prnt[0..plen], "DE89 3704 0044 0532 0130 00"), true);

    // Exact multiple of 4 length (BE = 16 -> 3 spaces = 19 bytes, no trailing space)
    const cd_be = computeIbanCheckDigits([0x42, 0x45], "510007547061") ?? unreachable;
    expect_eq(cd_be[0], 0x36 as u8); // '6'
    expect_eq(cd_be[1], 0x38 as u8); // '8'
    const be_plen = formatIbanPrint("BE68510007547061", prnt[0..]) ?? unreachable;
    expect_eq(be_plen, 19 as usize);
    expect_eq(bytesEq(prnt[0..be_plen], "BE68 5100 0754 7061"), true);
});

test("5. Single-character tampering, wrong country length, unknown country & buffer overflow rejection", () => {
    // Single-digit tampering (0 -> 1 in last digit) fails Mod-97
    expect_eq(isValidIban("DE89 3704 0044 0532 0130 01"), false);
    expect_eq(parseIban("DE89 3704 0044 0532 0130 01") == null, true);

    // Adjacent transposition tampering (37 -> 73) fails Mod-97
    expect_eq(isValidIban("DE89 7304 0044 0532 0130 00"), false);

    // Length mismatch against country table (DE expects 22, given 21 or 23)
    expect_eq(isValidIban("DE89 3704 0044 0532 0130 0"), false);
    expect_eq(isValidIban("DE89 3704 0044 0532 0130 000"), false);

    // Unknown country code
    expect_eq(isValidIban("ZZ89 3704 0044 0532 0130 00"), false);

    // Non-numeric check digits or illegal symbols
    expect_eq(isValidIban("DEAB 3704 0044 0532 0130 00"), false);
    expect_eq(isValidIban("DE89_3704_0044_0532_0130_00"), false);
    expect_eq(isValidIban("DE89!370400440532013000"), false);
    expect_eq(ibanMod97("DE89!370400440532013000") == null, true);

    // Too short / empty
    expect_eq(isValidIban(""), false);
    expect_eq(isValidIban("DE89"), false);

    // Output buffer overflow rejection
    let tiny_elec: [21]u8 = std.mem.zeroes([21]u8);
    expect_eq(formatIbanElectronic("DE89 3704 0044 0532 0130 00", tiny_elec[0..]) == null, true);
    let tiny_prnt: [26]u8 = std.mem.zeroes([26]u8);
    expect_eq(formatIbanPrint("DE89370400440532013000", tiny_prnt[0..]) == null, true);
});
