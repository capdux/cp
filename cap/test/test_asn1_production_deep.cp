// Source: cp/cap/test/test_asn1_production_deep.cp
// Deep Production Test Suite for cap:asn1
// Covers:
//   1. DerWriter / DerReader round-trip for all primitive types, long lengths, context tags & nested SEQUENCE/SET
//   2. OidCodec authoritative OID encode/decode (RSA/SHA256/Ed25519/CN/SAN) & knownName + error guards
//   3. Malformed DER interception: non-canonical lengths, indefinite length 0x80, truncated TLVs & depth > 64
//   4. Pure-CP X.509 v3 DER certificate synthesis with DerWriter & full X509Inspector.parseDer inspection
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.asn1`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ASN1_MAX_DEPTH,
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
    Asn1Error,
    DerElement,
    DerReader,
    DerWriter,
    OidCodec,
    X509CertInfo,
    X509Inspector,
} from "cap:asn1";
import cap from "cap";

fn writeRdnAttr(alloc: std.mem.Allocator, out: *DerWriter, oid: []const u8, val: []const u8, is_printable: bool)! {
    let atv = DerWriter.init(alloc);
    defer atv.deinit();
    atv.writeOid(oid)?;
    if (is_printable) {
        atv.writePrintableString(val)?;
    } else {
        atv.writeUtf8String(val)?;
    }

    let atv_seq = DerWriter.init(alloc);
    defer atv_seq.deinit();
    atv_seq.writeSequence(atv.asSlice())?;

    out.writeSet(atv_seq.asSlice())?;
}

fn buildSyntheticX509V3Cert(alloc: std.mem.Allocator): ([]u8)! {
    // 1. Build Name (Issuer: CN=CP Root CA, O=CP Foundation)
    let issuer_inner = DerWriter.init(alloc);
    defer issuer_inner.deinit();
    writeRdnAttr(alloc, &issuer_inner, "2.5.4.3", "CP Root CA", false)?;
    writeRdnAttr(alloc, &issuer_inner, "2.5.4.10", "CP Foundation", true)?;

    // 2. Build Name (Subject: CN=api.cp-lang.org, O=CP Cloud)
    let subject_inner = DerWriter.init(alloc);
    defer subject_inner.deinit();
    writeRdnAttr(alloc, &subject_inner, "2.5.4.3", "api.cp-lang.org", false)?;
    writeRdnAttr(alloc, &subject_inner, "2.5.4.10", "CP Cloud", true)?;

    // 3. Build signature AlgorithmIdentifier (sha256WithRSAEncryption + NULL)
    let sig_alg_inner = DerWriter.init(alloc);
    defer sig_alg_inner.deinit();
    sig_alg_inner.writeOid("1.2.840.113549.1.1.11")?;
    sig_alg_inner.writeNull()?;

    // 4. Build Validity (UTCTime 2025-01-01 .. GeneralizedTime 2035-12-31)
    let validity_inner = DerWriter.init(alloc);
    defer validity_inner.deinit();
    validity_inner.writeUtcTime("250101000000Z")?;
    validity_inner.writeGeneralizedTime("20351231235959Z")?;

    // 5. Build SubjectPublicKeyInfo (ecPublicKey + prime256v1 + 65-byte dummy SEC1 point)
    let spki_alg_inner = DerWriter.init(alloc);
    defer spki_alg_inner.deinit();
    spki_alg_inner.writeOid("1.2.840.10045.2.1")?;
    spki_alg_inner.writeOid("1.2.840.10045.3.1.7")?;

    let spki_inner = DerWriter.init(alloc);
    defer spki_inner.deinit();
    spki_inner.writeSequence(spki_alg_inner.asSlice())?;
    let dummy_pub: [65]u8 = [0x04; 65];
    spki_inner.writeBitString(0, &dummy_pub)?;

    // 6. Build Extensions (BasicConstraints cA=TRUE + SAN dNSName x3)
    let bc_val_inner = DerWriter.init(alloc);
    defer bc_val_inner.deinit();
    bc_val_inner.writeBoolean(true)?;
    let bc_val_seq = DerWriter.init(alloc);
    defer bc_val_seq.deinit();
    bc_val_seq.writeSequence(bc_val_inner.asSlice())?;

    let bc_ext_inner = DerWriter.init(alloc);
    defer bc_ext_inner.deinit();
    bc_ext_inner.writeOid("2.5.29.19")?;
    bc_ext_inner.writeBoolean(true)?;
    bc_ext_inner.writeOctetString(bc_val_seq.asSlice())?;

    let san_names_inner = DerWriter.init(alloc);
    defer san_names_inner.deinit();
    san_names_inner.writeContextImplicit(2, false, "api.cp-lang.org")?;
    san_names_inner.writeContextImplicit(2, false, "*.cp-lang.org")?;
    san_names_inner.writeContextImplicit(2, false, "edge.cp-lang.org")?;
    let san_val_seq = DerWriter.init(alloc);
    defer san_val_seq.deinit();
    san_val_seq.writeSequence(san_names_inner.asSlice())?;

    let san_ext_inner = DerWriter.init(alloc);
    defer san_ext_inner.deinit();
    san_ext_inner.writeOid("2.5.29.17")?;
    san_ext_inner.writeOctetString(san_val_seq.asSlice())?;

    let exts_seq_inner = DerWriter.init(alloc);
    defer exts_seq_inner.deinit();
    exts_seq_inner.writeSequence(bc_ext_inner.asSlice())?;
    exts_seq_inner.writeSequence(san_ext_inner.asSlice())?;

    let exts_seq_tlv = DerWriter.init(alloc);
    defer exts_seq_tlv.deinit();
    exts_seq_tlv.writeSequence(exts_seq_inner.asSlice())?;

    // 7. Build TBSCertificate
    let ver_int = DerWriter.init(alloc);
    defer ver_int.deinit();
    ver_int.writeUint64(2)?; // v3 = 2

    let tbs_inner = DerWriter.init(alloc);
    defer tbs_inner.deinit();
    tbs_inner.writeContextExplicit(0, ver_int.asSlice())?;
    const serial_bytes: [4]u8 = [0x9A, 0xBC, 0xDE, 0x01];
    tbs_inner.writeIntegerBytes(&serial_bytes)?;
    tbs_inner.writeSequence(sig_alg_inner.asSlice())?;
    tbs_inner.writeSequence(issuer_inner.asSlice())?;
    tbs_inner.writeSequence(validity_inner.asSlice())?;
    tbs_inner.writeSequence(subject_inner.asSlice())?;
    tbs_inner.writeSequence(spki_inner.asSlice())?;
    tbs_inner.writeContextExplicit(3, exts_seq_tlv.asSlice())?;

    // 8. Wrap into outer Certificate SEQUENCE
    let cert_inner = DerWriter.init(alloc);
    defer cert_inner.deinit();
    cert_inner.writeSequence(tbs_inner.asSlice())?;
    cert_inner.writeSequence(sig_alg_inner.asSlice())?;
    const dummy_sig: [8]u8 = [0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE];
    cert_inner.writeBitString(0, &dummy_sig)?;

    let cert_outer = DerWriter.init(alloc);
    errdefer cert_outer.deinit();
    cert_outer.writeSequence(cert_inner.asSlice())?;
    return cert_outer.toOwnedSlice();
}

fn testDerWriterReaderRoundTrip()! {
    const alloc = std.heap.page_allocator;

    let inner = DerWriter.init(alloc);
    defer inner.deinit();
    inner.writeBoolean(true)?;
    inner.writeBoolean(false)?;
    inner.writeUint64(0)?;
    inner.writeUint64(127)?;
    inner.writeUint64(128)?;
    inner.writeUint64(0x123456789ABCDEF0)?;

    const hi_bit_bytes: [2]u8 = [0x80, 0x01];
    inner.writeIntegerBytes(&hi_bit_bytes)?;
    const redundant_zero_bytes: [3]u8 = [0x00, 0x00, 0x05];
    inner.writeIntegerBytes(&redundant_zero_bytes)?;

    inner.writeOctetString("raw\x00bytes")?;
    const bs_payload: [2]u8 = [0xF0, 0xA8];
    inner.writeBitString(3, &bs_payload)?;
    inner.writeUtf8String("CP-语言")?;
    inner.writePrintableString("CP-Org")?;
    inner.writeIa5String("dev@cp-lang.org")?;
    inner.writeNull()?;
    inner.writeOid("1.3.101.112")?;

    // Long-form payload (> 127 bytes) inside SET and [2] EXPLICIT
    let long_buf: [140]u8 = [0x41; 140];
    let set_inner = DerWriter.init(alloc);
    defer set_inner.deinit();
    set_inner.writeOctetString(&long_buf)?;
    inner.writeSet(set_inner.asSlice())?;

    let ctx_inner = DerWriter.init(alloc);
    defer ctx_inner.deinit();
    ctx_inner.writeUint64(42)?;
    inner.writeContextExplicit(2, ctx_inner.asSlice())?;

    let outer = DerWriter.init(alloc);
    defer outer.deinit();
    outer.writeSequence(inner.asSlice())?;
    const der = outer.toOwnedSlice()?;
    defer alloc.free(der);

    let r = DerReader.init(der);
    let seq = r.readSequence()?;
    expect_eq(r.hasNext(), false);

    expect_eq(seq.readBoolean()?, true);
    expect_eq(seq.readBoolean()?, false);
    expect_eq(seq.readUint64()?, 0 as u64);
    expect_eq(seq.readUint64()?, 127 as u64);
    expect_eq(seq.readUint64()?, 128 as u64);
    expect_eq(seq.readUint64()?, 0x123456789ABCDEF0 as u64);

    // High-bit positive integer must carry leading 0x00 sign guard
    const hi_elem = seq.readElement()?;
    const hi_raw = hi_elem.asIntegerBytes()?;
    expect_eq(hi_raw.len, 3 as usize);
    expect_eq(hi_raw[0], 0x00 as u8);
    expect_eq(hi_raw[1], 0x80 as u8);
    expect_eq(hi_raw[2], 0x01 as u8);
    expect_eq(hi_elem.asUint64()?, 0x8001 as u64);

    // Redundant leading zeros stripped to canonical 1 byte
    const red_raw = seq.readIntegerBytes()?;
    expect_eq(red_raw.len, 1 as usize);
    expect_eq(red_raw[0], 0x05 as u8);

    expect_eq(std.mem.eql(u8, seq.readOctetString()?, "raw\x00bytes"), true);

    const bs_elem = seq.readElement()?;
    expect_eq(bs_elem.asBitStringUnusedBits()?, 3 as u8);
    expect_eq(std.mem.eql(u8, bs_elem.asBitString()?, &bs_payload), true);

    expect_eq(std.mem.eql(u8, seq.readString()?, "CP-语言"), true);
    expect_eq(std.mem.eql(u8, seq.readString()?, "CP-Org"), true);
    expect_eq(std.mem.eql(u8, seq.readString()?, "dev@cp-lang.org"), true);
    seq.readNull()?;

    const decoded_oid = seq.readOid(alloc)?;
    defer alloc.free(decoded_oid);
    expect_eq(std.mem.eql(u8, decoded_oid, "1.3.101.112"), true);

    // Long-form SET (>127 bytes -> multi-byte length 0x81 0x8F)
    let set_r = seq.readSet()?;
    const long_out = set_r.readOctetString()?;
    expect_eq(long_out.len, 140 as usize);
    expect_eq(long_out[0], 0x41 as u8);
    expect_eq(long_out[139], 0x41 as u8);
    expect_eq(set_r.hasNext(), false);

    // hasContextExplicit & tryReadContextExplicit miss & hit
    expect_eq(seq.hasContextExplicit(0), false);
    const miss_ctx = seq.tryReadContextExplicit(0) catch unreachable;
    expect_eq(miss_ctx == null, true);
    expect_eq(seq.hasContextExplicit(2), true);
    let ctx_r = (seq.tryReadContextExplicit(2) catch unreachable).?;
    expect_eq(ctx_r.readUint64()?, 42 as u64);
    expect_eq(ctx_r.hasNext(), false);
    expect_eq(seq.hasNext(), false);
}

fn testOidCodecAndKnownNames()! {
    const alloc = std.heap.page_allocator;

    // 1. Authoritative SHA256WithRSA OID bytes: 1.2.840.113549.1.1.11
    const sha256_rsa_oid = "1.2.840.113549.1.1.11";
    const encoded = OidCodec.encode(alloc, sha256_rsa_oid)?;
    defer alloc.free(encoded);
    const expected_bytes: [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B];
    expect_eq(std.mem.eql(u8, encoded, &expected_bytes), true);

    const decoded = OidCodec.decode(alloc, encoded)?;
    defer alloc.free(decoded);
    expect_eq(std.mem.eql(u8, decoded, sha256_rsa_oid), true);
    expect_eq(std.mem.eql(u8, OidCodec.knownName(sha256_rsa_oid).?, "sha256WithRSAEncryption"), true);

    // 2. Round-trip additional standard & arc=2 large OIDs
    const oids: [6][]const u8 = [
        "1.2.840.113549.1.1.1",
        "1.2.840.10045.4.3.2",
        "1.3.101.112",
        "2.5.4.3",
        "2.5.29.17",
        "2.999.123456.7.0",
    ];
    const names: [6]([]const u8)? = [
        "rsaEncryption",
        "ecdsa-with-SHA256",
        "Ed25519",
        "CN",
        "subjectAltName",
        null,
    ];
    let idx: usize = 0;
    while (idx < 6) : (idx += 1) {
        const b = OidCodec.encode(alloc, oids[idx])?;
        defer alloc.free(b);
        const s = OidCodec.decode(alloc, b)?;
        defer alloc.free(s);
        expect_eq(std.mem.eql(u8, s, oids[idx]), true);
        if (exp_name := names[idx]) {
            expect_eq(std.mem.eql(u8, OidCodec.knownName(oids[idx]).?, exp_name), true);
        } else {
            expect_eq(OidCodec.knownName(oids[idx]) == null, true);
        }
    }

    // 3. Invalid OID rejection
    const bad_oids: [6][]const u8 = ["", "1", "3.1.2", "1.40.1", "1.2.01", "1.2."];
    for (bad of bad_oids) {
        let hit = false;
        * = OidCodec.encode(alloc, bad) catch |e| {
            if (e == Asn1Error.InvalidOid) hit = true;
        };
        expect_eq(hit, true);
    }

    // Non-canonical VLQ (leading 0x80) and truncated VLQ
    const bad_vlq: [2]u8 = [0x80, 0x01];
    let vlq_hit = false;
    * = OidCodec.decode(alloc, &bad_vlq) catch |e| {
        if (e == Asn1Error.InvalidOid) vlq_hit = true;
    };
    expect_eq(vlq_hit, true);

    const trunc_vlq: [2]u8 = [0x2A, 0x86];
    let trunc_hit = false;
    * = OidCodec.decode(alloc, &trunc_vlq) catch |e| {
        if (e == Asn1Error.InvalidOid) trunc_hit = true;
    };
    expect_eq(trunc_hit, true);
}

fn testInvalidDerRejection()! {
    const alloc = std.heap.page_allocator;

    // 1. Non-canonical length (<128 encoded using 0x81 0x05)
    const non_canon_short: [7]u8 = [TAG_OCTET_STRING, 0x81, 0x05, 'h', 'e', 'l', 'l'];
    let r1 = DerReader.init(&non_canon_short);
    let hit1 = false;
    * = r1.readElement() catch |e| {
        if (e == Asn1Error.NonCanonicalLength) hit1 = true;
    };
    expect_eq(hit1, true);

    // 2. Non-canonical leading zero byte in length (0x82 0x00 0x80)
    const non_canon_zero: [4]u8 = [TAG_OCTET_STRING, 0x82, 0x00, 0x80];
    let r2 = DerReader.init(&non_canon_zero);
    let hit2 = false;
    * = r2.readElement() catch |e| {
        if (e == Asn1Error.NonCanonicalLength) hit2 = true;
    };
    expect_eq(hit2, true);

    // 3. Indefinite length 0x80 (BER only, forbidden in DER)
    const indef_seq: [4]u8 = [TAG_SEQUENCE, 0x80, 0x00, 0x00];
    let r3 = DerReader.init(&indef_seq);
    let hit3 = false;
    * = r3.readElement() catch |e| {
        if (e == Asn1Error.InvalidLength) hit3 = true;
    };
    expect_eq(hit3, true);

    // 4. Truncated TLV content
    const trunc_tlv: [5]u8 = [TAG_OCTET_STRING, 0x0A, 'a', 'b', 'c'];
    let r4 = DerReader.init(&trunc_tlv);
    let hit4 = false;
    * = r4.readElement() catch |e| {
        if (e == Asn1Error.UnexpectedEof) hit4 = true;
    };
    expect_eq(hit4, true);

    // 5. Non-canonical boolean (0x01 instead of 0xFF) & non-canonical integer (0x00 0x7F)
    const bad_bool: [3]u8 = [TAG_BOOLEAN, 0x01, 0x01];
    let r5 = DerReader.init(&bad_bool);
    let hit5 = false;
    * = r5.readBoolean() catch |e| {
        if (e == Asn1Error.NonCanonicalLength) hit5 = true;
    };
    expect_eq(hit5, true);

    const bad_int: [4]u8 = [TAG_INTEGER, 0x02, 0x00, 0x7F];
    let r6 = DerReader.init(&bad_int);
    let hit6 = false;
    * = r6.readIntegerBytes() catch |e| {
        if (e == Asn1Error.NonCanonicalLength) hit6 = true;
    };
    expect_eq(hit6, true);

    // 6. Nesting depth > ASN1_MAX_DEPTH (64)
    let deep_payload = DerWriter.init(alloc);
    defer deep_payload.deinit();
    deep_payload.writeNull()?;
    let d: usize = 0;
    while (d <= ASN1_MAX_DEPTH) : (d += 1) {
        let next_wrap = DerWriter.init(alloc);
        defer next_wrap.deinit();
        next_wrap.writeSequence(deep_payload.asSlice())?;
        deep_payload.reset();
        deep_payload.buf.appendSlice(alloc, next_wrap.asSlice()) catch return Asn1Error.OutOfMemory;
    }
    let cur = DerReader.init(deep_payload.asSlice());
    let depth_hit = false;
    let step: usize = 0;
    while (step <= ASN1_MAX_DEPTH + 1) : (step += 1) {
        cur = cur.readSequence() catch |e| {
            if (e == Asn1Error.MaxDepthExceeded) depth_hit = true;
            break;
        };
    }
    expect_eq(depth_hit, true);
}

fn testX509V3CertificateInspection()! {
    const alloc = std.heap.page_allocator;
    const cert_der = buildSyntheticX509V3Cert(alloc)?;
    defer alloc.free(cert_der);

    let info = X509Inspector.parseDer(alloc, cert_der)?;
    defer info.deinit();

    expect_eq(info.version, 3 as u8);
    expect_eq(std.mem.eql(u8, info.serial_hex, "9abcde01"), true);
    expect_eq(std.mem.eql(u8, info.sig_alg_oid, "1.2.840.113549.1.1.11"), true);
    expect_eq(std.mem.eql(u8, info.issuer_cn, "CP Root CA"), true);
    expect_eq(std.mem.eql(u8, info.issuer_org, "CP Foundation"), true);
    expect_eq(std.mem.eql(u8, info.subject_cn, "api.cp-lang.org"), true);
    expect_eq(std.mem.eql(u8, info.subject_org, "CP Cloud"), true);
    expect_eq(std.mem.eql(u8, info.pub_key_alg_oid, "1.2.840.10045.2.1"), true);
    expect_eq(info.is_ca, true);

    expect_eq(info.san_dns.len, 3 as usize);
    expect_eq(std.mem.eql(u8, info.san_dns[0], "api.cp-lang.org"), true);
    expect_eq(std.mem.eql(u8, info.san_dns[1], "*.cp-lang.org"), true);
    expect_eq(std.mem.eql(u8, info.san_dns[2], "edge.cp-lang.org"), true);

    // 2025-01-01T00:00:00Z = 1735689600, 2035-12-31T23:59:59Z = 2082758399
    expect_eq(info.not_before_sec, 1735689600 as i64);
    expect_eq(info.not_after_sec, 2082758399 as i64);
    expect_eq(info.isValidAt(1735689600), true);
    expect_eq(info.isValidAt(1900000000), true);
    expect_eq(info.isValidAt(2082758399), true);
    expect_eq(info.isValidAt(1735689599), false);
    expect_eq(info.isValidAt(2082758400), false);

    // Verify SHA-256 fingerprint independently
    let expected_digest: [32]u8 = [0; 32];
    std.crypto.hash.sha2.Sha256.hash(cert_der, &expected_digest, .{});
    const hex_chars = "0123456789abcdef";
    let expected_hex: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        expected_hex[i * 2] = hex_chars[(expected_digest[i] >> 4) & 0x0F];
        expected_hex[i * 2 + 1] = hex_chars[expected_digest[i] & 0x0F];
    }
    expect_eq(std.mem.eql(u8, info.fingerprintSlice(), &expected_hex), true);
    expect_eq(std.mem.eql(u8, info.sha256_fingerprint_hex[0..64], &expected_hex), true);
    const fp_copy = info.fingerprintHex();
    expect_eq(std.mem.eql(u8, &fp_copy, &expected_hex), true);

    // Direct time parser checks: leap day 2024-02-29 & 1999-12-31
    const leap_sec = X509Inspector.parseTimeToUnixSec(TAG_GENERALIZED_TIME, "20240229123045Z")?;
    expect_eq(leap_sec, 1709209845 as i64);
    const y1999 = X509Inspector.parseTimeToUnixSec(TAG_UTC_TIME, "991231235959Z")?;
    expect_eq(y1999, 946684799 as i64);

    // Invalid non-leap Feb 29 (2025)
    let bad_time_hit = false;
    * = X509Inspector.parseTimeToUnixSec(TAG_GENERALIZED_TIME, "20250229000000Z") catch |e| {
        if (e == Asn1Error.InvalidTime) bad_time_hit = true;
    };
    expect_eq(bad_time_hit, true);
}

fn testCapAsn1DynamicMount()! {
    const alloc = std.heap.page_allocator;

    // 1. Use cap.asn1.OidCodec & cap.asn1.DerWriter to synthesize a minimal X.509 v1 certificate
    const oid_bytes = cap.asn1.OidCodec.encode(alloc, "1.3.101.112")?;
    defer alloc.free(oid_bytes);
    expect_eq(std.mem.eql(u8, cap.asn1.OidCodec.knownName("1.3.101.112").?, "Ed25519"), true);

    let name_w = cap.asn1.DerWriter.init(alloc);
    defer name_w.deinit();
    writeRdnAttr(alloc, &name_w, "2.5.4.3", "v1.cp.local", false)?;

    let alg_w = cap.asn1.DerWriter.init(alloc);
    defer alg_w.deinit();
    alg_w.writeOid("1.3.101.112")?;

    let val_w = cap.asn1.DerWriter.init(alloc);
    defer val_w.deinit();
    val_w.writeUtcTime("260101000000Z")?;
    val_w.writeUtcTime("270101000000Z")?;

    let spki_w = cap.asn1.DerWriter.init(alloc);
    defer spki_w.deinit();
    spki_w.writeSequence(alg_w.asSlice())?;
    const ed_pub: [32]u8 = [0x11; 32];
    spki_w.writeBitString(0, &ed_pub)?;

    let tbs_w = cap.asn1.DerWriter.init(alloc);
    defer tbs_w.deinit();
    tbs_w.writeUint64(7)?;
    tbs_w.writeSequence(alg_w.asSlice())?;
    tbs_w.writeSequence(name_w.asSlice())?;
    tbs_w.writeSequence(val_w.asSlice())?;
    tbs_w.writeSequence(name_w.asSlice())?;
    tbs_w.writeSequence(spki_w.asSlice())?;

    let cert_inner = cap.asn1.DerWriter.init(alloc);
    defer cert_inner.deinit();
    cert_inner.writeSequence(tbs_w.asSlice())?;
    cert_inner.writeSequence(alg_w.asSlice())?;
    const sig: [4]u8 = [1, 2, 3, 4];
    cert_inner.writeBitString(0, &sig)?;

    let cert_outer = cap.asn1.DerWriter.init(alloc);
    defer cert_outer.deinit();
    cert_outer.writeSequence(cert_inner.asSlice())?;
    const der = cert_outer.toOwnedSlice()?;
    defer alloc.free(der);

    let info = cap.asn1.X509Inspector.parseDer(alloc, der)?;
    defer info.deinit();

    expect_eq(info.version, 1 as u8);
    expect_eq(std.mem.eql(u8, info.serial_hex, "07"), true);
    expect_eq(std.mem.eql(u8, info.subject_cn, "v1.cp.local"), true);
    expect_eq(std.mem.eql(u8, info.pub_key_alg_oid, "1.3.101.112"), true);
    expect_eq(info.san_dns.len, 0 as usize);
    expect_eq(info.is_ca, false);
}

fn main() {
    test("1. DerWriter / DerReader Primitive Round-Trip, Long Lengths & Nested SEQUENCE/SET", () => {
        testDerWriterReaderRoundTrip()?;
    });

    test("2. OidCodec Authoritative Encoding/Decoding & knownName Registry", () => {
        testOidCodecAndKnownNames()?;
    });

    test("3. Malformed DER Rejection: Non-Canonical Lengths, Indefinite 0x80, Truncated & MaxDepth", () => {
        testInvalidDerRejection()?;
    });

    test("4. Pure-CP X.509 v3 Certificate Synthesis & X509Inspector.parseDer Verification", () => {
        testX509V3CertificateInspection()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.asn1`", () => {
        testCapAsn1DynamicMount()?;
    });
}
