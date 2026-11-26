// cap/test/testCapWebpkiRoots.cp
// Comprehensive unit tests for @cap/webpki-roots

import { test, expect_eq } from "cap:test";
import { OID_ED25519, ObjectIdentifier } from "../../packages/@cap/const-oid/src/index.cp";
import { AlgorithmIdentifier, SubjectPublicKeyInfo, fingerprintSpki } from "../../packages/@cap/spki/src/index.cp";
import { X509Certificate, X509Name, X509Validity } from "../../packages/@cap/x509-cert/src/index.cp";
import {
    KEY_ALG_RSA,
    KEY_ALG_P256,
    KEY_ALG_P384,
    KEY_ALG_ED25519,
    MOZILLA_ROOT_COUNT,
    TRUST_STORE_CAPACITY,
    TrustAnchorInfo,
    TrustAnchorStore,
    mozillaRootAt,
    lookupMozillaRootByCn,
    certSpkiFingerprint,
} from "../../packages/@cap/webpki-roots/src/index.cp";

fn wrTestStrEq(a: string, b: string): bool {
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

fn wrMakeTestCert(issuer: X509Name, subject: X509Name, validity: X509Validity): X509Certificate {
    let cert = X509Certificate.empty();
    cert.version = 3 as u8;
    cert.serial[0] = 0x42;
    cert.serial_len = 1;
    cert.signature_alg = AlgorithmIdentifier.fromOid(ObjectIdentifier.fromStr(OID_ED25519).oid);
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
    return cert;
}

fn main() {
    test("1. @cap/webpki-roots — withMozillaDefaults() loads all 20 authoritative root CAs", () => {
        const store = TrustAnchorStore.withMozillaDefaults();
        expect_eq(store.count(), MOZILLA_ROOT_COUNT);
        expect_eq(MOZILLA_ROOT_COUNT, 20 as usize);

        let i: usize = 0;
        while (i < MOZILLA_ROOT_COUNT) {
            const a = store.at(i);
            const direct = mozillaRootAt(i);
            expect_eq(a.isEmpty(), false);
            expect_eq(wrTestStrEq(a.name, direct.name), true);
            expect_eq(a.spki_fingerprint != (0 as u64), true);
            expect_eq(a.not_after_year >= (2025 as u16), true);
            i = i + 1;
        }
        // Out of range index returns empty anchor
        expect_eq(mozillaRootAt(99).isEmpty(), true);
        expect_eq(store.at(99).isEmpty(), true);
    });

    test("2. @cap/webpki-roots — Exact CN lookup for ISRG Root X1, DigiCert & GTS Root R1", () => {
        const store = TrustAnchorStore.withMozillaDefaults();

        const isrg = lookupMozillaRootByCn("ISRG Root X1");
        expect_eq(isrg.found, true);
        expect_eq(wrTestStrEq(isrg.anchor.org, "Internet Security Research Group"), true);
        expect_eq(wrTestStrEq(isrg.anchor.country, "US"), true);
        expect_eq(isrg.anchor.key_alg, KEY_ALG_RSA);
        expect_eq(isrg.anchor.not_after_year, 2035 as u16);

        const digicert = store.findByCn("DigiCert Global Root G3");
        expect_eq(digicert.found, true);
        expect_eq(wrTestStrEq(digicert.anchor.org, "DigiCert Inc"), true);
        expect_eq(digicert.anchor.key_alg, KEY_ALG_P384);

        const gts = store.findByCn("GTS Root R1");
        expect_eq(gts.found, true);
        expect_eq(wrTestStrEq(gts.anchor.org, "Google Trust Services LLC"), true);
        expect_eq(gts.anchor.not_after_year, 2036 as u16);

        const missing = store.findByCn("Unknown Rogue Root CA");
        expect_eq(missing.found, false);
    });

    test("3. @cap/webpki-roots — Organization filtering (findByOrg & countByOrg)", () => {
        const store = TrustAnchorStore.withMozillaDefaults();

        const isrg_org = store.findByOrg("Internet Security Research Group");
        expect_eq(isrg_org.found, true);
        expect_eq(wrTestStrEq(isrg_org.anchor.name, "ISRG Root X1"), true);
        expect_eq(store.countByOrg("Internet Security Research Group"), 2 as usize);

        const google_org = store.findByOrg("Google Trust Services LLC");
        expect_eq(google_org.found, true);
        expect_eq(wrTestStrEq(google_org.anchor.name, "GTS Root R1"), true);
        expect_eq(store.countByOrg("Google Trust Services LLC"), 2 as usize);

        const amazon_org = store.findByOrg("Amazon");
        expect_eq(amazon_org.found, true);
        expect_eq(store.countByOrg("Amazon"), 2 as usize);

        const cf_org = store.findByOrg("Cloudflare, Inc.");
        expect_eq(cf_org.found, true);
        expect_eq(cf_org.anchor.key_alg, KEY_ALG_P256);

        expect_eq(store.findByOrg("Nonexistent CA Corp").found, false);
        expect_eq(store.countByOrg("Nonexistent CA Corp"), 0 as usize);
    });

    test("4. @cap/webpki-roots — addAnchor custom private root CA & capacity boundary", () => {
        let store = TrustAnchorStore.withMozillaDefaults();
        const custom = TrustAnchorInfo.init(
            "Cap Internal Root CA",
            "Cap Systems",
            "CN",
            KEY_ALG_ED25519,
            fingerprintSpki("Cap Internal Root CA"),
            2045 as u16,
        );
        expect_eq(store.addAnchor(custom), true);
        expect_eq(store.count(), (21 as usize));
        const found = store.findByCn("Cap Internal Root CA");
        expect_eq(found.found, true);
        expect_eq(found.anchor.key_alg, KEY_ALG_ED25519);

        // Reject empty anchor name
        expect_eq(store.addAnchor(TrustAnchorInfo.empty()), false);

        // Test capacity boundary on fresh store (fill all 32 slots, 33rd rejected)
        let bounded = TrustAnchorStore.empty();
        expect_eq(bounded.count(), 0 as usize);
        let i: usize = 0;
        while (i < TRUST_STORE_CAPACITY) {
            expect_eq(bounded.addAnchor(custom), true);
            i = i + 1;
        }
        expect_eq(bounded.count(), TRUST_STORE_CAPACITY);
        expect_eq(bounded.addAnchor(custom), false);
    });

    test("5. @cap/webpki-roots — Cross-package @cap/x509-cert isTrustedIssuer accept & reject", () => {
        const store = TrustAnchorStore.withMozillaDefaults();
        const subject = X509Name.init("api.cap.dev", "Cap Cloud", "US");
        const val = X509Validity.fromStrs("260101000000Z", "300101000000Z");

        // Certificate issued by trusted ISRG Root X1
        const trusted_issuer = X509Name.init("ISRG Root X1", "Internet Security Research Group", "US");
        const trusted_cert = wrMakeTestCert(trusted_issuer, subject, val);
        expect_eq(store.isTrustedIssuer(trusted_cert), true);

        // Verify cross-package SPKI fingerprint computation
        const fp = certSpkiFingerprint(trusted_cert);
        expect_eq(fp != (0 as u64), true);

        // Certificate issued by untrusted self-signed / unknown CA is rejected
        const untrusted_issuer = X509Name.init("Untrusted Local CA", "Unknown Org", "US");
        const untrusted_cert = wrMakeTestCert(untrusted_issuer, subject, val);
        expect_eq(store.isTrustedIssuer(untrusted_cert), false);

        // Matching CN but mismatched Organization is rejected
        const spoofed_issuer = X509Name.init("ISRG Root X1", "Fake Org", "US");
        const spoofed_cert = wrMakeTestCert(spoofed_issuer, subject, val);
        expect_eq(store.isTrustedIssuer(spoofed_cert), false);
    });

    test("6. @cap/webpki-roots — verifyIssuerTrusted rejects expired root CA or expired cert year", () => {
        const store = TrustAnchorStore.withMozillaDefaults();
        const subject = X509Name.init("tls.cap.dev", "Cap Cloud", "US");

        // ISRG Root X1 expires in 2035; cert expires in 2034
        const isrg_issuer = X509Name.init("ISRG Root X1", "Internet Security Research Group", "US");
        const valid_cert = wrMakeTestCert(isrg_issuer, subject, X509Validity.fromStrs("260101000000Z", "340101000000Z"));
        expect_eq(store.verifyIssuerTrusted(valid_cert, 2026 as u16), true);
        expect_eq(store.verifyIssuerTrusted(valid_cert, 2034 as u16), true);
        // Cert expired in 2034 -> rejected in 2035
        expect_eq(store.verifyIssuerTrusted(valid_cert, 2035 as u16), false);

        // Long-lived cert (GeneralizedTime 2040), but ISRG Root X1 expires in 2035 -> rejected in 2036
        const long_cert = wrMakeTestCert(isrg_issuer, subject, X509Validity.fromStrs("260101000000Z", "20400101000000Z"));
        expect_eq(store.verifyIssuerTrusted(long_cert, 2035 as u16), true);
        expect_eq(store.verifyIssuerTrusted(long_cert, 2036 as u16), false);

        // Baltimore CyberTrust Root expired in 2025 -> rejected in 2026
        const baltimore = X509Name.init("Baltimore CyberTrust Root", "Baltimore", "IE");
        const old_root_cert = wrMakeTestCert(baltimore, subject, X509Validity.fromStrs("240101000000Z", "280101000000Z"));
        expect_eq(store.isTrustedIssuer(old_root_cert), true);
        expect_eq(store.verifyIssuerTrusted(old_root_cert, 2025 as u16), true);
        expect_eq(store.verifyIssuerTrusted(old_root_cert, 2026 as u16), false);
    });
}
