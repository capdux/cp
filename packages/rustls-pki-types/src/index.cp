// packages/@cap/rustls-pki-types/src/index.cp
// Zero-allocation TLS/PKI core types in pure CP (rustls-pki-types v1 parity).
// Reuses @cap/pem-rfc7468 for RFC 7468 PEM label extraction and DER decoding.
// Clean-room MIT implementation.

import {
    decodeLabel,
    decodePem,
} from "../../pem-rfc7468/src/index.cp";
import { isDigit, isAlpha, hexVal, eqIgnoreCase, toLower } from "cap:ascii";
import { bytesEq } from "cap:bin";

// --- Status & Error Codes ---
pub const PKI_OK: i64 = 0;
pub const PKI_ERR_INVALID_DNS_NAME: i64 = -101;
pub const PKI_ERR_INVALID_IP_ADDR: i64 = -102;
pub const PKI_ERR_INVALID_SERVER_NAME: i64 = -103;
pub const PKI_ERR_UNKNOWN_PEM_KIND: i64 = -104;

// --- ServerName Kind Discriminants ---
pub const SERVER_NAME_INVALID: u8 = 0;
pub const SERVER_NAME_DNS: u8 = 1;
pub const SERVER_NAME_IPV4: u8 = 2;
pub const SERVER_NAME_IPV6: u8 = 3;

// --- PEM Object Kind Discriminants (RFC 7468 / rustls-pki-types) ---
pub const PEM_UNKNOWN: u8 = 0;
pub const PEM_CERTIFICATE: u8 = 1;
pub const PEM_PKCS8_KEY: u8 = 2;
pub const PEM_PKCS1_KEY: u8 = 3;
pub const PEM_SEC1_KEY: u8 = 4;
pub const PEM_PUBLIC_KEY: u8 = 5;
pub const PEM_CRL: u8 = 6;
pub const PEM_CSR: u8 = 7;

// --- Private Helper Functions (pki-prefixed to avoid Zig flattening collisions) ---

fn pkiIsDnsChar(ch: u8): bool {
    return isAlpha(ch) || isDigit(ch) || ch == ('-' as u8);
}

fn pkiEqIgnoreCase(a: string, b: string): bool {
    return eqIgnoreCase(a, b);
}

fn pkiClassifyPemLabel(label: []const u8): u8 {
    if (bytesEq(label, "CERTIFICATE")) {
        return PEM_CERTIFICATE;
    }
    if (bytesEq(label, "PRIVATE KEY")) {
        return PEM_PKCS8_KEY;
    }
    if (bytesEq(label, "RSA PRIVATE KEY")) {
        return PEM_PKCS1_KEY;
    }
    if (bytesEq(label, "EC PRIVATE KEY")) {
        return PEM_SEC1_KEY;
    }
    if (bytesEq(label, "PUBLIC KEY")) {
        return PEM_PUBLIC_KEY;
    }
    if (bytesEq(label, "X509 CRL")) {
        return PEM_CRL;
    }
    if (bytesEq(label, "CERTIFICATE REQUEST")) {
        return PEM_CSR;
    }
    return PEM_UNKNOWN;
}

fn pkiParseIpv4Slice(s: string, start: usize, end: usize, out: *[4]u8): bool {
    if (start >= end) {
        return false;
    }
    let octets: [4]u8 = undefined;
    let octet_idx: usize = 0;
    let pos: usize = start;

    while (octet_idx < (4 as usize)) {
        if (pos >= end) {
            return false;
        }
        let val: u32 = 0;
        let digits: usize = 0;
        const first_ch = s[pos];
        while (pos < end && isDigit(s[pos])) {
            if (digits >= (3 as usize)) {
                return false;
            }
            val = val * (10 as u32) + ((s[pos] - ('0' as u8)) as u32);
            digits = digits + 1;
            pos = pos + 1;
        }
        if (digits == 0) {
            return false;
        }
        // Reject leading zeros such as "01" or "00"
        if (digits > (1 as usize) && first_ch == ('0' as u8)) {
            return false;
        }
        if (val > (255 as u32)) {
            return false;
        }
        octets[octet_idx] = val as u8;
        octet_idx = octet_idx + 1;

        if (octet_idx < (4 as usize)) {
            if (pos >= end || s[pos] != ('.' as u8)) {
                return false;
            }
            pos = pos + 1;
        }
    }

    if (pos != end) {
        return false;
    }
    out.*[0] = octets[0];
    out.*[1] = octets[1];
    out.*[2] = octets[2];
    out.*[3] = octets[3];
    return true;
}

