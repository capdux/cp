// cap/test/testCapBase16ct.cp
// Comprehensive unit tests for @cap/base16ct

import { test, expect_eq } from "cap:test";
import {
    HexDecodeResult,
    encodedLen,
    encodeLower,
    encodeUpper,
    decodeLower,
    decodeUpper,
    decodeMixed
} from "../../packages/base16ct/src/index.cp";

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
    test("1. @cap/base16ct — Full 256 byte values 0x00..0xFF lower & upper round-trip", () => {
        let raw: [256]u8 = undefined;
        let i: usize = 0;
        while (i < 256) {
            raw[i] = i as u8;
            i = i + 1;
        }

        let hex_lo: [512]u8 = undefined;
        let hex_hi: [512]u8 = undefined;
        let out_lo: [256]u8 = undefined;
        let out_hi: [256]u8 = undefined;

        expect_eq(encodeLower(raw[0..256], hex_lo[0..512]), 512 as usize);
        expect_eq(encodeUpper(raw[0..256], hex_hi[0..512]), 512 as usize);

        // Spot-check boundary bytes: 0x00 -> "00", 0x0A -> "0a"/"0A", 0xFF -> "ff"/"FF"
        expect_eq(sliceEq(hex_lo[0..2], "00"), true);
        expect_eq(sliceEq(hex_lo[20..22], "0a"), true);
        expect_eq(sliceEq(hex_lo[510..512], "ff"), true);
        expect_eq(sliceEq(hex_hi[20..22], "0A"), true);
        expect_eq(sliceEq(hex_hi[510..512], "FF"), true);

        let d_lo = decodeLower(hex_lo[0..512], out_lo[0..256]);
        expect_eq(d_lo.ok, true);
        expect_eq(d_lo.len, 256 as usize);
        expect_eq(sliceEq(out_lo[0..256], raw[0..256]), true);

        let d_hi = decodeUpper(hex_hi[0..512], out_hi[0..256]);
        expect_eq(d_hi.ok, true);
        expect_eq(d_hi.len, 256 as usize);
        expect_eq(sliceEq(out_hi[0..256], raw[0..256]), true);
    });

    test("2. @cap/base16ct — Strict case enforcement (decodeLower rejects upper, decodeUpper rejects lower)", () => {
        let out: [8]u8 = undefined;

        let r1 = decodeLower("deadbeef", out[0..8]);
        expect_eq(r1.ok, true);
        expect_eq(r1.len, 4 as usize);
        expect_eq(out[0], 0xDE as u8);
        expect_eq(out[3], 0xEF as u8);

        expect_eq(decodeLower("DEADBEEF", out[0..8]).ok, false);
        expect_eq(decodeLower("deadBeeF", out[0..8]).ok, false);

        let r2 = decodeUpper("DEADBEEF", out[0..8]);
        expect_eq(r2.ok, true);
        expect_eq(r2.len, 4 as usize);
        expect_eq(out[0], 0xDE as u8);

        expect_eq(decodeUpper("deadbeef", out[0..8]).ok, false);
        expect_eq(decodeUpper("DeadBeef", out[0..8]).ok, false);
    });

    test("3. @cap/base16ct — decodeMixed accepts mixed case 0-9a-fA-F", () => {
        let out: [8]u8 = undefined;
        let r = decodeMixed("DeAdBeEf09aF", out[0..8]);
        expect_eq(r.ok, true);
        expect_eq(r.len, 6 as usize);
        let expected: [6]u8 = [0xDE, 0xAD, 0xBE, 0xEF, 0x09, 0xAF];
        expect_eq(sliceEq(out[0..6], expected[0..6]), true);
    });

    test("4. @cap/base16ct — Odd length & illegal chars ('g', '/', ':') rejected & output zeroed", () => {
        let out: [4]u8 = [0xAA, 0xBB, 0xCC, 0xDD];
        expect_eq(decodeMixed("abc", out[0..4]).ok, false);
        expect_eq(out[0], 0 as u8);
        expect_eq(out[1], 0 as u8);

        let out2: [4]u8 = [0x11, 0x22, 0x33, 0x44];
        expect_eq(decodeLower("123g", out2[0..4]).ok, false);
        expect_eq(out2[0], 0 as u8);
        expect_eq(out2[1], 0 as u8);

        // ASCII chars adjacent to '0' ('/') and '9' (':')
        expect_eq(decodeMixed("12/4", out2[0..4]).ok, false);
        expect_eq(decodeMixed("12:4", out2[0..4]).ok, false);
        expect_eq(decodeMixed("12@4", out2[0..4]).ok, false);
        expect_eq(decodeMixed("12G4", out2[0..4]).ok, false);
    });

    test("5. @cap/base16ct — Empty input & undersized buffer safety", () => {
        let empty: [1]u8 = [0];
        let out: [4]u8 = undefined;
        expect_eq(encodedLen(0), 0 as usize);
        expect_eq(encodedLen(5), 10 as usize);
        expect_eq(encodeLower(empty[0..0], out[0..4]), 0 as usize);
        expect_eq(encodeUpper(empty[0..0], out[0..4]), 0 as usize);

        let d0 = decodeLower("", out[0..4]);
        expect_eq(d0.ok, true);
        expect_eq(d0.len, 0 as usize);

        // Buffer too small for encode/decode
        let src: [3]u8 = [0x01, 0x02, 0x03];
        expect_eq(encodeLower(src[0..3], out[0..4]), 0 as usize);
        expect_eq(encodeUpper(src[0..3], out[0..4]), 0 as usize);
        expect_eq(decodeMixed("0102030405", out[0..4]).ok, false);
    });

    test("6. @cap/base16ct — RFC 4648 Section 10 official test vectors ('foobar' -> '666F6F626172')", () => {
        let raw: [6]u8 = ['f', 'o', 'o', 'b', 'a', 'r'];
        let enc: [12]u8 = undefined;
        let out: [6]u8 = undefined;

        expect_eq(encodeUpper(raw[0..1], enc[0..12]), 2 as usize);
        expect_eq(sliceEq(enc[0..2], "66"), true);

        expect_eq(encodeUpper(raw[0..3], enc[0..12]), 6 as usize);
        expect_eq(sliceEq(enc[0..6], "666F6F"), true);

        expect_eq(encodeUpper(raw[0..6], enc[0..12]), 12 as usize);
        expect_eq(sliceEq(enc[0..12], "666F6F626172"), true);

        let dr = decodeUpper("666F6F626172", out[0..6]);
        expect_eq(dr.ok, true);
        expect_eq(dr.len, 6 as usize);
        expect_eq(sliceEq(out[0..6], "foobar"), true);
    });
}
