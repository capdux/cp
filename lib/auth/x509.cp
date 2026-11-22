// Source: cp/lib/x509/x509.cp (Pure CP X.509 v3 Certificate DER/PEM Parser, SAN & Validity Engine)
// Zero-C-dependency RFC 5280 X.509 certificate parser, RFC 6125 hostname/SAN matcher,
// UTCTime/GeneralizedTime validator, and issuer chain verifier.

const std = @import("std");
import {
    Asn1Error,
    DerElement,
    DerReader,
    TAG_BOOLEAN,
    TAG_INTEGER,
    TAG_OCTET_STRING,
    TAG_SEQUENCE,
    TAG_UTC_TIME,
    TAG_GENERALIZED_TIME,
} from "./asn1Der.cp";
import {
    PemError,
    PemBlock,
    parse as parsePemBlocks,
    freeBlocks,
} from "./pem.cp";

const HEX_LOWER: []const u8 = "0123456789abcdef";

/// Errors returned by `cap:x509` DER/PEM parsing, timestamp decoding, and certificate validation.
pub err X509Error {
    InvalidDer,
    InvalidPem,
    InvalidCertificate,
    InvalidTimestamp,
    UnsupportedVersion,
    OutOfMemory,
}

/// Decoded ASN.1 X.509 validity timestamp (`UTCTime` or `GeneralizedTime`) in Unix epoch seconds.
pub struct X509Time {
    unix_sec: i64 = 0,
    is_generalized: bool = false,

    /// Parses an ASN.1 `UTCTime` (`0x17`) or `GeneralizedTime` (`0x18`) element into `X509Time`.
    pub static fn parse(tag: u8, bytes: []const u8): X509Time ! X509Error {
        const sec = parseAsn1Time(tag, bytes)?;
        return X509Time{
            unix_sec: sec,
            is_generalized: tag == TAG_GENERALIZED_TIME,
        };
    }

    /// Returns the Unix epoch seconds of this timestamp.
    pub fn toUnixSec(): i64 {
        return _.unix_sec;
    }
}

/// Subject Alternative Name (SAN) entry extracted from X.509 extension `2.5.29.17`.
pub enum SanEntry {
    DnsName: []const u8,
    IpAddress: []const u8,
}

/// X.509 v3 BasicConstraints extension (`2.5.29.19`).
pub struct BasicConstraints {
    is_ca: bool = false,
    path_len: u32? = null,
}

/// Parsed X.509 v1/v2/v3 certificate with owned Arena-backed slices.
/// Call `cert.deinit(allocator)` to release all allocated memory.
pub struct X509Certificate {
    version: u8 = 1,
    serial_number: []const u8 = "",
    signature_oid: []const u8 = "",
    issuer_cn: ([]const u8)? = null,
    subject_cn: ([]const u8)? = null,
    issuer_raw: []const u8 = "",
    subject_raw: []const u8 = "",
    not_before: i64 = 0,
    not_after: i64 = 0,
    spki_raw: []const u8 = "",
    sans: []SanEntry = [],
    basic_constraints: BasicConstraints? = null,
    sha256_digest: [32]u8 = [0; 32],
    owned_arena: (*std.heap.ArenaAllocator)? = null,