fn pkiParseIpv6Slice(input: string, out: *[16]u8): bool {
    if (input.len < (2 as usize)) {
        return false;
    }
    let start: usize = 0;
    let end: usize = input.len;

    // Allow optional surrounding brackets "[::1]"
    if (input[0] == ('[' as u8)) {
        if (input[end - 1] != (']' as u8)) {
            return false;
        }
        start = 1;
        end = end - 1;
        if (end - start < (2 as usize)) {
            return false;
        }
    } else if (input[end - 1] == (']' as u8)) {
        return false;
    }

    let head: [8]u16 = undefined;
    let head_len: usize = 0;
    let tail: [8]u16 = undefined;
    let tail_len: usize = 0;
    let seen_compress = false;
    let pos: usize = start;

    // Leading "::"
    if (pos + 1 < end && input[pos] == (':' as u8) && input[pos + 1] == (':' as u8)) {
        seen_compress = true;
        pos = pos + 2;
        if (pos == end) {
            // "::" (all-zeros address)
            let z: usize = 0;
            while (z < (16 as usize)) {
                out.*[z] = 0;
                z = z + 1;
            }
            return true;
        }
    } else if (input[pos] == (':' as u8)) {
        // Single leading ':' is invalid
        return false;
    }

    while (pos < end) {
        // Check if remaining segment is an embedded IPv4 address (contains '.')
        let scan: usize = pos;
        let has_dot = false;
        while (scan < end && input[scan] != (':' as u8)) {
            if (input[scan] == ('.' as u8)) {
                has_dot = true;
                break;
            }
            scan = scan + 1;
        }

        if (has_dot) {
            let v4: [4]u8 = undefined;
            if (!pkiParseIpv4Slice(input, pos, end, &v4)) {
                return false;
            }
            const hi: u16 = ((v4[0] as u16) << 8) | (v4[1] as u16);
            const lo: u16 = ((v4[2] as u16) << 8) | (v4[3] as u16);
            if (seen_compress) {
                if (head_len + tail_len + 2 > (7 as usize)) {
                    return false;
                }
                tail[tail_len] = hi;
                tail[tail_len + 1] = lo;
                tail_len = tail_len + 2;
            } else {
                if (head_len + 2 != (8 as usize)) {
                    return false;
                }
                head[head_len] = hi;
                head[head_len + 1] = lo;
                head_len = head_len + 2;
            }
            pos = end;
            break;
        }

        // Parse 1..4 hex digits for one 16-bit group
        let group: u16 = 0;
        let digits: usize = 0;
        while (pos < end && input[pos] != (':' as u8)) {
            if (hv := hexVal(input[pos])) {
                if (digits >= (4 as usize)) {
                    return false;
                }
                group = (group << 4) | (hv as u16);
                digits = digits + 1;
                pos = pos + 1;
            } else {
                return false;
            }
        }
        if (digits == 0) {
            return false;
        }

        if (seen_compress) {
            if (head_len + tail_len >= (7 as usize)) {
                return false;
            }
            tail[tail_len] = group;
            tail_len = tail_len + 1;
        } else {
            if (head_len >= (8 as usize)) {
                return false;
            }
            head[head_len] = group;
            head_len = head_len + 1;
        }

        if (pos == end) {
            break;
        }

        // Must be at ':'
        if (input[pos] != (':' as u8)) {
            return false;
        }
        pos = pos + 1;
        if (pos == end) {
            // Trailing single ':' is invalid
            return false;
        }

        if (input[pos] == (':' as u8)) {
            if (seen_compress) {
                // Only one "::" allowed
                return false;
            }
            seen_compress = true;
            pos = pos + 1;
            if (pos == end) {
                // Trailing "::" (e.g., "fe80::")
                break;
            }
            if (input[pos] == (':' as u8)) {
                // ":::" is invalid
                return false;
            }
        }
    }

    if (seen_compress) {
        if (head_len + tail_len > (7 as usize)) {
            return false;
        }
    } else {
        if (head_len != (8 as usize)) {
            return false;
        }
    }

    let groups: [8]u16 = [0; 8];
    let hi_i: usize = 0;
    while (hi_i < head_len) {
        groups[hi_i] = head[hi_i];
        hi_i = hi_i + 1;
    }
    const tail_start: usize = (8 as usize) - tail_len;
    let ti: usize = 0;
    while (ti < tail_len) {
        groups[tail_start + ti] = tail[ti];
        ti = ti + 1;
    }

    let gi: usize = 0;
    while (gi < (8 as usize)) {
        out.*[gi * 2] = ((groups[gi] >> 8) & (0xFF as u16)) as u8;
        out.*[gi * 2 + 1] = (groups[gi] & (0xFF as u16)) as u8;
        gi = gi + 1;
    }
    return true;
}

