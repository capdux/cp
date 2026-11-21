// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 5652 CMS / PKCS#7 DER Parser)
// Pure CP zero-heap-allocation RFC 5652 CMS / RFC 2315 PKCS#7 ASN.1 DER parser:
// ContentInfo OID dispatch, SignedData (attached & detached), X.509 CertificateSet
// iteration, and SignerInfo (digest/signature algorithm OIDs, signedAttrs, signature) iteration.

/// CMS / PKCS#7 `id-data` OID (`1.2.840.113549.1.7.1`).
pub const OID_DATA: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x07\x01";

/// CMS / PKCS#7 `id-signedData` OID (`1.2.840.113549.1.7.2`).
pub const OID_SIGNED_DATA: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x07\x02";

/// CMS / PKCS#7 `id-envelopedData` OID (`1.2.840.113549.1.7.3`).
pub const OID_ENVELOPED_DATA: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x07\x03";

/// CMS / PKCS#7 `id-digestedData` OID (`1.2.840.113549.1.7.5`).
pub const OID_DIGESTED_DATA: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x07\x05";

/// CMS / PKCS#7 `id-encryptedData` OID (`1.2.840.113549.1.7.6`).
pub const OID_ENCRYPTED_DATA: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x07\x06";

/// Recognized CMS / PKCS#7 `ContentInfo.contentType` classifications.
pub enum Pkcs7ContentType: u8 {
    data = 0,
    signed_data = 1,
    enveloped_data = 2,
    digested_data = 3,
    encrypted_data = 4,
    unknown = 255,
}

/// Zero-copy decoded ASN.1 DER `ContentInfo` container.
pub struct Pkcs7ContentInfo {
    content_type: Pkcs7ContentType = Pkcs7ContentType.unknown,
    content_type_oid: []const u8 = "",
    content_der: []const u8 = "",
}

/// Zero-copy decoded RFC 5652 `SignedData` container.
pub struct Pkcs7SignedData {
    version: u64 = 0,
    digest_algs_set: []const u8 = "",
    encap_content_type_oid: []const u8 = "",
    encap_content: []const u8 = "",
    is_detached: bool = true,
    certificates_der: []const u8 = "",
    crls_der: []const u8 = "",
    signer_infos_set: []const u8 = "",

    /// Returns a zero-allocation iterator over DER-encoded X.509 certificates in `certificates_der`.
    fn certs(): Pkcs7CertIter {
        return Pkcs7CertIter.init(_.certificates_der);
    }

    /// Returns a zero-allocation iterator over `SignerInfo` records in `signer_infos_set`.
    fn signers(): Pkcs7SignerIter {
        return Pkcs7SignerIter.init(_.signer_infos_set);
    }
}

/// Zero-copy decoded RFC 5652 `SignerInfo` entry.
pub struct Pkcs7SignerInfo {
    version: u64 = 0,
    digest_alg_oid: []const u8 = "",
    sig_alg_oid: []const u8 = "",
    signature: []const u8 = "",
    has_signed_attrs: bool = false,
}

const TAG_INTEGER: u8 = 0x02;
const TAG_OCTET_STRING: u8 = 0x04;
const TAG_OID: u8 = 0x06;
const TAG_SEQUENCE: u8 = 0x30;
const TAG_SET: u8 = 0x31;
const TAG_CTX0_PRIM: u8 = 0x80;
const TAG_CTX0_CONS: u8 = 0xA0;
const TAG_CTX1_CONS: u8 = 0xA1;

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

fn classifyContentTypeOid(oid: []const u8): Pkcs7ContentType {
    if (bytesEqual(oid, OID_DATA)) {
        return Pkcs7ContentType.data;
    }
    if (bytesEqual(oid, OID_SIGNED_DATA)) {
        return Pkcs7ContentType.signed_data;
    }
    if (bytesEqual(oid, OID_ENVELOPED_DATA)) {
        return Pkcs7ContentType.enveloped_data;
    }
    if (bytesEqual(oid, OID_DIGESTED_DATA)) {
        return Pkcs7ContentType.digested_data;
    }
    if (bytesEqual(oid, OID_ENCRYPTED_DATA)) {
        return Pkcs7ContentType.encrypted_data;
    }
    return Pkcs7ContentType.unknown;
}

