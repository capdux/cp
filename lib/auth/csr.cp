// Reference-hardened against RustCrypto/formats (MIT OR Apache-2.0) & RFC 2986/5280/6960
// Origin: Pure-CP Stdlib (Zero-Alloc RFC 2986 PKCS#10 CSR DER Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 2986 (PKCS#10 v1.7) Certificate Signing Request DER parser:
// strict ASN.1 DER validation, CertificationRequestInfo raw slice for verification,
// SubjectPublicKeyInfo extraction, multi-RDN Subject iteration, extensionRequest SAN extraction,
// and 16-lane SIMD byte comparison / payload copy helpers.

/// Module-local 16-lane byte vector alias for PKI extension hot paths.
type PkiExtVec16u8 = @Vector(16, u8);

/// Maximum accepted CSR DER input length (1 MiB) to bound parsing work.
pub const CSR_MAX_DER_LEN: usize = 1024 * 1024;

/// Maximum ASN.1 DER container nesting depth enforced during strict CSR validation.
pub const CSR_MAX_DEPTH: usize = 16;

/// Maximum number of Subject RDN attributes validated per CSR.
pub const CSR_MAX_RDNS: usize = 256;

/// Maximum number of PKCS#10 Attributes validated per CSR.
pub const CSR_MAX_ATTRIBUTES: usize = 256;

/// X.500 `commonName` (`CN`, `2.5.4.3`).
pub const OID_CN: []const u8 = "\x55\x04\x03";

/// X.500 `countryName` (`C`, `2.5.4.6`).
pub const OID_C: []const u8 = "\x55\x04\x06";

/// X.500 `localityName` (`L`, `2.5.4.7`).
pub const OID_L: []const u8 = "\x55\x04\x07";

/// X.500 `stateOrProvinceName` (`ST`, `2.5.4.8`).
pub const OID_ST: []const u8 = "\x55\x04\x08";

/// X.500 `organizationName` (`O`, `2.5.4.10`).
pub const OID_O: []const u8 = "\x55\x04\x0A";

/// X.500 `organizationalUnitName` (`OU`, `2.5.4.11`).
pub const OID_OU: []const u8 = "\x55\x04\x0B";

/// PKCS#9 `extensionRequest` (`1.2.840.113549.1.9.14`).
pub const OID_EXTENSION_REQUEST: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x09\x0E";

/// X.509 `subjectAltName` (`2.5.29.17`).
pub const OID_SAN: []const u8 = "\x55\x1D\x11";

// ---------------------------------------------------------------------------
// 16-Lane SIMD PKI Hot-Path Primitives (0-Diff with Scalar Reference)
// ---------------------------------------------------------------------------

/// Scalar reference: exact byte-slice equality for OIDs, serial numbers, and hashes.
pub fn scalarPkiBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

/// 16-lane SIMD accelerated exact byte-slice equality for OIDs, serial numbers, and hashes.
pub fn simdPkiBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: PkiExtVec16u8 = a[i .. i + 16][0..16].*;
        const vb: PkiExtVec16u8 = b[i .. i + 16][0..16].*;
        const diff = va ^ vb;
        if (@reduce(.Or, diff) != 0) {
            return false;
        }
        i += 16;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

/// Scalar reference: copy DER payload bytes from `src` into `dst`.
pub fn scalarPkiCopyBytes(dst: []u8, src: []const u8) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane SIMD accelerated DER payload copy from `src` into `dst`.
pub fn simdPkiCopyBytes(dst: []u8, src: []const u8) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v: PkiExtVec16u8 = src[i .. i + 16][0..16].*;
        dst[i .. i + 16][0..16].* = v;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Zero-copy X.500 Subject RDN attribute (`type OID` + raw string `value`).
pub struct CsrRdnAttr {
    oid: []const u8 = "",
    value: []const u8 = "",
}

/// Zero-copy Subject Alternative Name `GeneralName` entry.
/// Common `tag_num` values: `1` = rfc822Name, `2` = dNSName, `6` = uniformResourceIdentifier, `7` = iPAddress.
pub struct CsrSanEntry {
    tag_num: u8 = 0,
    value: []const u8 = "",
}

