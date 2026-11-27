// cap/test/testCapBlockPadding.cp
// Comprehensive unit tests for @cap/block-padding

import { test, expect_eq } from "cap:test";
import {
    BP_ERR,
    pkcs7Pad,
    pkcs7UnpadCt,
    iso7816Pad,
    iso7816Unpad,
    ansiX923Pad,
    ansiX923Unpad,
    zeroPad,
    zeroUnpad,
    ctrIncBe128,
    ctrIncLe128,
    xorBlock,
    wipeBlock,
} from "../../packages/block-padding/src/index.cp";

fn bpTestEqBytes(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/block-padding — PKCS#7 regular, aligned full-block & empty round-trip", () => {
        // Regular 11-byte input with block_size=16 -> 5 bytes of 0x05
        let out: [64]u8 = [0; 64];
        const p1 = pkcs7Pad("Hello World", 16, out[0..64]);
        expect_eq(p1, 16 as i64);
        expect_eq(out[11], 0x05 as u8);
        expect_eq(out[15], 0x05 as u8);
        expect_eq(pkcs7UnpadCt(out[0..16], 16), 11 as i64);

        // Already aligned 16-byte input -> appends full 16 bytes of 0x10
        const p2 = pkcs7Pad("0123456789ABCDEF", 16, out[0..64]);
        expect_eq(p2, 32 as i64);
        expect_eq(out[16], 0x10 as u8);
        expect_eq(out[31], 0x10 as u8);
        expect_eq(pkcs7UnpadCt(out[0..32], 16), 16 as i64);

        // Empty input with block_size=8 -> 8 bytes of 0x08
        const p3 = pkcs7Pad("", 8, out[0..64]);
        expect_eq(p3, 8 as i64);
        expect_eq(out[0], 0x08 as u8);
        expect_eq(out[7], 0x08 as u8);
        expect_eq(pkcs7UnpadCt(out[0..8], 8), 0 as i64);
    });

    test("2. @cap/block-padding — Constant-time pkcs7UnpadCt rejects invalid padding", () => {
        // pad_len = 0
        let bad0: [16]u8 = [0x41 as u8; 16];
        bad0[15] = 0x00 as u8;
        expect_eq(pkcs7UnpadCt(bad0[0..16], 16), BP_ERR);

        // pad_len > block_size (17 > 16)
        let bad_gt: [16]u8 = [0x11 as u8; 16];
        expect_eq(pkcs7UnpadCt(bad_gt[0..16], 16), BP_ERR);

        // Single-byte tamper in multi-byte padding
        let buf: [16]u8 = [0; 16];
        const plen = pkcs7Pad("secret", 16, buf[0..16]);
        expect_eq(plen, 16 as i64);
        buf[10] = 0x09 as u8; // should be 0x0A
        expect_eq(pkcs7UnpadCt(buf[0..16], 16), BP_ERR);

        // Unaligned input length
        expect_eq(pkcs7UnpadCt(buf[0..15], 16), BP_ERR);
    });

    test("3. @cap/block-padding — ISO/IEC 7816-4 pad/unpad & missing 0x80 rejection", () => {
        let out: [32]u8 = [0; 32];
        const p1 = iso7816Pad("abc", 8, out[0..32]);
        expect_eq(p1, 8 as i64);
        expect_eq(out[3], 0x80 as u8);
        expect_eq(out[4], 0x00 as u8);
        expect_eq(out[7], 0x00 as u8);
        expect_eq(iso7816Unpad(out[0..8], 8), 3 as i64);

        // Exact block alignment appends 0x80 followed by 7 zeros
        const p2 = iso7816Pad("12345678", 8, out[0..32]);
        expect_eq(p2, 16 as i64);
        expect_eq(out[8], 0x80 as u8);
        expect_eq(iso7816Unpad(out[0..16], 8), 8 as i64);

        // Reject block with no 0x80 marker or non-zero trailing byte
        let no_marker: [8]u8 = [0x00 as u8; 8];
        expect_eq(iso7816Unpad(no_marker[0..8], 8), BP_ERR);
        let bad_tail: [8]u8 = [0x80 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x01 as u8];
        expect_eq(iso7816Unpad(bad_tail[0..8], 8), BP_ERR);
    });

    test("4. @cap/block-padding — ANSI X9.23 pad/unpad & non-zero interior rejection", () => {
        let out: [32]u8 = [0; 32];
        const p1 = ansiX923Pad("HELLO", 8, out[0..32]);
        expect_eq(p1, 8 as i64);
        expect_eq(out[5], 0x00 as u8);
        expect_eq(out[6], 0x00 as u8);
        expect_eq(out[7], 0x03 as u8);
        expect_eq(ansiX923Unpad(out[0..8], 8), 5 as i64);

        // Tamper interior padding byte (must be 0x00)
        out[6] = 0x01 as u8;
        expect_eq(ansiX923Unpad(out[0..8], 8), BP_ERR);

        // pad_len = 0 rejected
        out[6] = 0x00 as u8;
        out[7] = 0x00 as u8;
        expect_eq(ansiX923Unpad(out[0..8], 8), BP_ERR);
    });

    test("5. @cap/block-padding — ZeroPadding, xorBlock & wipeBlock", () => {
        let out: [32]u8 = [0; 32];
        const p1 = zeroPad("CP-LANG", 16, out[0..32]);
        expect_eq(p1, 16 as i64);
        expect_eq(zeroUnpad(out[0..16], 16), 7 as i64);

        // Non-empty aligned input stays same length
        const p2 = zeroPad("01234567", 8, out[0..32]);
        expect_eq(p2, 8 as i64);
        expect_eq(zeroUnpad(out[0..8], 8), 8 as i64);

        // xorBlock round-trip: (a ^ b) ^ b == a
        let a: [8]u8 = [0x12 as u8, 0x34 as u8, 0x56 as u8, 0x78 as u8, 0x9A as u8, 0xBC as u8, 0xDE as u8, 0xF0 as u8];
        let b: [8]u8 = [0xFF as u8, 0x0F as u8, 0xF0 as u8, 0xAA as u8, 0x55 as u8, 0x11 as u8, 0x22 as u8, 0x33 as u8];
        let c: [8]u8 = [0; 8];
        let d: [8]u8 = [0; 8];
        expect_eq(xorBlock(c[0..8], a[0..8], b[0..8]), true);
        expect_eq(xorBlock(d[0..8], c[0..8], b[0..8]), true);
        expect_eq(bpTestEqBytes(d[0..8], a[0..8]), true);
        expect_eq(xorBlock(c[0..4], a[0..8], b[0..8]), false);

        wipeBlock(c[0..8]);
        let zeros: [8]u8 = [0; 8];
        expect_eq(bpTestEqBytes(c[0..8], zeros[0..8]), true);
    });

    test("6. @cap/block-padding — 128-bit BE & LE CTR counter carry propagation & wrap-around", () => {
        // Big-endian carry across multiple bytes
        let be: [16]u8 = [0; 16];
        be[14] = 0x01 as u8;
        be[15] = 0xFF as u8;
        ctrIncBe128(be[0..16]);
        expect_eq(be[14], 0x02 as u8);
        expect_eq(be[15], 0x00 as u8);

        // Big-endian all-0xFF wrap-around to all-0x00
        let be_max: [16]u8 = [0xFF as u8; 16];
        ctrIncBe128(be_max[0..16]);
        let zeros16: [16]u8 = [0; 16];
        expect_eq(bpTestEqBytes(be_max[0..16], zeros16[0..16]), true);

        // Little-endian carry across multiple bytes
        let le: [16]u8 = [0; 16];
        le[0] = 0xFF as u8;
        le[1] = 0xFF as u8;
        le[2] = 0x05 as u8;
        ctrIncLe128(le[0..16]);
        expect_eq(le[0], 0x00 as u8);
        expect_eq(le[1], 0x00 as u8);
        expect_eq(le[2], 0x06 as u8);

        // Little-endian all-0xFF wrap-around to all-0x00
        let le_max: [16]u8 = [0xFF as u8; 16];
        ctrIncLe128(le_max[0..16]);
        expect_eq(bpTestEqBytes(le_max[0..16], zeros16[0..16]), true);
    });
}