/// Strictly decodes a single ASN.1 DER Tag-Length-Value element starting at `pos`.
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
        // Reject indefinite length (0x80) and oversized length encodings (> 4 bytes)
        if (num_octets == 0 || num_octets > 4) {
            return null;
        }
        if (val_start + num_octets > data.len) {
            return null;
        }
        // Strict DER: leading length byte must be non-zero
        if (data[val_start] == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < num_octets) {
            val_len = (val_len << 8) | (data[val_start + i] as usize);
            i += 1;
        }
        // Strict DER: long form must only be used for lengths >= 128
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

/// Decodes a non-negative ASN.1 DER `INTEGER` payload into `u64`.
fn decodeDerU64(val: []const u8): u64? {
    if (val.len == 0 || val.len > 9) {
        return null;
    }
    // Reject negative integers (MSB set)
    if ((val[0] & 0x80) != 0) {
        return null;
    }
    let start: usize = 0;
    if (val.len == 9) {
        if (val[0] != 0) {
            return null;
        }
        start = 1;
    }
    let acc: u64 = 0;
    let i = start;
    while (i < val.len) {
        acc = (acc << 8) | (val[i] as u64);
        i += 1;
    }
    return acc;
}

/// Extracts the leading `OBJECT IDENTIFIER` from an `AlgorithmIdentifier ::= SEQUENCE { algorithm OID, ... }`.
fn parseAlgIdentifierOid(seq_body: []const u8): ([]const u8)? {
    if (oid_tlv := readDerTlv(seq_body, 0)) {
        if (oid_tlv.tag != TAG_OID || oid_tlv.value.len == 0) {
            return null;
        }
        return oid_tlv.value;
    }
    return null;
}

// ---------------------------------------------------------------------------
// Public ContentInfo & SignedData Parsing API
// ---------------------------------------------------------------------------

/// Parses an outer ASN.1 DER `ContentInfo` `SEQUENCE { contentType OID, content [0] EXPLICIT OPTIONAL }`.
pub fn parseContentInfo(der_data: []const u8): Pkcs7ContentInfo? {
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
    let oid_bytes: []const u8 = "";
    if (oid_tlv := readDerTlv(outer_body, pos)) {
        if (oid_tlv.tag != TAG_OID || oid_tlv.value.len == 0) {
            return null;
        }
        oid_bytes = oid_tlv.value;
        pos = oid_tlv.next_pos;
    } else {
        return null;
    }

    let content_der: []const u8 = "";
    if (pos < outer_body.len) {
        if (ctx0 := readDerTlv(outer_body, pos)) {
            if (ctx0.tag != TAG_CTX0_CONS || ctx0.next_pos != outer_body.len) {
                return null;
            }
            content_der = ctx0.value;
        } else {
            return null;
        }
    }

    return Pkcs7ContentInfo {
        content_type: classifyContentTypeOid(oid_bytes),
        content_type_oid: oid_bytes,
        content_der: content_der,
    };
}

/// Returns `true` if `der_data` is a valid DER `ContentInfo` whose `contentType` is `id-signedData`.
pub fn isPkcs7SignedData(der_data: []const u8): bool {
    if (ci := parseContentInfo(der_data)) {
        return ci.content_type == Pkcs7ContentType.signed_data && ci.content_der.len > 0;
    }
    return false;
}

