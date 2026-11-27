// cap/test/testCapStripBom.cp
// Comprehensive unit tests for @cap/strip-bom

import { test, expect_eq } from "cap:test";
import {
    BOM_NONE,
    BOM_UTF8,
    BOM_UTF16_LE,
    BOM_UTF16_BE,
    BOM_UTF32_LE,
    BOM_UTF32_BE,
    BomInfo,
    detectBom,
    hasUtf8Bom,
    stripUtf8Bom,
    stripAnyBomOffset,
    prependUtf8Bom
} from "../../packages/strip-bom/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/strip-bom — UTF-8 BOM detection & zero-copy stripping", () => {
        let data: [8]u8 = [0xEF, 0xBB, 0xBF, 'h', 'e', 'l', 'l', 'o'];
        let info = detectBom(data[0..8]);
        expect_eq(info.kind, BOM_UTF8);
        expect_eq(info.byte_len, 3 as usize);
        expect_eq(stripAnyBomOffset(data[0..8]), 3 as usize);

        expect_eq(hasUtf8Bom(data[0..8]), true);
        let stripped = stripUtf8Bom(data[0..8]);
        expect_eq(sliceEq(stripped, "hello"), true);
    });

    test("2. @cap/strip-bom — Strings without BOM pass through unchanged", () => {
        let plain: [5]u8 = ['w', 'o', 'r', 'l', 'd'];
        let info = detectBom(plain[0..5]);
        expect_eq(info.kind, BOM_NONE);
        expect_eq(info.byte_len, 0 as usize);
        expect_eq(stripAnyBomOffset(plain[0..5]), 0 as usize);

        expect_eq(hasUtf8Bom("world"), false);
        expect_eq(sliceEq(stripUtf8Bom("world"), "world"), true);
    });

    test("3. @cap/strip-bom — UTF-16LE & UTF-16BE detection", () => {
        // UTF-16LE: FF FE followed by 'A' (0x41 0x00)
        let u16le: [4]u8 = [0xFF, 0xFE, 0x41, 0x00];
        let i1 = detectBom(u16le[0..4]);
        expect_eq(i1.kind, BOM_UTF16_LE);
        expect_eq(i1.byte_len, 2 as usize);
        expect_eq(stripAnyBomOffset(u16le[0..4]), 2 as usize);

        // UTF-16BE: FE FF followed by 'A' (0x00 0x41)
        let u16be: [4]u8 = [0xFE, 0xFF, 0x00, 0x41];
        let i2 = detectBom(u16be[0..4]);
        expect_eq(i2.kind, BOM_UTF16_BE);
        expect_eq(i2.byte_len, 2 as usize);
        expect_eq(stripAnyBomOffset(u16be[0..4]), 2 as usize);
    });

    test("4. @cap/strip-bom — UTF-32LE (FF FE 00 00) takes priority over UTF-16LE & UTF-32BE", () => {
        // UTF-32LE: FF FE 00 00 MUST be detected as BOM_UTF32_LE (4 bytes), NOT BOM_UTF16_LE!
        let u32le: [8]u8 = [0xFF, 0xFE, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00];
        let i1 = detectBom(u32le[0..8]);
        expect_eq(i1.kind, BOM_UTF32_LE);
        expect_eq(i1.byte_len, 4 as usize);
        expect_eq(stripAnyBomOffset(u32le[0..8]), 4 as usize);

        // UTF-32BE: 00 00 FE FF
        let u32be: [8]u8 = [0x00, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x41];
        let i2 = detectBom(u32be[0..8]);
        expect_eq(i2.kind, BOM_UTF32_BE);
        expect_eq(i2.byte_len, 4 as usize);
        expect_eq(stripAnyBomOffset(u32be[0..8]), 4 as usize);
    });

    test("5. @cap/strip-bom — prependUtf8Bom is idempotent & checks output buffer capacity", () => {
        let out1: [16]u8 = undefined;
        let n1 = prependUtf8Bom("abc", out1[0..16]);
        expect_eq(n1, 6 as usize);
        expect_eq(hasUtf8Bom(out1[0..n1]), true);
        expect_eq(sliceEq(stripUtf8Bom(out1[0..n1]), "abc"), true);

        // Calling prependUtf8Bom again on already-BOM-prefixed slice does NOT duplicate BOM
        let out2: [16]u8 = undefined;
        let n2 = prependUtf8Bom(out1[0..n1], out2[0..16]);
        expect_eq(n2, 6 as usize);
        expect_eq(sliceEq(out2[0..n2], out1[0..n1]), true);

        // Undersized output buffer safely returns 0
        let tiny: [5]u8 = undefined;
        expect_eq(prependUtf8Bom("abc", tiny[0..5]), 0 as usize);
    });

    test("6. @cap/strip-bom — Empty & 1-2 byte short input boundary safety", () => {
        let buf: [2]u8 = [0xEF, 0xBB];
        expect_eq(detectBom(buf[0..0]).kind, BOM_NONE);
        expect_eq(detectBom(buf[0..1]).kind, BOM_NONE);
        expect_eq(detectBom(buf[0..2]).kind, BOM_NONE);
        expect_eq(hasUtf8Bom(""), false);
        expect_eq(hasUtf8Bom(buf[0..2]), false);
        expect_eq(sliceEq(stripUtf8Bom(""), ""), true);

        // Exact 3-byte UTF-8 BOM with empty body -> strips to ""
        let only_bom: [3]u8 = [0xEF, 0xBB, 0xBF];
        expect_eq(hasUtf8Bom(only_bom[0..3]), true);
        expect_eq(stripUtf8Bom(only_bom[0..3]).len, 0 as usize);

        // Exact 2-byte UTF-16LE BOM
        let only_u16: [2]u8 = [0xFF, 0xFE];
        expect_eq(detectBom(only_u16[0..2]).kind, BOM_UTF16_LE);
    });
}
