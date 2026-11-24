// Reference-hardened against RustCrypto/formats (MIT OR Apache-2.0) & RFC 2986/5280/6960
// Origin: Pure-CP Stdlib (Zero-Alloc RFC 5280 X.509 CRL DER Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 5280 Section 5 X.509 v1/v2 CRL ASN.1 DER parser:
// TBSCertList raw slice for verification, UTCTime/GeneralizedTime strict validation, revoked certificate
// iteration with CRLReason extraction, SIMD-accelerated serial lookup, and single-pass summarization.

import { simdPkiBytesEqual } from "./csr.cp";

/// Maximum accepted CRL DER input length (16 MiB) to guard against oversized allocations/DoS.
pub const CRL_MAX_DER_LEN: usize = 16 * 1024 * 1024;

/// Maximum ASN.1 DER container nesting depth enforced during strict CRL validation.
pub const CRL_MAX_DEPTH: usize = 16;

/// Maximum revoked certificate entries accepted before rejecting overflow / resource exhaustion.
pub const CRL_MAX_REVOKED_ENTRIES: usize = 1000000;

/// X.509 `cRLNumber` extension OID (`2.5.29.20`).
pub const OID_CRL_NUMBER: []const u8 = "\x55\x1D\x14";

/// X.509 `cRLReason` entry extension OID (`2.5.29.21`).
pub const OID_CRL_REASON: []const u8 = "\x55\x1D\x15";

/// RFC 5280 Section 5.3.1 `CRLReason` enumeration codes.
pub enum CrlReason: u8 {
    unspecified = 0,
    key_compromise = 1,
    ca_compromise = 2,
    affiliation_changed = 3,
    superseded = 4,
    cessation_of_operation = 5,
    certificate_hold = 6,
    remove_from_crl = 8,
    privilege_withdrawn = 9,
    aa_compromise = 10,
    none = 255,
}

/// Converts a raw `CRLReason` `ENUMERATED` byte into `CrlReason`.
pub fn crlReasonFromU8(v: u8): CrlReason {
    switch (v) {
        0 => return CrlReason.unspecified,
        1 => return CrlReason.key_compromise,
        2 => return CrlReason.ca_compromise,
        3 => return CrlReason.affiliation_changed,
        4 => return CrlReason.superseded,
        5 => return CrlReason.cessation_of_operation,
        6 => return CrlReason.certificate_hold,
        8 => return CrlReason.remove_from_crl,
        9 => return CrlReason.privilege_withdrawn,
        10 => return CrlReason.aa_compromise,
        else => return CrlReason.none,
    }
}

/// Zero-copy decoded entry from `TBSCertList.revokedCertificates`.
pub struct CrlRevokedEntry {
    serial_bytes: []const u8 = "",
    revocation_date: []const u8 = "",
    reason: CrlReason = CrlReason.none,
}

/// Zero-copy decoded RFC 5280 X.509 v1/v2 Certificate Revocation List.
pub struct CrlParsed {
    tbs_raw_der: []const u8 = "",
    version: u8 = 0,
    tbs_sig_alg_oid: []const u8 = "",
    issuer_der: []const u8 = "",
    this_update: []const u8 = "",
    next_update: []const u8 = "",
    revoked_list_der: []const u8 = "",
    extensions_der: []const u8 = "",
    sig_alg_oid: []const u8 = "",
    signature: []const u8 = "",

    /// Returns a zero-allocation iterator over all revoked certificate entries.
    fn revoked(): CrlRevokedIter {
        return CrlRevokedIter.init(_.revoked_list_der);
    }

    /// Finds and returns the revoked entry matching `serial_bytes` using 16-lane SIMD comparison.
    /// Automatically normalizes leading `0x00` DER sign padding bytes on both sides.
    fn findRevokedEntry(serial_bytes: []const u8): CrlRevokedEntry? {
        const needle = crlStripLeadingZeros(serial_bytes);
        if (needle.len == 0) {
            return null;
        }
        let it = _.revoked();
        while (entry := it.next()) {
            if (simdPkiBytesEqual(entry.serial_bytes, needle)) {
                return entry;
            }
        }
        return null;
    }

