// cap/test/testCapRustlsWebpki.cp
// Comprehensive unit tests for @cap/rustls-webpki (RFC 6125 & RFC 5280 WebPKI verifier)

import { test, expect_eq } from "cap:test";
import { OID_ED25519, ObjectIdentifier } from "../../packages/const-oid/src/index.cp";
import { AlgorithmIdentifier, SubjectPublicKeyInfo } from "../../packages/spki/src/index.cp";
import {
    X509Validity,
    X509Name,
    X509Certificate,
    encodeX509Der,
} from "../../packages/x509-cert/src/index.cp";
import {
    WPKI_OK,
    WPKI_ERR_NOT_YET_VALID,
    WPKI_ERR_EXPIRED,
    WPKI_ERR_INVALID_TIME,
    WPKI_ERR_HOSTNAME_MISMATCH,
    WPKI_ERR_CHAIN_BROKEN,
    WPKI_ERR_NOT_SELF_SIGNED,
    verifyDnsHostname,
    parseX509TimeUtc,
    verifyValidityAt,
    verifyCertForDnsName,
    verifyDerCertForDnsName,
    verifyChainLink,
    verifyTrustAnchor,
    verifyEndEntityChain,
} from "../../packages/rustls-webpki/src/index.cp";

fn makeTestCert(issuer: X509Name, subject: X509Name, validity: X509Validity): X509Certificate {
    let cert = X509Certificate.empty();
    cert.version = 3 as u8;
    cert.serial[0] = 0x10;
    cert.serial[1] = 0x20;
    cert.serial_len = 2;

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
        cert.signature[i] = ((0xA0 + i) & 0xFF) as u8;
        i = i + 1;
    }
    cert.signature_len = 64;
    return cert;
}

