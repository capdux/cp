// Origin: Pure-CP Stdlib (Zero-Alloc RFC 7292 PKCS#12 PFX Container DER Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 7292 PKCS#12 (.p12/.pfx) ASN.1 DER parser:
// PFX v3 envelope, MacData (DigestInfo + salt + iterations default 1),
// AuthenticatedSafe ContentInfo iteration, and SafeBag + PKCS12Attribute
// (localKeyId, friendlyName BMPString) zero-copy iteration.

/// PKCS#7 `id-data` OID (`1.2.840.113549.1.7.1`).
pub const OID_DATA: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x07\x01";

/// PKCS#7 `id-encryptedData` OID (`1.2.840.113549.1.7.6`).
pub const OID_ENCRYPTED_DATA: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x07\x06";

/// PKCS#12 `keyBag` OID (`1.2.840.113549.1.12.10.1.1`).
pub const OID_KEY_BAG: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x0A\x01\x01";

/// PKCS#12 `pkcs8ShroudedKeyBag` OID (`1.2.840.113549.1.12.10.1.2`).
pub const OID_SHROUDED_KEY_BAG: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x0A\x01\x02";

/// PKCS#12 `certBag` OID (`1.2.840.113549.1.12.10.1.3`).
pub const OID_CERT_BAG: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x0A\x01\x03";

/// PKCS#12 `crlBag` OID (`1.2.840.113549.1.12.10.1.4`).
pub const OID_CRL_BAG: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x0A\x01\x04";

/// PKCS#12 `secretBag` OID (`1.2.840.113549.1.12.10.1.5`).
pub const OID_SECRET_BAG: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x0A\x01\x05";

/// PKCS#9 `friendlyName` attribute OID (`1.2.840.113549.1.9.20`).
pub const OID_FRIENDLY_NAME: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x09\x14";

/// PKCS#9 `localKeyId` attribute OID (`1.2.840.113549.1.9.21`).
pub const OID_LOCAL_KEY_ID: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x09\x15";

/// Recognized RFC 7292 `SafeBag.bagId` classifications.
pub enum Pkcs12BagKind: u8 {
    key_bag = 0,
    shrouded_key_bag = 1,
    cert_bag = 2,
    crl_bag = 3,
    secret_bag = 4,
    unknown = 255,
}

/// Zero-copy decoded RFC 7292 `MacData` structure.
/// Per RFC 7292 Section 4, `iterations` defaults to `1` when omitted.
pub struct Pkcs12MacData {
    digest_alg_oid: []const u8 = "",
    digest: []const u8 = "",
    mac_salt: []const u8 = "",
    iterations: u64 = 1,
    present: bool = false,
}

/// Zero-copy decoded RFC 7292 `PFX` container (version 3).
pub struct Pkcs12Pfx {
    version: u64 = 3,
    auth_safe_type_oid: []const u8 = "",
    auth_safe_data: []const u8 = "",
    mac_data: Pkcs12MacData = Pkcs12MacData {},

    /// Returns a zero-allocation iterator over `AuthenticatedSafe` (`SEQUENCE OF ContentInfo`).
    fn safeContents(): Pkcs12SafeIter {
        return Pkcs12SafeIter.init(_.auth_safe_data);
    }
}

/// Zero-copy decoded `ContentInfo` entry inside `AuthenticatedSafe`.
pub struct Pkcs12SafeContent {
    content_type_oid: []const u8 = "",
    is_encrypted: bool = false,
    content_der: []const u8 = "",

    /// Returns a zero-allocation iterator over `SafeContents` (`SEQUENCE OF SafeBag`)
    /// for plaintext `id-data` entries.
    fn bags(): Pkcs12BagIter {
        return Pkcs12BagIter.init(_.content_der);
    }
}

/// Zero-copy decoded RFC 7292 `SafeBag` entry with extracted PKCS#9 attributes.
pub struct Pkcs12Bag {
    bag_kind: Pkcs12BagKind = Pkcs12BagKind.unknown,
    bag_id_oid: []const u8 = "",
    bag_value_der: []const u8 = "",
    local_key_id: []const u8 = "",
    friendly_name_bmp: []const u8 = "",
}

