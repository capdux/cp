// Source: cp/cap/test/test_der_production_deep.cp (Production deep tests for cap:der)

import { test, expect_eq } from "cap:test";
import {
    TAG_BOOLEAN,
    TAG_INTEGER,
    TAG_BIT_STRING,
    TAG_OCTET_STRING,
    TAG_NULL,
    TAG_OID,
    TAG_UTF8_STRING,
    TAG_PRINTABLE_STRING,
    TAG_IA5_STRING,
    TAG_UTC_TIME,
    TAG_GENERALIZED_TIME,
    TAG_SEQUENCE,
    TAG_SET,
    CLASS_MASK,
    CONSTRUCTED_MASK,
    DerTlv,
    DerBitString,
    DerReader,
    DerWriter,
    parseDerTlv,
    formatDerOid,
} from "cap:der";

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

test("1. X.509/PKCS-style nested SEQUENCE + CONTEXT [0] + OID + INTEGER + BIT STRING parsing", () => {
    expect_eq(TAG_BOOLEAN, 0x01);
    expect_eq(TAG_INTEGER, 0x02);
    expect_eq(TAG_BIT_STRING, 0x03);
    expect_eq(TAG_OCTET_STRING, 0x04);
    expect_eq(TAG_NULL, 0x05);
    expect_eq(TAG_OID, 0x06);
    expect_eq(TAG_UTF8_STRING, 0x0C);
    expect_eq(TAG_PRINTABLE_STRING, 0x13);
    expect_eq(TAG_IA5_STRING, 0x16);
    expect_eq(TAG_UTC_TIME, 0x17);
    expect_eq(TAG_GENERALIZED_TIME, 0x18);
    expect_eq(TAG_SEQUENCE, 0x30);
    expect_eq(TAG_SET, 0x31);
    expect_eq(CLASS_MASK, 0xC0);
    expect_eq(CONSTRUCTED_MASK, 0x20);

    // 构造类 X.509 TBSCertificate 片段：
    // SEQUENCE {
    //   [0] EXPLICIT INTEGER 2 (v3)
    //   INTEGER 0x010001 (65537)
    //   SEQUENCE { OID 1.2.840.113549.1.1.11 (sha256WithRSAEncryption), NULL }
    //   BIT STRING (unused=3, bytes=0xA0, 0xF8)
    // }
    const cert_der: [32]u8 = [
        0x30, 0x1E,
        0xA0, 0x03, 0x02, 0x01, 0x02,
        0x02, 0x03, 0x01, 0x00, 0x01,
        0x30, 0x0D,
            0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B,
            0x05, 0x00,
        0x03, 0x03, 0x03, 0xA0, 0xF8,
    ];

    let root = DerReader.init(cert_der[0..32]);
    let seq = root.readSequence() ?? unreachable;
    expect_eq(root.isEmpty(), true);

    let ctx0 = seq.readContextExplicit(0) ?? unreachable;
    expect_eq(ctx0.readU64() ?? 0, 2);
    expect_eq(ctx0.isEmpty(), true);

    expect_eq(seq.readU64() ?? 0, 65537);

    let alg = seq.readSequence() ?? unreachable;
    const oid_raw = alg.readOid() ?? unreachable;
    let oid_buf: [64]u8 = [0; 64];
    let oid_slice: []u8 = oid_buf[0..64];
    const oid_len = formatDerOid(oid_raw, *oid_slice) ?? unreachable;
    expect_eq(bytesEq(oid_buf[0..oid_len], "1.2.840.113549.1.1.11"), true);
    expect_eq(alg.readNull(), true);
    expect_eq(alg.isEmpty(), true);

    const bs = seq.readBitString() ?? unreachable;
    expect_eq(bs.unused_bits, 3);
    expect_eq(bs.bytes.len, 2);
    expect_eq(bs.bytes[0], 0xA0);
    expect_eq(bs.bytes[1], 0xF8);
    expect_eq(seq.isEmpty(), true);
});

