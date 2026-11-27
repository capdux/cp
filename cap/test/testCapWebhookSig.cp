import { test, expect_eq } from "cap:test";
import {
    parseStripeHeader,
    parseGitHubSig256,
    parseStandardWebhookSig,
    isWithinTolerance,
    constantTimeEq,
    verifyStripeExpected,
    formatStripeSignedPayload,
} from "../../packages/standardwebhooks/src/index.cp";

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
    test("1. @cap/webhook-sig — Stripe single & multi-v1 rotated signature parsing", () => {
        const hdr = "t=1492774577,v1=5257a869e7ecebeda32affa62cdca3fa51cad7e77a0e56ff536d0ce8e108d8bd,v1=6ffbb59b2300aae63f272406069a9788598b792a944a07aba816edb039989a39,v0=ignored";
        const parsed = parseStripeHeader(hdr);
        expect_eq(parsed.valid, true);
        expect_eq(parsed.timestamp, 1492774577 as i64);
        expect_eq(parsed.v1_count, 2 as usize);
        expect_eq(testEq(parsed.v1_sigs[0], "5257a869e7ecebeda32affa62cdca3fa51cad7e77a0e56ff536d0ce8e108d8bd"), true);
        expect_eq(testEq(parsed.v1_sigs[1], "6ffbb59b2300aae63f272406069a9788598b792a944a07aba816edb039989a39"), true);

        expect_eq(parseStripeHeader("t=1492774577").valid, false);
        expect_eq(parseStripeHeader("v1=abcdef").valid, false);
    });

    test("2. @cap/webhook-sig — verifyStripeExpected success, expired replay & tampered rejection", () => {
        const hdr = "t=1700000000,v1=deadbeef01,v1=cafebabe02";
        // Valid rotated second signature within 300s window
        expect_eq(verifyStripeExpected(hdr, "cafebabe02", 1700000120, 300), true);
        // Future clock skew within tolerance
        expect_eq(isWithinTolerance(1700000050, 1700000000, 300), true);
        // Expired replay (> 300s) rejected
        expect_eq(verifyStripeExpected(hdr, "cafebabe02", 1700000500, 300), false);
        // Tampered signature rejected
        expect_eq(verifyStripeExpected(hdr, "cafebabe03", 1700000100, 300), false);
    });

    test("3. @cap/webhook-sig — GitHub X-Hub-Signature-256 valid & malformed length rejection", () => {
        const digest = "757107ea0eb2509fc211221cce984b8a37570b6d7586c22c46f4379c8b043e17";
        const hdr = "sha256=757107ea0eb2509fc211221cce984b8a37570b6d7586c22c46f4379c8b043e17";
        expect_eq(testEq(parseGitHubSig256(hdr), digest), true);

        // Wrong prefix, too short, non-hex char
        expect_eq(testEq(parseGitHubSig256("sha1=757107ea0eb2509fc211221cce984b8a37570b6d7586c22c46f4379c8b043e17"), ""), true);
        expect_eq(testEq(parseGitHubSig256("sha256=abcd"), ""), true);
        expect_eq(testEq(parseGitHubSig256("sha256=zzzz07ea0eb2509fc211221cce984b8a37570b6d7586c22c46f4379c8b043e17"), ""), true);
    });

    test("4. @cap/webhook-sig — Standard Webhooks multi-version v1,<sig> extraction", () => {
        const hdr = "v1,g0hM9SsE+OTPJTGt/tmIKtSyZlE3uFJELVlNIOLJ1OE= v2,ignoredSig v1,K5oZfzN95Z9UVu1EsfQmfVNQhnkZ2pj9o9NDN/H/pI4=";
        let out: [4]string = ["", "", "", ""];
        const count = parseStandardWebhookSig(hdr, out[0..4]);
        expect_eq(count, 2 as usize);
        expect_eq(testEq(out[0], "g0hM9SsE+OTPJTGt/tmIKtSyZlE3uFJELVlNIOLJ1OE="), true);
        expect_eq(testEq(out[1], "K5oZfzN95Z9UVu1EsfQmfVNQhnkZ2pj9o9NDN/H/pI4="), true);
    });

    test("5. @cap/webhook-sig — formatStripeSignedPayload assembly", () => {
        let buf: [128]u8 = undefined;
        const n = formatStripeSignedPayload(1492774577, "{\"id\":\"evt_1\"}", buf[0..128]);
        const expected = "1492774577.{\"id\":\"evt_1\"}";
        expect_eq(n, expected.len);
        expect_eq(testEq(buf[0..n], expected), true);
    });

    test("6. @cap/webhook-sig — constantTimeEq & undersized buffer guard", () => {
        expect_eq(constantTimeEq("abcdef012345", "abcdef012345"), true);
        expect_eq(constantTimeEq("abcdef012345", "abcdef012346"), false);
        expect_eq(constantTimeEq("abc", "abcd"), false);

        let tiny: [8]u8 = undefined;
        expect_eq(formatStripeSignedPayload(1492774577, "{\"a\":1}", tiny[0..8]), 0 as usize);
        expect_eq(formatStripeSignedPayload(-1, "payload", tiny[0..8]), 0 as usize);
    });
}
