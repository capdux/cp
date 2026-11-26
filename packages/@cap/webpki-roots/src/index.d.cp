// Type definitions for CP

import { X509Certificate, X509Name } from "../../x509-cert/src/index.cp";
import { encodeSpkiDer, fingerprintSpki } from "../../spki/src/index.cp";
export fn wrStrEq(a: string, b: string): bool;
export fn wrBytesEqStr(b: &[]u8, s: string): bool;
export fn wrIsDigit(ch: u8): bool;
export fn wrParseCertNotAfterYear(cert: X509Certificate): u16;
export class TrustAnchorInfo {
    name: string,
    org: string,
    country: string,
    key_alg: u8,
    spki_fingerprint: u64,
    not_after_year: u16,
    fn empty(): TrustAnchorInfo;
    fn init(name: string, org: string, country: string, key_alg: u8, spki_fingerprint: u64, not_after_year: u16): TrustAnchorInfo;
    fn isEmpty(): bool;
    fn isValidAtYear(year: u16): bool;
    fn matchesIssuer(issuer: X509Name): bool;
}

export class TrustAnchorResult {
    found: bool,
    anchor: TrustAnchorInfo,
}

export fn wrMakeRoot(name: string, org: string, country: string, key_alg: u8, not_after_year: u16): TrustAnchorInfo;
export fn mozillaRootAt(idx: usize): TrustAnchorInfo;
export fn lookupMozillaRootByCn(cn: string): TrustAnchorResult;
export fn certSpkiFingerprint(cert: X509Certificate): u64;
export fn wrEmptyAnchors(): [32]TrustAnchorInfo;
export class TrustAnchorStore {
    anchors: [32]TrustAnchorInfo,
    len: usize,
    fn empty(): TrustAnchorStore;
    fn withMozillaDefaults(): TrustAnchorStore;
    fn count(): usize;
    fn capacity(): usize;
    fn at(idx: usize): TrustAnchorInfo;
    fn addAnchor(_: auto, anchor: TrustAnchorInfo): bool;
    fn findByCn(cn: string): TrustAnchorResult;
    fn findByOrg(org: string): TrustAnchorResult;
    fn countByOrg(org: string): usize;
    fn isTrustedIssuer(cert: X509Certificate): bool;
    fn verifyIssuerTrusted(cert: X509Certificate, current_year: u16): bool;
}