test("2. formatDerOid classic RSA, ECDSA, SHA-256, and strict base-128 rejection", () => {
    let out: [64]u8 = [0; 64];
    let out_s: []u8 = out[0..64];

    // sha256WithRSAEncryption: 1.2.840.113549.1.1.11
    const oid_rsa: [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B];
    const n1 = formatDerOid(oid_rsa[0..9], *out_s) ?? unreachable;
    expect_eq(bytesEq(out[0..n1], "1.2.840.113549.1.1.11"), true);

    // ecdsa-with-SHA256: 1.2.840.10045.4.3.2
    const oid_ecdsa: [8]u8 = [0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02];
    const n2 = formatDerOid(oid_ecdsa[0..8], *out_s) ?? unreachable;
    expect_eq(bytesEq(out[0..n2], "1.2.840.10045.4.3.2"), true);

    // id-sha256: 2.16.840.1.101.3.4.2.1
    const oid_sha256: [9]u8 = [0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01];
    const n3 = formatDerOid(oid_sha256[0..9], *out_s) ?? unreachable;
    expect_eq(bytesEq(out[0..n3], "2.16.840.1.101.3.4.2.1"), true);

    // 严格拒绝 base-128 非规范前导 0x80 填充
    const bad_oid_pad: [4]u8 = [0x2A, 0x80, 0x01, 0x02];
    expect_eq(formatDerOid(bad_oid_pad[0..4], *out_s) == null, true);

    // 严格拒绝截断的 base-128 序列（末字节最高位仍为 1）
    const bad_oid_trunc: [2]u8 = [0x2A, 0x86];
    expect_eq(formatDerOid(bad_oid_trunc[0..2], *out_s) == null, true);

    // 缓冲区不足返回 null
    let tiny: [4]u8 = [0; 4];
    let tiny_s: []u8 = tiny[0..4];
    expect_eq(formatDerOid(oid_rsa[0..9], *tiny_s) == null, true);
});

test("3. DerWriter -> DerReader round-trip (SEQUENCE, Bool, U64 boundary, Null, Octet/UTF8)", () => {
    let inner_buf: [256]u8 = [0; 256];
    let iw = DerWriter.init(inner_buf[0..256]);
    expect_eq(iw.writeBool(true), true);
    expect_eq(iw.writeBool(false), true);
    expect_eq(iw.writeU64(0), true);
    expect_eq(iw.writeU64(127), true);
    expect_eq(iw.writeU64(128), true); // 需要 0x00 符号前导字节
    expect_eq(iw.writeU64(0xFFFFFFFFFFFFFFFF), true); // 9 字节 DER INTEGER (0x00 + 8 * 0xFF)
    expect_eq(iw.writeNull(), true);
    expect_eq(iw.writeOctetString("deadbeef"), true);
    expect_eq(iw.writeUtf8String("CP-DER-OK"), true);

    const inner_bytes = iw.written();

    let outer_buf: [300]u8 = [0; 300];
    let ow = DerWriter.init(outer_buf[0..300]);
    expect_eq(ow.writeSequenceHeader(inner_bytes.len), true);
    // 再用 writeTlv 写入完整嵌套结构进行验证
    let full_buf: [300]u8 = [0; 300];
    let fw = DerWriter.init(full_buf[0..300]);
    expect_eq(fw.writeTlv(TAG_SEQUENCE, inner_bytes), true);

    let r = DerReader.init(fw.written());
    let seq = r.readSequence() ?? unreachable;
    expect_eq(seq.readBool() ?? false, true);
    expect_eq(seq.readBool() ?? true, false);
    expect_eq(seq.readU64() ?? 1, 0);
    expect_eq(seq.readU64() ?? 0, 127);
    expect_eq(seq.readU64() ?? 0, 128);
    expect_eq(seq.readU64() ?? 0, 0xFFFFFFFFFFFFFFFF);
    expect_eq(seq.readNull(), true);
    expect_eq(bytesEq(seq.readOctetString() ?? "", "deadbeef"), true);
    const utf8_tlv = seq.nextTlv() ?? unreachable;
    expect_eq(utf8_tlv.tag, TAG_UTF8_STRING);
    expect_eq(bytesEq(utf8_tlv.value, "CP-DER-OK"), true);
    expect_eq(seq.isEmpty(), true);
});

