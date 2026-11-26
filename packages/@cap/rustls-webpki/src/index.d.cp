// Type definitions for CP

import { X509Certificate, X509Validity, decodeX509Der, isSelfSigned } from "../../x509-cert/src/index.cp";
export class X509TimeResult {
    ok: bool,
    unix_secs: u64,
    year: u32,
    month: u8,
    day: u8,
    hour: u8,
    minute: u8,
    second: u8,
    fn fail(): X509TimeResult;
}

export fn wpkiAsciiLower(b: u8): u8;
export fn wpkiIsAsciiAlphaNum(b: u8): bool;
export fn wpkiIsValidDnsLabelBytes(label: &[]u8): bool;
export fn wpkiValidateDnsNameBytes(name: &[]u8): bool;
export fn wpkiCountDotsBytes(s: &[]u8): usize;
export fn wpkiEqIgnoreCaseBytes(a: &[]u8, b: &[]u8): bool;
export fn wpkiHasStarBytes(s: &[]u8): bool;
export fn verifyDnsHostnameBytes(pattern: &[]u8, hostname: &[]u8): bool;
export fn verifyDnsHostname(pattern: string, hostname: string): bool;
export fn wpkiDigit(b: u8): i32;
export fn wpkiParse2Digits(buf: &[]u8, off: usize): i32;
export fn wpkiParse4Digits(buf: &[]u8, off: usize): i32;
export fn wpkiIsLeapYear(year: u32): bool;
export fn wpkiDaysInMonth(year: u32, month: u8): u8;
export fn wpkiDaysFromCivil(year: u32, month: u8, day: u8): u64;
export fn parseX509TimeUtc(time_bytes: &[]u8): X509TimeResult;
export fn verifyValidityWindow(validity: X509Validity, now_unix: u64): i32;
export fn verifyValidityAt(cert: X509Certificate, now_unix: u64): i32;
export fn verifyCertForDnsName(cert: X509Certificate, hostname: string, now_unix: u64): i32;
export fn verifyDerCertForDnsName(der: &[]u8, hostname: string, now_unix: u64): i32;
export fn verifyChainLink(leaf: X509Certificate, issuer: X509Certificate): bool;
export fn verifyTrustAnchor(anchor: X509Certificate, now_unix: u64): i32;
export fn verifyEndEntityChain(leaf: X509Certificate, issuer: X509Certificate, hostname: string, now_unix: u64): i32;