/// Zero-copy decoded RFC 2986 PKCS#10 Certificate Signing Request.
pub struct CsrParsed {
    cri_raw_der: []const u8 = "",
    version: u64 = 0,
    subject_der: []const u8 = "",
    spki_der: []const u8 = "",
    pub_alg_oid: []const u8 = "",
    pub_key_bytes: []const u8 = "",
    attributes_der: []const u8 = "",
    sig_alg_oid: []const u8 = "",
    signature: []const u8 = "",

    /// Returns a zero-allocation iterator over all Subject RDN attributes.
    fn subjectAttrs(): CsrSubjectIter {
        return CsrSubjectIter.init(_.subject_der);
    }

    /// Finds the first Subject RDN attribute matching `oid`.
    fn findSubjectAttr(oid: []const u8): ([]const u8)? {
        let it = _.subjectAttrs();
        while (attr := it.next()) {
            if (simdPkiBytesEqual(attr.oid, oid)) {
                return attr.value;
            }
        }
        return null;
    }

    /// Returns Subject Common Name (`CN`, `2.5.4.3`), if present.
    fn commonName(): ([]const u8)? {
        return _.findSubjectAttr(OID_CN);
    }

    /// Returns Subject Organization (`O`, `2.5.4.10`), if present.
    fn organization(): ([]const u8)? {
        return _.findSubjectAttr(OID_O);
    }

    /// Returns Subject Country (`C`, `2.5.4.6`), if present.
    fn country(): ([]const u8)? {
        return _.findSubjectAttr(OID_C);
    }

    /// Returns a zero-allocation iterator over SAN `GeneralName` entries in `extensionRequest`.
    fn sans(): CsrSanIter {
        return CsrSanIter.init(_.attributes_der);
    }
}

const CSR_TAG_BOOLEAN: u8 = 0x01;
const CSR_TAG_INTEGER: u8 = 0x02;
const CSR_TAG_BIT_STRING: u8 = 0x03;
const CSR_TAG_OCTET_STRING: u8 = 0x04;
const CSR_TAG_OID: u8 = 0x06;
const CSR_TAG_SEQUENCE: u8 = 0x30;
const CSR_TAG_SET: u8 = 0x31;
const CSR_TAG_CTX0_CONS: u8 = 0xA0;

struct CsrDerTlv {
    tag: u8,
    raw: []const u8,
    value: []const u8,
    next_pos: usize,
}

// ---------------------------------------------------------------------------
// Internal Strict ASN.1 DER Helpers (RustCrypto/formats der compliant)
// ---------------------------------------------------------------------------

fn csrReadDerTlv(data: []const u8, pos: usize): CsrDerTlv? {
    if (pos + 2 > data.len) {
        return null;
    }
    const tag = data[pos];
    // Reject high-tag-number form (low 5 bits all 1s) and EOC tag 0x00.
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
        // Reject non-canonical leading 0x00 length octet.
        if (data[val_start] == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < num_octets) : (i += 1) {
            val_len = (val_len << 8) | (data[val_start + i] as usize);
        }
        // Reject non-canonical long form for length < 128.
        if (val_len < 128) {
            return null;
        }
        val_start += num_octets;
    }

    if (val_len > data.len - val_start) {
        return null;
    }
    const end = val_start + val_len;
    return CsrDerTlv {
        tag: tag,
        raw: data[pos..end],
        value: data[val_start..end],
        next_pos: end,
    };
}

