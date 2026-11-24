import { test, expect_eq } from "cap:test";
import {
    CSR_MAX_DEPTH,
    OID_CN,
    OID_O,
    OID_C,
    OID_EXTENSION_REQUEST,
    OID_SAN,
    scalarPkiBytesEqual,
    simdPkiBytesEqual,
    scalarPkiCopyBytes,
    simdPkiCopyBytes,
    parseCsrDer,
    validateCsrStrict,
    isCsrDer,
} from "../../lib/auth/csr.cp";
import {
    CrlReason,
    OID_CRL_REASON,
    parseCrlDer,
    validateCrlStrict,
    isCrlDer,
    summarizeCrl,
} from "../../lib/auth/crl.cp";
import {
    OID_OCSP_BASIC,
    OcspResponseStatus,
    OcspCertStatus,
    OcspCertId,
    ocspResponseStatusFromU8,
    ocspCertIdMatches,
    parseOcspResponseDer,
    parseOcspRequestDer,
    validateOcspResponseStrict,
    validateOcspRequestStrict,
    validateOcspStrict,
} from "../../lib/auth/ocsp.cp";

const std = @import("std");

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn expect_false(cond: bool): void! {
    expect_eq(cond, false);
}

// ---------------------------------------------------------------------------
// Minimal Zero-Alloc DER Builder Helpers for Test Fixtures
// ---------------------------------------------------------------------------

struct DerBuf {
    data: [2048]u8 = [0; 2048],
    len: usize = 0,

    fn slice(): []const u8 {
        return _.data[0.._.len];
    }

    fn pushByte(*_, b: u8) {
        _.data[_.len] = b;
        _.len += 1;
    }

    fn pushSlice(*_, s: []const u8) {
        simdPkiCopyBytes(_.data[_.len .. _.len + s.len], s);
        _.len += s.len;
    }

    fn pushTlv(*_, tag: u8, val: []const u8) {
        _.pushByte(tag);
        if (val.len < 128) {
            _.pushByte(val.len as u8);
        } else if (val.len < 256) {
            _.pushByte(0x81);
            _.pushByte(val.len as u8);
        } else {
            _.pushByte(0x82);
            _.pushByte(((val.len >> 8) & 0xFF) as u8);
            _.pushByte((val.len & 0xFF) as u8);
        }
        _.pushSlice(val);
    }
}

// Common OIDs
const OID_SHA256_WITH_RSA: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x01\x0B";
const OID_ECDSA_SHA256: []const u8 = "\x2A\x86\x48\xCE\x3D\x04\x03\x02";
const OID_RSA_ENCRYPTION: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01";
const OID_SHA256: []const u8 = "\x60\x86\x48\x01\x65\x03\x04\x02\x01";

fn buildAlgId(oid: []const u8): DerBuf {
    let inner = DerBuf {};
    inner.pushTlv(0x06, oid);
    inner.pushTlv(0x05, ""); // NULL parameters
    let out = DerBuf {};
    out.pushTlv(0x30, inner.slice());
    return out;
}

fn buildRdn(oid: []const u8, val: []const u8): DerBuf {
    let atv_body = DerBuf {};
    atv_body.pushTlv(0x06, oid);
    atv_body.pushTlv(0x0C, val); // UTF8String
    let atv = DerBuf {};
    atv.pushTlv(0x30, atv_body.slice());
    let rdn = DerBuf {};
    rdn.pushTlv(0x31, atv.slice());
    return rdn;
}