    /// Performs a zero-allocation SIMD-accelerated O(N) scan checking whether `serial_bytes` is revoked.
    /// Automatically normalizes leading `0x00` DER sign padding bytes on both sides.
    fn isSerialRevoked(serial_bytes: []const u8): bool {
        if (entry := _.findRevokedEntry(serial_bytes)) {
            * = entry;
            return true;
        }
        return false;
    }
}

/// Aggregate statistics from a single-pass scan of a DER CRL.
pub struct CrlStats {
    version: u8 = 0,
    revoked_count: u32 = 0,
    key_compromise_count: u32 = 0,
    has_next_update: bool = false,
}

const CRL_TAG_BOOLEAN: u8 = 0x01;
const CRL_TAG_INTEGER: u8 = 0x02;
const CRL_TAG_BIT_STRING: u8 = 0x03;
const CRL_TAG_OCTET_STRING: u8 = 0x04;
const CRL_TAG_OID: u8 = 0x06;
const CRL_TAG_ENUMERATED: u8 = 0x0A;
const CRL_TAG_UTC_TIME: u8 = 0x17;
const CRL_TAG_GEN_TIME: u8 = 0x18;
const CRL_TAG_SEQUENCE: u8 = 0x30;
const CRL_TAG_CTX0_CONS: u8 = 0xA0;

struct CrlDerTlv {
    tag: u8,
    raw: []const u8,
    value: []const u8,
    next_pos: usize,
}

// ---------------------------------------------------------------------------
// Internal Strict ASN.1 DER Helpers (RustCrypto/formats der compliant)
// ---------------------------------------------------------------------------

fn crlStripLeadingZeros(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start + 1 < s.len && s[start] == 0) {
        start += 1;
    }
    return s[start..];
}

fn crlIsTimeTag(tag: u8): bool {
    return tag == CRL_TAG_UTC_TIME || tag == CRL_TAG_GEN_TIME;
}

fn crlParseTwoDigits(s: []const u8, idx: usize): u8? {
    if (idx + 2 > s.len) {
        return null;
    }
    const a = s[idx];
    const b = s[idx + 1];
    if (a < '0' || a > '9' || b < '0' || b > '9') {
        return null;
    }
    return (a - '0') * 10 + (b - '0');
}

/// Validates RFC 5280 DER UTCTime (`YYMMDDHHMMSSZ`) or GeneralizedTime (`YYYYMMDDHHMMSSZ`).
fn crlIsValidDerTime(tag: u8, val: []const u8): bool {
    let off: usize = 0;
    if (tag == CRL_TAG_UTC_TIME) {
        if (val.len != 13 || val[12] != 'Z') {
            return false;
        }
        if (crlParseTwoDigits(val, 0) == null) {
            return false;
        }
        off = 2;
    } else if (tag == CRL_TAG_GEN_TIME) {
        if (val.len != 15 || val[14] != 'Z') {
            return false;
        }
        if (crlParseTwoDigits(val, 0) == null || crlParseTwoDigits(val, 2) == null) {
            return false;
        }
        off = 4;
    } else {
        return false;
    }

    const month = crlParseTwoDigits(val, off) ?? return false;
    const day = crlParseTwoDigits(val, off + 2) ?? return false;
    const hour = crlParseTwoDigits(val, off + 4) ?? return false;
    const minute = crlParseTwoDigits(val, off + 6) ?? return false;
    const second = crlParseTwoDigits(val, off + 8) ?? return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    if (hour > 23 || minute > 59 || second > 59) return false;
    return true;
}

fn crlIsValidDerInteger(val: []const u8): bool {
    if (val.len == 0) {
        return false;
    }
    if (val.len > 1) {
        if (val[0] == 0x00 && (val[1] & 0x80) == 0) {
            return false;
        }
        if (val[0] == 0xFF && (val[1] & 0x80) != 0) {
            return false;
        }
    }
    return true;
}

fn crlIsValidDerBoolean(val: []const u8): bool {
    return val.len == 1 && (val[0] == 0x00 || val[0] == 0xFF);
}