test("4. Strict DER non-canonical rejections (indefinite len, non-minimal len, redundant int pad, bad bool/bitstring)", () => {
    // 1. 不定长编码 0x80
    const indef: [4]u8 = [0x30, 0x80, 0x00, 0x00];
    expect_eq(parseDerTlv(indef[0..4]) == null, true);

    // 2. 长形式表示 < 128 的长度（0x81 0x05）
    const non_min_len: [8]u8 = [0x04, 0x81, 0x05, 1, 2, 3, 4, 5];
    expect_eq(parseDerTlv(non_min_len[0..8]) == null, true);

    // 3. 长形式长度前导零（0x82 0x00 0x80）
    const lead_zero_len: [4]u8 = [0x04, 0x82, 0x00, 0x80];
    expect_eq(parseDerTlv(lead_zero_len[0..4]) == null, true);

    // 4. 整型冗余前导 0x00（0x02 0x02 0x00 0x7F -> 应为 0x02 0x01 0x7F）
    const bad_int_pos: [4]u8 = [0x02, 0x02, 0x00, 0x7F];
    let r1 = DerReader.init(bad_int_pos[0..4]);
    expect_eq(r1.readU64() == null, true);
    expect_eq(r1.readI64() == null, true);
    expect_eq(r1.pos, 0); // 失败不推进游标

    // 5. 整型冗余前导 0xFF（0x02 0x02 0xFF 0x80 -> 应为 0x02 0x01 0x80）
    const bad_int_neg: [4]u8 = [0x02, 0x02, 0xFF, 0x80];
    let r2 = DerReader.init(bad_int_neg[0..4]);
    expect_eq(r2.readI64() == null, true);

    // 6. 合法负整数：readI64 成功 (-128)，readU64 拒绝负数
    const valid_neg: [3]u8 = [0x02, 0x01, 0x80];
    let r3 = DerReader.init(valid_neg[0..3]);
    expect_eq(r3.readU64() == null, true);
    expect_eq(r3.readI64() ?? 0, -128);

    // 7. Boolean 非 0x00 / 0xFF（0x01 非法）
    const bad_bool: [3]u8 = [0x01, 0x01, 0x01];
    let r4 = DerReader.init(bad_bool[0..3]);
    expect_eq(r4.readBool() == null, true);

    // 8. BitString 末字节未用低位非零（unused=3, last=0x01 -> 低 3 位非 0）
    const bad_bs: [4]u8 = [0x03, 0x02, 0x03, 0x01];
    let r5 = DerReader.init(bad_bs[0..4]);
    expect_eq(r5.readBitString() == null, true);

    // 9. BitString unused_bits > 7
    const bad_bs_unused: [4]u8 = [0x03, 0x02, 0x08, 0x00];
    let r6 = DerReader.init(bad_bs_unused[0..4]);
    expect_eq(r6.readBitString() == null, true);
});

test("5. Long-form >= 128 length encoding, truncated TLV, and atomic DerWriter overflow protection", () => {
    // 构造 130 字节 OCTET STRING 验证长形式长度（0x81 0x82）往返
    let payload: [130]u8 = [0x5A; 130];
    let buf: [160]u8 = [0; 160];
    let w = DerWriter.init(buf[0..160]);
    expect_eq(w.writeOctetString(payload[0..130]), true);
    const encoded = w.written();
    expect_eq(encoded.len, 133); // 1B tag + 2B len (0x81 0x82) + 130B payload
    expect_eq(encoded[0], TAG_OCTET_STRING);
    expect_eq(encoded[1], 0x81);
    expect_eq(encoded[2], 0x82);

    let r = DerReader.init(encoded);
    const decoded = r.readOctetString() ?? unreachable;
    expect_eq(decoded.len, 130);
    expect_eq(decoded[0], 0x5A);
    expect_eq(decoded[129], 0x5A);

    // 截断检测（少 1 字节）
    expect_eq(parseDerTlv(encoded[0..132]) == null, true);

    // DerWriter 溢出原子失败：已写内容不被破坏，pos 保持不变
    let small: [5]u8 = [0; 5];
    let sw = DerWriter.init(small[0..5]);
    expect_eq(sw.writeNull(), true); // 占用 2 字节
    expect_eq(sw.pos, 2);
    expect_eq(sw.writeOctetString("1234"), false); // 需要 6 字节，剩余仅 3 字节 -> 原子失败
    expect_eq(sw.pos, 2);
    expect_eq(sw.writeBool(true), true); // 恰好 3 字节，成功
    expect_eq(sw.pos, 5);
});