fn buildSampleCsr(): DerBuf {
    // Subject: C=US, O=Example Corp, CN=api.example.com
    let subj_body = DerBuf {};
    const r1 = buildRdn(OID_C, "US");
    const r2 = buildRdn(OID_O, "Example Corp");
    const r3 = buildRdn(OID_CN, "api.example.com");
    subj_body.pushSlice(r1.slice());
    subj_body.pushSlice(r2.slice());
    subj_body.pushSlice(r3.slice());
    let subj = DerBuf {};
    subj.pushTlv(0x30, subj_body.slice());

    // SPKI
    const pub_alg = buildAlgId(OID_RSA_ENCRYPTION);
    let spki_body = DerBuf {};
    spki_body.pushSlice(pub_alg.slice());
    spki_body.pushTlv(0x03, "\x00\x30\x05\x02\x01\x03\x02\x01\x11");
    let spki = DerBuf {};
    spki.pushTlv(0x30, spki_body.slice());

    // SAN GeneralNames: dNSName("api.example.com"), dNSName("*.example.com")
    let gn_body = DerBuf {};
    gn_body.pushTlv(0x82, "api.example.com");
    gn_body.pushTlv(0x82, "*.example.com");
    let gn_seq = DerBuf {};
    gn_seq.pushTlv(0x30, gn_body.slice());

    let ext_body = DerBuf {};
    ext_body.pushTlv(0x06, OID_SAN);
    ext_body.pushTlv(0x01, "\x00"); // critical = FALSE (canonical 0x00)
    ext_body.pushTlv(0x04, gn_seq.slice());
    let ext_seq = DerBuf {};
    ext_seq.pushTlv(0x30, ext_body.slice());

    let exts_seq = DerBuf {};
    exts_seq.pushTlv(0x30, ext_seq.slice());
    let attr_vals = DerBuf {};
    attr_vals.pushTlv(0x31, exts_seq.slice());

    let attr_body = DerBuf {};
    attr_body.pushTlv(0x06, OID_EXTENSION_REQUEST);
    attr_body.pushSlice(attr_vals.slice());
    let attr_seq = DerBuf {};
    attr_seq.pushTlv(0x30, attr_body.slice());

    // CRI
    let cri_body = DerBuf {};
    cri_body.pushTlv(0x02, "\x00"); // version v1(0)
    cri_body.pushSlice(subj.slice());
    cri_body.pushSlice(spki.slice());
    cri_body.pushTlv(0xA0, attr_seq.slice());

    let outer_body = DerBuf {};
    outer_body.pushTlv(0x30, cri_body.slice());
    const sig_alg = buildAlgId(OID_SHA256_WITH_RSA);
    outer_body.pushSlice(sig_alg.slice());
    outer_body.pushTlv(0x03, "\x00\xDE\xAD\xBE\xEF\x01\x02\x03\x04\x11\x22\x33\x44\x55\x66\x77\x88");

    let csr = DerBuf {};
    csr.pushTlv(0x30, outer_body.slice());
    return csr;
}

fn buildSampleCrl(v2: bool, match_sig_alg: bool): DerBuf {
    let tbs_body = DerBuf {};
    if (v2) {
        tbs_body.pushTlv(0x02, "\x01"); // version v2(1)
    }
    const tbs_alg = buildAlgId(OID_SHA256_WITH_RSA);
    tbs_body.pushSlice(tbs_alg.slice());

    // Issuer Name
    const iss_rdn = buildRdn(OID_CN, "Example Root CA");
    tbs_body.pushTlv(0x30, iss_rdn.slice());

    // thisUpdate (UTCTime) & nextUpdate (GeneralizedTime)
    tbs_body.pushTlv(0x17, "261124000000Z");
    tbs_body.pushTlv(0x18, "20261224000000Z");

    // revokedCertificates: 2 entries
    // Entry 1: serial 0x00 0x80 0x01 ... (16+ bytes to exercise SIMD path), reason = key_compromise (1)
    let reason_ext_val = DerBuf {};
    reason_ext_val.pushTlv(0x0A, "\x01"); // ENUMERATED key_compromise
    let reason_ext_body = DerBuf {};
    reason_ext_body.pushTlv(0x06, OID_CRL_REASON);
    reason_ext_body.pushTlv(0x04, reason_ext_val.slice());
    let reason_ext = DerBuf {};
    reason_ext.pushTlv(0x30, reason_ext_body.slice());
    let entry1_exts = DerBuf {};
    entry1_exts.pushTlv(0x30, reason_ext.slice());

    let e1_body = DerBuf {};
    e1_body.pushTlv(0x02, "\x00\x80\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10");
    e1_body.pushTlv(0x17, "261120123000Z");
    e1_body.pushSlice(entry1_exts.slice());

    // Entry 2: serial 0x05, reason = none
    let e2_body = DerBuf {};
    e2_body.pushTlv(0x02, "\x05");
    e2_body.pushTlv(0x17, "261121081500Z");

    let rev_list_body = DerBuf {};
    rev_list_body.pushTlv(0x30, e1_body.slice());
    rev_list_body.pushTlv(0x30, e2_body.slice());
    tbs_body.pushTlv(0x30, rev_list_body.slice());

    // Optional crlExtensions [0] EXPLICIT SEQUENCE (only valid on v2)
    if (v2) {
        let crl_ext_body = DerBuf {};
        crl_ext_body.pushTlv(0x06, "\x55\x1D\x14"); // cRLNumber
        crl_ext_body.pushTlv(0x04, "\x02\x01\x2A"); // OCTET STRING { INTEGER 42 }
        let crl_ext = DerBuf {};
        crl_ext.pushTlv(0x30, crl_ext_body.slice());
        let crl_exts_seq = DerBuf {};
        crl_exts_seq.pushTlv(0x30, crl_ext.slice());
        tbs_body.pushTlv(0xA0, crl_exts_seq.slice());
    }

    let outer_body = DerBuf {};
    outer_body.pushTlv(0x30, tbs_body.slice());
    const outer_alg = if (match_sig_alg) buildAlgId(OID_SHA256_WITH_RSA) else buildAlgId(OID_ECDSA_SHA256);
    outer_body.pushSlice(outer_alg.slice());
    outer_body.pushTlv(0x03, "\x00\xCA\xFE\xBA\xBE\x10\x20\x30\x40");

    let crl = DerBuf {};
    crl.pushTlv(0x30, outer_body.slice());
    return crl;
}

