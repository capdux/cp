// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 5280 X.509 CRL DER Parser)
// Pure CP zero-heap-allocation RFC 5280 Section 5 X.509 v1/v2 CRL ASN.1 DER parser:
// TBSCertList raw slice for verification, UTCTime/GeneralizedTime support, revoked certificate
// iteration with CRLReason extraction, O(N) serial lookup, and single-pass summarization.

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

    /// Performs a zero-allocation O(N) scan checking whether `serial_bytes` is revoked.
    /// Automatically normalizes leading `0x00` DER sign padding bytes on both sides.
    fn isSerialRevoked(serial_bytes: []const u8): bool {
        const needle = stripLeadingZeros(serial_bytes);
        if (needle.len == 0) {
            return false;
        }
        let it = _.revoked();
        while (entry := it.next()) {
            if (bytesEqual(entry.serial_bytes, needle)) {
                return true;
            }
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

const TAG_BOOLEAN: u8 = 0x01;
const TAG_INTEGER: u8 = 0x02;
const TAG_BIT_STRING: u8 = 0x03;
const TAG_OCTET_STRING: u8 = 0x04;
const TAG_OID: u8 = 0x06;
const TAG_ENUMERATED: u8 = 0x0A;
const TAG_UTC_TIME: u8 = 0x17;
const TAG_GEN_TIME: u8 = 0x18;
const TAG_SEQUENCE: u8 = 0x30;
const TAG_CTX0_CONS: u8 = 0xA0;

struct DerTlv {
    tag: u8,
    raw: []const u8,
    value: []const u8,
    next_pos: usize,
}

// ---------------------------------------------------------------------------
// Internal Strict ASN.1 DER Helpers
// ---------------------------------------------------------------------------

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn stripLeadingZeros(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start + 1 < s.len && s[start] == 0) {
        start += 1;
    }
    return s[start..];
}

fn isTimeTag(tag: u8): bool {
    return tag == TAG_UTC_TIME || tag == TAG_GEN_TIME;
}

fn readDerTlv(data: []const u8, pos: usize): DerTlv? {
    if (pos + 2 > data.len) {
        return null;
    }
    const tag = data[pos];
    const len_byte = data[pos + 1];
    let val_start: usize = pos + 2;
    let val_len: usize = 0;

    if ((len_byte & 0x80) == 0) {
        val_len = len_byte as usize;
    } else {
        const num_octets = (len_byte & 0x7F) as usize;
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
        while (i < num_octets) {
            val_len = (val_len << 8) | (data[val_start + i] as usize);
            i += 1;
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
    return DerTlv {
        tag: tag,
        raw: data[pos..end],
        value: data[val_start..end],
        next_pos: end,
    };
}

fn parseAlgIdOid(seq_body: []const u8): ([]const u8)? {
    if (oid_tlv := readDerTlv(seq_body, 0)) {
        if (oid_tlv.tag != TAG_OID || oid_tlv.value.len == 0) {
            return null;
        }
        return oid_tlv.value;
    }
    return null;
}

fn stripZeroUnusedBitString(val: []const u8): ([]const u8)? {
    if (val.len < 1 || val[0] != 0) {
        return null;
    }
    return val[1..];
}

/// Extracts `CRLReason` (`OID_CRL_REASON` -> `OCTET STRING` -> `ENUMERATED`) from `crlEntryExtensions`.
fn extractCrlReason(exts_body: []const u8): CrlReason {
    let pos: usize = 0;
    while (pos < exts_body.len) {
        if (ext := readDerTlv(exts_body, pos)) {
            pos = ext.next_pos;
            if (ext.tag != TAG_SEQUENCE) {
                continue;
            }
            if (eoid := readDerTlv(ext.value, 0)) {
                if (eoid.tag != TAG_OID || !bytesEqual(eoid.value, OID_CRL_REASON)) {
                    continue;
                }
                let fpos = eoid.next_pos;
                if (fpos < ext.value.len && ext.value[fpos] == TAG_BOOLEAN) {
                    if (crit := readDerTlv(ext.value, fpos)) {
                        fpos = crit.next_pos;
                    } else {
                        return CrlReason.none;
                    }
                }
                if (oct := readDerTlv(ext.value, fpos)) {
                    if (oct.tag == TAG_OCTET_STRING) {
                        if (en := readDerTlv(oct.value, 0)) {
                            if (en.tag == TAG_ENUMERATED && en.value.len == 1) {
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
// Public CRL Parsing, Iteration & Summary API
// ---------------------------------------------------------------------------

/// Parses an RFC 5280 X.509 v1/v2 DER-encoded Certificate Revocation List.
/// Strictly enforces valid DER encoding, `BIT STRING` `unused_bits == 0`, and zero trailing bytes.
pub fn parseCrlDer(der_data: []const u8): CrlParsed? {
    // Outer CertificateList SEQUENCE
    let outer_body: []const u8 = "";
    if (outer := readDerTlv(der_data, 0)) {
        if (outer.tag != TAG_SEQUENCE || outer.next_pos != der_data.len) {
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
    if (tbs := readDerTlv(outer_body, pos)) {
        if (tbs.tag != TAG_SEQUENCE) {
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
    if (sa := readDerTlv(outer_body, pos)) {
        if (sa.tag != TAG_SEQUENCE) {
            return null;
        }
        if (oid := parseAlgIdOid(sa.value)) {
            sig_alg_oid = oid;
        } else {
            return null;
        }
        pos = sa.next_pos;
    } else {
        return null;
    }

    // 3. signatureValue BIT STRING (unused_bits == 0)
    let signature: []const u8 = "";
    if (sv := readDerTlv(outer_body, pos)) {
        if (sv.tag != TAG_BIT_STRING || sv.next_pos != outer_body.len) {
            return null;
        }
        if (raw := stripZeroUnusedBitString(sv.value)) {
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
    if (tpos < tbs_body.len && tbs_body[tpos] == TAG_INTEGER) {
        if (vt := readDerTlv(tbs_body, tpos)) {
            if (vt.value.len != 1 || vt.value[0] > 1) {
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
    if (tsa := readDerTlv(tbs_body, tpos)) {
        if (tsa.tag != TAG_SEQUENCE) {
            return null;
        }
        if (oid := parseAlgIdOid(tsa.value)) {
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
    if (iss := readDerTlv(tbs_body, tpos)) {
        if (iss.tag != TAG_SEQUENCE) {
            return null;
        }
        issuer_der = iss.raw;
        tpos = iss.next_pos;
    } else {
        return null;
    }

    // 1d. thisUpdate Time (UTCTime or GeneralizedTime)
    let this_update: []const u8 = "";
    if (tu := readDerTlv(tbs_body, tpos)) {
        if (!isTimeTag(tu.tag) || tu.value.len == 0) {
            return null;
        }
        this_update = tu.value;
        tpos = tu.next_pos;
    } else {
        return null;
    }

    // 1e. Optional nextUpdate Time (UTCTime or GeneralizedTime)
    let next_update: []const u8 = "";
    if (tpos < tbs_body.len && isTimeTag(tbs_body[tpos])) {
        if (nu := readDerTlv(tbs_body, tpos)) {
            if (nu.value.len == 0) {
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
    if (tpos < tbs_body.len && tbs_body[tpos] == TAG_SEQUENCE) {
        if (rev := readDerTlv(tbs_body, tpos)) {
            revoked_der = rev.value;
            tpos = rev.next_pos;
        } else {
            return null;
        }
    }

    // 1g. Optional crlExtensions [0] EXPLICIT Extensions
    let extensions_der: []const u8 = "";
    if (tpos < tbs_body.len && tbs_body[tpos] == TAG_CTX0_CONS) {
        if (ext := readDerTlv(tbs_body, tpos)) {
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

    pub fn init(revoked_list_der: []const u8): CrlRevokedIter {
        return CrlRevokedIter {
            data: revoked_list_der,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): CrlRevokedEntry? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        let body: []const u8 = "";
        if (entry_seq := readDerTlv(_.data, _.pos)) {
            if (entry_seq.tag != TAG_SEQUENCE) {
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

        // 1. userCertificate INTEGER (strip leading 0x00 DER sign byte)
        let serial: []const u8 = "";
        if (st := readDerTlv(body, cur)) {
            if (st.tag != TAG_INTEGER || st.value.len == 0) {
                _.failed = true;
                return null;
            }
            serial = stripLeadingZeros(st.value);
            cur = st.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 2. revocationDate Time
        let rev_date: []const u8 = "";
        if (dt := readDerTlv(body, cur)) {
            if (!isTimeTag(dt.tag) || dt.value.len == 0) {
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
            if (exts := readDerTlv(body, cur)) {
                if (exts.tag != TAG_SEQUENCE || exts.next_pos != body.len) {
                    _.failed = true;
                    return null;
                }
                reason = extractCrlReason(exts.value);
            } else {
                _.failed = true;
                return null;
            }
        }

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
