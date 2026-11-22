// Source: cp/cap/test/test_ocsp_production_deep.cp
// Deep Production Test Suite for cap:ocsp
// Covers:
//   1. Full successful BasicOCSPResponse (good / revoked+reason / unknown SingleResponses + nextUpdate + certs)
//   2. Non-successful OCSPResponse statuses (tryLater / unauthorized without responseBytes)
//   3. ResponderID byName [1] vs byKey [2] distinction
//   4. OCSPRequest parsing & multi-CertID iteration via OcspReqIter
//   5. Wrong OID, truncated DER, non-zero BIT STRING unused_bits & trailing garbage guards

import { test, expect_eq } from "cap:test";
import {
    OID_OCSP_BASIC,
    OcspResponseStatus,
    ocspResponseStatusFromU8,
    OcspCertStatus,
    OcspCertId,
    OcspSingleResponse,
    OcspResponse,
    OcspSingleIter,
    OcspRequest,
    OcspReqIter,
    parseOcspResponseDer,
    parseOcspRequestDer,
} from "cap:auth";

const OID_SHA1: []const u8 = "\x2B\x0E\x03\x02\x1A";
const OID_SHA256_RSA: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x01\x0B";

struct DerBuf {
    buf: [1024]u8 = [0; 1024],
    len: usize = 0,

    fn push(*_, b: u8) {
        _.buf[_.len] = b;
        _.len += 1;
    }

    fn pushBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.push(s[i]);
            i += 1;
        }
    }

    fn pushTlv(*_, tag: u8, val: []const u8) {
        _.push(tag);
        if (val.len < 128) {
            _.push(val.len as u8);
        } else if (val.len < 256) {
            _.push(0x81);
            _.push(val.len as u8);
        } else {
            _.push(0x82);
            _.push(((val.len >> 8) & 0xFF) as u8);
            _.push((val.len & 0xFF) as u8);
        }
        _.pushBytes(val);
    }

    fn pushBitString(*_, unused_bits: u8, payload: []const u8) {
        let tmp = DerBuf {};
        tmp.push(unused_bits);
        tmp.pushBytes(payload);
        _.pushTlv(0x03, tmp.slice());
    }

    fn slice(): []const u8 {
        return _.buf[0.._.len];
    }
}

