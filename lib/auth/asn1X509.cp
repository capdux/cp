// Source: cp/lib/asn1/x509.cp (Zero-C-Dep X.509 v3 Certificate Inspector)
// Pure CP Arena-backed RFC 5280 X.509 v3 DER certificate metadata, SAN,
// UTCTime/GeneralizedTime validity window, BasicConstraints CA, and SHA-256 fingerprint inspector.

const std = @import("std");
import {
    Asn1Error,
    DerElement,
    DerReader,
    OidCodec,
    TAG_BOOLEAN,
    TAG_UTC_TIME,
    TAG_GENERALIZED_TIME,
} from "./asn1Der.cp";

const HEX_LOWER: []const u8 = "0123456789abcdef";

/// Parsed X.509 v1/v2/v3 certificate metadata backed by an owned `ArenaAllocator`.
/// Call `info.deinit()` to release all decoded strings and SAN slices in O(1).
pub struct X509CertInfo {
    version: u8 = 1,
    serial_hex: []const u8 = "",
    sig_alg_oid: []const u8 = "",
    issuer_cn: []const u8 = "",
    issuer_org: []const u8 = "",
    subject_cn: []const u8 = "",
    subject_org: []const u8 = "",
    not_before_sec: i64 = 0,
    not_after_sec: i64 = 0,
    pub_key_alg_oid: []const u8 = "",
    san_dns: []string = [],
    is_ca: bool = false,
    sha256_fingerprint_hex: [64]u8 = [0; 64],
    owned_arena: (*std.heap.ArenaAllocator)? = null,
    parent_alloc: std.mem.Allocator? = null,

    /// Returns `true` if `now_sec` falls inclusively within `[not_before_sec, not_after_sec]`.
    pub fn isValidAt(now_sec: i64): bool {
        return now_sec >= _.not_before_sec && now_sec <= _.not_after_sec;
    }

    /// Returns a slice view of the 64-character lowercase SHA-256 certificate fingerprint hex.
    pub fn fingerprintSlice(*_): []const u8 {
        return _.sha256_fingerprint_hex[0..64];
    }

    /// Returns a value copy of the 64-character lowercase SHA-256 certificate fingerprint hex.
    pub fn fingerprintHex(): [64]u8 {
        return _.sha256_fingerprint_hex;
    }

