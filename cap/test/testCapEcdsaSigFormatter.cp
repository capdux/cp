import { test, expect_eq } from "cap:test";
import {
    EcdsaAlg,
    paramBytes,
    derToJose,
    joseToDer,
} from "../../packages/@cap/ecdsa-sig-formatter/src/index.cp";

fn main() {
    test("1. @cap/ecdsa-sig-formatter — paramBytes for ES256 / ES384 / ES512", () => {
        expect_eq(paramBytes(EcdsaAlg.ES256), 32 as usize);
        expect_eq(paramBytes(EcdsaAlg.ES384), 48 as usize);
        expect_eq(paramBytes(EcdsaAlg.ES512), 66 as usize);
    });

    test("2. @cap/ecdsa-sig-formatter — ES256 JOSE <-> DER round-trip with high-bit 0x00 padding", () => {
        let jose: [64]u8 = [0; 64];
        // r starts with 0x80 (needs DER 0x00 prefix), s has leading zeros (trimmed in DER)
        jose[0] = 0x80;
        jose[31] = 0x55;
        jose[32] = 0x00;
        jose[33] = 0x00;
        jose[34] = 0x7F;
        jose[63] = 0xAA;

        let der: [80]u8 = [0; 80];
        const dlen: i64 = joseToDer(jose[0..64], EcdsaAlg.ES256, der[0..80]);
        // r_der_len = 33 (padded), s_der_len = 30 (2 leading zeros stripped)
        // seq_len = 2+33+2+30 = 67, total = 2+67 = 69
        expect_eq(dlen, 69 as i64);
        expect_eq(der[0], 0x30 as u8);
        expect_eq(der[1], 67 as u8);
        expect_eq(der[2], 0x02 as u8);
        expect_eq(der[3], 33 as u8);
        expect_eq(der[4], 0x00 as u8);
        expect_eq(der[5], 0x80 as u8);

        let rt: [64]u8 = [0; 64];
        const jlen: i64 = derToJose(der[0..usize(dlen)], EcdsaAlg.ES256, rt[0..64]);
        expect_eq(jlen, 64 as i64);
        expect_eq(rt[0], 0x80 as u8);
        expect_eq(rt[31], 0x55 as u8);
        expect_eq(rt[32], 0x00 as u8);
        expect_eq(rt[34], 0x7F as u8);
        expect_eq(rt[63], 0xAA as u8);
    });

    test("3. @cap/ecdsa-sig-formatter — ES384 round-trip", () => {
        let jose: [96]u8 = [0; 96];
        let i: usize = 0;
        while (i < 96) : (i += 1) {
            jose[i] = u8((i + 1) & 0xFF);
        }
        jose[0] = 0x99;
        jose[48] = 0x88;

        let der: [128]u8 = [0; 128];
        const dlen: i64 = joseToDer(jose[0..96], EcdsaAlg.ES384, der[0..128]);
        // seq_len = 2+49+2+49 = 102 (< 128), total = 104
        expect_eq(dlen, 104 as i64);
        expect_eq(der[0], 0x30 as u8);
        expect_eq(der[1], 102 as u8);

        let rt: [96]u8 = [0; 96];
        const jlen: i64 = derToJose(der[0..usize(dlen)], EcdsaAlg.ES384, rt[0..96]);
        expect_eq(jlen, 96 as i64);
        expect_eq(rt[0], 0x99 as u8);
        expect_eq(rt[47], 48 as u8);
        expect_eq(rt[48], 0x88 as u8);
        expect_eq(rt[95], 96 as u8);
    });

    test("4. @cap/ecdsa-sig-formatter — ES512 long-form DER length (0x81)", () => {
        let jose: [132]u8 = [0; 132];
        jose[0] = 0x81;
        jose[65] = 0x11;
        jose[66] = 0xFE;
        jose[131] = 0x22;

        let der: [160]u8 = [0; 160];
        const dlen: i64 = joseToDer(jose[0..132], EcdsaAlg.ES512, der[0..160]);
        // r_der_len = 67, s_der_len = 67 -> seq_len = 138 >= 128 -> 0x30 0x81 138, total = 141
        expect_eq(dlen, 141 as i64);
        expect_eq(der[0], 0x30 as u8);
        expect_eq(der[1], 0x81 as u8);
        expect_eq(der[2], 138 as u8);

        let rt: [132]u8 = [0; 132];
        const jlen: i64 = derToJose(der[0..usize(dlen)], EcdsaAlg.ES512, rt[0..132]);
        expect_eq(jlen, 132 as i64);
        expect_eq(rt[0], 0x81 as u8);
        expect_eq(rt[65], 0x11 as u8);
        expect_eq(rt[66], 0xFE as u8);
        expect_eq(rt[131], 0x22 as u8);
    });

    test("5. @cap/ecdsa-sig-formatter — rejects malformed DER inputs", () => {
        let out: [64]u8 = [0; 64];

        // Wrong outer tag
        const bad_tag: [8]u8 = [0x31, 6, 0x02, 1, 1, 0x02, 1, 1];
        expect_eq(derToJose(bad_tag[0..8], EcdsaAlg.ES256, out[0..64]), -1 as i64);

        // Trailing garbage byte
        const trailing: [9]u8 = [0x30, 6, 0x02, 1, 1, 0x02, 1, 1, 0x00];
        expect_eq(derToJose(trailing[0..9], EcdsaAlg.ES256, out[0..64]), -1 as i64);

        // Negative integer (high bit set without leading 0x00)
        const neg_int: [8]u8 = [0x30, 6, 0x02, 1, 0x80, 0x02, 1, 1];
        expect_eq(derToJose(neg_int[0..8], EcdsaAlg.ES256, out[0..64]), -1 as i64);

        // Non-canonical long length for short sequence
        const non_canon: [9]u8 = [0x30, 0x81, 6, 0x02, 1, 1, 0x02, 1, 1];
        expect_eq(derToJose(non_canon[0..9], EcdsaAlg.ES256, out[0..64]), -1 as i64);
    });

    test("6. @cap/ecdsa-sig-formatter — rejects wrong JOSE length & small output buffers", () => {
        let der: [80]u8 = [0; 80];
        const short_jose: [10]u8 = [0; 10];
        expect_eq(joseToDer(short_jose[0..10], EcdsaAlg.ES256, der[0..80]), -1 as i64);

        // All-zero JOSE signature encodes to minimal DER: 30 06 02 01 00 02 01 00
        const zero_jose: [64]u8 = [0; 64];
        const dlen: i64 = joseToDer(zero_jose[0..64], EcdsaAlg.ES256, der[0..80]);
        expect_eq(dlen, 8 as i64);
        expect_eq(der[0], 0x30 as u8);
        expect_eq(der[1], 6 as u8);

        // Insufficient output buffer
        let tiny: [4]u8 = [0; 4];
        expect_eq(joseToDer(zero_jose[0..64], EcdsaAlg.ES256, tiny[0..4]), -1 as i64);
        expect_eq(derToJose(der[0..8], EcdsaAlg.ES256, tiny[0..4]), -1 as i64);
    });
}
