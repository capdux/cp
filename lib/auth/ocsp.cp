// Reference-hardened against RustCrypto/formats (MIT OR Apache-2.0) & RFC 2986/5280/6960
// Origin: Pure-CP Stdlib (Zero-Alloc RFC 6960 OCSP DER Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 6960 Online Certificate Status Protocol (OCSP) DER parser:
// OCSPResponse / BasicOCSPResponse (good/revoked/unknown SingleResponses, byName/byKey ResponderID,
// ResponseData raw slice for verification), OCSPRequest CertID iteration, and SIMD CertID matching.

import { simdPkiBytesEqual } from "./csr.cp";

/// Maximum accepted OCSP DER message length (1 MiB).
pub const OCSP_MAX_DER_LEN: usize = 1024 * 1024;

/// Maximum ASN.1 DER container nesting depth enforced during strict OCSP validation.
pub const OCSP_MAX_DEPTH: usize = 16;

/// Maximum SingleResponse / Request entries accepted per OCSP message.
pub const OCSP_MAX_ENTRIES: usize = 1024;

/// RFC 6960 `id-pkix-ocsp-basic` OID (`1.3.6.1.5.5.7.48.1.1`).
pub const OID_OCSP_BASIC: []const u8 = "\x2B\x06\x01\x05\x05\x07\x30\x01\x01";

/// RFC 6960 Section 4.2.1 `OCSPResponseStatus` enumeration (value 4 is reserved and rejected).
pub enum OcspResponseStatus: u8 {
    successful = 0,
    malformed_request = 1,
    internal_error = 2,
    try_later = 3,
    sig_required = 5,
    unauthorized = 6,
    unknown = 255,
}

/// Converts a raw `OCSPResponseStatus` byte into `OcspResponseStatus` (rejecting reserved value 4).
pub fn ocspResponseStatusFromU8(v: u8): OcspResponseStatus {
    switch (v) {
        0 => return OcspResponseStatus.successful,
        1 => return OcspResponseStatus.malformed_request,
        2 => return OcspResponseStatus.internal_error,
        3 => return OcspResponseStatus.try_later,
        5 => return OcspResponseStatus.sig_required,
        6 => return OcspResponseStatus.unauthorized,
        else => return OcspResponseStatus.unknown,
    }
}

/// RFC 6960 Section 4.2.1 `CertStatus` choice discriminator.
pub enum OcspCertStatus: u8 {
    good = 0,
    revoked = 1,
    unknown_status = 2,
}

/// Zero-copy decoded RFC 6960 `CertID`.
pub struct OcspCertId {
    hash_alg_oid: []const u8 = "",
    issuer_name_hash: []const u8 = "",
    issuer_key_hash: []const u8 = "",
    serial_bytes: []const u8 = "",

    /// Compares two `OcspCertId` structures using 16-lane SIMD byte comparison across
    /// `hash_alg_oid`, `issuer_name_hash`, `issuer_key_hash`, and normalized `serial_bytes`.
    fn equals(other: OcspCertId): bool {
        return ocspCertIdMatches(_, other);
    }
}

/// Compares two RFC 6960 `OcspCertId` values using 16-lane SIMD comparison.
pub fn ocspCertIdMatches(a: OcspCertId, b: OcspCertId): bool {
    if (!simdPkiBytesEqual(a.hash_alg_oid, b.hash_alg_oid)) {
        return false;
    }
    if (!simdPkiBytesEqual(a.issuer_name_hash, b.issuer_name_hash)) {
        return false;
    }
    if (!simdPkiBytesEqual(a.issuer_key_hash, b.issuer_key_hash)) {
        return false;
    }
    return simdPkiBytesEqual(ocspStripLeadingZeros(a.serial_bytes), ocspStripLeadingZeros(b.serial_bytes));
}

/// Zero-copy decoded RFC 6960 `SingleResponse`.
pub struct OcspSingleResponse {
    cert_id: OcspCertId = OcspCertId {},
    cert_status: OcspCertStatus = OcspCertStatus.unknown_status,
    revocation_time: []const u8 = "",
    revocation_reason: u8 = 255,
    this_update: []const u8 = "",
    next_update: []const u8 = "",
}

