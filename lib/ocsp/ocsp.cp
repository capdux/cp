// Origin: Pure-CP Stdlib (Zero-Alloc RFC 6960 OCSP DER Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 6960 Online Certificate Status Protocol (OCSP) DER parser:
// OCSPResponse / BasicOCSPResponse (good/revoked/unknown SingleResponses, byName/byKey ResponderID,
// TBSCert/ResponseData raw slice for verification) and OCSPRequest CertID iteration.

/// RFC 6960 `id-pkix-ocsp-basic` OID (`1.3.6.1.5.5.7.48.1.1`).
pub const OID_OCSP_BASIC: []const u8 = "\x2B\x06\x01\x05\x05\x07\x30\x01\x01";

/// RFC 6960 Section 4.2.1 `OCSPResponseStatus` enumeration.
pub enum OcspResponseStatus: u8 {
    successful = 0,
    malformed_request = 1,
    internal_error = 2,
    try_later = 3,
    sig_required = 5,
    unauthorized = 6,
    unknown = 255,
}

/// Converts a raw `OCSPResponseStatus` byte into `OcspResponseStatus`.
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

const TAG_INTEGER: u8 = 0x02;
const TAG_BIT_STRING: u8 = 0x03;
const TAG_OCTET_STRING: u8 = 0x04;
const TAG_OID: u8 = 0x06;
const TAG_ENUMERATED: u8 = 0x0A;
const TAG_GEN_TIME: u8 = 0x18;
const TAG_SEQUENCE: u8 = 0x30;
const TAG_CTX0_PRIM: u8 = 0x80;
const TAG_CTX2_PRIM: u8 = 0x82;
const TAG_CTX0_CONS: u8 = 0xA0;
const TAG_CTX1_CONS: u8 = 0xA1;
const TAG_CTX2_CONS: u8 = 0xA2;

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
    if (val.len == 0 || val.len > 9 || (val[0] & 0x80) != 0) {
        return null;
    }
    let start: usize = 0;
    if (val.len == 9) {
        if (val[0] != 0) { return null; }
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

fn stripZeroUnusedBitString(val: []const u8): ([]const u8)? {
    if (val.len < 1 || val[0] != 0) {
        return null;
    }
    return val[1..];
}

fn parseCertIdBody(body: []const u8): OcspCertId? {
    let pos: usize = 0;

    let hash_oid: []const u8 = "";
    if (alg := readDerTlv(body, pos)) {
        if (alg.tag != TAG_SEQUENCE) { return null; }
        if (oid := parseAlgIdOid(alg.value)) { hash_oid = oid; } else { return null; }
        pos = alg.next_pos;
    } else {
        return null;
    }

    let name_hash: []const u8 = "";
    if (nh := readDerTlv(body, pos)) {
        if (nh.tag != TAG_OCTET_STRING) { return null; }
        name_hash = nh.value;
        pos = nh.next_pos;
    } else {
        return null;
    }

    let key_hash: []const u8 = "";
    if (kh := readDerTlv(body, pos)) {
        if (kh.tag != TAG_OCTET_STRING) { return null; }
        key_hash = kh.value;
        pos = kh.next_pos;
    } else {
        return null;
    }

    let serial: []const u8 = "";
    if (sn := readDerTlv(body, pos)) {
        if (sn.tag != TAG_INTEGER || sn.value.len == 0 || sn.next_pos != body.len) {
            return null;
        }
        serial = stripLeadingZeros(sn.value);
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
// Public OCSP Response & Request Parsing API
// ---------------------------------------------------------------------------

/// Parses an RFC 6960 DER-encoded `OCSPResponse`.
/// For `successful (0)` responses with `OID_OCSP_BASIC`, automatically unwraps `BasicOCSPResponse`.
pub fn parseOcspResponseDer(der_data: []const u8): OcspResponse? {
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

    // 1. responseStatus ENUMERATED
    let status = OcspResponseStatus.unknown;
    if (st := readDerTlv(outer_body, pos)) {
        if (st.tag != TAG_ENUMERATED || st.value.len != 1) {
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
    if (ctx0 := readDerTlv(outer_body, pos)) {
        if (ctx0.tag != TAG_CTX0_CONS || ctx0.next_pos != outer_body.len) {
            return null;
        }
        if (rb := readDerTlv(ctx0.value, 0)) {
            if (rb.tag != TAG_SEQUENCE || rb.next_pos != ctx0.value.len) {
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
    if (ot := readDerTlv(rb_seq_body, rpos)) {
        if (ot.tag != TAG_OID || ot.value.len == 0) { return null; }
        resp_oid = ot.value;
        rpos = ot.next_pos;
    } else {
        return null;
    }

    let resp_oct: []const u8 = "";
    if (oct := readDerTlv(rb_seq_body, rpos)) {
        if (oct.tag != TAG_OCTET_STRING || oct.next_pos != rb_seq_body.len) {
            return null;
        }
        resp_oct = oct.value;
    } else {
        return null;
    }

    if (status != OcspResponseStatus.successful || !bytesEqual(resp_oid, OID_OCSP_BASIC)) {
        return null;
    }

    // Unwrap BasicOCSPResponse SEQUENCE
    let basic_body: []const u8 = "";
    if (bseq := readDerTlv(resp_oct, 0)) {
        if (bseq.tag != TAG_SEQUENCE || bseq.next_pos != resp_oct.len) {
            return null;
        }
        basic_body = bseq.value;
    } else {
        return null;
    }

    let bpos: usize = 0;

    // Basic 1: tbsResponseData SEQUENCE
    let tbs_body: []const u8 = "";
    if (tbs := readDerTlv(basic_body, bpos)) {
        if (tbs.tag != TAG_SEQUENCE) { return null; }
        resp.tbs_raw_der = tbs.raw;
        tbs_body = tbs.value;
        bpos = tbs.next_pos;
    } else {
        return null;
    }

    // Basic 2: signatureAlgorithm SEQUENCE
    if (sa := readDerTlv(basic_body, bpos)) {
        if (sa.tag != TAG_SEQUENCE) { return null; }
        if (oid := parseAlgIdOid(sa.value)) {
            resp.sig_alg_oid = oid;
        } else {
            return null;
        }
        bpos = sa.next_pos;
    } else {
        return null;
    }

    // Basic 3: signature BIT STRING (unused_bits == 0)
    if (sv := readDerTlv(basic_body, bpos)) {
        if (sv.tag != TAG_BIT_STRING) { return null; }
        if (raw := stripZeroUnusedBitString(sv.value)) {
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
        if (cctx := readDerTlv(basic_body, bpos)) {
            if (cctx.tag != TAG_CTX0_CONS || cctx.next_pos != basic_body.len) {
                return null;
            }
            if (cseq := readDerTlv(cctx.value, 0)) {
                if (cseq.tag != TAG_SEQUENCE || cseq.next_pos != cctx.value.len) {
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

    // Optional version [0] EXPLICIT INTEGER
    if (tpos < tbs_body.len && tbs_body[tpos] == TAG_CTX0_CONS) {
        if (vctx := readDerTlv(tbs_body, tpos)) {
            if (vi := readDerTlv(vctx.value, 0)) {
                if (vi.tag != TAG_INTEGER || vi.next_pos != vctx.value.len) {
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

    // responderID: [1] EXPLICIT Name (0xA1) OR [2] EXPLICIT KeyHash (0xA2 -> OCTET STRING)
    if (rid := readDerTlv(tbs_body, tpos)) {
        if (rid.tag == TAG_CTX1_CONS) {
            resp.responder_is_by_key = false;
            resp.responder_id = rid.value;
        } else if (rid.tag == TAG_CTX2_CONS) {
            resp.responder_is_by_key = true;
            if (oct := readDerTlv(rid.value, 0)) {
                if (oct.tag != TAG_OCTET_STRING || oct.next_pos != rid.value.len) {
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
    if (pa := readDerTlv(tbs_body, tpos)) {
        if (pa.tag != TAG_GEN_TIME || pa.value.len == 0) {
            return null;
        }
        resp.produced_at = pa.value;
        tpos = pa.next_pos;
    } else {
        return null;
    }

    // responses SEQUENCE OF SingleResponse
    if (rlist := readDerTlv(tbs_body, tpos)) {
        if (rlist.tag != TAG_SEQUENCE) {
            return null;
        }
        resp.responses_der = rlist.value;
        tpos = rlist.next_pos;
    } else {
        return null;
    }

    // Optional responseExtensions [1] EXPLICIT Extensions
    if (tpos < tbs_body.len) {
        if (rext := readDerTlv(tbs_body, tpos)) {
            if (rext.tag != TAG_CTX1_CONS || rext.next_pos != tbs_body.len) {
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

    pub fn init(responses_der: []const u8): OcspSingleIter {
        return OcspSingleIter {
            data: responses_der,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): OcspSingleResponse? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        let body: []const u8 = "";
        if (seq := readDerTlv(_.data, _.pos)) {
            if (seq.tag != TAG_SEQUENCE) { _.failed = true; return null; }
            body = seq.value;
            _.pos = seq.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        let cur: usize = 0;

        // 1. certID SEQUENCE
        let cid = OcspCertId {};
        if (ct := readDerTlv(body, cur)) {
            if (ct.tag != TAG_SEQUENCE) { _.failed = true; return null; }
            if (parsed := parseCertIdBody(ct.value)) {
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

        if (cs := readDerTlv(body, cur)) {
            cur = cs.next_pos;
            if (cs.tag == TAG_CTX0_PRIM && cs.value.len == 0) {
                cstatus = OcspCertStatus.good;
            } else if (cs.tag == TAG_CTX2_PRIM && cs.value.len == 0) {
                cstatus = OcspCertStatus.unknown_status;
            } else if (cs.tag == TAG_CTX1_CONS) {
                cstatus = OcspCertStatus.revoked;
                if (rt := readDerTlv(cs.value, 0)) {
                    if (rt.tag != TAG_GEN_TIME || rt.value.len == 0) {
                        _.failed = true;
                        return null;
                    }
                    rev_time = rt.value;
                    if (rt.next_pos < cs.value.len) {
                        if (rctx := readDerTlv(cs.value, rt.next_pos)) {
                            if (rctx.tag != TAG_CTX0_CONS || rctx.next_pos != cs.value.len) {
                                _.failed = true;
                                return null;
                            }
                            if (en := readDerTlv(rctx.value, 0)) {
                                if (en.tag != TAG_ENUMERATED || en.value.len != 1) {
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
        if (tu := readDerTlv(body, cur)) {
            if (tu.tag != TAG_GEN_TIME || tu.value.len == 0) {
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
        if (cur < body.len && body[cur] == TAG_CTX0_CONS) {
            if (nctx := readDerTlv(body, cur)) {
                if (nu := readDerTlv(nctx.value, 0)) {
                    if (nu.tag != TAG_GEN_TIME || nu.next_pos != nctx.value.len) {
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
            if (se := readDerTlv(body, cur)) {
                if (se.tag != TAG_CTX1_CONS || se.next_pos != body.len) {
                    _.failed = true;
                    return null;
                }
            } else {
                _.failed = true;
                return null;
            }
        }

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

    // 1. tbsRequest SEQUENCE
    let tbs_body: []const u8 = "";
    if (tbs := readDerTlv(outer_body, pos)) {
        if (tbs.tag != TAG_SEQUENCE) { return null; }
        tbs_body = tbs.value;
        pos = tbs.next_pos;
    } else {
        return null;
    }

    // 2. Optional optionalSignature [0] EXPLICIT Signature
    let has_sig = false;
    if (pos < outer_body.len) {
        if (osig := readDerTlv(outer_body, pos)) {
            if (osig.tag != TAG_CTX0_CONS || osig.next_pos != outer_body.len) {
                return null;
            }
            has_sig = true;
        } else {
            return null;
        }
    }

    let tpos: usize = 0;
    let ver: u64 = 0;

    // Optional version [0] EXPLICIT INTEGER
    if (tpos < tbs_body.len && tbs_body[tpos] == TAG_CTX0_CONS) {
        if (vctx := readDerTlv(tbs_body, tpos)) {
            if (vi := readDerTlv(vctx.value, 0)) {
                if (vi.tag != TAG_INTEGER || vi.next_pos != vctx.value.len) {
                    return null;
                }
                if (v := decodeDerU64(vi.value)) { ver = v; } else { return null; }
            } else {
                return null;
            }
            tpos = vctx.next_pos;
        } else {
            return null;
        }
    }

    // Optional requestorName [1] EXPLICIT GeneralName
    if (tpos < tbs_body.len && tbs_body[tpos] == TAG_CTX1_CONS) {
        if (rn := readDerTlv(tbs_body, tpos)) {
            tpos = rn.next_pos;
        } else {
            return null;
        }
    }

    // requestList SEQUENCE OF Request
    let req_list: []const u8 = "";
    if (rl := readDerTlv(tbs_body, tpos)) {
        if (rl.tag != TAG_SEQUENCE) { return null; }
        req_list = rl.value;
        tpos = rl.next_pos;
    } else {
        return null;
    }

    // Optional requestExtensions [2] EXPLICIT Extensions
    if (tpos < tbs_body.len) {
        if (rext := readDerTlv(tbs_body, tpos)) {
            if (rext.tag != TAG_CTX2_CONS || rext.next_pos != tbs_body.len) {
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

    pub fn init(request_list_der: []const u8): OcspReqIter {
        return OcspReqIter {
            data: request_list_der,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): OcspCertId? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        let body: []const u8 = "";
        if (req_seq := readDerTlv(_.data, _.pos)) {
            if (req_seq.tag != TAG_SEQUENCE) { _.failed = true; return null; }
            body = req_seq.value;
            _.pos = req_seq.next_pos;
        } else {
            _.failed = true;
            return null;
        }

        if (cid_tlv := readDerTlv(body, 0)) {
            if (cid_tlv.tag != TAG_SEQUENCE) { _.failed = true; return null; }
            if (cid_tlv.next_pos < body.len) {
                // Optional singleRequestExtensions [0] EXPLICIT
                if (sre := readDerTlv(body, cid_tlv.next_pos)) {
                    if (sre.tag != TAG_CTX0_CONS || sre.next_pos != body.len) {
                        _.failed = true;
                        return null;
                    }
                } else {
                    _.failed = true;
                    return null;
                }
            }
            if (cid := parseCertIdBody(cid_tlv.value)) {
                return cid;
            }
        }
        _.failed = true;
        return null;
    }
}