    /// Releases all Arena-allocated slices and metadata owned by this certificate.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (owned := _.owned_arena) {
            owned.deinit();
            allocator.destroy(owned);
            _.owned_arena = null;
        }
        _.serial_number = "";
        _.signature_oid = "";
        _.issuer_cn = null;
        _.subject_cn = null;
        _.issuer_raw = "";
        _.subject_raw = "";
        _.spki_raw = "";
        _.sans = [];
        _.basic_constraints = null;
    }

    /// Parses a DER-encoded X.509 certificate (`Certificate` SEQUENCE) and computes its SHA-256 digest.
    pub static fn parseDer(allocator: std.mem.Allocator, der_bytes: []const u8): X509Certificate ! X509Error {
        if (der_bytes.len == 0) {
            return X509Error.InvalidDer;
        }

        let arena_ptr = allocator.create(std.heap.ArenaAllocator) catch return X509Error.OutOfMemory;
        unsafe {
            arena_ptr.* = std.heap.ArenaAllocator.init(allocator);
        }
        let cert = X509Certificate{
            owned_arena: arena_ptr,
        };
        errdefer cert.deinit(allocator);

        const arena_alloc = arena_ptr.allocator();

        // Compute SHA-256 fingerprint over the complete DER certificate bytes.
        std.crypto.hash.sha2.Sha256.hash(der_bytes, &cert.sha256_digest, .{});

        let root_reader = DerReader.init(der_bytes);
        let cert_seq = root_reader.readSequence() catch e => return mapAsn1Error(e);
        if (root_reader.hasNext()) {
            return X509Error.InvalidDer;
        }

        // 1. TBSCertificate SEQUENCE
        let tbs = cert_seq.readSequence() catch e => return mapAsn1Error(e);

        // 1.1 Optional [0] EXPLICIT version (DEFAULT v1 = 0)
        if (tbs.hasContextExplicit(0)) {
            let ver_reader = tbs.readContextExplicit(0) catch e => return mapAsn1Error(e);
            const ver_val = ver_reader.readUint64() catch e => return mapAsn1Error(e);
            if (ver_reader.hasNext()) {
                return X509Error.InvalidCertificate;
            }
            if (ver_val > 2) {
                return X509Error.UnsupportedVersion;
            }
            cert.version = (ver_val as u8) + 1;
        } else {
            cert.version = 1;
        }

        // 1.2 serialNumber INTEGER
        const raw_serial = tbs.readIntegerBytes() catch e => return mapAsn1Error(e);
        let serial_mag = raw_serial;
        if (serial_mag.len > 1 && serial_mag[0] == 0x00) {
            serial_mag = serial_mag[1..];
        }
        cert.serial_number = arena_alloc.dupe(u8, serial_mag) catch return X509Error.OutOfMemory;

        // 1.3 signature AlgorithmIdentifier
        cert.signature_oid = parseAlgorithmOid(arena_alloc, &tbs)?;

        // 1.4 issuer Name (preserve raw DER TLV for exact issuer/subject comparison)
        const issuer_elem = tbs.expectTag(TAG_SEQUENCE) catch e => return mapAsn1Error(e);
        cert.issuer_raw = arena_alloc.dupe(u8, issuer_elem.raw) catch return X509Error.OutOfMemory;
        let issuer_reader = issuer_elem.reader();
        cert.issuer_cn = parseNameCommonName(arena_alloc, &issuer_reader)?;

        // 1.5 validity Validity SEQUENCE { notBefore, notAfter }
        let validity_seq = tbs.readSequence() catch e => return mapAsn1Error(e);
        const nb_elem = validity_seq.readElement() catch e => return mapAsn1Error(e);
        cert.not_before = parseAsn1Time(nb_elem.tag, nb_elem.content)?;
        const na_elem = validity_seq.readElement() catch e => return mapAsn1Error(e);
        cert.not_after = parseAsn1Time(na_elem.tag, na_elem.content)?;
        if (validity_seq.hasNext()) {
            return X509Error.InvalidCertificate;
        }

        // 1.6 subject Name (preserve raw DER TLV for exact issuer/subject comparison)
        const subject_elem = tbs.expectTag(TAG_SEQUENCE) catch e => return mapAsn1Error(e);
        cert.subject_raw = arena_alloc.dupe(u8, subject_elem.raw) catch return X509Error.OutOfMemory;
        let subject_reader = subject_elem.reader();
        cert.subject_cn = parseNameCommonName(arena_alloc, &subject_reader)?;

        // 1.7 subjectPublicKeyInfo SEQUENCE
        const spki_elem = tbs.expectTag(TAG_SEQUENCE) catch e => return mapAsn1Error(e);
        let spki_reader = spki_elem.reader();
        * = parseAlgorithmOid(arena_alloc, &spki_reader)?;
        * = spki_reader.readBitString() catch e => return mapAsn1Error(e);
        if (spki_reader.hasNext()) {
            return X509Error.InvalidCertificate;
        }
        cert.spki_raw = arena_alloc.dupe(u8, spki_elem.raw) catch return X509Error.OutOfMemory;

        // 1.8 Optional [1] IMPLICIT issuerUniqueID (0x81) & [2] IMPLICIT subjectUniqueID (0x82)
        if (tag1 := tbs.peekTag()) {
            if (tag1 == 0x81) {
                tbs.skip() catch e => return mapAsn1Error(e);
            }
        }
        if (tag2 := tbs.peekTag()) {
            if (tag2 == 0x82) {
                tbs.skip() catch e => return mapAsn1Error(e);
            }
        }

        // 1.9 Optional [3] EXPLICIT extensions (0xA3)
        let san_list: std.ArrayList(SanEntry) = .empty;
        if (tbs.hasContextExplicit(3)) {
            let ext_reader = tbs.readContextExplicit(3) catch e => return mapAsn1Error(e);
            cert.basic_constraints = parseExtensions(arena_alloc, &ext_reader, &san_list)?;
        }
        cert.sans = san_list.toOwnedSlice(arena_alloc) catch return X509Error.OutOfMemory;

        if (tbs.hasNext()) {
            return X509Error.InvalidCertificate;
        }

        // 2. signatureAlgorithm AlgorithmIdentifier
        * = parseAlgorithmOid(arena_alloc, &cert_seq)?;

        // 3. signatureValue BIT STRING
        * = cert_seq.readBitString() catch e => return mapAsn1Error(e);

        if (cert_seq.hasNext()) {
            return X509Error.InvalidCertificate;
        }

        return cert;
    }

    /// Extracts the first `"CERTIFICATE"` PEM block using `cap:pem` and parses its DER payload.
    pub static fn parsePem(allocator: std.mem.Allocator, pem_text: []const u8): X509Certificate ! X509Error {
        const blocks = parsePemBlocks(allocator, pem_text) catch e => {
            if (e == PemError.OutOfMemory) {
                return X509Error.OutOfMemory;
            }
            return X509Error.InvalidPem;
        };
        defer freeBlocks(allocator, blocks);

        for (blk of blocks) {
            if (std.mem.eql(u8, blk.label, "CERTIFICATE")) {
                return X509Certificate.parseDer(allocator, blk.bytes);
            }
        }
        return X509Error.InvalidPem;
    }

    /// Returns `true` if `unix_sec` falls inclusively within `[not_before, not_after]`.
    pub fn isValidAt(unix_sec: i64): bool {
        return unix_sec >= _.not_before && unix_sec <= _.not_after;
    }

    /// Matches `hostname` (DNS name or IP literal) against SAN entries (or `subject_cn` if no SANs)
    /// following RFC 6125 single-label wildcard rules and case-insensitive comparison.
    pub fn matchesHostname(hostname: []const u8): bool {
        if (hostname.len == 0) {
            return false;
        }

        if (target_ip := parseIpLiteral(hostname)) {
            for (san of _.sans) {
                switch (san) {
                    .IpAddress ip_str => {
                        if (san_ip := parseIpLiteral(ip_str)) {
                            if (ipAddrEqual(target_ip, san_ip)) {
                                return true;
                            }
                        }
                    },
                    .DnsName => {},
                }
            }
            return false;
        }

        for (san of _.sans) {
            switch (san) {
                .DnsName dns_pattern => {
                    if (matchesDnsPattern(dns_pattern, hostname)) {
                        return true;
                    }
                },
                .IpAddress => {},
            }
        }

        // RFC 6125 Section 6.4.4: fall back to CN only when no SAN entries are present.
        if (_.sans.len == 0) {
            if (cn := _.subject_cn) {
                return matchesDnsPattern(cn, hostname);
            }
        }
        return false;
    }

    /// Checks whether this certificate's `issuer_raw` matches `issuer_cert.subject_raw`
    /// and `issuer_cert` satisfies CA BasicConstraints.
    pub fn isIssuedBy(issuer_cert: *const X509Certificate): bool {
        if (_.issuer_raw.len == 0 || issuer_cert.subject_raw.len == 0) {
            return false;
        }
        if (!std.mem.eql(u8, _.issuer_raw, issuer_cert.subject_raw)) {
            return false;
        }
        if (issuer_cert.spki_raw.len == 0) {
            return false;
        }
        if (bc := issuer_cert.basic_constraints) {
            return bc.is_ca;
        }
        // RFC 5280: v3 CA certificates require BasicConstraints with cA=TRUE.
        return issuer_cert.version < 3;
    }

    /// Returns the 64-character lowercase SHA-256 certificate fingerprint hex string allocated via `allocator`.
    pub fn fingerprintSha256(allocator: std.mem.Allocator): ([]u8) ! X509Error {
        let out = allocator.alloc(u8, 64) catch return X509Error.OutOfMemory;
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            const b = _.sha256_digest[i];
            out[i * 2] = HEX_LOWER[(b >> 4) & 0x0F];
            out[i * 2 + 1] = HEX_LOWER[b & 0x0F];
        }
        return out;
    }

    /// Returns the raw 32-byte SHA-256 digest of the DER-encoded certificate.
    pub fn fingerprintSha256Raw(): [32]u8 {
        return _.sha256_digest;
    }
}

