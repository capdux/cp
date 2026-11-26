import { test, expect_eq } from "cap:test";
import {
    SIGNATURE_LENGTH,
    KEY_LENGTH,
    SPKI_DER_LENGTH,
    PKCS8_DER_LENGTH,
    Ed25519Signature,
    Ed25519PublicKey,
    Ed25519SecretKey,
} from "../../packages/@cap/ed25519/src/index.cp";

fn main() {
    test("1. @cap/ed25519 — constants & signature fromComponents / rBytes / sBytes / toBytes", () => {
        expect_eq(SIGNATURE_LENGTH, 64 as usize);
        expect_eq(KEY_LENGTH, 32 as usize);
        expect_eq(SPKI_DER_LENGTH, 44 as usize);
        expect_eq(PKCS8_DER_LENGTH, 48 as usize);

        const r: [32]u8 = [0xAA as u8; 32];
        const s: [32]u8 = [0x01 as u8; 32];
        const sig = Ed25519Signature.fromComponents(r, s);
        const r_out = sig.rBytes();
        const s_out = sig.sBytes();
        const full = sig.toBytes();
        expect_eq(r_out[0], 0xAA as u8);
        expect_eq(r_out[31], 0xAA as u8);
        expect_eq(s_out[0], 0x01 as u8);
        expect_eq(s_out[31], 0x01 as u8);
        expect_eq(full[0], 0xAA as u8);
        expect_eq(full[32], 0x01 as u8);
        expect_eq(sig.isCanonicalS(), true);
    });

    test("2. @cap/ed25519 — isCanonicalS() strict boundary around group order L", () => {
        const r: [32]u8 = [0x11 as u8; 32];
        // Exact group order L (little-endian) -> non-canonical (S == L)
        let s_eq_l: [32]u8 = [
            0xED as u8, 0xD3 as u8, 0xF5 as u8, 0x5C as u8,
            0x1A as u8, 0x63 as u8, 0x12 as u8, 0x58 as u8,
            0xD6 as u8, 0x9C as u8, 0xF7 as u8, 0xA2 as u8,
            0xDE as u8, 0xF9 as u8, 0xDE as u8, 0x14 as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x10 as u8,
        ];
        const sig_eq = Ed25519Signature.fromComponents(r, s_eq_l);
        expect_eq(sig_eq.isCanonicalS(), false);

        // L - 1 (s[0] = 0xEC) -> canonical
        s_eq_l[0] = 0xEC as u8;
        const sig_lt = Ed25519Signature.fromComponents(r, s_eq_l);
        expect_eq(sig_lt.isCanonicalS(), true);

        // L + 1 (s[0] = 0xEE) -> non-canonical
        s_eq_l[0] = 0xEE as u8;
        const sig_gt = Ed25519Signature.fromComponents(r, s_eq_l);
        expect_eq(sig_gt.isCanonicalS(), false);

        // High bit set (s[31] = 0x80) -> non-canonical
        const s_hi: [32]u8 = [0x80 as u8; 32];
        const sig_hi = Ed25519Signature.fromComponents(r, s_hi);
        expect_eq(sig_hi.isCanonicalS(), false);
    });

    test("3. @cap/ed25519 — signature hex roundtrip (RFC 8032 test vector #1)", () => {
        const hex_sig = "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b";
        if (sig := Ed25519Signature.fromHex(hex_sig)) {
            expect_eq(sig.isCanonicalS(), true);
            let out: [128]u8 = [0 as u8; 128];
            const n: usize = sig.toHex(out[0..128]);
            expect_eq(n, 128 as usize);
            let i: usize = 0;
            while (i < 128) {
                expect_eq(out[i], hex_sig[i]);
                i += 1;
            }
        } else {
            expect_eq(true, false);
        }
        // Invalid length / non-hex rejected
        expect_eq(Ed25519Signature.fromHex("deadbeef") == null, true);
    });

    test("4. @cap/ed25519 — Ed25519PublicKey RFC 8410 SPKI DER roundtrip (OID 1.3.101.112)", () => {
        const pk_hex = "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a";
        if (pk := Ed25519PublicKey.fromHex(pk_hex)) {
            let der: [44]u8 = [0 as u8; 44];
            const n: usize = pk.toSpkiDer(der[0..44]);
            expect_eq(n, 44 as usize);
            // Verify OID 1.3.101.112 bytes at offsets 4..9
            expect_eq(der[0], 0x30 as u8);
            expect_eq(der[1], 0x2A as u8);
            expect_eq(der[4], 0x06 as u8);
            expect_eq(der[6], 0x2B as u8);
            expect_eq(der[7], 0x65 as u8);
            expect_eq(der[8], 0x70 as u8);

            if (pk2 := Ed25519PublicKey.fromSpkiDer(der[0..44])) {
                expect_eq(pk.eql(pk2), true);
            } else {
                expect_eq(true, false);
            }
            // Corrupted OID rejected
            der[8] = 0x71 as u8;
            expect_eq(Ed25519PublicKey.fromSpkiDer(der[0..44]) == null, true);
        } else {
            expect_eq(true, false);
        }
    });

    test("5. @cap/ed25519 — Ed25519SecretKey RFC 8410 PKCS#8 v1 DER roundtrip (OID 1.3.101.112)", () => {
        const sk_hex = "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60";
        if (sk := Ed25519SecretKey.fromHex(sk_hex)) {
            let der: [48]u8 = [0 as u8; 48];
            const n: usize = sk.toPkcs8Der(der[0..48]);
            expect_eq(n, 48 as usize);
            expect_eq(der[0], 0x30 as u8);
            expect_eq(der[1], 0x2E as u8);
            expect_eq(der[9], 0x2B as u8);
            expect_eq(der[10], 0x65 as u8);
            expect_eq(der[11], 0x70 as u8);
            expect_eq(der[14], 0x04 as u8);
            expect_eq(der[15], 0x20 as u8);

            if (sk2 := Ed25519SecretKey.fromPkcs8Der(der[0..48])) {
                expect_eq(sk.eql(sk2), true);
            } else {
                expect_eq(true, false);
            }
            // Wrong version byte rejected
            der[4] = 0x01 as u8;
            expect_eq(Ed25519SecretKey.fromPkcs8Der(der[0..48]) == null, true);
        } else {
            expect_eq(true, false);
        }
    });

    test("6. @cap/ed25519 — fromSlice length validation & small output buffer protection", () => {
        const short: [16]u8 = [0x42 as u8; 16];
        expect_eq(Ed25519Signature.fromSlice(short[0..16]) == null, true);
        expect_eq(Ed25519PublicKey.fromSlice(short[0..16]) == null, true);
        expect_eq(Ed25519SecretKey.fromSlice(short[0..16]) == null, true);

        const raw_k: [32]u8 = [0x33 as u8; 32];
        const pk = Ed25519PublicKey.fromBytes(raw_k);
        const sk = Ed25519SecretKey.fromBytes(raw_k);
        let tiny: [10]u8 = [0 as u8; 10];
        expect_eq(pk.toSpkiDer(tiny[0..10]), 0 as usize);
        expect_eq(sk.toPkcs8Der(tiny[0..10]), 0 as usize);
        expect_eq(pk.toHex(tiny[0..10]), 0 as usize);
    });
}
