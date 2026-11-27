// @cap/rustls-webpki — Pure CP zero-allocation WebPKI X.509 certificate verification engine
// Clean-room MIT implementation with parity to `rustls-webpki` (RFC 6125 & RFC 5280).
// Reuses `@cap/x509-cert` with zero duplication for X.509 certificate parsing, DN matching,
// validity representation, and self-signed detection.

import {
    X509Certificate,
    X509Validity,
    decodeX509Der,
    isSelfSigned,
} from "../../x509-cert/src/index.cp";

pub const WPKI_OK: i32 = 0;
pub const WPKI_ERR_NOT_YET_VALID: i32 = -1;
pub const WPKI_ERR_EXPIRED: i32 = -2;
pub const WPKI_ERR_INVALID_TIME: i32 = -3;
pub const WPKI_ERR_HOSTNAME_MISMATCH: i32 = -4;
pub const WPKI_ERR_INVALID_CERT: i32 = -5;
pub const WPKI_ERR_CHAIN_BROKEN: i32 = -6;
pub const WPKI_ERR_NOT_SELF_SIGNED: i32 = -7;

const WPKI_MAX_DNS_LEN: usize = 253;
const WPKI_MAX_LABEL_LEN: usize = 63;

/// Parsed RFC 5280 UTCTime or GeneralizedTime converted to UTC calendar fields and Unix seconds.
pub struct X509TimeResult {
    pub ok: bool,
    pub unix_secs: u64,
    pub year: u32,
    pub month: u8,
    pub day: u8,
    pub hour: u8,
    pub minute: u8,
    pub second: u8,

    pub static fn fail(): X509TimeResult {
        return X509TimeResult {
            ok: false,
            unix_secs: 0 as u64,
            year: 0 as u32,
            month: 0 as u8,
            day: 0 as u8,
            hour: 0 as u8,
            minute: 0 as u8,
            second: 0 as u8,
        };
    }
}

// -----------------------------------------------------------------------------
// Private ASCII / DNS helpers (all prefixed with `wpki` to avoid symbol collisions)
// -----------------------------------------------------------------------------

fn wpkiAsciiLower(b: u8): u8 {
    if (b >= (65 as u8) && b <= (90 as u8)) {
        return b + (32 as u8);
    }
    return b;
}

fn wpkiIsAsciiAlphaNum(b: u8): bool {
    if (b >= (97 as u8) && b <= (122 as u8)) {
        return true;
    }
    if (b >= (65 as u8) && b <= (90 as u8)) {
        return true;
    }
    if (b >= (48 as u8) && b <= (57 as u8)) {
        return true;
    }
    return false;
}

fn wpkiIsValidDnsLabelBytes(label: []const u8): bool {
    if (label.len == 0 || label.len > WPKI_MAX_LABEL_LEN) {
        return false;
    }
    // RFC 1035 / RFC 1123: label cannot start or end with '-'
    if (label[0] == (45 as u8) || label[label.len - 1] == (45 as u8)) {
        return false;
    }
    let i: usize = 0;
    while (i < label.len) {
        const c = label[i];
        if (!wpkiIsAsciiAlphaNum(c) && c != (45 as u8)) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn wpkiValidateDnsNameBytes(name: []const u8): bool {
    if (name.len == 0 || name.len > WPKI_MAX_DNS_LEN) {
        return false;
    }
    let start: usize = 0;
    let i: usize = 0;
    while (i <= name.len) {
        if (i == name.len || name[i] == (46 as u8)) {
            if (!wpkiIsValidDnsLabelBytes(name[start..i])) {
                return false;
            }
            start = i + 1;
        }
        i = i + 1;
    }
    return true;
}

fn wpkiCountDotsBytes(s: []const u8): usize {
    let dots: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == (46 as u8)) {
            dots = dots + 1;
        }
        i = i + 1;
    }
    return dots;
}

fn wpkiEqIgnoreCaseBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (wpkiAsciiLower(a[i]) != wpkiAsciiLower(b[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn wpkiHasStarBytes(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == (42 as u8)) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Core RFC 6125 DNS hostname matching on raw byte slices.
///
/// Rules enforced:
/// - Case-insensitive ASCII matching.
/// - Both pattern labels and hostname labels must be valid non-empty DNS labels (1..63 bytes,
///   ASCII `[A-Za-z0-9-]`, no leading/trailing `-`, no empty labels such as leading/trailing/double dots).
/// - Wildcard `*` is ONLY permitted as the entire leftmost label (`*.rest`), never partial
///   (`a*.example.com`, `*a.example.com`) and never in multiple labels (`*.*.example.com`).
/// - Wildcard suffix `rest` must contain at least two labels (at least one `.`), rejecting
///   top-level wildcards such as `*.com`.
/// - Wildcard matches exactly ONE non-empty label in `hostname` (never crosses `.`, and never
///   matches the bare apex domain `example.com`).
pub fn verifyDnsHostnameBytes(pattern: []const u8, hostname: []const u8): bool {
    if (pattern.len == 0 || pattern.len > WPKI_MAX_DNS_LEN) {
        return false;
    }
    if (!wpkiValidateDnsNameBytes(hostname)) {
        return false;
    }

    if (!wpkiHasStarBytes(pattern)) {
        if (!wpkiValidateDnsNameBytes(pattern)) {
            return false;
        }
        return wpkiEqIgnoreCaseBytes(pattern, hostname);
    }

    // Wildcard must be the entire leftmost label: "*." followed by suffix
    if (pattern.len < 5) {
        return false;
    }
    if (pattern[0] != (42 as u8) || pattern[1] != (46 as u8)) {
        return false;
    }

    const suffix = pattern[2..pattern.len];
    // Reject additional wildcards such as "*.*.example.com" or "*.a*.example.com"
    if (wpkiHasStarBytes(suffix)) {
        return false;
    }
    // Suffix must be a valid DNS name with at least 2 labels (rejects "*.com")
    if (wpkiCountDotsBytes(suffix) < 1) {
        return false;
    }
    if (!wpkiValidateDnsNameBytes(suffix)) {
        return false;
    }

    // Find first '.' in hostname: must match exactly one single label before suffix
    let first_dot: usize = 0;
    let found_dot = false;
    let i: usize = 0;
    while (i < hostname.len) {
        if (hostname[i] == (46 as u8)) {
            first_dot = i;
            found_dot = true;
            break;
        }
        i = i + 1;
    }
    if (!found_dot || first_dot == 0) {
        return false;
    }

    const host_rest = hostname[(first_dot + 1)..hostname.len];
    return wpkiEqIgnoreCaseBytes(suffix, host_rest);
}

/// Strictly match a certificate DNS pattern against a target hostname per RFC 6125.
pub fn verifyDnsHostname(pattern: string, hostname: string): bool {
    return verifyDnsHostnameBytes(pattern[0..pattern.len], hostname[0..hostname.len]);
}

// -----------------------------------------------------------------------------
// RFC 5280 §4.1.2.5 UTCTime & GeneralizedTime -> Unix timestamp conversion
// -----------------------------------------------------------------------------

fn wpkiDigit(b: u8): i32 {
    if (b >= (48 as u8) && b <= (57 as u8)) {
        return (b - (48 as u8)) as i32;
    }
    return -1;
}

fn wpkiParse2Digits(buf: []const u8, off: usize): i32 {
    const d0 = wpkiDigit(buf[off]);
    const d1 = wpkiDigit(buf[off + 1]);
    if (d0 < 0 || d1 < 0) {
        return -1;
    }
    return d0 * 10 + d1;
}

fn wpkiParse4Digits(buf: []const u8, off: usize): i32 {
    const hi = wpkiParse2Digits(buf, off);
    const lo = wpkiParse2Digits(buf, off + 2);
    if (hi < 0 || lo < 0) {
        return -1;
    }
    return hi * 100 + lo;
}

fn wpkiIsLeapYear(year: u32): bool {
    if ((year % (400 as u32)) == (0 as u32)) {
        return true;
    }
    if ((year % (100 as u32)) == (0 as u32)) {
        return false;
    }
    return (year % (4 as u32)) == (0 as u32);
}

fn wpkiDaysInMonth(year: u32, month: u8): u8 {
    if (month == (2 as u8)) {
        if (wpkiIsLeapYear(year)) {
            return 29 as u8;
        }
        return 28 as u8;
    }
    if (month == (4 as u8) || month == (6 as u8) || month == (9 as u8) || month == (11 as u8)) {
        return 30 as u8;
    }
    return 31 as u8;
}

/// Convert civil UTC date (year >= 1970) to days since 1970-01-01 (Howard Hinnant algorithm).
fn wpkiDaysFromCivil(year: u32, month: u8, day: u8): u64 {
    let y: u64 = year as u64;
    let m: u64 = month as u64;
    if (m <= (2 as u64)) {
        y = y - (1 as u64);
    }
    const era: u64 = y / (400 as u64);
    const yoe: u64 = y - era * (400 as u64);
    let mp: u64 = 0 as u64;
    if (m > (2 as u64)) {
        mp = m - (3 as u64);
    } else {
        mp = m + (9 as u64);
    }
    const doy: u64 = ((153 as u64) * mp + (2 as u64)) / (5 as u64) + (day as u64) - (1 as u64);
    const doe: u64 = yoe * (365 as u64) + yoe / (4 as u64) - yoe / (100 as u64) + doy;
    return era * (146097 as u64) + doe - (719468 as u64);
}

/// Parse an RFC 5280 X.509 UTCTime (`YYMMDDHHMMSSZ`, 13 bytes) or GeneralizedTime
/// (`YYYYMMDDHHMMSSZ`, 15 bytes) into UTC calendar components and Unix seconds.
///
/// Per RFC 5280 §4.1.2.5.1, UTCTime `00..49` maps to `2000..2049` and `50..99` maps
/// to `1950..1999`. Years prior to 1970 cannot be represented as unsigned Unix seconds
/// and return `ok: false`.
pub fn parseX509TimeUtc(time_bytes: []const u8): X509TimeResult {
    let year_i: i32 = 0;
    let off: usize = 0;

    if (time_bytes.len == 13) {
        // UTCTime: YYMMDDHHMMSSZ
        if (time_bytes[12] != (90 as u8)) {
            return X509TimeResult.fail();
        }
        const yy = wpkiParse2Digits(time_bytes, 0);
        if (yy < 0) {
            return X509TimeResult.fail();
        }
        if (yy < 50) {
            year_i = 2000 + yy;
        } else {
            year_i = 1900 + yy;
        }
        off = 2;
    } else if (time_bytes.len == 15) {
        // GeneralizedTime: YYYYMMDDHHMMSSZ
        if (time_bytes[14] != (90 as u8)) {
            return X509TimeResult.fail();
        }
        year_i = wpkiParse4Digits(time_bytes, 0);
        if (year_i < 0) {
            return X509TimeResult.fail();
        }
        off = 4;
    } else {
        return X509TimeResult.fail();
    }

    if (year_i < 1970 || year_i > 9999) {
        return X509TimeResult.fail();
    }

    const month_i = wpkiParse2Digits(time_bytes, off);
    const day_i = wpkiParse2Digits(time_bytes, off + 2);
    const hour_i = wpkiParse2Digits(time_bytes, off + 4);
    const min_i = wpkiParse2Digits(time_bytes, off + 6);
    const sec_i = wpkiParse2Digits(time_bytes, off + 8);

    if (month_i < 1 || month_i > 12) {
        return X509TimeResult.fail();
    }
    const year = year_i as u32;
    const month = month_i as u8;
    const max_day = wpkiDaysInMonth(year, month);
    if (day_i < 1 || day_i > (max_day as i32)) {
        return X509TimeResult.fail();
    }
    if (hour_i < 0 || hour_i > 23) {
        return X509TimeResult.fail();
    }
    if (min_i < 0 || min_i > 59) {
        return X509TimeResult.fail();
    }
    if (sec_i < 0 || sec_i > 59) {
        return X509TimeResult.fail();
    }

    const day = day_i as u8;
    const hour = hour_i as u8;
    const minute = min_i as u8;
    const second = sec_i as u8;

    const days = wpkiDaysFromCivil(year, month, day);
    const unix_secs = days * (86400 as u64)
        + (hour as u64) * (3600 as u64)
        + (minute as u64) * (60 as u64)
        + (second as u64);

    return X509TimeResult {
        ok: true,
        unix_secs: unix_secs,
        year: year,
        month: month,
        day: day,
        hour: hour,
        minute: minute,
        second: second,
    };
}

// -----------------------------------------------------------------------------
// RFC 5280 Certificate Validity, Hostname & Chain Verification
// -----------------------------------------------------------------------------

/// Verify an `X509Validity` window against `now_unix` (inclusive `[not_before, not_after]`).
/// Returns `WPKI_OK (0)`, `WPKI_ERR_NOT_YET_VALID (-1)`, `WPKI_ERR_EXPIRED (-2)`,
/// or `WPKI_ERR_INVALID_TIME (-3)`.
pub fn verifyValidityWindow(validity: X509Validity, now_unix: u64): i32 {
    if (validity.not_before_len == 0 || validity.not_before_len > 20) {
        return WPKI_ERR_INVALID_TIME;
    }
    if (validity.not_after_len == 0 || validity.not_after_len > 20) {
        return WPKI_ERR_INVALID_TIME;
    }
    const nb = parseX509TimeUtc(validity.not_before[0..validity.not_before_len]);
    if (!nb.ok) {
        return WPKI_ERR_INVALID_TIME;
    }
    const na = parseX509TimeUtc(validity.not_after[0..validity.not_after_len]);
    if (!na.ok) {
        return WPKI_ERR_INVALID_TIME;
    }
    if (na.unix_secs < nb.unix_secs) {
        return WPKI_ERR_INVALID_TIME;
    }
    if (now_unix < nb.unix_secs) {
        return WPKI_ERR_NOT_YET_VALID;
    }
    if (now_unix > na.unix_secs) {
        return WPKI_ERR_EXPIRED;
    }
    return WPKI_OK;
}

/// Verify an `X509Certificate` validity period at `now_unix`.
/// Returns `WPKI_OK (0)`, `WPKI_ERR_NOT_YET_VALID (-1)`, `WPKI_ERR_EXPIRED (-2)`,
/// or `WPKI_ERR_INVALID_TIME (-3)`.
pub fn verifyValidityAt(cert: X509Certificate, now_unix: u64): i32 {
    return verifyValidityWindow(cert.validity, now_unix);
}

/// Verify an `X509Certificate` for a target DNS `hostname` at `now_unix`:
/// 1. Validates certificate validity period (`not_before <= now_unix <= not_after`).
/// 2. Validates `cert.subject.cn` matches `hostname` strictly per RFC 6125.
pub fn verifyCertForDnsName(cert: X509Certificate, hostname: string, now_unix: u64): i32 {
    const val_rc = verifyValidityAt(cert, now_unix);
    if (val_rc != WPKI_OK) {
        return val_rc;
    }
    if (cert.subject.cn_len == 0) {
        return WPKI_ERR_HOSTNAME_MISMATCH;
    }
    if (!verifyDnsHostnameBytes(cert.subject.cn[0..cert.subject.cn_len], hostname[0..hostname.len])) {
        return WPKI_ERR_HOSTNAME_MISMATCH;
    }
    return WPKI_OK;
}

/// Decode a DER-encoded X.509 certificate (via `@cap/x509-cert` `decodeX509Der`) and
/// verify its validity period and RFC 6125 DNS hostname match.
pub fn verifyDerCertForDnsName(der: []const u8, hostname: string, now_unix: u64): i32 {
    const decoded = decodeX509Der(der);
    if (!decoded.ok) {
        return WPKI_ERR_INVALID_CERT;
    }
    return verifyCertForDnsName(decoded.cert, hostname, now_unix);
}

/// Verify issuer-to-subject chain linkage (`leaf.issuer == issuer.subject`, non-empty issuer DN).
pub fn verifyChainLink(leaf: X509Certificate, issuer: X509Certificate): bool {
    const issuer_has_dn = issuer.subject.cn_len > 0
        || issuer.subject.org_len > 0
        || issuer.subject.country_len > 0;
    if (!issuer_has_dn) {
        return false;
    }
    return leaf.issuer.eq(issuer.subject);
}

/// Verify a root trust anchor is self-issued (`isSelfSigned` from `@cap/x509-cert`)
/// and valid at `now_unix`.
pub fn verifyTrustAnchor(anchor: X509Certificate, now_unix: u64): i32 {
    if (!isSelfSigned(anchor)) {
        return WPKI_ERR_NOT_SELF_SIGNED;
    }
    return verifyValidityAt(anchor, now_unix);
}

/// Verify an end-entity leaf certificate issued by `issuer` for `hostname` at `now_unix`:
/// checks chain linkage, issuer validity, leaf validity, and RFC 6125 hostname match.
pub fn verifyEndEntityChain(
    leaf: X509Certificate,
    issuer: X509Certificate,
    hostname: string,
    now_unix: u64
): i32 {
    if (!verifyChainLink(leaf, issuer)) {
        return WPKI_ERR_CHAIN_BROKEN;
    }
    const issuer_rc = verifyValidityAt(issuer, now_unix);
    if (issuer_rc != WPKI_OK) {
        return issuer_rc;
    }
    return verifyCertForDnsName(leaf, hostname, now_unix);
}

export {
    WPKI_OK,
    WPKI_ERR_NOT_YET_VALID,
    WPKI_ERR_EXPIRED,
    WPKI_ERR_INVALID_TIME,
    WPKI_ERR_HOSTNAME_MISMATCH,
    WPKI_ERR_INVALID_CERT,
    WPKI_ERR_CHAIN_BROKEN,
    WPKI_ERR_NOT_SELF_SIGNED,
    X509TimeResult,
    verifyDnsHostnameBytes,
    verifyDnsHostname,
    parseX509TimeUtc,
    verifyValidityWindow,
    verifyValidityAt,
    verifyCertForDnsName,
    verifyDerCertForDnsName,
    verifyChainLink,
    verifyTrustAnchor,
    verifyEndEntityChain,
};
