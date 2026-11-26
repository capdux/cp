import { test, expect_eq } from "cap:test";
import {
    Urn,
    isValidNid,
    parseUrn,
    urnEquivalent,
    formatUrn,
} from "../../packages/@cap/urn-parse/src/index.cp";

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
    test("1. @cap/urn-parse — Standard urn:isbn: & urn:uuid: parsing", () => {
        const u1 = parseUrn("urn:isbn:0451450523");
        expect_eq(u1.valid, true);
        expect_eq(testEq(u1.nid, "isbn"), true);
        expect_eq(testEq(u1.nss, "0451450523"), true);

        const u2 = parseUrn("URN:uuid:6e8bc430-9c3a-11d9-9669-0800200c9a66");
        expect_eq(u2.valid, true);
        expect_eq(testEq(u2.nid, "uuid"), true);
        expect_eq(testEq(u2.nss, "6e8bc430-9c3a-11d9-9669-0800200c9a66"), true);
    });

    test("2. @cap/urn-parse — Full RFC 8141 URN with ?+r, ?=q & #f components", () => {
        const u = parseUrn("urn:example:a123,z456?+resolver=fast?=page=2&lang=en#sec3");
        expect_eq(u.valid, true);
        expect_eq(testEq(u.nid, "example"), true);
        expect_eq(testEq(u.nss, "a123,z456"), true);
        expect_eq(testEq(u.r_component, "resolver=fast"), true);
        expect_eq(testEq(u.q_component, "page=2&lang=en"), true);
        expect_eq(testEq(u.f_component, "sec3"), true);
    });

    test("3. @cap/urn-parse — urnEquivalent ignores NID case & r/q/f components", () => {
        const a = parseUrn("URN:IETF:rfc:8141?+r1?=q1#f1");
        const b = parseUrn("urn:ietf:rfc:8141#other");
        const c = parseUrn("urn:ietf:rfc:8142");

        expect_eq(urnEquivalent(a, b), true);
        expect_eq(urnEquivalent(a, c), false);
    });

    test("4. @cap/urn-parse — formatUrn canonical lowercase roundtrip", () => {
        const u = parseUrn("URN:My-NID:item/42?+r=1?=q=2#top");
        expect_eq(u.valid, true);

        let buf: [128]u8 = undefined;
        const n = formatUrn(u, buf[0..128]);
        const expected = "urn:my-nid:item/42?+r=1?=q=2#top";
        expect_eq(n, expected.len);
        expect_eq(testEq(buf[0..n], expected), true);

        const reparsed = parseUrn(buf[0..n]);
        expect_eq(urnEquivalent(u, reparsed), true);
    });

    test("5. @cap/urn-parse — isValidNid length & hyphen boundary checks", () => {
        expect_eq(isValidNid("ab"), true);
        expect_eq(isValidNid("my-ns-123"), true);
        expect_eq(isValidNid("a123456789012345678901234567890b"), true);  // 32 chars
        expect_eq(isValidNid("a1234567890123456789012345678901c"), false); // 33 chars
        expect_eq(isValidNid("a"), false);
        expect_eq(isValidNid("-abc"), false);
        expect_eq(isValidNid("abc-"), false);
        expect_eq(isValidNid("ab_cd"), false);
    });

    test("6. @cap/urn-parse — Rejects bad scheme, single-char NID, empty NSS & undersized buffer", () => {
        expect_eq(parseUrn("http:isbn:123").valid, false);
        expect_eq(parseUrn("urn:a:123").valid, false);
        expect_eq(parseUrn("urn:isbn:").valid, false);
        expect_eq(parseUrn("urn:isbn:?+r").valid, false);
        expect_eq(parseUrn("urn:isbn:123?+").valid, false);

        const ok = parseUrn("urn:isbn:0451450523");
        let tiny: [6]u8 = undefined;
        expect_eq(formatUrn(ok, tiny[0..6]), 0 as usize);
    });
}
