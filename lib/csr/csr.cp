// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 2986 PKCS#10 CSR DER Parser)
// Pure CP zero-heap-allocation RFC 2986 (PKCS#10 v1.7) Certificate Signing Request DER parser:
// strict ASN.1 DER validation, CertificationRequestInfo raw slice for verification,
// SubjectPublicKeyInfo extraction, multi-RDN Subject iteration, and extensionRequest SAN extraction.

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
            if (bytesEqual(attr.oid, oid)) {
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

const TAG_BOOLEAN: u8 = 0x01;
const TAG_INTEGER: u8 = 0x02;
const TAG_BIT_STRING: u8 = 0x03;
const TAG_OCTET_STRING: u8 = 0x04;
const TAG_OID: u8 = 0x06;
const TAG_SEQUENCE: u8 = 0x30;
const TAG_SET: u8 = 0x31;
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

fn decodeDerU64(val: []const u8): u64? {
    if (val.len == 0 || val.len > 9) {
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
    while (i < val.len) {
        acc = (acc << 8) | (val[i] as u64);
        i += 1;
    }
    return acc;
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

/// Validates a DER `BIT STRING` with `unused_bits == 0` and returns the inner payload bytes.
fn stripZeroUnusedBitString(val: []const u8): ([]const u8)? {
    if (val.len < 1 || val[0] != 0) {
        return null;
    }
    return val[1..];
}

// ---------------------------------------------------------------------------
// Public CSR Parsing & Detection API
// ---------------------------------------------------------------------------

/// Parses an RFC 2986 / PKCS#10 DER-encoded Certificate Signing Request.
/// Strictly enforces `version == 0`, `unused_bits == 0` on BIT STRINGs, and zero trailing bytes.
pub fn parseCsrDer(der_data: []const u8): CsrParsed? {
    // Outer CertificationRequest SEQUENCE
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

    // 1. certificationRequestInfo SEQUENCE
    let cri_raw: []const u8 = "";
    let cri_body: []const u8 = "";
    if (cri := readDerTlv(outer_body, pos)) {
        if (cri.tag != TAG_SEQUENCE) {
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

    // 3. signature BIT STRING (unused_bits == 0)
    let signature: []const u8 = "";
    if (sig_tlv := readDerTlv(outer_body, pos)) {
        if (sig_tlv.tag != TAG_BIT_STRING || sig_tlv.next_pos != outer_body.len) {
            return null;
        }
        if (raw := stripZeroUnusedBitString(sig_tlv.value)) {
            signature = raw;
        } else {
            return null;
        }
    } else {
        return null;
    }

    // Parse inside CertificationRequestInfo
    let cpos: usize = 0;

    // 1a. version INTEGER (MUST be 0 per RFC 2986)
    let version: u64 = 0;
    if (vt := readDerTlv(cri_body, cpos)) {
        if (vt.tag != TAG_INTEGER) {
            return null;
        }
        if (v := decodeDerU64(vt.value)) {
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
    if (subj := readDerTlv(cri_body, cpos)) {
        if (subj.tag != TAG_SEQUENCE) {
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
    if (spki := readDerTlv(cri_body, cpos)) {
        if (spki.tag != TAG_SEQUENCE) {
            return null;
        }
        spki_der = spki.raw;
        cpos = spki.next_pos;

        let spos: usize = 0;
        if (palg := readDerTlv(spki.value, spos)) {
            if (palg.tag != TAG_SEQUENCE) {
                return null;
            }
            if (oid := parseAlgIdOid(palg.value)) {
                pub_alg_oid = oid;
            } else {
                return null;
            }
            spos = palg.next_pos;
        } else {
            return null;
        }

        if (pkey := readDerTlv(spki.value, spos)) {
            if (pkey.tag != TAG_BIT_STRING || pkey.next_pos != spki.value.len) {
                return null;
            }
            if (raw := stripZeroUnusedBitString(pkey.value)) {
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
    if (attrs := readDerTlv(cri_body, cpos)) {
        if (attrs.tag != TAG_CTX0_CONS || attrs.next_pos != cri_body.len) {
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

    pub fn init(subject_der: []const u8): CsrSubjectIter {
        let body = subject_der;
        if (subject_der.len > 0 && subject_der[0] == TAG_SEQUENCE) {
            if (seq := readDerTlv(subject_der, 0)) {
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
                if (atv := readDerTlv(_.cur_set, _.set_pos)) {
                    _.set_pos = atv.next_pos;
                    if (atv.tag != TAG_SEQUENCE) {
                        continue;
                    }
                    if (oid_tlv := readDerTlv(atv.value, 0)) {
                        if (oid_tlv.tag != TAG_OID) {
                            continue;
                        }
                        if (val_tlv := readDerTlv(atv.value, oid_tlv.next_pos)) {
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
            if (rdn := readDerTlv(_.name_body, _.name_pos)) {
                _.name_pos = rdn.next_pos;
                if (rdn.tag == TAG_SET) {
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
        if (attr := readDerTlv(attributes_der, apos)) {
            apos = attr.next_pos;
            if (attr.tag != TAG_SEQUENCE) {
                continue;
            }
            if (type_oid := readDerTlv(attr.value, 0)) {
                if (type_oid.tag != TAG_OID || !bytesEqual(type_oid.value, OID_EXTENSION_REQUEST)) {
                    continue;
                }
                if (vals_set := readDerTlv(attr.value, type_oid.next_pos)) {
                    if (vals_set.tag != TAG_SET) {
                        continue;
                    }
                    if (exts_seq := readDerTlv(vals_set.value, 0)) {
                        if (exts_seq.tag != TAG_SEQUENCE) {
                            continue;
                        }
                        let epos: usize = 0;
                        while (epos < exts_seq.value.len) {
                            if (ext := readDerTlv(exts_seq.value, epos)) {
                                epos = ext.next_pos;
                                if (ext.tag != TAG_SEQUENCE) {
                                    continue;
                                }
                                if (eoid := readDerTlv(ext.value, 0)) {
                                    if (eoid.tag != TAG_OID || !bytesEqual(eoid.value, OID_SAN)) {
                                        continue;
                                    }
                                    let fpos = eoid.next_pos;
                                    // Skip optional critical BOOLEAN
                                    if (fpos < ext.value.len && ext.value[fpos] == TAG_BOOLEAN) {
                                        if (crit := readDerTlv(ext.value, fpos)) {
                                            fpos = crit.next_pos;
                                        } else {
                                            break;
                                        }
                                    }
                                    if (oct := readDerTlv(ext.value, fpos)) {
                                        if (oct.tag == TAG_OCTET_STRING) {
                                            if (gn := readDerTlv(oct.value, 0)) {
                                                if (gn.tag == TAG_SEQUENCE) {
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

    pub fn init(attributes_der: []const u8): CsrSanIter {
        return CsrSanIter {
            gn_body: findSanGeneralNamesBody(attributes_der),
            pos: 0,
        };
    }

    fn next(*_): CsrSanEntry? {
        if (_.pos >= _.gn_body.len) {
            return null;
        }
        if (tlv := readDerTlv(_.gn_body, _.pos)) {
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
