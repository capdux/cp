// cap/test/testCapTotpUri.cp
// Comprehensive unit tests for @cap/totp-uri

import { test, expect_eq } from "cap:test";
import {
    OTP_TOTP,
    OTP_HOTP,
    ALG_SHA1,
    ALG_SHA256,
    ALG_SHA512,
    OtpAuthUri,
    isValidBase32Secret,
    parseOtpAuthUri,
    formatOtpAuthUri,
    splitLabel,
} from "../../packages/@cap/totp-uri/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/totp-uri — Standard TOTP URI full parameter parsing", () => {
        const uri = "otpauth://totp/Acme:alice@example.com?secret=JBSWY3DPEHPK3PXP&issuer=Acme&algorithm=SHA256&digits=8&period=60";
        const p = parseOtpAuthUri(uri);
        expect_eq(p.valid, true);
        expect_eq(p.kind, OTP_TOTP);
        expect_eq(sliceEq(p.label, "Acme:alice@example.com"), true);
        expect_eq(sliceEq(p.secret, "JBSWY3DPEHPK3PXP"), true);
        expect_eq(sliceEq(p.issuer, "Acme"), true);
        expect_eq(p.algorithm, ALG_SHA256);
        expect_eq(p.digits, 8 as u8);
        expect_eq(p.period, 60 as u32);
    });

    test("2. @cap/totp-uri — Default parameters SHA1 / 6 digits / 30s period", () => {
        const uri = "otpauth://totp/user@example.com?secret=JBSWY3DPEHPK3PXP";
        const p = parseOtpAuthUri(uri);
        expect_eq(p.valid, true);
        expect_eq(p.kind, OTP_TOTP);
        expect_eq(p.algorithm, ALG_SHA1);
        expect_eq(p.digits, 6 as u8);
        expect_eq(p.period, 30 as u32);
    });

    test("3. @cap/totp-uri — HOTP counter parsing & missing counter rejection", () => {
        const goodHotp = "otpauth://hotp/Bank:bob?secret=GEZDGNBVGY3TQOJQ&counter=42&algorithm=SHA512";
        const p1 = parseOtpAuthUri(goodHotp);
        expect_eq(p1.valid, true);
        expect_eq(p1.kind, OTP_HOTP);
        expect_eq(p1.counter, 42 as u64);
        expect_eq(p1.algorithm, ALG_SHA512);

        const missingCounter = "otpauth://hotp/Bank:bob?secret=GEZDGNBVGY3TQOJQ";
        expect_eq(parseOtpAuthUri(missingCounter).valid, false);
    });

    test("4. @cap/totp-uri — formatOtpAuthUri serialization & round-trip", () => {
        const cfg = OtpAuthUri{
            kind: OTP_TOTP,
            label: "My App:alice@example.com",
            secret: "JBSWY3DPEHPK3PXP",
            issuer: "My App",
            algorithm: ALG_SHA256,
            digits: 8 as u8,
            period: 30 as u32,
            counter: 0 as u64,
            valid: true,
        };
        let buf: [256]u8 = undefined;
        const n = formatOtpAuthUri(cfg, buf[0..256]);
        expect_eq(n > (0 as usize), true);
        const expected = "otpauth://totp/My%20App:alice@example.com?secret=JBSWY3DPEHPK3PXP&issuer=My%20App&algorithm=SHA256&digits=8&period=30";
        expect_eq(sliceEq(buf[0..n], expected), true);

        const reparsed = parseOtpAuthUri(buf[0..n]);
        expect_eq(reparsed.valid, true);
        expect_eq(reparsed.algorithm, ALG_SHA256);
        expect_eq(reparsed.digits, 8 as u8);
        expect_eq(reparsed.period, 30 as u32);
    });

    test("5. @cap/totp-uri — splitLabel & isValidBase32Secret validation", () => {
        const parts1 = splitLabel("GitHub:octocat");
        expect_eq(sliceEq(parts1.issuer, "GitHub"), true);
        expect_eq(sliceEq(parts1.account, "octocat"), true);

        const parts2 = splitLabel("solo@example.com");
        expect_eq(parts2.issuer.len, 0 as usize);
        expect_eq(sliceEq(parts2.account, "solo@example.com"), true);

        expect_eq(isValidBase32Secret("JBSWY3DPEHPK3PXP"), true);
        expect_eq(isValidBase32Secret("jbswy3dp===="), true);
        expect_eq(isValidBase32Secret(""), false);
        expect_eq(isValidBase32Secret("===="), false);
        expect_eq(isValidBase32Secret("INVALID0189"), false);
        expect_eq(isValidBase32Secret("JBSW=Y3DP"), false);
    });

    test("6. @cap/totp-uri — Reject invalid scheme, non-Base32 secret, digits=7 & tiny buffer", () => {
        expect_eq(parseOtpAuthUri("https://totp/a?secret=JBSWY3DP").valid, false);
        expect_eq(parseOtpAuthUri("otpauth://totp/a?secret=BAD0189!").valid, false);
        expect_eq(parseOtpAuthUri("otpauth://totp/a?secret=JBSWY3DP&digits=7").valid, false);
        expect_eq(parseOtpAuthUri("otpauth://totp/a?secret=JBSWY3DP&algorithm=MD5").valid, false);

        const cfg = OtpAuthUri{
            kind: OTP_TOTP,
            label: "a",
            secret: "JBSWY3DP",
            issuer: "",
            algorithm: ALG_SHA1,
            digits: 6 as u8,
            period: 30 as u32,
            counter: 0 as u64,
            valid: true,
        };
        let tiny: [10]u8 = undefined;
        expect_eq(formatOtpAuthUri(cfg, tiny[0..10]), 0 as usize);
    });
}