fn buildCertId(serial: []const u8): DerBuf {
    const hash_alg = buildAlgId(OID_SHA256);
    let body = DerBuf {};
    body.pushSlice(hash_alg.slice());
    body.pushTlv(0x04, "0123456789ABCDEF0123456789ABCDEF"); // 32-byte issuerNameHash
    body.pushTlv(0x04, "FEDCBA9876543210FEDCBA9876543210"); // 32-byte issuerKeyHash
    body.pushTlv(0x02, serial);
    let out = DerBuf {};
    out.pushTlv(0x30, body.slice());
    return out;
}

fn buildSampleOcspRequest(): DerBuf {
    const cid1 = buildCertId("\x01\x02\x03\x04");
    const cid2 = buildCertId("\x00\x99\x88\x77\x66\x55\x44\x33\x22\x11\x00\xAA\xBB\xCC\xDD\xEE\xFF");

    let req1 = DerBuf {};
    req1.pushTlv(0x30, cid1.slice());
    let req2 = DerBuf {};
    req2.pushTlv(0x30, cid2.slice());

    let req_list_body = DerBuf {};
    req_list_body.pushSlice(req1.slice());
    req_list_body.pushSlice(req2.slice());

    let tbs_body = DerBuf {};
    tbs_body.pushTlv(0x30, req_list_body.slice());

    let outer_body = DerBuf {};
    outer_body.pushTlv(0x30, tbs_body.slice());

    let out = DerBuf {};
    out.pushTlv(0x30, outer_body.slice());
    return out;
}

