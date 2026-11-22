// Source: cp/cap/test/test_crl_production_deep.cp
// Deep Production Test Suite for cap:crl
// Covers:
//   1. Complete X.509 v2 CRL (nextUpdate, multiple revoked entries with CRLReason, [0] extensions)
//   2. X.509 v1 CRL (omitted version=0, omitted nextUpdate, empty revoked list)
//   3. isSerialRevoked positive/negative lookup including high-bit serial with DER 0x00 sign pad
//   4. GeneralizedTime (0x18) & UTCTime (0x17) interoperability
//   5. Non-DER, trailing garbage, non-zero BIT STRING unused_bits & truncated DER guards

import { test, expect_eq } from "cap:test";
import {
    OID_CRL_NUMBER,
    OID_CRL_REASON,
    CrlReason,
    crlReasonFromU8,
    CrlRevokedEntry,
    CrlParsed,
    CrlStats,
    CrlRevokedIter,
    parseCrlDer,
    isCrlDer,
    summarizeCrl,
} from "cap:auth";

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

fn buildRevokedEntry(serial_der: []const u8, time_tag: u8, time_str: []const u8, with_reason: bool, reason_code: u8): DerBuf {
    let body = DerBuf {};
    body.pushTlv(0x02, serial_der);
    body.pushTlv(time_tag, time_str);
    if (with_reason) {
        let r_arr: [1]u8 = [reason_code];
        let en = DerBuf {};
        en.pushTlv(0x0A, r_arr[0..1]); // ENUMERATED
        let ext_body = DerBuf {};
        ext_body.pushTlv(0x06, OID_CRL_REASON);
        ext_body.pushTlv(0x04, en.slice());
        let exts = DerBuf {};
        exts.pushTlv(0x30, ext_body.slice());
        body.pushTlv(0x30, exts.slice());
    }
    let seq = DerBuf {};
    seq.pushTlv(0x30, body.slice());
    return seq;
}

fn buildCrl(
    include_ver: bool,
    ver: u8,
    this_tag: u8,
    this_upd: []const u8,
    next_tag: u8,
    next_upd: []const u8,
    revoked_items: []const u8,
    crl_exts: []const u8,
    sig_unused: u8,
    sig_bytes: []const u8,
): DerBuf {
    let tbs = DerBuf {};
    if (include_ver) {
        let v_arr: [1]u8 = [ver];
        tbs.pushTlv(0x02, v_arr[0..1]);
    }

    // signature AlgorithmIdentifier
    let alg_body = DerBuf {};
    alg_body.pushTlv(0x06, OID_SHA256_RSA);
    alg_body.pushTlv(0x05, "");
    tbs.pushTlv(0x30, alg_body.slice());

    // issuer Name SEQUENCE
    tbs.pushTlv(0x30, "");

    // thisUpdate
    tbs.pushTlv(this_tag, this_upd);

    // Optional nextUpdate
    if (next_upd.len > 0) {
        tbs.pushTlv(next_tag, next_upd);
    }

    // Optional revokedCertificates
    if (revoked_items.len > 0) {
        tbs.pushTlv(0x30, revoked_items);
    }

    // Optional crlExtensions [0] EXPLICIT
    if (crl_exts.len > 0) {
        tbs.pushTlv(0xA0, crl_exts);
    }

    let outer = DerBuf {};
    outer.pushTlv(0x30, tbs.slice());
    outer.pushTlv(0x30, alg_body.slice());
    outer.pushBitString(sig_unused, sig_bytes);

    let final_der = DerBuf {};
    final_der.pushTlv(0x30, outer.slice());
    return final_der;
}

test("crl: complete X.509 v2 CRL with nextUpdate, CRLReason entries & [0] extensions") {
    let rev = DerBuf {};
    const e1 = buildRevokedEntry("\x01\x02\x03", 0x17, "261001120000Z", true, 1); // key_compromise
    const e2 = buildRevokedEntry("\x00\x8F\x12\x34", 0x17, "261015000000Z", true, 4); // superseded
    const e3 = buildRevokedEntry("\x7A", 0x17, "261101000000Z", false, 0); // none
    rev.pushBytes(e1.slice());
    rev.pushBytes(e2.slice());
    rev.pushBytes(e3.slice());

    // CRL extension [0] containing OID_CRL_NUMBER
    let ext_num = DerBuf {};
    ext_num.pushTlv(0x06, OID_CRL_NUMBER);
    ext_num.pushTlv(0x04, "\x02\x01\x05");
    let exts_seq = DerBuf {};
    exts_seq.pushTlv(0x30, ext_num.slice());
    let exts_wrap = DerBuf {};
    exts_wrap.pushTlv(0x30, exts_seq.slice());

    const der = buildCrl(
        true,
        1,
        0x17,
        "261121000000Z",
        0x17,
        "261221000000Z",
        rev.slice(),
        exts_wrap.slice(),
        0,
        "\xDE\xAD\xBE\xEF",
    );

    expect_eq(isCrlDer(der.slice()), true);
    if (crl := parseCrlDer(der.slice())) {
        expect_eq(crl.version, 1);
        expect_eq(crl.tbs_raw_der[0], 0x30);
        expect_eq(strEq(crl.tbs_sig_alg_oid, OID_SHA256_RSA), true);
        expect_eq(strEq(crl.sig_alg_oid, OID_SHA256_RSA), true);
        expect_eq(strEq(crl.this_update, "261121000000Z"), true);
        expect_eq(strEq(crl.next_update, "261221000000Z"), true);
        expect_eq(crl.extensions_der.len > 0, true);
        expect_eq(strEq(crl.signature, "\xDE\xAD\xBE\xEF"), true);

        let it = crl.revoked();
        let idx: u32 = 0;
        while (entry := it.next()) {
            idx += 1;
            if (idx == 1) {
                expect_eq(strEq(entry.serial_bytes, "\x01\x02\x03"), true);
                expect_eq(entry.reason == CrlReason.key_compromise, true);
            } else if (idx == 2) {
                // Leading 0x00 DER sign byte stripped!
                expect_eq(strEq(entry.serial_bytes, "\x8F\x12\x34"), true);
                expect_eq(entry.reason == CrlReason.superseded, true);
            } else if (idx == 3) {
                expect_eq(strEq(entry.serial_bytes, "\x7A"), true);
                expect_eq(entry.reason == CrlReason.none, true);
            }
        }
        expect_eq(idx, 3);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }

    if (stats := summarizeCrl(der.slice())) {
        expect_eq(stats.version, 1);
        expect_eq(stats.revoked_count, 3);
        expect_eq(stats.key_compromise_count, 1);
        expect_eq(stats.has_next_update, true);
    } else {
        expect_eq(true, false);
    }
}

