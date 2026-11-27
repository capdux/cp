// cap/test/testCapRustlsPemfile.cp
// Comprehensive unit tests for @cap/rustls-pemfile

import { test, expect_eq } from "cap:test";
import {
    PEM_ITEM_NONE,
    PEM_ITEM_X509_CERT,
    PEM_ITEM_PKCS8_KEY,
    PEM_ITEM_PKCS1_KEY,
    PEM_ITEM_SEC1_KEY,
    PEM_ITEM_SPKI_PUBKEY,
    PEM_ITEM_CRL,
    PEM_ITEM_CSR,
    PEM_ITEM_UNKNOWN,
    PemBundleReader,
    classifyPemLabel,
    isPrivateKeyKind,
    readFirstPrivateKey,
} from "../../packages/rustls-pemfile/src/index.cp";

fn rpfTestEqBytes(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/rustls-pemfile — Multi-cert CA bundle iteration & countCerts", () => {
        // 3 certificates: "CERT-ONE" (Q0VSVC1PTkU=), "CERT-TWO" (Q0VSVC1UV08=), "CERT-THREE" (Q0VSVC1USFJF) -> let's use exact Base64:
        // "CERT-1" -> "Q0VSVC0x"
        // "CERT-22" -> "Q0VSVC0yMg=="
        // "CERT-333" -> "Q0VSVC0zMzM="
        const bundle =
            "-----BEGIN CERTIFICATE-----\n" +
            "Q0VSVC0x\n" +
            "-----END CERTIFICATE-----\n" +
            "-----BEGIN CERTIFICATE-----\n" +
            "Q0VSVC0yMg==\n" +
            "-----END CERTIFICATE-----\n" +
            "-----BEGIN CERTIFICATE-----\n" +
            "Q0VSVC0zMzM=\n" +
            "-----END CERTIFICATE-----\n";

        let reader = PemBundleReader.new(bundle);
        expect_eq(reader.countItems(), 3 as usize);
        expect_eq(reader.countCerts(), 3 as usize);
        expect_eq(reader.peekNextKind(), PEM_ITEM_X509_CERT);

        let der: [64]u8 = [0; 64];

        const item1 = reader.nextItem(der[0..64]);
        expect_eq(item1.ok, true);
        expect_eq(item1.eof, false);
        expect_eq(item1.kind, PEM_ITEM_X509_CERT);
        expect_eq(item1.isCert(), true);
        expect_eq(item1.der_len, 6 as usize);
        expect_eq(rpfTestEqBytes(der[0..item1.der_len], "CERT-1"), true);

        const item2 = reader.nextItem(der[0..64]);
        expect_eq(item2.ok, true);
        expect_eq(item2.eof, false);
        expect_eq(item2.kind, PEM_ITEM_X509_CERT);
        expect_eq(item2.der_len, 7 as usize);
        expect_eq(rpfTestEqBytes(der[0..item2.der_len], "CERT-22"), true);

        const item3 = reader.nextItem(der[0..64]);
        expect_eq(item3.ok, true);
        expect_eq(item3.eof, false);
        expect_eq(item3.kind, PEM_ITEM_X509_CERT);
        expect_eq(item3.der_len, 8 as usize);
        expect_eq(rpfTestEqBytes(der[0..item3.der_len], "CERT-333"), true);

        const item4 = reader.nextItem(der[0..64]);
        expect_eq(item4.ok, true);
        expect_eq(item4.eof, true);
        expect_eq(item4.kind, PEM_ITEM_NONE);
        expect_eq(reader.isEof(), true);
    });

    test("2. @cap/rustls-pemfile — Skip inter-block comments & CRLF line endings", () => {
        // "Hello!" -> "SGVsbG8h", "World!" -> "V29ybGQh"
        const bundle =
            "# Root CA Bundle\r\n" +
            "Issuer: CN=Example Root CA\r\n" +
            "\r\n" +
            "-----BEGIN CERTIFICATE-----\r\n" +
            "SGVsbG8h\r\n" +
            "-----END CERTIFICATE-----\r\n" +
            "\r\n" +
            "Bag Attributes\r\n" +
            "    friendlyName: Intermediate CA\r\n" +
            "-----BEGIN CERTIFICATE-----\r\n" +
            "V29ybGQh\r\n" +
            "-----END CERTIFICATE-----\r\n" +
            "# Trailing note with no PEM block\r\n";

        let reader = PemBundleReader.new(bundle);
        expect_eq(reader.countItems(), 2 as usize);
        expect_eq(reader.countCerts(), 2 as usize);

        let der: [64]u8 = [0; 64];
        const first = reader.nextItem(der[0..64]);
        expect_eq(first.ok, true);
        expect_eq(first.eof, false);
        expect_eq(first.kind, PEM_ITEM_X509_CERT);
        expect_eq(rpfTestEqBytes(der[0..first.der_len], "Hello!"), true);

        const second = reader.nextItem(der[0..64]);
        expect_eq(second.ok, true);
        expect_eq(second.eof, false);
        expect_eq(second.kind, PEM_ITEM_X509_CERT);
        expect_eq(rpfTestEqBytes(der[0..second.der_len], "World!"), true);

        const done = reader.nextItem(der[0..64]);
        expect_eq(done.ok, true);
        expect_eq(done.eof, true);
        expect_eq(done.kind, PEM_ITEM_NONE);
    });

    test("3. @cap/rustls-pemfile — Mixed CERTIFICATE + PRIVATE KEY + RSA/EC KEY classification", () => {
        // Verify classifyPemLabel for all standard + unknown + invalid labels
        expect_eq(classifyPemLabel("CERTIFICATE"), PEM_ITEM_X509_CERT);
        expect_eq(classifyPemLabel("PRIVATE KEY"), PEM_ITEM_PKCS8_KEY);
        expect_eq(classifyPemLabel("RSA PRIVATE KEY"), PEM_ITEM_PKCS1_KEY);
        expect_eq(classifyPemLabel("EC PRIVATE KEY"), PEM_ITEM_SEC1_KEY);
        expect_eq(classifyPemLabel("PUBLIC KEY"), PEM_ITEM_SPKI_PUBKEY);
        expect_eq(classifyPemLabel("X509 CRL"), PEM_ITEM_CRL);
        expect_eq(classifyPemLabel("CERTIFICATE REQUEST"), PEM_ITEM_CSR);
        expect_eq(classifyPemLabel("DH PARAMETERS"), PEM_ITEM_UNKNOWN);
        expect_eq(classifyPemLabel(""), PEM_ITEM_NONE);
        expect_eq(classifyPemLabel("BAD  LABEL"), PEM_ITEM_NONE);

        expect_eq(isPrivateKeyKind(PEM_ITEM_PKCS8_KEY), true);
        expect_eq(isPrivateKeyKind(PEM_ITEM_PKCS1_KEY), true);
        expect_eq(isPrivateKeyKind(PEM_ITEM_SEC1_KEY), true);
        expect_eq(isPrivateKeyKind(PEM_ITEM_X509_CERT), false);
        expect_eq(isPrivateKeyKind(PEM_ITEM_SPKI_PUBKEY), false);

        // "abc" -> "YWJj"
        const mixed =
            "-----BEGIN CERTIFICATE-----\nYWJj\n-----END CERTIFICATE-----\n" +
            "-----BEGIN PRIVATE KEY-----\nYWJj\n-----END PRIVATE KEY-----\n" +
            "-----BEGIN RSA PRIVATE KEY-----\nYWJj\n-----END RSA PRIVATE KEY-----\n" +
            "-----BEGIN EC PRIVATE KEY-----\nYWJj\n-----END EC PRIVATE KEY-----\n" +
            "-----BEGIN PUBLIC KEY-----\nYWJj\n-----END PUBLIC KEY-----\n" +
            "-----BEGIN X509 CRL-----\nYWJj\n-----END X509 CRL-----\n" +
            "-----BEGIN CERTIFICATE REQUEST-----\nYWJj\n-----END CERTIFICATE REQUEST-----\n" +
            "-----BEGIN TRUSTED CERTIFICATE-----\nYWJj\n-----END TRUSTED CERTIFICATE-----\n";

        let reader = PemBundleReader.new(mixed);
        expect_eq(reader.countItems(), 8 as usize);
        expect_eq(reader.countCerts(), 1 as usize);

        let der: [32]u8 = [0; 32];
        const i1 = reader.nextItem(der[0..32]);
        expect_eq(i1.kind, PEM_ITEM_X509_CERT);
        const i2 = reader.nextItem(der[0..32]);
        expect_eq(i2.kind, PEM_ITEM_PKCS8_KEY);
        expect_eq(i2.isPrivateKey(), true);
        const i3 = reader.nextItem(der[0..32]);
        expect_eq(i3.kind, PEM_ITEM_PKCS1_KEY);
        expect_eq(i3.isPrivateKey(), true);
        const i4 = reader.nextItem(der[0..32]);
        expect_eq(i4.kind, PEM_ITEM_SEC1_KEY);
        expect_eq(i4.isPrivateKey(), true);
        const i5 = reader.nextItem(der[0..32]);
        expect_eq(i5.kind, PEM_ITEM_SPKI_PUBKEY);
        const i6 = reader.nextItem(der[0..32]);
        expect_eq(i6.kind, PEM_ITEM_CRL);
        const i7 = reader.nextItem(der[0..32]);
        expect_eq(i7.kind, PEM_ITEM_CSR);
        const i8 = reader.nextItem(der[0..32]);
        expect_eq(i8.kind, PEM_ITEM_UNKNOWN);
        const end = reader.nextItem(der[0..32]);
        expect_eq(end.eof, true);
    });

    test("4. @cap/rustls-pemfile — readFirstPrivateKey skips certs & extracts PKCS8/PKCS1/SEC1", () => {
        // "PKCS8!" -> "UEtDUzgh", "RSA-KEY" -> "UlNBLUtFWQ==", "EC-KEY!" -> "RUMtS0VZIQ=="
        const bundle_pkcs8 =
            "-----BEGIN CERTIFICATE-----\nQ0VSVC0x\n-----END CERTIFICATE-----\n" +
            "-----BEGIN PUBLIC KEY-----\nQ0VSVC0x\n-----END PUBLIC KEY-----\n" +
            "-----BEGIN PRIVATE KEY-----\nUEtDUzgh\n-----END PRIVATE KEY-----\n" +
            "-----BEGIN RSA PRIVATE KEY-----\nUlNBLUtFWQ==\n-----END RSA PRIVATE KEY-----\n";

        let der: [64]u8 = [0; 64];
        const k1 = readFirstPrivateKey(bundle_pkcs8, der[0..64]);
        expect_eq(k1.ok, true);
        expect_eq(k1.eof, false);
        expect_eq(k1.kind, PEM_ITEM_PKCS8_KEY);
        expect_eq(k1.isPrivateKey(), true);
        expect_eq(rpfTestEqBytes(der[0..k1.der_len], "PKCS8!"), true);

        const bundle_pkcs1 =
            "-----BEGIN CERTIFICATE-----\nQ0VSVC0x\n-----END CERTIFICATE-----\n" +
            "-----BEGIN RSA PRIVATE KEY-----\nUlNBLUtFWQ==\n-----END RSA PRIVATE KEY-----\n";
        const k2 = readFirstPrivateKey(bundle_pkcs1, der[0..64]);
        expect_eq(k2.ok, true);
        expect_eq(k2.kind, PEM_ITEM_PKCS1_KEY);
        expect_eq(rpfTestEqBytes(der[0..k2.der_len], "RSA-KEY"), true);

        const bundle_sec1 =
            "-----BEGIN EC PRIVATE KEY-----\nRUMtS0VZIQ==\n-----END EC PRIVATE KEY-----\n";
        const k3 = readFirstPrivateKey(bundle_sec1, der[0..64]);
        expect_eq(k3.ok, true);
        expect_eq(k3.kind, PEM_ITEM_SEC1_KEY);
        expect_eq(rpfTestEqBytes(der[0..k3.der_len], "EC-KEY!"), true);

        // Bundle with only certificates returns EOF
        const certs_only = "-----BEGIN CERTIFICATE-----\nQ0VSVC0x\n-----END CERTIFICATE-----\n";
        const k4 = readFirstPrivateKey(certs_only, der[0..64]);
        expect_eq(k4.ok, true);
        expect_eq(k4.eof, true);
        expect_eq(k4.kind, PEM_ITEM_NONE);
    });

    test("5. @cap/rustls-pemfile — Empty input & comment-only input safely return EOF", () => {
        let der: [32]u8 = [0; 32];

        let r_empty = PemBundleReader.new("");
        expect_eq(r_empty.countItems(), 0 as usize);
        expect_eq(r_empty.countCerts(), 0 as usize);
        expect_eq(r_empty.peekNextKind(), PEM_ITEM_NONE);
        const e1 = r_empty.nextItem(der[0..32]);
        expect_eq(e1.ok, true);
        expect_eq(e1.eof, true);
        expect_eq(e1.kind, PEM_ITEM_NONE);
        expect_eq(e1.der_len, 0 as usize);

        const comments_only = "# No certificates here\nJust explanatory text\r\n   \t  \n";
        let r_comments = PemBundleReader.new(comments_only);
        expect_eq(r_comments.countItems(), 0 as usize);
        expect_eq(r_comments.countCerts(), 0 as usize);
        const e2 = r_comments.nextItem(der[0..32]);
        expect_eq(e2.ok, true);
        expect_eq(e2.eof, true);
        expect_eq(e2.kind, PEM_ITEM_NONE);

        const e3 = readFirstPrivateKey(comments_only, der[0..32]);
        expect_eq(e3.ok, true);
        expect_eq(e3.eof, true);
    });

    test("6. @cap/rustls-pemfile — Malformed & truncated PEM blocks are rejected", () => {
        let der: [32]u8 = [0; 32];

        // Truncated PEM block (missing END footer)
        const truncated = "-----BEGIN CERTIFICATE-----\nQ0VSVC0x\n";
        let r1 = PemBundleReader.new(truncated);
        const res1 = r1.nextItem(der[0..32]);
        expect_eq(res1.ok, false);
        expect_eq(res1.eof, false);

        // Mismatched BEGIN/END labels
        const mismatched = "-----BEGIN CERTIFICATE-----\nQ0VSVC0x\n-----END PRIVATE KEY-----\n";
        let r2 = PemBundleReader.new(mismatched);
        const res2 = r2.nextItem(der[0..32]);
        expect_eq(res2.ok, false);
        expect_eq(res2.eof, false);

        // Invalid Base64 payload
        const bad_b64 = "-----BEGIN PRIVATE KEY-----\n!!!INVALID!!!\n-----END PRIVATE KEY-----\n";
        let r3 = PemBundleReader.new(bad_b64);
        const res3 = r3.nextItem(der[0..32]);
        expect_eq(res3.ok, false);
        expect_eq(res3.eof, false);

        // Malformed block inside readFirstPrivateKey
        const res4 = readFirstPrivateKey(bad_b64, der[0..32]);
        expect_eq(res4.ok, false);
        expect_eq(res4.eof, false);
    });
}
