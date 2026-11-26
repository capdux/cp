// cap/test/testCapCurve25519Dalek.cp
// Comprehensive unit tests for @cap/curve25519-dalek (RFC 7748 §5.2 & §6.1 verified)

import { test, expect_eq } from "cap:test";
import {
    FieldElement,
    clampScalar,
    x25519,
    x25519Basepoint,
} from "../../packages/@cap/curve25519-dalek/src/index.cp";

fn hexVal(ch: u8): u8 {
    if (ch >= ('0' as u8) && ch <= ('9' as u8)) {
        return ch - ('0' as u8);
    }
    if (ch >= ('a' as u8) && ch <= ('f' as u8)) {
        return (ch - ('a' as u8)) + (10 as u8);
    }
    return (ch - ('A' as u8)) + (10 as u8);
}

fn hex32(s: string): [32]u8 {
    let out: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 32) {
        out[i] = (hexVal(s[i * 2]) << 4) | hexVal(s[i * 2 + 1]);
        i += 1;
    }
    return out;
}

fn eq32(a: [32]u8, b: [32]u8): bool {
    let i: usize = 0;
    while (i < 32) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/curve25519-dalek — RFC 7748 §5.2 test vector 1", () => {
        const scalar = hex32("a546e36bf0527c9d3b16154b82465edd62144c0ac1fc5a18506a2244ba449ac4");
        const uCoord = hex32("e6db6867583030db3594c1a424b15f7c726624ec26b3353b10a903a6d0ab1c4c");
        const expected = hex32("c3da55379de9c6908e94ea4df28d084f32eccf03491c71f754b4075577a28552");
        const got = x25519(scalar, uCoord);
        expect_eq(eq32(got, expected), true);
    });

    test("2. @cap/curve25519-dalek — RFC 7748 §5.2 test vector 2", () => {
        const scalar = hex32("4b66e9d4d1b4673c5ad22691957d6af5c11b6421e0ea01d42ca4169e7918ba0d");
        const uCoord = hex32("e5210f12786811d3f4b7959d0538ae2c31dbe7106fc03c3efc4cd549c715a493");
        const expected = hex32("95cbde9476e8907d7aade45cb4b873f88b595a68799fa152e6f8f7647aac7957");
        const got = x25519(scalar, uCoord);
        expect_eq(eq32(got, expected), true);
    });

    test("3. @cap/curve25519-dalek — RFC 7748 §6.1 Alice/Bob Diffie-Hellman key agreement", () => {
        const alicePriv = hex32("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a");
        const alicePubExpected = hex32("8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a");
        const bobPriv = hex32("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb");
        const bobPubExpected = hex32("de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f");
        const sharedExpected = hex32("4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742");

        const alicePub = x25519Basepoint(alicePriv);
        const bobPub = x25519Basepoint(bobPriv);
        expect_eq(eq32(alicePub, alicePubExpected), true);
        expect_eq(eq32(bobPub, bobPubExpected), true);

        const sharedAlice = x25519(alicePriv, bobPub);
        const sharedBob = x25519(bobPriv, alicePub);
        expect_eq(eq32(sharedAlice, sharedExpected), true);
        expect_eq(eq32(sharedBob, sharedExpected), true);
    });

    test("4. @cap/curve25519-dalek — RFC 7748 §5.2 basepoint iterative 1-round vector", () => {
        const base = hex32("0900000000000000000000000000000000000000000000000000000000000000");
        const expected1 = hex32("422c8e7a6227d7bca1350b3e2bb7279f7897b87bb6854b783c60e80311ae3079");
        const got1 = x25519(base, base);
        expect_eq(eq32(got1, expected1), true);
    });

    test("5. @cap/curve25519-dalek — FieldElement add/sub/mul/square/invert identities", () => {
        const aBytes = hex32("123456789abcdef0112233445566778899aabbccddeeff001020304050607001");
        const bBytes = hex32("fedcba98765432100102030405060708090a0b0c0d0e0f102030405060700002");
        const a = FieldElement.fromBytes(aBytes);
        const b = FieldElement.fromBytes(bBytes);
        const oneFe: FieldElement = FieldElement.ONE;
        const zeroFe: FieldElement = FieldElement.ZERO;
        const oneBytes: [32]u8 = oneFe.toBytes();
        const zeroBytes: [32]u8 = zeroFe.toBytes();

        // a * a^(-1) == 1
        const inv = a.invert();
        expect_eq(eq32(a.mul(inv).toBytes(), oneBytes), true);
        // (a + b) - b == a
        expect_eq(eq32(a.add(b).sub(b).toBytes(), a.toBytes()), true);
        // a - a == 0
        expect_eq(eq32(a.sub(a).toBytes(), zeroBytes), true);
        // a.square() == a.mul(a)
        expect_eq(eq32(a.square().toBytes(), a.mul(a).toBytes()), true);
    });

    test("6. @cap/curve25519-dalek — clampScalar bitmask invariants", () => {
        const allOnes = hex32("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        const clamped = clampScalar(allOnes);
        expect_eq(clamped[0] & (0x07 as u8), 0 as u8);
        expect_eq(clamped[31] & (0x80 as u8), 0 as u8);
        expect_eq(clamped[31] & (0x40 as u8), 0x40 as u8);

        const allZeros = hex32("0000000000000000000000000000000000000000000000000000000000000000");
        const clampedZero = clampScalar(allZeros);
        expect_eq(clampedZero[0], 0 as u8);
        expect_eq(clampedZero[31], 0x40 as u8);
    });
}