/// Validates X.690 DER canonical minimal INTEGER encoding.
fn csrIsValidDerInteger(val: []const u8): bool {
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

/// Validates X.690 DER canonical BOOLEAN encoding (`0x00` or `0xFF`).
fn csrIsValidDerBoolean(val: []const u8): bool {
    return val.len == 1 && (val[0] == 0x00 || val[0] == 0xFF);
}

/// Validates X.690 DER OID encoding (non-empty, canonical VLQ arcs without leading 0x80).
fn csrIsValidDerOid(val: []const u8): bool {
    if (val.len == 0) {
        return false;
    }
    if ((val[val.len - 1] & 0x80) != 0) {
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

fn csrDecodeDerU64(val: []const u8): u64? {
    if (!csrIsValidDerInteger(val) || val.len > 9) {
        return null;
    }
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
    while (i < val.len) : (i += 1) {
        acc = (acc << 8) | (val[i] as u64);
    }
    return acc;
}

/// Validates an AlgorithmIdentifier SEQUENCE body (`{ algorithm OID, parameters ANY OPTIONAL }`).
fn csrParseAlgIdOid(seq_body: []const u8): ([]const u8)? {
    if (oid_tlv := csrReadDerTlv(seq_body, 0)) {
        if (oid_tlv.tag != CSR_TAG_OID || !csrIsValidDerOid(oid_tlv.value)) {
            return null;
        }
        if (oid_tlv.next_pos < seq_body.len) {
            // Optional single parameters TLV; reject trailing garbage after parameters.
            if (param := csrReadDerTlv(seq_body, oid_tlv.next_pos)) {
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

/// Validates a DER `BIT STRING` with `unused_bits == 0` and non-empty payload.
fn csrStripZeroUnusedBitString(val: []const u8): ([]const u8)? {
    if (val.len < 2 || val[0] != 0) {
        return null;
    }
    return val[1..];
}

/// Recursively validates ASN.1 DER structural integrity and nesting depth up to `max_depth`.
fn csrValidateDerTree(slice: []const u8, depth: usize, max_depth: usize): bool {
    if (depth > max_depth) {
        return false;
    }
    let pos: usize = 0;
    while (pos < slice.len) {
        if (tlv := csrReadDerTlv(slice, pos)) {
            pos = tlv.next_pos;
            if (tlv.tag == CSR_TAG_BOOLEAN && !csrIsValidDerBoolean(tlv.value)) {
                return false;
            }
            if (tlv.tag == CSR_TAG_INTEGER && !csrIsValidDerInteger(tlv.value)) {
                return false;
            }
            if (tlv.tag == CSR_TAG_OID && !csrIsValidDerOid(tlv.value)) {
                return false;
            }
            if (tlv.tag == CSR_TAG_BIT_STRING) {
                if (tlv.value.len == 0 || tlv.value[0] > 7) {
                    return false;
                }
                if (tlv.value.len == 1 && tlv.value[0] != 0) {
                    return false;
                }
                if (tlv.value.len > 1 && tlv.value[0] > 0) {
                    const mask: u8 = ((1 as u8) << (tlv.value[0] as u3)) - 1;
                    if ((tlv.value[tlv.value.len - 1] & mask) != 0) {
                        return false;
                    }
                }
            }
            // Constructed tag (bit 5 set): recursively validate children.
            if ((tlv.tag & 0x20) != 0) {
                if (!csrValidateDerTree(tlv.value, depth + 1, max_depth)) {
                    return false;
                }
            }
        } else {
            return false;
        }
    }
    return pos == slice.len;
}

/// Strictly validates X.500 `Name` SEQUENCE (`SEQUENCE OF SET OF SEQUENCE { OID, value }`).
fn validateSubjectNameDer(subject_der: []const u8): bool {
    if (seq := csrReadDerTlv(subject_der, 0)) {
        if (seq.tag != CSR_TAG_SEQUENCE || seq.next_pos != subject_der.len) {
            return false;
        }
        let pos: usize = 0;
        let rdn_count: usize = 0;
        while (pos < seq.value.len) {
            if (rdn := csrReadDerTlv(seq.value, pos)) {
                if (rdn.tag != CSR_TAG_SET || rdn.value.len == 0) {
                    return false;
                }
                pos = rdn.next_pos;
                let spos: usize = 0;
                while (spos < rdn.value.len) {
                    rdn_count += 1;
                    if (rdn_count > CSR_MAX_RDNS) {
                        return false;
                    }
                    if (atv := csrReadDerTlv(rdn.value, spos)) {
                        if (atv.tag != CSR_TAG_SEQUENCE) {
                            return false;
                        }
                        spos = atv.next_pos;
                        if (oid_tlv := csrReadDerTlv(atv.value, 0)) {
                            if (oid_tlv.tag != CSR_TAG_OID || !csrIsValidDerOid(oid_tlv.value)) {
                                return false;
                            }
                            if (val_tlv := csrReadDerTlv(atv.value, oid_tlv.next_pos)) {
                                if (val_tlv.next_pos != atv.value.len) {
                                    return false;
                                }
                            } else {
                                return false;
                            }
                        } else {
                            return false;
                        }
                    } else {
                        return false;
                    }
                }
            } else {
                return false;
            }
        }
        return true;
    }
    return false;
}

/// Strictly validates PKCS#10 `[0] IMPLICIT Attributes` (`SET OF SEQUENCE { OID, SET OF ANY }`).
fn validateCsrAttributesDer(attributes_der: []const u8): bool {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < attributes_der.len) {
        count += 1;
        if (count > CSR_MAX_ATTRIBUTES) {
            return false;
        }
        if (attr := csrReadDerTlv(attributes_der, pos)) {
            if (attr.tag != CSR_TAG_SEQUENCE) {
                return false;
            }
            pos = attr.next_pos;
            if (oid_tlv := csrReadDerTlv(attr.value, 0)) {
                if (oid_tlv.tag != CSR_TAG_OID || !csrIsValidDerOid(oid_tlv.value)) {
                    return false;
                }
                if (vals := csrReadDerTlv(attr.value, oid_tlv.next_pos)) {
                    if (vals.tag != CSR_TAG_SET || vals.next_pos != attr.value.len) {
                        return false;
                    }
                    if (!csrValidateDerTree(vals.value, 3, CSR_MAX_DEPTH)) {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Public CSR Parsing & Detection API
// ---------------------------------------------------------------------------

/// Parses an RFC 2986 / PKCS#10 DER-encoded Certificate Signing Request.
/// Strictly enforces canonical DER lengths, `version == 0`, `unused_bits == 0` on BIT STRINGs,
/// complete Subject/SPKI/Attributes/Signature structures, and zero trailing bytes.
pub fn parseCsrDer(der_data: []const u8): CsrParsed? {
    if (der_data.len == 0 || der_data.len > CSR_MAX_DER_LEN) {
        return null;
    }

    // Outer CertificationRequest SEQUENCE
    let outer_body: []const u8 = "";
    if (outer := csrReadDerTlv(der_data, 0)) {
        if (outer.tag != CSR_TAG_SEQUENCE || outer.next_pos != der_data.len) {
            return null;
        }
        outer_body = outer.value;
    } else {
        return null;
    }

    let pos: usize = 0;

    // 1. certificationRequestInfo SEQUENCE
    let cri_raw: []const u8 = "";
    let cri_body: []const u8 = "";
    if (cri := csrReadDerTlv(outer_body, pos)) {
        if (cri.tag != CSR_TAG_SEQUENCE) {
            return null;
        }
        cri_raw = cri.raw;
        cri_body = cri.value;
        pos = cri.next_pos;
    } else {
        return null;
    }

    // 2. signatureAlgorithm AlgorithmIdentifier SEQUENCE
    let sig_alg_oid: []const u8 = "";
    if (sa := csrReadDerTlv(outer_body, pos)) {
        if (sa.tag != CSR_TAG_SEQUENCE) {
            return null;
        }
        if (oid := csrParseAlgIdOid(sa.value)) {
            sig_alg_oid = oid;
        } else {
            return null;
        }
        pos = sa.next_pos;
    } else {
        return null;
    }

    // 3. signature BIT STRING (unused_bits == 0, non-empty)
    let signature: []const u8 = "";
    if (sig_tlv := csrReadDerTlv(outer_body, pos)) {
        if (sig_tlv.tag != CSR_TAG_BIT_STRING || sig_tlv.next_pos != outer_body.len) {
            return null;
        }
        if (raw := csrStripZeroUnusedBitString(sig_tlv.value)) {
            signature = raw;
        } else {
            return null;
        }
    } else {
        return null;
    }

    // Parse inside CertificationRequestInfo
    let cpos: usize = 0;

    // 1a. version INTEGER (MUST be v1(0) per RFC 2986, canonical DER)
    let version: u64 = 0;
    if (vt := csrReadDerTlv(cri_body, cpos)) {
        if (vt.tag != CSR_TAG_INTEGER) {
            return null;
        }
        if (v := csrDecodeDerU64(vt.value)) {
            if (v != 0) {
                return null;
            }
            version = v;
        } else {
            return null;
        }
        cpos = vt.next_pos;
    } else {
        return null;
    }

    // 1b. subject Name SEQUENCE
    let subject_der: []const u8 = "";
    if (subj := csrReadDerTlv(cri_body, cpos)) {
        if (subj.tag != CSR_TAG_SEQUENCE) {
            return null;
        }
        subject_der = subj.raw;
        cpos = subj.next_pos;
    } else {
        return null;
    }

    // 1c. subjectPKInfo SubjectPublicKeyInfo SEQUENCE
    let spki_der: []const u8 = "";
    let pub_alg_oid: []const u8 = "";
    let pub_key_bytes: []const u8 = "";
    if (spki := csrReadDerTlv(cri_body, cpos)) {
        if (spki.tag != CSR_TAG_SEQUENCE) {
            return null;
        }
        spki_der = spki.raw;
        cpos = spki.next_pos;

        let spos: usize = 0;
        if (palg := csrReadDerTlv(spki.value, spos)) {
            if (palg.tag != CSR_TAG_SEQUENCE) {
                return null;
            }
            if (oid := csrParseAlgIdOid(palg.value)) {
                pub_alg_oid = oid;
            } else {
                return null;
            }
            spos = palg.next_pos;
        } else {
            return null;
        }

        if (pkey := csrReadDerTlv(spki.value, spos)) {
            if (pkey.tag != CSR_TAG_BIT_STRING || pkey.next_pos != spki.value.len) {
                return null;
            }
            if (raw := csrStripZeroUnusedBitString(pkey.value)) {
                pub_key_bytes = raw;
            } else {
                return null;
            }
        } else {
            return null;
        }
    } else {
        return null;
    }

    // 1d. attributes [0] IMPLICIT Attributes
    let attributes_der: []const u8 = "";
    if (attrs := csrReadDerTlv(cri_body, cpos)) {
        if (attrs.tag != CSR_TAG_CTX0_CONS || attrs.next_pos != cri_body.len) {
            return null;
        }
        attributes_der = attrs.value;
    } else {
        return null;
    }

    return CsrParsed {
        cri_raw_der: cri_raw,
        version: version,
        subject_der: subject_der,
        spki_der: spki_der,
        pub_alg_oid: pub_alg_oid,
        pub_key_bytes: pub_key_bytes,
        attributes_der: attributes_der,
        sig_alg_oid: sig_alg_oid,
        signature: signature,
    };
}

/// Zero-heap strict RFC 2986 / PKCS#10 DER validator.
/// Enforces full DER tree depth limits (`CSR_MAX_DEPTH`), canonical lengths/booleans/integers/OIDs,
/// `version == 0`, valid Subject RDN & Attributes structures, and zero trailing garbage bytes.
pub fn validateCsrStrict(der_data: []const u8): bool {
    if (der_data.len == 0 || der_data.len > CSR_MAX_DER_LEN) {
        return false;
    }
    if (!csrValidateDerTree(der_data, 0, CSR_MAX_DEPTH)) {
        return false;
    }
    if (csr := parseCsrDer(der_data)) {
        if (!validateSubjectNameDer(csr.subject_der)) {
            return false;
        }
        if (!validateCsrAttributesDer(csr.attributes_der)) {
            return false;
        }
        return true;
    }
    return false;
}

/// Returns `true` if `der_data` is a strictly valid RFC 2986 PKCS#10 DER CSR.
pub fn isCsrDer(der_data: []const u8): bool {
    if (csr := parseCsrDer(der_data)) {
        * = csr;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Zero-Alloc Subject RDN & SAN Extension Iterators
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over all `SET -> SEQUENCE { oid, value }` entries in `subject_der`.
pub struct CsrSubjectIter {
    name_body: []const u8 = "",
    name_pos: usize = 0,
    cur_set: []const u8 = "",
    set_pos: usize = 0,

    pub static fn init(subject_der: []const u8): CsrSubjectIter {
        let body = subject_der;
        if (subject_der.len > 0 && subject_der[0] == CSR_TAG_SEQUENCE) {
            if (seq := csrReadDerTlv(subject_der, 0)) {
                if (seq.next_pos == subject_der.len) {
                    body = seq.value;
                }
            }
        }
        return CsrSubjectIter {
            name_body: body,
            name_pos: 0,
            cur_set: "",
            set_pos: 0,
        };
    }

    fn next(*_): CsrRdnAttr? {
        while (true) {
            if (_.set_pos < _.cur_set.len) {
                if (atv := csrReadDerTlv(_.cur_set, _.set_pos)) {
                    _.set_pos = atv.next_pos;
                    if (atv.tag != CSR_TAG_SEQUENCE) {
                        continue;
                    }
                    if (oid_tlv := csrReadDerTlv(atv.value, 0)) {
                        if (oid_tlv.tag != CSR_TAG_OID) {
                            continue;
                        }
                        if (val_tlv := csrReadDerTlv(atv.value, oid_tlv.next_pos)) {
                            return CsrRdnAttr {
                                oid: oid_tlv.value,
                                value: val_tlv.value,
                            };
                        }
                    }
                    continue;
                } else {
                    _.set_pos = _.cur_set.len;
                }
            }

            if (_.name_pos >= _.name_body.len) {
                return null;
            }
            if (rdn := csrReadDerTlv(_.name_body, _.name_pos)) {
                _.name_pos = rdn.next_pos;
                if (rdn.tag == CSR_TAG_SET) {
                    _.cur_set = rdn.value;
                    _.set_pos = 0;
                }
            } else {
                _.name_pos = _.name_body.len;
                return null;
            }
        }
    }
}

/// Locates `OID_EXTENSION_REQUEST` -> `OID_SAN` -> `GeneralNames SEQUENCE` body inside `attributes_der`.
fn findSanGeneralNamesBody(attributes_der: []const u8): []const u8 {
    let apos: usize = 0;
    while (apos < attributes_der.len) {
        if (attr := csrReadDerTlv(attributes_der, apos)) {
            apos = attr.next_pos;
            if (attr.tag != CSR_TAG_SEQUENCE) {
                continue;
            }
            if (type_oid := csrReadDerTlv(attr.value, 0)) {
                if (type_oid.tag != CSR_TAG_OID || !simdPkiBytesEqual(type_oid.value, OID_EXTENSION_REQUEST)) {
                    continue;
                }
                if (vals_set := csrReadDerTlv(attr.value, type_oid.next_pos)) {
                    if (vals_set.tag != CSR_TAG_SET) {
                        continue;
                    }
                    if (exts_seq := csrReadDerTlv(vals_set.value, 0)) {
                        if (exts_seq.tag != CSR_TAG_SEQUENCE) {
                            continue;
                        }
                        let epos: usize = 0;
                        while (epos < exts_seq.value.len) {
                            if (ext := csrReadDerTlv(exts_seq.value, epos)) {
                                epos = ext.next_pos;
                                if (ext.tag != CSR_TAG_SEQUENCE) {
                                    continue;
                                }
                                if (eoid := csrReadDerTlv(ext.value, 0)) {
                                    if (eoid.tag != CSR_TAG_OID || !simdPkiBytesEqual(eoid.value, OID_SAN)) {
                                        continue;
                                    }
                                    let fpos = eoid.next_pos;
                                    // Skip optional critical BOOLEAN
                                    if (fpos < ext.value.len && ext.value[fpos] == CSR_TAG_BOOLEAN) {
                                        if (crit := csrReadDerTlv(ext.value, fpos)) {
                                            if (!csrIsValidDerBoolean(crit.value)) {
                                                break;
                                            }
                                            fpos = crit.next_pos;
                                        } else {
                                            break;
                                        }
                                    }
                                    if (oct := csrReadDerTlv(ext.value, fpos)) {
                                        if (oct.tag == CSR_TAG_OCTET_STRING) {
                                            if (gn := csrReadDerTlv(oct.value, 0)) {
                                                if (gn.tag == CSR_TAG_SEQUENCE && gn.next_pos == oct.value.len) {
                                                    return gn.value;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            break;
        }
    }
    return "";
}

/// Zero-allocation iterator over Subject Alternative Name (`GeneralNames`) entries extracted
/// from PKCS#9 `extensionRequest` inside `attributes_der`.
pub struct CsrSanIter {
    gn_body: []const u8 = "",
    pos: usize = 0,

    pub static fn init(attributes_der: []const u8): CsrSanIter {
        return CsrSanIter {
            gn_body: findSanGeneralNamesBody(attributes_der),
            pos: 0,
        };
    }

    fn next(*_): CsrSanEntry? {
        if (_.pos >= _.gn_body.len) {
            return null;
        }
        if (tlv := csrReadDerTlv(_.gn_body, _.pos)) {
            _.pos = tlv.next_pos;
            return CsrSanEntry {
                tag_num: tlv.tag & 0x1F,
                value: tlv.value,
            };
        }
        _.pos = _.gn_body.len;
        return null;
    }
}
