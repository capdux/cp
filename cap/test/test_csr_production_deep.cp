// Source: cp/cap/test/test_csr_production_deep.cp
// Deep Production Test Suite for cap:csr
// Covers:
//   1. Complete Ed25519 PKCS#10 DER CSR parsing (cri_raw_der, pub_key_bytes, signature)
//   2. Multi-RDN Subject CN/O/OU/C/L/ST iteration & shortcut lookups
//   3. extensionRequest + SAN (multiple dNSName, URI, IPv4 iPAddress) extraction
//   4. Minimal CSR with empty [0] attributes (no extensions)
//   5. version!=0, non-zero BIT STRING unused_bits, trailing garbage & truncated DER guards

import { test, expect_eq } from "cap:test";
import {
    OID_CN,
    OID_C,
    OID_L,
    OID_ST,
    OID_O,
    OID_OU,
    OID_EXTENSION_REQUEST,
    OID_SAN,
    CsrRdnAttr,
    CsrSanEntry,
    CsrParsed,
    CsrSubjectIter,
    CsrSanIter,
    parseCsrDer,
    isCsrDer,
} from "cap:auth";

const OID_ED25519: []const u8 = "\x2B\x65\x70"; // 1.3.101.112

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

fn appendRdn(name_body: *DerBuf, oid: []const u8, val: []const u8) {
    let atv = DerBuf {};
    atv.pushTlv(0x06, oid);
    atv.pushTlv(0x0C, val); // UTF8String
    let seq = DerBuf {};
    seq.pushTlv(0x30, atv.slice());
    name_body.pushTlv(0x31, seq.slice());
}

fn buildCsr(
    ver: u8,
    subject_body: []const u8,
    pub_unused: u8,
    pub_key: []const u8,
    attrs_body: []const u8,
    sig_unused: u8,
    sig_bytes: []const u8,
): DerBuf {
    let cri_body = DerBuf {};
    let v_arr: [1]u8 = [ver];
    cri_body.pushTlv(0x02, v_arr[0..1]);
    cri_body.pushTlv(0x30, subject_body);

    // SubjectPublicKeyInfo
    let spki_body = DerBuf {};
    let palg = DerBuf {};
    palg.pushTlv(0x06, OID_ED25519);
    spki_body.pushTlv(0x30, palg.slice());
    spki_body.pushBitString(pub_unused, pub_key);
    cri_body.pushTlv(0x30, spki_body.slice());

    // attributes [0] IMPLICIT
    cri_body.pushTlv(0xA0, attrs_body);

    // Outer CertificationRequest
    let outer = DerBuf {};
    outer.pushTlv(0x30, cri_body.slice());
    let salg = DerBuf {};
    salg.pushTlv(0x06, OID_ED25519);
    outer.pushTlv(0x30, salg.slice());
    outer.pushBitString(sig_unused, sig_bytes);

    let final_der = DerBuf {};
    final_der.pushTlv(0x30, outer.slice());
    return final_der;
}

test("csr: complete Ed25519 PKCS#10 DER CSR parsing & cri_raw_der extraction") {
    let subj = DerBuf {};
    appendRdn(*subj, OID_C, "US");
    appendRdn(*subj, OID_O, "Example Corp");
    appendRdn(*subj, OID_OU, "Platform");
    appendRdn(*subj, OID_CN, "api.example.com");

    let pk: [32]u8 = [0x33; 32];
    let sig: [64]u8 = [0x77; 64];
    const der = buildCsr(0, subj.slice(), 0, pk[0..32], "", 0, sig[0..64]);

    expect_eq(isCsrDer(der.slice()), true);
    if (csr := parseCsrDer(der.slice())) {
        expect_eq(csr.version, 0);
        expect_eq(csr.cri_raw_der[0], 0x30);
        expect_eq(csr.spki_der[0], 0x30);
        expect_eq(strEq(csr.pub_alg_oid, OID_ED25519), true);
        expect_eq(csr.pub_key_bytes.len, 32);
        expect_eq(csr.pub_key_bytes[0], 0x33);
        expect_eq(strEq(csr.sig_alg_oid, OID_ED25519), true);
        expect_eq(csr.signature.len, 64);
        expect_eq(csr.signature[63], 0x77);
    } else {
        expect_eq(true, false);
    }
}

test("csr: Subject CN/O/OU/C/L/ST multi-RDN iteration & shortcut lookups") {
    let subj = DerBuf {};
    appendRdn(*subj, OID_C, "CN");
    appendRdn(*subj, OID_ST, "Beijing");
    appendRdn(*subj, OID_L, "Haidian");
    appendRdn(*subj, OID_O, "CP Labs");
    appendRdn(*subj, OID_OU, "Core");
    appendRdn(*subj, OID_CN, "cp.dev");

    let pk: [32]u8 = [0x11; 32];
    let sig: [64]u8 = [0x22; 64];
    const der = buildCsr(0, subj.slice(), 0, pk[0..32], "", 0, sig[0..64]);

    if (csr := parseCsrDer(der.slice())) {
        if (cn := csr.commonName()) { expect_eq(strEq(cn, "cp.dev"), true); } else { expect_eq(true, false); }
        if (org := csr.organization()) { expect_eq(strEq(org, "CP Labs"), true); } else { expect_eq(true, false); }
        if (c := csr.country()) { expect_eq(strEq(c, "CN"), true); } else { expect_eq(true, false); }
        if (ou := csr.findSubjectAttr(OID_OU)) { expect_eq(strEq(ou, "Core"), true); } else { expect_eq(true, false); }
        if (loc := csr.findSubjectAttr(OID_L)) { expect_eq(strEq(loc, "Haidian"), true); } else { expect_eq(true, false); }
        if (st := csr.findSubjectAttr(OID_ST)) { expect_eq(strEq(st, "Beijing"), true); } else { expect_eq(true, false); }

        let it = csr.subjectAttrs();
        let count: u32 = 0;
        while (attr := it.next()) {
            * = attr;
            count += 1;
        }
        expect_eq(count, 6);
    } else {
        expect_eq(true, false);
    }
}

