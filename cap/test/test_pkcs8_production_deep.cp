// Source: cp/cap/test/test_pkcs8_production_deep.cp (Production deep tests for cap:pkcs8)

import { test, expect_eq } from "cap:test";
import {
    OID_RSA_ENCRYPTION,
    OID_EC_PUBLIC_KEY,
    OID_P256,
    OID_P384,
    OID_P521,
    OID_X25519,
    OID_X448,
    OID_ED25519,
    OID_ED448,
    OID_PBES2,
    OID_PBKDF2,
    KeyAlgorithm,
    PrivateKeyInfo,
    EncryptedPrivateKeyInfo,
    SubjectPublicKeyInfo,
    parsePkcs8PrivateKey,
    parsePkcs8Encrypted,
    parseSpkiPublicKey,
} from "cap:pkcs8";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Ed25519 PKCS#8 v1 and v2 (with [1] publicKey) parsing + 32B raw seed extraction", () => {
    // RFC 8410 Ed25519 PKCS#8 v1:
    // 30 2E (46B): 02 01 00 (v1), 30 05 06 03 2B 65 70 (Ed25519), 04 22 (04 20 <32B seed>)
    let v1: [48]u8 = [0x11; 48];
    v1[0] = 0x30; v1[1] = 0x2E;
    v1[2] = 0x02; v1[3] = 0x01; v1[4] = 0x00;
    v1[5] = 0x30; v1[6] = 0x05; v1[7] = 0x06; v1[8] = 0x03; v1[9] = 0x2B; v1[10] = 0x65; v1[11] = 0x70;
    v1[12] = 0x04; v1[13] = 0x22;
    v1[14] = 0x04; v1[15] = 0x20;
    // seed bytes @ 16..48 = 0x11

    const p1 = parsePkcs8PrivateKey(v1[0..48]) ?? unreachable;
    expect_eq(p1.version, 0);
    expect_eq(p1.algorithm == .ed25519, true);
    expect_eq(bytesEq(p1.alg_oid, OID_ED25519[0..3]), true);
    expect_eq(p1.private_key_bytes.len, 34);
    expect_eq(p1.public_key_bytes.len, 0);

    const seed1 = p1.rawCurvePrivateKey() ?? unreachable;
    expect_eq(seed1.len, 32);
    expect_eq(seed1[0], 0x11);
    expect_eq(seed1[31], 0x11);

    // RFC 5958 Ed25519 PKCS#8 v2 (带 [1] IMPLICIT BIT STRING 公钥: 81 21 00 <32B pub>)
    // 总内长 = 46 + 35 = 81 (0x51)，总长 83B
    let v2: [83]u8 = [0x22; 83];
    let i: usize = 0;
    while (i < 48) {
        v2[i] = v1[i];
        i += 1;
    }
    v2[1] = 0x51; // 81B
    v2[4] = 0x01; // version = 1 (v2)
    v2[48] = 0x81; v2[49] = 0x21; v2[50] = 0x00; // [1] BIT STRING unused=0 + 32B pub (0x22)

    const p2 = parsePkcs8PrivateKey(v2[0..83]) ?? unreachable;
    expect_eq(p2.version, 1);
    expect_eq(p2.algorithm == .ed25519, true);
    expect_eq(p2.public_key_bytes.len, 32);
    expect_eq(p2.public_key_bytes[0], 0x22);
    expect_eq(p2.public_key_bytes[31], 0x22);
});

test("2. ECDSA P-256 / P-384 and RSA PrivateKeyInfo algorithm identification", () => {
    // ECDSA P-256: 30 1D (29B): v1 (3B) + AlgId (21B: 30 13 ...) + privKey (5B: 04 03 AA BB CC) -> 31B total
    const p256_der: [31]u8 = [
        0x30, 0x1D,
        0x02, 0x01, 0x00,
        0x30, 0x13,
            0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01,
            0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07,
        0x04, 0x03, 0xAA, 0xBB, 0xCC,
    ];
    const k256 = parsePkcs8PrivateKey(p256_der[0..31]) ?? unreachable;
    expect_eq(k256.algorithm == .ec_p256, true);
    expect_eq(k256.private_key_bytes.len, 3);
    expect_eq(k256.rawCurvePrivateKey() == null, true); // 非 RFC 8410 曲线

    // ECDSA P-384: 30 1A (26B): v1 (3B) + AlgId (18B: 30 10 ...) + privKey (5B: 04 03 01 02 03) -> 28B total
    const p384_der: [28]u8 = [
        0x30, 0x1A,
        0x02, 0x01, 0x00,
        0x30, 0x10,
            0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01,
            0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x22,
        0x04, 0x03, 0x01, 0x02, 0x03,
    ];
    const k384 = parsePkcs8PrivateKey(p384_der[0..28]) ?? unreachable;
    expect_eq(k384.algorithm == .ec_p384, true);

    // RSA: 30 16 (22B): v1 (3B) + AlgId (15B: 30 0D ...) + privKey (4B: 04 02 DE AD) -> 24B total
    const rsa_der: [24]u8 = [
        0x30, 0x16,
        0x02, 0x01, 0x00,
        0x30, 0x0D,
            0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01,
            0x05, 0x00,
        0x04, 0x02, 0xDE, 0xAD,
    ];
    const krsa = parsePkcs8PrivateKey(rsa_der[0..24]) ?? unreachable;
    expect_eq(krsa.algorithm == .rsa, true);
    expect_eq(krsa.alg_params_der.len, 2); // 05 00
});