test("crl: X.509 v1 CRL with omitted version, no nextUpdate & empty revoked list") {
    const der = buildCrl(false, 0, 0x17, "261121000000Z", 0x17, "", "", "", 0, "\x01\x02");

    if (crl := parseCrlDer(der.slice())) {
        expect_eq(crl.version, 0);
        expect_eq(crl.next_update.len, 0);
        expect_eq(crl.revoked_list_der.len, 0);
        expect_eq(crl.isSerialRevoked("\x01"), false);
    } else {
        expect_eq(true, false);
    }

    if (stats := summarizeCrl(der.slice())) {
        expect_eq(stats.version, 0);
        expect_eq(stats.revoked_count, 0);
        expect_eq(stats.key_compromise_count, 0);
        expect_eq(stats.has_next_update, false);
    } else {
        expect_eq(true, false);
    }
}

test("crl: isSerialRevoked positive/negative lookup with DER leading 0x00 normalization") {
    let rev = DerBuf {};
    const e1 = buildRevokedEntry("\x00\xFF\xEE\xDD", 0x17, "261001000000Z", false, 0);
    rev.pushBytes(e1.slice());

    const der = buildCrl(true, 1, 0x17, "261121000000Z", 0x17, "", rev.slice(), "", 0, "\xAA");
    if (crl := parseCrlDer(der.slice())) {
        expect_eq(crl.isSerialRevoked("\xFF\xEE\xDD"), true);
        expect_eq(crl.isSerialRevoked("\x00\xFF\xEE\xDD"), true);
        expect_eq(crl.isSerialRevoked("\xFF\xEE\xDC"), false);
        expect_eq(crl.isSerialRevoked(""), false);
    } else {
        expect_eq(true, false);
    }
}

test("crl: GeneralizedTime (0x18) & UTCTime (0x17) interoperability & reason enum") {
    let rev = DerBuf {};
    const e1 = buildRevokedEntry("\x42", 0x18, "20500615083000Z", true, 8); // remove_from_crl
    rev.pushBytes(e1.slice());

    const der = buildCrl(true, 1, 0x18, "20500101000000Z", 0x18, "20501231235959Z", rev.slice(), "", 0, "\xBB");
    if (crl := parseCrlDer(der.slice())) {
        expect_eq(strEq(crl.this_update, "20500101000000Z"), true);
        expect_eq(strEq(crl.next_update, "20501231235959Z"), true);
        let it = crl.revoked();
        if (entry := it.next()) {
            expect_eq(strEq(entry.revocation_date, "20500615083000Z"), true);
            expect_eq(entry.reason == CrlReason.remove_from_crl, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
    expect_eq(crlReasonFromU8(10) == CrlReason.aa_compromise, true);
    expect_eq(crlReasonFromU8(7) == CrlReason.none, true);
}

test("crl: non-DER, trailing garbage, non-zero BIT STRING unused_bits & invalid version guards") {
    expect_eq(isCrlDer("not a crl"), false);

    // Non-zero unused_bits on signatureValue BIT STRING
    const bad_unused = buildCrl(true, 1, 0x17, "261121000000Z", 0x17, "", "", "", 2, "\xAA");
    expect_eq(isCrlDer(bad_unused.slice()), false);

    // Invalid version (v3 = 2)
    const bad_ver = buildCrl(true, 2, 0x17, "261121000000Z", 0x17, "", "", "", 0, "\xAA");
    expect_eq(isCrlDer(bad_ver.slice()), false);

    // Trailing garbage byte
    let valid = buildCrl(true, 1, 0x17, "261121000000Z", 0x17, "", "", "", 0, "\xAA");
    valid.push(0x99);
    expect_eq(isCrlDer(valid.slice()), false);

    // Truncated DER
    expect_eq(isCrlDer(valid.slice()[0..10]), false);
}
