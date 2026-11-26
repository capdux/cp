// cap/test/testCapPhone.cp
// Comprehensive unit tests for @cap/phone

import { test, expect_eq } from "cap:test";
import {
    PhoneNumber,
    parsePhoneNumber,
    isValidPhone,
    countryCodeFromIso2,
    iso2FromCountryCode,
    supportedCountryCount,
} from "../../packages/@cap/phone/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn main() {
    test("1. @cap/phone — International parsing (CN/US/AU/GB/JP/SG/HK)", () => {
        let buf: [40]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

        const cn = parsePhoneNumber("+86 138 0013 8000", "");
        expect_eq(cn.is_valid, true);
        expect_eq(cn.country_code, 86 as u16);
        expect_eq(strEq(cn.iso2, "CN"), true);
        expect_eq(strEq(cn.formatE164(buf[0..40]), "+8613800138000"), true);

        const us = parsePhoneNumber("+1 (415) 555-2671", "");
        expect_eq(us.is_valid, true);
        expect_eq(us.country_code, 1 as u16);
        expect_eq(strEq(us.iso2, "US"), true);
        expect_eq(strEq(us.formatE164(buf[0..40]), "+14155552671"), true);

        const au = parsePhoneNumber("+61 412-345-678", "");
        expect_eq(au.is_valid, true);
        expect_eq(au.country_code, 61 as u16);
        expect_eq(strEq(au.iso2, "AU"), true);
        expect_eq(strEq(au.formatE164(buf[0..40]), "+61412345678"), true);

        const gb = parsePhoneNumber("+44 (0) 7700 900123", "");
        expect_eq(gb.is_valid, true);
        expect_eq(gb.country_code, 44 as u16);
        expect_eq(strEq(gb.iso2, "GB"), true);
        expect_eq(strEq(gb.formatE164(buf[0..40]), "+447700900123"), true);

        const jp = parsePhoneNumber("+81 90-1234-5678", "");
        expect_eq(jp.is_valid, true);
        expect_eq(jp.country_code, 81 as u16);
        expect_eq(strEq(jp.iso2, "JP"), true);
        expect_eq(strEq(jp.formatE164(buf[0..40]), "+819012345678"), true);

        const sg = parsePhoneNumber("+65 8123 4567", "");
        expect_eq(sg.is_valid, true);
        expect_eq(sg.country_code, 65 as u16);
        expect_eq(strEq(sg.iso2, "SG"), true);
        expect_eq(strEq(sg.formatE164(buf[0..40]), "+6581234567"), true);

        const hk = parsePhoneNumber("+852 9123 4567", "");
        expect_eq(hk.is_valid, true);
        expect_eq(hk.country_code, 852 as u16);
        expect_eq(strEq(hk.iso2, "HK"), true);
        expect_eq(strEq(hk.formatE164(buf[0..40]), "+85291234567"), true);
    });

    test("2. @cap/phone — 00 international prefix & tel: URI parsing", () => {
        let buf: [40]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

        const cn00 = parsePhoneNumber("0086 138-0013-8000", "");
        expect_eq(cn00.is_valid, true);
        expect_eq(cn00.country_code, 86 as u16);
        expect_eq(strEq(cn00.formatE164(buf[0..40]), "+8613800138000"), true);

        const au00 = parsePhoneNumber("0061 0412 345 678", "");
        expect_eq(au00.is_valid, true);
        expect_eq(au00.country_code, 61 as u16);
        expect_eq(strEq(au00.formatE164(buf[0..40]), "+61412345678"), true);

        const tel = parsePhoneNumber("tel:+852-9123-4567", "");
        expect_eq(tel.is_valid, true);
        expect_eq(tel.country_code, 852 as u16);
        expect_eq(strEq(tel.formatE164(buf[0..40]), "+85291234567"), true);
    });

    test("3. @cap/phone — Default country completion & leading-zero stripping", () => {
        let buf: [40]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

        const au = parsePhoneNumber("0412 345 678", "AU");
        expect_eq(au.is_valid, true);
        expect_eq(au.country_code, 61 as u16);
        expect_eq(strEq(au.formatE164(buf[0..40]), "+61412345678"), true);

        const gb = parsePhoneNumber("07700 900123", "gb");
        expect_eq(gb.is_valid, true);
        expect_eq(gb.country_code, 44 as u16);
        expect_eq(strEq(gb.formatE164(buf[0..40]), "+447700900123"), true);

        const jp = parsePhoneNumber("090-1234-5678", "JP");
        expect_eq(jp.is_valid, true);
        expect_eq(jp.country_code, 81 as u16);
        expect_eq(strEq(jp.formatE164(buf[0..40]), "+819012345678"), true);

        const ca = parsePhoneNumber("+1 416-555-0199", "CA");
        expect_eq(ca.is_valid, true);
        expect_eq(ca.country_code, 1 as u16);
        expect_eq(strEq(ca.iso2, "CA"), true);

        const us_local = parsePhoneNumber("1 (415) 555-2671", "US");
        expect_eq(us_local.is_valid, true);
        expect_eq(strEq(us_local.formatE164(buf[0..40]), "+14155552671"), true);
    });

    test("4. @cap/phone — 4 formatting modes (E.164, International, National, RFC 3966)", () => {
        let buf: [40]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

        const au = parsePhoneNumber("0412 345 678", "AU");
        expect_eq(strEq(au.formatE164(buf[0..40]), "+61412345678"), true);
        expect_eq(strEq(au.formatInternational(buf[0..40]), "+61 412 345 678"), true);
        expect_eq(strEq(au.formatNational(buf[0..40]), "0412 345 678"), true);
        expect_eq(strEq(au.formatRfc3966(buf[0..40]), "tel:+61-412-345-678"), true);

        const us = parsePhoneNumber("(415) 555-2671", "US");
        expect_eq(strEq(us.formatE164(buf[0..40]), "+14155552671"), true);
        expect_eq(strEq(us.formatInternational(buf[0..40]), "+1 415 555 2671"), true);
        expect_eq(strEq(us.formatNational(buf[0..40]), "(415) 555-2671"), true);
        expect_eq(strEq(us.formatRfc3966(buf[0..40]), "tel:+1-415-555-2671"), true);

        const jp = parsePhoneNumber("090-1234-5678", "JP");
        expect_eq(strEq(jp.formatE164(buf[0..40]), "+819012345678"), true);
        expect_eq(strEq(jp.formatInternational(buf[0..40]), "+81 90 1234 5678"), true);
        expect_eq(strEq(jp.formatNational(buf[0..40]), "090-1234-5678"), true);
        expect_eq(strEq(jp.formatRfc3966(buf[0..40]), "tel:+81-90-1234-5678"), true);

        const cn = parsePhoneNumber("+8613800138000", "");
        expect_eq(strEq(cn.formatInternational(buf[0..40]), "+86 138 0013 8000"), true);
        expect_eq(strEq(cn.formatNational(buf[0..40]), "138 0013 8000"), true);
        expect_eq(strEq(cn.formatRfc3966(buf[0..40]), "tel:+86-138-0013-8000"), true);
    });

    test("5. @cap/phone — Bidirectional country code & ISO2 mapping", () => {
        expect_eq(supportedCountryCount() >= 40, true);

        expect_eq(countryCodeFromIso2("US"), 1 as u16);
        expect_eq(countryCodeFromIso2("ca"), 1 as u16);
        expect_eq(countryCodeFromIso2("CN"), 86 as u16);
        expect_eq(countryCodeFromIso2("AU"), 61 as u16);
        expect_eq(countryCodeFromIso2("GB"), 44 as u16);
        expect_eq(countryCodeFromIso2("HK"), 852 as u16);
        expect_eq(countryCodeFromIso2("TW"), 886 as u16);
        expect_eq(countryCodeFromIso2("AE"), 971 as u16);
        expect_eq(countryCodeFromIso2("XX"), 0 as u16);
        expect_eq(countryCodeFromIso2(""), 0 as u16);

        expect_eq(strEq(iso2FromCountryCode(1), "US"), true);
        expect_eq(strEq(iso2FromCountryCode(86), "CN"), true);
        expect_eq(strEq(iso2FromCountryCode(61), "AU"), true);
        expect_eq(strEq(iso2FromCountryCode(852), "HK"), true);
        expect_eq(strEq(iso2FromCountryCode(971), "AE"), true);
        expect_eq(strEq(iso2FromCountryCode(999), ""), true);
    });

    test("6. @cap/phone — Invalid & malformed phone number rejection", () => {
        expect_eq(isValidPhone("", "US"), false);
        expect_eq(isValidPhone("   ", "AU"), false);
        expect_eq(isValidPhone("abc-def-ghij", "US"), false);
        expect_eq(isValidPhone("+86 1380013", ""), false);          // too short for CN
        expect_eq(isValidPhone("+1 415 555 267199999", ""), false); // exceeds E.164 max 15 digits
        expect_eq(isValidPhone("+1 015 555 2671", ""), false);      // invalid NANP area code starting with 0
        expect_eq(isValidPhone("+86 11800138000", ""), false);      // invalid CN mobile prefix 11x
        expect_eq(isValidPhone("+999 12345678", ""), false);        // unknown country code
        expect_eq(isValidPhone("0412 345 678", ""), false);         // domestic without default_iso2
        expect_eq(isValidPhone("((415) 555-2671", "US"), false);    // unbalanced parentheses
        expect_eq(isValidPhone("+61+412345678", ""), false);        // duplicate '+'
        expect_eq(isValidPhone("+61 000 000 000", ""), false);      // all zeros

        let buf: [32]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        const bad = parsePhoneNumber("not-a-phone", "US");
        expect_eq(bad.is_valid, false);
        expect_eq(bad.is_possible, false);
        expect_eq(strEq(bad.formatE164(buf[0..32]), ""), true);
        expect_eq(strEq(bad.formatInternational(buf[0..32]), ""), true);
        expect_eq(strEq(bad.formatNational(buf[0..32]), ""), true);
        expect_eq(strEq(bad.formatRfc3966(buf[0..32]), ""), true);
    });
}
