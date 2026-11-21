// Source: cp/cap/test/test_pkcs7_production_deep.cp
// Deep Production Test Suite for cap:pkcs7
// Covers:
//   1. Attached SignedData + Pkcs7CertIter extracting multiple DER X.509 certificates
//   2. Detached SignedData (CMS v3, subjectKeyIdentifier [0], crls [1])
//   3. Pkcs7SignerIter extracting SHA-256 + RSA (with signedAttrs [0]) & ECDSA SignerInfos
//   4. Other ContentInfo OIDs (Data, EnvelopedData, DigestedData, EncryptedData, unknown)
//   5. Non-DER input, truncated TLV, indefinite BER length (0x80), wrong OID & trailing bytes guards

import { test, expect_eq } from "cap:test";
import {
    OID_DATA,
    OID_SIGNED_DATA,
    OID_ENVELOPED_DATA,
    OID_DIGESTED_DATA,
    OID_ENCRYPTED_DATA,
    Pkcs7ContentType,
    Pkcs7ContentInfo,
    Pkcs7SignedData,
    Pkcs7SignerInfo,
    Pkcs7CertIter,
    Pkcs7SignerIter,
    parseContentInfo,
    isPkcs7SignedData,
    parseSignedData,
} from "cap:pkcs7";

// Standard algorithm OIDs for tests
const OID_SHA256: []const u8 = "\x60\x86\x48\x01\x65\x03\x04\x02\x01";
const OID_RSA_ENC: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01";
const OID_ECDSA_SHA256: []const u8 = "\x2A\x86\x48\xCE\x3D\x04\x03\x02";

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

fn buildAlgId(oid: []const u8): DerBuf {
    let inner = DerBuf {};
    inner.pushTlv(0x06, oid);
    inner.pushTlv(0x05, ""); // NULL params
    let seq = DerBuf {};
    seq.pushTlv(0x30, inner.slice());
    return seq;
}

fn buildSignerInfo(
    ver: u8,
    use_ski: bool,
    digest_oid: []const u8,
    with_signed_attrs: bool,
    sig_oid: []const u8,
    sig_bytes: []const u8,
): DerBuf {
    let body = DerBuf {};
    let v_arr: [1]u8 = [ver];
    body.pushTlv(0x02, v_arr[0..1]);

    if (use_ski) {
        body.pushTlv(0x80, "\xAA\xBB\xCC\xDD");
    } else {
        // Minimal IssuerAndSerialNumber SEQUENCE
        let ias = DerBuf {};
        ias.pushTlv(0x30, "");
        ias.pushTlv(0x02, "\x01");
        body.pushTlv(0x30, ias.slice());
    }

    const da = buildAlgId(digest_oid);
    body.pushBytes(da.slice());

    if (with_signed_attrs) {
        body.pushTlv(0xA0, "\x30\x02\x05\x00");
    }

    const sa = buildAlgId(sig_oid);
    body.pushBytes(sa.slice());

    body.pushTlv(0x04, sig_bytes);

    let seq = DerBuf {};
    seq.pushTlv(0x30, body.slice());
    return seq;
}

fn wrapContentInfo(oid: []const u8, content_der: []const u8): DerBuf {
    let body = DerBuf {};
    body.pushTlv(0x06, oid);
    if (content_der.len > 0) {
        body.pushTlv(0xA0, content_der);
    }
    let ci = DerBuf {};
    ci.pushTlv(0x30, body.slice());
    return ci;
}

