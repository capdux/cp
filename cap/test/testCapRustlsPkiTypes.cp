// cap/test/testCapRustlsPkiTypes.cp
// Comprehensive unit tests for @cap/rustls-pki-types

import { test, expect_eq } from "cap:test";
import {
    PKI_OK,
    PKI_ERR_UNKNOWN_PEM_KIND,
    SERVER_NAME_INVALID,
    SERVER_NAME_DNS,
    SERVER_NAME_IPV4,
    SERVER_NAME_IPV6,
    PEM_UNKNOWN,
    PEM_CERTIFICATE,
    PEM_PKCS8_KEY,
    PEM_PKCS1_KEY,
    PEM_SEC1_KEY,
    PEM_PUBLIC_KEY,
    PEM_CRL,
    PEM_CSR,
    DnsName,
    ServerName,
    UnixTime,
    isValidDnsName,
    parseDnsName,
    parseServerName,
    detectPemKind,
    decodePemDer,
    pemKindLabel,
    isPrivateKeyKind,
} from "../../packages/@cap/rustls-pki-types/src/index.cp";

fn pkiTestEqBytes(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/rustls-pki-types — DnsName valid hostnames, trailing dot, subdomain & case-insensitive eq", () => {
        const d1 = DnsName.parse("Example.COM");
        const d2 = parseDnsName("example.com.");
        expect_eq(d1.isValid(), true);
        expect_eq(d2.isValid(), true);
        // Trailing dot normalized away
        expect_eq(d1.len, 11 as usize);
        expect_eq(d2.len, 11 as usize);
        expect_eq(d1.eqIgnoreCase(d2), true);
        expect_eq(d1.eqIgnoreCaseStr("EXAMPLE.com."), true);
        expect_eq(d1.labelCount(), 2 as usize);

        const sub = DnsName.parse("api-v2.sub.Example.Com");
        expect_eq(sub.isValid(), true);
        expect_eq(sub.labelCount(), 4 as usize);
        expect_eq(sub.isSubdomainOf(d1), true);
        expect_eq(d1.isSubdomainOf(sub), false);
        expect_eq(d1.isSubdomainOf(d2), false);

        // Lowercase normalization
        let low: [32]u8 = [0; 32];
        const low_len = d1.toLowercase(low[0..32]);
        expect_eq(low_len, 11 as usize);
        expect_eq(pkiTestEqBytes(low[0..low_len], "example.com"), true);

        // Single-label valid hostname (e.g. localhost) and 63-char label boundary
        expect_eq(isValidDnsName("localhost"), true);
        expect_eq(isValidDnsName("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com"), true);
    });

    test("2. @cap/rustls-pki-types — DnsName rejects invalid labels, hyphens, empty, numeric TLD & overlong", () => {
        // Empty and lone dot
        expect_eq(isValidDnsName(""), false);
        expect_eq(isValidDnsName("."), false);
        // Leading / trailing hyphen in label
        expect_eq(isValidDnsName("-bad.com"), false);
        expect_eq(isValidDnsName("bad-.com"), false);
        expect_eq(isValidDnsName("ok.-bad.com"), false);
        // Empty labels / double dots
        expect_eq(isValidDnsName("a..b"), false);
        expect_eq(isValidDnsName(".example.com"), false);
        expect_eq(isValidDnsName("example.com.."), false);
        // Illegal characters (underscore, space, slash)
        expect_eq(isValidDnsName("bad_host.com"), false);
        expect_eq(isValidDnsName("bad host.com"), false);
        expect_eq(isValidDnsName("example.com/path"), false);
        // All-numeric TLD rejected (RFC 1123 / rustls-pki-types)
        expect_eq(isValidDnsName("127.0.0.1"), false);
        expect_eq(isValidDnsName("example.123"), false);
        // 64-char label exceeds RFC 1035 max of 63
        expect_eq(isValidDnsName("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com"), false);
    });

    test("3. @cap/rustls-pki-types — ServerName auto-detects DNS, IPv4, IPv6 & bracketed IPv6", () => {
        // DNS
        const s_dns = ServerName.parse("tls.example.org");
        expect_eq(s_dns.isValid(), true);
        expect_eq(s_dns.kind, SERVER_NAME_DNS);
        expect_eq(s_dns.isDns(), true);
        expect_eq(s_dns.isIp(), false);
        expect_eq(s_dns.dns.eqIgnoreCaseStr("TLS.EXAMPLE.ORG"), true);

        // IPv4
        const s_v4 = parseServerName("127.0.0.1");
        expect_eq(s_v4.isValid(), true);
        expect_eq(s_v4.kind, SERVER_NAME_IPV4);
        expect_eq(s_v4.isIpv4(), true);
        expect_eq(s_v4.isIp(), true);
        expect_eq(s_v4.ipv4[0], 127 as u8);
        expect_eq(s_v4.ipv4[1], 0 as u8);
        expect_eq(s_v4.ipv4[2], 0 as u8);
        expect_eq(s_v4.ipv4[3], 1 as u8);

        // IPv6 loopback "::1" and bracketed "[::1]"
        const s_v6 = ServerName.parse("::1");
        expect_eq(s_v6.isValid(), true);
        expect_eq(s_v6.kind, SERVER_NAME_IPV6);
        expect_eq(s_v6.isIpv6(), true);
        expect_eq(s_v6.ipv6[0], 0 as u8);
        expect_eq(s_v6.ipv6[15], 1 as u8);

        const s_v6b = ServerName.parse("[::1]");
        expect_eq(s_v6b.kind, SERVER_NAME_IPV6);
        expect_eq(s_v6b.ipv6[15], 1 as u8);

        // IPv6 with IPv4-mapped tail "::ffff:192.0.2.128"
        const s_v6m = ServerName.parse("::ffff:192.0.2.128");
        expect_eq(s_v6m.kind, SERVER_NAME_IPV6);
        expect_eq(s_v6m.ipv6[10], 0xFF as u8);
        expect_eq(s_v6m.ipv6[11], 0xFF as u8);
        expect_eq(s_v6m.ipv6[12], 192 as u8);
        expect_eq(s_v6m.ipv6[15], 128 as u8);

        // Invalid inputs
        expect_eq(ServerName.parse("").kind, SERVER_NAME_INVALID);
        expect_eq(ServerName.parse("999.0.0.1").kind, SERVER_NAME_INVALID);
        expect_eq(ServerName.parse("[::1").kind, SERVER_NAME_INVALID);
        expect_eq(ServerName.parse("1::2::3").kind, SERVER_NAME_INVALID);
    });

    test("4. @cap/rustls-pki-types — UnixTime comparisons, saturating arithmetic & validity window", () => {
        const t0 = UnixTime.epoch();
        const t1 = UnixTime.fromSeconds(1700000000);
        const t2 = t1.addSeconds(3600);
        const t3 = t2.subSeconds(3600);

        expect_eq(t0.asSeconds(), 0 as u64);
        expect_eq(t2.asSeconds(), 1700003600 as u64);
        expect_eq(t1.isBefore(t2), true);
        expect_eq(t2.isAfter(t1), true);
        expect_eq(t1.isAfter(t2), false);
        expect_eq(t1.equals(t3), true);

        // Validity window check
        const not_before = UnixTime.fromSeconds(1700000000);
        const not_after = UnixTime.fromSeconds(1700086400);
        expect_eq(t2.isWithin(not_before, not_after), true);
        expect_eq(t0.isWithin(not_before, not_after), false);

        // Saturating underflow / overflow protection
        expect_eq(t0.subSeconds(100).asSeconds(), 0 as u64);
        const u64_max: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        const max_t = UnixTime.fromSeconds(u64_max - (5 as u64));
        expect_eq(max_t.addSeconds(100).asSeconds(), u64_max);
    });

    test("5. @cap/rustls-pki-types — detectPemKind identifies 7 RFC 7468 PKI PEM labels via @cap/pem-rfc7468", () => {
        const pem_cert = "-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----\n";
        const pem_pkcs8 = "-----BEGIN PRIVATE KEY-----\nAAAA\n-----END PRIVATE KEY-----\n";
        const pem_pkcs1 = "-----BEGIN RSA PRIVATE KEY-----\nAAAA\n-----END RSA PRIVATE KEY-----\n";
        const pem_sec1 = "-----BEGIN EC PRIVATE KEY-----\nAAAA\n-----END EC PRIVATE KEY-----\n";
        const pem_spki = "-----BEGIN PUBLIC KEY-----\nAAAA\n-----END PUBLIC KEY-----\n";
        const pem_crl = "-----BEGIN X509 CRL-----\nAAAA\n-----END X509 CRL-----\n";
        const pem_csr = "-----BEGIN CERTIFICATE REQUEST-----\nAAAA\n-----END CERTIFICATE REQUEST-----\n";
        const pem_other = "-----BEGIN UNKNOWN THING-----\nAAAA\n-----END UNKNOWN THING-----\n";

        expect_eq(detectPemKind(pem_cert), PEM_CERTIFICATE);
        expect_eq(detectPemKind(pem_pkcs8), PEM_PKCS8_KEY);
        expect_eq(detectPemKind(pem_pkcs1), PEM_PKCS1_KEY);
        expect_eq(detectPemKind(pem_sec1), PEM_SEC1_KEY);
        expect_eq(detectPemKind(pem_spki), PEM_PUBLIC_KEY);
        expect_eq(detectPemKind(pem_crl), PEM_CRL);
        expect_eq(detectPemKind(pem_csr), PEM_CSR);
        expect_eq(detectPemKind(pem_other), PEM_UNKNOWN);
        expect_eq(detectPemKind("not a pem block"), PEM_UNKNOWN);

        expect_eq(isPrivateKeyKind(PEM_PKCS8_KEY), true);
        expect_eq(isPrivateKeyKind(PEM_PKCS1_KEY), true);
        expect_eq(isPrivateKeyKind(PEM_SEC1_KEY), true);
        expect_eq(isPrivateKeyKind(PEM_CERTIFICATE), false);
        expect_eq(pkiTestEqBytes(pemKindLabel(PEM_CERTIFICATE), "CERTIFICATE"), true);
        expect_eq(pkiTestEqBytes(pemKindLabel(PEM_PKCS8_KEY), "PRIVATE KEY"), true);
    });

    test("6. @cap/rustls-pki-types — decodePemDer decodes Certificate & PrivateKey DER via @cap/pem-rfc7468", () => {
        // DER bytes: 0x30 0x06 0x02 0x01 0x01 0x02 0x01 0x02 -> Base64 "MAYCAQECAQIA" (actually 8 bytes: 30 06 02 01 01 02 01 02 -> "MAYCAQECAQIA" is 9 bytes; exact 8 bytes is "MAYCAQECAQI=")
        const cert_pem = "Subject: CN=test.local\n-----BEGIN CERTIFICATE-----\nMAYCAQECAQI=\n-----END CERTIFICATE-----\n";
        let der_out: [64]u8 = [0; 64];
        const res = decodePemDer(cert_pem, der_out[0..64]);
        expect_eq(res.isOk(), true);
        expect_eq(res.err, PKI_OK);
        expect_eq(res.kind, PEM_CERTIFICATE);
        expect_eq(res.isCertificate(), true);
        expect_eq(res.isPrivateKey(), false);
        expect_eq(res.der_len, 8 as usize);
        expect_eq(der_out[0], 0x30 as u8);
        expect_eq(der_out[1], 0x06 as u8);
        expect_eq(der_out[2], 0x02 as u8);
        expect_eq(der_out[3], 0x01 as u8);
        expect_eq(der_out[4], 0x01 as u8);
        expect_eq(der_out[5], 0x02 as u8);
        expect_eq(der_out[6], 0x01 as u8);
        expect_eq(der_out[7], 0x02 as u8);

        // PKCS#8 Private Key DER decode
        const key_pem = "-----BEGIN PRIVATE KEY-----\nSGVsbG8=\n-----END PRIVATE KEY-----\n";
        const key_res = decodePemDer(key_pem, der_out[0..64]);
        expect_eq(key_res.isOk(), true);
        expect_eq(key_res.kind, PEM_PKCS8_KEY);
        expect_eq(key_res.isPrivateKey(), true);
        expect_eq(key_res.der_len, 5 as usize);
        expect_eq(pkiTestEqBytes(der_out[0..5], "Hello"), true);

        // Unknown PEM label returns PKI_ERR_UNKNOWN_PEM_KIND
        const unk_pem = "-----BEGIN CUSTOM DATA-----\nSGVsbG8=\n-----END CUSTOM DATA-----\n";
        const unk_res = decodePemDer(unk_pem, der_out[0..64]);
        expect_eq(unk_res.isOk(), false);
        expect_eq(unk_res.err, PKI_ERR_UNKNOWN_PEM_KIND);

        // Corrupted PEM (mismatched END label) propagates negative error from @cap/pem-rfc7468
        const bad_pem = "-----BEGIN CERTIFICATE-----\nMAYCAQECAQI=\n-----END PUBLIC KEY-----\n";
        const bad_res = decodePemDer(bad_pem, der_out[0..64]);
        expect_eq(bad_res.isOk(), false);
        expect_eq(bad_res.err < 0, true);
    });
}
