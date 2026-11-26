// cap/test/testCapConstOid.cp
// Comprehensive unit tests for @cap/const-oid (ITU-T X.690 §8.19)

import { test, expect_eq } from "cap:test";
import {
    OID_TAG,
    OID_RSA_ENCRYPTION,
    OID_SHA256_WITH_RSA,
    OID_EC_PUBLIC_KEY,
    OID_PRIME256V1,
    OID_ED25519,
    OID_COMMON_NAME,
    OID_SHA256,
    ObjectIdentifier,
} from "../../packages/@cap/const-oid/src/index.cp";

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

fn strEqBytes(s: string, b: []const u8): bool {
    if (s.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/const-oid — Standard RSA/ECDSA/Ed25519/CommonName string <-> DER exact bytes", () => {
        // rsaEncryption: 1.2.840.113549.1.1.1 -> 2a 86 48 86 f7 0d 01 01 01
        const r_rsa = ObjectIdentifier.fromStr(OID_RSA_ENCRYPTION);
        expect_eq(r_rsa.ok, true);
        expect_eq(r_rsa.oid.arcCount(), 7 as usize);
        expect_eq(r_rsa.oid.arcAt(0), 1 as u32);
        expect_eq(r_rsa.oid.arcAt(2), 840 as u32);
        expect_eq(r_rsa.oid.arcAt(3), 113549 as u32);

        let der_buf: [64]u8 = undefined;
        const rsa_dlen: usize = r_rsa.oid.toDer(der_buf[0..64]);
        let exp_rsa: [9]u8 = [0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01];
        expect_eq(rsa_dlen, 9 as usize);
        expect_eq(bytesEq(der_buf[0..rsa_dlen], exp_rsa[0..9]), true);

        const r_rsa_der = ObjectIdentifier.fromDer(exp_rsa[0..9]);
        expect_eq(r_rsa_der.ok, true);
        expect_eq(r_rsa_der.oid.eq(r_rsa.oid), true);

        let str_buf: [64]u8 = undefined;
        const rsa_slen: usize = r_rsa_der.oid.toStr(str_buf[0..64]);
        expect_eq(strEqBytes(OID_RSA_ENCRYPTION, str_buf[0..rsa_slen]), true);

        // sha256WithRSAEncryption: 1.2.840.113549.1.1.11 -> 2a 86 48 86 f7 0d 01 01 0b
        const r_sha256_rsa = ObjectIdentifier.fromStr(OID_SHA256_WITH_RSA);
        expect_eq(r_sha256_rsa.ok, true);
        let exp_sha256_rsa: [9]u8 = [0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b];
        expect_eq(r_sha256_rsa.oid.toDer(der_buf[0..64]), 9 as usize);
        expect_eq(bytesEq(der_buf[0..9], exp_sha256_rsa[0..9]), true);

        // ecPublicKey: 1.2.840.10045.2.1 -> 2a 86 48 ce 3d 02 01
        const r_ec = ObjectIdentifier.fromStr(OID_EC_PUBLIC_KEY);
        expect_eq(r_ec.ok, true);
        let exp_ec: [7]u8 = [0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01];
        expect_eq(r_ec.oid.toDer(der_buf[0..64]), 7 as usize);
        expect_eq(bytesEq(der_buf[0..7], exp_ec[0..7]), true);

        // prime256v1: 1.2.840.10045.3.1.7 -> 2a 86 48 ce 3d 03 01 07
        const r_p256 = ObjectIdentifier.fromStr(OID_PRIME256V1);
        expect_eq(r_p256.ok, true);
        let exp_p256: [8]u8 = [0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07];
        expect_eq(r_p256.oid.toDer(der_buf[0..64]), 8 as usize);
        expect_eq(bytesEq(der_buf[0..8], exp_p256[0..8]), true);

        // Ed25519: 1.3.101.112 -> 2b 65 70
        const r_ed = ObjectIdentifier.fromStr(OID_ED25519);
        expect_eq(r_ed.ok, true);
        let exp_ed: [3]u8 = [0x2b, 0x65, 0x70];
        expect_eq(r_ed.oid.toDer(der_buf[0..64]), 3 as usize);
        expect_eq(bytesEq(der_buf[0..3], exp_ed[0..3]), true);

        // commonName: 2.5.4.3 -> 55 04 03
        const r_cn = ObjectIdentifier.fromStr(OID_COMMON_NAME);
        expect_eq(r_cn.ok, true);
        let exp_cn: [3]u8 = [0x55, 0x04, 0x03];
        expect_eq(r_cn.oid.toDer(der_buf[0..64]), 3 as usize);
        expect_eq(bytesEq(der_buf[0..3], exp_cn[0..3]), true);
    });

    test("2. @cap/const-oid — arc0=2 with arc1>=40 (2.999.3) & SHA-256 (2.16.840.1.101.3.4.2.1)", () => {
        // 2.999.3: 40*2 + 999 = 1079 = 0x88 0x37, then 0x03
        const r_999 = ObjectIdentifier.fromStr("2.999.3");
        expect_eq(r_999.ok, true);
        expect_eq(r_999.oid.arcCount(), 3 as usize);
        expect_eq(r_999.oid.arcAt(0), 2 as u32);
        expect_eq(r_999.oid.arcAt(1), 999 as u32);
        expect_eq(r_999.oid.arcAt(2), 3 as u32);

        let der_buf: [64]u8 = undefined;
        let exp_999: [3]u8 = [0x88, 0x37, 0x03];
        expect_eq(r_999.oid.toDer(der_buf[0..64]), 3 as usize);
        expect_eq(bytesEq(der_buf[0..3], exp_999[0..3]), true);

        const r_999_back = ObjectIdentifier.fromDer(exp_999[0..3]);
        expect_eq(r_999_back.ok, true);
        expect_eq(r_999_back.oid.eq(r_999.oid), true);
        let str_buf: [64]u8 = undefined;
        const slen: usize = r_999_back.oid.toStr(str_buf[0..64]);
        expect_eq(strEqBytes("2.999.3", str_buf[0..slen]), true);

        // SHA-256: 2.16.840.1.101.3.4.2.1 -> 60 86 48 01 65 03 04 02 01
        const r_sha = ObjectIdentifier.fromStr(OID_SHA256);
        expect_eq(r_sha.ok, true);
        let exp_sha: [9]u8 = [0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01];
        expect_eq(r_sha.oid.toDer(der_buf[0..64]), 9 as usize);
        expect_eq(bytesEq(der_buf[0..9], exp_sha[0..9]), true);

        const r_sha_back = ObjectIdentifier.fromDer(exp_sha[0..9]);
        expect_eq(r_sha_back.ok, true);
        expect_eq(r_sha_back.oid.eq(r_sha.oid), true);
    });

    test("3. @cap/const-oid — DER TLV (tag 0x06 + length + value) round-trip", () => {
        expect_eq(OID_TAG, 0x06 as u8);
        const r_rsa = ObjectIdentifier.fromStr(OID_RSA_ENCRYPTION);
        expect_eq(r_rsa.ok, true);

        let tlv: [64]u8 = undefined;
        const tlen: usize = r_rsa.oid.toDerTlv(tlv[0..64]);
        expect_eq(tlen, 11 as usize);

        let exp_tlv: [11]u8 = [0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01];
        expect_eq(bytesEq(tlv[0..tlen], exp_tlv[0..11]), true);

        const r_back = ObjectIdentifier.fromDerTlv(tlv[0..tlen]);
        expect_eq(r_back.ok, true);
        expect_eq(r_back.oid.eq(r_rsa.oid), true);

        // Wrong tag or mismatched length rejected
        tlv[0] = 0x05;
        expect_eq(ObjectIdentifier.fromDerTlv(tlv[0..tlen]).ok, false);
        tlv[0] = 0x06;
        expect_eq(ObjectIdentifier.fromDerTlv(tlv[0..10]).ok, false);
        expect_eq(r_rsa.oid.toDerTlv(tlv[0..10]), 0 as usize);
    });

    test("4. @cap/const-oid — Equality and isPrefixOf hierarchy checks", () => {
        const pkcs1 = ObjectIdentifier.fromStr("1.2.840.113549.1.1").oid;
        const rsa_enc = ObjectIdentifier.fromStr(OID_RSA_ENCRYPTION).oid;
        const sha256_rsa = ObjectIdentifier.fromStr(OID_SHA256_WITH_RSA).oid;
        const ed25519 = ObjectIdentifier.fromStr(OID_ED25519).oid;

        expect_eq(pkcs1.isPrefixOf(rsa_enc), true);
        expect_eq(pkcs1.isPrefixOf(sha256_rsa), true);
        expect_eq(rsa_enc.isPrefixOf(rsa_enc), true);
        expect_eq(rsa_enc.isPrefixOf(pkcs1), false);
        expect_eq(rsa_enc.isPrefixOf(sha256_rsa), false);
        expect_eq(pkcs1.isPrefixOf(ed25519), false);

        expect_eq(rsa_enc.eq(sha256_rsa), false);
        expect_eq(rsa_enc.eq(ObjectIdentifier.fromStr("1.2.840.113549.1.1.1").oid), true);
    });

    test("5. @cap/const-oid — Strictly reject invalid OID strings", () => {
        // arc0 > 2
        expect_eq(ObjectIdentifier.fromStr("3.1").ok, false);
        // arc0 < 2 && arc1 > 39
        expect_eq(ObjectIdentifier.fromStr("1.40").ok, false);
        expect_eq(ObjectIdentifier.fromStr("0.40").ok, false);
        // Leading zeros
        expect_eq(ObjectIdentifier.fromStr("1.02").ok, false);
        expect_eq(ObjectIdentifier.fromStr("01.2.3").ok, false);
        expect_eq(ObjectIdentifier.fromStr("1.2.00").ok, false);
        // Too few arcs / empty / trailing or double dots
        expect_eq(ObjectIdentifier.fromStr("").ok, false);
        expect_eq(ObjectIdentifier.fromStr("1").ok, false);
        expect_eq(ObjectIdentifier.fromStr("1.2.").ok, false);
        expect_eq(ObjectIdentifier.fromStr(".1.2").ok, false);
        expect_eq(ObjectIdentifier.fromStr("1..2").ok, false);
        // Non-digit characters & u32 overflow
        expect_eq(ObjectIdentifier.fromStr("1.2.abc").ok, false);
        expect_eq(ObjectIdentifier.fromStr("1.2.4294967296").ok, false);
        // Valid boundary: "0.0" and "1.39" and "2.0"
        expect_eq(ObjectIdentifier.fromStr("0.0").ok, true);
        expect_eq(ObjectIdentifier.fromStr("1.39").ok, true);
        expect_eq(ObjectIdentifier.fromStr("2.0").ok, true);
    });

    test("6. @cap/const-oid — Strictly reject non-canonical DER (0x80 leading) and truncated VLQ", () => {
        // Empty DER
        let empty: [1]u8 = [0];
        expect_eq(ObjectIdentifier.fromDer(empty[0..0]).ok, false);

        // Non-canonical leading 0x80 on first subidentifier
        let bad_first: [3]u8 = [0x80, 0x2a, 0x01];
        expect_eq(ObjectIdentifier.fromDer(bad_first[0..3]).ok, false);

        // Non-canonical leading 0x80 on later subidentifier
        let bad_mid: [4]u8 = [0x2a, 0x80, 0x01, 0x02];
        expect_eq(ObjectIdentifier.fromDer(bad_mid[0..4]).ok, false);

        // Truncated multi-byte VLQ (high bit set on final byte)
        let trunc1: [1]u8 = [0x86];
        expect_eq(ObjectIdentifier.fromDer(trunc1[0..1]).ok, false);

        let trunc2: [3]u8 = [0x2a, 0x86, 0xf7];
        expect_eq(ObjectIdentifier.fromDer(trunc2[0..3]).ok, false);

        // u32 overflow on later subidentifier (> 0xFFFFFFFF: 5 bytes 0x90 0x80 0x80 0x80 0x00)
        let ovf: [6]u8 = [0x2a, 0x90, 0x80, 0x80, 0x80, 0x00];
        expect_eq(ObjectIdentifier.fromDer(ovf[0..6]).ok, false);
    });
}