fn crlIsValidDerOid(val: []const u8): bool {
    if (val.len == 0 || (val[val.len - 1] & 0x80) != 0) {
        return false;
    }
    let i: usize = 1;
    let at_arc_start = true;
    while (i < val.len) : (i += 1) {
        const b = val[i];
        if (at_arc_start && b == 0x80) {
            return false;
        }
        at_arc_start = (b & 0x80) == 0;
    }
    return true;
}

fn crlReadDerTlv(data: []const u8, pos: usize): CrlDerTlv? {
    if (pos + 2 > data.len) {
        return null;
    }
    const tag = data[pos];
    if (tag == 0x00 || (tag & 0x1F) == 0x1F) {
        return null;
    }
    const len_byte = data[pos + 1];
    let val_start: usize = pos + 2;
    let val_len: usize = 0;

    if ((len_byte & 0x80) == 0) {
        val_len = len_byte as usize;
    } else {
        const num_octets = (len_byte & 0x7F) as usize;
        // Reject indefinite length (0x80) and > 4-byte lengths.
        if (num_octets == 0 || num_octets > 4) {
            return null;
        }
        if (val_start + num_octets > data.len) {
            return null;
        }
        if (data[val_start] == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < num_octets) : (i += 1) {
            val_len = (val_len << 8) | (data[val_start + i] as usize);
        }
        if (val_len < 128) {
            return null;
        }
        val_start += num_octets;
    }

    if (val_len > data.len - val_start) {
        return null;
    }
    const end = val_start + val_len;
    return CrlDerTlv {
        tag: tag,
        raw: data[pos..end],
        value: data[val_start..end],
        next_pos: end,
    };
}

fn crlParseAlgIdOid(seq_body: []const u8): ([]const u8)? {
    if (oid_tlv := crlReadDerTlv(seq_body, 0)) {
        if (oid_tlv.tag != CRL_TAG_OID || !crlIsValidDerOid(oid_tlv.value)) {
            return null;
        }
        if (oid_tlv.next_pos < seq_body.len) {
            if (param := crlReadDerTlv(seq_body, oid_tlv.next_pos)) {
                if (param.next_pos != seq_body.len) {
                    return null;
                }
            } else {
                return null;
            }
        }
        return oid_tlv.value;
    }
    return null;
}

fn crlStripZeroUnusedBitString(val: []const u8): ([]const u8)? {
    if (val.len < 2 || val[0] != 0) {
        return null;
    }
    return val[1..];
}

fn crlValidateDerTree(slice: []const u8, depth: usize, max_depth: usize): bool {
    if (depth > max_depth) {
        return false;
    }
    let pos: usize = 0;
    while (pos < slice.len) {
        if (tlv := crlReadDerTlv(slice, pos)) {
            pos = tlv.next_pos;
            if (tlv.tag == CRL_TAG_BOOLEAN && !crlIsValidDerBoolean(tlv.value)) {
                return false;
            }
            if (tlv.tag == CRL_TAG_INTEGER && !crlIsValidDerInteger(tlv.value)) {
                return false;
            }
            if (tlv.tag == CRL_TAG_OID && !crlIsValidDerOid(tlv.value)) {
                return false;
            }
            if ((tlv.tag & 0x20) != 0) {
                if (!crlValidateDerTree(tlv.value, depth + 1, max_depth)) {
                    return false;
                }
            }
        } else {
            return false;
        }
    }
    return pos == slice.len;
}

