import { test, expect_eq } from "cap:test";
import {
    DigestChallenge,
    DigestResponse,
    parseChallenge,
    parseAuthorization,
    supportsQop,
    formatAuthorization,
    formatChallenge,
} from "../../packages/http-digest-client/src/index.cp";

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
    test("1. @cap/digest-auth — parseChallenge full fields with SHA-256 & stale=true", () => {
        const hdr = "Digest realm=\"http-auth@example.org\", qop=\"auth, auth-int\", algorithm=SHA-256, nonce=\"7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v\", opaque=\"FQhe/qaU925kfnzjCev0ciny7QMkPqMAFRtzCUYo5tdS\", stale=true";
        const ch = parseChallenge(hdr);
        expect_eq(ch.valid, true);
        expect_eq(testEq(ch.realm, "http-auth@example.org"), true);
        expect_eq(testEq(ch.qop, "auth, auth-int"), true);
        expect_eq(testEq(ch.algorithm, "SHA-256"), true);
        expect_eq(testEq(ch.nonce, "7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v"), true);
        expect_eq(testEq(ch.opaque_val, "FQhe/qaU925kfnzjCev0ciny7QMkPqMAFRtzCUYo5tdS"), true);
        expect_eq(ch.stale, true);
    });

    test("2. @cap/digest-auth — parseAuthorization required & optional fields", () => {
        const hdr = "digest username=\"Mufasa\", realm=\"http-auth@example.org\", uri=\"/dir/index.html\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", nc=00000001, cnonce=\"0a4f113b\", qop=auth, response=\"6629fae49393a05397450978507c4ef1\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
        const r = parseAuthorization(hdr);
        expect_eq(r.valid, true);
        expect_eq(testEq(r.username, "Mufasa"), true);
        expect_eq(testEq(r.realm, "http-auth@example.org"), true);
        expect_eq(testEq(r.uri, "/dir/index.html"), true);
        expect_eq(testEq(r.nc, "00000001"), true);
        expect_eq(testEq(r.cnonce, "0a4f113b"), true);
        expect_eq(testEq(r.qop, "auth"), true);
        expect_eq(testEq(r.response, "6629fae49393a05397450978507c4ef1"), true);
        expect_eq(testEq(r.opaque_val, "5ccc069c403ebaf9f0171e9517f40e41"), true);
    });

    test("3. @cap/digest-auth — supportsQop multi-item matching", () => {
        expect_eq(supportsQop("auth, auth-int", "auth"), true);
        expect_eq(supportsQop("auth, auth-int", "auth-int"), true);
        expect_eq(supportsQop("auth, auth-int", "AUTH-INT"), true);
        expect_eq(supportsQop("auth", "auth-int"), false);
        expect_eq(supportsQop("", "auth"), false);
    });

    test("4. @cap/digest-auth — formatAuthorization -> parseAuthorization roundtrip", () => {
        const orig = DigestResponse {
            username: "alice",
            realm: "api.cap.dev",
            nonce: "nonce123",
            uri: "/v1/me",
            qop: "auth",
            nc: "00000002",
            cnonce: "cn456",
            response: "abcdef0123456789",
            opaque_val: "op789",
            valid: true,
        };
        let buf: [512]u8 = undefined;
        const n = formatAuthorization(orig, buf[0..512]);
        expect_eq(n > 0, true);

        const reparsed = parseAuthorization(buf[0..n]);
        expect_eq(reparsed.valid, true);
        expect_eq(testEq(reparsed.username, "alice"), true);
        expect_eq(testEq(reparsed.realm, "api.cap.dev"), true);
        expect_eq(testEq(reparsed.nonce, "nonce123"), true);
        expect_eq(testEq(reparsed.uri, "/v1/me"), true);
        expect_eq(testEq(reparsed.qop, "auth"), true);
        expect_eq(testEq(reparsed.nc, "00000002"), true);
        expect_eq(testEq(reparsed.cnonce, "cn456"), true);
        expect_eq(testEq(reparsed.response, "abcdef0123456789"), true);
        expect_eq(testEq(reparsed.opaque_val, "op789"), true);
    });

    test("5. @cap/digest-auth — formatChallenge -> parseChallenge roundtrip", () => {
        const orig = DigestChallenge {
            realm: "Protected, Area",
            nonce: "xyz987",
            opaque_val: "op1",
            qop: "auth,auth-int",
            algorithm: "SHA-256",
            stale: true,
            valid: true,
        };
        let buf: [256]u8 = undefined;
        const n = formatChallenge(orig, buf[0..256]);
        expect_eq(n > 0, true);

        const reparsed = parseChallenge(buf[0..n]);
        expect_eq(reparsed.valid, true);
        expect_eq(testEq(reparsed.realm, "Protected, Area"), true);
        expect_eq(testEq(reparsed.nonce, "xyz987"), true);
        expect_eq(testEq(reparsed.algorithm, "SHA-256"), true);
        expect_eq(reparsed.stale, true);
    });

    test("6. @cap/digest-auth — Rejects non-Digest, missing required fields & undersized buffer", () => {
        expect_eq(parseChallenge("Basic realm=\"x\"").valid, false);
        expect_eq(parseChallenge("Digest realm=\"only-realm\"").valid, false);
        expect_eq(parseAuthorization("Bearer tok").valid, false);
        expect_eq(parseAuthorization("Digest username=\"a\", realm=\"b\"").valid, false);

        const ch = DigestChallenge {
            realm: "api",
            nonce: "12345",
            opaque_val: "",
            qop: "",
            algorithm: "",
            stale: false,
            valid: true,
        };
        let tiny: [10]u8 = undefined;
        expect_eq(formatChallenge(ch, tiny[0..10]), 0 as usize);
    });
}
