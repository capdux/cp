// cap/test/testCapPasswordHash.cp
// Comprehensive unit tests for @cap/password-hash (PHC string format)

import { test, expect_eq } from "cap:test";
import {
    PhcParam,
    PhcHash,
    parsePhc,
    formatPhc,
    getPhcParam,
    ctEqual,
} from "../../packages/password-hash/src/index.cp";

fn main() {
    test("1. @cap/password-hash — Parse full Argon2id PHC string and query params", () => {
        const s = "$argon2id$v=19$m=65536,t=3,p=4$c29tZXNhbHQ$RdescudvJCsgt3ub+b+dWRWJTmaaJObG";
        const h = parsePhc(s);
        expect_eq(h.valid, true);
        expect_eq(ctEqual(h.algorithm, "argon2id"), true);
        expect_eq(h.has_version, true);
        expect_eq(h.version, 19 as u32);
        expect_eq(h.param_count, 3 as usize);
        expect_eq(ctEqual(h.salt, "c29tZXNhbHQ"), true);
        expect_eq(ctEqual(h.hash, "RdescudvJCsgt3ub+b+dWRWJTmaaJObG"), true);

        const m = getPhcParam(h, "m");
        expect_eq(m != null, true);
        expect_eq(ctEqual(m.?, "65536"), true);

        const t = getPhcParam(h, "t");
        expect_eq(t != null, true);
        expect_eq(ctEqual(t.?, "3"), true);

        const p = getPhcParam(h, "p");
        expect_eq(p != null, true);
        expect_eq(ctEqual(p.?, "4"), true);

        expect_eq(getPhcParam(h, "missing") == null, true);
    });

    test("2. @cap/password-hash — Lossless formatPhc round-trip", () => {
        const s = "$argon2id$v=19$m=65536,t=3,p=4$c29tZXNhbHQ$RdescudvJCsgt3ub+b+dWRWJTmaaJObG";
        const h = parsePhc(s);
        expect_eq(h.valid, true);

        let out: [128]u8 = undefined;
        const n = formatPhc(h, out[0..128]);
        expect_eq(n, s.len);
        expect_eq(ctEqual(out[0..n], s), true);

        // Buffer too small returns 0
        expect_eq(formatPhc(h, out[0..10]), 0 as usize);
    });

    test("3. @cap/password-hash — Valid PHC variants (no version, no params, salt-only, algo-only)", () => {
        let buf: [128]u8 = undefined;

        // No version: $pbkdf2-sha256$i=10000,l=32$c2FsdA$aGFzaA
        const s1 = "$pbkdf2-sha256$i=10000,l=32$c2FsdA$aGFzaA";
        const h1 = parsePhc(s1);
        expect_eq(h1.valid, true);
        expect_eq(ctEqual(h1.algorithm, "pbkdf2-sha256"), true);
        expect_eq(h1.has_version, false);
        expect_eq(h1.param_count, 2 as usize);
        expect_eq(ctEqual(getPhcParam(h1, "i").?, "10000"), true);
        const n1 = formatPhc(h1, buf[0..128]);
        expect_eq(ctEqual(buf[0..n1], s1), true);

        // No version & no params: $scrypt$c29tZXNhbHQ$RdescudvJCs
        const s2 = "$scrypt$c29tZXNhbHQ$RdescudvJCs";
        const h2 = parsePhc(s2);
        expect_eq(h2.valid, true);
        expect_eq(h2.has_version, false);
        expect_eq(h2.param_count, 0 as usize);
        expect_eq(ctEqual(h2.salt, "c29tZXNhbHQ"), true);
        expect_eq(ctEqual(h2.hash, "RdescudvJCs"), true);
        const n2 = formatPhc(h2, buf[0..128]);
        expect_eq(ctEqual(buf[0..n2], s2), true);

        // Salt-only (no hash): $argon2i$v=19$m=4096$c29tZXNhbHQ
        const s3 = "$argon2i$v=19$m=4096$c29tZXNhbHQ";
        const h3 = parsePhc(s3);
        expect_eq(h3.valid, true);
        expect_eq(ctEqual(h3.salt, "c29tZXNhbHQ"), true);
        expect_eq(h3.hash.len, 0 as usize);
        const n3 = formatPhc(h3, buf[0..128]);
        expect_eq(ctEqual(buf[0..n3], s3), true);

        // Algorithm-only: $sha256
        const s4 = "$sha256";
        const h4 = parsePhc(s4);
        expect_eq(h4.valid, true);
        expect_eq(ctEqual(h4.algorithm, "sha256"), true);
        expect_eq(h4.salt.len, 0 as usize);
        const n4 = formatPhc(h4, buf[0..128]);
        expect_eq(ctEqual(buf[0..n4], s4), true);
    });

    test("4. @cap/password-hash — Reject malformed PHC strings and illegal characters", () => {
        expect_eq(parsePhc("").valid, false);
        expect_eq(parsePhc("argon2id$v=19").valid, false);
        expect_eq(parsePhc("$").valid, false);
        expect_eq(parsePhc("$argon2id$").valid, false);
        expect_eq(parsePhc("$$argon2id").valid, false);
        expect_eq(parsePhc("$Argon2id$v=19").valid, false); // uppercase in algorithm
        expect_eq(parsePhc("$argon2id$v=abc").valid, false); // non-numeric version
        expect_eq(parsePhc("$argon2id$v=99999999999").valid, false); // u32 overflow
        expect_eq(parsePhc("$argon2id$m=65536,$c2FsdA").valid, false); // trailing comma
        expect_eq(parsePhc("$argon2id$m=1=2$c2FsdA").valid, false); // double '='
        expect_eq(parsePhc("$argon2id$c2FsdA==$aGFzaA").valid, false); // '=' padding in salt
        expect_eq(parsePhc("$argon2id$c2FsdA$aGFzaA==").valid, false); // '=' padding in hash
        expect_eq(parsePhc("$argon2id$c2FsdA$aGFzaA$extra").valid, false); // extra field
    });

    test("5. @cap/password-hash — Constant-time equality ctEqual", () => {
        expect_eq(ctEqual("", ""), true);
        expect_eq(ctEqual("RdescudvJCsgt3ub+b+dWRWJTmaaJObG", "RdescudvJCsgt3ub+b+dWRWJTmaaJObG"), true);
        expect_eq(ctEqual("RdescudvJCsgt3ub+b+dWRWJTmaaJObG", "RdescudvJCsgt3ub+b+dWRWJTmaaJObH"), false);
        expect_eq(ctEqual("abc", "abcd"), false);
        expect_eq(ctEqual("abcd", "abc"), false);
    });

    test("6. @cap/password-hash — Invalid PhcHash handling in getPhcParam and formatPhc", () => {
        const bad = parsePhc("not-a-phc-string");
        expect_eq(bad.valid, false);
        expect_eq(getPhcParam(bad, "m") == null, true);

        let buf: [64]u8 = undefined;
        expect_eq(formatPhc(bad, buf[0..64]), 0 as usize);
    });
}