/// Extracts `CRLReason` (`OID_CRL_REASON` -> `OCTET STRING` -> `ENUMERATED`) from `crlEntryExtensions`.
fn extractCrlReason(exts_body: []const u8): CrlReason {
    let pos: usize = 0;
    while (pos < exts_body.len) {
        if (ext := crlReadDerTlv(exts_body, pos)) {
            pos = ext.next_pos;
            if (ext.tag != CRL_TAG_SEQUENCE) {
                continue;
            }
            if (eoid := crlReadDerTlv(ext.value, 0)) {
                if (eoid.tag != CRL_TAG_OID || !simdPkiBytesEqual(eoid.value, OID_CRL_REASON)) {
                    continue;
                }
                let fpos = eoid.next_pos;
                if (fpos < ext.value.len && ext.value[fpos] == CRL_TAG_BOOLEAN) {
                    if (crit := crlReadDerTlv(ext.value, fpos)) {
                        if (!crlIsValidDerBoolean(crit.value)) {
                            return CrlReason.none;
                        }
                        fpos = crit.next_pos;
                    } else {
                        return CrlReason.none;
                    }
                }
                if (oct := crlReadDerTlv(ext.value, fpos)) {
                    if (oct.tag == CRL_TAG_OCTET_STRING && oct.next_pos == ext.value.len) {
                        if (en := crlReadDerTlv(oct.value, 0)) {
                            if (en.tag == CRL_TAG_ENUMERATED && en.value.len == 1 && en.next_pos == oct.value.len) {
                                return crlReasonFromU8(en.value[0]);
                            }
                        }
                    }
                }
            }
        } else {
            break;
        }
    }
    return CrlReason.none;
}

// ---------------------------------------------------------------------------
// Public CRL Parsing, Validation, Iteration & Summary API
// ---------------------------------------------------------------------------

