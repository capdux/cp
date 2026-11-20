// Source: cp/cap/test/test_x509_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:x509 & cap.x509

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { DerWriter, TAG_UTC_TIME, TAG_GENERALIZED_TIME } from "cap:asn1";
import { encode as encodePem } from "cap:pem";
import {
    X509Certificate,
    X509Error,
    X509Time,
    SanEntry,
    BasicConstraints,
    parseAsn1Time,
} from "cap:x509";

fn buildX500Name(alloc: std.mem.Allocator, cn: []const u8): ([]u8) ! X509Error {
    let oid_w = DerWriter.init(alloc);
    defer oid_w.deinit();
    oid_w.writeOid("2.5.4.3") catch return X509Error.OutOfMemory;
    oid_w.writeUtf8String(cn) catch return X509Error.OutOfMemory;

    let atv_w = DerWriter.init(alloc);
    defer atv_w.deinit();
    atv_w.writeSequence(oid_w.asSlice()) catch return X509Error.OutOfMemory;

    let rdn_w = DerWriter.init(alloc);
    defer rdn_w.deinit();
    rdn_w.writeSet(atv_w.asSlice()) catch return X509Error.OutOfMemory;

    let name_w = DerWriter.init(alloc);
    defer name_w.deinit();
    name_w.writeSequence(rdn_w.asSlice()) catch return X509Error.OutOfMemory;
    return name_w.toOwnedSlice() catch return X509Error.OutOfMemory;
}

fn buildAlgId(alloc: std.mem.Allocator, oid: []const u8): ([]u8) ! X509Error {
    let inner = DerWriter.init(alloc);
    defer inner.deinit();
    inner.writeOid(oid) catch return X509Error.OutOfMemory;
    inner.writeNull() catch return X509Error.OutOfMemory;

    let seq = DerWriter.init(alloc);
    defer seq.deinit();
    seq.writeSequence(inner.asSlice()) catch return X509Error.OutOfMemory;
    return seq.toOwnedSlice() catch return X509Error.OutOfMemory;
}

fn buildExtension(alloc: std.mem.Allocator, oid: []const u8, critical: bool, value_der: []const u8): ([]u8) ! X509Error {
    let inner = DerWriter.init(alloc);
    defer inner.deinit();
    inner.writeOid(oid) catch return X509Error.OutOfMemory;
    if (critical) {
        inner.writeBoolean(true) catch return X509Error.OutOfMemory;
    }
    inner.writeOctetString(value_der) catch return X509Error.OutOfMemory;

    let seq = DerWriter.init(alloc);
    defer seq.deinit();
    seq.writeSequence(inner.asSlice()) catch return X509Error.OutOfMemory;
    return seq.toOwnedSlice() catch return X509Error.OutOfMemory;
}

fn buildTestCertDer(
    alloc: std.mem.Allocator,
    serial: u64,
    issuer_cn: []const u8,
    subject_cn: []const u8,
    not_before_str: []const u8,
    not_before_gen: bool,
    not_after_str: []const u8,
    not_after_gen: bool,
    is_ca: bool,
    path_len: u32?,
    dns_sans: []const []const u8,
    ipv4_san: ([4]u8)?,
): ([]u8) ! X509Error {
    let tbs_inner = DerWriter.init(alloc);
    defer tbs_inner.deinit();

    // [0] EXPLICIT version = 2 (v3)
    let ver_w = DerWriter.init(alloc);
    defer ver_w.deinit();
    ver_w.writeUint64(2) catch return X509Error.OutOfMemory;
    tbs_inner.writeContextExplicit(0, ver_w.asSlice()) catch return X509Error.OutOfMemory;

    // serialNumber
    tbs_inner.writeUint64(serial) catch return X509Error.OutOfMemory;

    // signature AlgorithmIdentifier (sha256WithRSAEncryption)
    const sig_alg = buildAlgId(alloc, "1.2.840.113549.1.1.11")?;
    defer alloc.free(sig_alg);
    tbs_inner.writeTlv(0x30, sig_alg[2..]) catch return X509Error.OutOfMemory;

    // Wait: sig_alg already has tag+len, write raw bytes directly by appending to list or re-using helper.
    // Let's reset tbs_inner and write raw TLV cleanly via DerWriter's buf, or build using inner contents.
    return buildTestCertDerClean(
        alloc, serial, issuer_cn, subject_cn,
        not_before_str, not_before_gen, not_after_str, not_after_gen,
        is_ca, path_len, dns_sans, ipv4_san,
    );
}

fn appendRaw(w: *DerWriter, raw: []const u8): void ! X509Error {
    w.buf.appendSlice(w.alloc, raw) catch return X509Error.OutOfMemory;
}