test("3. PBES2 EncryptedPrivateKeyInfo parsing", () => {
    // SEQUENCE {
    //   SEQUENCE { OID PBES2, SEQUENCE { ... } }
    //   OCTET STRING (ciphertext)
    // }
    const enc_der: [25]u8 = [
        0x30, 0x17,
        0x30, 0x0F,
            0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x05, 0x0D,
            0x30, 0x02, 0x05, 0x00,
        0x04, 0x04, 0xCA, 0xFE, 0xBA, 0xBE,
    ];
    const epki = parsePkcs8Encrypted(enc_der[0..25]) ?? unreachable;
    expect_eq(epki.isPbes2(), true);
    expect_eq(bytesEq(epki.enc_alg_oid, OID_PBES2[0..9]), true);
    expect_eq(epki.enc_params_der.len, 4);
    expect_eq(epki.encrypted_data.len, 4);
    expect_eq(epki.encrypted_data[0], 0xCA);
    expect_eq(epki.encrypted_data[3], 0xBE);
});

test("4. SubjectPublicKeyInfo (SPKI) Ed25519, P-256, and RSA public key parsing", () => {
    // Ed25519 SPKI (44B): 30 2A, 30 05 06 03 2B 65 70, 03 21 00 <32B pub>
    let ed_spki: [44]u8 = [0x77; 44];
    ed_spki[0] = 0x30; ed_spki[1] = 0x2A;
    ed_spki[2] = 0x30; ed_spki[3] = 0x05; ed_spki[4] = 0x06; ed_spki[5] = 0x03;
    ed_spki[6] = 0x2B; ed_spki[7] = 0x65; ed_spki[8] = 0x70;
    ed_spki[9] = 0x03; ed_spki[10] = 0x21; ed_spki[11] = 0x00;

    const sp1 = parseSpkiPublicKey(ed_spki[0..44]) ?? unreachable;
    expect_eq(sp1.algorithm == .ed25519, true);
    expect_eq(sp1.public_key_bytes.len, 32);
    expect_eq(sp1.public_key_bytes[0], 0x77);

    // P-256 SPKI
    const ec_spki: [28]u8 = [
        0x30, 0x1A,
        0x30, 0x13,
            0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01,
            0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07,
        0x03, 0x03, 0x00, 0x04, 0x99,
    ];
    const sp2 = parseSpkiPublicKey(ec_spki[0..28]) ?? unreachable;
    expect_eq(sp2.algorithm == .ec_p256, true);
    expect_eq(sp2.public_key_bytes.len, 2);
    expect_eq(sp2.public_key_bytes[0], 0x04);
});

test("5. Malformed PKCS#8/SPKI rejection: version > 1, non-zero unused_bits, truncation, trailing bytes", () => {
    // 1. version = 2 非法
    const bad_ver: [16]u8 = [
        0x30, 0x0E,
        0x02, 0x01, 0x02,
        0x30, 0x05, 0x06, 0x03, 0x2B, 0x65, 0x70,
        0x04, 0x02, 0x00, 0x00,
    ];
    expect_eq(parsePkcs8PrivateKey(bad_ver[0..16]) == null, true);

    // 2. SPKI BIT STRING unused_bits != 0 非法
    const bad_unused: [15]u8 = [
        0x30, 0x0D,
        0x30, 0x05, 0x06, 0x03, 0x2B, 0x65, 0x70,
        0x03, 0x04, 0x01, 0xAA, 0xBB, 0xCC,
    ];
    expect_eq(parseSpkiPublicKey(bad_unused[0..15]) == null, true);

    // 3. 尾随垃圾字节（有效 14B SPKI 后多 1 字节 0xFF）
    const trailing_junk: [15]u8 = [
        0x30, 0x0C,
        0x30, 0x05, 0x06, 0x03, 0x2B, 0x65, 0x70,
        0x03, 0x03, 0x00, 0xAA, 0xBB,
        0xFF,
    ];
    expect_eq(parseSpkiPublicKey(trailing_junk[0..14]) != null, true);
    expect_eq(parseSpkiPublicKey(trailing_junk[0..15]) == null, true);

    // 4. 截断 DER
    expect_eq(parseSpkiPublicKey(trailing_junk[0..10]) == null, true);
    expect_eq(parsePkcs8Encrypted(trailing_junk[0..5]) == null, true);
});