fn main() {
    test("1. @cap/rustls-webpki — RFC 6125 exact & single-label wildcard case-insensitive matching", () => {
        // Exact matches (case-insensitive)
        expect_eq(verifyDnsHostname("example.com", "example.com"), true);
        expect_eq(verifyDnsHostname("Example.COM", "example.com"), true);
        expect_eq(verifyDnsHostname("api.cap.dev", "API.CAP.DEV"), true);
        expect_eq(verifyDnsHostname("a-1.example.org", "a-1.example.org"), true);

        // Single-label leftmost wildcard matches (case-insensitive)
        expect_eq(verifyDnsHostname("*.example.com", "foo.example.com"), true);
        expect_eq(verifyDnsHostname("*.example.com", "BAR.Example.COM"), true);
        expect_eq(verifyDnsHostname("*.EXAMPLE.COM", "api-v2.example.com"), true);
        expect_eq(verifyDnsHostname("*.sub.example.com", "edge.sub.example.com"), true);

        // Different domain rejected
        expect_eq(verifyDnsHostname("example.com", "example.org"), false);
        expect_eq(verifyDnsHostname("*.example.com", "foo.other.com"), false);
    });

    test("2. @cap/rustls-webpki — RFC 6125 strict wildcard & malformed label rejections", () => {
        // Reject multi-level subdomain crossing '.'
        expect_eq(verifyDnsHostname("*.example.com", "a.b.example.com"), false);
        // Reject bare apex domain against wildcard
        expect_eq(verifyDnsHostname("*.example.com", "example.com"), false);
        // Reject top-level wildcard
        expect_eq(verifyDnsHostname("*.com", "example.com"), false);
        expect_eq(verifyDnsHostname("*", "example.com"), false);
        // Reject partial label wildcards
        expect_eq(verifyDnsHostname("f*.example.com", "foo.example.com"), false);
        expect_eq(verifyDnsHostname("*o.example.com", "foo.example.com"), false);
        expect_eq(verifyDnsHostname("f*o.example.com", "foo.example.com"), false);
        // Reject multi-wildcard patterns
        expect_eq(verifyDnsHostname("*.*.example.com", "a.b.example.com"), false);
        expect_eq(verifyDnsHostname("foo.*.example.com", "foo.bar.example.com"), false);
        // Reject empty labels & invalid hyphen placement
        expect_eq(verifyDnsHostname("*.example.com", ".example.com"), false);
        expect_eq(verifyDnsHostname("*.example.com", "foo..example.com"), false);
        expect_eq(verifyDnsHostname("example.com.", "example.com."), false);
        expect_eq(verifyDnsHostname("", "example.com"), false);
        expect_eq(verifyDnsHostname("example.com", ""), false);
        expect_eq(verifyDnsHostname("*.example.com", "-bad.example.com"), false);
    });

    test("3. @cap/rustls-webpki — RFC 5280 UTCTime & GeneralizedTime exact Unix timestamp conversion", () => {
        // 1970-01-01T00:00:00Z (UTCTime 70..99 -> 1970..1999)
        const epoch = parseX509TimeUtc("700101000000Z");
        expect_eq(epoch.ok, true);
        expect_eq(epoch.year, 1970 as u32);
        expect_eq(epoch.unix_secs, 0 as u64);

        // 1999-12-31T23:59:59Z (UTCTime 99 -> 1999)
        const y99 = parseX509TimeUtc("991231235959Z");
        expect_eq(y99.ok, true);
        expect_eq(y99.year, 1999 as u32);
        expect_eq(y99.unix_secs, 946684799 as u64);

        // 2000-01-01T00:00:00Z (UTCTime 00 -> 2000)
        const y2k = parseX509TimeUtc("000101000000Z");
        expect_eq(y2k.ok, true);
        expect_eq(y2k.year, 2000 as u32);
        expect_eq(y2k.unix_secs, 946684800 as u64);

        // 2024-02-29T12:30:45Z (leap day in UTCTime)
        const leap = parseX509TimeUtc("240229123045Z");
        expect_eq(leap.ok, true);
        expect_eq(leap.year, 2024 as u32);
        expect_eq(leap.month, 2 as u8);
        expect_eq(leap.day, 29 as u8);
        expect_eq(leap.unix_secs, 1709209845 as u64);

        // 2050-01-01T00:00:00Z (GeneralizedTime)
        const y2050 = parseX509TimeUtc("20500101000000Z");
        expect_eq(y2050.ok, true);
        expect_eq(y2050.year, 2050 as u32);
        expect_eq(y2050.unix_secs, 2524608000 as u64);

        // Reject invalid leap day (2025 is not a leap year), bad month, missing 'Z'
        expect_eq(parseX509TimeUtc("250229000000Z").ok, false);
        expect_eq(parseX509TimeUtc("261301000000Z").ok, false);
        expect_eq(parseX509TimeUtc("260101000000X").ok, false);
        expect_eq(parseX509TimeUtc("260101").ok, false);
    });

    test("4. @cap/rustls-webpki — verifyValidityAt not-yet-valid / valid / expired / invalid-time states", () => {
        const dn = X509Name.init("time-test.cap.dev", "Cap PKI", "US");
        // Valid: [2026-01-01T00:00:00Z (1767225600), 2027-01-01T00:00:00Z (1798761600)]
        const val = X509Validity.fromStrs("260101000000Z", "270101000000Z");
        const cert = makeTestCert(dn, dn, val);

        // Before notBefore
        expect_eq(verifyValidityAt(cert, 1767225599 as u64), WPKI_ERR_NOT_YET_VALID);
        // Exact boundary notBefore (inclusive)
        expect_eq(verifyValidityAt(cert, 1767225600 as u64), WPKI_OK);
        // Mid-window (2026-06-01)
        expect_eq(verifyValidityAt(cert, 1780272000 as u64), WPKI_OK);
        // Exact boundary notAfter (inclusive)
        expect_eq(verifyValidityAt(cert, 1798761600 as u64), WPKI_OK);
        // After notAfter
        expect_eq(verifyValidityAt(cert, 1798761601 as u64), WPKI_ERR_EXPIRED);

        // Inverted validity window (notAfter < notBefore) -> WPKI_ERR_INVALID_TIME
        const bad_val = X509Validity.fromStrs("270101000000Z", "260101000000Z");
        const bad_cert = makeTestCert(dn, dn, bad_val);
        expect_eq(verifyValidityAt(bad_cert, 1780272000 as u64), WPKI_ERR_INVALID_TIME);
    });

    test("5. @cap/rustls-webpki — Cross-package @cap/x509-cert DER encode + verifyCertForDnsName", () => {
        const ca_dn = X509Name.init("Cap Root CA", "Cap Trust", "US");
        const leaf_dn = X509Name.init("*.cloud.cap.dev", "Cap Cloud", "US");
        const val = X509Validity.fromStrs("260101000000Z", "20500101000000Z");
        const leaf = makeTestCert(ca_dn, leaf_dn, val);

        const now_2026: u64 = 1780272000 as u64;
        expect_eq(verifyCertForDnsName(leaf, "api.cloud.cap.dev", now_2026), WPKI_OK);
        expect_eq(verifyCertForDnsName(leaf, "EDGE.Cloud.Cap.Dev", now_2026), WPKI_OK);
        expect_eq(verifyCertForDnsName(leaf, "a.b.cloud.cap.dev", now_2026), WPKI_ERR_HOSTNAME_MISMATCH);
        expect_eq(verifyCertForDnsName(leaf, "cloud.cap.dev", now_2026), WPKI_ERR_HOSTNAME_MISMATCH);
        expect_eq(verifyCertForDnsName(leaf, "api.cloud.cap.dev", 1700000000 as u64), WPKI_ERR_NOT_YET_VALID);

        // Round-trip through DER bytes via @cap/x509-cert encodeX509Der + verifyDerCertForDnsName
        let der: [1024]u8 = undefined;
        const dlen = encodeX509Der(leaf, der[0..1024]);
        expect_eq(dlen > (150 as i64), true);
        expect_eq(verifyDerCertForDnsName(der[0..(dlen as usize)], "gateway.cloud.cap.dev", now_2026), WPKI_OK);
        expect_eq(verifyDerCertForDnsName(der[0..(dlen as usize)], "wrong.example.org", now_2026), WPKI_ERR_HOSTNAME_MISMATCH);
    });

    test("6. @cap/rustls-webpki — verifyChainLink, verifyTrustAnchor & verifyEndEntityChain", () => {
        const root_dn = X509Name.init("Cap Root CA G1", "Cap Trust Foundation", "US");
        const other_ca_dn = X509Name.init("Rogue CA", "Untrusted Corp", "XX");
        const leaf_dn = X509Name.init("service.cap.dev", "Cap Services", "US");

        const ca_val = X509Validity.fromStrs("250101000000Z", "350101000000Z");
        const leaf_val = X509Validity.fromStrs("260101000000Z", "280101000000Z");

        const root_ca = makeTestCert(root_dn, root_dn, ca_val);
        const rogue_ca = makeTestCert(other_ca_dn, other_ca_dn, ca_val);
        const leaf = makeTestCert(root_dn, leaf_dn, leaf_val);

        const now_2026: u64 = 1780272000 as u64;

        // Valid leaf -> root_ca link
        expect_eq(verifyChainLink(leaf, root_ca), true);
        // Broken link: leaf was NOT issued by rogue_ca
        expect_eq(verifyChainLink(leaf, rogue_ca), false);
        // Broken link: empty issuer cert rejected
        expect_eq(verifyChainLink(X509Certificate.empty(), X509Certificate.empty()), false);

        // Trust anchor verification (self-signed + valid)
        expect_eq(verifyTrustAnchor(root_ca, now_2026), WPKI_OK);
        expect_eq(verifyTrustAnchor(leaf, now_2026), WPKI_ERR_NOT_SELF_SIGNED);

        // Full end-entity chain verification
        expect_eq(verifyEndEntityChain(leaf, root_ca, "service.cap.dev", now_2026), WPKI_OK);
        expect_eq(verifyEndEntityChain(leaf, rogue_ca, "service.cap.dev", now_2026), WPKI_ERR_CHAIN_BROKEN);
        expect_eq(verifyEndEntityChain(leaf, root_ca, "other.cap.dev", now_2026), WPKI_ERR_HOSTNAME_MISMATCH);
    });
}