    /// Releases all Arena-allocated slices owned by this certificate info struct.
    pub fn deinit(*_) {
        if (owned := _.owned_arena) {
            owned.deinit();
            if (p := _.parent_alloc) {
                p.destroy(owned);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
        _.serial_hex = "";
        _.sig_alg_oid = "";
        _.issuer_cn = "";
        _.issuer_org = "";
        _.subject_cn = "";
        _.subject_org = "";
        _.pub_key_alg_oid = "";
        _.san_dns = [];
    }
}

/// Formats `bytes` into lowercase hexadecimal using `alloc`.
fn encodeHexLower(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! Asn1Error {
    let out = alloc.alloc(u8, bytes.len * 2) catch return Asn1Error.OutOfMemory;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        const b = bytes[i];
        out[i * 2] = HEX_LOWER[(b >> 4) & 0x0F];
        out[i * 2 + 1] = HEX_LOWER[b & 0x0F];
    }
    return out;
}

/// Parses `n` ASCII digits starting at `s[start..start+n]` as `i64`.
fn parseDigits(s: []const u8, start: usize, n: usize): i64 ! Asn1Error {
    if (start + n > s.len) {
        return Asn1Error.InvalidTime;
    }
    let acc: i64 = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const c = s[start + i];
        if (c < '0' || c > '9') {
            return Asn1Error.InvalidTime;
        }
        acc = acc * 10 + ((c - '0') as i64);
    }
    return acc;
}

/// Returns `true` if `year` is a Gregorian leap year.
fn isLeapYear(year: i64): bool {
    if ((@rem(year, 4) != 0)) {
        return false;
    }
    if ((@rem(year, 100) != 0)) {
        return true;
    }
    return @rem(year, 400) == 0;
}

/// Returns the number of days in `(year, month)` for `month` in `1..=12`.
fn daysInMonth(year: i64, month: i64): i64 {
    return switch (month) {
        1 | 3 | 5 | 7 | 8 | 10 | 12 => 31,
        4 | 6 | 9 | 11 => 30,
        2 => if (isLeapYear(year)) 29 else 28,
        else => 0,
    };
}

/// Converts a validated Gregorian UTC civil date-time to Unix epoch seconds.
/// Uses Howard Hinnant's civil-to-days algorithm with exact leap-year rules.
fn civilToUnixSec(year: i64, month: i64, day: i64, hour: i64, minute: i64, second: i64): i64 {
    let y: i64 = year;
    if (month <= 2) {
        y -= 1;
    }
    const era: i64 = if (y >= 0) @divTrunc(y, 400) else @divTrunc(y - 399, 400);
    const yoe: i64 = y - era * 400;
    const m_adj: i64 = if (month > 2) (month - 3) else (month + 9);
    const doy: i64 = @divTrunc(153 * m_adj + 2, 5) + day - 1;
    const doe: i64 = yoe * 365 + @divTrunc(yoe, 4) - @divTrunc(yoe, 100) + doy;
    const days: i64 = era * 146097 + doe - 719468;
    return days * 86400 + hour * 3600 + minute * 60 + second;
}

/// Extracted CommonName (CN) and Organization (O) from an X.500 Name SEQUENCE.
struct ParsedName {
    cn: []const u8 = "",
    org: []const u8 = "",
}

/// Parses an X.500 `Name ::= SEQUENCE OF RelativeDistinguishedName` extracting `CN` and `O`.
fn parseX500Name(alloc: std.mem.Allocator, reader: *DerReader): ParsedName ! Asn1Error {
    let name_seq = reader.readSequence()?;
    let res = ParsedName{};
    while (name_seq.hasNext()) {
        let rdn_set = name_seq.readSet()?;
        while (rdn_set.hasNext()) {
            let atv_seq = rdn_set.readSequence()?;
            const oid = atv_seq.readOid(alloc)?;
            const val_str = atv_seq.readAnyStringAlloc(alloc)?;
            if (atv_seq.hasNext()) {
                return Asn1Error.MalformedCert;
            }
            if (std.mem.eql(u8, oid, "2.5.4.3") && res.cn.len == 0) {
                res.cn = val_str;
            } else if (std.mem.eql(u8, oid, "2.5.4.10") && res.org.len == 0) {
                res.org = val_str;
            }
        }
    }
    return res;
}

/// Parses an RFC 5280 `AlgorithmIdentifier ::= SEQUENCE { algorithm OID, parameters ANY OPTIONAL }`.
fn parseAlgorithmOid(alloc: std.mem.Allocator, reader: *DerReader): ([]u8) ! Asn1Error {
    let alg_seq = reader.readSequence()?;
    const oid = alg_seq.readOid(alloc)?;
    while (alg_seq.hasNext()) {
        alg_seq.skip()?;
    }
    return oid;
}

/// Parses X.509 v3 `Extensions ::= SEQUENCE OF Extension`, populating `san_dns` and `is_ca`.
fn parseExtensions(
    alloc: std.mem.Allocator,
    ext_wrapper: *DerReader,
    san_list: *std.ArrayList(string),
    is_ca: *bool,
): void ! Asn1Error {
    let ext_seq = ext_wrapper.readSequence()?;
    if (ext_wrapper.hasNext()) {
        return Asn1Error.MalformedCert;
    }

    while (ext_seq.hasNext()) {
        let item_seq = ext_seq.readSequence()?;
        const ext_oid = item_seq.readOid(alloc)?;

        if (next_t := item_seq.peekTag()) {
            if (next_t == TAG_BOOLEAN) {
                * = item_seq.readBoolean()?;
            }
        }

        const ext_val = item_seq.readOctetString()?;
        if (item_seq.hasNext()) {
            return Asn1Error.MalformedCert;
        }

        if (std.mem.eql(u8, ext_oid, "2.5.29.17")) {
            // subjectAltName: GeneralNames ::= SEQUENCE OF GeneralName
            let gn_wrapper = DerReader.initWithDepth(ext_val, 0)?;
            let gn_seq = gn_wrapper.readSequence()?;
            if (gn_wrapper.hasNext()) {
                return Asn1Error.MalformedCert;
            }
            while (gn_seq.hasNext()) {
                const gn = gn_seq.readElement()?;
                // [2] IMPLICIT dNSName IA5String -> tag 0x82
                if (gn.tag == 0x82) {
                    const duped = alloc.dupe(u8, gn.content) catch return Asn1Error.OutOfMemory;
                    san_list.append(alloc, duped) catch return Asn1Error.OutOfMemory;
                }
            }
        } else if (std.mem.eql(u8, ext_oid, "2.5.29.19")) {
            // basicConstraints: SEQUENCE { cA BOOLEAN DEFAULT FALSE, pathLenConstraint INTEGER OPTIONAL }
            let bc_wrapper = DerReader.initWithDepth(ext_val, 0)?;
            let bc_seq = bc_wrapper.readSequence()?;
            if (bc_wrapper.hasNext()) {
                return Asn1Error.MalformedCert;
            }
            if (bc_tag := bc_seq.peekTag()) {
                if (bc_tag == TAG_BOOLEAN) {
                    is_ca.* = bc_seq.readBoolean()?;
                }
            }
        }
    }
}

/// Zero-C-dependency X.509 v3 DER certificate inspector and ASN.1 time parser.
pub static class X509Inspector {
    /// Parses an ASN.1 DER `UTCTime` (`YYMMDDHHMMSSZ`) or `GeneralizedTime` (`YYYYMMDDHHMMSSZ`)
    /// into Unix epoch seconds. Rejects non-UTC formats, invalid dates, and out-of-range fields.
    pub fn parseTimeToUnixSec(tag: u8, bytes: []const u8): i64 ! Asn1Error {
        let year: i64 = 0;
        let offset: usize = 0;

        if (tag == TAG_UTC_TIME) {
            if (bytes.len != 13 || bytes[12] != 'Z') {
                return Asn1Error.InvalidTime;
            }
            const yy = parseDigits(bytes, 0, 2)?;
            // RFC 5280 Section 4.1.2.5.1: YY >= 50 -> 19xx; YY < 50 -> 20xx
            year = if (yy >= 50) (1900 + yy) else (2000 + yy);
            offset = 2;
        } else if (tag == TAG_GENERALIZED_TIME) {
            if (bytes.len != 15 || bytes[14] != 'Z') {
                return Asn1Error.InvalidTime;
            }
            year = parseDigits(bytes, 0, 4)?;
            offset = 4;
        } else {
            return Asn1Error.TagMismatch;
        }

        const month = parseDigits(bytes, offset, 2)?;
        const day = parseDigits(bytes, offset + 2, 2)?;
        const hour = parseDigits(bytes, offset + 4, 2)?;
        const minute = parseDigits(bytes, offset + 6, 2)?;
        const second = parseDigits(bytes, offset + 8, 2)?;

        if (month < 1 || month > 12) {
            return Asn1Error.InvalidTime;
        }
        const max_day = daysInMonth(year, month);
        if (day < 1 || day > max_day) {
            return Asn1Error.InvalidTime;
        }
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
            return Asn1Error.InvalidTime;
        }

