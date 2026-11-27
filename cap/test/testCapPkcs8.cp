// cap/test/testCapPkcs8.cp
// Comprehensive unit tests for @cap/pkcs8

import { test, expect_eq } from "cap:test";
import {
    OID_ED25519,
    OID_EC_PUBLIC_KEY,
    OID_PRIME256V1,
    ObjectIdentifier,
} from "../../packages/const-oid/src/index.cp";
import {
    PK8_VERSION_V1,
    PK8_VERSION_V2,
    PK8_ERR_INVALID_VERSION,
    PK8_ERR_TRUNCATED,
    PK8_ERR_INVALID_PEM_LABEL,
    PK8_ERR_INVALID_PUBLIC_KEY,
    PrivateKeyInfo,
    encodePkcs8Der,
    decodePkcs8Der,
    encodePkcs8Pem,
    decodePkcs8Pem,
} from "../../packages/pkcs8/src/index.cp";

fn pk8TestEqBytes(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/pkcs8 — RFC 8410 §10.3 Ed25519 official standard DER exact bytes", () => {
        // RFC 8410 §10.3 test vector seed:
        // D4EE72DBF913584AD5B6D8F1F769F8AD3AFE7C28CBF1D4FBE097A88F44755842
        let seed: [32]u8 = [
            0xD4 as u8, 0xEE as u8, 0x72 as u8, 0xDB as u8,
            0xF9 as u8, 0x13 as u8, 0x58 as u8, 0x4A as u8,
            0xD5 as u8, 0xB6 as u8, 0xD8 as u8, 0xF1 as u8,
            0xF7 as u8, 0x69 as u8, 0xF8 as u8, 0xAD as u8,
            0x3A as u8, 0xFE as u8, 0x7C as u8, 0x28 as u8,
            0xCB as u8, 0xF1 as u8, 0xD4 as u8, 0xFB as u8,
            0xE0 as u8, 0x97 as u8, 0xA8 as u8, 0x8F as u8,
            0x44 as u8, 0x75 as u8, 0x58 as u8, 0x42 as u8,
        ];
        const mk = PrivateKeyInfo.ed25519(seed[0..32]);
        expect_eq(mk.ok, true);

        let der_out: [64]u8 = [0; 64];
        const dlen = encodePkcs8Der(mk.info, der_out[0..64]);
        // 48 bytes total: 30 2e 02 01 00 30 05 06 03 2b 65 70 04 22 04 20 || seed32
        expect_eq(dlen, 48 as i64);

        let expected_prefix: [16]u8 = [
            0x30 as u8, 0x2E as u8, 0x02 as u8, 0x01 as u8,
            0x00 as u8, 0x30 as u8, 0x05 as u8, 0x06 as u8,
            0x03 as u8, 0x2B as u8, 0x65 as u8, 0x70 as u8,
            0x04 as u8, 0x22 as u8, 0x04 as u8, 0x20 as u8,
        ];
        expect_eq(pk8TestEqBytes(der_out[0..16], expected_prefix[0..16]), true);
        expect_eq(pk8TestEqBytes(der_out[16..48], seed[0..32]), true);

        const decoded = decodePkcs8Der(der_out[0..48]);
        expect_eq(decoded.ok, true);
        expect_eq(decoded.info.version, PK8_VERSION_V1);
        expect_eq(decoded.info.has_public_key, false);
        expect_eq(decoded.info.private_key_len, 34 as usize);
        expect_eq(pk8TestEqBytes(decoded.info.private_key[2..34], seed[0..32]), true);
        expect_eq(decoded.info.algorithm.oid.eq(ObjectIdentifier.fromStr(OID_ED25519).oid), true);
    });

    test("2. @cap/pkcs8 — RFC 5958 v2 OneAsymmetricKey with [1] IMPLICIT BIT STRING publicKey", () => {
        let seed: [32]u8 = [0; 32];
        let pubk: [32]u8 = [0; 32];
        let i: usize = 0;
        while (i < 32) {
            seed[i] = (0x10 + i) as u8;
            pubk[i] = (0xA0 + i) as u8;
            i = i + 1;
        }
        const mk = PrivateKeyInfo.ed25519V2(seed[0..32], pubk[0..32]);
        expect_eq(mk.ok, true);
        expect_eq(mk.info.version, PK8_VERSION_V2);
        expect_eq(mk.info.has_public_key, true);

        let der_out: [128]u8 = [0; 128];
        const dlen = encodePkcs8Der(mk.info, der_out[0..128]);
        // 48 (v1) + 35 ([1] tag 0x81 + len 0x21 + unused 0x00 + 32 pub bytes) = 83 bytes
        expect_eq(dlen, 83 as i64);
        expect_eq(der_out[4], 0x01 as u8);  // version = 1 (v2)
        expect_eq(der_out[48], 0x81 as u8); // [1] IMPLICIT BIT STRING
        expect_eq(der_out[49], 0x21 as u8); // length = 33
        expect_eq(der_out[50], 0x00 as u8); // unused_bits = 0

        const decoded = decodePkcs8Der(der_out[0..83]);
        expect_eq(decoded.ok, true);
        expect_eq(decoded.info.version, PK8_VERSION_V2);
        expect_eq(decoded.info.has_public_key, true);
        expect_eq(decoded.info.public_key_len, 32 as usize);
        expect_eq(pk8TestEqBytes(decoded.info.public_key[0..32], pubk[0..32]), true);
        expect_eq(pk8TestEqBytes(decoded.info.private_key[2..34], seed[0..32]), true);
    });

    test("3. @cap/pkcs8 — NIST P-256 ECDSA PKCS#8 round-trip", () => {
        // Minimal mock ECPrivateKey DER bytes
        let ec_der: [39]u8 = [0; 39];
        ec_der[0] = 0x30 as u8;
        ec_der[1] = 0x25 as u8;
        ec_der[2] = 0x02 as u8;
        ec_der[3] = 0x01 as u8;
        ec_der[4] = 0x01 as u8;
        ec_der[5] = 0x04 as u8;
        ec_der[6] = 0x20 as u8;
        let i: usize = 0;
        while (i < 32) {
            ec_der[7 + i] = ((0x55 as u8) ^ (i as u8));
            i = i + 1;
        }
        const mk = PrivateKeyInfo.p256(ec_der[0..39]);
        expect_eq(mk.ok, true);

        let der_out: [128]u8 = [0; 128];
        const dlen = encodePkcs8Der(mk.info, der_out[0..128]);
        expect_eq(dlen > 0, true);

        const decoded = decodePkcs8Der(der_out[0..(dlen as usize)]);
        expect_eq(decoded.ok, true);
        expect_eq(decoded.info.algorithm.oid.eq(ObjectIdentifier.fromStr(OID_EC_PUBLIC_KEY).oid), true);
        expect_eq(decoded.info.algorithm.has_param_oid, true);
        expect_eq(decoded.info.algorithm.param_oid.eq(ObjectIdentifier.fromStr(OID_PRIME256V1).oid), true);
        expect_eq(decoded.info.private_key_len, 39 as usize);
        expect_eq(pk8TestEqBytes(decoded.info.private_key[0..39], ec_der[0..39]), true);
    });

    test("4. @cap/pkcs8 — RFC 7468 PRIVATE KEY PEM exact round-trip", () => {
        let seed: [32]u8 = [0; 32];
        let i: usize = 0;
        while (i < 32) {
            seed[i] = (0x31 + i) as u8;
            i = i + 1;
        }
        const mk = PrivateKeyInfo.ed25519(seed[0..32]);
        expect_eq(mk.ok, true);

        let pem_buf: [256]u8 = [0; 256];
        const plen = encodePkcs8Pem(mk.info, false, pem_buf[0..256]);
        expect_eq(plen > 0, true);
        expect_eq(pk8TestEqBytes(pem_buf[0..28], "-----BEGIN PRIVATE KEY-----\n"), true);

        const decoded = decodePkcs8Pem(pem_buf[0..(plen as usize)]);
        expect_eq(decoded.ok, true);
        expect_eq(decoded.info.version, PK8_VERSION_V1);
        expect_eq(pk8TestEqBytes(decoded.info.private_key[2..34], seed[0..32]), true);
    });

    test("5. @cap/pkcs8 — PrivateKeyInfo.zeroize() secure wiping via @cap/subtle-ct", () => {
        let seed: [32]u8 = [0xAB as u8; 32];
        let pubk: [32]u8 = [0xCD as u8; 32];
        const mk = PrivateKeyInfo.ed25519V2(seed[0..32], pubk[0..32]);
        expect_eq(mk.ok, true);

        let info = mk.info;
        expect_eq(info.private_key_len, 34 as usize);
        expect_eq(info.public_key_len, 32 as usize);
        expect_eq(info.private_key[2], 0xAB as u8);
        expect_eq(info.public_key[0], 0xCD as u8);

        info.zeroize();
        expect_eq(info.private_key_len, 0 as usize);
        expect_eq(info.public_key_len, 0 as usize);
        expect_eq(info.has_public_key, false);

        let any_nonzero = false;
        let i: usize = 0;
        while (i < 160) {
            if (info.private_key[i] != (0 as u8)) {
                any_nonzero = true;
            }
            i = i + 1;
        }
        i = 0;
        while (i < 64) {
            if (info.public_key[i] != (0 as u8)) {
                any_nonzero = true;
            }
            i = i + 1;
        }
        expect_eq(any_nonzero, false);
    });

    test("6. @cap/pkcs8 — Reject wrong PEM label PUBLIC KEY, invalid version v=2 & truncated DER", () => {
        // Wrong PEM label: PUBLIC KEY instead of PRIVATE KEY
        const wrong_pem = "-----BEGIN PUBLIC KEY-----\nMC4CAQAwBQYDK2VwBCIEINTuctv5E1hK1bbY8fdp+K06/nwoy/HU++CXqI9EdVhC\n-----END PUBLIC KEY-----\n";
        const r_pem = decodePkcs8Pem(wrong_pem);
        expect_eq(r_pem.ok, false);
        expect_eq(r_pem.err, PK8_ERR_INVALID_PEM_LABEL);

        // Invalid version = 2 (only v1=0 and v2=1 are valid)
        let bad_ver: [16]u8 = [
            0x30 as u8, 0x0E as u8,
            0x02 as u8, 0x01 as u8, 0x02 as u8, // version = 2 (invalid!)
            0x30 as u8, 0x05 as u8, 0x06 as u8, 0x03 as u8, 0x2B as u8, 0x65 as u8, 0x70 as u8,
            0x04 as u8, 0x02 as u8, 0xAA as u8, 0xBB as u8,
        ];
        const r_ver = decodePkcs8Der(bad_ver[0..16]);
        expect_eq(r_ver.ok, false);
        expect_eq(r_ver.err, PK8_ERR_INVALID_VERSION);

        // v1 with forbidden [1] publicKey field
        let v1_with_pub: [20]u8 = [
            0x30 as u8, 0x12 as u8,
            0x02 as u8, 0x01 as u8, 0x00 as u8, // version = 0 (v1)
            0x30 as u8, 0x05 as u8, 0x06 as u8, 0x03 as u8, 0x2B as u8, 0x65 as u8, 0x70 as u8,
            0x04 as u8, 0x02 as u8, 0xAA as u8, 0xBB as u8,
            0x81 as u8, 0x02 as u8, 0x00 as u8, 0x11 as u8, // forbidden in v1!
        ];
        const r_pub = decodePkcs8Der(v1_with_pub[0..20]);
        expect_eq(r_pub.ok, false);
        expect_eq(r_pub.err, PK8_ERR_INVALID_PUBLIC_KEY);

        // Truncated DER (empty & cut short)
        let empty_b: [1]u8 = [0 as u8];
        const r_empty = decodePkcs8Der(empty_b[0..0]);
        expect_eq(r_empty.ok, false);
        expect_eq(r_empty.err, PK8_ERR_TRUNCATED);

        const r_trunc = decodePkcs8Der(bad_ver[0..5]);
        expect_eq(r_trunc.ok, false);
    });
}