fn buildSampleOcspResponse(): DerBuf {
    // SingleResponse 1: good
    const cid1 = buildCertId("\x01\x02\x03\x04");
    let sr1_body = DerBuf {};
    sr1_body.pushSlice(cid1.slice());
    sr1_body.pushTlv(0x80, ""); // good [0] IMPLICIT NULL
    sr1_body.pushTlv(0x18, "20261124000000Z"); // thisUpdate
    let next1 = DerBuf {};
    next1.pushTlv(0x18, "20261125000000Z");
    sr1_body.pushTlv(0xA0, next1.slice()); // nextUpdate

    // SingleResponse 2: revoked with reason key_compromise (1)
    const cid2 = buildCertId("\x00\x99\x88\x77\x66\x55\x44\x33\x22\x11\x00\xAA\xBB\xCC\xDD\xEE\xFF");
    let rev_info = DerBuf {};
    rev_info.pushTlv(0x18, "20261120000000Z");
    let reason_en = DerBuf {};
    reason_en.pushTlv(0x0A, "\x01");
    rev_info.pushTlv(0xA0, reason_en.slice());
    let sr2_body = DerBuf {};
    sr2_body.pushSlice(cid2.slice());
    sr2_body.pushTlv(0xA1, rev_info.slice()); // revoked
    sr2_body.pushTlv(0x18, "20261124000000Z");

    // SingleResponse 3: unknown
    const cid3 = buildCertId("\x7F");
    let sr3_body = DerBuf {};
    sr3_body.pushSlice(cid3.slice());
    sr3_body.pushTlv(0x82, ""); // unknown
    sr3_body.pushTlv(0x18, "20261124000000Z");

    let responses_body = DerBuf {};
    responses_body.pushTlv(0x30, sr1_body.slice());
    responses_body.pushTlv(0x30, sr2_body.slice());
    responses_body.pushTlv(0x30, sr3_body.slice());

    // ResponseData
    let key_hash_oct = DerBuf {};
    key_hash_oct.pushTlv(0x04, "0123456789ABCDEFGHIJ"); // 20-byte SHA-1 key hash

    let rd_body = DerBuf {};
    rd_body.pushTlv(0xA2, key_hash_oct.slice()); // responderID byKey
    rd_body.pushTlv(0x18, "20261124000100Z"); // producedAt
    rd_body.pushTlv(0x30, responses_body.slice());

    // BasicOCSPResponse
    let basic_body = DerBuf {};
    basic_body.pushTlv(0x30, rd_body.slice());
    const sig_alg = buildAlgId(OID_ECDSA_SHA256);
    basic_body.pushSlice(sig_alg.slice());
    basic_body.pushTlv(0x03, "\x00\xAA\xBB\xCC\xDD\xEE\xFF\x11\x22");
    let basic_seq = DerBuf {};
    basic_seq.pushTlv(0x30, basic_body.slice());

    // ResponseBytes
    let rb_body = DerBuf {};
    rb_body.pushTlv(0x06, OID_OCSP_BASIC);
    rb_body.pushTlv(0x04, basic_seq.slice());
    let rb_seq = DerBuf {};
    rb_seq.pushTlv(0x30, rb_body.slice());

    // OCSPResponse
    let outer_body = DerBuf {};
    outer_body.pushTlv(0x0A, "\x00"); // successful (0)
    outer_body.pushTlv(0xA0, rb_seq.slice());

    let out = DerBuf {};
    out.pushTlv(0x30, outer_body.slice());
    return out;
}

// ---------------------------------------------------------------------------
// Test 1: CSR Strict Parsing & Negative DER/RFC 2986 Rejection
// ---------------------------------------------------------------------------

fn runTestCsrStrict(): void! {
    const csr_buf = buildSampleCsr();
    const der = csr_buf.slice();

    expect_true(isCsrDer(der))?;
    expect_true(validateCsrStrict(der))?;

    const parsed = parseCsrDer(der).?;
    expect_eq(parsed.version, 0 as u64);
    expect_true(simdPkiBytesEqual(parsed.commonName().?, "api.example.com"))?;
    expect_true(simdPkiBytesEqual(parsed.organization().?, "Example Corp"))?;
    expect_true(simdPkiBytesEqual(parsed.country().?, "US"))?;
    expect_true(simdPkiBytesEqual(parsed.sig_alg_oid, OID_SHA256_WITH_RSA))?;
    expect_true(simdPkiBytesEqual(parsed.pub_alg_oid, OID_RSA_ENCRYPTION))?;

    // Check SAN entries
    let san_it = parsed.sans();
    const s1 = san_it.next().?;
    expect_eq(s1.tag_num, 2 as u8);
    expect_true(simdPkiBytesEqual(s1.value, "api.example.com"))?;
    const s2 = san_it.next().?;
    expect_eq(s2.tag_num, 2 as u8);
    expect_true(simdPkiBytesEqual(s2.value, "*.example.com"))?;
    expect_true(san_it.next() == null)?;

    // Reject trailing garbage byte
    let trailing = DerBuf {};
    trailing.pushSlice(der);
    trailing.pushByte(0x00);
    expect_true(parseCsrDer(trailing.slice()) == null)?;
    expect_false(validateCsrStrict(trailing.slice()))?;

    // Reject truncated DER
    expect_true(parseCsrDer(der[0 .. der.len - 1]) == null)?;
    expect_false(validateCsrStrict(der[0 .. der.len / 2]))?;

    // Reject indefinite length (0x80)
    const indef: []const u8 = "\x30\x80\x02\x01\x00\x00\x00";
    expect_true(parseCsrDer(indef) == null)?;
    expect_false(validateCsrStrict(indef))?;

    // Reject non-canonical long-form length (< 128)
    const non_canon_len: []const u8 = "\x30\x81\x03\x02\x01\x00";
    expect_true(parseCsrDer(non_canon_len) == null)?;
    expect_false(validateCsrStrict(non_canon_len))?;

    // Reject CSR with version != 0 (mutate version byte from 0x00 to 0x01)
    let bad_ver = csr_buf;
    let found_ver = false;
    let idx: usize = 0;
    while (idx + 3 <= bad_ver.len) : (idx += 1) {
        if (bad_ver.data[idx] == 0x02 && bad_ver.data[idx + 1] == 0x01 && bad_ver.data[idx + 2] == 0x00) {
            bad_ver.data[idx + 2] = 0x01;
            found_ver = true;
            break;
        }
    }
    expect_true(found_ver)?;
    expect_true(parseCsrDer(bad_ver.slice()) == null)?;
    expect_false(validateCsrStrict(bad_ver.slice()))?;

    // Reject excessive nesting depth (> CSR_MAX_DEPTH)
    let deep: [64]u8 = [0; 64];
    let d: usize = 0;
    while (d < CSR_MAX_DEPTH + 4) : (d += 1) {
        deep[d * 2] = 0x30;
        deep[d * 2 + 1] = ((CSR_MAX_DEPTH + 3 - d) * 2) as u8;
    }
    expect_false(validateCsrStrict(deep[0 .. (CSR_MAX_DEPTH + 4) * 2]))?;
}

