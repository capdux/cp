import { test, expect_eq } from "cap:test";
import {
    isValidTokenChars,
    extractBearer,
    formatBearer,
    isJwtLike,
    constantTimeEq,
    verifyBearer,
    formatChallenge,
} from "../../packages/express-bearer-token/src/index.cp";

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
    test("1. @cap/bearer-auth — Case-insensitive Bearer/bearer extraction & extra whitespace", () => {
        expect_eq(testEq(extractBearer("Bearer mF_9.B5f-4.1JqM"), "mF_9.B5f-4.1JqM"), true);
        expect_eq(testEq(extractBearer("  bearer   abc123+/_~==  "), "abc123+/_~=="), true);
        expect_eq(testEq(extractBearer("BEARER\ttok_99"), "tok_99"), true);
    });

    test("2. @cap/bearer-auth — Rejects invalid chars, wrong scheme & empty tokens", () => {
        expect_eq(isValidTokenChars("valid-Token_1.2~+/="), true);
        expect_eq(isValidTokenChars(""), false);
        expect_eq(isValidTokenChars("has space"), false);
        expect_eq(isValidTokenChars("bad=mid=pad"), false);

        expect_eq(testEq(extractBearer("Basic dXNlcjpwYXNz"), ""), true);
        expect_eq(testEq(extractBearer("Bearer "), ""), true);
        expect_eq(testEq(extractBearer("Bearer bad token"), ""), true);
        expect_eq(testEq(extractBearer("BearerOnly"), ""), true);
    });

    test("3. @cap/bearer-auth — formatBearer roundtrip & buffer guard", () => {
        let buf: [64]u8 = undefined;
        const n = formatBearer("eyJhbGciOiJIUzI1NiJ9.e30.sig", buf[0..64]);
        expect_eq(n > 0, true);
        expect_eq(testEq(extractBearer(buf[0..n]), "eyJhbGciOiJIUzI1NiJ9.e30.sig"), true);

        let tiny: [8]u8 = undefined;
        expect_eq(formatBearer("longtoken123", tiny[0..8]), 0 as usize);
        expect_eq(formatBearer("bad token", buf[0..64]), 0 as usize);
    });

    test("4. @cap/bearer-auth — isJwtLike 3-segment base64url check", () => {
        expect_eq(isJwtLike("eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxMjM0In0.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c"), true);
        expect_eq(isJwtLike("a.b.c"), true);
        expect_eq(isJwtLike("only.two"), false);
        expect_eq(isJwtLike("a.b.c.d"), false);
        expect_eq(isJwtLike("a..c"), false);
        expect_eq(isJwtLike(".b.c"), false);
        expect_eq(isJwtLike("a.b+c.d"), false);
    });

    test("5. @cap/bearer-auth — constantTimeEq & verifyBearer", () => {
        expect_eq(constantTimeEq("secret-token-42", "secret-token-42"), true);
        expect_eq(constantTimeEq("secret-token-42", "secret-token-43"), false);
        expect_eq(constantTimeEq("short", "longer"), false);

        expect_eq(verifyBearer("Bearer secret-token-42", "secret-token-42"), true);
        expect_eq(verifyBearer("bearer secret-token-42", "secret-token-42"), true);
        expect_eq(verifyBearer("Bearer wrong-token-42", "secret-token-42"), false);
        expect_eq(verifyBearer("Basic secret-token-42", "secret-token-42"), false);
    });

    test("6. @cap/bearer-auth — formatChallenge full header & quote escaping", () => {
        let buf: [256]u8 = undefined;
        const n1 = formatChallenge("api", "invalid_token", "Token \"expired\" \\ revoked", buf[0..256]);
        const expected = "Bearer realm=\"api\", error=\"invalid_token\", error_description=\"Token \\\"expired\\\" \\\\ revoked\"";
        expect_eq(n1, expected.len);
        expect_eq(testEq(buf[0..n1], expected), true);

        const n2 = formatChallenge("", "", "", buf[0..256]);
        expect_eq(testEq(buf[0..n2], "Bearer"), true);

        let tiny: [5]u8 = undefined;
        expect_eq(formatChallenge("api", "invalid_token", "", tiny[0..5]), 0 as usize);
    });
}