fn buildTestCertDerClean(
    alloc: std.mem.Allocator,
    serial: u64,
    issuer_cn: []const u8,
    subject_cn: []const u8,
    not_before_str: []const u8,
    not_before_gen: bool,
    not_after_str: []const u8,
    not_after_gen: bool,
    is_ca: bool,
    path_len: u32?,
    dns_sans: []const []const u8,
    ipv4_san: ([4]u8)?,
): ([]u8) ! X509Error {
    let tbs_inner = DerWriter.init(alloc);
    defer tbs_inner.deinit();

    // [0] EXPLICIT version = 2 (v3)
    let ver_w = DerWriter.init(alloc);
    defer ver_w.deinit();
    ver_w.writeUint64(2) catch return X509Error.OutOfMemory;
    tbs_inner.writeContextExplicit(0, ver_w.asSlice()) catch return X509Error.OutOfMemory;

    // serialNumber
    tbs_inner.writeUint64(serial) catch return X509Error.OutOfMemory;

    // signature AlgorithmIdentifier
    const sig_alg = buildAlgId(alloc, "1.2.840.113549.1.1.11")?;
    defer alloc.free(sig_alg);
    appendRaw(&tbs_inner, sig_alg)?;

    // issuer Name
    const issuer_der = buildX500Name(alloc, issuer_cn)?;
    defer alloc.free(issuer_der);
    appendRaw(&tbs_inner, issuer_der)?;

    // validity SEQUENCE
    let val_inner = DerWriter.init(alloc);
    defer val_inner.deinit();
    if (not_before_gen) {
        val_inner.writeGeneralizedTime(not_before_str) catch return X509Error.OutOfMemory;
    } else {
        val_inner.writeUtcTime(not_before_str) catch return X509Error.OutOfMemory;
    }
    if (not_after_gen) {
        val_inner.writeGeneralizedTime(not_after_str) catch return X509Error.OutOfMemory;
    } else {
        val_inner.writeUtcTime(not_after_str) catch return X509Error.OutOfMemory;
    }
    tbs_inner.writeSequence(val_inner.asSlice()) catch return X509Error.OutOfMemory;

    // subject Name
    const subject_der = buildX500Name(alloc, subject_cn)?;
    defer alloc.free(subject_der);
    appendRaw(&tbs_inner, subject_der)?;

    // subjectPublicKeyInfo SEQUENCE
    let spki_inner = DerWriter.init(alloc);
    defer spki_inner.deinit();
    const pk_alg = buildAlgId(alloc, "1.2.840.113549.1.1.1")?;
    defer alloc.free(pk_alg);
    appendRaw(&spki_inner, pk_alg)?;
    const dummy_pk: [4]u8 = [0x01, 0x02, 0x03, 0x04];
    spki_inner.writeBitString(0, &dummy_pk) catch return X509Error.OutOfMemory;
    tbs_inner.writeSequence(spki_inner.asSlice()) catch return X509Error.OutOfMemory;

    // [3] EXPLICIT extensions
    let exts_inner = DerWriter.init(alloc);
    defer exts_inner.deinit();

    // BasicConstraints (2.5.29.19)
    let bc_inner = DerWriter.init(alloc);
    defer bc_inner.deinit();
    bc_inner.writeBoolean(is_ca) catch return X509Error.OutOfMemory;
    if (pl := path_len) {
        bc_inner.writeUint64(pl as u64) catch return X509Error.OutOfMemory;
    }
    let bc_seq = DerWriter.init(alloc);
    defer bc_seq.deinit();
    bc_seq.writeSequence(bc_inner.asSlice()) catch return X509Error.OutOfMemory;
    const bc_ext = buildExtension(alloc, "2.5.29.19", true, bc_seq.asSlice())?;
    defer alloc.free(bc_ext);
    appendRaw(&exts_inner, bc_ext)?;

    // SAN (2.5.29.17)
    if (dns_sans.len > 0 || ipv4_san != null) {
        let gn_inner = DerWriter.init(alloc);
        defer gn_inner.deinit();
        for (d of dns_sans) {
            gn_inner.writeContextImplicit(2, false, d) catch return X509Error.OutOfMemory;
        }
        if (ip4 := ipv4_san) {
            gn_inner.writeContextImplicit(7, false, &ip4) catch return X509Error.OutOfMemory;
        }
        let gn_seq = DerWriter.init(alloc);
        defer gn_seq.deinit();
        gn_seq.writeSequence(gn_inner.asSlice()) catch return X509Error.OutOfMemory;
        const san_ext = buildExtension(alloc, "2.5.29.17", false, gn_seq.asSlice())?;
        defer alloc.free(san_ext);
        appendRaw(&exts_inner, san_ext)?;
    }

    let exts_seq = DerWriter.init(alloc);
    defer exts_seq.deinit();
    exts_seq.writeSequence(exts_inner.asSlice()) catch return X509Error.OutOfMemory;
    tbs_inner.writeContextExplicit(3, exts_seq.asSlice()) catch return X509Error.OutOfMemory;

    // Wrap Certificate SEQUENCE { tbsCertificate, signatureAlgorithm, signatureValue }
    let cert_inner = DerWriter.init(alloc);
    defer cert_inner.deinit();
    cert_inner.writeSequence(tbs_inner.asSlice()) catch return X509Error.OutOfMemory;
    appendRaw(&cert_inner, sig_alg)?;
    const dummy_sig: [4]u8 = [0xAA, 0xBB, 0xCC, 0xDD];
    cert_inner.writeBitString(0, &dummy_sig) catch return X509Error.OutOfMemory;

    let root = DerWriter.init(alloc);
    defer root.deinit();
    root.writeSequence(cert_inner.asSlice()) catch return X509Error.OutOfMemory;
    return root.toOwnedSlice() catch return X509Error.OutOfMemory;
}