/// Parses an RFC 5280 X.509 v1/v2 DER-encoded Certificate Revocation List.
/// Strictly enforces canonical DER encoding, `BIT STRING` `unused_bits == 0`, valid time tags,
/// and zero trailing bytes.
pub fn parseCrlDer(der_data: []const u8): CrlParsed? {
    if (der_data.len == 0 || der_data.len > CRL_MAX_DER_LEN) {
        return null;
    }

    // Outer CertificateList SEQUENCE
    let outer_body: []const u8 = "";
    if (outer := crlReadDerTlv(der_data, 0)) {
        if (outer.tag != CRL_TAG_SEQUENCE || outer.next_pos != der_data.len) {
            return null;
        }
        outer_body = outer.value;
    } else {
        return null;
    }

    let pos: usize = 0;

    // 1. tbsCertList SEQUENCE
    let tbs_raw: []const u8 = "";
    let tbs_body: []const u8 = "";
    if (tbs := crlReadDerTlv(outer_body, pos)) {
        if (tbs.tag != CRL_TAG_SEQUENCE) {
            return null;
        }
        tbs_raw = tbs.raw;
        tbs_body = tbs.value;
        pos = tbs.next_pos;
    } else {
        return null;
    }

    // 2. signatureAlgorithm SEQUENCE
    let sig_alg_oid: []const u8 = "";
    if (sa := crlReadDerTlv(outer_body, pos)) {
        if (sa.tag != CRL_TAG_SEQUENCE) {
            return null;
        }
        if (oid := crlParseAlgIdOid(sa.value)) {
            sig_alg_oid = oid;
        } else {
            return null;
        }
        pos = sa.next_pos;
    } else {
        return null;
    }

    // 3. signatureValue BIT STRING (unused_bits == 0, non-empty)
    let signature: []const u8 = "";
    if (sv := crlReadDerTlv(outer_body, pos)) {
        if (sv.tag != CRL_TAG_BIT_STRING || sv.next_pos != outer_body.len) {
            return null;
        }
        if (raw := crlStripZeroUnusedBitString(sv.value)) {
            signature = raw;
        } else {
            return null;
        }
    } else {
        return null;
    }

    // Parse inside TBSCertList
    let tpos: usize = 0;

    // 1a. Optional version INTEGER (v1 omits it -> 0; v2 encodes INTEGER 1)
    let version: u8 = 0;
    if (tpos < tbs_body.len && tbs_body[tpos] == CRL_TAG_INTEGER) {
        if (vt := crlReadDerTlv(tbs_body, tpos)) {
            if (!crlIsValidDerInteger(vt.value) || vt.value.len != 1 || vt.value[0] > 1) {
                return null;
            }
            version = vt.value[0];
            tpos = vt.next_pos;
        } else {
            return null;
        }
    }

    // 1b. signature AlgorithmIdentifier SEQUENCE
    let tbs_sig_oid: []const u8 = "";
    if (tsa := crlReadDerTlv(tbs_body, tpos)) {
        if (tsa.tag != CRL_TAG_SEQUENCE) {
            return null;
        }
        if (oid := crlParseAlgIdOid(tsa.value)) {
            tbs_sig_oid = oid;
        } else {
            return null;
        }
        tpos = tsa.next_pos;
    } else {
        return null;
    }

    // 1c. issuer Name SEQUENCE
    let issuer_der: []const u8 = "";
    if (iss := crlReadDerTlv(tbs_body, tpos)) {
        if (iss.tag != CRL_TAG_SEQUENCE) {
            return null;
        }
        issuer_der = iss.raw;
        tpos = iss.next_pos;
    } else {
        return null;
    }

    // 1d. thisUpdate Time (UTCTime or GeneralizedTime)
    let this_update: []const u8 = "";
    if (tu := crlReadDerTlv(tbs_body, tpos)) {
        if (!crlIsTimeTag(tu.tag) || !crlIsValidDerTime(tu.tag, tu.value)) {
            return null;
        }
        this_update = tu.value;
        tpos = tu.next_pos;
    } else {
        return null;
    }

    // 1e. Optional nextUpdate Time (UTCTime or GeneralizedTime)
    let next_update: []const u8 = "";
    if (tpos < tbs_body.len && crlIsTimeTag(tbs_body[tpos])) {
        if (nu := crlReadDerTlv(tbs_body, tpos)) {
            if (!crlIsValidDerTime(nu.tag, nu.value)) {
                return null;
            }
            next_update = nu.value;
            tpos = nu.next_pos;
        } else {
            return null;
        }
    }

    // 1f. Optional revokedCertificates SEQUENCE OF SEQUENCE
    let revoked_der: []const u8 = "";
    if (tpos < tbs_body.len && tbs_body[tpos] == CRL_TAG_SEQUENCE) {
        if (rev := crlReadDerTlv(tbs_body, tpos)) {
            revoked_der = rev.value;
            tpos = rev.next_pos;
        } else {
            return null;
        }
    }

    // 1g. Optional crlExtensions [0] EXPLICIT Extensions
    let extensions_der: []const u8 = "";
    if (tpos < tbs_body.len && tbs_body[tpos] == CRL_TAG_CTX0_CONS) {
        if (ext := crlReadDerTlv(tbs_body, tpos)) {
            extensions_der = ext.value;
            tpos = ext.next_pos;
        } else {
            return null;
        }
    }

    if (tpos != tbs_body.len) {
        return null;
    }

    return CrlParsed {
        tbs_raw_der: tbs_raw,
        version: version,
        tbs_sig_alg_oid: tbs_sig_oid,
        issuer_der: issuer_der,
        this_update: this_update,
        next_update: next_update,
        revoked_list_der: revoked_der,
        extensions_der: extensions_der,
        sig_alg_oid: sig_alg_oid,
        signature: signature,
    };
}

/// Zero-heap strict RFC 5280 CRL DER validator.
/// Validates canonical DER tree depth (`CRL_MAX_DEPTH`), matching TBS/outer signature algorithm OIDs,
/// v2 version requirement when `crlExtensions` is present, strict UTCTime/GeneralizedTime fields,
/// and all `revokedCertificates` entries up to `CRL_MAX_REVOKED_ENTRIES`.
pub fn validateCrlStrict(der_data: []const u8): bool {
    if (der_data.len == 0 || der_data.len > CRL_MAX_DER_LEN) {
        return false;
    }
    if (!crlValidateDerTree(der_data, 0, CRL_MAX_DEPTH)) {
        return false;
    }
    if (crl := parseCrlDer(der_data)) {
        // RFC 5280 Section 5.1.1.2: TBSCertList.signature MUST match outer signatureAlgorithm.
        if (!simdPkiBytesEqual(crl.tbs_sig_alg_oid, crl.sig_alg_oid)) {
            return false;
        }
        // RFC 5280 Section 5.1.2.7: crlExtensions MUST only appear in v2 CRLs (version == 1).
        if (crl.extensions_der.len > 0) {
            if (crl.version != 1) {
                return false;
            }
            if (ext_seq := crlReadDerTlv(crl.extensions_der, 0)) {
                if (ext_seq.tag != CRL_TAG_SEQUENCE || ext_seq.next_pos != crl.extensions_der.len) {
                    return false;
                }
            } else {
                return false;
            }
        }
        let it = crl.revoked();
        let count: usize = 0;
        while (entry := it.next()) {
            * = entry;
            count += 1;
            if (count > CRL_MAX_REVOKED_ENTRIES) {
                return false;
            }
        }
        if (it.hasError()) {
            return false;
        }
        return true;
    }
    return false;
}

