// cap/test/testCapX509Cert.cp
// Comprehensive unit tests for @cap/x509-cert (RFC 5280 X.509 v1/v3 DER & PEM)

import { test, expect_eq } from "cap:test";
import { OID_ED25519, ObjectIdentifier } from "../../packages/const-oid/src/index.cp";
import { AlgorithmIdentifier, SubjectPublicKeyInfo } from "../../packages/spki/src/index.cp";
import {
    X509_ERR_INVALID_VERSION,
    X509_ERR_INVALID_PEM_LABEL,
    X509Validity,
    X509Name,
    X509Certificate,
    encodeX509Der,
    decodeX509Der,
    encodeX509Pem,
    decodeX509Pem,
    isSelfSigned,
} from "../../packages/x509-cert/src/index.cp";

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

fn makeSampleEd25519Cert(version: u8, issuer: X509Name, subject: X509Name, validity: X509Validity): X509Certificate {
    let cert = X509Certificate.empty();
    cert.version = version;
    cert.serial[0] = 0x01;
    cert.serial[1] = 0x23;
    cert.serial[2] = 0x45;
    cert.serial[3] = 0x67;
    cert.serial_len = 4;

    const ed_oid = ObjectIdentifier.fromStr(OID_ED25519).oid;
    cert.signature_alg = AlgorithmIdentifier.fromOid(ed_oid);
    cert.issuer = issuer;
    cert.validity = validity;
    cert.subject = subject;

    let pub32: [32]u8 = [
        0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
        0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
        0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
        0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a
    ];
    cert.spki = SubjectPublicKeyInfo.ed25519(pub32[0..32]).spki;

    let i: usize = 0;
    while (i < 64) {
        cert.signature[i] = ((0xB0 + i) & 0xFF) as u8;
        i = i + 1;
    }
    cert.signature_len = 64;
    return cert;
}