/// Parses a complete DER `ContentInfo` wrapping an RFC 5652 `SignedData` structure.
pub fn parseSignedData(der_data: []const u8): Pkcs7SignedData? {
    let sd_der: []const u8 = "";
    if (ci := parseContentInfo(der_data)) {
        if (ci.content_type != Pkcs7ContentType.signed_data || ci.content_der.len == 0) {
            return null;
        }
        sd_der = ci.content_der;
    } else {
        return null;
    }

    // Unwrap SignedData SEQUENCE
    let sd_body: []const u8 = "";
    if (sd_seq := readDerTlv(sd_der, 0)) {
        if (sd_seq.tag != TAG_SEQUENCE || sd_seq.next_pos != sd_der.len) {
            return null;
        }
        sd_body = sd_seq.value;
    } else {
        return null;
    }

    let pos: usize = 0;

    // 1. version INTEGER
    let version: u64 = 0;
    if (ver_tlv := readDerTlv(sd_body, pos)) {
        if (ver_tlv.tag != TAG_INTEGER) {
            return null;
        }
        if (v := decodeDerU64(ver_tlv.value)) {
            version = v;
        } else {
            return null;
        }
        pos = ver_tlv.next_pos;
    } else {
        return null;
    }

    // 2. digestAlgorithms SET
    let digest_algs: []const u8 = "";
    if (da_tlv := readDerTlv(sd_body, pos)) {
        if (da_tlv.tag != TAG_SET) {
            return null;
        }
        digest_algs = da_tlv.value;
        pos = da_tlv.next_pos;
    } else {
        return null;
    }

    // 3. encapContentInfo SEQUENCE
    let encap_oid: []const u8 = "";
    let encap_content: []const u8 = "";
    let is_detached = true;
    if (eci_tlv := readDerTlv(sd_body, pos)) {
        if (eci_tlv.tag != TAG_SEQUENCE) {
            return null;
        }
        pos = eci_tlv.next_pos;

        let eci_pos: usize = 0;
        if (eoid := readDerTlv(eci_tlv.value, eci_pos)) {
            if (eoid.tag != TAG_OID || eoid.value.len == 0) {
                return null;
            }
            encap_oid = eoid.value;
            eci_pos = eoid.next_pos;
        } else {
            return null;
        }

        if (eci_pos < eci_tlv.value.len) {
            if (ectx0 := readDerTlv(eci_tlv.value, eci_pos)) {
                if (ectx0.tag != TAG_CTX0_CONS || ectx0.next_pos != eci_tlv.value.len) {
                    return null;
                }
                if (oct := readDerTlv(ectx0.value, 0)) {
                    if (oct.tag != TAG_OCTET_STRING || oct.next_pos != ectx0.value.len) {
                        return null;
                    }
                    encap_content = oct.value;
                    is_detached = false;
                } else {
                    return null;
                }
            } else {
                return null;
            }
        }
    } else {
        return null;
    }

    // 4. Optional certificates [0] IMPLICIT CertificateSet
    let certs_der: []const u8 = "";
    if (pos < sd_body.len && sd_body[pos] == TAG_CTX0_CONS) {
        if (c_tlv := readDerTlv(sd_body, pos)) {
            certs_der = c_tlv.value;
            pos = c_tlv.next_pos;
        } else {
            return null;
        }
    }

    // 5. Optional crls [1] IMPLICIT RevocationInfoChoices
    let crls_der: []const u8 = "";
    if (pos < sd_body.len && sd_body[pos] == TAG_CTX1_CONS) {
        if (r_tlv := readDerTlv(sd_body, pos)) {
            crls_der = r_tlv.value;
            pos = r_tlv.next_pos;
        } else {
            return null;
        }
    }

    // 6. signerInfos SET
    let signer_infos: []const u8 = "";
    if (si_tlv := readDerTlv(sd_body, pos)) {
        if (si_tlv.tag != TAG_SET || si_tlv.next_pos != sd_body.len) {
            return null;
        }
        signer_infos = si_tlv.value;
    } else {
        return null;
    }

    return Pkcs7SignedData {
        version: version,
        digest_algs_set: digest_algs,
        encap_content_type_oid: encap_oid,
        encap_content: encap_content,
        is_detached: is_detached,
        certificates_der: certs_der,
        crls_der: crls_der,
        signer_infos_set: signer_infos,
    };
}

// ---------------------------------------------------------------------------
// Zero-Alloc CertificateSet & SignerInfos Iterators
// ---------------------------------------------------------------------------