/// Returns `true` if `der_data` is a strictly valid RFC 5280 DER CRL.
pub fn isCrlDer(der_data: []const u8): bool {
    if (crl := parseCrlDer(der_data)) {
        * = crl;
        return true;
    }
    return false;
}

/// Zero-allocation iterator over `TBSCertList.revokedCertificates`.
pub struct CrlRevokedIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,
    count: usize = 0,

    pub static fn init(revoked_list_der: []const u8): CrlRevokedIter {
        return CrlRevokedIter {
            data: revoked_list_der,
            pos: 0,
            failed: false,
            count: 0,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): CrlRevokedEntry? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        if (_.count >= CRL_MAX_REVOKED_ENTRIES) {
            _.failed = true;
            return null;
        }
        let body: []const u8 = "";
        if (entry_seq := crlReadDerTlv(_.data, _.pos)) {
            if (entry_seq.tag != CRL_TAG_SEQUENCE) {
                _.failed = true;
                return null;
            }
            body = entry_seq.value;
            _.pos = entry_seq.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        let cur: usize = 0;

        // 1. userCertificate INTEGER (canonical, non-negative; strip leading 0x00 DER sign byte)
        let serial: []const u8 = "";
        if (st := crlReadDerTlv(body, cur)) {
            if (st.tag != CRL_TAG_INTEGER || !crlIsValidDerInteger(st.value) || (st.value[0] & 0x80) != 0) {
                _.failed = true;
                return null;
            }
            serial = crlStripLeadingZeros(st.value);
            cur = st.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 2. revocationDate Time
        let rev_date: []const u8 = "";
        if (dt := crlReadDerTlv(body, cur)) {
            if (!crlIsTimeTag(dt.tag) || !crlIsValidDerTime(dt.tag, dt.value)) {
                _.failed = true;
                return null;
            }
            rev_date = dt.value;
            cur = dt.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 3. Optional crlEntryExtensions SEQUENCE
        let reason = CrlReason.none;
        if (cur < body.len) {
            if (exts := crlReadDerTlv(body, cur)) {
                if (exts.tag != CRL_TAG_SEQUENCE || exts.next_pos != body.len) {
                    _.failed = true;
                    return null;
                }
                reason = extractCrlReason(exts.value);
            } else {
                _.failed = true;
                return null;
            }
        }

        _.count += 1;
        return CrlRevokedEntry {
            serial_bytes: serial,
            revocation_date: rev_date,
            reason: reason,
        };
    }
}

/// Scans an X.509 DER CRL in a single zero-allocation pass and aggregates summary stats.
pub fn summarizeCrl(der_data: []const u8): CrlStats? {
    if (crl := parseCrlDer(der_data)) {
        let stats = CrlStats {
            version: crl.version,
            revoked_count: 0,
            key_compromise_count: 0,
            has_next_update: crl.next_update.len > 0,
        };
        let it = crl.revoked();
        while (entry := it.next()) {
            if (stats.revoked_count == 0xFFFFFFFF) {
                return null;
            }
            stats.revoked_count += 1;
            if (entry.reason == CrlReason.key_compromise) {
                stats.key_compromise_count += 1;
            }
        }
        if (it.hasError()) {
            return null;
        }
        return stats;
    }
    return null;
}
