// Source: cp/cap/test/test_jks_production_deep.cp (Production deep tests for cap:jks)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    JKS_MAGIC,
    JKS_VERSION_1,
    JKS_VERSION_2,
    JKS_TAG_PRIVATE_KEY,
    JKS_TAG_TRUSTED_CERT,
    JKS_DIGEST_SIZE,
    JksHeader,
    JksCert,
    JksEntry,
    JksStats,
    JksCertIter,
    JksEntryIter,
    isJks,
    parseJksHeader,
    findJksEntry,
    summarizeJks,
} from "cap:binfmt";

/// 零堆分配栈上大端序 JKS 构造器（测试专用）
struct JksBuilder {
    buf: [512]u8,
    len: usize,

    static fn init(): JksBuilder {
        return JksBuilder{
            buf: [0; 512],
            len: 0,
        };
    }

    fn putU16Be(*_, v: u16) {
        _.buf[_.len] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.len + 1] = (v & 0xFF) as u8;
        _.len += 2;
    }

    fn putU32Be(*_, v: u32) {
        _.buf[_.len] = ((v >> 24) & 0xFF) as u8;
        _.buf[_.len + 1] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.len + 2] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.len + 3] = (v & 0xFF) as u8;
        _.len += 4;
    }

    fn putI64Be(*_, v: i64) {
        const u = @as(u64, @bitCast(v));
        _.putU32Be(((u >> 32) & 0xFFFFFFFF) as u32);
        _.putU32Be((u & 0xFFFFFFFF) as u32);
    }

    fn putBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.len] = s[i];
            _.len += 1;
            i += 1;
        }
    }

    fn putUtf8(*_, s: []const u8) {
        _.putU16Be(s.len as u16);
        _.putBytes(s);
    }

    fn putDigest(*_, fill: u8) {
        let i: usize = 0;
        while (i < 20) {
            _.buf[_.len] = fill;
            _.len += 1;
            i += 1;
        }
    }

    fn slice(): []const u8 {
        return _.buf[0.._.len];
    }
}

fn buildSampleJksV2(): JksBuilder {
    let b = JksBuilder.init();
    // Header: magic, version=2, entry_count=2
    b.putU32Be(JKS_MAGIC);
    b.putU32Be(JKS_VERSION_2);
    b.putU32Be(2);

    // Entry 1: PrivateKeyEntry ("server-key", 2 certs)
    b.putU32Be(JKS_TAG_PRIVATE_KEY);
    b.putUtf8("server-key");
    b.putI64Be(1700000000000);
    b.putU32Be(6);
    b.putBytes("\x30\x04\x01\x02\x03\x04"); // protected PKCS#8 DER
    b.putU32Be(2); // cert_count = 2
    // Cert 1 (leaf)
    b.putUtf8("X.509");
    b.putU32Be(4);
    b.putBytes("\x30\x82\x01\x0A");
    // Cert 2 (intermediate)
    b.putUtf8("X.509");
    b.putU32Be(5);
    b.putBytes("\x30\x82\x02\x0B\x0C");

    // Entry 2: TrustedCertEntry ("root-ca")
    b.putU32Be(JKS_TAG_TRUSTED_CERT);
    b.putUtf8("root-ca");
    b.putI64Be(1700000001234);
    b.putUtf8("X.509");
    b.putU32Be(3);
    b.putBytes("\x30\x01\xFF");

    // Trailing 20B SHA-1 digest
    b.putDigest(0xAB);
    return b;
}

test("1. JKS v2 full parsing: header, PrivateKeyEntry (2-cert chain), TrustedCertEntry, and 20B digest", () => {
    const b = buildSampleJksV2();
    const data = b.slice();

    expect_eq(isJks(data), true);
    const hdr = parseJksHeader(data) ?? unreachable;
    expect_eq(hdr.version, JKS_VERSION_2);
    expect_eq(hdr.entry_count, 2);
    expect_eq(hdr.entries_offset, 12);
    expect_eq(hdr.digest.len, JKS_DIGEST_SIZE);
    expect_eq(hdr.digest[0], 0xAB);
    expect_eq(hdr.digest[19], 0xAB);
    expect_eq(hdr.signed_body.len, data.len - 20);

    let it = JksEntryIter.init(data);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.isPrivateKey(), true);
    expect_eq(e1.isTrustedCert(), false);
    expect_eq(std.mem.eql(u8, e1.alias, "server-key"), true);
    expect_eq(e1.timestamp_ms, 1700000000000);
    expect_eq(e1.protected_key_der.len, 6);
    expect_eq(e1.cert_count, 2);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.isTrustedCert(), true);
    expect_eq(std.mem.eql(u8, e2.alias, "root-ca"), true);
    expect_eq(e2.timestamp_ms, 1700000001234);
    expect_eq(std.mem.eql(u8, e2.cert_type, "X.509"), true);
    expect_eq(std.mem.eql(u8, e2.cert_der, "\x30\x01\xFF"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.malformed, false);

    const stats = summarizeJks(data) ?? unreachable;
    expect_eq(stats.version, 2);
    expect_eq(stats.entry_count, 2);
    expect_eq(stats.private_key_count, 1);
    expect_eq(stats.trusted_cert_count, 1);
    expect_eq(stats.total_certs, 3);
});

