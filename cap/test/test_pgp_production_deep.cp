// Source: cp/cap/test/test_pgp_production_deep.cp (Production deep tests for cap:pgp)

import { test, expect_eq } from "cap:test";
import {
    PGP_CRC24_INIT,
    PGP_CRC24_POLY,
    TAG_PKESK,
    TAG_SIGNATURE,
    TAG_SKESK,
    TAG_ONE_PASS_SIG,
    TAG_SECRET_KEY,
    TAG_PUBLIC_KEY,
    TAG_SECRET_SUBKEY,
    TAG_COMPRESSED_DATA,
    TAG_SED,
    TAG_MARKER,
    TAG_LITERAL_DATA,
    TAG_TRUST,
    TAG_USER_ID,
    TAG_PUBLIC_SUBKEY,
    TAG_USER_ATTR,
    TAG_SEIPD,
    TAG_MDC,
    PgpArmorHeader,
    PgpPacket,
    PgpPacketIter,
    PgpPublicKeyInfo,
    PgpSignatureInfo,
    PgpLiteralData,
    pgpCrc24,
    parsePgpArmor,
    decodePgpArmor,
    parsePgpPublicKey,
    parsePgpSignature,
    parsePgpLiteralData,
} from "cap:pgp";

fn bytesEq(a: []const u8, b: []const u8): bool {
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

test("1. RFC 4880 CRC-24 standard vectors and end-to-end ASCII Armor decode + CRC-24 verification", () => {
    expect_eq(PGP_CRC24_INIT, 0xB704CE);
    expect_eq(PGP_CRC24_POLY, 0x1864CFB);
    expect_eq(TAG_PUBLIC_KEY, 6);
    expect_eq(TAG_USER_ID, 13);
    expect_eq(TAG_SIGNATURE, 2);
    expect_eq(TAG_LITERAL_DATA, 11);

    // RFC 4880 官方检验向量：CRC24("") == 0xB704CE，CRC24("123456789") == 0x21CF02
    expect_eq(pgpCrc24(""), 0xB704CE);
    expect_eq(pgpCrc24("123456789"), 0x21CF02);

    // "123456789" 的 Base64 为 "MTIzNDU2Nzg5"，CRC-24 = 0x21CF02 的 Base64 为 "Ic8C"
    const armor_ok =
        "-----BEGIN PGP MESSAGE-----\n" +
        "Version: CP-OpenPGP 1.0\n" +
        "\n" +
        "MTIz\n" +
        "NDU2Nzg5\n" +
        "=Ic8C\n" +
        "-----END PGP MESSAGE-----\n";

    const hdr = parsePgpArmor(armor_ok) ?? unreachable;
    expect_eq(bytesEq(hdr.block_type, "PGP MESSAGE"), true);
    expect_eq(bytesEq(hdr.crc24_b64, "Ic8C"), true);

    let out: [64]u8 = [0; 64];
    let out_s: []u8 = out[0..64];
    const n = decodePgpArmor(armor_ok, *out_s) ?? unreachable;
    expect_eq(n, 9);
    expect_eq(bytesEq(out[0..n], "123456789"), true);
});

test("2. Old Format (1B/2B/4B/Indeterminate) and New Format (<192 / 192..223 / 255) packet iteration", () => {
    // Old Format 包序列：
    // 1) Tag 13 (User ID), 1B len (ptag = 0x80 | (13<<2) | 0 = 0xB4), len = 3, body = "Bob"
    // 2) Tag 10 (Marker), 2B BE len (ptag = 0x80 | (10<<2) | 1 = 0xA9), len = 3, body = "PGP"
    // 3) Tag 12 (Trust), 4B BE len (ptag = 0x80 | (12<<2) | 2 = 0xB2), len = 2, body = [0x01, 0x02]
    // 4) Tag 11 (Literal), indeterminate len (ptag = 0x80 | (11<<2) | 3 = 0xAF), body = "END"
    const old_stream: [20]u8 = [
        0xB4, 0x03, 'B', 'o', 'b',
        0xA9, 0x00, 0x03, 'P', 'G', 'P',
        0xB2, 0x00, 0x00, 0x00, 0x02, 0x01, 0x02,
        0xAF, 'Z',
    ];

    let it_old = PgpPacketIter.init(old_stream[0..20]);
    const p1 = it_old.next() ?? unreachable;
    expect_eq(p1.tag, TAG_USER_ID);
    expect_eq(p1.is_new_format, false);
    expect_eq(bytesEq(p1.body, "Bob"), true);

    const p2 = it_old.next() ?? unreachable;
    expect_eq(p2.tag, TAG_MARKER);
    expect_eq(bytesEq(p2.body, "PGP"), true);

    const p3 = it_old.next() ?? unreachable;
    expect_eq(p3.tag, TAG_TRUST);
    expect_eq(p3.body.len, 2);
    expect_eq(p3.body[1], 0x02);

    const p4 = it_old.next() ?? unreachable;
    expect_eq(p4.tag, TAG_LITERAL_DATA);
    expect_eq(p4.body.len, 1);
    expect_eq(p4.body[0], 'Z');

    expect_eq(it_old.next() == null, true);
    expect_eq(it_old.had_error, false);

    // New Format 3 种长度编码：<192 (len=2)、192..223 (len=200 => b1=192, b2=8)、255+4B BE (len=3)
    let new_stream: [215]u8 = [0xAB; 215];
    // Packet A @ 0: Tag 13 (0xC0 | 13 = 0xCD), len = 2
    new_stream[0] = 0xCD;
    new_stream[1] = 2;
    new_stream[2] = 'O';
    new_stream[3] = 'K';
    // Packet B @ 4: Tag 18 (0xC0 | 18 = 0xD2), 2B len = 200 -> ((192-192)<<8)+8+192 = 200
    new_stream[4] = 0xD2;
    new_stream[5] = 192;
    new_stream[6] = 8;
    // body @ 7..207 (200 bytes)
    // Packet C @ 207: Tag 19 (0xC0 | 19 = 0xD3), 5B len = 255 + 0x00000003
    new_stream[207] = 0xD3;
    new_stream[208] = 255;
    new_stream[209] = 0;
    new_stream[210] = 0;
    new_stream[211] = 0;
    new_stream[212] = 3;
    new_stream[213] = 0x11;
    new_stream[214] = 0x22;
    // 注意 213..216 需要 3 字节 -> 用 216 长度数组！
    let new_stream_ok: [216]u8 = [0xAB; 216];
    let k: usize = 0;
    while (k < 215) {
        new_stream_ok[k] = new_stream[k];
        k += 1;
    }
    new_stream_ok[215] = 0x33;

    let it_new = PgpPacketIter.init(new_stream_ok[0..216]);
    const na = it_new.next() ?? unreachable;
    expect_eq(na.tag, TAG_USER_ID);
    expect_eq(na.is_new_format, true);
    expect_eq(bytesEq(na.body, "OK"), true);

    const nb = it_new.next() ?? unreachable;
    expect_eq(nb.tag, TAG_SEIPD);
    expect_eq(nb.body.len, 200);

    const nc = it_new.next() ?? unreachable;
    expect_eq(nc.tag, TAG_MDC);
    expect_eq(nc.body.len, 3);
    expect_eq(nc.body[0], 0x11);
    expect_eq(nc.body[2], 0x33);

    expect_eq(it_new.next() == null, true);
    expect_eq(it_new.had_error, false);
});

test("3. v4 PublicKey + UserID + v4 Signature keyblock parsing", () => {
    // 组合 3 个 New-Format 包：PublicKey (Tag 6) + UserID (Tag 13) + Signature (Tag 2)
    const keyblock: [36]u8 = [
        // Tag 6 (0xC6), len = 8: v4, ts=0x65000001 (1694498817), alg=22 (EdDSA), key_material=[0xDE, 0xAD]
        0xC6, 0x08, 0x04, 0x65, 0x00, 0x00, 0x01, 22, 0xDE, 0xAD,
        // Tag 13 (0xCD), len = 10: "Alice <a@b"
        0xCD, 0x0A, 'A', 'l', 'i', 'c', 'e', ' ', '<', 'a', '@', 'b',
        // Tag 2 (0xC2), len = 12: v4, sig_type=0x13, pub_alg=22, hash_alg=8 (SHA-256),
        // hashed_len=2 ([0x01, 0x02]), unhashed_len=2 ([0x03, 0x04])
        0xC2, 0x0C, 0x04, 0x13, 22, 8, 0x00, 0x02, 0x01, 0x02, 0x00, 0x02, 0x03, 0x04,
    ];

    let it = PgpPacketIter.init(keyblock[0..36]);

    const pk_pkt = it.next() ?? unreachable;
    expect_eq(pk_pkt.tag, TAG_PUBLIC_KEY);
    const pk = parsePgpPublicKey(pk_pkt.body) ?? unreachable;
    expect_eq(pk.version, 4);
    expect_eq(pk.creation_time, 0x65000001);
    expect_eq(pk.algorithm, 22);
    expect_eq(pk.key_material.len, 2);
    expect_eq(pk.key_material[0], 0xDE);

    const uid_pkt = it.next() ?? unreachable;
    expect_eq(uid_pkt.tag, TAG_USER_ID);
    expect_eq(bytesEq(uid_pkt.body, "Alice <a@b"), true);

    const sig_pkt = it.next() ?? unreachable;
    expect_eq(sig_pkt.tag, TAG_SIGNATURE);
    const sig = parsePgpSignature(sig_pkt.body) ?? unreachable;
    expect_eq(sig.version, 4);
    expect_eq(sig.sig_type, 0x13);
    expect_eq(sig.pub_alg, 22);
    expect_eq(sig.hash_alg, 8);
    expect_eq(sig.hashed_subpackets.len, 2);
    expect_eq(sig.hashed_subpackets[0], 0x01);
    expect_eq(sig.unhashed_subpackets.len, 2);
    expect_eq(sig.unhashed_subpackets[1], 0x04);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("4. Literal Data Packet (Tag 11) parsing", () => {
    // format='u', filename="note.txt" (8B), timestamp=0x65500000, payload="hello pgp"
    const lit_body: [23]u8 = [
        'u', 8,
        'n', 'o', 't', 'e', '.', 't', 'x', 't',
        0x65, 0x50, 0x00, 0x00,
        'h', 'e', 'l', 'l', 'o', ' ', 'p', 'g', 'p',
    ];
    const lit = parsePgpLiteralData(lit_body[0..23]) ?? unreachable;
    expect_eq(lit.format, 'u');
    expect_eq(bytesEq(lit.filename, "note.txt"), true);
    expect_eq(lit.timestamp, 0x65500000);
    expect_eq(bytesEq(lit.payload, "hello pgp"), true);
});

test("5. Malformed CRC-24, missing bit7==1, truncated packet lengths, and invalid v4 headers", () => {
    // 1. 错 CRC-24（将 =Ic8C 篡改为 =AAAA）必须被 decodePgpArmor 拒绝
    const bad_crc =
        "-----BEGIN PGP MESSAGE-----\n" +
        "\n" +
        "MTIzNDU2Nzg5\n" +
        "=AAAA\n" +
        "-----END PGP MESSAGE-----\n";
    let out: [32]u8 = [0; 32];
    let out_s: []u8 = out[0..32];
    expect_eq(decodePgpArmor(bad_crc, *out_s) == null, true);

    // 2. BEGIN/END 类型不匹配
    const bad_end =
        "-----BEGIN PGP MESSAGE-----\n\nMTIz\n-----END PGP SIGNATURE-----\n";
    expect_eq(parsePgpArmor(bad_end) == null, true);

    // 3. 缺少 bit7 == 1 的非法包头字节 (0x46)
    const no_bit7: [4]u8 = [0x46, 0x02, 0x01, 0x02];
    let it1 = PgpPacketIter.init(no_bit7[0..4]);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.had_error, true);

    // 4. 越界包长（声明 50 字节但只有 2 字节）
    const trunc_pkt: [4]u8 = [0xCD, 50, 'a', 'b'];
    let it2 = PgpPacketIter.init(trunc_pkt[0..4]);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.had_error, true);

    // 5. 非法 v3 PublicKey / 截断 Signature / 非法 Literal format
    const bad_pk: [6]u8 = [3, 0, 0, 0, 0, 1];
    expect_eq(parsePgpPublicKey(bad_pk[0..6]) == null, true);
    const bad_sig: [6]u8 = [4, 0, 1, 8, 0x00, 0x50]; // hashed_len=80 越界
    expect_eq(parsePgpSignature(bad_sig[0..6]) == null, true);
    const bad_lit: [6]u8 = ['x', 0, 0, 0, 0, 0];
    expect_eq(parsePgpLiteralData(bad_lit[0..6]) == null, true);
});
