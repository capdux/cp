// cap/test/testCapEcdsaSig.cp
// Comprehensive unit tests for @cap/ecdsa-sig (RFC 3279 §2.2.3 & BIP-62 Low-S)

import { test, expect_eq } from "cap:test";
import {
    ECDSA_P256_COORD_LEN,
    ECDSA_P384_COORD_LEN,
    ECDSA_ERR_ZERO_COMPONENT,
    ECDSA_ERR_NON_CANONICAL_INT,
    ECDSA_ERR_NEGATIVE_INT,
    ECDSA_ERR_TRAILING_DATA,
    EcdsaSignature,
} from "../../packages/ecdsa/src/index.cp";

fn bytesEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/ecdsa-sig — P-256 fixed 64B <-> DER with high-bit 0x80 padding & leading-zero stripping", () => {
        let fixed: [64]u8 = undefined;
        // r starts with 0x85 (>= 0x80) -> DER must prepend 0x00 (length 33 = 0x21)
        let i: usize = 0;
        while (i < 32) {
            fixed[i] = (0x85 + (i & 0x0F)) as u8;
            i = i + 1;
        }
        // s has 2 leading zero bytes, followed by 0x42 (< 0x80) -> DER strips to length 30 = 0x1e
        fixed[32] = 0x00;
        fixed[33] = 0x00;
        i = 2;
        while (i < 32) {
            fixed[32 + i] = (0x40 + i) as u8;
            i = i + 1;
        }

        const parsed = EcdsaSignature.fromFixed(fixed[0..64]);
        expect_eq(parsed.ok, true);
        expect_eq(parsed.sig.coord_len, ECDSA_P256_COORD_LEN);

        let der: [80]u8 = undefined;
        const dlen: i64 = parsed.sig.toDer(der[0..80]);
        // SEQ(2) + INT_r(2 + 33) + INT_s(2 + 30) = 69 bytes (content 67 = 0x43)
        expect_eq(dlen, 69 as i64);
        expect_eq(der[0], 0x30 as u8);
        expect_eq(der[1], 0x43 as u8);
        expect_eq(der[2], 0x02 as u8);
        expect_eq(der[3], 0x21 as u8);
        expect_eq(der[4], 0x00 as u8);
        expect_eq(der[5], 0x85 as u8);
        expect_eq(der[37], 0x02 as u8);
        expect_eq(der[38], 0x1e as u8);
        expect_eq(der[39], 0x42 as u8);

        // Round-trip DER -> fixed 64B
        const back = EcdsaSignature.fromDer(der[0..(dlen as usize)], ECDSA_P256_COORD_LEN);
        expect_eq(back.ok, true);
        let out_fixed: [64]u8 = undefined;
        const flen: usize = back.sig.toFixed(out_fixed[0..64]);
        expect_eq(flen, 64 as usize);
        expect_eq(bytesEq(out_fixed[0..64], fixed[0..64]), true);
    });

    test("2. @cap/ecdsa-sig — P-384 fixed 96B <-> ASN.1 DER round-trip", () => {
        let fixed96: [96]u8 = undefined;
        let i: usize = 0;
        while (i < 48) {
            fixed96[i] = (0x90 + (i % 16)) as u8;       // high bit set -> 49B DER int
            fixed96[48 + i] = (0x10 + (i % 16)) as u8;  // high bit clear -> 48B DER int
            i = i + 1;
        }

        const parsed = EcdsaSignature.fromFixed(fixed96[0..96]);
        expect_eq(parsed.ok, true);
        expect_eq(parsed.sig.coord_len, ECDSA_P384_COORD_LEN);

        let der: [112]u8 = undefined;
        const dlen: i64 = parsed.sig.toDer(der[0..112]);
        // SEQ(2) + INT_r(2+49=51) + INT_s(2+48=50) = 103 bytes
        expect_eq(dlen, 103 as i64);

        const back = EcdsaSignature.fromDer(der[0..(dlen as usize)], ECDSA_P384_COORD_LEN);
        expect_eq(back.ok, true);
        expect_eq(back.sig.ctEq(parsed.sig), true);

        let out96: [96]u8 = undefined;
        expect_eq(back.sig.toFixed(out96[0..96]), 96 as usize);
        expect_eq(bytesEq(out96[0..96], fixed96[0..96]), true);
    });

    test("3. @cap/ecdsa-sig — RFC 6979 NIST P-256 known DER signature vector exact match", () => {
        // RFC 6979 Appendix A.2.5 (P-256, SHA-256, "sample"):
        // r = EFD48B2AACB6A8FD114039CD465795910ABF4FA9B43B6D63B0D62D0382B73438
        // s = F7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8
        let der_vec: [72]u8 = [
            0x30, 0x46,
            0x02, 0x21, 0x00,
            0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8, 0xfd,
            0x11, 0x40, 0x39, 0xcd, 0x46, 0x57, 0x95, 0x91,
            0x0a, 0xbf, 0x4f, 0xa9, 0xb4, 0x3b, 0x6d, 0x63,
            0xb0, 0xd6, 0x2d, 0x03, 0x82, 0xb7, 0x34, 0x38,
            0x02, 0x21, 0x00,
            0xf7, 0xcb, 0x1c, 0x94, 0x2d, 0x65, 0x7c, 0x41,
            0xd4, 0x36, 0xc7, 0xa1, 0xb6, 0xe2, 0x9f, 0x65,
            0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf, 0xf4, 0x06,
            0x4d, 0xc4, 0xab, 0x2f, 0x84, 0x3a, 0xcd, 0xa8
        ];

        const parsed = EcdsaSignature.fromDer(der_vec[0..72], ECDSA_P256_COORD_LEN);
        expect_eq(parsed.ok, true);
        expect_eq(parsed.sig.r[0], 0xef as u8);
        expect_eq(parsed.sig.r[31], 0x38 as u8);
        expect_eq(parsed.sig.s[0], 0xf7 as u8);
        expect_eq(parsed.sig.s[31], 0xa8 as u8);

        // Re-encode to DER and verify exact byte-for-byte match
        let re_der: [80]u8 = undefined;
        const rlen: i64 = parsed.sig.toDer(re_der[0..80]);
        expect_eq(rlen, 72 as i64);
        expect_eq(bytesEq(re_der[0..72], der_vec[0..72]), true);
    });

    test("4. @cap/ecdsa-sig — isLowSP256 & normalizeLowSP256 exact 256-bit n - s normalization", () => {
        // Construct high-S: s = n - 1 -> must normalize to s' = 1
        let fixed: [64]u8 = [
            // r = 1
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            // s = n - 1 = FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632550
            0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
            0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x50
        ];

        let sig = EcdsaSignature.fromFixed(fixed[0..64]).sig;
        expect_eq(sig.isLowSP256(), false);
        expect_eq(sig.normalizeLowSP256(), true);
        expect_eq(sig.isLowSP256(), true);

        // Verify normalized s == 1
        let expected_s: [32]u8 = [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
        ];
        expect_eq(bytesEq(sig.s[0..32], expected_s[0..32]), true);
        // Second call to normalizeLowSP256 is a no-op returning false
        expect_eq(sig.normalizeLowSP256(), false);

        // Exact boundary test: s = floor(n/2) + 1 -> must normalize to floor(n/2)
        // floor(n/2) + 1 = 7FFFFFFF800000007FFFFFFFFFFFFFFFDE737D56D38BCF4279DCE5617E3192A9
        let boundary_high: [64]u8 = [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
            0x7F, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00,
            0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xDE, 0x73, 0x7D, 0x56, 0xD3, 0x8B, 0xCF, 0x42,
            0x79, 0xDC, 0xE5, 0x61, 0x7E, 0x31, 0x92, 0xA9
        ];
        let bsig = EcdsaSignature.fromFixed(boundary_high[0..64]).sig;
        expect_eq(bsig.isLowSP256(), false);
        expect_eq(bsig.normalizeLowSP256(), true);
        expect_eq(bsig.isLowSP256(), true);
        expect_eq(bsig.s[31], 0xA8 as u8);
    });

    test("5. @cap/ecdsa-sig — Constant-time ctEq comparison", () => {
        let a_bytes: [64]u8 = undefined;
        let b_bytes: [64]u8 = undefined;
        let i: usize = 0;
        while (i < 64) {
            a_bytes[i] = (0x20 + i) as u8;
            b_bytes[i] = (0x20 + i) as u8;
            i = i + 1;
        }
        const sa = EcdsaSignature.fromFixed(a_bytes[0..64]).sig;
        const sb = EcdsaSignature.fromFixed(b_bytes[0..64]).sig;
        expect_eq(sa.ctEq(sb), true);

        // Flip 1 bit in s -> not equal
        b_bytes[63] = b_bytes[63] ^ 0x01;
        const sc = EcdsaSignature.fromFixed(b_bytes[0..64]).sig;
        expect_eq(sa.ctEq(sc), false);
    });

    test("6. @cap/ecdsa-sig — Strictly reject zero r/s, non-canonical DER, negative int & trailing bytes", () => {
        // All-zero r rejected
        let zero_r: [64]u8 = undefined;
        let i: usize = 0;
        while (i < 64) {
            zero_r[i] = 0;
            i = i + 1;
        }
        zero_r[63] = 1;
        const rz = EcdsaSignature.fromFixed(zero_r[0..64]);
        expect_eq(rz.ok, false);
        expect_eq(rz.err, ECDSA_ERR_ZERO_COMPONENT);

        // Non-canonical DER leading 0x00 when next byte < 0x80: 30 07 02 02 00 01 02 01 01
        let non_canon: [9]u8 = [0x30, 0x07, 0x02, 0x02, 0x00, 0x01, 0x02, 0x01, 0x01];
        const rnc = EcdsaSignature.fromDer(non_canon[0..9], ECDSA_P256_COORD_LEN);
        expect_eq(rnc.ok, false);
        expect_eq(rnc.err, ECDSA_ERR_NON_CANONICAL_INT);

        // Negative integer in DER (r = 0x80 without leading 0x00): 30 06 02 01 80 02 01 01
        let neg_der: [8]u8 = [0x30, 0x06, 0x02, 0x01, 0x80, 0x02, 0x01, 0x01];
        const rneg = EcdsaSignature.fromDer(neg_der[0..8], ECDSA_P256_COORD_LEN);
        expect_eq(rneg.ok, false);
        expect_eq(rneg.err, ECDSA_ERR_NEGATIVE_INT);

        // Trailing garbage byte after valid SEQUENCE: 30 06 02 01 01 02 01 01 FF
        let trail: [9]u8 = [0x30, 0x06, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01, 0xFF];
        const rtr = EcdsaSignature.fromDer(trail[0..9], ECDSA_P256_COORD_LEN);
        expect_eq(rtr.ok, false);
        expect_eq(rtr.err, ECDSA_ERR_TRAILING_DATA);
    });
}