// --- DnsName (RFC 1035 / RFC 5890 / rustls-pki-types) ---

pub struct DnsName {
    pub buf: [253]u8,
    pub len: usize,
    pub valid: bool,

    pub static fn invalid(): DnsName {
        return DnsName {
            buf: [0; 253],
            len: 0,
            valid: false,
        };
    }

    /// Parses and validates a DNS name per RFC 1035 / RFC 5890 / rustls-pki-types:
    /// - Strips a single optional trailing dot ('.') for canonical form.
    /// - Total length must be 1..=253 bytes after trailing-dot normalization.
    /// - Each dot-separated label must be 1..=63 ASCII bytes of [A-Za-z0-9-].
    /// - Labels may not begin or end with '-'.
    /// - Rejects all-numeric TLDs (preventing IPv4 misclassification).
    pub static fn parse(input: string): DnsName {
        if (input.len == 0) {
            return DnsName.invalid();
        }
        let eff_len: usize = input.len;
        if (input[eff_len - 1] == ('.' as u8)) {
            eff_len = eff_len - 1;
        }
        if (eff_len == 0 || eff_len > (253 as usize)) {
            return DnsName.invalid();
        }

        let out_buf: [253]u8 = undefined;
        let label_len: usize = 0;
        let label_all_digits = true;
        let last_ch: u8 = 0;

        let i: usize = 0;
        while (i < eff_len) {
            const ch = input[i];
            if (ch == ('.' as u8)) {
                if (label_len == 0 || last_ch == ('-' as u8)) {
                    return DnsName.invalid();
                }
                label_len = 0;
                label_all_digits = true;
                last_ch = ch;
            } else {
                if (!pkiIsDnsChar(ch)) {
                    return DnsName.invalid();
                }
                if (label_len == 0 && ch == ('-' as u8)) {
                    return DnsName.invalid();
                }
                label_len = label_len + 1;
                if (label_len > (63 as usize)) {
                    return DnsName.invalid();
                }
                if (!isDigit(ch)) {
                    label_all_digits = false;
                }
                last_ch = ch;
            }
            out_buf[i] = ch;
            i = i + 1;
        }

        // Validate final label (TLD)
        if (label_len == 0 || last_ch == ('-' as u8) || label_all_digits) {
            return DnsName.invalid();
        }

        return DnsName {
            buf: out_buf,
            len: eff_len,
            valid: true,
        };
    }

    pub fn isValid(): bool {
        return _.valid;
    }

    pub fn eqIgnoreCase(other: DnsName): bool {
        if (!_.valid || !other.valid) {
            return false;
        }
        return pkiEqIgnoreCase(_.buf[0.._.len], other.buf[0..other.len]);
    }

    pub fn eqIgnoreCaseStr(other: string): bool {
        const parsed = DnsName.parse(other);
        return _.eqIgnoreCase(parsed);
    }

    pub fn labelCount(): usize {
        if (!_.valid || _.len == 0) {
            return 0;
        }
        let count: usize = 1;
        let i: usize = 0;
        while (i < _.len) {
            if (_.buf[i] == ('.' as u8)) {
                count = count + 1;
            }
            i = i + 1;
        }
        return count;
    }