// ---------------------------------------------------------------------------
// Test 2: CRL Strict Parsing, SIMD Revocation Lookup & Negative Rejection
// ---------------------------------------------------------------------------

fn runTestCrlStrict(): void! {
    const crl_v2 = buildSampleCrl(true, true);
    const der2 = crl_v2.slice();

    expect_true(isCrlDer(der2))?;
    expect_true(validateCrlStrict(der2))?;

    const parsed = parseCrlDer(der2).?;
    expect_eq(parsed.version, 1 as u8);
    expect_true(simdPkiBytesEqual(parsed.this_update, "261124000000Z"))?;
    expect_true(simdPkiBytesEqual(parsed.next_update, "20261224000000Z"))?;

    // SIMD serial lookup (both with and without leading 0x00 sign byte)
    const long_serial_raw: []const u8 = "\x80\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10";
    const long_serial_padded: []const u8 = "\x00\x80\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10";
    expect_true(parsed.isSerialRevoked(long_serial_raw))?;
    expect_true(parsed.isSerialRevoked(long_serial_padded))?;
    expect_true(parsed.isSerialRevoked("\x05"))?;
    expect_false(parsed.isSerialRevoked("\x06"))?;

    const found = parsed.findRevokedEntry(long_serial_raw).?;
    expect_eq(found.reason, CrlReason.key_compromise);
    expect_true(simdPkiBytesEqual(found.revocation_date, "261120123000Z"))?;

    const stats = summarizeCrl(der2).?;
    expect_eq(stats.version, 1 as u8);
    expect_eq(stats.revoked_count, 2 as u32);
    expect_eq(stats.key_compromise_count, 1 as u32);
    expect_true(stats.has_next_update)?;

    // Valid v1 CRL (no version field, no extensions)
    const crl_v1 = buildSampleCrl(false, true);
    expect_true(validateCrlStrict(crl_v1.slice()))?;
    const p1 = parseCrlDer(crl_v1.slice()).?;
    expect_eq(p1.version, 0 as u8);

    // Reject mismatched TBS vs outer signatureAlgorithm OID in validateCrlStrict
    const crl_mismatch = buildSampleCrl(true, false);
    expect_false(validateCrlStrict(crl_mismatch.slice()))?;

    // Reject trailing garbage & truncation
    let trailing = DerBuf {};
    trailing.pushSlice(der2);
    trailing.pushByte(0xFF);
    expect_true(parseCrlDer(trailing.slice()) == null)?;
    expect_false(validateCrlStrict(trailing.slice()))?;
    expect_false(validateCrlStrict(der2[0 .. der2.len - 2]))?;

    // Reject negative serial number in revokedCertificates (0xFF)
    let neg_crl = crl_v2;
    let mutated = false;
    let k: usize = 0;
    while (k + 3 <= neg_crl.len) : (k += 1) {
        if (neg_crl.data[k] == 0x02 && neg_crl.data[k + 1] == 0x01 && neg_crl.data[k + 2] == 0x05) {
            neg_crl.data[k + 2] = 0xFF; // negative INTEGER -1
            mutated = true;
            break;
        }
    }
    expect_true(mutated)?;
    expect_false(validateCrlStrict(neg_crl.slice()))?;
    expect_true(summarizeCrl(neg_crl.slice()) == null)?;
}

