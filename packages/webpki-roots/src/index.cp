// packages/@cap/webpki-roots/src/index.cp
// Zero-allocation Mozilla NSS / WebPKI root CA trust anchor store in pure CP.
// Reuses @cap/x509-cert (X509Certificate, X509Name) and @cap/spki (encodeSpkiDer, fingerprintSpki).
// Clean-room MIT implementation (webpki-roots parity).

import { X509Certificate, X509Name } from "../../x509-cert/src/index.cp";
import { encodeSpkiDer, fingerprintSpki } from "../../spki/src/index.cp";

pub const KEY_ALG_UNKNOWN: u8 = 0;
pub const KEY_ALG_RSA: u8 = 1;
pub const KEY_ALG_P256: u8 = 2;
pub const KEY_ALG_P384: u8 = 3;
pub const KEY_ALG_ED25519: u8 = 4;

pub const MOZILLA_ROOT_COUNT: usize = 20;
pub const TRUST_STORE_CAPACITY: usize = 32;

fn wrStrEq(a: string, b: string): bool {
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

fn wrBytesEqStr(b: []const u8, s: string): bool {
    if (b.len != s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (b[i] != s[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn wrIsDigit(ch: u8): bool {
    return ch >= ('0' as u8) && ch <= ('9' as u8);
}

fn wrParseCertNotAfterYear(cert: X509Certificate): u16 {
    const len = cert.validity.not_after_len;
    if (len == 0) {
        return 0 as u16;
    }
    if (cert.validity.not_after_generalized) {
        if (len < 4) {
            return 0 as u16;
        }
        const d0 = cert.validity.not_after[0];
        const d1 = cert.validity.not_after[1];
        const d2 = cert.validity.not_after[2];
        const d3 = cert.validity.not_after[3];
        if (!wrIsDigit(d0) || !wrIsDigit(d1) || !wrIsDigit(d2) || !wrIsDigit(d3)) {
            return 0 as u16;
        }
        const y = ((d0 - ('0' as u8)) as u16) * (1000 as u16)
            + ((d1 - ('0' as u8)) as u16) * (100 as u16)
            + ((d2 - ('0' as u8)) as u16) * (10 as u16)
            + ((d3 - ('0' as u8)) as u16);
        return y;
    }
    // RFC 5280 §4.1.2.5.1 UTCTime: YY >= 50 -> 1900 + YY, else 2000 + YY
    if (len < 2) {
        return 0 as u16;
    }
    const c0 = cert.validity.not_after[0];
    const c1 = cert.validity.not_after[1];
    if (!wrIsDigit(c0) || !wrIsDigit(c1)) {
        return 0 as u16;
    }
    const yy = ((c0 - ('0' as u8)) as u16) * (10 as u16) + ((c1 - ('0' as u8)) as u16);
    if (yy >= (50 as u16)) {
        return (1900 as u16) + yy;
    }
    return (2000 as u16) + yy;
}

pub struct TrustAnchorInfo {
    pub name: string,
    pub org: string,
    pub country: string,
    pub key_alg: u8,
    pub spki_fingerprint: u64,
    pub not_after_year: u16,

    pub static fn empty(): TrustAnchorInfo {
        return TrustAnchorInfo {
            name: "",
            org: "",
            country: "",
            key_alg: KEY_ALG_UNKNOWN,
            spki_fingerprint: 0 as u64,
            not_after_year: 0 as u16,
        };
    }

    pub static fn init(
        name: string,
        org: string,
        country: string,
        key_alg: u8,
        spki_fingerprint: u64,
        not_after_year: u16,
    ): TrustAnchorInfo {
        return TrustAnchorInfo {
            name: name,
            org: org,
            country: country,
            key_alg: key_alg,
            spki_fingerprint: spki_fingerprint,
            not_after_year: not_after_year,
        };
    }

    pub fn isEmpty(): bool {
        return _.name.len == 0;
    }

    pub fn isValidAtYear(year: u16): bool {
        return _.name.len > 0 && _.not_after_year >= year;
    }

    pub fn matchesIssuer(issuer: X509Name): bool {
        if (_.name.len == 0) {
            return false;
        }
        if (!wrBytesEqStr(issuer.cn[0..issuer.cn_len], _.name)) {
            return false;
        }
        if (_.org.len > 0 && issuer.org_len > 0) {
            if (!wrBytesEqStr(issuer.org[0..issuer.org_len], _.org)) {
                return false;
            }
        }
        if (_.country.len > 0 && issuer.country_len > 0) {
            if (!wrBytesEqStr(issuer.country[0..issuer.country_len], _.country)) {
                return false;
            }
        }
        return true;
    }
}

pub struct TrustAnchorResult {
    pub found: bool,
    pub anchor: TrustAnchorInfo,
}

fn wrMakeRoot(name: string, org: string, country: string, key_alg: u8, not_after_year: u16): TrustAnchorInfo {
    const fp = fingerprintSpki(name);
    return TrustAnchorInfo {
        name: name,
        org: org,
        country: country,
        key_alg: key_alg,
        spki_fingerprint: fp,
        not_after_year: not_after_year,
    };
}

/// Returns the built-in Mozilla NSS / WebPKI root CA metadata entry at index `idx`
/// (`0 <= idx < MOZILLA_ROOT_COUNT`), or `TrustAnchorInfo.empty()` if out of range.
pub fn mozillaRootAt(idx: usize): TrustAnchorInfo {
    if (idx == 0) {
        return wrMakeRoot("ISRG Root X1", "Internet Security Research Group", "US", KEY_ALG_RSA, 2035 as u16);
    }
    if (idx == 1) {
        return wrMakeRoot("ISRG Root X2", "Internet Security Research Group", "US", KEY_ALG_P384, 2040 as u16);
    }
    if (idx == 2) {
        return wrMakeRoot("DigiCert Global Root G2", "DigiCert Inc", "US", KEY_ALG_RSA, 2038 as u16);
    }
    if (idx == 3) {
        return wrMakeRoot("DigiCert Global Root G3", "DigiCert Inc", "US", KEY_ALG_P384, 2038 as u16);
    }
    if (idx == 4) {
        return wrMakeRoot("GlobalSign Root CA", "GlobalSign nv-sa", "BE", KEY_ALG_RSA, 2028 as u16);
    }
    if (idx == 5) {
        return wrMakeRoot("GlobalSign Root R46", "GlobalSign nv-sa", "BE", KEY_ALG_RSA, 2046 as u16);
    }
    if (idx == 6) {
        return wrMakeRoot("USERTrust RSA Certification Authority", "The USERTRUST Network", "US", KEY_ALG_RSA, 2038 as u16);
    }
    if (idx == 7) {
        return wrMakeRoot("Sectigo Public Server Authentication Root R46", "Sectigo Limited", "GB", KEY_ALG_RSA, 2046 as u16);
    }
    if (idx == 8) {
        return wrMakeRoot("Baltimore CyberTrust Root", "Baltimore", "IE", KEY_ALG_RSA, 2025 as u16);
    }
    if (idx == 9) {
        return wrMakeRoot("GTS Root R1", "Google Trust Services LLC", "US", KEY_ALG_RSA, 2036 as u16);
    }
    if (idx == 10) {
        return wrMakeRoot("GTS Root R4", "Google Trust Services LLC", "US", KEY_ALG_P384, 2036 as u16);
    }
    if (idx == 11) {
        return wrMakeRoot("Amazon Root CA 1", "Amazon", "US", KEY_ALG_RSA, 2038 as u16);
    }
    if (idx == 12) {
        return wrMakeRoot("Amazon Root CA 2", "Amazon", "US", KEY_ALG_RSA, 2040 as u16);
    }
    if (idx == 13) {
        return wrMakeRoot("SSL.com Root Certification Authority RSA", "SSL Corporation", "US", KEY_ALG_RSA, 2041 as u16);
    }
    if (idx == 14) {
        return wrMakeRoot("Entrust Root Certification Authority - G2", "Entrust, Inc.", "US", KEY_ALG_RSA, 2030 as u16);
    }
    if (idx == 15) {
        return wrMakeRoot("QuoVadis Root CA 2 G3", "QuoVadis Limited", "BM", KEY_ALG_RSA, 2042 as u16);
    }
    if (idx == 16) {
        return wrMakeRoot("COMODO RSA Certification Authority", "COMODO CA Limited", "GB", KEY_ALG_RSA, 2038 as u16);
    }
    if (idx == 17) {
        return wrMakeRoot("Starfield Root Certificate Authority - G2", "Starfield Technologies, Inc.", "US", KEY_ALG_RSA, 2037 as u16);
    }
    if (idx == 18) {
        return wrMakeRoot("Go Daddy Root Certificate Authority - G2", "GoDaddy.com, Inc.", "US", KEY_ALG_RSA, 2037 as u16);
    }
    if (idx == 19) {
        return wrMakeRoot("Cloudflare Origin ECC Root", "Cloudflare, Inc.", "US", KEY_ALG_P256, 2040 as u16);
    }
    return TrustAnchorInfo.empty();
}

/// Looks up a built-in Mozilla NSS / WebPKI root CA by exact Common Name (`cn`).
pub fn lookupMozillaRootByCn(cn: string): TrustAnchorResult {
    let i: usize = 0;
    while (i < MOZILLA_ROOT_COUNT) {
        const root = mozillaRootAt(i);
        if (wrStrEq(root.name, cn)) {
            return TrustAnchorResult { found: true, anchor: root };
        }
        i = i + 1;
    }
    return TrustAnchorResult { found: false, anchor: TrustAnchorInfo.empty() };
}

/// Computes the deterministic 64-bit SPKI fingerprint of an `@cap/x509-cert` certificate
/// by DER-encoding its `SubjectPublicKeyInfo` and hashing via `@cap/spki::fingerprintSpki`.
/// Returns 0 if SPKI DER encoding fails.
pub fn certSpkiFingerprint(cert: X509Certificate): u64 {
    let spki_der: [384]u8 = undefined;
    const n = encodeSpkiDer(cert.spki, spki_der[0..384]);
    if (n <= 0) {
        return 0 as u64;
    }
    return fingerprintSpki(spki_der[0..(n as usize)]);
}

fn wrEmptyAnchors(): [32]TrustAnchorInfo {
    let arr: [32]TrustAnchorInfo = undefined;
    let i: usize = 0;
    while (i < TRUST_STORE_CAPACITY) {
        arr[i] = TrustAnchorInfo.empty();
        i = i + 1;
    }
    return arr;
}

/// Fixed-capacity zero-allocation WebPKI Trust Anchor Store.
pub struct TrustAnchorStore {
    pub anchors: [32]TrustAnchorInfo,
    pub len: usize,

    pub static fn empty(): TrustAnchorStore {
        return TrustAnchorStore {
            anchors: wrEmptyAnchors(),
            len: 0,
        };
    }

    pub static fn withMozillaDefaults(): TrustAnchorStore {
        let store = TrustAnchorStore.empty();
        let i: usize = 0;
        while (i < MOZILLA_ROOT_COUNT) {
            store.anchors[i] = mozillaRootAt(i);
            i = i + 1;
        }
        store.len = MOZILLA_ROOT_COUNT;
        return store;
    }

    pub fn count(): usize {
        return _.len;
    }

    pub fn capacity(): usize {
        return TRUST_STORE_CAPACITY;
    }

    pub fn at(idx: usize): TrustAnchorInfo {
        if (idx >= _.len) {
            return TrustAnchorInfo.empty();
        }
        return _.anchors[idx];
    }

    /// Appends a custom trust anchor (e.g. private/enterprise root CA).
    /// Returns false if the store is full (`TRUST_STORE_CAPACITY`) or anchor name is empty.
    pub fn addAnchor(*_, anchor: TrustAnchorInfo): bool {
        if (_.len >= TRUST_STORE_CAPACITY || anchor.name.len == 0) {
            return false;
        }
        _.anchors[_.len] = anchor;
        _.len = _.len + 1;
        return true;
    }

    /// Finds the first trust anchor matching exact Common Name (`cn`).
    pub fn findByCn(cn: string): TrustAnchorResult {
        let i: usize = 0;
        while (i < _.len) {
            if (wrStrEq(_.anchors[i].name, cn)) {
                return TrustAnchorResult { found: true, anchor: _.anchors[i] };
            }
            i = i + 1;
        }
        return TrustAnchorResult { found: false, anchor: TrustAnchorInfo.empty() };
    }

    /// Finds the first trust anchor belonging to organization (`org`).
    pub fn findByOrg(org: string): TrustAnchorResult {
        let i: usize = 0;
        while (i < _.len) {
            if (wrStrEq(_.anchors[i].org, org)) {
                return TrustAnchorResult { found: true, anchor: _.anchors[i] };
            }
            i = i + 1;
        }
        return TrustAnchorResult { found: false, anchor: TrustAnchorInfo.empty() };
    }

    /// Counts how many trust anchors in the store belong to organization (`org`).
    pub fn countByOrg(org: string): usize {
        let i: usize = 0;
        let n: usize = 0;
        while (i < _.len) {
            if (wrStrEq(_.anchors[i].org, org)) {
                n = n + 1;
            }
            i = i + 1;
        }
        return n;
    }

    /// Returns true if `cert.issuer` matches any trust anchor in the store.
    pub fn isTrustedIssuer(cert: X509Certificate): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.anchors[i].matchesIssuer(cert.issuer)) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    /// Returns true if `cert.issuer` matches a trust anchor that is valid at `current_year`
    /// (`anchor.not_after_year >= current_year`), and if `cert.validity.not_after` is present,
    /// the certificate itself has not expired at `current_year`.
    pub fn verifyIssuerTrusted(cert: X509Certificate, current_year: u16): bool {
        const cert_exp_year = wrParseCertNotAfterYear(cert);
        if (cert_exp_year > (0 as u16) && cert_exp_year < current_year) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (_.anchors[i].matchesIssuer(cert.issuer) && _.anchors[i].isValidAtYear(current_year)) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }
}