        return civilToUnixSec(year, month, day, hour, minute, second);
    }

    /// Parses a DER-encoded X.509 certificate (`Certificate` SEQUENCE) and computes its SHA-256 fingerprint.
    /// Caller owns the returned `X509CertInfo` and must call `info.deinit()`.
    pub fn parseDer(alloc: std.mem.Allocator, der_bytes: []const u8): X509CertInfo ! Asn1Error {
        if (der_bytes.len == 0) {
            return Asn1Error.UnexpectedEof;
        }

        let arena_ptr = alloc.create(std.heap.ArenaAllocator) catch return Asn1Error.OutOfMemory;
        unsafe {
            arena_ptr.* = std.heap.ArenaAllocator.init(alloc);
        }
        let info = X509CertInfo{
            owned_arena: arena_ptr,
            parent_alloc: alloc,
        };
        errdefer info.deinit();

        const arena_alloc = arena_ptr.allocator();

        // Compute SHA-256 fingerprint over the complete DER certificate bytes.
        let digest: [32]u8 = [0; 32];
        std.crypto.hash.sha2.Sha256.hash(der_bytes, &digest, .{});
        let di: usize = 0;
        while (di < 32) : (di += 1) {
            const b = digest[di];
            info.sha256_fingerprint_hex[di * 2] = HEX_LOWER[(b >> 4) & 0x0F];
            info.sha256_fingerprint_hex[di * 2 + 1] = HEX_LOWER[b & 0x0F];
        }

        let root_reader = DerReader.init(der_bytes);
        let cert_seq = root_reader.readSequence()?;
        if (root_reader.hasNext()) {
            return Asn1Error.MalformedCert;
        }

        // 1. TBSCertificate SEQUENCE
        let tbs = cert_seq.readSequence()?;

        // 1.1 Optional [0] EXPLICIT version (DEFAULT v1 = 0)
        if (tbs.hasContextExplicit(0)) {
            let ver_reader = tbs.readContextExplicit(0)?;
            const ver_val = ver_reader.readUint64()?;
            if (ver_val > 2 || ver_reader.hasNext()) {
                return Asn1Error.MalformedCert;
            }
            info.version = (ver_val as u8) + 1;
        } else {
            info.version = 1;
        }

        // 1.2 serialNumber INTEGER
        const raw_serial = tbs.readIntegerBytes()?;
        let serial_mag = raw_serial;
        if (serial_mag.len > 1 && serial_mag[0] == 0x00) {
            serial_mag = serial_mag[1..];
        }
        info.serial_hex = encodeHexLower(arena_alloc, serial_mag)?;

        // 1.3 signature AlgorithmIdentifier
        info.sig_alg_oid = parseAlgorithmOid(arena_alloc, &tbs)?;

        // 1.4 issuer Name
        const issuer_name = parseX500Name(arena_alloc, &tbs)?;
        info.issuer_cn = issuer_name.cn;
        info.issuer_org = issuer_name.org;

        // 1.5 validity Validity SEQUENCE { notBefore, notAfter }
        let validity_seq = tbs.readSequence()?;
        const nb_elem = validity_seq.readElement()?;
        info.not_before_sec = X509Inspector.parseTimeToUnixSec(nb_elem.tag, nb_elem.content)?;
        const na_elem = validity_seq.readElement()?;
        info.not_after_sec = X509Inspector.parseTimeToUnixSec(na_elem.tag, na_elem.content)?;
        if (validity_seq.hasNext()) {
            return Asn1Error.MalformedCert;
        }

        // 1.6 subject Name
        const subject_name = parseX500Name(arena_alloc, &tbs)?;
        info.subject_cn = subject_name.cn;
        info.subject_org = subject_name.org;

        // 1.7 subjectPublicKeyInfo SEQUENCE { algorithm, subjectPublicKey BIT STRING }
        let spki_seq = tbs.readSequence()?;
        info.pub_key_alg_oid = parseAlgorithmOid(arena_alloc, &spki_seq)?;
        * = spki_seq.readBitString()?;
        if (spki_seq.hasNext()) {
            return Asn1Error.MalformedCert;
        }

        // 1.8 Optional [1] IMPLICIT issuerUniqueID (0x81) & [2] IMPLICIT subjectUniqueID (0x82)
        if (tag1 := tbs.peekTag()) {
            if (tag1 == 0x81) {
                tbs.skip()?;
            }
        }
        if (tag2 := tbs.peekTag()) {
            if (tag2 == 0x82) {
                tbs.skip()?;
            }
        }

        // 1.9 Optional [3] EXPLICIT extensions (0xA3)
        let san_list: std.ArrayList(string) = .empty;
        let is_ca_flag = false;
        if (tbs.hasContextExplicit(3)) {
            let ext_reader = tbs.readContextExplicit(3)?;
            parseExtensions(arena_alloc, &ext_reader, &san_list, &is_ca_flag)?;
        }
        info.san_dns = san_list.toOwnedSlice(arena_alloc) catch return Asn1Error.OutOfMemory;
        info.is_ca = is_ca_flag;

        if (tbs.hasNext()) {
            return Asn1Error.MalformedCert;
        }

        // 2. signatureAlgorithm AlgorithmIdentifier
        * = parseAlgorithmOid(arena_alloc, &cert_seq)?;

        // 3. signatureValue BIT STRING
        * = cert_seq.readBitString()?;

        if (cert_seq.hasNext()) {
            return Asn1Error.MalformedCert;
        }

        return info;
    }
}
