// cap/test/testCapBasicAuth.cp
// Comprehensive unit tests for @cap/basic-auth (RFC 7617 HTTP Basic Auth)

import { test, expect_eq } from "cap:test";
import {
    BasicCredentials,
    decodeHeader,
    encodeHeader,
    constantTimeEq,
    verifyHeader,
    formatChallenge,
} from "../../packages/basic-auth/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/basic-auth — decode standard 'Basic dXNlcjpwYXNz'", () => {
        let u: [64]u8 = undefined;
        let p: [64]u8 = undefined;
        const c: BasicCredentials = decodeHeader("Basic dXNlcjpwYXNz", u[0..64], p[0..64]);

        expect_eq(c.valid, true);
        expect_eq(sliceEq(u[0..c.user_len], "user"), true);
        expect_eq(sliceEq(p[0..c.pass_len], "pass"), true);

        // Case-insensitive scheme prefix
        const c2: BasicCredentials = decodeHeader("  basic  dXNlcjpwYXNz  ", u[0..64], p[0..64]);
        expect_eq(c2.valid, true);
        expect_eq(sliceEq(u[0..c2.user_len], "user"), true);
    });

    test("2. @cap/basic-auth — password containing multiple colons & empty password", () => {
        let hdr: [128]u8 = undefined;
        let u: [64]u8 = undefined;
        let p: [64]u8 = undefined;

        const n1: usize = encodeHeader("admin", "p:a:s:s", hdr[0..128]);
        expect_eq(n1 > 0, true);
        const c1: BasicCredentials = decodeHeader(hdr[0..n1], u[0..64], p[0..64]);
        expect_eq(c1.valid, true);
        expect_eq(sliceEq(u[0..c1.user_len], "admin"), true);
        expect_eq(sliceEq(p[0..c1.pass_len], "p:a:s:s"), true);

        const n2: usize = encodeHeader("token_only", "", hdr[0..128]);
        expect_eq(n2 > 0, true);
        const c2: BasicCredentials = decodeHeader(hdr[0..n2], u[0..64], p[0..64]);
        expect_eq(c2.valid, true);
        expect_eq(sliceEq(u[0..c2.user_len], "token_only"), true);
        expect_eq(c2.pass_len, 0 as usize);
    });

    test("3. @cap/basic-auth — encodeHeader/decodeHeader round-trip & rejects colon in user", () => {
        let hdr: [128]u8 = undefined;
        const n: usize = encodeHeader("user", "pass", hdr[0..128]);
        expect_eq(sliceEq(hdr[0..n], "Basic dXNlcjpwYXNz"), true);

        // Reject ':' inside userid
        expect_eq(encodeHeader("bad:user", "pass", hdr[0..128]), 0 as usize);
    });

    test("4. @cap/basic-auth — verifyHeader constant-time verification", () => {
        expect_eq(constantTimeEq("secret", "secret"), true);
        expect_eq(constantTimeEq("secret", "secreX"), false);
        expect_eq(constantTimeEq("secret", "sec"), false);

        expect_eq(verifyHeader("Basic dXNlcjpwYXNz", "user", "pass"), true);
        expect_eq(verifyHeader("Basic dXNlcjpwYXNz", "user", "wrong"), false);
        expect_eq(verifyHeader("Basic dXNlcjpwYXNz", "wrong", "pass"), false);
        expect_eq(verifyHeader("Bearer xyz", "user", "pass"), false);
    });

    test("5. @cap/basic-auth — formatChallenge with escaped quotes & UTF-8 charset", () => {
        let out: [128]u8 = undefined;
        const n1: usize = formatChallenge("Admin \"Zone\" \\1", true, out[0..128]);
        expect_eq(
            sliceEq(out[0..n1], "Basic realm=\"Admin \\\"Zone\\\" \\\\1\", charset=\"UTF-8\""),
            true,
        );

        const n2: usize = formatChallenge("API", false, out[0..128]);
        expect_eq(sliceEq(out[0..n2], "Basic realm=\"API\""), true);
    });

    test("6. @cap/basic-auth — rejects non-Basic, invalid Base64, missing colon & tiny buffers", () => {
        let u: [64]u8 = undefined;
        let p: [64]u8 = undefined;

        expect_eq(decodeHeader("Bearer dXNlcjpwYXNz", u[0..64], p[0..64]).valid, false);
        expect_eq(decodeHeader("Basic !!!invalid!!!", u[0..64], p[0..64]).valid, false);
        // "dXNlcg==" decodes to "user" (no colon)
        expect_eq(decodeHeader("Basic dXNlcg==", u[0..64], p[0..64]).valid, false);
        expect_eq(decodeHeader("Basic ", u[0..64], p[0..64]).valid, false);

        let tiny: [4]u8 = undefined;
        expect_eq(encodeHeader("user", "pass", tiny[0..4]), 0 as usize);
        expect_eq(formatChallenge("Realm", true, tiny[0..4]), 0 as usize);
    });
}
