import { test, expect_eq } from "cap:test";
import { toIdentifier, toCamelIdentifier, toConstIdentifier, isValidIdentifier } from "../../packages/toidentifier/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/toidentifier — Converts HTTP status phrases to PascalCase", () => {
        let buf: [64]u8 = undefined;
        let n1 = toIdentifier("Bad Request", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "BadRequest"), true);

        let n2 = toIdentifier("Internal Server Error", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "InternalServerError"), true);
    });

    test("2. @cap/toidentifier — Strips apostrophes and punctuation", () => {
        let buf: [64]u8 = undefined;
        let n1 = toIdentifier("I'm a teapot", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "ImATeapot"), true);

        let n2 = toIdentifier("Non-Authoritative Information", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "NonAuthoritativeInformation"), true);
    });

    test("3. @cap/toidentifier — Collapses spaces and preserves inner casing", () => {
        let buf: [64]u8 = undefined;
        let n1 = toIdentifier("  URI   Too   Long  ", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "URITooLong"), true);

        let n2 = toIdentifier("HTTP Version Not Supported", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "HTTPVersionNotSupported"), true);
    });

    test("4. @cap/toidentifier — toCamelIdentifier produces lowerCamelCase", () => {
        let buf: [64]u8 = undefined;
        let n1 = toCamelIdentifier("Bad Gateway", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "badGateway"), true);

        let n2 = toCamelIdentifier("", buf[0..64]);
        expect_eq(n2, 0);
    });

    test("5. @cap/toidentifier — toConstIdentifier produces SCREAMING_SNAKE_CASE", () => {
        let buf: [64]u8 = undefined;
        let n1 = toConstIdentifier("Too Many Requests", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "TOO_MANY_REQUESTS"), true);

        let n2 = toConstIdentifier("non-authoritative info", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "NON_AUTHORITATIVE_INFO"), true);
    });

    test("6. @cap/toidentifier — isValidIdentifier validates ASCII identifiers", () => {
        expect_eq(isValidIdentifier("BadRequest"), true);
        expect_eq(isValidIdentifier("_private_1"), true);
        expect_eq(isValidIdentifier("404NotFound"), false);
        expect_eq(isValidIdentifier("bad-request"), false);
        expect_eq(isValidIdentifier(""), false);
    });
}