fn main() {
    test("1. @cap/x509-cert — v3 Ed25519 self-signed certificate DER encode & decode round-trip", () => {
        const dn = X509Name.init("cap.example.org", "Cap PKI", "US");
        const val = X509Validity.fromStrs("260101000000Z", "360101000000Z");
        const cert = makeSampleEd25519Cert(3 as u8, dn, dn, val);

        let der: [1024]u8 = undefined;
        const dlen = encodeX509Der(cert, der[0..1024]);
        expect_eq(dlen > (150 as i64), true);

        const dec_res = decodeX509Der(der[0..(dlen as usize)]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.cert.version, 3 as u8);
        expect_eq(dec_res.cert.serial_len, 4 as usize);
        expect_eq(dec_res.cert.serial[0], 0x01 as u8);
        expect_eq(dec_res.cert.serial[3], 0x67 as u8);
        expect_eq(dec_res.cert.signature_alg.oid.eq(ObjectIdentifier.fromStr(OID_ED25519).oid), true);
        expect_eq(dec_res.cert.spki.key_len, 32 as usize);
        expect_eq(dec_res.cert.spki.key_bytes[0], 0xd7 as u8);
        expect_eq(dec_res.cert.signature_len, 64 as usize);
        expect_eq(bytesEq(dec_res.cert.signature[0..64], cert.signature[0..64]), true);
        expect_eq(isSelfSigned(dec_res.cert), true);
    });

    test("2. @cap/x509-cert — v1 certificate without [0] explicit version tag defaults to v1", () => {
        const dn = X509Name.init("legacy-v1.local", "Legacy Org", "GB");
        const val = X509Validity.fromStrs("250101000000Z", "300101000000Z");
        const cert_v1 = makeSampleEd25519Cert(1 as u8, dn, dn, val);
        const cert_v3 = makeSampleEd25519Cert(3 as u8, dn, dn, val);

        let der_v1: [1024]u8 = undefined;
        let der_v3: [1024]u8 = undefined;
        const len_v1 = encodeX509Der(cert_v1, der_v1[0..1024]);
        const len_v3 = encodeX509Der(cert_v3, der_v3[0..1024]);
        expect_eq(len_v1 > (100 as i64), true);
        // v1 omits the 5-byte [0] EXPLICIT version (A0 03 02 01 02)
        expect_eq(len_v3 - len_v1, 5 as i64);

        const dec_v1 = decodeX509Der(der_v1[0..(len_v1 as usize)]);
        expect_eq(dec_v1.ok, true);
        expect_eq(dec_v1.cert.version, 1 as u8);
        expect_eq(strEqBytes("legacy-v1.local", dec_v1.cert.subject.cn[0..dec_v1.cert.subject.cn_len]), true);
    });

    test("3. @cap/x509-cert — DN extraction (CN/O/C) & isSelfSigned distinction", () => {
        const issuer = X509Name.init("Cap Root CA", "Cap Trust Foundation", "CN");
        const subject = X509Name.init("api.cap.dev", "Cap Cloud", "US");
        const val = X509Validity.fromStrs("260101000000Z", "270101000000Z");
        const leaf = makeSampleEd25519Cert(3 as u8, issuer, subject, val);

        let der: [1024]u8 = undefined;
        const dlen = encodeX509Der(leaf, der[0..1024]);
        expect_eq(dlen > (150 as i64), true);

        const dec_res = decodeX509Der(der[0..(dlen as usize)]);
        expect_eq(dec_res.ok, true);
        expect_eq(isSelfSigned(dec_res.cert), false);
        expect_eq(strEqBytes("Cap Root CA", dec_res.cert.issuer.cn[0..dec_res.cert.issuer.cn_len]), true);
        expect_eq(strEqBytes("Cap Trust Foundation", dec_res.cert.issuer.org[0..dec_res.cert.issuer.org_len]), true);
        expect_eq(strEqBytes("CN", dec_res.cert.issuer.country[0..dec_res.cert.issuer.country_len]), true);
        expect_eq(strEqBytes("api.cap.dev", dec_res.cert.subject.cn[0..dec_res.cert.subject.cn_len]), true);
        expect_eq(strEqBytes("Cap Cloud", dec_res.cert.subject.org[0..dec_res.cert.subject.org_len]), true);
        expect_eq(strEqBytes("US", dec_res.cert.subject.country[0..dec_res.cert.subject.country_len]), true);
    });

    test("4. @cap/x509-cert — Mixed UTCTime (0x17) & GeneralizedTime (0x18) validity", () => {
        const dn = X509Name.init("long-lived.cap", "Cap CA", "DE");
        // notBefore in 2026 (UTCTime), notAfter in 2099 (GeneralizedTime)
        const val = X509Validity.fromStrs("260615120000Z", "20991231235959Z");
        const cert = makeSampleEd25519Cert(3 as u8, dn, dn, val);

        let der: [1024]u8 = undefined;
        const dlen = encodeX509Der(cert, der[0..1024]);
        expect_eq(dlen > (150 as i64), true);

        const dec_res = decodeX509Der(der[0..(dlen as usize)]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.cert.validity.not_before_generalized, false);
        expect_eq(dec_res.cert.validity.not_after_generalized, true);
        expect_eq(strEqBytes("260615120000Z", dec_res.cert.validity.not_before[0..dec_res.cert.validity.not_before_len]), true);
        expect_eq(strEqBytes("20991231235959Z", dec_res.cert.validity.not_after[0..dec_res.cert.validity.not_after_len]), true);
    });

    test("5. @cap/x509-cert — RFC 7468 CERTIFICATE PEM encode & decode round-trip", () => {
        const dn = X509Name.init("pem-test.cap", "Cap Labs", "JP");
        const val = X509Validity.fromStrs("260101000000Z", "280101000000Z");
        const cert = makeSampleEd25519Cert(3 as u8, dn, dn, val);

        let pem_buf: [2048]u8 = undefined;
        const plen = encodeX509Pem(cert, false, pem_buf[0..2048]);
        expect_eq(plen > (200 as i64), true);

        // Verify BEGIN CERTIFICATE header
        const hdr = "-----BEGIN CERTIFICATE-----";
        let hdr_ok = true;
        let i: usize = 0;
        while (i < hdr.len) {
            if (pem_buf[i] != hdr[i]) {
                hdr_ok = false;
            }
            i = i + 1;
        }
        expect_eq(hdr_ok, true);

        const dec_res = decodeX509Pem(pem_buf[0..(plen as usize)]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.cert.version, 3 as u8);
        expect_eq(strEqBytes("pem-test.cap", dec_res.cert.subject.cn[0..dec_res.cert.subject.cn_len]), true);
        expect_eq(isSelfSigned(dec_res.cert), true);
    });

    test("6. @cap/x509-cert — Reject wrong PEM label (PUBLIC KEY), truncated DER & invalid version", () => {
        // Wrong PEM label: PUBLIC KEY instead of CERTIFICATE
        const pub_pem = "-----BEGIN PUBLIC KEY-----\nMCowBQYDK2VwAyEA11qYAYKxCrfVS/7TyWQHOg7hcvPapiMlrwIaaPcHURo=\n-----END PUBLIC KEY-----\n";
        const r_pem = decodeX509Pem(pub_pem);
        expect_eq(r_pem.ok, false);
        expect_eq(r_pem.err, X509_ERR_INVALID_PEM_LABEL);

        // Truncated DER
        const dn = X509Name.init("trunc.cap", "Cap", "US");
        const val = X509Validity.fromStrs("260101000000Z", "270101000000Z");
        const cert = makeSampleEd25519Cert(3 as u8, dn, dn, val);
        let der: [1024]u8 = undefined;
        const dlen = encodeX509Der(cert, der[0..1024]);
        expect_eq(dlen > (100 as i64), true);
        expect_eq(decodeX509Der(der[0..((dlen as usize) - 1)]).ok, false);
        expect_eq(decodeX509Der(der[0..12]).ok, false);

        // Malformed version = 4 (DER integer value 3 > max allowed 2)
        // Outer SEQ(12) { TBS SEQ(10) { [0](3) { INT(1) = 3 } ... } }
        let bad_ver: [12]u8 = [0x30, 0x0a, 0x30, 0x08, 0xa0, 0x03, 0x02, 0x01, 0x03, 0x02, 0x01, 0x01];
        const r_ver = decodeX509Der(bad_ver[0..12]);
        expect_eq(r_ver.ok, false);
        expect_eq(r_ver.err, X509_ERR_INVALID_VERSION);
    });
}