const TAG_INTEGER: u8 = 0x02;
const TAG_OCTET_STRING: u8 = 0x04;
const TAG_OID: u8 = 0x06;
const TAG_BMP_STRING: u8 = 0x1E;
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

fn classifyBagOid(oid: []const u8): Pkcs12BagKind {
    if (bytesEqual(oid, OID_KEY_BAG)) {
        return Pkcs12BagKind.key_bag;
    }
    if (bytesEqual(oid, OID_SHROUDED_KEY_BAG)) {
        return Pkcs12BagKind.shrouded_key_bag;
    }
    if (bytesEqual(oid, OID_CERT_BAG)) {
        return Pkcs12BagKind.cert_bag;
    }
    if (bytesEqual(oid, OID_CRL_BAG)) {
        return Pkcs12BagKind.crl_bag;
    }
    if (bytesEqual(oid, OID_SECRET_BAG)) {
        return Pkcs12BagKind.secret_bag;
    }
    return Pkcs12BagKind.unknown;
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

/// Decodes a non-negative canonical ASN.1 DER `INTEGER` payload into `u64`.
fn decodeDerU64(val: []const u8): u64? {
    if (val.len == 0 || val.len > 9) {
        return null;
    }
    // Reject negative integers (MSB set)
    if ((val[0] & 0x80) != 0) {
        return null;
    }
    // Strict DER: reject non-minimal leading zero byte
    if (val.len >= 2 && val[0] == 0 && (val[1] & 0x80) == 0) {
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

/// Parses RFC 7292 `MacData ::= SEQUENCE { mac DigestInfo, macSalt OCTET STRING, iterations INTEGER DEFAULT 1 }`.
fn parseMacDataBody(mac_body: []const u8): Pkcs12MacData? {
    let pos: usize = 0;

    // 1. mac DigestInfo ::= SEQUENCE { digestAlgorithm AlgorithmIdentifier, digest OCTET STRING }
    let digest_alg_oid: []const u8 = "";
    let digest: []const u8 = "";
    if (di_tlv := readDerTlv(mac_body, pos)) {
        if (di_tlv.tag != TAG_SEQUENCE) {
            return null;
        }
        pos = di_tlv.next_pos;

        let di_pos: usize = 0;
        if (alg_tlv := readDerTlv(di_tlv.value, di_pos)) {
            if (alg_tlv.tag != TAG_SEQUENCE) {
                return null;
            }
            if (oid := parseAlgIdentifierOid(alg_tlv.value)) {
                digest_alg_oid = oid;
            } else {
                return null;
            }
            di_pos = alg_tlv.next_pos;
        } else {
            return null;
        }

        if (dig_tlv := readDerTlv(di_tlv.value, di_pos)) {
            if (dig_tlv.tag != TAG_OCTET_STRING || dig_tlv.next_pos != di_tlv.value.len) {
                return null;
            }
            digest = dig_tlv.value;
        } else {
            return null;
        }
    } else {
        return null;
    }

    // 2. macSalt OCTET STRING
    let mac_salt: []const u8 = "";
    if (salt_tlv := readDerTlv(mac_body, pos)) {
        if (salt_tlv.tag != TAG_OCTET_STRING) {
            return null;
        }
        mac_salt = salt_tlv.value;
        pos = salt_tlv.next_pos;
    } else {
        return null;
    }

    // 3. Optional iterations INTEGER DEFAULT 1
    let iterations: u64 = 1;
    if (pos < mac_body.len) {
        if (iter_tlv := readDerTlv(mac_body, pos)) {
            if (iter_tlv.tag != TAG_INTEGER || iter_tlv.next_pos != mac_body.len) {
                return null;
            }
            if (it := decodeDerU64(iter_tlv.value)) {
                iterations = it;
            } else {
                return null;
            }
        } else {
            return null;
        }
    }

    return Pkcs12MacData {
        digest_alg_oid: digest_alg_oid,
        digest: digest,
        mac_salt: mac_salt,
        iterations: iterations,
        present: true,
    };
}

// ---------------------------------------------------------------------------
// Public PFX Parsing API
// ---------------------------------------------------------------------------

/// Strictly parses an RFC 7292 DER-encoded `PFX` container (`.p12` / `.pfx`).
/// Validates `version == 3`, unwraps `[0] EXPLICIT OCTET STRING` when `authSafe.contentType == OID_DATA`,
/// and decodes optional `MacData` (defaulting `iterations` to `1` when omitted).
pub fn parsePkcs12Der(der_data: []const u8): Pkcs12Pfx? {
    // Outer PFX SEQUENCE
    let pfx_body: []const u8 = "";
    if (outer := readDerTlv(der_data, 0)) {
        if (outer.tag != TAG_SEQUENCE || outer.next_pos != der_data.len) {
            return null;
        }
        pfx_body = outer.value;
    } else {
        return null;
    }

    let pos: usize = 0;

    // 1. version INTEGER { v3(3) }
    let version: u64 = 0;
    if (ver_tlv := readDerTlv(pfx_body, pos)) {
        if (ver_tlv.tag != TAG_INTEGER) {
            return null;
        }
        if (v := decodeDerU64(ver_tlv.value)) {
            if (v != 3) {
                return null;
            }
            version = v;
        } else {
            return null;
        }
        pos = ver_tlv.next_pos;
    } else {
        return null;
    }

    // 2. authSafe ContentInfo ::= SEQUENCE { contentType OID, content [0] EXPLICIT OPTIONAL }
    let auth_safe_oid: []const u8 = "";
    let auth_safe_data: []const u8 = "";
    if (ci_tlv := readDerTlv(pfx_body, pos)) {
        if (ci_tlv.tag != TAG_SEQUENCE) {
            return null;
        }
        pos = ci_tlv.next_pos;

        let ci_pos: usize = 0;
        if (oid_tlv := readDerTlv(ci_tlv.value, ci_pos)) {
            if (oid_tlv.tag != TAG_OID || oid_tlv.value.len == 0) {
                return null;
            }
            auth_safe_oid = oid_tlv.value;
            ci_pos = oid_tlv.next_pos;
        } else {
            return null;
        }

        if (ci_pos < ci_tlv.value.len) {
            if (ctx0 := readDerTlv(ci_tlv.value, ci_pos)) {
                if (ctx0.tag != TAG_CTX0_CONS || ctx0.next_pos != ci_tlv.value.len) {
                    return null;
                }
                if (bytesEqual(auth_safe_oid, OID_DATA)) {
                    // Strip inner OCTET STRING to expose AuthenticatedSafe DER directly
                    if (oct := readDerTlv(ctx0.value, 0)) {
                        if (oct.tag != TAG_OCTET_STRING || oct.next_pos != ctx0.value.len) {
                            return null;
                        }
                        auth_safe_data = oct.value;
                    } else {
                        return null;
                    }
                } else {
                    auth_safe_data = ctx0.value;
                }
            } else {
                return null;
            }
        }
    } else {
        return null;
    }

    // 3. Optional macData MacData
    let mac_data = Pkcs12MacData {};
    if (pos < pfx_body.len) {
        if (mac_tlv := readDerTlv(pfx_body, pos)) {
            if (mac_tlv.tag != TAG_SEQUENCE || mac_tlv.next_pos != pfx_body.len) {
                return null;
            }
            if (md := parseMacDataBody(mac_tlv.value)) {
                mac_data = md;
            } else {
                return null;
            }
        } else {
            return null;
        }
    }

    return Pkcs12Pfx {
        version: version,
        auth_safe_type_oid: auth_safe_oid,
        auth_safe_data: auth_safe_data,
        mac_data: mac_data,
    };
}

/// Returns `true` if `der_data` is a valid RFC 7292 PKCS#12 v3 `PFX` DER container.
pub fn isPkcs12Der(der_data: []const u8): bool {
    if (pfx := parsePkcs12Der(der_data)) {
        * = pfx;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Zero-Alloc AuthenticatedSafe & SafeContents Iterators
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over `AuthenticatedSafe ::= SEQUENCE OF ContentInfo`.
pub struct Pkcs12SafeIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    /// Initializes the iterator from the DER-encoded `AuthenticatedSafe` `SEQUENCE`.
    pub static fn init(auth_safe_der: []const u8): Pkcs12SafeIter {
        if (auth_safe_der.len == 0) {
            return Pkcs12SafeIter {
                data: "",
                pos: 0,
                failed: false,
            };
        }
        if (seq := readDerTlv(auth_safe_der, 0)) {
            if (seq.tag != TAG_SEQUENCE || seq.next_pos != auth_safe_der.len) {
                return Pkcs12SafeIter {
                    data: "",
                    pos: 0,
                    failed: true,
                };
            }
            return Pkcs12SafeIter {
                data: seq.value,
                pos: 0,
                failed: false,
            };
        }
        return Pkcs12SafeIter {
            data: "",
            pos: 0,
            failed: true,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    /// Yields the next `Pkcs12SafeContent` entry, or `null` at EOF or on malformed DER.
    fn next(*_): Pkcs12SafeContent? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        let ci_body: []const u8 = "";
        if (ci_tlv := readDerTlv(_.data, _.pos)) {
            if (ci_tlv.tag != TAG_SEQUENCE) {
                _.failed = true;
                return null;
            }
            ci_body = ci_tlv.value;
            _.pos = ci_tlv.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        let cur: usize = 0;
        let oid_bytes: []const u8 = "";
        if (oid_tlv := readDerTlv(ci_body, cur)) {
            if (oid_tlv.tag != TAG_OID || oid_tlv.value.len == 0) {
                _.failed = true;
                return null;
            }
            oid_bytes = oid_tlv.value;
            cur = oid_tlv.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        let content_der: []const u8 = "";
        const is_data = bytesEqual(oid_bytes, OID_DATA);
        const is_encrypted = bytesEqual(oid_bytes, OID_ENCRYPTED_DATA);

        if (cur < ci_body.len) {
            if (ctx0 := readDerTlv(ci_body, cur)) {
                if (ctx0.tag != TAG_CTX0_CONS || ctx0.next_pos != ci_body.len) {
                    _.failed = true;
                    return null;
                }
                if (is_data) {
                    // Strip OCTET STRING wrapper so content_der is directly SafeContents DER
                    if (oct := readDerTlv(ctx0.value, 0)) {
                        if (oct.tag != TAG_OCTET_STRING || oct.next_pos != ctx0.value.len) {
                            _.failed = true;
                            return null;
                        }
                        content_der = oct.value;
                    } else {
                        _.failed = true;
                        return null;
                    }
                } else {
                    content_der = ctx0.value;
                }
            } else {
                _.failed = true;
                return null;
            }
        }

        return Pkcs12SafeContent {
            content_type_oid: oid_bytes,
            is_encrypted: is_encrypted,
            content_der: content_der,
        };
    }
}

/// Zero-allocation iterator over plaintext `SafeContents ::= SEQUENCE OF SafeBag`.
pub struct Pkcs12BagIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    /// Initializes the iterator from the DER-encoded `SafeContents` `SEQUENCE`.
    pub static fn init(safe_contents_der: []const u8): Pkcs12BagIter {
        if (safe_contents_der.len == 0) {
            return Pkcs12BagIter {
                data: "",
                pos: 0,
                failed: false,
            };
        }
        if (seq := readDerTlv(safe_contents_der, 0)) {
            if (seq.tag != TAG_SEQUENCE || seq.next_pos != safe_contents_der.len) {
                return Pkcs12BagIter {
                    data: "",
                    pos: 0,
                    failed: true,
                };
            }
            return Pkcs12BagIter {
                data: seq.value,
                pos: 0,
                failed: false,
            };
        }
        return Pkcs12BagIter {
            data: "",
            pos: 0,
            failed: true,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    /// Yields the next `Pkcs12Bag` with unwrapped `bagValue` and extracted `localKeyId`/`friendlyName`,
    /// or `null` at EOF or on malformed DER.
    fn next(*_): Pkcs12Bag? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        let bag_body: []const u8 = "";
        if (bag_tlv := readDerTlv(_.data, _.pos)) {
            if (bag_tlv.tag != TAG_SEQUENCE) {
                _.failed = true;
                return null;
            }
            bag_body = bag_tlv.value;
            _.pos = bag_tlv.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        let cur: usize = 0;

        // 1. bagId OBJECT IDENTIFIER
        let bag_id_oid: []const u8 = "";
        if (oid_tlv := readDerTlv(bag_body, cur)) {
            if (oid_tlv.tag != TAG_OID || oid_tlv.value.len == 0) {
                _.failed = true;
                return null;
            }
            bag_id_oid = oid_tlv.value;
            cur = oid_tlv.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 2. bagValue [0] EXPLICIT
        let bag_value_der: []const u8 = "";
        if (val_tlv := readDerTlv(bag_body, cur)) {
            if (val_tlv.tag != TAG_CTX0_CONS) {
                _.failed = true;
                return null;
            }
            bag_value_der = val_tlv.value;
            cur = val_tlv.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 3. Optional bagAttributes SET OF PKCS12Attribute
        let local_key_id: []const u8 = "";
        let friendly_name_bmp: []const u8 = "";
        if (cur < bag_body.len) {
            if (attrs_tlv := readDerTlv(bag_body, cur)) {
                if (attrs_tlv.tag != TAG_SET || attrs_tlv.next_pos != bag_body.len) {
                    _.failed = true;
                    return null;
                }
                let apos: usize = 0;
                while (apos < attrs_tlv.value.len) {
                    if (attr := readDerTlv(attrs_tlv.value, apos)) {
                        if (attr.tag != TAG_SEQUENCE) {
                            _.failed = true;
                            return null;
                        }
                        apos = attr.next_pos;

                        // PKCS12Attribute ::= SEQUENCE { attrId OID, attrValues SET }
                        let acur: usize = 0;
                        let attr_oid: []const u8 = "";
                        if (aoid := readDerTlv(attr.value, acur)) {
                            if (aoid.tag != TAG_OID || aoid.value.len == 0) {
                                _.failed = true;
                                return null;
                            }
                            attr_oid = aoid.value;
                            acur = aoid.next_pos;
                        } else {
                            _.failed = true;
                            return null;
                        }

                        if (avals := readDerTlv(attr.value, acur)) {
                            if (avals.tag != TAG_SET || avals.next_pos != attr.value.len) {
                                _.failed = true;
                                return null;
                            }
                            if (bytesEqual(attr_oid, OID_LOCAL_KEY_ID)) {
                                if (v := readDerTlv(avals.value, 0)) {
                                    if (v.tag != TAG_OCTET_STRING) {
                                        _.failed = true;
                                        return null;
                                    }
                                    local_key_id = v.value;
                                } else {
                                    _.failed = true;
                                    return null;
                                }
                            } else if (bytesEqual(attr_oid, OID_FRIENDLY_NAME)) {
                                if (v := readDerTlv(avals.value, 0)) {
                                    if (v.tag != TAG_BMP_STRING) {
                                        _.failed = true;
                                        return null;
                                    }
                                    friendly_name_bmp = v.value;
                                } else {
                                    _.failed = true;
                                    return null;
                                }
                            }
                        } else {
                            _.failed = true;
                            return null;
                        }
                    } else {
                        _.failed = true;
                        return null;
                    }
                }
            } else {
                _.failed = true;
                return null;
            }
        }

        return Pkcs12Bag {
            bag_kind: classifyBagOid(bag_id_oid),
            bag_id_oid: bag_id_oid,
            bag_value_der: bag_value_der,
            local_key_id: local_key_id,
            friendly_name_bmp: friendly_name_bmp,
        };
    }
}
