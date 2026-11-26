import { test, expect_eq } from "cap:test";
import {
    parseMailto,
    countRecipients,
    formatMailto,
    isBasicEmail,
} from "../../packages/@cap/mailto-uri/src/index.cp";

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
    test("1. @cap/mailto-uri — Single recipient mailto:a@b.com", () => {
        const m = parseMailto("mailto:alice@example.com");
        expect_eq(m.valid, true);
        expect_eq(testEq(m.to, "alice@example.com"), true);
        expect_eq(testEq(m.subject, ""), true);
        expect_eq(testEq(m.body, ""), true);
    });

    test("2. @cap/mailto-uri — Multi-recipient + subject, cc, bcc & body", () => {
        const uri = "mailto:alice@example.com,bob@example.com?subject=Hi%20CP&cc=carol@example.com&bcc=dave@example.com&body=Hello%20World";
        const m = parseMailto(uri);
        expect_eq(m.valid, true);
        expect_eq(testEq(m.to, "alice@example.com,bob@example.com"), true);
        expect_eq(testEq(m.subject, "Hi%20CP"), true);
        expect_eq(testEq(m.cc, "carol@example.com"), true);
        expect_eq(testEq(m.bcc, "dave@example.com"), true);
        expect_eq(testEq(m.body, "Hello%20World"), true);
    });

    test("3. @cap/mailto-uri — countRecipients & isBasicEmail validation", () => {
        expect_eq(countRecipients("a@b.com, c@d.org, e@f.io"), 3 as usize);
        expect_eq(countRecipients("a@b.com,, "), 1 as usize);
        expect_eq(countRecipients(""), 0 as usize);

        expect_eq(isBasicEmail("user.name+tag@sub.example.com"), true);
        expect_eq(isBasicEmail("no-at.com"), false);
        expect_eq(isBasicEmail("a@b@c.com"), false);
        expect_eq(isBasicEmail("@example.com"), false);
        expect_eq(isBasicEmail("user@localhost"), false);
        expect_eq(isBasicEmail("user@.example.com"), false);
        expect_eq(isBasicEmail("user@exam..ple.com"), false);
        expect_eq(isBasicEmail("has space@example.com"), false);
    });

    test("4. @cap/mailto-uri — formatMailto percent-encoding & roundtrip", () => {
        let buf: [256]u8 = undefined;
        const n = formatMailto("alice@example.com", "Hi CP!", "Line 1\nLine 2", "bob@example.com", buf[0..256]);
        const expected = "mailto:alice@example.com?subject=Hi%20CP%21&body=Line%201%0ALine%202&cc=bob@example.com";
        expect_eq(n, expected.len);
        expect_eq(testEq(buf[0..n], expected), true);

        const reparsed = parseMailto(buf[0..n]);
        expect_eq(reparsed.valid, true);
        expect_eq(testEq(reparsed.to, "alice@example.com"), true);
        expect_eq(testEq(reparsed.subject, "Hi%20CP%21"), true);
        expect_eq(testEq(reparsed.cc, "bob@example.com"), true);
    });

    test("5. @cap/mailto-uri — Case-insensitive MAILTO: & ?to= query parameter", () => {
        const m = parseMailto("MAILTO:?to=support@cap.dev&SUBJECT=Help");
        expect_eq(m.valid, true);
        expect_eq(testEq(m.to, "support@cap.dev"), true);
        expect_eq(testEq(m.subject, "Help"), true);
    });

    test("6. @cap/mailto-uri — Rejects non-mailto, empty & undersized buffer", () => {
        expect_eq(parseMailto("").valid, false);
        expect_eq(parseMailto("mailto:").valid, false);
        expect_eq(parseMailto("https://example.com").valid, false);

        let tiny: [8]u8 = undefined;
        expect_eq(formatMailto("alice@example.com", "Hi", "", "", tiny[0..8]), 0 as usize);
        expect_eq(formatMailto("", "", "", "", tiny[0..8]), 0 as usize);
    });
}