// ---------------------------------------------------------------------------
// Test 3: OCSP Request/Response Strict Parsing, Reserved Status 4 & CertID SIMD
// ---------------------------------------------------------------------------

fn runTestOcspStrict(): void! {
    // 1. OCSPRequest
    const req_buf = buildSampleOcspRequest();
    const req_der = req_buf.slice();
    expect_true(validateOcspRequestStrict(req_der))?;
    expect_true(validateOcspStrict(req_der))?;

    const req = parseOcspRequestDer(req_der).?;
    expect_eq(req.version, 0 as u64);
    expect_false(req.has_signature)?;
    let rit = req.requests();
    const cid1 = rit.next().?;
    const cid2 = rit.next().?;
    expect_true(rit.next() == null)?;
    expect_false(rit.hasError())?;
    expect_true(simdPkiBytesEqual(cid1.serial_bytes, "\x01\x02\x03\x04"))?;

    // 2. OCSPResponse (successful + BasicOCSPResponse)
    const resp_buf = buildSampleOcspResponse();
    const resp_der = resp_buf.slice();
    expect_true(validateOcspResponseStrict(resp_der))?;
    expect_true(validateOcspStrict(resp_der))?;

    const resp = parseOcspResponseDer(resp_der).?;
    expect_eq(resp.status, OcspResponseStatus.successful);
    expect_true(resp.is_basic)?;
    expect_true(resp.responder_is_by_key)?;
    expect_true(simdPkiBytesEqual(resp.produced_at, "20261124000100Z"))?;

    // SIMD CertID matching across request <-> response
    const sr1 = resp.findSingleResponse(cid1).?;
    expect_eq(sr1.cert_status, OcspCertStatus.good);
    expect_true(sr1.cert_id.equals(cid1))?;
    expect_true(ocspCertIdMatches(sr1.cert_id, cid1))?;
    expect_false(ocspCertIdMatches(sr1.cert_id, cid2))?;

    const sr2 = resp.findSingleResponse(cid2).?;
    expect_eq(sr2.cert_status, OcspCertStatus.revoked);
    expect_eq(sr2.revocation_reason, 1 as u8);
    expect_true(simdPkiBytesEqual(sr2.revocation_time, "20261120000000Z"))?;

    const miss_cid = OcspCertId {
        hash_alg_oid: OID_SHA256,
        issuer_name_hash: "0123456789ABCDEF0123456789ABCDEF",
        issuer_key_hash: "FEDCBA9876543210FEDCBA9876543210",
        serial_bytes: "\x42",
    };
    expect_true(resp.findSingleResponse(miss_cid) == null)?;

    // 3. Valid non-successful statuses: 1, 2, 3, 5, 6
    const valid_err_statuses: [5]u8 = [1, 2, 3, 5, 6];
    let si: usize = 0;
    while (si < valid_err_statuses.len) : (si += 1) {
        const st_byte = valid_err_statuses[si];
        let eb = DerBuf {};
        let inner = DerBuf {};
        const st_arr: [1]u8 = [st_byte];
        inner.pushTlv(0x0A, st_arr[0..]);
        eb.pushTlv(0x30, inner.slice());
        expect_true(validateOcspResponseStrict(eb.slice()))?;
        const er = parseOcspResponseDer(eb.slice()).?;
        expect_eq(er.status, ocspResponseStatusFromU8(st_byte));
        expect_false(er.is_basic)?;
    }

    // 4. Reject reserved status 4 and out-of-range status 7
    expect_eq(ocspResponseStatusFromU8(4), OcspResponseStatus.unknown);
    expect_eq(ocspResponseStatusFromU8(7), OcspResponseStatus.unknown);
    const reserved4: []const u8 = "\x30\x03\x0A\x01\x04";
    expect_true(parseOcspResponseDer(reserved4) == null)?;
    expect_false(validateOcspStrict(reserved4))?;
    const out_of_range7: []const u8 = "\x30\x03\x0A\x01\x07";
    expect_true(parseOcspResponseDer(out_of_range7) == null)?;
    expect_false(validateOcspStrict(out_of_range7))?;

    // 5. Reject successful(0) missing responseBytes in strict validator
    const succ_no_bytes: []const u8 = "\x30\x03\x0A\x01\x00";
    expect_false(validateOcspResponseStrict(succ_no_bytes))?;
    expect_false(validateOcspStrict(succ_no_bytes))?;

    // 6. Reject trailing garbage & indefinite length
    let trailing = DerBuf {};
    trailing.pushSlice(resp_der);
    trailing.pushByte(0x00);
    expect_false(validateOcspStrict(trailing.slice()))?;
    expect_false(validateOcspStrict("\x30\x80\x0A\x01\x03\x00\x00"))?;
}