test("pkcs7: attached SignedData + Pkcs7CertIter extracting 2 X.509 DER certs") {
    let sd_body = DerBuf {};
    sd_body.pushTlv(0x02, "\x01"); // version = 1

    // digestAlgorithms SET
    const da = buildAlgId(OID_SHA256);
    sd_body.pushTlv(0x31, da.slice());

    // encapContentInfo (attached "hello signed world")
    let eci = DerBuf {};
    eci.pushTlv(0x06, OID_DATA);
    let oct = DerBuf {};
    oct.pushTlv(0x04, "hello signed world");
    eci.pushTlv(0xA0, oct.slice());
    sd_body.pushTlv(0x30, eci.slice());

    // certificates [0] IMPLICIT with 2 dummy X.509 DER SEQUENCE certs
    let certs = DerBuf {};
    certs.pushTlv(0x30, "CERT-ONE-PAYLOAD-DATA");
    certs.pushTlv(0x30, "CERT-TWO-PAYLOAD-DATA-LONGER");
    sd_body.pushTlv(0xA0, certs.slice());

    // signerInfos SET with 1 signer
    const si = buildSignerInfo(1, false, OID_SHA256, true, OID_RSA_ENC, "\xDE\xAD\xBE\xEF");
    sd_body.pushTlv(0x31, si.slice());

    let sd_seq = DerBuf {};
    sd_seq.pushTlv(0x30, sd_body.slice());
    const ci = wrapContentInfo(OID_SIGNED_DATA, sd_seq.slice());

    expect_eq(isPkcs7SignedData(ci.slice()), true);
    if (sd := parseSignedData(ci.slice())) {
        expect_eq(sd.version, 1);
        expect_eq(sd.is_detached, false);
        expect_eq(strEq(sd.encap_content_type_oid, OID_DATA), true);
        expect_eq(strEq(sd.encap_content, "hello signed world"), true);

        let cit = sd.certs();
        let c_count: u32 = 0;
        while (c_der := cit.next()) {
            c_count += 1;
            expect_eq(c_der[0], 0x30);
            if (c_count == 1) {
                expect_eq(c_der.len, 2 + 21);
            } else if (c_count == 2) {
                expect_eq(c_der.len, 2 + 28);
            }
        }
        expect_eq(c_count, 2);
        expect_eq(cit.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("pkcs7: detached SignedData (CMS v3, SKI [0], CRLs [1])") {
    let sd_body = DerBuf {};
    sd_body.pushTlv(0x02, "\x03"); // version = 3

    const da = buildAlgId(OID_SHA256);
    sd_body.pushTlv(0x31, da.slice());

    // encapContentInfo WITHOUT [0] eContent -> detached!
    let eci = DerBuf {};
    eci.pushTlv(0x06, OID_DATA);
    sd_body.pushTlv(0x30, eci.slice());

    // Optional crls [1] IMPLICIT
    sd_body.pushTlv(0xA1, "\x30\x02\x05\x00");

    // signerInfos SET with v3 SKI signer
    const si = buildSignerInfo(3, true, OID_SHA256, false, OID_ECDSA_SHA256, "\x11\x22\x33");
    sd_body.pushTlv(0x31, si.slice());

    let sd_seq = DerBuf {};
    sd_seq.pushTlv(0x30, sd_body.slice());
    const ci = wrapContentInfo(OID_SIGNED_DATA, sd_seq.slice());

    if (sd := parseSignedData(ci.slice())) {
        expect_eq(sd.version, 3);
        expect_eq(sd.is_detached, true);
        expect_eq(sd.encap_content.len, 0);
        expect_eq(sd.certificates_der.len, 0);
        expect_eq(sd.crls_der.len, 4);
    } else {
        expect_eq(true, false);
    }
}

test("pkcs7: Pkcs7SignerIter extracts RSA (with signedAttrs) & ECDSA SignerInfos") {
    let si_set = DerBuf {};
    const s1 = buildSignerInfo(1, false, OID_SHA256, true, OID_RSA_ENC, "\x01\x02\x03\x04");
    const s2 = buildSignerInfo(3, true, OID_SHA256, false, OID_ECDSA_SHA256, "\x05\x06\x07\x08\x09");
    si_set.pushBytes(s1.slice());
    si_set.pushBytes(s2.slice());

    let it = Pkcs7SignerIter.init(si_set.slice());
    let idx: u32 = 0;
    while (si := it.next()) {
        idx += 1;
        expect_eq(strEq(si.digest_alg_oid, OID_SHA256), true);
        if (idx == 1) {
            expect_eq(si.version, 1);
            expect_eq(si.has_signed_attrs, true);
            expect_eq(strEq(si.sig_alg_oid, OID_RSA_ENC), true);
            expect_eq(strEq(si.signature, "\x01\x02\x03\x04"), true);
        } else if (idx == 2) {
            expect_eq(si.version, 3);
            expect_eq(si.has_signed_attrs, false);
            expect_eq(strEq(si.sig_alg_oid, OID_ECDSA_SHA256), true);
            expect_eq(strEq(si.signature, "\x05\x06\x07\x08\x09"), true);
        }
    }
    expect_eq(idx, 2);
    expect_eq(it.hasError(), false);
}

test("pkcs7: ContentInfo dispatch for Data, EnvelopedData, DigestedData, EncryptedData & unknown") {
    let oct = DerBuf {};
    oct.pushTlv(0x04, "payload");

    const c_data = wrapContentInfo(OID_DATA, oct.slice());
    expect_eq(isPkcs7SignedData(c_data.slice()), false);
    if (ci := parseContentInfo(c_data.slice())) {
        expect_eq(ci.content_type == Pkcs7ContentType.data, true);
        expect_eq(strEq(ci.content_der, oct.slice()), true);
    } else {
        expect_eq(true, false);
    }

    const c_env = wrapContentInfo(OID_ENVELOPED_DATA, "\x30\x00");
    if (ci := parseContentInfo(c_env.slice())) {
        expect_eq(ci.content_type == Pkcs7ContentType.enveloped_data, true);
    } else {
        expect_eq(true, false);
    }

    const c_dig = wrapContentInfo(OID_DIGESTED_DATA, "");
    if (ci := parseContentInfo(c_dig.slice())) {
        expect_eq(ci.content_type == Pkcs7ContentType.digested_data, true);
    } else {
        expect_eq(true, false);
    }

    const c_enc = wrapContentInfo(OID_ENCRYPTED_DATA, "");
    if (ci := parseContentInfo(c_enc.slice())) {
        expect_eq(ci.content_type == Pkcs7ContentType.encrypted_data, true);
    } else {
        expect_eq(true, false);
    }

    const c_unk = wrapContentInfo("\x2A\x86\x48\x86\xF7\x0D\x01\x07\x09", "");
    if (ci := parseContentInfo(c_unk.slice())) {
        expect_eq(ci.content_type == Pkcs7ContentType.unknown, true);
    } else {
        expect_eq(true, false);
    }
}

test("pkcs7: non-DER, truncated TLV, indefinite BER length & wrong OID guards") {
    // 1. Non-DER text
    expect_eq(isPkcs7SignedData("not der"), false);
    if (ci := parseContentInfo("not der")) {
        * = ci;
        expect_eq(true, false);
    }

    // 2. Truncated TLV (SEQUENCE claims 50 bytes, only 2 follow)
    const trunc = "\x30\x32\x06\x01";
    if (ci := parseContentInfo(trunc)) {
        * = ci;
        expect_eq(true, false);
    }

    // 3. Indefinite BER length (0x80) rejected in strict DER
    const ber_indef = "\x30\x80\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x07\x01\x00\x00";
    if (ci := parseContentInfo(ber_indef)) {
        * = ci;
        expect_eq(true, false);
    }

    // 4. Wrong ContentInfo OID passed to parseSignedData
    const c_data = wrapContentInfo(OID_DATA, "\x04\x02hi");
    if (sd := parseSignedData(c_data.slice())) {
        * = sd;
        expect_eq(true, false);
    }

    // 5. Trailing garbage after outer ContentInfo SEQUENCE
    let with_garbage = DerBuf {};
    with_garbage.pushBytes(c_data.slice());
    with_garbage.push(0xFF);
    if (ci := parseContentInfo(with_garbage.slice())) {
        * = ci;
        expect_eq(true, false);
    }
}