/// Parses an ASN.1 `UTCTime` (`0x17`, `YYMMDDHHMMSSZ`) or `GeneralizedTime` (`0x18`, `YYYYMMDDHHMMSSZ`)
/// into Unix epoch seconds.
pub fn parseAsn1Time(tag: u8, bytes: []const u8): i64 ! X509Error {
    let year: i64 = 0;
    let offset: usize = 0;

    if (tag == TAG_UTC_TIME) {
        if (bytes.len != 13 || bytes[12] != 'Z') {
            return X509Error.InvalidTimestamp;
        }
        const yy = parseDigits(bytes, 0, 2)?;
        // RFC 5280 Section 4.1.2.5.1: YY >= 50 -> 19xx; YY < 50 -> 20xx
        year = if (yy >= 50) (1900 + yy) else (2000 + yy);
        offset = 2;
    } else if (tag == TAG_GENERALIZED_TIME) {
        if (bytes.len != 15 || bytes[14] != 'Z') {
            return X509Error.InvalidTimestamp;
        }
        year = parseDigits(bytes, 0, 4)?;
        offset = 4;
    } else {
        return X509Error.InvalidTimestamp;
    }

    const month = parseDigits(bytes, offset, 2)?;
    const day = parseDigits(bytes, offset + 2, 2)?;
    const hour = parseDigits(bytes, offset + 4, 2)?;
    const minute = parseDigits(bytes, offset + 6, 2)?;
    const second = parseDigits(bytes, offset + 8, 2)?;

    if (month < 1 || month > 12) {
        return X509Error.InvalidTimestamp;
    }
    const max_day = daysInMonth(year, month);
    if (day < 1 || day > max_day) {
        return X509Error.InvalidTimestamp;
    }
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return X509Error.InvalidTimestamp;
    }

    return civilToUnixSec(year, month, day, hour, minute, second);
}