// ---------------------------------------------------------------------------
// Test 4: 16-Lane SIMD vs Scalar 0-Diff Differential Verification
// ---------------------------------------------------------------------------

fn runTestSimdZeroDiff(): void! {
    let a: [96]u8 = [0; 96];
    let b: [96]u8 = [0; 96];
    let dst_simd: [96]u8 = [0; 96];
    let dst_scalar: [96]u8 = [0; 96];

    let i: usize = 0;
    while (i < 96) : (i += 1) {
        a[i] = ((i * 37 + 11) & 0xFF) as u8;
        b[i] = a[i];
    }

    let len: usize = 0;
    while (len <= 96) : (len += 1) {
        // Equal slices
        expect_eq(simdPkiBytesEqual(a[0..len], b[0..len]), scalarPkiBytesEqual(a[0..len], b[0..len]));
        expect_true(simdPkiBytesEqual(a[0..len], b[0..len]))?;

        // Copy equality
        let k: usize = 0;
        while (k < 96) : (k += 1) {
            dst_simd[k] = 0xAA;
            dst_scalar[k] = 0xAA;
        }
        simdPkiCopyBytes(dst_simd[0..len], a[0..len]);
        scalarPkiCopyBytes(dst_scalar[0..len], a[0..len]);
        expect_true(scalarPkiBytesEqual(dst_simd[0..96], dst_scalar[0..96]))?;

        // Single-byte mismatch at every position for representative lengths
        if (len > 0 && (len <= 34 || len == 48 || len == 64 || len == 96)) {
            let pos: usize = 0;
            while (pos < len) : (pos += 1) {
                b[pos] ^= 0x01;
                expect_eq(simdPkiBytesEqual(a[0..len], b[0..len]), scalarPkiBytesEqual(a[0..len], b[0..len]));
                expect_false(simdPkiBytesEqual(a[0..len], b[0..len]))?;
                b[pos] ^= 0x01;
            }
        }
    }
    // Different lengths
    expect_eq(simdPkiBytesEqual(a[0..16], b[0..17]), scalarPkiBytesEqual(a[0..16], b[0..17]));
    expect_false(simdPkiBytesEqual(a[0..16], b[0..17]))?;
}

// ---------------------------------------------------------------------------
// Test 5: 1,800-Case Deterministic PRNG Fuzz (600 CSR + 600 CRL + 600 OCSP)
// ---------------------------------------------------------------------------

