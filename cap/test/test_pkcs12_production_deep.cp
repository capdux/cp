// Source: cp/cap/test/test_pkcs12_production_deep.cp
// Deep Production Test Suite for cap:pkcs12
// Covers:
//   1. Full PFX v3 + AuthenticatedSafe (plaintext SafeContents with shroudedKeyBag & certBag
//      carrying localKeyId & friendlyName BMPString + EncryptedData entry) + MacData (SHA-256, iter=2048)
//   2. PFX without MacData & MacData with omitted iterations (RFC 7292 default = 1)
//   3. All SafeBag kinds (keyBag, shroudedKeyBag, certBag, crlBag, secretBag, unknown) + unknown attr skip
//   4. Non-DATA authSafe & empty AuthenticatedSafe / SafeContents edge cases
//   5. Strict DER guards: version != 3, non-DER, truncated, BER indefinite length (0x80),
//      non-minimal length/integer, and trailing garbage bytes
//   6. Malformed AuthenticatedSafe & SafeContents iterator error propagation

import { test, expect_eq } from "cap:test";
import {
    OID_DATA,
    OID_ENCRYPTED_DATA,
    OID_KEY_BAG,
    OID_SHROUDED_KEY_BAG,
    OID_CERT_BAG,
    OID_CRL_BAG,
    OID_SECRET_BAG,
    OID_FRIENDLY_NAME,
    OID_LOCAL_KEY_ID,
    Pkcs12BagKind,
    Pkcs12MacData,
    Pkcs12Pfx,
    Pkcs12SafeContent,
    Pkcs12Bag,
    Pkcs12SafeIter,
    Pkcs12BagIter,
    parsePkcs12Der,
    isPkcs12Der,
} from "cap:crypto";

const OID_SHA256: []const u8 = "\x60\x86\x48\x01\x65\x03\x04\x02\x01";
const OID_SHA1: []const u8 = "\x2B\x0E\x03\x02\x1A";
const OID_CUSTOM_BAG: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x0A\x01\x7F";
const OID_CUSTOM_ATTR: []const u8 = "\x2A\x86\x48\x86\xF7\x0D\x01\x09\x7F";