fn mapAsn1Error(e: Asn1Error): X509Error {
    return switch (e) {
        .OutOfMemory => X509Error.OutOfMemory,
        .InvalidTime => X509Error.InvalidTimestamp,
        .MalformedCert => X509Error.InvalidCertificate,
        else => X509Error.InvalidDer,
    };
}

fn parseDigits(s: []const u8, start: usize, n: usize): i64 ! X509Error {
    if (start + n > s.len) {
        return X509Error.InvalidTimestamp;
    }
    let acc: i64 = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const c = s[start + i];
        if (c < '0' || c > '9') {
            return X509Error.InvalidTimestamp;
        }
        acc = acc * 10 + ((c - '0') as i64);
    }
    return acc;
}

fn isLeapYear(year: i64): bool {
    if (@rem(year, 4) != 0) {
        return false;
    }
    if (@rem(year, 100) != 0) {
        return true;
    }
    return @rem(year, 400) == 0;
}

fn daysInMonth(year: i64, month: i64): i64 {
    return switch (month) {
        1 | 3 | 5 | 7 | 8 | 10 | 12 => 31,
        4 | 6 | 9 | 11 => 30,
        2 => if (isLeapYear(year)) 29 else 28,
        else => 0,
    };
}

/// Howard Hinnant's civil-to-days algorithm converting UTC date-time to Unix epoch seconds.
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

fn parseAlgorithmOid(arena_alloc: std.mem.Allocator, reader: *DerReader): ([]const u8) ! X509Error {
    let alg_seq = reader.readSequence() catch e => return mapAsn1Error(e);
    const oid = alg_seq.readOid(arena_alloc) catch e => return mapAsn1Error(e);
    while (alg_seq.hasNext()) {
        alg_seq.skip() catch e => return mapAsn1Error(e);
    }
    return oid;
}

fn parseNameCommonName(arena_alloc: std.mem.Allocator, name_seq: *DerReader): (([]const u8)?) ! X509Error {
    let cn: ([]const u8)? = null;
    while (name_seq.hasNext()) {
        let rdn_set = name_seq.readSet() catch e => return mapAsn1Error(e);
        while (rdn_set.hasNext()) {
            let atv_seq = rdn_set.readSequence() catch e => return mapAsn1Error(e);
            const oid = atv_seq.readOid(arena_alloc) catch e => return mapAsn1Error(e);
            const val_str = atv_seq.readAnyStringAlloc(arena_alloc) catch e => return mapAsn1Error(e);
            if (atv_seq.hasNext()) {
                return X509Error.InvalidCertificate;
            }
            if (std.mem.eql(u8, oid, "2.5.4.3") && cn == null) {
                cn = val_str;
            }
        }
    }
    return cn;
}

