import { test, expect_eq } from "cap:test";
import {
    splitToken,
    decodeBase64Url,
    decodeHeader,
    decodePayload,
    isExpired,
    isNotYetValid,
    isAlgNone,
} from "../../packages/@cap/jwt-parse/src/index.cp";

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

// Canonical RFC 7519 example token:
// header:  {"alg":"HS256","typ":"JWT"}
// payload: {"sub":"1234567890","name":"John Doe","iat":1516239022}
const SAMPLE_JWT: string = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c";

fn main() {
    test("1. @cap/jwt-parse — splitToken 3-segment split & zero-copy signing_input", () => {
        const p = splitToken(SAMPLE_JWT);
        expect_eq(p.valid, true);
        expect_eq(testEq(p.header_b64, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"), true);
        expect_eq(testEq(p.payload_b64, "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ"), true);
        expect_eq(testEq(p.signature_b64, "SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c"), true);
        expect_eq(testEq(p.signing_input, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ"), true);
    });

    test("2. @cap/jwt-parse — decodeHeader extracts {\"alg\":\"HS256\",\"typ\":\"JWT\"}", () => {
        let buf: [128]u8 = undefined;
        const n = decodeHeader(SAMPLE_JWT, buf[0..128]);
        const expected = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
        expect_eq(n, expected.len as i64);
        expect_eq(testEq(buf[0..n as usize], expected), true);
    });

    test("3. @cap/jwt-parse — decodePayload extracts full claims JSON", () => {
        let buf: [128]u8 = undefined;
        const n = decodePayload(SAMPLE_JWT, buf[0..128]);
        const expected = "{\"sub\":\"1234567890\",\"name\":\"John Doe\",\"iat\":1516239022}";
        expect_eq(n, expected.len as i64);
        expect_eq(testEq(buf[0..n as usize], expected), true);
    });

    test("4. @cap/jwt-parse — isExpired & isNotYetValid with clock-skew leeway", () => {
        expect_eq(isExpired(1000, 999, 0), false);
        expect_eq(isExpired(1000, 1000, 0), true);
        expect_eq(isExpired(1000, 1020, 30), false);
        expect_eq(isExpired(1000, 1030, 30), true);

        expect_eq(isNotYetValid(2000, 1999, 0), true);
        expect_eq(isNotYetValid(2000, 2000, 0), false);
        expect_eq(isNotYetValid(2000, 1980, 30), false);
        expect_eq(isNotYetValid(2000, 1969, 30), true);
    });

    test("5. @cap/jwt-parse — isAlgNone detects case-insensitive unsecured none algorithm", () => {
        expect_eq(isAlgNone("{\"alg\":\"none\",\"typ\":\"JWT\"}"), true);
        expect_eq(isAlgNone("{\"typ\":\"JWT\", \"alg\" : \"None\"}"), true);
        expect_eq(isAlgNone("{\"alg\":\"NONE\"}"), true);
        expect_eq(isAlgNone("{\"alg\":\"HS256\",\"typ\":\"JWT\"}"), false);
        expect_eq(isAlgNone("{\"alg\":\"RS256\"}"), false);
    });

    test("6. @cap/jwt-parse — Rejects malformed segments, non-Base64URL & undersized buffers", () => {
        expect_eq(splitToken("only.two").valid, false);
        expect_eq(splitToken("a.b.c.d").valid, false);
        expect_eq(splitToken(".b.c").valid, false);
        expect_eq(splitToken("a..c").valid, false);
        expect_eq(splitToken("a+b.c.d").valid, false);

        let buf: [64]u8 = undefined;
        // Invalid base64url length (rem == 1) and illegal char
        expect_eq(decodeBase64Url("a", buf[0..64]), -1 as i64);
        expect_eq(decodeBase64Url("ab+c", buf[0..64]), -1 as i64);

        let tiny: [4]u8 = undefined;
        expect_eq(decodeHeader(SAMPLE_JWT, tiny[0..4]), -1 as i64);
        expect_eq(decodePayload(SAMPLE_JWT, tiny[0..4]), -1 as i64);
    });
}