struct DerBuf {
    buf: [2048]u8 = [0; 2048],
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

fn buildContentInfoData(inner_bytes: []const u8): DerBuf {
    let oct = DerBuf {};
    oct.pushTlv(0x04, inner_bytes);

    let body = DerBuf {};
    body.pushTlv(0x06, OID_DATA);
    body.pushTlv(0xA0, oct.slice());

    let seq = DerBuf {};
    seq.pushTlv(0x30, body.slice());
    return seq;
}

fn buildContentInfoRaw(oid: []const u8, inner_der: []const u8): DerBuf {
    let body = DerBuf {};
    body.pushTlv(0x06, oid);
    if (inner_der.len > 0) {
        body.pushTlv(0xA0, inner_der);
    }
    let seq = DerBuf {};
    seq.pushTlv(0x30, body.slice());
    return seq;
}

fn buildAttr(attr_oid: []const u8, val_tag: u8, val_bytes: []const u8): DerBuf {
    let val_set = DerBuf {};
    val_set.pushTlv(val_tag, val_bytes);

    let attr_body = DerBuf {};
    attr_body.pushTlv(0x06, attr_oid);
    attr_body.pushTlv(0x31, val_set.slice());

    let seq = DerBuf {};
    seq.pushTlv(0x30, attr_body.slice());
    return seq;
}

fn buildSafeBag(
    bag_oid: []const u8,
    bag_val_der: []const u8,
    local_key_id: []const u8,
    friendly_name_bmp: []const u8,
    extra_attr: []const u8,
): DerBuf {
    let body = DerBuf {};
    body.pushTlv(0x06, bag_oid);
    body.pushTlv(0xA0, bag_val_der);

    let attrs = DerBuf {};
    if (local_key_id.len > 0) {
        const a = buildAttr(OID_LOCAL_KEY_ID, 0x04, local_key_id);
        attrs.pushBytes(a.slice());
    }
    if (friendly_name_bmp.len > 0) {
        const a = buildAttr(OID_FRIENDLY_NAME, 0x1E, friendly_name_bmp);
        attrs.pushBytes(a.slice());
    }
    if (extra_attr.len > 0) {
        attrs.pushBytes(extra_attr);
    }
    if (attrs.len > 0) {
        body.pushTlv(0x31, attrs.slice());
    }

    let seq = DerBuf {};
    seq.pushTlv(0x30, body.slice());
    return seq;
}

fn buildMacData(
    digest_oid: []const u8,
    digest_bytes: []const u8,
    salt_bytes: []const u8,
    iter_bytes: []const u8,
): DerBuf {
    // AlgorithmIdentifier
    let alg_body = DerBuf {};
    alg_body.pushTlv(0x06, digest_oid);
    alg_body.pushTlv(0x05, ""); // NULL
    let alg_seq = DerBuf {};
    alg_seq.pushTlv(0x30, alg_body.slice());

    // DigestInfo
    let di_body = DerBuf {};
    di_body.pushBytes(alg_seq.slice());
    di_body.pushTlv(0x04, digest_bytes);
    let di_seq = DerBuf {};
    di_seq.pushTlv(0x30, di_body.slice());

    // MacData
    let mac_body = DerBuf {};
    mac_body.pushBytes(di_seq.slice());
    mac_body.pushTlv(0x04, salt_bytes);
    if (iter_bytes.len > 0) {
        mac_body.pushTlv(0x02, iter_bytes);
    }

    let seq = DerBuf {};
    seq.pushTlv(0x30, mac_body.slice());
    return seq;
}

fn buildPfx(ver_bytes: []const u8, auth_safe_ci: []const u8, mac_data_der: []const u8): DerBuf {
    let body = DerBuf {};
    body.pushTlv(0x02, ver_bytes);
    body.pushBytes(auth_safe_ci);
    if (mac_data_der.len > 0) {
        body.pushBytes(mac_data_der);
    }
    let seq = DerBuf {};
    seq.pushTlv(0x30, body.slice());
    return seq;
}

test("pkcs12: full PFX v3 + MacData(SHA-256, iter=2048) + AuthenticatedSafe + SafeBag attrs") {
    // 1. Plaintext SafeContents with shroudedKeyBag + certBag
    let key_val = DerBuf {};
    key_val.pushTlv(0x30, "SHROUDED-KEY-PAYLOAD");
    const key_bmp: []const u8 = "\x00k\x00e\x00y"; // "key" in UTF-16BE BMPString
    const key_bag = buildSafeBag(
        OID_SHROUDED_KEY_BAG,
        key_val.slice(),
        "\x11\x22\x33\x44",
        key_bmp,
        "",
    );

    let cert_val = DerBuf {};
    cert_val.pushTlv(0x30, "X509-CERT-BAG-PAYLOAD");
    const cert_bmp: []const u8 = "\x00c\x00e\x00r\x00t"; // "cert" in UTF-16BE BMPString
    const cert_bag = buildSafeBag(
        OID_CERT_BAG,
        cert_val.slice(),
        "\xAA\xBB\xCC\xDD",
        cert_bmp,
        "",
    );

    let sc_body = DerBuf {};
    sc_body.pushBytes(key_bag.slice());
    sc_body.pushBytes(cert_bag.slice());
    let sc_seq = DerBuf {};
    sc_seq.pushTlv(0x30, sc_body.slice());

    const ci_plain = buildContentInfoData(sc_seq.slice());

    // 2. EncryptedData entry inside AuthenticatedSafe
    let enc_inner = DerBuf {};
    enc_inner.pushTlv(0x30, "ENCRYPTED-SAFE-CONTENTS");
    const ci_enc = buildContentInfoRaw(OID_ENCRYPTED_DATA, enc_inner.slice());

    // AuthenticatedSafe ::= SEQUENCE OF ContentInfo
    let as_body = DerBuf {};
    as_body.pushBytes(ci_plain.slice());
    as_body.pushBytes(ci_enc.slice());
    let as_seq = DerBuf {};
    as_seq.pushTlv(0x30, as_body.slice());

    const auth_safe_ci = buildContentInfoData(as_seq.slice());

    // MacData with SHA-256, 8-byte salt, iterations = 2048 (0x0800)
    const mac = buildMacData(
        OID_SHA256,
        "\xDE\xAD\xBE\xEF\x01\x02\x03\x04",
        "\xA1\xB2\xC3\xD4\xE5\xF6\x07\x08",
        "\x08\x00",
    );

    const pfx_der = buildPfx("\x03", auth_safe_ci.slice(), mac.slice());

    expect_eq(isPkcs12Der(pfx_der.slice()), true);
    if (pfx := parsePkcs12Der(pfx_der.slice())) {
        expect_eq(pfx.version, 3);
        expect_eq(strEq(pfx.auth_safe_type_oid, OID_DATA), true);
        expect_eq(strEq(pfx.auth_safe_data, as_seq.slice()), true);

        // Verify MacData
        expect_eq(pfx.mac_data.present, true);
        expect_eq(strEq(pfx.mac_data.digest_alg_oid, OID_SHA256), true);
        expect_eq(strEq(pfx.mac_data.digest, "\xDE\xAD\xBE\xEF\x01\x02\x03\x04"), true);
        expect_eq(strEq(pfx.mac_data.mac_salt, "\xA1\xB2\xC3\xD4\xE5\xF6\x07\x08"), true);
        expect_eq(pfx.mac_data.iterations, 2048);

        // Iterate AuthenticatedSafe
        let sit = pfx.safeContents();
        let safe_idx: u32 = 0;
        while (sc := sit.next()) {
            safe_idx += 1;
            if (safe_idx == 1) {
                expect_eq(sc.is_encrypted, false);
                expect_eq(strEq(sc.content_type_oid, OID_DATA), true);
                expect_eq(strEq(sc.content_der, sc_seq.slice()), true);

                // Iterate SafeBags
                let bit = sc.bags();
                let bag_idx: u32 = 0;
                while (bag := bit.next()) {
                    bag_idx += 1;
                    if (bag_idx == 1) {
                        expect_eq(bag.bag_kind == Pkcs12BagKind.shrouded_key_bag, true);
                        expect_eq(strEq(bag.bag_id_oid, OID_SHROUDED_KEY_BAG), true);
                        expect_eq(strEq(bag.bag_value_der, key_val.slice()), true);
                        expect_eq(strEq(bag.local_key_id, "\x11\x22\x33\x44"), true);
                        expect_eq(strEq(bag.friendly_name_bmp, key_bmp), true);
                    } else if (bag_idx == 2) {
                        expect_eq(bag.bag_kind == Pkcs12BagKind.cert_bag, true);
                        expect_eq(strEq(bag.bag_id_oid, OID_CERT_BAG), true);
                        expect_eq(strEq(bag.bag_value_der, cert_val.slice()), true);
                        expect_eq(strEq(bag.local_key_id, "\xAA\xBB\xCC\xDD"), true);
                        expect_eq(strEq(bag.friendly_name_bmp, cert_bmp), true);
                    }
                }
                expect_eq(bag_idx, 2);
                expect_eq(bit.hasError(), false);
            } else if (safe_idx == 2) {
                expect_eq(sc.is_encrypted, true);
                expect_eq(strEq(sc.content_type_oid, OID_ENCRYPTED_DATA), true);
                expect_eq(strEq(sc.content_der, enc_inner.slice()), true);
            }
        }
        expect_eq(safe_idx, 2);
        expect_eq(sit.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("pkcs12: PFX without MacData & MacData with omitted iterations defaults to 1") {
    let empty_as = DerBuf {};
    empty_as.pushTlv(0x30, "");
    const auth_safe_ci = buildContentInfoData(empty_as.slice());

    // Case A: No MacData
    const pfx_no_mac = buildPfx("\x03", auth_safe_ci.slice(), "");
    if (pfx := parsePkcs12Der(pfx_no_mac.slice())) {
        expect_eq(pfx.version, 3);
        expect_eq(pfx.mac_data.present, false);
        expect_eq(pfx.mac_data.iterations, 1);
        expect_eq(pfx.mac_data.digest.len, 0);
        expect_eq(pfx.mac_data.mac_salt.len, 0);
    } else {
        expect_eq(true, false);
    }

    // Case B: MacData present, iterations field omitted -> RFC 7292 default 1
    const mac_default_iter = buildMacData(
        OID_SHA1,
        "\x01\x02\x03\x04\x05",
        "\xFE\xED\xFA\xCE",
        "",
    );
    const pfx_def_iter = buildPfx("\x03", auth_safe_ci.slice(), mac_default_iter.slice());
    if (pfx := parsePkcs12Der(pfx_def_iter.slice())) {
        expect_eq(pfx.mac_data.present, true);
        expect_eq(strEq(pfx.mac_data.digest_alg_oid, OID_SHA1), true);
        expect_eq(strEq(pfx.mac_data.digest, "\x01\x02\x03\x04\x05"), true);
        expect_eq(strEq(pfx.mac_data.mac_salt, "\xFE\xED\xFA\xCE"), true);
        expect_eq(pfx.mac_data.iterations, 1);
    } else {
        expect_eq(true, false);
    }
}

test("pkcs12: all SafeBag kinds + unknown attribute skip + attributeless bags") {
    const custom_attr = buildAttr(OID_CUSTOM_ATTR, 0x04, "\x99\x88");

    const b1 = buildSafeBag(OID_KEY_BAG, "\x04\x02\x01\x02", "", "", custom_attr.slice());
    const b2 = buildSafeBag(OID_SHROUDED_KEY_BAG, "\x04\x02\x03\x04", "", "", "");
    const b3 = buildSafeBag(OID_CERT_BAG, "\x04\x02\x05\x06", "\x77", "", "");
    const b4 = buildSafeBag(OID_CRL_BAG, "\x04\x02\x07\x08", "", "", "");
    const b5 = buildSafeBag(OID_SECRET_BAG, "\x04\x02\x09\x0A", "", "", "");
    const b6 = buildSafeBag(OID_CUSTOM_BAG, "\x04\x02\x0B\x0C", "", "", "");

    let sc_body = DerBuf {};
    sc_body.pushBytes(b1.slice());
    sc_body.pushBytes(b2.slice());
    sc_body.pushBytes(b3.slice());
    sc_body.pushBytes(b4.slice());
    sc_body.pushBytes(b5.slice());
    sc_body.pushBytes(b6.slice());
    let sc_seq = DerBuf {};
    sc_seq.pushTlv(0x30, sc_body.slice());

    let bit = Pkcs12BagIter.init(sc_seq.slice());
    let count: u32 = 0;
    while (bag := bit.next()) {
        count += 1;
        if (count == 1) {
            expect_eq(bag.bag_kind == Pkcs12BagKind.key_bag, true);
            expect_eq(bag.local_key_id.len, 0);
            expect_eq(bag.friendly_name_bmp.len, 0);
        } else if (count == 2) {
            expect_eq(bag.bag_kind == Pkcs12BagKind.shrouded_key_bag, true);
        } else if (count == 3) {
            expect_eq(bag.bag_kind == Pkcs12BagKind.cert_bag, true);
            expect_eq(strEq(bag.local_key_id, "\x77"), true);
        } else if (count == 4) {
            expect_eq(bag.bag_kind == Pkcs12BagKind.crl_bag, true);
        } else if (count == 5) {
            expect_eq(bag.bag_kind == Pkcs12BagKind.secret_bag, true);
        } else if (count == 6) {
            expect_eq(bag.bag_kind == Pkcs12BagKind.unknown, true);
            expect_eq(strEq(bag.bag_id_oid, OID_CUSTOM_BAG), true);
        }
    }
    expect_eq(count, 6);
    expect_eq(bit.hasError(), false);
}

test("pkcs12: non-DATA authSafe & empty iterators edge cases") {
    // Non-DATA authSafe (e.g. OID_ENCRYPTED_DATA) strips [0] EXPLICIT only
    let inner = DerBuf {};
    inner.pushTlv(0x30, "SIGNED-OR-OTHER-AUTHSAFE");
    const ci = buildContentInfoRaw(OID_ENCRYPTED_DATA, inner.slice());
    const pfx_der = buildPfx("\x03", ci.slice(), "");

    if (pfx := parsePkcs12Der(pfx_der.slice())) {
        expect_eq(strEq(pfx.auth_safe_type_oid, OID_ENCRYPTED_DATA), true);
        expect_eq(strEq(pfx.auth_safe_data, inner.slice()), true);
    } else {
        expect_eq(true, false);
    }

    // Empty slices & empty SEQUENCE iterators
    let s_empty = Pkcs12SafeIter.init("");
    expect_eq(s_empty.next() == null, true);
    expect_eq(s_empty.hasError(), false);

    let s_seq0 = Pkcs12SafeIter.init("\x30\x00");
    expect_eq(s_seq0.next() == null, true);
    expect_eq(s_seq0.hasError(), false);

    let b_empty = Pkcs12BagIter.init("");
    expect_eq(b_empty.next() == null, true);
    expect_eq(b_empty.hasError(), false);

    let b_seq0 = Pkcs12BagIter.init("\x30\x00");
    expect_eq(b_seq0.next() == null, true);
    expect_eq(b_seq0.hasError(), false);
}

test("pkcs12: strict DER guards (version!=3, non-DER, truncated, BER 0x80, non-minimal, trailing)") {
    let empty_as = DerBuf {};
    empty_as.pushTlv(0x30, "");
    const auth_safe_ci = buildContentInfoData(empty_as.slice());

    // 1. version != 3 (version = 2 and version = 4)
    const pfx_v2 = buildPfx("\x02", auth_safe_ci.slice(), "");
    expect_eq(isPkcs12Der(pfx_v2.slice()), false);
    expect_eq(parsePkcs12Der(pfx_v2.slice()) == null, true);

    const pfx_v4 = buildPfx("\x04", auth_safe_ci.slice(), "");
    expect_eq(parsePkcs12Der(pfx_v4.slice()) == null, true);

    // 2. Non-minimal DER INTEGER (0x00 0x03 for 3)
    const pfx_nonmin = buildPfx("\x00\x03", auth_safe_ci.slice(), "");
    expect_eq(parsePkcs12Der(pfx_nonmin.slice()) == null, true);

    // 3. Empty & non-SEQUENCE input
    expect_eq(isPkcs12Der(""), false);
    expect_eq(isPkcs12Der("-----BEGIN PKCS12-----"), false);

    // 4. BER indefinite length (0x80)
    expect_eq(parsePkcs12Der("\x30\x80\x02\x01\x03\x00\x00") == null, true);

    // 5. Non-minimal DER length (0x81 0x03 instead of 0x03)
    expect_eq(parsePkcs12Der("\x30\x81\x03\x02\x01\x03") == null, true);

    // 6. Valid PFX truncated & valid PFX with trailing garbage byte
    const valid = buildPfx("\x03", auth_safe_ci.slice(), "");
    expect_eq(isPkcs12Der(valid.slice()), true);

    const truncated = valid.slice()[0..(valid.len - 2)];
    expect_eq(parsePkcs12Der(truncated) == null, true);

    let with_trailing = DerBuf {};
    with_trailing.pushBytes(valid.slice());
    with_trailing.push(0x00);
    expect_eq(parsePkcs12Der(with_trailing.slice()) == null, true);
}

test("pkcs12: malformed SafeIter and BagIter error propagation") {
    // Non-SEQUENCE passed to Pkcs12SafeIter
    let bad_s1 = Pkcs12SafeIter.init("\x04\x02\xAA\xBB");
    expect_eq(bad_s1.hasError(), true);
    expect_eq(bad_s1.next() == null, true);

    // AuthenticatedSafe SEQUENCE containing malformed ContentInfo (INTEGER instead of SEQUENCE)
    let bad_s2 = Pkcs12SafeIter.init("\x30\x03\x02\x01\x05");
    expect_eq(bad_s2.hasError(), false);
    expect_eq(bad_s2.next() == null, true);
    expect_eq(bad_s2.hasError(), true);

    // Non-SEQUENCE passed to Pkcs12BagIter
    let bad_b1 = Pkcs12BagIter.init("\x04\x02\xCC\xDD");
    expect_eq(bad_b1.hasError(), true);
    expect_eq(bad_b1.next() == null, true);

    // SafeBag missing [0] EXPLICIT bagValue (only OID present)
    let bad_bag = DerBuf {};
    bad_bag.pushTlv(0x06, OID_CERT_BAG);
    let bag_seq = DerBuf {};
    bag_seq.pushTlv(0x30, bad_bag.slice());
    let sc_seq = DerBuf {};
    sc_seq.pushTlv(0x30, bag_seq.slice());

    let bad_b2 = Pkcs12BagIter.init(sc_seq.slice());
    expect_eq(bad_b2.hasError(), false);
    expect_eq(bad_b2.next() == null, true);
    expect_eq(bad_b2.hasError(), true);
}