fn formatIpv4(arena_alloc: std.mem.Allocator, bytes: []const u8): ([]const u8) ! X509Error {
    let buf: [16]u8 = [0; 16];
    let pos: usize = 0;
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        if (i > 0) {
            buf[pos] = '.';
            pos += 1;
        }
        let v: u8 = bytes[i];
        if (v >= 100) {
            buf[pos] = ('0' as u8) + (v / 100);
            pos += 1;
            v = v % 100;
            buf[pos] = ('0' as u8) + (v / 10);
            pos += 1;
            buf[pos] = ('0' as u8) + (v % 10);
            pos += 1;
        } else if (v >= 10) {
            buf[pos] = ('0' as u8) + (v / 10);
            pos += 1;
            buf[pos] = ('0' as u8) + (v % 10);
            pos += 1;
        } else {
            buf[pos] = ('0' as u8) + v;
            pos += 1;
        }
    }
    return arena_alloc.dupe(u8, buf[0..pos]) catch return X509Error.OutOfMemory;
}

fn formatIpv6(arena_alloc: std.mem.Allocator, bytes: []const u8): ([]const u8) ! X509Error {
    let buf: [40]u8 = [0; 40];
    let pos: usize = 0;
    let g: usize = 0;
    while (g < 8) : (g += 1) {
        if (g > 0) {
            buf[pos] = ':';
            pos += 1;
        }
        const hi = bytes[g * 2];
        const lo = bytes[g * 2 + 1];
        const val: u16 = ((hi as u16) << 8) | (lo as u16);
        let started = false;
        let shift: u4 = 12;
        while (true) {
            const nib: u8 = ((val >> shift) & 0x0F) as u8;
            if (nib != 0 || started || shift == 0) {
                buf[pos] = HEX_LOWER[nib];
                pos += 1;
                started = true;
            }
            if (shift == 0) {
                break;
            }
            shift -= 4;
        }
    }
    return arena_alloc.dupe(u8, buf[0..pos]) catch return X509Error.OutOfMemory;
}