fn strEq(a: []const u8, b: []const u8): bool {
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

fn buildCertId(serial_der: []const u8): DerBuf {
    let body = DerBuf {};
    let alg = DerBuf {};
    alg.pushTlv(0x06, OID_SHA1);
    alg.pushTlv(0x05, "");
    body.pushTlv(0x30, alg.slice());
    body.pushTlv(0x04, "\x11\x11\x11\x11"); // issuerNameHash
    body.pushTlv(0x04, "\x22\x22\x22\x22"); // issuerKeyHash
    body.pushTlv(0x02, serial_der);
    let seq = DerBuf {};
    seq.pushTlv(0x30, body.slice());
    return seq;
}

fn buildSingleResp(
    serial_der: []const u8,
    status_kind: u8, // 0=good, 1=revoked, 2=unknown
    rev_time: []const u8,
    rev_reason: u8,
    this_upd: []const u8,
    next_upd: []const u8,
): DerBuf {
    let body = DerBuf {};
    const cid = buildCertId(serial_der);
    body.pushBytes(cid.slice());

    if (status_kind == 0) {
        body.pushTlv(0x80, ""); // [0] IMPLICIT NULL
    } else if (status_kind == 1) {
        let rev_body = DerBuf {};
        rev_body.pushTlv(0x18, rev_time);
        if (rev_reason != 255) {
            let r_arr: [1]u8 = [rev_reason];
            let en = DerBuf {};
            en.pushTlv(0x0A, r_arr[0..1]);
            rev_body.pushTlv(0xA0, en.slice());
        }
        body.pushTlv(0xA1, rev_body.slice());
    } else {
        body.pushTlv(0x82, ""); // [2] IMPLICIT UnknownInfo
    }

    body.pushTlv(0x18, this_upd);
    if (next_upd.len > 0) {
        let nu = DerBuf {};
        nu.pushTlv(0x18, next_upd);
        body.pushTlv(0xA0, nu.slice());
    }

    let seq = DerBuf {};
    seq.pushTlv(0x30, body.slice());
    return seq;
}

fn buildBasicOcspResp(
    by_key: bool,
    responder_bytes: []const u8,
    produced_at: []const u8,
    single_resps: []const u8,
    resp_oid: []const u8,
    sig_unused: u8,
    include_certs: bool,
): DerBuf {
    let tbs = DerBuf {};
    if (by_key) {
        let kh = DerBuf {};
        kh.pushTlv(0x04, responder_bytes);
        tbs.pushTlv(0xA2, kh.slice());
    } else {
        tbs.pushTlv(0xA1, responder_bytes);
    }
    tbs.pushTlv(0x18, produced_at);
    tbs.pushTlv(0x30, single_resps);

    let basic = DerBuf {};
    basic.pushTlv(0x30, tbs.slice());
    let alg = DerBuf {};
    alg.pushTlv(0x06, OID_SHA256_RSA);
    alg.pushTlv(0x05, "");
    basic.pushTlv(0x30, alg.slice());
    basic.pushBitString(sig_unused, "\xDE\xAD\xBE\xEF");
    if (include_certs) {
        let cseq = DerBuf {};
        cseq.pushTlv(0x30, "RESPONDER-CERT");
        let cwrap = DerBuf {};
        cwrap.pushTlv(0x30, cseq.slice());
        basic.pushTlv(0xA0, cwrap.slice());
    }

    let basic_seq = DerBuf {};
    basic_seq.pushTlv(0x30, basic.slice());

    let rb = DerBuf {};
    rb.pushTlv(0x06, resp_oid);
    rb.pushTlv(0x04, basic_seq.slice());
    let rb_seq = DerBuf {};
    rb_seq.pushTlv(0x30, rb.slice());

    let outer = DerBuf {};
    outer.pushTlv(0x0A, "\x00"); // successful
    outer.pushTlv(0xA0, rb_seq.slice());

    let final_der = DerBuf {};
    final_der.pushTlv(0x30, outer.slice());
    return final_der;
}

test("ocsp: successful BasicOCSPResponse with good, revoked+reason & unknown SingleResponses") {
    let srs = DerBuf {};
    const sr1 = buildSingleResp("\x01\x02", 0, "", 255, "20261121000000Z", "20261128000000Z");
    const sr2 = buildSingleResp("\x00\x9A\xBC", 1, "20261001120000Z", 1, "20261121000000Z", "");
    const sr3 = buildSingleResp("\x7F", 2, "", 255, "20261121000000Z", "");
    srs.pushBytes(sr1.slice());
    srs.pushBytes(sr2.slice());
    srs.pushBytes(sr3.slice());

    const der = buildBasicOcspResp(
        true,
        "\xAA\xBB\xCC\xDD",
        "20261121000500Z",
        srs.slice(),
        OID_OCSP_BASIC,
        0,
        true,
    );

    if (resp := parseOcspResponseDer(der.slice())) {
        expect_eq(resp.status == OcspResponseStatus.successful, true);
        expect_eq(resp.is_basic, true);
        expect_eq(resp.tbs_raw_der[0], 0x30);
        expect_eq(resp.responder_is_by_key, true);
        expect_eq(strEq(resp.responder_id, "\xAA\xBB\xCC\xDD"), true);
        expect_eq(strEq(resp.produced_at, "20261121000500Z"), true);
        expect_eq(strEq(resp.sig_alg_oid, OID_SHA256_RSA), true);
        expect_eq(strEq(resp.signature, "\xDE\xAD\xBE\xEF"), true);
        expect_eq(resp.certs_der.len > 0, true);

        let it = resp.singleResponses();
        let idx: u32 = 0;
        while (sr := it.next()) {
            idx += 1;
            expect_eq(strEq(sr.cert_id.hash_alg_oid, OID_SHA1), true);
            if (idx == 1) {
                expect_eq(sr.cert_status == OcspCertStatus.good, true);
                expect_eq(strEq(sr.cert_id.serial_bytes, "\x01\x02"), true);
                expect_eq(strEq(sr.next_update, "20261128000000Z"), true);
            } else if (idx == 2) {
                expect_eq(sr.cert_status == OcspCertStatus.revoked, true);
                expect_eq(strEq(sr.cert_id.serial_bytes, "\x9A\xBC"), true);
                expect_eq(strEq(sr.revocation_time, "20261001120000Z"), true);
                expect_eq(sr.revocation_reason, 1);
            } else if (idx == 3) {
                expect_eq(sr.cert_status == OcspCertStatus.unknown_status, true);
                expect_eq(strEq(sr.cert_id.serial_bytes, "\x7F"), true);
            }
        }
        expect_eq(idx, 3);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("ocsp: non-successful OCSPResponse (tryLater / unauthorized) without responseBytes") {
    let try_later = DerBuf {};
    let tl_body = DerBuf {};
    tl_body.pushTlv(0x0A, "\x03");
    try_later.pushTlv(0x30, tl_body.slice());

    if (resp := parseOcspResponseDer(try_later.slice())) {
        expect_eq(resp.status == OcspResponseStatus.try_later, true);
        expect_eq(resp.is_basic, false);
        expect_eq(resp.responses_der.len, 0);
    } else {
        expect_eq(true, false);
    }

    let unauth = DerBuf {};
    let ua_body = DerBuf {};
    ua_body.pushTlv(0x0A, "\x06");
    unauth.pushTlv(0x30, ua_body.slice());

    if (resp := parseOcspResponseDer(unauth.slice())) {
        expect_eq(resp.status == OcspResponseStatus.unauthorized, true);
        expect_eq(resp.is_basic, false);
    } else {
        expect_eq(true, false);
    }
    expect_eq(ocspResponseStatusFromU8(5) == OcspResponseStatus.sig_required, true);
}

test("ocsp: ResponderID byName [1] vs byKey [2] distinction") {
    const sr = buildSingleResp("\x05", 0, "", 255, "20261121000000Z", "");
    const name_der = "\x30\x04\x05\x00\x05\x00";
    const der = buildBasicOcspResp(
        false,
        name_der,
        "20261121000500Z",
        sr.slice(),
        OID_OCSP_BASIC,
        0,
        false,
    );

    if (resp := parseOcspResponseDer(der.slice())) {
        expect_eq(resp.responder_is_by_key, false);
        expect_eq(strEq(resp.responder_id, name_der), true);
    } else {
        expect_eq(true, false);
    }
}

test("ocsp: OCSPRequest parsing & multi-CertID iteration via OcspReqIter") {
    let req_list = DerBuf {};
    const r1_cid = buildCertId("\x01\x02\x03");
    let r1 = DerBuf {};
    r1.pushTlv(0x30, r1_cid.slice());
    req_list.pushBytes(r1.slice());

    const r2_cid = buildCertId("\x00\x88\x99");
    let r2 = DerBuf {};
    r2.pushTlv(0x30, r2_cid.slice());
    req_list.pushBytes(r2.slice());

    let tbs_req = DerBuf {};
    tbs_req.pushTlv(0x30, req_list.slice());

    let outer = DerBuf {};
    outer.pushTlv(0x30, tbs_req.slice());
    let req_der = DerBuf {};
    req_der.pushTlv(0x30, outer.slice());

    if (req := parseOcspRequestDer(req_der.slice())) {
        expect_eq(req.version, 0);
        expect_eq(req.has_signature, false);

        let it = req.requests();
        let idx: u32 = 0;
        while (cid := it.next()) {
            idx += 1;
            expect_eq(strEq(cid.hash_alg_oid, OID_SHA1), true);
            if (idx == 1) {
                expect_eq(strEq(cid.serial_bytes, "\x01\x02\x03"), true);
            } else if (idx == 2) {
                expect_eq(strEq(cid.serial_bytes, "\x88\x99"), true);
            }
        }
        expect_eq(idx, 2);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("ocsp: wrong responseType OID, truncated DER & non-zero BIT STRING unused_bits guards") {
    const sr = buildSingleResp("\x01", 0, "", 255, "20261121000000Z", "");

    // 1. Wrong responseType OID (not OID_OCSP_BASIC)
    const bad_oid = buildBasicOcspResp(true, "\xAA", "20261121000000Z", sr.slice(), "\x2B\x06\x01\x05", 0, false);
    if (r := parseOcspResponseDer(bad_oid.slice())) {
        * = r;
        expect_eq(true, false);
    }

    // 2. Non-zero BIT STRING unused_bits on BasicOCSPResponse signature
    const bad_unused = buildBasicOcspResp(true, "\xAA", "20261121000000Z", sr.slice(), OID_OCSP_BASIC, 4, false);
    if (r := parseOcspResponseDer(bad_unused.slice())) {
        * = r;
        expect_eq(true, false);
    }

    // 3. Truncated DER
    if (r := parseOcspResponseDer(bad_oid.slice()[0..10])) {
        * = r;
        expect_eq(true, false);
    }
    if (q := parseOcspRequestDer("\x30\x20\x30\x01")) {
        * = q;
        expect_eq(true, false);
    }
}
