// cap/test/testCapPunycode.cp
// Comprehensive unit tests for @cap/punycode

import { test, expect_eq } from "cap:test";
import {
    encodeLabel,
    decodeLabel,
    toAsciiDomain,
    toUnicodeDomain,
    isAsciiOnly,
} from "../../packages/punycode/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/punycode — Pure ASCII pass-through & isAsciiOnly", () => {
        expect_eq(isAsciiOnly("example.com"), true);
        expect_eq(isAsciiOnly("münchen"), false);

        let buf: [64]u8 = undefined;
        const n1 = toAsciiDomain("api.example.com", buf[0..64]);
        expect_eq(sliceEq(buf[0..(n1 as usize)], "api.example.com"), true);

        const n2 = toUnicodeDomain("api.example.com", buf[0..64]);
        expect_eq(sliceEq(buf[0..(n2 as usize)], "api.example.com"), true);
    });

    test("2. @cap/punycode — RFC 3492 German vectors (münchen <-> mnchen-3ya, bücher <-> bcher-kva)", () => {
        let enc: [64]u8 = undefined;
        let out: [64]u8 = undefined;

        const n1 = encodeLabel("münchen", enc[0..64]);
        expect_eq(sliceEq(enc[0..(n1 as usize)], "mnchen-3ya"), true);
        const d1 = decodeLabel("mnchen-3ya", out[0..64]);
        expect_eq(sliceEq(out[0..(d1 as usize)], "münchen"), true);

        const n2 = encodeLabel("bücher", enc[0..64]);
        expect_eq(sliceEq(enc[0..(n2 as usize)], "bcher-kva"), true);
        const d2 = decodeLabel("bcher-kva", out[0..64]);
        expect_eq(sliceEq(out[0..(d2 as usize)], "bücher"), true);
    });

    test("3. @cap/punycode — Chinese label round-trip (中文 <-> fiq228c)", () => {
        let enc: [64]u8 = undefined;
        const n = encodeLabel("中文", enc[0..64]);
        expect_eq(sliceEq(enc[0..(n as usize)], "fiq228c"), true);

        let out: [64]u8 = undefined;
        const d = decodeLabel("fiq228c", out[0..64]);
        expect_eq(sliceEq(out[0..(d as usize)], "中文"), true);
    });

    test("4. @cap/punycode — Full IDNA domain toAsciiDomain & toUnicodeDomain", () => {
        let asc: [128]u8 = undefined;
        const na = toAsciiDomain("bücher.example.com", asc[0..128]);
        expect_eq(sliceEq(asc[0..(na as usize)], "xn--bcher-kva.example.com"), true);

        let uni: [128]u8 = undefined;
        const nu = toUnicodeDomain("XN--bcher-kva.example.com", uni[0..128]);
        expect_eq(sliceEq(uni[0..(nu as usize)], "bücher.example.com"), true);
    });

    test("5. @cap/punycode — Labels containing hyphens (a-b-ü) round-trip cleanly", () => {
        let enc: [64]u8 = undefined;
        const n = encodeLabel("a-b-ü", enc[0..64]);
        expect_eq(n > (0 as i64), true);

        let out: [64]u8 = undefined;
        const d = decodeLabel(enc[0..(n as usize)], out[0..64]);
        expect_eq(sliceEq(out[0..(d as usize)], "a-b-ü"), true);
    });

    test("6. @cap/punycode — Reject invalid Punycode, overflow & tiny output buffers", () => {
        let out: [64]u8 = undefined;
        // Invalid character '!' in Punycode delta stream.
        expect_eq(decodeLabel("abc-!", out[0..64]), -1 as i64);
        // Truncated variable-length integer (never drops below threshold).
        expect_eq(decodeLabel("zzzzzzzz", out[0..64]), -1 as i64);

        let tiny: [3]u8 = undefined;
        expect_eq(encodeLabel("münchen", tiny[0..3]), -1 as i64);
        expect_eq(decodeLabel("mnchen-3ya", tiny[0..3]), -1 as i64);
        expect_eq(toAsciiDomain("bücher.example.com", tiny[0..3]), -1 as i64);
    });
}
