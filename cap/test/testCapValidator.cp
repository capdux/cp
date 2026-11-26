// cap/test/testCapValidator.cp
// Comprehensive unit tests for @cap/validator

import { test, expect_eq } from "cap:test";
import {
    Validator,
    isAscii,
    isAlphanumeric,
    isNumeric,
    isHex,
    isBase64,
    isSlug,
    hasLenBetween,
    startsWith,
    endsWith,
    containsStr,
    isOneOf,
    isEmail,
    isIpv4,
    isIpv6,
    isCidr,
    isUrl,
    isUuid,
    isUlid,
    isSemver,
    isIsoDate,
    isIsoDateTime,
    isLuhn,
    isE164Phone,
    isPort,
    inRangeI64,
    isMultipleOf,
} from "../../packages/@cap/validator/src/index.cp";

fn main() {
    test("1. @cap/validator — Email & URL validation", () => {
        expect_eq(isEmail("alice@capdux.org"), true);
        expect_eq(isEmail("dev.ops+ci@sub.capdux.dev"), true);
        expect_eq(isEmail("bad..dot@capdux.org"), false);
        expect_eq(isEmail("@capdux.org"), false);
        expect_eq(isEmail("alice@localhost"), false);
        expect_eq(isEmail("alice@domain.123"), false);

        expect_eq(isUrl("https://capdux.org/packages/@cap/lru"), true);
        expect_eq(isUrl("http://localhost:8080/health?ok=1"), true);
        expect_eq(isUrl("wss://[::1]:9000/ws"), true);
        expect_eq(isUrl("ftp://capdux.org"), false);
        expect_eq(isUrl("https://capdux.org:99999/badport"), false);
        expect_eq(isUrl("https://bad host.com"), false);
    });

    test("2. @cap/validator — IPv4, IPv6, CIDR, UUID & ULID", () => {
        expect_eq(isIpv4("192.168.1.1"), true);
        expect_eq(isIpv4("0.0.0.0"), true);
        expect_eq(isIpv4("256.0.0.1"), false);
        expect_eq(isIpv4("01.2.3.4"), false);

        expect_eq(isIpv6("2001:0db8:85a3:0000:0000:8a2e:0370:7334"), true);
        expect_eq(isIpv6("::1"), true);
        expect_eq(isIpv6("::"), true);
        expect_eq(isIpv6("fe80::1"), true);
        expect_eq(isIpv6("2001::db8::1"), false);
        expect_eq(isIpv6("gggg::1"), false);

        expect_eq(isCidr("10.0.0.0/8"), true);
        expect_eq(isCidr("10.0.0.0/33"), false);
        expect_eq(isCidr("2001:db8::/32"), true);
        expect_eq(isCidr("2001:db8::/129"), false);

        expect_eq(isUuid("550e8400-e29b-41d4-a716-446655440000"), true);
        expect_eq(isUuid("550e8400-e29b-41d4-a716-44665544000z"), false);
        expect_eq(isUlid("01ARZ3NDEKTSV4RRFFQ69G5FAV"), true);
        expect_eq(isUlid("81ARZ3NDEKTSV4RRFFQ69G5FAV"), false); // overflow first char
        expect_eq(isUlid("01ARZ3NDEKTSV4RRFFQ69G5FAI"), false); // 'I' disallowed
    });

    test("3. @cap/validator — SemVer, ISO-8601 Date/DateTime, Luhn & E.164", () => {
        expect_eq(isSemver("1.0.0"), true);
        expect_eq(isSemver("2.13.3-rc.1+build.42"), true);
        expect_eq(isSemver("01.0.0"), false);
        expect_eq(isSemver("1.2"), false);

        expect_eq(isIsoDate("2024-02-29"), true);  // leap year
        expect_eq(isIsoDate("2025-02-29"), false); // non-leap year
        expect_eq(isIsoDate("2026-11-31"), false); // Nov has 30 days
        expect_eq(isIsoDateTime("2026-11-26T14:05:30Z"), true);
        expect_eq(isIsoDateTime("2026-11-26T14:05:30.123+11:00"), true);
        expect_eq(isIsoDateTime("2026-11-26T25:00:00Z"), false);

        expect_eq(isLuhn("4532 0151 1283 0366"), true);
        expect_eq(isLuhn("4532-0151-1283-0367"), false);
        expect_eq(isE164Phone("+61412345678"), true);
        expect_eq(isE164Phone("+012345678"), false);
        expect_eq(isE164Phone("13800138000"), false);
    });

    test("4. @cap/validator — String & Numeric Helpers", () => {
        expect_eq(isAscii("hello-123!"), true);
        expect_eq(isAlphanumeric("Capdux2026"), true);
        expect_eq(isAlphanumeric("Capdux-2026"), false);
        expect_eq(isNumeric("-42.5"), true);
        expect_eq(isNumeric("12.3.4"), false);
        expect_eq(isHex("0xdeadBEEF"), true);
        expect_eq(isBase64("SGVsbG8="), true);
        expect_eq(isBase64("not!base64"), false);
        expect_eq(isSlug("my-agent-v2"), true);
        expect_eq(isSlug("My--Agent"), false);
        expect_eq(hasLenBetween("capdux", 3 as usize, 10 as usize), true);
        expect_eq(startsWith("capdux.org", "capdux"), true);
        expect_eq(endsWith("capdux.org", ".org"), true);
        expect_eq(containsStr("hello capdux world", "capdux"), true);

        const roles: [3]string = ["free", "pro", "enterprise"];
        expect_eq(isOneOf("pro", roles[0..3]), true);
        expect_eq(isOneOf("vip", roles[0..3]), false);

        expect_eq(isPort(443), true);
        expect_eq(isPort(0), false);
        expect_eq(inRangeI64(50, 1, 100), true);
        expect_eq(isMultipleOf(15, 5), true);
    });

    test("5. @cap/validator — Structured Multi-Error Collector", () => {
        let ok = Validator.init();
        expect_eq(ok.required("name", "cap-agent"), true);
        expect_eq(ok.slug("name", "cap-agent"), true);
        expect_eq(ok.email("email", "ops@capdux.com"), true);
        expect_eq(ok.url("endpoint", "https://agent.capdux.app"), true);
        expect_eq(ok.semver("ver", "1.0.0"), true);
        expect_eq(ok.port("port", 8080), true);
        expect_eq(ok.isValid(), true);
        expect_eq(ok.errorCount(), 0 as usize);

        let bad = Validator.init();
        expect_eq(bad.required("name", ""), false);
        expect_eq(bad.email("email", "not-an-email"), false);
        expect_eq(bad.port("port", 70000), false);
        expect_eq(bad.isValid(), false);
        expect_eq(bad.errorCount(), 3 as usize);
        expect_eq(bad.errorAt(0 as usize).field, "name");
        expect_eq(bad.errorAt(1 as usize).code, "invalid_email");
        expect_eq(bad.errorAt(2 as usize).code, "invalid_port");
    });
}