fn parseExtensions(
    arena_alloc: std.mem.Allocator,
    ext_wrapper: *DerReader,
    san_list: *std.ArrayList(SanEntry),
): (BasicConstraints?) ! X509Error {
    let ext_seq = ext_wrapper.readSequence() catch e => return mapAsn1Error(e);
    if (ext_wrapper.hasNext()) {
        return X509Error.InvalidCertificate;
    }

    let bc_result: BasicConstraints? = null;

    while (ext_seq.hasNext()) {
        let item_seq = ext_seq.readSequence() catch e => return mapAsn1Error(e);
        const ext_oid = item_seq.readOid(arena_alloc) catch e => return mapAsn1Error(e);

        if (next_t := item_seq.peekTag()) {
            if (next_t == TAG_BOOLEAN) {
                * = item_seq.readBoolean() catch e => return mapAsn1Error(e);
            }
        }

        const ext_val = item_seq.readOctetString() catch e => return mapAsn1Error(e);
        if (item_seq.hasNext()) {
            return X509Error.InvalidCertificate;
        }

        if (std.mem.eql(u8, ext_oid, "2.5.29.17")) {
            // subjectAltName: GeneralNames ::= SEQUENCE OF GeneralName
            let gn_wrapper = DerReader.initWithDepth(ext_val, 0) catch e => return mapAsn1Error(e);
            let gn_seq = gn_wrapper.readSequence() catch e => return mapAsn1Error(e);
            if (gn_wrapper.hasNext()) {
                return X509Error.InvalidCertificate;
            }
            while (gn_seq.hasNext()) {
                const gn = gn_seq.readElement() catch e => return mapAsn1Error(e);
                if (gn.tag == 0x82) {
                    // [2] IMPLICIT dNSName IA5String
                    const duped = arena_alloc.dupe(u8, gn.content) catch return X509Error.OutOfMemory;
                    san_list.append(arena_alloc, SanEntry{ DnsName: duped }) catch return X509Error.OutOfMemory;
                } else if (gn.tag == 0x87) {
                    // [7] IMPLICIT iPAddress OCTET STRING (4 bytes IPv4 or 16 bytes IPv6)
                    if (gn.content.len == 4) {
                        const ip_str = formatIpv4(arena_alloc, gn.content)?;
                        san_list.append(arena_alloc, SanEntry{ IpAddress: ip_str }) catch return X509Error.OutOfMemory;
                    } else if (gn.content.len == 16) {
                        const ip_str = formatIpv6(arena_alloc, gn.content)?;
                        san_list.append(arena_alloc, SanEntry{ IpAddress: ip_str }) catch return X509Error.OutOfMemory;
                    } else {
                        return X509Error.InvalidCertificate;
                    }
                }
            }
        } else if (std.mem.eql(u8, ext_oid, "2.5.29.19")) {
            // basicConstraints: SEQUENCE { cA BOOLEAN DEFAULT FALSE, pathLenConstraint INTEGER OPTIONAL }
            let bc_wrapper = DerReader.initWithDepth(ext_val, 0) catch e => return mapAsn1Error(e);
            let bc_seq = bc_wrapper.readSequence() catch e => return mapAsn1Error(e);
            if (bc_wrapper.hasNext()) {
                return X509Error.InvalidCertificate;
            }
            let bc = BasicConstraints{};
            if (bc_tag := bc_seq.peekTag()) {
                if (bc_tag == TAG_BOOLEAN) {
                    bc.is_ca = bc_seq.readBoolean() catch e => return mapAsn1Error(e);
                }
            }
            if (pl_tag := bc_seq.peekTag()) {
                if (pl_tag == TAG_INTEGER) {
                    const pl = bc_seq.readUint64() catch e => return mapAsn1Error(e);
                    if (pl > 0xFFFFFFFF) {
                        return X509Error.InvalidCertificate;
                    }
                    bc.path_len = pl as u32;
                }
            }
            if (bc_seq.hasNext()) {
                return X509Error.InvalidCertificate;
            }
            bc_result = bc;
        }
    }

    return bc_result;
}

/// RFC 6125 DNS matching:
/// - Exact match (case-insensitive)
/// - Single-label leftmost wildcard `*.domain.tld` matching `sub.domain.tld`,
///   rejecting multi-level `a.b.domain.tld` and bare `domain.tld`.
fn matchesDnsPattern(pattern: []const u8, hostname: []const u8): bool {
    if (pattern.len == 0 || hostname.len == 0) {
        return false;
    }

    if (std.mem.indexOfScalar(u8, pattern, '*') == null) {
        return std.ascii.eqlIgnoreCase(pattern, hostname);
    }

    // Only allow leftmost "*." wildcard with at least one character after "*."
    if (pattern.len < 3 || pattern[0] != '*' || pattern[1] != '.') {
        return false;
    }
    const suffix = pattern[1..]; // ".example.com"
    if (std.mem.indexOfScalar(u8, suffix, '*') != null) {
        return false;
    }

    if (hostname.len <= suffix.len) {
        return false;
    }

    const prefix_len = hostname.len - suffix.len;
    const host_prefix = hostname[0..prefix_len];
    const host_suffix = hostname[prefix_len..];

    if (host_prefix.len == 0) {
        return false;
    }
    if (std.mem.indexOfScalar(u8, host_prefix, '.') != null) {
        return false;
    }
    if (std.mem.indexOfScalar(u8, host_prefix, '*') != null) {
        return false;
    }

    return std.ascii.eqlIgnoreCase(host_suffix, suffix);
}

struct ParsedIp {
    is_v6: bool = false,
    bytes: [16]u8 = [0; 16],
}

fn ipAddrEqual(a: ParsedIp, b: ParsedIp): bool {
    if (a.is_v6 != b.is_v6) {
        return false;
    }
    const len: usize = if (a.is_v6) 16 else 4;
    return std.mem.eql(u8, a.bytes[0..len], b.bytes[0..len]);
}

fn parseIpLiteral(s: []const u8): ParsedIp? {
    if (v4 := parseIpv4Bytes(s)) {
        let out = ParsedIp{ is_v6: false };
        out.bytes[0] = v4[0];
        out.bytes[1] = v4[1];
        out.bytes[2] = v4[2];
        out.bytes[3] = v4[3];
        return out;
    }
    if (v6 := parseIpv6Bytes(s)) {
        return ParsedIp{ is_v6: true, bytes: v6 };
    }
    return null;
}