test("csr: extensionRequest + SAN (multiple dNSName, URI & IPv4 iPAddress)") {
    let subj = DerBuf {};
    appendRdn(*subj, OID_CN, "api.example.com");

    // GeneralNames SEQUENCE: [2] dNSName, [2] dNSName, [6] URI, [7] iPAddress (10.0.0.1)
    let gn_body = DerBuf {};
    gn_body.pushTlv(0x82, "api.example.com");
    gn_body.pushTlv(0x82, "*.example.com");
    gn_body.pushTlv(0x86, "spiffe://cluster.local/ns/default/sa/web");
    gn_body.pushTlv(0x87, "\x0A\x00\x00\x01");
    let gn_seq = DerBuf {};
    gn_seq.pushTlv(0x30, gn_body.slice());

    // Extension SEQUENCE { OID_SAN, critical=TRUE, OCTET STRING(gn_seq) }
    let san_ext_body = DerBuf {};
    san_ext_body.pushTlv(0x06, OID_SAN);
    san_ext_body.pushTlv(0x01, "\xFF"); // critical = true
    san_ext_body.pushTlv(0x04, gn_seq.slice());
    let exts_body = DerBuf {};
    exts_body.pushTlv(0x30, san_ext_body.slice());
    let exts_seq = DerBuf {};
    exts_seq.pushTlv(0x30, exts_body.slice());

    // Attribute SEQUENCE { OID_EXTENSION_REQUEST, SET { exts_seq } }
    let attr_body = DerBuf {};
    attr_body.pushTlv(0x06, OID_EXTENSION_REQUEST);
    attr_body.pushTlv(0x31, exts_seq.slice());
    let attrs = DerBuf {};
    attrs.pushTlv(0x30, attr_body.slice());

    let pk: [32]u8 = [0x44; 32];
    let sig: [64]u8 = [0x55; 64];
    const der = buildCsr(0, subj.slice(), 0, pk[0..32], attrs.slice(), 0, sig[0..64]);

    if (csr := parseCsrDer(der.slice())) {
        let sit = csr.sans();
        let idx: u32 = 0;
        while (entry := sit.next()) {
            idx += 1;
            if (idx == 1) {
                expect_eq(entry.tag_num, 2);
                expect_eq(strEq(entry.value, "api.example.com"), true);
            } else if (idx == 2) {
                expect_eq(entry.tag_num, 2);
                expect_eq(strEq(entry.value, "*.example.com"), true);
            } else if (idx == 3) {
                expect_eq(entry.tag_num, 6);
                expect_eq(strEq(entry.value, "spiffe://cluster.local/ns/default/sa/web"), true);
            } else if (idx == 4) {
                expect_eq(entry.tag_num, 7);
                expect_eq(entry.value.len, 4);
                expect_eq(entry.value[0], 10);
                expect_eq(entry.value[3], 1);
            }
        }
        expect_eq(idx, 4);
    } else {
        expect_eq(true, false);
    }
}

test("csr: minimal CSR with empty [0] attributes (no extensions)") {
    let subj = DerBuf {};
    appendRdn(*subj, OID_CN, "minimal.local");

    let pk: [16]u8 = [0xAA; 16];
    let sig: [32]u8 = [0xBB; 32];
    const der = buildCsr(0, subj.slice(), 0, pk[0..16], "", 0, sig[0..32]);

    if (csr := parseCsrDer(der.slice())) {
        expect_eq(csr.attributes_der.len, 0);
        let sit = csr.sans();
        if (s := sit.next()) {
            * = s;
            expect_eq(true, false);
        }
        if (org := csr.organization()) {
            * = org;
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("csr: version!=0, non-zero BIT STRING unused_bits, trailing garbage & truncated DER guards") {
    let subj = DerBuf {};
    appendRdn(*subj, OID_CN, "test.local");
    let pk: [16]u8 = [0x01; 16];
    let sig: [16]u8 = [0x02; 16];

    // 1. version != 0
    const bad_ver = buildCsr(1, subj.slice(), 0, pk[0..16], "", 0, sig[0..16]);
    expect_eq(isCsrDer(bad_ver.slice()), false);

    // 2. Non-zero unused_bits on public key BIT STRING
    const bad_pk_unused = buildCsr(0, subj.slice(), 3, pk[0..16], "", 0, sig[0..16]);
    expect_eq(isCsrDer(bad_pk_unused.slice()), false);

    // 3. Non-zero unused_bits on signature BIT STRING
    const bad_sig_unused = buildCsr(0, subj.slice(), 0, pk[0..16], "", 1, sig[0..16]);
    expect_eq(isCsrDer(bad_sig_unused.slice()), false);

    // 4. Trailing garbage after valid CSR
    let valid = buildCsr(0, subj.slice(), 0, pk[0..16], "", 0, sig[0..16]);
    valid.push(0x00);
    expect_eq(isCsrDer(valid.slice()), false);

    // 5. Truncated DER
    expect_eq(isCsrDer(valid.slice()[0..12]), false);
}
