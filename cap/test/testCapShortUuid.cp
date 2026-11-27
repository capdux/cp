// cap/test/testCapShortUuid.cp
// Comprehensive unit tests for @cap/short-uuid

import { test, expect_eq } from "cap:test";
import {
    FLICKR_BASE58,
    URL_BASE62,
    COOKIE_BASE90,
    uuidToBytes,
    bytesToUuid,
    shortenUuid,
    expandUuid,
    shortenUuidWith,
    expandUuidWith,
} from "../../packages/short-uuid/src/index.cp";

fn main() {
    test("1. @cap/short-uuid — Standard UUID v4 lossless shorten/expand & fixed 22-char length", () => {
        expect_eq(FLICKR_BASE58.len, 58 as usize);
        expect_eq(URL_BASE62.len, 62 as usize);
        expect_eq(COOKIE_BASE90.len, 90 as usize);

        let sbuf: [32]u8 = undefined;
        let ubuf: [36]u8 = undefined;

        const u1 = "550e8400-e29b-41d4-a716-446655440000";
        const short1 = shortenUuid(u1, sbuf[0..32]);
        expect_eq(short1 != null, true);
        expect_eq(short1.?.len, 22 as usize);

        const exp1 = expandUuid(short1.?, ubuf[0..36]);
        expect_eq(exp1 != null, true);
        expect_eq(exp1.?, u1);

        const u2 = "a44521d0-0fb8-4ade-8002-3385545c3318";
        const short2 = shortenUuid(u2, sbuf[0..32]).?;
        expect_eq(short2.len, 22 as usize);
        expect_eq(expandUuid(short2, ubuf[0..36]).?, u2);
    });

    test("2. @cap/short-uuid — Extreme 128-bit bounds (all-zero & all-F lossless round-trip)", () => {
        let sbuf: [32]u8 = undefined;
        let ubuf: [36]u8 = undefined;

        const nil_uuid = "00000000-0000-0000-0000-000000000000";
        const nil_short = shortenUuid(nil_uuid, sbuf[0..32]).?;
        expect_eq(nil_short, "1111111111111111111111");
        expect_eq(expandUuid(nil_short, ubuf[0..36]).?, nil_uuid);

        const max_uuid = "ffffffff-ffff-ffff-ffff-ffffffffffff";
        const max_short = shortenUuid(max_uuid, sbuf[0..32]).?;
        expect_eq(max_short.len, 22 as usize);
        expect_eq(expandUuid(max_short, ubuf[0..36]).?, max_uuid);
    });

    test("3. @cap/short-uuid — Compact 32-char & uppercase UUID normalization", () => {
        let bytes: [16]u8 = undefined;
        let sbuf: [32]u8 = undefined;
        let ubuf: [36]u8 = undefined;

        const compact_upper = "550E8400E29B41D4A716446655440000";
        expect_eq(uuidToBytes(compact_upper, bytes[0..16]), true);
        expect_eq(bytesToUuid(bytes[0..16], ubuf[0..36]), "550e8400-e29b-41d4-a716-446655440000");

        const short_id = shortenUuid(compact_upper, sbuf[0..32]).?;
        expect_eq(short_id.len, 22 as usize);
        expect_eq(expandUuid(short_id, ubuf[0..36]).?, "550e8400-e29b-41d4-a716-446655440000");
    });

    test("4. @cap/short-uuid — Custom alphabets (URL_BASE62 & COOKIE_BASE90)", () => {
        let sbuf: [32]u8 = undefined;
        let ubuf: [36]u8 = undefined;
        const orig = "9b1deb4d-3b7d-4bad-9bdd-2b0d7b3dcb6d";

        const b62 = shortenUuidWith(orig, URL_BASE62, sbuf[0..32]).?;
        expect_eq(b62.len, 22 as usize);
        expect_eq(expandUuidWith(b62, URL_BASE62, ubuf[0..36]).?, orig);

        const b90 = shortenUuidWith(orig, COOKIE_BASE90, sbuf[0..32]).?;
        expect_eq(b90.len, 20 as usize);
        expect_eq(expandUuidWith(b90, COOKIE_BASE90, ubuf[0..36]).?, orig);
    });

    test("5. @cap/short-uuid — Leading-zero small values preserve fixed length & expand accurately", () => {
        let sbuf: [32]u8 = undefined;
        let ubuf: [36]u8 = undefined;

        const tiny = "00000000-0000-0000-0000-000000000001";
        const tiny_short = shortenUuid(tiny, sbuf[0..32]).?;
        expect_eq(tiny_short, "1111111111111111111112");
        expect_eq(expandUuid(tiny_short, ubuf[0..36]).?, tiny);
    });

    test("6. @cap/short-uuid — Invalid UUIDs, invalid alphabet chars & 128-bit overflow rejected", () => {
        let sbuf: [32]u8 = undefined;
        let ubuf: [36]u8 = undefined;

        // Invalid UUIDs
        expect_eq(shortenUuid("not-a-uuid", sbuf[0..32]) == null, true);
        expect_eq(shortenUuid("550e8400_e29b_41d4_a716_446655440000", sbuf[0..32]) == null, true);
        expect_eq(shortenUuid("zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz", sbuf[0..32]) == null, true);

        // Invalid chars in Flickr Base58 ('0', 'O', 'I', 'l' are excluded)
        expect_eq(expandUuid("", ubuf[0..36]) == null, true);
        expect_eq(expandUuid("0000000000000000000000", ubuf[0..36]) == null, true);
        expect_eq(expandUuid("OOOOOOOOOOOOOOOOOOOOOO", ubuf[0..36]) == null, true);

        // 128-bit overflow (58^22 - 1 > 2^128 - 1)
        expect_eq(expandUuid("ZZZZZZZZZZZZZZZZZZZZZZ", ubuf[0..36]) == null, true);

        // Duplicate-char custom alphabet rejected
        expect_eq(shortenUuidWith("550e8400-e29b-41d4-a716-446655440000", "aabbcc", sbuf[0..32]) == null, true);
    });
}
