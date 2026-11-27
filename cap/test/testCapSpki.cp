// cap/test/testCapSpki.cp
// Comprehensive unit tests for @cap/spki (RFC 5280 & RFC 8410)

import { test, expect_eq } from "cap:test";
import {
    OID_RSA_ENCRYPTION,
    OID_EC_PUBLIC_KEY,
    OID_PRIME256V1,
    OID_ED25519,
    ObjectIdentifier,
} from "../../packages/const-oid/src/index.cp";
import {
    SPKI_ERR_INVALID_PEM_LABEL,
    SPKI_ERR_NON_ZERO_UNUSED_BITS,
    AlgorithmIdentifier,
    SubjectPublicKeyInfo,
    encodeAlgorithmId,
    decodeAlgorithmId,
    encodeSpkiDer,
    decodeSpkiDer,
    encodeSpkiPem,
    decodeSpkiPem,
    fingerprintSpki,
} from "../../packages/spki/src/index.cp";

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
    test("1. @cap/spki — RFC 8410 Ed25519 SPKI official DER exact byte comparison", () => {
        // RFC 8410 §10.1 Ed25519 raw 32-byte public key
        let pub32: [32]u8 = [
            0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
            0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
            0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
            0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a
        ];

        const mk = SubjectPublicKeyInfo.ed25519(pub32[0..32]);
        expect_eq(mk.ok, true);

        let der: [128]u8 = undefined;
        const dlen = encodeSpkiDer(mk.spki, der[0..128]);
        expect_eq(dlen, 44 as i64);

        // RFC 8410 exact SPKI DER: 30 2a 30 05 06 03 2b 65 70 03 21 00 || pub32
        let expected_prefix: [12]u8 = [
            0x30, 0x2a,
            0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70,
            0x03, 0x21, 0x00
        ];
        expect_eq(bytesEq(der[0..12], expected_prefix[0..12]), true);
        expect_eq(bytesEq(der[12..44], pub32[0..32]), true);

        const dec_res = decodeSpkiDer(der[0..44]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.spki.algorithm.oid.eq(ObjectIdentifier.fromStr(OID_ED25519).oid), true);
        expect_eq(dec_res.spki.algorithm.has_params, false);
        expect_eq(dec_res.spki.key_len, 32 as usize);
        expect_eq(dec_res.spki.unused_bits, 0 as u8);
        expect_eq(bytesEq(dec_res.spki.key_bytes[0..32], pub32[0..32]), true);

        // Deterministic fingerprint non-zero & stable
        const fp1 = fingerprintSpki(der[0..44]);
        const fp2 = fingerprintSpki(der[0..44]);
        expect_eq(fp1, fp2);
        expect_eq(fp1 != (0 as u64), true);
    });

    test("2. @cap/spki — NIST P-256 ECDSA SPKI with namedCurve OID parameter", () => {
        // 65-byte uncompressed SEC1 point (0x04 || X || Y)
        let p256_pub: [65]u8 = undefined;
        p256_pub[0] = 0x04;
        let i: usize = 1;
        while (i < 65) {
            p256_pub[i] = ((i * 5 + 3) & 0xFF) as u8;
            i = i + 1;
        }

        const mk = SubjectPublicKeyInfo.p256(p256_pub[0..65]);
        expect_eq(mk.ok, true);

        let der: [160]u8 = undefined;
        const dlen = encodeSpkiDer(mk.spki, der[0..160]);
        // AlgId = 30 13 (06 07 2a 86 48 ce 3d 02 01 + 06 08 2a 86 48 ce 3d 03 01 07) = 21 bytes
        // BitString = 03 42 00 || 65 bytes = 68 bytes
        // Outer SEQ = 30 59 || 89 bytes = 91 bytes total
        expect_eq(dlen, 91 as i64);

        let expected_hdr: [26]u8 = [
            0x30, 0x59,
            0x30, 0x13,
            0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01,
            0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07,
            0x03, 0x42, 0x00
        ];
        expect_eq(bytesEq(der[0..26], expected_hdr[0..26]), true);
        expect_eq(bytesEq(der[26..91], p256_pub[0..65]), true);

        const dec_res = decodeSpkiDer(der[0..91]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.spki.algorithm.oid.eq(ObjectIdentifier.fromStr(OID_EC_PUBLIC_KEY).oid), true);
        expect_eq(dec_res.spki.algorithm.has_params, true);
        expect_eq(dec_res.spki.algorithm.has_param_oid, true);
        expect_eq(dec_res.spki.algorithm.param_is_null, false);
        expect_eq(dec_res.spki.algorithm.param_oid.eq(ObjectIdentifier.fromStr(OID_PRIME256V1).oid), true);
        expect_eq(dec_res.spki.key_len, 65 as usize);
        expect_eq(bytesEq(dec_res.spki.key_bytes[0..65], p256_pub[0..65]), true);
    });

    test("3. @cap/spki — RSA AlgorithmIdentifier with NULL parameter encode & decode", () => {
        const rsa_oid = ObjectIdentifier.fromStr(OID_RSA_ENCRYPTION).oid;
        const alg = AlgorithmIdentifier.withNull(rsa_oid);

        let der: [64]u8 = undefined;
        const alen = encodeAlgorithmId(alg, der[0..64]);
        // 30 0d 06 09 2a 86 48 86 f7 0d 01 01 01 05 00 (15 bytes)
        expect_eq(alen, 15 as i64);

        let expected: [15]u8 = [
            0x30, 0x0d,
            0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01,
            0x05, 0x00
        ];
        expect_eq(bytesEq(der[0..15], expected[0..15]), true);

        const dec_res = decodeAlgorithmId(der[0..15]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.consumed, 15 as usize);
        expect_eq(dec_res.alg.eq(alg), true);
        expect_eq(dec_res.alg.has_params, true);
        expect_eq(dec_res.alg.param_is_null, true);
        expect_eq(dec_res.alg.has_param_oid, false);
    });

    test("4. @cap/spki — RFC 7468 PUBLIC KEY PEM encode & round-trip decode", () => {
        // Official RFC 8410 Ed25519 SPKI PEM
        const rfc_pem = "-----BEGIN PUBLIC KEY-----\nMCowBQYDK2VwAyEA11qYAYKxCrfVS/7TyWQHOg7hcvPapiMlrwIaaPcHURo=\n-----END PUBLIC KEY-----\n";
        const dec_res = decodeSpkiPem(rfc_pem);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.spki.key_len, 32 as usize);
        expect_eq(dec_res.spki.key_bytes[0], 0xd7 as u8);
        expect_eq(dec_res.spki.key_bytes[31], 0x1a as u8);

        // Re-encode to PEM and verify exact byte-for-byte match with RFC 8410 PEM
        let pem_out: [256]u8 = undefined;
        const plen = encodeSpkiPem(dec_res.spki, false, pem_out[0..256]);
        expect_eq(plen, rfc_pem.len as i64);

        let i: usize = 0;
        let matches = true;
        while (i < rfc_pem.len) {
            if (pem_out[i] != rfc_pem[i]) {
                matches = false;
            }
            i = i + 1;
        }
        expect_eq(matches, true);
    });

    test("5. @cap/spki — Reject wrong PEM label such as PRIVATE KEY or CERTIFICATE", () => {
        const wrong_priv_pem = "-----BEGIN PRIVATE KEY-----\nMCowBQYDK2VwAyEA11qYAYKxCrfVS/7TyWQHOg7hcvPapiMlrwIaaPcHURo=\n-----END PRIVATE KEY-----\n";
        const r1 = decodeSpkiPem(wrong_priv_pem);
        expect_eq(r1.ok, false);
        expect_eq(r1.err, SPKI_ERR_INVALID_PEM_LABEL);

        const wrong_cert_pem = "-----BEGIN CERTIFICATE-----\nMCowBQYDK2VwAyEA11qYAYKxCrfVS/7TyWQHOg7hcvPapiMlrwIaaPcHURo=\n-----END CERTIFICATE-----\n";
        const r2 = decodeSpkiPem(wrong_cert_pem);
        expect_eq(r2.ok, false);
        expect_eq(r2.err, SPKI_ERR_INVALID_PEM_LABEL);
    });

    test("6. @cap/spki — Reject malformed DER, non-zero unused_bits, and truncated data", () => {
        // Valid 44-byte Ed25519 SPKI baseline
        let pub32: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            pub32[i] = (i + 1) as u8;
            i = i + 1;
        }
        const mk = SubjectPublicKeyInfo.ed25519(pub32[0..32]);
        let der: [64]u8 = undefined;
        const dlen = encodeSpkiDer(mk.spki, der[0..64]);
        expect_eq(dlen, 44 as i64);

        // Non-zero unused_bits in BIT STRING (der[11] = 0x01) -> rejected
        der[11] = 0x01;
        const r_ub = decodeSpkiDer(der[0..44]);
        expect_eq(r_ub.ok, false);
        expect_eq(r_ub.err, SPKI_ERR_NON_ZERO_UNUSED_BITS);
        der[11] = 0x00;

        // Truncated DER -> rejected
        expect_eq(decodeSpkiDer(der[0..43]).ok, false);
        expect_eq(decodeSpkiDer(der[0..10]).ok, false);
        expect_eq(decodeSpkiDer(der[0..0]).ok, false);

        // Trailing garbage byte -> rejected
        der[44] = 0x00;
        expect_eq(decodeSpkiDer(der[0..45]).ok, false);

        // Wrong outer tag (0x31 SET instead of 0x30 SEQUENCE) -> rejected
        der[0] = 0x31;
        expect_eq(decodeSpkiDer(der[0..44]).ok, false);
    });
}