struct XorShift64 {
    state: u64 = 0x1E3779B97F4A7C15,

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

fn runTestPkiFuzz1800(): void! {
    let rng = XorShift64 { state: 0x36A7F11E20261124 };
    const seed_csr = buildSampleCsr();
    const seed_crl = buildSampleCrl(true, true);
    const seed_ocsp_resp = buildSampleOcspResponse();
    const seed_ocsp_req = buildSampleOcspRequest();

    let buf: [512]u8 = [0; 512];

    // 600 CSR fuzz cases
    let c: usize = 0;
    while (c < 600) : (c += 1) {
        let flen: usize = 0;
        if ((c & 1) == 0) {
            // Mutate valid CSR
            flen = seed_csr.len;
            simdPkiCopyBytes(buf[0..flen], seed_csr.slice());
            const mode = rng.next() % 4;
            if (mode == 0) {
                const p = (rng.next() as usize) % flen;
                buf[p] ^= ((rng.next() & 0xFF) as u8) | 1;
            } else if (mode == 1) {
                flen = (rng.next() as usize) % flen;
            } else if (mode == 2 && flen + 1 < buf.len) {
                buf[flen] = (rng.next() & 0xFF) as u8;
                flen += 1;
            } else {
                const p = (rng.next() as usize) % flen;
                buf[p] = 0x80;
            }
        } else {
            // Pure random bytes
            flen = (rng.next() as usize) % 256;
            let j: usize = 0;
            while (j < flen) : (j += 1) {
                buf[j] = (rng.next() & 0xFF) as u8;
            }
        }
        const sl = buf[0..flen];
        const strict_ok = validateCsrStrict(sl);
        const parsed = parseCsrDer(sl);
        if (strict_ok) {
            expect_true(parsed != null)?;
        }
        if (p := parsed) {
            * = p.commonName();
            let sit = p.sans();
            while (s := sit.next()) {
                * = s;
            }
        }
    }

    // 600 CRL fuzz cases
    let r: usize = 0;
    while (r < 600) : (r += 1) {
        let flen: usize = 0;
        if ((r & 1) == 0) {
            flen = seed_crl.len;
            simdPkiCopyBytes(buf[0..flen], seed_crl.slice());
            const mode = rng.next() % 4;
            if (mode == 0) {
                const p = (rng.next() as usize) % flen;
                buf[p] ^= ((rng.next() & 0xFF) as u8) | 1;
            } else if (mode == 1) {
                flen = (rng.next() as usize) % flen;
            } else if (mode == 2 && flen + 1 < buf.len) {
                buf[flen] = (rng.next() & 0xFF) as u8;
                flen += 1;
            } else {
                const p = (rng.next() as usize) % flen;
                buf[p] = 0x80;
            }
        } else {
            flen = (rng.next() as usize) % 256;
            let j: usize = 0;
            while (j < flen) : (j += 1) {
                buf[j] = (rng.next() & 0xFF) as u8;
            }
        }
        const sl = buf[0..flen];
        const strict_ok = validateCrlStrict(sl);
        const parsed = parseCrlDer(sl);
        if (strict_ok) {
            expect_true(parsed != null)?;
            expect_true(summarizeCrl(sl) != null)?;
        }
        if (p := parsed) {
            * = p.isSerialRevoked("\x05");
        }
    }

    // 600 OCSP fuzz cases (300 response-seeded/random + 300 request-seeded/random)
    let o: usize = 0;
    while (o < 600) : (o += 1) {
        let flen: usize = 0;
        if ((o % 3) == 0) {
            flen = seed_ocsp_resp.len;
            simdPkiCopyBytes(buf[0..flen], seed_ocsp_resp.slice());
            const p = (rng.next() as usize) % flen;
            buf[p] ^= ((rng.next() & 0xFF) as u8) | 1;
        } else if ((o % 3) == 1) {
            flen = seed_ocsp_req.len;
            simdPkiCopyBytes(buf[0..flen], seed_ocsp_req.slice());
            const mode = rng.next() % 2;
            if (mode == 0) {
                const p = (rng.next() as usize) % flen;
                buf[p] ^= ((rng.next() & 0xFF) as u8) | 1;
            } else {
                flen = (rng.next() as usize) % flen;
            }
        } else {
            flen = (rng.next() as usize) % 256;
            let j: usize = 0;
            while (j < flen) : (j += 1) {
                buf[j] = (rng.next() & 0xFF) as u8;
            }
        }
        const sl = buf[0..flen];
        const strict_ok = validateOcspStrict(sl);
        const presp = parseOcspResponseDer(sl);
        const preq = parseOcspRequestDer(sl);
        if (strict_ok) {
            expect_true(presp != null || preq != null)?;
        }
        if (rp := presp) {
            let sit = rp.singleResponses();
            while (sr := sit.next()) {
                * = sr;
            }
        }
        if (rq := preq) {
            let qit = rq.requests();
            while (q := qit.next()) {
                * = q;
            }
        }
    }
}

fn main() {
    test("RFC 2986 CSR strict DER parsing & negative rejection", () => {
        runTestCsrStrict()?;
    });
    test("RFC 5280 CRL strict DER parsing, SIMD serial lookup & rejection", () => {
        runTestCrlStrict()?;
    });
    test("RFC 6960 OCSP strict DER parsing, reserved status 4 & CertID SIMD", () => {
        runTestOcspStrict()?;
    });
    test("16-lane SIMD vs Scalar 0-diff differential verification", () => {
        runTestSimdZeroDiff()?;
    });
    test("1800-case deterministic PRNG Fuzz (600 CSR + 600 CRL + 600 OCSP)", () => {
        runTestPkiFuzz1800()?;
    });
}
