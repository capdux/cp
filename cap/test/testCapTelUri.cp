// cap/test/testCapTelUri.cp
// Comprehensive unit tests for @cap/tel-uri (RFC 3966 & E.164)

import { test, expect_eq } from "cap:test";
import {
    TelUri,
    parseTelUri,
    normalizeE164,
    isValidE164,
    formatTelUri,
} from "../../packages/@cap/tel-uri/src/index.cp";

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
    test("1. @cap/tel-uri — global tel:+1-201-555-0123 parsing", () => {
        const t: TelUri = parseTelUri("TEL:+1-201-555-0123");
        expect_eq(t.valid, true);
        expect_eq(t.is_global, true);
        expect_eq(sliceEq(t.number, "+1-201-555-0123"), true);
        expect_eq(t.ext.len, 0 as usize);
        expect_eq(t.phone_context.len, 0 as usize);
    });

    test("2. @cap/tel-uri — global with ;ext=123 & local with ;phone-context=example.com", () => {
        const t1: TelUri = parseTelUri("tel:+44-20-7946-0958;ext=123");
        expect_eq(t1.valid, true);
        expect_eq(t1.is_global, true);
        expect_eq(sliceEq(t1.ext, "123"), true);

        const t2: TelUri = parseTelUri("tel:555-0199;phone-context=example.com;ext=9");
        expect_eq(t2.valid, true);
        expect_eq(t2.is_global, false);
        expect_eq(sliceEq(t2.number, "555-0199"), true);
        expect_eq(sliceEq(t2.phone_context, "example.com"), true);
        expect_eq(sliceEq(t2.ext, "9"), true);
    });

    test("3. @cap/tel-uri — rejects local number missing phone-context", () => {
        expect_eq(parseTelUri("tel:555-0199").valid, false);
        expect_eq(parseTelUri("tel:555-0199;ext=10").valid, false);
    });

    test("4. @cap/tel-uri — normalizeE164 strips visual separators & validates length", () => {
        let out: [32]u8 = undefined;
        const n: usize = normalizeE164("  +1 (201) 555-0123  ", out[0..32]);
        expect_eq(n, 12 as usize);
        expect_eq(sliceEq(out[0..n], "+12015550123"), true);
        expect_eq(isValidE164(out[0..n]), true);

        const n2: usize = normalizeE164("+86.138.0013.8000", out[0..32]);
        expect_eq(sliceEq(out[0..n2], "+8613800138000"), true);

        // Too short (< 7 digits), too long (> 15 digits), leading zero
        expect_eq(normalizeE164("+123456", out[0..32]), 0 as usize);
        expect_eq(normalizeE164("+1234567890123456", out[0..32]), 0 as usize);
        expect_eq(normalizeE164("+0123456789", out[0..32]), 0 as usize);
    });

    test("5. @cap/tel-uri — formatTelUri round-trip with and without ext", () => {
        let out: [64]u8 = undefined;
        const n1: usize = formatTelUri("+12015550123", "", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "tel:+12015550123"), true);
        expect_eq(parseTelUri(out[0..n1]).valid, true);

        const n2: usize = formatTelUri("+12015550123", "404", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "tel:+12015550123;ext=404"), true);
        const parsed: TelUri = parseTelUri(out[0..n2]);
        expect_eq(parsed.valid, true);
        expect_eq(sliceEq(parsed.ext, "404"), true);
    });

    test("6. @cap/tel-uri — rejects non-tel scheme, invalid E.164 & tiny buffers", () => {
        expect_eq(parseTelUri("http://example.com").valid, false);
        expect_eq(parseTelUri("tel:").valid, false);
        expect_eq(parseTelUri("tel:+abc").valid, false);

        expect_eq(isValidE164("12015550123"), false);
        expect_eq(isValidE164("+01234567"), false);
        expect_eq(isValidE164("+123456"), false);

        let tiny: [8]u8 = undefined;
        expect_eq(normalizeE164("+12015550123", tiny[0..8]), 0 as usize);
        expect_eq(formatTelUri("+12015550123", "12", tiny[0..8]), 0 as usize);
        expect_eq(formatTelUri("not-e164", "", tiny[0..8]), 0 as usize);
    });
}