/// Zero-copy decoded RFC 6960 `OCSPResponse` (with automatic `BasicOCSPResponse` unwrapping).
pub struct OcspResponse {
    status: OcspResponseStatus = OcspResponseStatus.unknown,
    is_basic: bool = false,
    tbs_raw_der: []const u8 = "",
    produced_at: []const u8 = "",
    responder_is_by_key: bool = false,
    responder_id: []const u8 = "",
    responses_der: []const u8 = "",
    sig_alg_oid: []const u8 = "",
    signature: []const u8 = "",
    certs_der: []const u8 = "",

    /// Returns a zero-allocation iterator over `SingleResponse` entries.
    fn singleResponses(): OcspSingleIter {
        return OcspSingleIter.init(_.responses_der);
    }

    /// Finds the first `SingleResponse` matching `target` via 16-lane SIMD `CertID` comparison.
    fn findSingleResponse(target: OcspCertId): OcspSingleResponse? {
        let it = _.singleResponses();
        while (sr := it.next()) {
            if (ocspCertIdMatches(sr.cert_id, target)) {
                return sr;
            }
        }
        return null;
    }
}

/// Zero-copy decoded RFC 6960 `OCSPRequest`.
pub struct OcspRequest {
    version: u64 = 0,
    request_list_der: []const u8 = "",
    has_signature: bool = false,

    /// Returns a zero-allocation iterator over requested `OcspCertId` entries.
    fn requests(): OcspReqIter {
        return OcspReqIter.init(_.request_list_der);
    }
}

const OCSP_TAG_BOOLEAN: u8 = 0x01;
const OCSP_TAG_INTEGER: u8 = 0x02;
const OCSP_TAG_BIT_STRING: u8 = 0x03;
const OCSP_TAG_OCTET_STRING: u8 = 0x04;
const OCSP_TAG_OID: u8 = 0x06;
const OCSP_TAG_ENUMERATED: u8 = 0x0A;
const OCSP_TAG_GEN_TIME: u8 = 0x18;
const OCSP_TAG_SEQUENCE: u8 = 0x30;
const OCSP_TAG_CTX0_PRIM: u8 = 0x80;
const OCSP_TAG_CTX2_PRIM: u8 = 0x82;
const OCSP_TAG_CTX0_CONS: u8 = 0xA0;
const OCSP_TAG_CTX1_CONS: u8 = 0xA1;
const OCSP_TAG_CTX2_CONS: u8 = 0xA2;

struct OcspDerTlv {
    tag: u8,
    raw: []const u8,
    value: []const u8,
    next_pos: usize,
}

// ---------------------------------------------------------------------------
// Internal Strict ASN.1 DER Helpers (RustCrypto/formats der compliant)
// ---------------------------------------------------------------------------

fn ocspStripLeadingZeros(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start + 1 < s.len && s[start] == 0) {
        start += 1;
    }
    return s[start..];
}