fn parseIpv4Bytes(s: []const u8): ([4]u8)? {
    if (s.len < 7 || s.len > 15) {
        return null;
    }
    let out: [4]u8 = [0; 4];
    let oct_idx: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= s.len) : (i += 1) {
        if (i == s.len || s[i] == '.') {
            if (oct_idx >= 4) {
                return null;
            }
            const part = s[start..i];
            if (part.len == 0 || part.len > 3) {
                return null;
            }
            if (part.len > 1 && part[0] == '0') {
                return null;
            }
            let val: u16 = 0;
            for (c of part) {
                if (c < '0' || c > '9') {
                    return null;
                }
                val = val * 10 + ((c - '0') as u16);
            }
            if (val > 255) {
                return null;
            }
            out[oct_idx] = val as u8;
            oct_idx += 1;
            start = i + 1;
        }
    }
    if (oct_idx != 4) {
        return null;
    }
    return out;
}

fn parseHexGroup(part: []const u8): u16? {
    if (part.len == 0 || part.len > 4) {
        return null;
    }
    let val: u16 = 0;
    for (c of part) {
        let nib: u16 = 0;
        if (c >= '0' && c <= '9') {
            nib = (c - '0') as u16;
        } else if (c >= 'a' && c <= 'f') {
            nib = (c - 'a' + 10) as u16;
        } else if (c >= 'A' && c <= 'F') {
            nib = (c - 'A' + 10) as u16;
        } else {
            return null;
        }
        val = (val << 4) | nib;
    }
    return val;
}

fn parseIpv6Bytes(s: []const u8): ([16]u8)? {
    if (s.len < 2 || s.len > 39) {
        return null;
    }
    let dcolon: usize? = null;
    let i: usize = 0;
    while (i + 1 < s.len) : (i += 1) {
        if (s[i] == ':' && s[i + 1] == ':') {
            if (dcolon != null) {
                return null;
            }
            dcolon = i;
            if (i + 2 < s.len && s[i + 2] == ':') {
                return null;
            }
        }
    }

    let groups: [8]u16 = [0; 8];
    if (dc := dcolon) {
        let left_groups: [8]u16 = [0; 8];
        let left_count: usize = 0;
        const left_str = s[0..dc];
        if (left_str.len > 0) {
            let start: usize = 0;
            let idx: usize = 0;
            while (idx <= left_str.len) : (idx += 1) {
                if (idx == left_str.len || left_str[idx] == ':') {
                    if (left_count >= 8) return null;
                    const g = parseHexGroup(left_str[start..idx]) orelse return null;
                    left_groups[left_count] = g;
                    left_count += 1;
                    start = idx + 1;
                }
            }
        }

        let right_groups: [8]u16 = [0; 8];
        let right_count: usize = 0;
        const right_str = s[dc + 2..];
        if (right_str.len > 0) {
            let start: usize = 0;
            let idx: usize = 0;
            while (idx <= right_str.len) : (idx += 1) {
                if (idx == right_str.len || right_str[idx] == ':') {
                    if (right_count >= 8) return null;
                    const g = parseHexGroup(right_str[start..idx]) orelse return null;
                    right_groups[right_count] = g;
                    right_count += 1;
                    start = idx + 1;
                }
            }
        }

        if (left_count + right_count >= 8) {
            return null;
        }
        let li: usize = 0;
        while (li < left_count) : (li += 1) {
            groups[li] = left_groups[li];
        }
        let ri: usize = 0;
        const right_base = 8 - right_count;
        while (ri < right_count) : (ri += 1) {
            groups[right_base + ri] = right_groups[ri];
        }
    } else {
        let count: usize = 0;
        let start: usize = 0;
        let idx: usize = 0;
        while (idx <= s.len) : (idx += 1) {
            if (idx == s.len || s[idx] == ':') {
                if (count >= 8) return null;
                const g = parseHexGroup(s[start..idx]) orelse return null;
                groups[count] = g;
                count += 1;
                start = idx + 1;
            }
        }
        if (count != 8) {
            return null;
        }
    }

    let out: [16]u8 = [0; 16];
    let g: usize = 0;
    while (g < 8) : (g += 1) {
        out[g * 2] = ((groups[g] >> 8) & 0xFF) as u8;
        out[g * 2 + 1] = (groups[g] & 0xFF) as u8;
    }
    return out;
}