test("2. JKS v1 compatibility without cert_type string defaults to X.509", () => {
    let b = JksBuilder.init();
    b.putU32Be(JKS_MAGIC);
    b.putU32Be(JKS_VERSION_1);
    b.putU32Be(2);

    // v1 PrivateKeyEntry (no cert_type string before der_len)
    b.putU32Be(JKS_TAG_PRIVATE_KEY);
    b.putUtf8("v1-key");
    b.putI64Be(1600000000000);
    b.putU32Be(2);
    b.putBytes("\xAA\xBB");
    b.putU32Be(1);
    b.putU32Be(3);
    b.putBytes("\x11\x22\x33");

    // v1 TrustedCertEntry (no cert_type string)
    b.putU32Be(JKS_TAG_TRUSTED_CERT);
    b.putUtf8("v1-ca");
    b.putI64Be(1600000000500);
    b.putU32Be(2);
    b.putBytes("\x44\x55");

    b.putDigest(0x55);
    const data = b.slice();

    expect_eq(isJks(data), true);
    const stats = summarizeJks(data) ?? unreachable;
    expect_eq(stats.version, 1);
    expect_eq(stats.entry_count, 2);
    expect_eq(stats.total_certs, 2);

    const k = findJksEntry(data, "v1-key") ?? unreachable;
    let cit = k.certs(JKS_VERSION_1);
    const c0 = cit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, c0.cert_type, "X.509"), true);
    expect_eq(std.mem.eql(u8, c0.der, "\x11\x22\x33"), true);
    expect_eq(cit.next() == null, true);

    const ca = findJksEntry(data, "v1-ca") ?? unreachable;
    expect_eq(std.mem.eql(u8, ca.cert_type, "X.509"), true);
    expect_eq(std.mem.eql(u8, ca.cert_der, "\x44\x55"), true);
});

test("3. JksCertIter traverses PrivateKeyEntry multi-certificate chain cleanly", () => {
    const b = buildSampleJksV2();
    const data = b.slice();

    const entry = findJksEntry(data, "server-key") ?? unreachable;
    let cit = entry.certs(JKS_VERSION_2);

    const leaf = cit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, leaf.cert_type, "X.509"), true);
    expect_eq(std.mem.eql(u8, leaf.der, "\x30\x82\x01\x0A"), true);

    const inter = cit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, inter.cert_type, "X.509"), true);
    expect_eq(std.mem.eql(u8, inter.der, "\x30\x82\x02\x0B\x0C"), true);

    expect_eq(cit.next() == null, true);
    expect_eq(cit.malformed, false);
});

test("4. findJksEntry locates existing aliases and returns null for missing aliases", () => {
    const b = buildSampleJksV2();
    const data = b.slice();

    const ca = findJksEntry(data, "root-ca") ?? unreachable;
    expect_eq(ca.isTrustedCert(), true);
    expect_eq(findJksEntry(data, "nonexistent-alias") == null, true);
});

test("5. Resilience against wrong magic, invalid version, unknown tag, truncation, and trailing garbage", () => {
    // 过短缓冲区 (< 32B)
    expect_eq(isJks("\xFE\xED\xFE\xED\x00\x00\x00\x02"), false);
    expect_eq(parseJksHeader("") == null, true);

    // 错魔数
    let bad_magic = buildSampleJksV2();
    bad_magic.buf[0] = 0xDE;
    expect_eq(isJks(bad_magic.slice()), false);
    expect_eq(parseJksHeader(bad_magic.slice()) == null, true);

    // 非法 version = 3
    let bad_ver = buildSampleJksV2();
    bad_ver.buf[7] = 3;
    expect_eq(isJks(bad_ver.slice()), false);
    expect_eq(summarizeJks(bad_ver.slice()) == null, true);

    // 未知 entry tag = 99
    let bad_tag = JksBuilder.init();
    bad_tag.putU32Be(JKS_MAGIC);
    bad_tag.putU32Be(JKS_VERSION_2);
    bad_tag.putU32Be(1);
    bad_tag.putU32Be(99);
    bad_tag.putUtf8("bad");
    bad_tag.putI64Be(0);
    bad_tag.putDigest(0x00);
    expect_eq(summarizeJks(bad_tag.slice()) == null, true);

    // 截断（条目越过尾部 20B 摘要边界）
    const full = buildSampleJksV2();
    const truncated = full.buf[0..full.len - 5];
    expect_eq(summarizeJks(truncated) == null, true);

    // 尾部多出 1 字节垃圾（剩余不为精确 20B 摘要）
    let extra = buildSampleJksV2();
    extra.putBytes("\x99");
    expect_eq(summarizeJks(extra.slice()) == null, true);
});