fn emptyCert(): X509Certificate {
    return X509Certificate{};
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    const sans: [2][]const u8 = ["api.cp.dev", "*.edge.cp.dev"];
    const ip4: [4]u8 = [10, 20, 30, 40];

    const der = buildTestCertDer(
        alloc,
        0x123456,
        "CP Root CA",
        "api.cp.dev",
        "250101000000Z", false,
        "20300101000000Z", true,
        false,
        null,
        &sans,
        ip4,
    )?;
    defer alloc.free(der);

    let cert = X509Certificate.parseDer(alloc, der)?;
    defer cert.deinit(alloc);

    expect_eq(cert.version, 3 as u8);
    const exp_serial: [3]u8 = [0x12, 0x34, 0x56];
    expect_eq(std.mem.eql(u8, cert.serial_number, &exp_serial), true);
    expect_eq(std.mem.eql(u8, cert.signature_oid, "1.2.840.113549.1.1.11"), true);
    expect_eq(cert.issuer_cn != null, true);
    expect_eq(std.mem.eql(u8, cert.issuer_cn.?, "CP Root CA"), true);
    expect_eq(cert.subject_cn != null, true);
    expect_eq(std.mem.eql(u8, cert.subject_cn.?, "api.cp.dev"), true);
    expect_eq(cert.spki_raw.len > 0, true);
    expect_eq(cert.basic_constraints != null, true);
    expect_eq(cert.basic_constraints.?.is_ca, false);
    expect_eq(cert.sans.len, 3 as usize);

    // Verify UTCTime & GeneralizedTime timestamps
    const t_utc = parseAsn1Time(TAG_UTC_TIME, "250101000000Z")?;
    const t_gen = parseAsn1Time(TAG_GENERALIZED_TIME, "20300101000000Z")?;
    expect_eq(cert.not_before, t_utc);
    expect_eq(cert.not_after, t_gen);

    // Verify SHA-256 fingerprint matches std.crypto directly
    let expected_sha: [32]u8 = [0; 32];
    std.crypto.hash.sha2.Sha256.hash(der, &expected_sha, .{});
    const got_raw = cert.fingerprintSha256Raw();
    expect_eq(std.mem.eql(u8, &got_raw, &expected_sha), true);

    const hex_fp = cert.fingerprintSha256(alloc)?;
    defer alloc.free(hex_fp);
    expect_eq(hex_fp.len, 64 as usize);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    const sans: [1][]const u8 = ["svc.cp.dev"];
    const der = buildTestCertDer(
        alloc,
        42,
        "CP Intermediate CA",
        "svc.cp.dev",
        "260101000000Z", false,
        "270101000000Z", false,
        false,
        null,
        &sans,
        null,
    )?;
    defer alloc.free(der);

    const pem_text = encodePem(alloc, "CERTIFICATE", der)?;
    defer alloc.free(pem_text);

    let cert = X509Certificate.parsePem(alloc, pem_text)?;
    defer cert.deinit(alloc);

    expect_eq(cert.isValidAt(cert.not_before - 1), false);
    expect_eq(cert.isValidAt(cert.not_before), true);
    expect_eq(cert.isValidAt(cert.not_before + 86400), true);
    expect_eq(cert.isValidAt(cert.not_after), true);
    expect_eq(cert.isValidAt(cert.not_after + 1), false);

    // Verify X509Time wrapper
    const xt = X509Time.parse(TAG_UTC_TIME, "260101000000Z")?;
    expect_eq(xt.toUnixSec(), cert.not_before);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;
    const sans: [1][]const u8 = ["*.api.cp.dev"];
    const ip4: [4]u8 = [192, 168, 10, 5];

    const der = buildTestCertDer(
        alloc,
        99,
        "CP CA",
        "ignored.cn.cp.dev",
        "260101000000Z", false,
        "270101000000Z", false,
        false,
        null,
        &sans,
        ip4,
    )?;
    defer alloc.free(der);

    let cert = X509Certificate.parseDer(alloc, der)?;
    defer cert.deinit(alloc);

    // RFC 6125 single-label wildcard + case-insensitive
    expect_eq(cert.matchesHostname("v1.api.cp.dev"), true);
    expect_eq(cert.matchesHostname("V1.API.CP.DEV"), true);
    expect_eq(cert.matchesHostname("x.y.api.cp.dev"), false);
    expect_eq(cert.matchesHostname("api.cp.dev"), false);

    // IPv4 SAN matching
    expect_eq(cert.matchesHostname("192.168.10.5"), true);
    expect_eq(cert.matchesHostname("192.168.10.6"), false);

    // CN fallback must NOT trigger when SANs exist
    expect_eq(cert.matchesHostname("ignored.cn.cp.dev"), false);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;
    const no_sans: [0][]const u8 = [];

    const ca_der = buildTestCertDer(
        alloc,
        1,
        "CP Root CA",
        "CP Root CA",
        "250101000000Z", false,
        "350101000000Z", false,
        true,
        1,
        &no_sans,
        null,
    )?;
    defer alloc.free(ca_der);

    const leaf_sans: [1][]const u8 = ["node1.cp.dev"];
    const leaf_der = buildTestCertDer(
        alloc,
        2,
        "CP Root CA",
        "node1.cp.dev",
        "260101000000Z", false,
        "270101000000Z", false,
        false,
        null,
        &leaf_sans,
        null,
    )?;
    defer alloc.free(leaf_der);

    let ca_cert = X509Certificate.parseDer(alloc, ca_der)?;
    defer ca_cert.deinit(alloc);

    let leaf_cert = X509Certificate.parseDer(alloc, leaf_der)?;
    defer leaf_cert.deinit(alloc);

    expect_eq(ca_cert.basic_constraints.?.is_ca, true);
    expect_eq(ca_cert.basic_constraints.?.path_len.?, 1 as u32);
    expect_eq(leaf_cert.isIssuedBy(&ca_cert), true);
    expect_eq(ca_cert.isIssuedBy(&leaf_cert), false);

    // Truncated / malformed DER -> InvalidDer
    let got_invalid_der = false;
    * = X509Certificate.parseDer(alloc, leaf_der[0..10]) catch e => err_der: {
        got_invalid_der = (e == X509Error.InvalidDer);
        break :err_der emptyCert();
    };
    expect_eq(got_invalid_der, true);

    // Wrong PEM label -> InvalidPem
    const key_pem = encodePem(alloc, "PRIVATE KEY", leaf_der)?;
    defer alloc.free(key_pem);
    let got_invalid_pem = false;
    * = X509Certificate.parsePem(alloc, key_pem) catch e => err_pem: {
        got_invalid_pem = (e == X509Error.InvalidPem);
        break :err_pem emptyCert();
    };
    expect_eq(got_invalid_pem, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;
    const sans: [1][]const u8 = ["dynamic.cp.dev"];
    const der = buildTestCertDer(
        alloc,
        777,
        "CP Dynamic CA",
        "dynamic.cp.dev",
        "260101000000Z", false,
        "280101000000Z", false,
        false,
        null,
        &sans,
        null,
    )?;
    defer alloc.free(der);

    const pem_text = encodePem(alloc, "CERTIFICATE", der)?;
    defer alloc.free(pem_text);

    // Dynamic mounting via import cap from "cap"; cap.x509
    let cert = cap.x509.X509Certificate.parsePem(alloc, pem_text)?;
    defer cert.deinit(alloc);

    expect_eq(cert.matchesHostname("dynamic.cp.dev"), true);
    const t = cap.x509.parseAsn1Time(TAG_UTC_TIME, "260601120000Z")?;
    expect_eq(cert.isValidAt(t), true);
}

test("cap:x509 parseDer full fields, UTCTime/GeneralizedTime & SHA-256 fingerprint") {
    runTest1() catch unreachable;
}

test("cap:x509 parsePem via cap:pem & isValidAt boundary checks") {
    runTest2() catch unreachable;
}

test("cap:x509 matchesHostname RFC 6125 wildcard & IP SAN matching") {
    runTest3() catch unreachable;
}

test("cap:x509 isIssuedBy CA chain & InvalidDer/InvalidPem error interception") {
    runTest4() catch unreachable;
}

test("cap.x509 dynamic mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