    /// Returns true if `self` is a strict subdomain of `parent` (case-insensitive),
    /// e.g., `api.example.com` is a subdomain of `example.com`.
    pub fn isSubdomainOf(parent: DnsName): bool {
        if (!_.valid || !parent.valid) {
            return false;
        }
        if (_.len <= parent.len + 1) {
            return false;
        }
        const offset = _.len - parent.len;
        if (_.buf[offset - 1] != ('.' as u8)) {
            return false;
        }
        return pkiEqIgnoreCase(_.buf[offset.._.len], parent.buf[0..parent.len]);
    }

    /// Writes the canonical ASCII-lowercased DNS name into `out`.
    /// Returns bytes written, or 0 if invalid or `out` is too small.
    pub fn toLowercase(out: []u8): usize {
        if (!_.valid || out.len < _.len) {
            return 0;
        }
        return toLower(_.buf[0.._.len], out);
    }
}

pub fn isValidDnsName(input: string): bool {
    return DnsName.parse(input).valid;
}

pub fn parseDnsName(input: string): DnsName {
    return DnsName.parse(input);
}

// --- ServerName (DNS | IPv4 | IPv6) ---

pub struct ServerName {
    pub kind: u8,
    pub dns: DnsName,
    pub ipv4: [4]u8,
    pub ipv6: [16]u8,

    pub static fn invalid(): ServerName {
        return ServerName {
            kind: SERVER_NAME_INVALID,
            dns: DnsName.invalid(),
            ipv4: [0; 4],
            ipv6: [0; 16],
        };
    }

    pub static fn fromDns(dns: DnsName): ServerName {
        if (!dns.valid) {
            return ServerName.invalid();
        }
        return ServerName {
            kind: SERVER_NAME_DNS,
            dns: dns,
            ipv4: [0; 4],
            ipv6: [0; 16],
        };
    }

    pub static fn fromIpv4(octets: [4]u8): ServerName {
        return ServerName {
            kind: SERVER_NAME_IPV4,
            dns: DnsName.invalid(),
            ipv4: octets,
            ipv6: [0; 16],
        };
    }

    pub static fn fromIpv6(octets: [16]u8): ServerName {
        return ServerName {
            kind: SERVER_NAME_IPV6,
            dns: DnsName.invalid(),
            ipv4: [0; 4],
            ipv6: octets,
        };
    }

    /// Automatically parses an IPv4 address, IPv6 address (with or without `[...]`),
    /// or RFC 1035/5890 DNS hostname into a `ServerName`.
    pub static fn parse(input: string): ServerName {
        if (input.len == 0) {
            return ServerName.invalid();
        }
        let v4: [4]u8 = undefined;
        if (pkiParseIpv4Slice(input, 0, input.len, &v4)) {
            return ServerName.fromIpv4(v4);
        }
        let v6: [16]u8 = undefined;
        if (pkiParseIpv6Slice(input, &v6)) {
            return ServerName.fromIpv6(v6);
        }
        const dns = DnsName.parse(input);
        if (dns.valid) {
            return ServerName.fromDns(dns);
        }
        return ServerName.invalid();
    }

    pub fn isValid(): bool {
        return _.kind != SERVER_NAME_INVALID;
    }

    pub fn isDns(): bool {
        return _.kind == SERVER_NAME_DNS;
    }

    pub fn isIpv4(): bool {
        return _.kind == SERVER_NAME_IPV4;
    }

    pub fn isIpv6(): bool {
        return _.kind == SERVER_NAME_IPV6;
    }

    pub fn isIp(): bool {
        return _.kind == SERVER_NAME_IPV4 || _.kind == SERVER_NAME_IPV6;
    }
}

pub fn parseServerName(input: string): ServerName {
    return ServerName.parse(input);
}

// --- UnixTime ---

pub struct UnixTime {
    pub seconds: u64,

    pub static fn epoch(): UnixTime {
        return UnixTime { seconds: 0 };
    }

    pub static fn fromSeconds(secs: u64): UnixTime {
        return UnixTime { seconds: secs };
    }

    pub fn asSeconds(): u64 {
        return _.seconds;
    }

    pub fn addSeconds(delta: u64): UnixTime {
        const max_u64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        if (delta > max_u64 - _.seconds) {
            return UnixTime { seconds: max_u64 };
        }
        return UnixTime { seconds: _.seconds + delta };
    }