fn ocspParseTwoDigits(s: []const u8, idx: usize): u8? {
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

/// Validates RFC 6960 DER GeneralizedTime (`YYYYMMDDHHMMSSZ`).
fn ocspIsValidGenTime(val: []const u8): bool {
    if (val.len != 15 || val[14] != 'Z') {
        return false;
    }
    if (ocspParseTwoDigits(val, 0) == null || ocspParseTwoDigits(val, 2) == null) {
        return false;
    }
    const month = ocspParseTwoDigits(val, 4) ?? return false;
    const day = ocspParseTwoDigits(val, 6) ?? return false;
    const hour = ocspParseTwoDigits(val, 8) ?? return false;
    const minute = ocspParseTwoDigits(val, 10) ?? return false;
    const second = ocspParseTwoDigits(val, 12) ?? return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    if (hour > 23 || minute > 59 || second > 59) return false;
    return true;
}

fn ocspIsValidDerInteger(val: []const u8): bool {
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

fn ocspIsValidDerBoolean(val: []const u8): bool {
    return val.len == 1 && (val[0] == 0x00 || val[0] == 0xFF);
}

fn ocspIsValidDerOid(val: []const u8): bool {
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

fn ocspIsValidCrlReasonByte(v: u8): bool {
    return (v <= 6) || (v >= 8 && v <= 10);
}

fn ocspReadDerTlv(data: []const u8, pos: usize): OcspDerTlv? {
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
    return OcspDerTlv {
        tag: tag,
        raw: data[pos..end],
        value: data[val_start..end],
        next_pos: end,
    };
}

fn ocspDecodeDerU64(val: []const u8): u64? {
    if (!ocspIsValidDerInteger(val) || val.len > 9 || (val[0] & 0x80) != 0) {
        return null;
    }
    let start: usize = 0;
    if (val.len == 9) {
        if (val[0] != 0) { return null; }
        start = 1;
    }
    let acc: u64 = 0;
    let i = start;
    while (i < val.len) : (i += 1) {
        acc = (acc << 8) | (val[i] as u64);
    }
    return acc;
}

fn ocspParseAlgIdOid(seq_body: []const u8): ([]const u8)? {
    if (oid_tlv := ocspReadDerTlv(seq_body, 0)) {
        if (oid_tlv.tag != OCSP_TAG_OID || !ocspIsValidDerOid(oid_tlv.value)) {
            return null;
        }
        if (oid_tlv.next_pos < seq_body.len) {
            if (param := ocspReadDerTlv(seq_body, oid_tlv.next_pos)) {
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

fn ocspStripZeroUnusedBitString(val: []const u8): ([]const u8)? {
    if (val.len < 2 || val[0] != 0) {
        return null;
    }
    return val[1..];
}

fn ocspValidateDerTree(slice: []const u8, depth: usize, max_depth: usize): bool {
    if (depth > max_depth) {
        return false;
    }
    let pos: usize = 0;
    while (pos < slice.len) {
        if (tlv := ocspReadDerTlv(slice, pos)) {
            pos = tlv.next_pos;
            if (tlv.tag == OCSP_TAG_BOOLEAN && !ocspIsValidDerBoolean(tlv.value)) {
                return false;
            }
            if (tlv.tag == OCSP_TAG_INTEGER && !ocspIsValidDerInteger(tlv.value)) {
                return false;
            }
            if (tlv.tag == OCSP_TAG_OID && !ocspIsValidDerOid(tlv.value)) {
                return false;
            }
            if ((tlv.tag & 0x20) != 0) {
                if (!ocspValidateDerTree(tlv.value, depth + 1, max_depth)) {
                    return false;
                }
            }
        } else {
            return false;
        }
    }
    return pos == slice.len;
}

fn ocspParseCertIdBody(body: []const u8): OcspCertId? {
    let pos: usize = 0;

    let hash_oid: []const u8 = "";
    if (alg := ocspReadDerTlv(body, pos)) {
        if (alg.tag != OCSP_TAG_SEQUENCE) { return null; }
        if (oid := ocspParseAlgIdOid(alg.value)) { hash_oid = oid; } else { return null; }
        pos = alg.next_pos;
    } else {
        return null;
    }

    let name_hash: []const u8 = "";
    if (nh := ocspReadDerTlv(body, pos)) {
        if (nh.tag != OCSP_TAG_OCTET_STRING || nh.value.len == 0) { return null; }
        name_hash = nh.value;
        pos = nh.next_pos;
    } else {
        return null;
    }

    let key_hash: []const u8 = "";
    if (kh := ocspReadDerTlv(body, pos)) {
        if (kh.tag != OCSP_TAG_OCTET_STRING || kh.value.len == 0) { return null; }
        key_hash = kh.value;
        pos = kh.next_pos;
    } else {
        return null;
    }

    let serial: []const u8 = "";
    if (sn := ocspReadDerTlv(body, pos)) {
        if (sn.tag != OCSP_TAG_INTEGER || !ocspIsValidDerInteger(sn.value) || (sn.value[0] & 0x80) != 0 || sn.next_pos != body.len) {
            return null;
        }
        serial = ocspStripLeadingZeros(sn.value);
    } else {
        return null;
    }

    return OcspCertId {
        hash_alg_oid: hash_oid,
        issuer_name_hash: name_hash,
        issuer_key_hash: key_hash,
        serial_bytes: serial,
    };
}

// ---------------------------------------------------------------------------
// Public OCSP Response & Request Parsing & Strict Validation API
// ---------------------------------------------------------------------------

/// Parses an RFC 6960 DER-encoded `OCSPResponse`.
/// Rejects reserved status value `4`, non-canonical DER lengths, and trailing garbage bytes.
/// For `successful (0)` responses with `OID_OCSP_BASIC`, automatically unwraps `BasicOCSPResponse`.
pub fn parseOcspResponseDer(der_data: []const u8): OcspResponse? {
    if (der_data.len == 0 || der_data.len > OCSP_MAX_DER_LEN) {
        return null;
    }

    let outer_body: []const u8 = "";
    if (outer := ocspReadDerTlv(der_data, 0)) {
        if (outer.tag != OCSP_TAG_SEQUENCE || outer.next_pos != der_data.len) {
            return null;
        }
        outer_body = outer.value;
    } else {
        return null;
    }

    let pos: usize = 0;

    // 1. responseStatus ENUMERATED (0..=6 excluding reserved 4)
    let status = OcspResponseStatus.unknown;
    if (st := ocspReadDerTlv(outer_body, pos)) {
        if (st.tag != OCSP_TAG_ENUMERATED || st.value.len != 1) {
            return null;
        }
        status = ocspResponseStatusFromU8(st.value[0]);
        if (status == OcspResponseStatus.unknown) {
            return null;
        }
        pos = st.next_pos;
    } else {
        return null;
    }

    let resp = OcspResponse { status: status };

    // Non-successful responses normally omit responseBytes
    if (pos == outer_body.len) {
        return resp;
    }

    // 2. responseBytes [0] EXPLICIT ResponseBytes
    let rb_seq_body: []const u8 = "";
    if (ctx0 := ocspReadDerTlv(outer_body, pos)) {
        if (ctx0.tag != OCSP_TAG_CTX0_CONS || ctx0.next_pos != outer_body.len) {
            return null;
        }
        if (rb := ocspReadDerTlv(ctx0.value, 0)) {
            if (rb.tag != OCSP_TAG_SEQUENCE || rb.next_pos != ctx0.value.len) {
                return null;
            }
            rb_seq_body = rb.value;
        } else {
            return null;
        }
    } else {
        return null;
    }

    let rpos: usize = 0;
    let resp_oid: []const u8 = "";
    if (ot := ocspReadDerTlv(rb_seq_body, rpos)) {
        if (ot.tag != OCSP_TAG_OID || !ocspIsValidDerOid(ot.value)) { return null; }
        resp_oid = ot.value;
        rpos = ot.next_pos;
    } else {
        return null;
    }

    let resp_oct: []const u8 = "";
    if (oct := ocspReadDerTlv(rb_seq_body, rpos)) {
        if (oct.tag != OCSP_TAG_OCTET_STRING || oct.next_pos != rb_seq_body.len) {
            return null;
        }
        resp_oct = oct.value;
    } else {
        return null;
    }

    if (status != OcspResponseStatus.successful || !simdPkiBytesEqual(resp_oid, OID_OCSP_BASIC)) {
        return null;
    }

    // Unwrap BasicOCSPResponse SEQUENCE
    let basic_body: []const u8 = "";
    if (bseq := ocspReadDerTlv(resp_oct, 0)) {
        if (bseq.tag != OCSP_TAG_SEQUENCE || bseq.next_pos != resp_oct.len) {
            return null;
        }
        basic_body = bseq.value;
    } else {
        return null;
    }

    let bpos: usize = 0;

    // Basic 1: tbsResponseData SEQUENCE
    let tbs_body: []const u8 = "";
    if (tbs := ocspReadDerTlv(basic_body, bpos)) {
        if (tbs.tag != OCSP_TAG_SEQUENCE) { return null; }
        resp.tbs_raw_der = tbs.raw;
        tbs_body = tbs.value;
        bpos = tbs.next_pos;
    } else {
        return null;
    }

    // Basic 2: signatureAlgorithm SEQUENCE
    if (sa := ocspReadDerTlv(basic_body, bpos)) {
        if (sa.tag != OCSP_TAG_SEQUENCE) { return null; }
        if (oid := ocspParseAlgIdOid(sa.value)) {
            resp.sig_alg_oid = oid;
        } else {
            return null;
        }
        bpos = sa.next_pos;
    } else {
        return null;
    }

    // Basic 3: signature BIT STRING (unused_bits == 0, non-empty)
    if (sv := ocspReadDerTlv(basic_body, bpos)) {
        if (sv.tag != OCSP_TAG_BIT_STRING) { return null; }
        if (raw := ocspStripZeroUnusedBitString(sv.value)) {
            resp.signature = raw;
        } else {
            return null;
        }
        bpos = sv.next_pos;
    } else {
        return null;
    }

    // Basic 4: Optional certs [0] EXPLICIT SEQUENCE OF Certificate
    if (bpos < basic_body.len) {
        if (cctx := ocspReadDerTlv(basic_body, bpos)) {
            if (cctx.tag != OCSP_TAG_CTX0_CONS || cctx.next_pos != basic_body.len) {
                return null;
            }
            if (cseq := ocspReadDerTlv(cctx.value, 0)) {
                if (cseq.tag != OCSP_TAG_SEQUENCE || cseq.next_pos != cctx.value.len) {
                    return null;
                }
                resp.certs_der = cseq.value;
            } else {
                return null;
            }
        } else {
            return null;
        }
    }

    // Parse inside ResponseData (tbs_body)
    let tpos: usize = 0;

    // Optional version [0] EXPLICIT INTEGER (RFC 6960 only defines v1(0))
    if (tpos < tbs_body.len && tbs_body[tpos] == OCSP_TAG_CTX0_CONS) {
        if (vctx := ocspReadDerTlv(tbs_body, tpos)) {
            if (vi := ocspReadDerTlv(vctx.value, 0)) {
                if (vi.tag != OCSP_TAG_INTEGER || vi.next_pos != vctx.value.len) {
                    return null;
                }
                if (v := ocspDecodeDerU64(vi.value)) {
                    if (v != 0) {
                        return null;
                    }
                } else {
                    return null;
                }
            } else {
                return null;
            }
            tpos = vctx.next_pos;
        } else {
            return null;
        }
    }

    // responderID: [1] EXPLICIT Name (0xA1 -> SEQUENCE) OR [2] EXPLICIT KeyHash (0xA2 -> OCTET STRING)
    if (rid := ocspReadDerTlv(tbs_body, tpos)) {
        if (rid.tag == OCSP_TAG_CTX1_CONS) {
            if (nm := ocspReadDerTlv(rid.value, 0)) {
                if (nm.tag != OCSP_TAG_SEQUENCE || nm.next_pos != rid.value.len) {
                    return null;
                }
                resp.responder_is_by_key = false;
                resp.responder_id = rid.value;
            } else {
                return null;
            }
        } else if (rid.tag == OCSP_TAG_CTX2_CONS) {
            resp.responder_is_by_key = true;
            if (oct := ocspReadDerTlv(rid.value, 0)) {
                if (oct.tag != OCSP_TAG_OCTET_STRING || oct.value.len == 0 || oct.next_pos != rid.value.len) {
                    return null;
                }
                resp.responder_id = oct.value;
            } else {
                return null;
            }
        } else {
            return null;
        }
        tpos = rid.next_pos;
    } else {
        return null;
    }

    // producedAt GeneralizedTime
    if (pa := ocspReadDerTlv(tbs_body, tpos)) {
        if (pa.tag != OCSP_TAG_GEN_TIME || !ocspIsValidGenTime(pa.value)) {
            return null;
        }
        resp.produced_at = pa.value;
        tpos = pa.next_pos;
    } else {
        return null;
    }

    // responses SEQUENCE OF SingleResponse
    if (rlist := ocspReadDerTlv(tbs_body, tpos)) {
        if (rlist.tag != OCSP_TAG_SEQUENCE) {
            return null;
        }
        resp.responses_der = rlist.value;
        tpos = rlist.next_pos;
    } else {
        return null;
    }

    // Optional responseExtensions [1] EXPLICIT Extensions
    if (tpos < tbs_body.len) {
        if (rext := ocspReadDerTlv(tbs_body, tpos)) {
            if (rext.tag != OCSP_TAG_CTX1_CONS || rext.next_pos != tbs_body.len) {
                return null;
            }
            if (eseq := ocspReadDerTlv(rext.value, 0)) {
                if (eseq.tag != OCSP_TAG_SEQUENCE || eseq.next_pos != rext.value.len) {
                    return null;
                }
            } else {
                return null;
            }
        } else {
            return null;
        }
    }

    resp.is_basic = true;
    return resp;
}

/// Zero-allocation iterator over `SingleResponse` records in `responses_der`.
pub struct OcspSingleIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,
    count: usize = 0,

    pub static fn init(responses_der: []const u8): OcspSingleIter {
        return OcspSingleIter {
            data: responses_der,
            pos: 0,
            failed: false,
            count: 0,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): OcspSingleResponse? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        if (_.count >= OCSP_MAX_ENTRIES) {
            _.failed = true;
            return null;
        }
        let body: []const u8 = "";
        if (seq := ocspReadDerTlv(_.data, _.pos)) {
            if (seq.tag != OCSP_TAG_SEQUENCE) { _.failed = true; return null; }
            body = seq.value;
            _.pos = seq.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        let cur: usize = 0;

        // 1. certID SEQUENCE
        let cid = OcspCertId {};
        if (ct := ocspReadDerTlv(body, cur)) {
            if (ct.tag != OCSP_TAG_SEQUENCE) { _.failed = true; return null; }
            if (parsed := ocspParseCertIdBody(ct.value)) {
                cid = parsed;
            } else {
                _.failed = true;
                return null;
            }
            cur = ct.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 2. certStatus: [0] IMPLICIT NULL (0x80), [1] IMPLICIT RevokedInfo (0xA1), [2] IMPLICIT UnknownInfo (0x82)
        let cstatus = OcspCertStatus.unknown_status;
        let rev_time: []const u8 = "";
        let rev_reason: u8 = 255;

        if (cs := ocspReadDerTlv(body, cur)) {
            cur = cs.next_pos;
            if (cs.tag == OCSP_TAG_CTX0_PRIM && cs.value.len == 0) {
                cstatus = OcspCertStatus.good;
            } else if (cs.tag == OCSP_TAG_CTX2_PRIM && cs.value.len == 0) {
                cstatus = OcspCertStatus.unknown_status;
            } else if (cs.tag == OCSP_TAG_CTX1_CONS) {
                cstatus = OcspCertStatus.revoked;
                if (rt := ocspReadDerTlv(cs.value, 0)) {
                    if (rt.tag != OCSP_TAG_GEN_TIME || !ocspIsValidGenTime(rt.value)) {
                        _.failed = true;
                        return null;
                    }
                    rev_time = rt.value;
                    if (rt.next_pos < cs.value.len) {
                        if (rctx := ocspReadDerTlv(cs.value, rt.next_pos)) {
                            if (rctx.tag != OCSP_TAG_CTX0_CONS || rctx.next_pos != cs.value.len) {
                                _.failed = true;
                                return null;
                            }
                            if (en := ocspReadDerTlv(rctx.value, 0)) {
                                if (en.tag != OCSP_TAG_ENUMERATED || en.value.len != 1 || en.next_pos != rctx.value.len || !ocspIsValidCrlReasonByte(en.value[0])) {
                                    _.failed = true;
                                    return null;
                                }
                                rev_reason = en.value[0];
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
            } else {
                _.failed = true;
                return null;
            }
        } else {
            _.failed = true;
            return null;
        }

        // 3. thisUpdate GeneralizedTime
        let this_upd: []const u8 = "";
        if (tu := ocspReadDerTlv(body, cur)) {
            if (tu.tag != OCSP_TAG_GEN_TIME || !ocspIsValidGenTime(tu.value)) {
                _.failed = true;
                return null;
            }
            this_upd = tu.value;
            cur = tu.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        // 4. Optional nextUpdate [0] EXPLICIT GeneralizedTime
        let next_upd: []const u8 = "";
        if (cur < body.len && body[cur] == OCSP_TAG_CTX0_CONS) {
            if (nctx := ocspReadDerTlv(body, cur)) {
                if (nu := ocspReadDerTlv(nctx.value, 0)) {
                    if (nu.tag != OCSP_TAG_GEN_TIME || !ocspIsValidGenTime(nu.value) || nu.next_pos != nctx.value.len) {
                        _.failed = true;
                        return null;
                    }
                    next_upd = nu.value;
                } else {
                    _.failed = true;
                    return null;
                }
                cur = nctx.next_pos;
            } else {
                _.failed = true;
                return null;
            }
        }

        // 5. Optional singleExtensions [1] EXPLICIT Extensions
        if (cur < body.len) {
            if (se := ocspReadDerTlv(body, cur)) {
                if (se.tag != OCSP_TAG_CTX1_CONS || se.next_pos != body.len) {
                    _.failed = true;
                    return null;
                }
                if (sseq := ocspReadDerTlv(se.value, 0)) {
                    if (sseq.tag != OCSP_TAG_SEQUENCE || sseq.next_pos != se.value.len) {
                        _.failed = true;
                        return null;
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

        _.count += 1;
        return OcspSingleResponse {
            cert_id: cid,
            cert_status: cstatus,
            revocation_time: rev_time,
            revocation_reason: rev_reason,
            this_update: this_upd,
            next_update: next_upd,
        };
    }
}

/// Parses an RFC 6960 DER-encoded `OCSPRequest` and extracts `request_list_der`.
pub fn parseOcspRequestDer(der_data: []const u8): OcspRequest? {
    if (der_data.len == 0 || der_data.len > OCSP_MAX_DER_LEN) {
        return null;
    }

    let outer_body: []const u8 = "";
    if (outer := ocspReadDerTlv(der_data, 0)) {
        if (outer.tag != OCSP_TAG_SEQUENCE || outer.next_pos != der_data.len) {
            return null;
        }
        outer_body = outer.value;
    } else {
        return null;
    }

    let pos: usize = 0;

    // 1. tbsRequest SEQUENCE
    let tbs_body: []const u8 = "";
    if (tbs := ocspReadDerTlv(outer_body, pos)) {
        if (tbs.tag != OCSP_TAG_SEQUENCE) { return null; }
        tbs_body = tbs.value;
        pos = tbs.next_pos;
    } else {
        return null;
    }

    // 2. Optional optionalSignature [0] EXPLICIT Signature
    let has_sig = false;
    if (pos < outer_body.len) {
        if (osig := ocspReadDerTlv(outer_body, pos)) {
            if (osig.tag != OCSP_TAG_CTX0_CONS || osig.next_pos != outer_body.len) {
                return null;
            }
            if (sseq := ocspReadDerTlv(osig.value, 0)) {
                if (sseq.tag != OCSP_TAG_SEQUENCE || sseq.next_pos != osig.value.len) {
                    return null;
                }
            } else {
                return null;
            }
            has_sig = true;
        } else {
            return null;
        }
    }

    let tpos: usize = 0;
    let ver: u64 = 0;

    // Optional version [0] EXPLICIT INTEGER (RFC 6960 only defines v1(0))
    if (tpos < tbs_body.len && tbs_body[tpos] == OCSP_TAG_CTX0_CONS) {
        if (vctx := ocspReadDerTlv(tbs_body, tpos)) {
            if (vi := ocspReadDerTlv(vctx.value, 0)) {
                if (vi.tag != OCSP_TAG_INTEGER || vi.next_pos != vctx.value.len) {
                    return null;
                }
                if (v := ocspDecodeDerU64(vi.value)) {
                    if (v != 0) {
                        return null;
                    }
                    ver = v;
                } else {
                    return null;
                }
            } else {
                return null;
            }
            tpos = vctx.next_pos;
        } else {
            return null;
        }
    }

    // Optional requestorName [1] EXPLICIT GeneralName
    if (tpos < tbs_body.len && tbs_body[tpos] == OCSP_TAG_CTX1_CONS) {
        if (rn := ocspReadDerTlv(tbs_body, tpos)) {
            if (gn := ocspReadDerTlv(rn.value, 0)) {
                if (gn.next_pos != rn.value.len) {
                    return null;
                }
            } else {
                return null;
            }
            tpos = rn.next_pos;
        } else {
            return null;
        }
    }

    // requestList SEQUENCE OF Request
    let req_list: []const u8 = "";
    if (rl := ocspReadDerTlv(tbs_body, tpos)) {
        if (rl.tag != OCSP_TAG_SEQUENCE) { return null; }
        req_list = rl.value;
        tpos = rl.next_pos;
    } else {
        return null;
    }

    // Optional requestExtensions [2] EXPLICIT Extensions
    if (tpos < tbs_body.len) {
        if (rext := ocspReadDerTlv(tbs_body, tpos)) {
            if (rext.tag != OCSP_TAG_CTX2_CONS || rext.next_pos != tbs_body.len) {
                return null;
            }
            if (eseq := ocspReadDerTlv(rext.value, 0)) {
                if (eseq.tag != OCSP_TAG_SEQUENCE || eseq.next_pos != rext.value.len) {
                    return null;
                }
            } else {
                return null;
            }
        } else {
            return null;
        }
    }

    return OcspRequest {
        version: ver,
        request_list_der: req_list,
        has_signature: has_sig,
    };
}

/// Zero-allocation iterator over `Request` entries in `OCSPRequest.request_list_der`.
pub struct OcspReqIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,
    count: usize = 0,

    pub static fn init(request_list_der: []const u8): OcspReqIter {
        return OcspReqIter {
            data: request_list_der,
            pos: 0,
            failed: false,
            count: 0,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): OcspCertId? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        if (_.count >= OCSP_MAX_ENTRIES) {
            _.failed = true;
            return null;
        }
        let body: []const u8 = "";
        if (req_seq := ocspReadDerTlv(_.data, _.pos)) {
            if (req_seq.tag != OCSP_TAG_SEQUENCE) { _.failed = true; return null; }
            body = req_seq.value;
            _.pos = req_seq.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        if (cid_tlv := ocspReadDerTlv(body, 0)) {
            if (cid_tlv.tag != OCSP_TAG_SEQUENCE) { _.failed = true; return null; }
            if (cid_tlv.next_pos < body.len) {
                // Optional singleRequestExtensions [0] EXPLICIT
                if (sre := ocspReadDerTlv(body, cid_tlv.next_pos)) {
                    if (sre.tag != OCSP_TAG_CTX0_CONS || sre.next_pos != body.len) {
                        _.failed = true;
                        return null;
                    }
                } else {
                    _.failed = true;
                    return null;
                }
            }
            if (cid := ocspParseCertIdBody(cid_tlv.value)) {
                _.count += 1;
                return cid;
            }
        }
        _.failed = true;
        return null;
    }
}

/// Zero-heap strict RFC 6960 `OCSPResponse` DER validator.
/// Enforces canonical DER tree depth (`OCSP_MAX_DEPTH`), valid responseStatus (`0..=6`, rejecting `4`),
/// presence of `BasicOCSPResponse` on `successful`, and full validation of all `SingleResponse` entries.
pub fn validateOcspResponseStrict(der_data: []const u8): bool {
    if (der_data.len == 0 || der_data.len > OCSP_MAX_DER_LEN) {
        return false;
    }
    if (!ocspValidateDerTree(der_data, 0, OCSP_MAX_DEPTH)) {
        return false;
    }
    if (resp := parseOcspResponseDer(der_data)) {
        if (resp.status == OcspResponseStatus.successful) {
            if (!resp.is_basic) {
                return false;
            }
            if (!ocspValidateDerTree(resp.tbs_raw_der, 0, OCSP_MAX_DEPTH)) {
                return false;
            }
            let it = resp.singleResponses();
            let count: usize = 0;
            while (sr := it.next()) {
                * = sr;
                count += 1;
                if (count > OCSP_MAX_ENTRIES) {
                    return false;
                }
            }
            if (it.hasError()) {
                return false;
            }
        }
        return true;
    }
    return false;
}

/// Zero-heap strict RFC 6960 `OCSPRequest` DER validator.
/// Enforces canonical DER tree depth (`OCSP_MAX_DEPTH`), `version == 0`, non-empty `requestList`,
/// and full validation of all `CertID` entries.
pub fn validateOcspRequestStrict(der_data: []const u8): bool {
    if (der_data.len == 0 || der_data.len > OCSP_MAX_DER_LEN) {
        return false;
    }
    if (!ocspValidateDerTree(der_data, 0, OCSP_MAX_DEPTH)) {
        return false;
    }
    if (req := parseOcspRequestDer(der_data)) {
        let it = req.requests();
        let count: usize = 0;
        while (cid := it.next()) {
            * = cid;
            count += 1;
            if (count > OCSP_MAX_ENTRIES) {
                return false;
            }
        }
        if (it.hasError() || count == 0) {
            return false;
        }
        return true;
    }
    return false;
}

/// Zero-heap strict RFC 6960 OCSP DER validator accepting either a valid `OCSPResponse` or `OCSPRequest`.
pub fn validateOcspStrict(der_data: []const u8): bool {
    if (validateOcspResponseStrict(der_data)) {
        return true;
    }
    return validateOcspRequestStrict(der_data);
}