/// Zero-allocation iterator yielding each full DER-encoded X.509 certificate in `certificates_der`.
pub struct Pkcs7CertIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    pub fn init(certificates_der: []const u8): Pkcs7CertIter {
        return Pkcs7CertIter {
            data: certificates_der,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    /// Returns the complete DER-encoded X.509 certificate (`raw_der`), or `null` at EOF/error.
    fn next(*_): ([]const u8)? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        if (tlv := readDerTlv(_.data, _.pos)) {
            _.pos = tlv.next_pos;
            return tlv.raw;
        }
        _.failed = true;
        return null;
    }
}

/// Zero-allocation iterator yielding each `Pkcs7SignerInfo` in `signer_infos_set`.
pub struct Pkcs7SignerIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    pub fn init(signer_infos_set: []const u8): Pkcs7SignerIter {
        return Pkcs7SignerIter {
            data: signer_infos_set,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): Pkcs7SignerInfo? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        let si_body: []const u8 = "";
        if (si_tlv := readDerTlv(_.data, _.pos)) {
            if (si_tlv.tag != TAG_SEQUENCE) {
                _.failed = true;
                return null;
            }
            si_body = si_tlv.value;
            _.pos = si_tlv.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        let cur: usize = 0;

        // 1. version INTEGER
        let ver: u64 = 0;
        if (vt := readDerTlv(si_body, cur)) {
            if (vt.tag != TAG_INTEGER) { _.failed = true; return null; }
            if (v := decodeDerU64(vt.value)) { ver = v; } else { _.failed = true; return null; }
            cur = vt.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 2. sid SignerIdentifier (SEQUENCE IssuerAndSerialNumber OR [0] subjectKeyIdentifier)
        if (sid := readDerTlv(si_body, cur)) {
            if (sid.tag != TAG_SEQUENCE && sid.tag != TAG_CTX0_PRIM) {
                _.failed = true;
                return null;
            }
            cur = sid.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 3. digestAlgorithm AlgorithmIdentifier
        let digest_oid: []const u8 = "";
        if (da := readDerTlv(si_body, cur)) {
            if (da.tag != TAG_SEQUENCE) { _.failed = true; return null; }
            if (oid := parseAlgIdentifierOid(da.value)) { digest_oid = oid; } else { _.failed = true; return null; }
            cur = da.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 4. Optional signedAttrs [0] IMPLICIT
        let has_sa = false;
        if (cur < si_body.len && si_body[cur] == TAG_CTX0_CONS) {
            if (sa := readDerTlv(si_body, cur)) {
                has_sa = true;
                cur = sa.next_pos;
            } else {
                _.failed = true;
                return null;
            }
        }

        // 5. signatureAlgorithm AlgorithmIdentifier
        let sig_oid: []const u8 = "";
        if (sa_alg := readDerTlv(si_body, cur)) {
            if (sa_alg.tag != TAG_SEQUENCE) { _.failed = true; return null; }
            if (oid := parseAlgIdentifierOid(sa_alg.value)) { sig_oid = oid; } else { _.failed = true; return null; }
            cur = sa_alg.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 6. signature OCTET STRING
        let sig_bytes: []const u8 = "";
        if (sv := readDerTlv(si_body, cur)) {
            if (sv.tag != TAG_OCTET_STRING) { _.failed = true; return null; }
            sig_bytes = sv.value;
            cur = sv.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 7. Optional unsignedAttrs [1] IMPLICIT
        if (cur < si_body.len && si_body[cur] == TAG_CTX1_CONS) {
            if (ua := readDerTlv(si_body, cur)) {
                cur = ua.next_pos;
            } else {
                _.failed = true;
                return null;
            }
        }
        if (cur != si_body.len) {
            _.failed = true;
            return null;
        }

        return Pkcs7SignerInfo {
            version: ver,
            digest_alg_oid: digest_oid,
            sig_alg_oid: sig_oid,
            signature: sig_bytes,
            has_signed_attrs: has_sa,
        };
    }
}
