// cap/test/testCapAjvFormats.cp
// Comprehensive unit tests for @cap/ajv-formats

import { test, expect_eq } from "cap:test";
import {
    isDate,
    isTime,
    isDateTime,
    isEmail,
    isHostname,
    isIpv4,
    isIpv6,
    isUuid,
    isUri,
    isJsonPointer,
    validateFormat,
} from "../../packages/ajv-formats/src/index.cp";

fn main() {
    test("1. @cap/ajv-formats — isDate RFC 3339 & leap year validation", () => {
        expect_eq(isDate("2024-02-29"), true);
        expect_eq(isDate("2000-02-29"), true);
        expect_eq(isDate("1900-02-29"), false);
        expect_eq(isDate("2023-02-29"), false);
        expect_eq(isDate("2026-04-31"), false);
        expect_eq(isDate("2026-13-01"), false);
        expect_eq(isDate("2026-11-27"), true);
    });

    test("2. @cap/ajv-formats — isTime & isDateTime RFC 3339 with leap second", () => {
        expect_eq(isTime("12:34:56Z"), true);
        expect_eq(isTime("12:34:56.789+08:00"), true);
        expect_eq(isTime("23:59:60Z"), true);
        expect_eq(isTime("22:59:60-01:00"), true);
        expect_eq(isTime("12:00:60Z"), false);
        expect_eq(isTime("12:34:56"), false);

        expect_eq(isDateTime("2026-11-27T09:30:00Z"), true);
        expect_eq(isDateTime("2024-02-29 23:59:59.5-05:00"), true);
        expect_eq(isDateTime("2023-02-29T10:00:00Z"), false);
    });

    test("3. @cap/ajv-formats — isEmail & isHostname RFC 1034/1123", () => {
        expect_eq(isHostname("example.com"), true);
        expect_eq(isHostname("sub-1.example.co.uk."), true);
        expect_eq(isHostname("-bad.com"), false);
        expect_eq(isHostname("bad-.com"), false);
        expect_eq(isHostname("a..b"), false);

        expect_eq(isEmail("alice.bob+tag@example.com"), true);
        expect_eq(isEmail(".alice@example.com"), false);
        expect_eq(isEmail("a..b@example.com"), false);
        expect_eq(isEmail("alice@-example.com"), false);
    });

    test("4. @cap/ajv-formats — isIpv4 & isIpv6 with :: compression & embedded IPv4", () => {
        expect_eq(isIpv4("192.168.0.1"), true);
        expect_eq(isIpv4("0.0.0.0"), true);
        expect_eq(isIpv4("256.0.0.1"), false);
        expect_eq(isIpv4("01.2.3.4"), false);

        expect_eq(isIpv6("::1"), true);
        expect_eq(isIpv6("::"), true);
        expect_eq(isIpv6("2001:0db8:85a3:0000:0000:8a2e:0370:7334"), true);
        expect_eq(isIpv6("::ffff:192.0.2.128"), true);
        expect_eq(isIpv6("2001::db8::1"), false);
        expect_eq(isIpv6("2001:db8:::1"), false);
    });

    test("5. @cap/ajv-formats — isUuid, isUri, and isJsonPointer", () => {
        expect_eq(isUuid("550e8400-e29b-41d4-a716-446655440000"), true);
        expect_eq(isUuid("550e8400-e29b-41d4-a716-44665544000g"), false);
        expect_eq(isUuid("550e8400e29b41d4a716446655440000"), false);

        expect_eq(isUri("https://example.com:8080/a%20b?x=1#top"), true);
        expect_eq(isUri("mailto:user@example.com"), true);
        expect_eq(isUri("/relative/path"), false);
        expect_eq(isUri("http://example.com/%ZZ"), false);

        expect_eq(isJsonPointer(""), true);
        expect_eq(isJsonPointer("/foo/0/a~1b/m~0n"), true);
        expect_eq(isJsonPointer("foo/bar"), false);
        expect_eq(isJsonPointer("/bad~2escape"), false);
    });

    test("6. @cap/ajv-formats — validateFormat unified dispatcher", () => {
        expect_eq(validateFormat("date", "2024-02-29"), true);
        expect_eq(validateFormat("time", "08:15:00+02:00"), true);
        expect_eq(validateFormat("date-time", "2026-11-27T12:00:00Z"), true);
        expect_eq(validateFormat("email", "dev@cp-lang.org"), true);
        expect_eq(validateFormat("hostname", "api.cp-lang.org"), true);
        expect_eq(validateFormat("ipv4", "127.0.0.1"), true);
        expect_eq(validateFormat("ipv6", "fe80::1"), true);
        expect_eq(validateFormat("uuid", "123e4567-e89b-12d3-a456-426614174000"), true);
        expect_eq(validateFormat("uri", "https://cp-lang.org/docs"), true);
        expect_eq(validateFormat("json-pointer", "/a/b/0"), true);
        expect_eq(validateFormat("unknown-format", "anything"), false);
    });
}