    pub fn subSeconds(delta: u64): UnixTime {
        if (delta >= _.seconds) {
            return UnixTime { seconds: 0 };
        }
        return UnixTime { seconds: _.seconds - delta };
    }

    pub fn isBefore(other: UnixTime): bool {
        return _.seconds < other.seconds;
    }

    pub fn isAfter(other: UnixTime): bool {
        return _.seconds > other.seconds;
    }

    pub fn equals(other: UnixTime): bool {
        return _.seconds == other.seconds;
    }

    pub fn isWithin(not_before: UnixTime, not_after: UnixTime): bool {
        return _.seconds >= not_before.seconds && _.seconds <= not_after.seconds;
    }
}

// --- PEM Object Classification & DER Decoding (via @cap/pem-rfc7468) ---

pub struct PkiPemResult {
    pub kind: u8,
    pub der_len: usize,
    pub err: i64,

    pub fn isOk(): bool {
        return _.err == PKI_OK && _.kind != PEM_UNKNOWN;
    }

    pub fn isCertificate(): bool {
        return _.kind == PEM_CERTIFICATE;
    }

    pub fn isPrivateKey(): bool {
        return _.kind == PEM_PKCS8_KEY
            || _.kind == PEM_PKCS1_KEY
            || _.kind == PEM_SEC1_KEY;
    }
}

/// Detects the PKI PEM object kind (`PEM_CERTIFICATE`, `PEM_PKCS8_KEY`, `PEM_PKCS1_KEY`,
/// `PEM_SEC1_KEY`, `PEM_PUBLIC_KEY`, `PEM_CRL`, `PEM_CSR`, or `PEM_UNKNOWN`)
/// by delegating header label extraction to `@cap/pem-rfc7468::decodeLabel`.
pub fn detectPemKind(pem: string): u8 {
    let label_buf: [64]u8 = undefined;
    const n = decodeLabel(pem, label_buf[0..64]);
    if (n <= 0) {
        return PEM_UNKNOWN;
    }
    return pkiClassifyPemLabel(label_buf[0..(n as usize)]);
}

/// Decodes a PEM-encoded PKI object (Certificate, PKCS#8/PKCS#1/SEC1 private key,
/// SPKI public key, CRL, or CSR) into `der_out` by delegating to `@cap/pem-rfc7468::decodePem`.
pub fn decodePemDer(pem: string, der_out: []u8): PkiPemResult {
    let label_buf: [64]u8 = undefined;
    let label_len: usize = 0;
    const decoded_len = decodePem(pem, label_buf[0..64], &label_len, der_out);
    if (decoded_len < 0) {
        return PkiPemResult {
            kind: PEM_UNKNOWN,
            der_len: 0,
            err: decoded_len,
        };
    }
    const kind = pkiClassifyPemLabel(label_buf[0..label_len]);
    if (kind == PEM_UNKNOWN) {
        return PkiPemResult {
            kind: PEM_UNKNOWN,
            der_len: decoded_len as usize,
            err: PKI_ERR_UNKNOWN_PEM_KIND,
        };
    }
    return PkiPemResult {
        kind: kind,
        der_len: decoded_len as usize,
        err: PKI_OK,
    };
}

/// Returns the canonical RFC 7468 PEM label string for a `PEM_*` kind constant.
pub fn pemKindLabel(kind: u8): string {
    if (kind == PEM_CERTIFICATE) {
        return "CERTIFICATE";
    }
    if (kind == PEM_PKCS8_KEY) {
        return "PRIVATE KEY";
    }
    if (kind == PEM_PKCS1_KEY) {
        return "RSA PRIVATE KEY";
    }
    if (kind == PEM_SEC1_KEY) {
        return "EC PRIVATE KEY";
    }
    if (kind == PEM_PUBLIC_KEY) {
        return "PUBLIC KEY";
    }
    if (kind == PEM_CRL) {
        return "X509 CRL";
    }
    if (kind == PEM_CSR) {
        return "CERTIFICATE REQUEST";
    }
    return "";
}

pub fn isPrivateKeyKind(kind: u8): bool {
    return kind == PEM_PKCS8_KEY || kind == PEM_PKCS1_KEY || kind == PEM_SEC1_KEY;
}
