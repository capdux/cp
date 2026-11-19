// Source: /data/dev/bun/src/boringssl/boringssl.cp & /data/dev/bun/src/sql/ & /data/dev/bun/src/dns/
// cap/test/test_crypto_sql_dns.cp
// Comprehensive Full E2E Regression Suite for cap:crypto, cap:sql, and cap:dns

import { test, expect_eq } from "cap:test";
import {
    HashAlgorithm,
    Hash,
    sha256,
    sha256Raw,
    sha512,
    sha512Raw,
    sha1,
    md5,
    createHash,
    HmacAlgorithm,
    Hmac,
    hmacSha256,
    hmacSha256Raw,
    hmacSha512,
    createHmac,
    pbkdf2Into,
    pbkdf2Sync,
    pbkdf2Raw,
    Aes256Gcm,
    ChaCha20Poly1305,
    fillRandom,
    randomBytes,
    randomUUID,
    randomUUIDInto,
    randomInt,
    isSafeAltName,
    matchDnsName,
    TlsHostnameVerifier,
} from "cap:crypto";
import {
    SqlDialect,
    SqlType,
    SqlParam,
    SqlStatement,
    PostgresWire,
    PostgresAuthKind,
    PostgresAuthChallenge,
    PostgresDataRow,
    PostgresCommandComplete,
    MySqlPacketHeader,
    MySqlLenEnc,
    MySqlCapabilities,
    MySqlOkPacket,
    MySqlErrPacket,
    SqliteDatabase,
    SqliteRows,
} from "cap:sql";
import {
    ARES_SUCCESS,
    ARES_ETIMEOUT,
    ARES_ENOTFOUND,
    NSClass,
    NSType,
    CaresStatus,
    DnsRCode,
    DnsHeader,
    DnsWireCodec,
    DnsFamily,
    DnsCache,
    HostsTable,
} from "cap:dns";
const std = @import("std");

fn testCryptoHashHmacPbkdf2(): !void {
    // 1. One-shot & raw SHA-256 / SHA-512 / SHA-1 / MD5
    const h256 = sha256("hello world");
    expect_eq(h256, "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");

    let raw256: [32]u8 = undefined;
    sha256Raw(&raw256, "");
    expect_eq(raw256[0], 0xe3 as u8);
    expect_eq(raw256[31], 0x55 as u8);

    const h512 = sha512("hello world");
    expect_eq(h512.len, 128 as usize);
    expect_eq(sha1("hello world"), "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed");
    expect_eq(md5("hello world"), "5eb63bbbe01eeed093cb22bb8f5acdc3");

    // 2. Streaming Hash & HMAC
    let sh = createHash("sha256").?;
    sh.update("hello ");
    sh.update("world");
    expect_eq(sh.digest(), h256);

    const mac_oneshot = hmacSha256("signing_key", "payload_data");
    let smac = createHmac("sha256", "signing_key").?;
    smac.update("payload_");
    smac.update("data");
    expect_eq(smac.digest(), mac_oneshot);

    const mac512 = hmacSha512("signing_key", "payload_data");
    expect_eq(mac512.len, 128 as usize);

    // 3. PBKDF2-HMAC-SHA256 (allocating & zero-allocation pbkdf2Into)
    const k_hex = pbkdf2Sync("password", "salt", 1000, 32);
    expect_eq(k_hex, "632c2812e46d4604102ba7618e9d6d7d2f8128f6266b4a03264d2a0460b7dcb3");

    let k_stack: [32]u8 = undefined;
    expect_eq(pbkdf2Into(&k_stack, "password", "salt", 1000), true);
    expect_eq(k_stack[0], 0x63 as u8);
    expect_eq(k_stack[31], 0xb3 as u8);
}

fn testCryptoAeadUuidAndTlsVerifier(): !void {
    // 1. AES-256-GCM & ChaCha20-Poly1305 encryption, authentication & tamper rejection
    let key: [32]u8 = [0x42; 32];
    let nonce: [12]u8 = [0x24; 12];
    const msg = "Zero-Copy Agent Secret Message!";
    const aad = "channel:secure-01";
    let ct: [31]u8 = undefined;
    let tag: [16]u8 = undefined;
    let pt: [31]u8 = undefined;

    Aes256Gcm.encrypt(&key, &nonce, msg, aad, &ct, &tag);
    expect_eq(Aes256Gcm.decrypt(&key, &nonce, &ct, tag, aad, &pt), true);
    expect_eq(pt[0..31], msg);
    // Tampered AAD must fail authentication
    expect_eq(Aes256Gcm.decrypt(&key, &nonce, &ct, tag, "channel:tampered", &pt), false);

    ChaCha20Poly1305.encrypt(&key, &nonce, msg, aad, &ct, &tag);
    expect_eq(ChaCha20Poly1305.decrypt(&key, &nonce, &ct, tag, aad, &pt), true);
    expect_eq(pt[0..31], msg);
    // Tampered tag must fail authentication
    let bad_tag = tag;
    bad_tag[0] ^= 0xFF;
    expect_eq(ChaCha20Poly1305.decrypt(&key, &nonce, &ct, bad_tag, aad, &pt), false);

    // 2. CSPRNG & RFC 4122 UUIDv4
    let uuid_stack: [36]u8 = undefined;
    randomUUIDInto(&uuid_stack);
    expect_eq(uuid_stack[8], '-' as u8);
    expect_eq(uuid_stack[13], '-' as u8);
    expect_eq(uuid_stack[14], '4' as u8);
    expect_eq(uuid_stack[18], '-' as u8);
    expect_eq(uuid_stack[23], '-' as u8);

    const r_int = randomInt(100, 200);
    expect_eq(r_int >= 100 && r_int < 200, true);

    // 3. RFC 6125 TlsHostnameVerifier wildcard & SAN safety verification
    expect_eq(TlsHostnameVerifier.isSafeAltName("api.cp-lang.org", false), true);
    expect_eq(TlsHostnameVerifier.isSafeAltName("bad host.org", false), false);
    expect_eq(TlsHostnameVerifier.verify("*.cp-lang.org", "registry.cp-lang.org"), true);
    expect_eq(TlsHostnameVerifier.verify("*.cp-lang.org", "CP-LANG.ORG"), false);
    expect_eq(TlsHostnameVerifier.verify("*.cp-lang.org", "deep.sub.cp-lang.org"), false);
    expect_eq(TlsHostnameVerifier.verify("*.org", "cp-lang.org"), false);
}

fn testSqlPostgresAndMySqlWire(): !void {
    // 1. Parameterized SqlStatement bindings
    let stmt = SqlStatement.prepare("SELECT * FROM users WHERE id = $1 AND name = $2 AND active = $3 AND deleted_at IS $4", SqlDialect.Postgres);
    expect_eq(stmt.bindInt(77), true);
    expect_eq(stmt.bindText("alice"), true);
    expect_eq(stmt.bindBool(true), true);
    expect_eq(stmt.bindNull(), true);
    expect_eq(stmt.param_count, 4 as usize);
    if (p0 := stmt.getParam(0)) {
        expect_eq(p0.kind == SqlType.Int64, true);
        expect_eq(p0.int_val, 77 as i64);
    }

    // 2. PostgreSQL v3 StartupMessage, PasswordMessage, SimpleQuery, Auth, DataRow & CommandComplete
    let wire_buf: [128]u8 = undefined;
    const startup_len = PostgresWire.encodeStartupMessage(&wire_buf, "postgres", "cp_db");
    // 4 (len) + 4 (ver) + 5 ("user\0") + 9 ("postgres\0") + 9 ("database\0") + 6 ("cp_db\0") + 1 ("\0") = 38
    expect_eq(startup_len, 38 as usize);
    expect_eq(wire_buf[5], 3 as u8); // v3.0

    const pw_len = PostgresWire.encodePasswordMessage(&wire_buf, "secret");
    expect_eq(wire_buf[0], 'p' as u8);
    expect_eq(pw_len, 12 as usize);

    const q_len = PostgresWire.encodeSimpleQuery(&wire_buf, "SELECT 1;");
    expect_eq(wire_buf[0], 'Q' as u8);
    expect_eq(PostgresWire.parseFrameLength(wire_buf[0..q_len]).?, 14 as u32);

    const md5_auth = PostgresAuthChallenge.parse("\x00\x00\x00\x05\x11\x22\x33\x44").?;
    expect_eq(md5_auth.kind == PostgresAuthKind.MD5Password, true);
    expect_eq(md5_auth.md5_salt[0], 0x11 as u8);
    expect_eq(md5_auth.md5_salt[3], 0x44 as u8);

    let row = PostgresDataRow.init("\x00\x02\x00\x00\x00\x03abc\xFF\xFF\xFF\xFF").?;
    expect_eq(row.column_count, 2 as u16);
    const c1 = row.nextColumn().?;
    expect_eq(c1.is_null, false);
    expect_eq(c1.data, "abc");
    const c2 = row.nextColumn().?;
    expect_eq(c2.is_null, true);

    const cc = PostgresCommandComplete.parse("INSERT 0 19\x00");
    expect_eq(cc.rows_affected, 19 as u64);

    // 3. MySQL 4.1 PacketHeader, LenEnc, Capabilities, OK_Packet & ERR_Packet
    let m_hdr: [4]u8 = undefined;
    expect_eq(MySqlPacketHeader.encode(&m_hdr, 258, 7), 4 as usize);
    const dec_m = MySqlPacketHeader.decode(&m_hdr).?;
    expect_eq(dec_m.payload_length, 258 as u32);
    expect_eq(dec_m.sequence_id, 7 as u8);

    let l_buf: [16]u8 = undefined;
    expect_eq(MySqlLenEnc.encode(&l_buf, 70000), 4 as usize);
    expect_eq(MySqlLenEnc.decode(l_buf[0..4]).?.value, 70000 as u64);

    const ok_pkt = MySqlOkPacket.parse("\x00\x05\x2A\x02\x00\x00\x00").?;
    expect_eq(ok_pkt.affected_rows, 5 as u64);
    expect_eq(ok_pkt.last_insert_id, 42 as u64);

    const err_pkt = MySqlErrPacket.parse("\xFF\x15\x04#28000Access denied").?;
    expect_eq(err_pkt.error_code, 1045 as u16);
    expect_eq(err_pkt.sql_state, "28000");
    expect_eq(err_pkt.message, "Access denied");
}

fn testSqliteInMemoryDatabase(): !void {
    let db = SqliteDatabase.open(":memory:")?;
    defer db.close();

    db.exec("CREATE TABLE kv_store (k TEXT PRIMARY KEY, v INTEGER);")?;
    db.exec("INSERT INTO kv_store (k, v) VALUES ('alpha', 100), ('beta', 250);")?;

    let rows = db.query("SELECT k, v FROM kv_store ORDER BY v ASC;")?;
    defer rows.close();

    expect_eq(rows.next(), true);
    expect_eq(rows.columnText(0), "alpha");
    expect_eq(rows.columnInt(1), 100 as i64);

    expect_eq(rows.next(), true);
    expect_eq(rows.columnText(0), "beta");
    expect_eq(rows.columnInt(1), 250 as i64);

    expect_eq(rows.next(), false);
}

fn testDnsPacketHostsAndCache(): !void {
    // 1. RFC 1035 DNS Header, QNAME Codec & 0xC0 Pointer Decompression
    let pkt: [64]u8 = [0; 64];
    const q_len = DnsWireCodec.buildQueryPacket(0xABCD, "dns.cp-lang.org", NSType.AAAA, &pkt);
    expect_eq(q_len, 33 as usize);

    const hdr = DnsHeader.decode(pkt[0..12]).?;
    expect_eq(hdr.id, 0xABCD as u16);
    expect_eq(hdr.recursion_desired, true);
    expect_eq(hdr.qd_count, 1 as u16);

    let out_dom: [64]u8 = undefined;
    const dec_dom = DnsWireCodec.decodeDomainName(pkt[0..q_len], 12, &out_dom).?;
    expect_eq(out_dom[0..dec_dom.name_len], "dns.cp-lang.org");

    // 2. HostsTable (/etc/hosts) parsing & IPv4 lookup
    const hosts_content =
        "# Cluster static hosts\n" +
        "127.0.0.1\tlocalhost loopback\n" +
        "192.168.10.25  api.internal gateway.internal # internal VIP\n" +
        "::1            ip6-localhost\n";
    let hosts = HostsTable.parse(hosts_content);
    expect_eq(hosts.entryCount(), 5 as usize);
    expect_eq(hosts.lookup("localhost").?, "127.0.0.1");
    expect_eq(hosts.lookup("gateway.internal").?, "192.168.10.25");
    expect_eq(hosts.lookupIPv4("api.internal").?, 0xC0A80A19 as u32);
    expect_eq(hosts.lookup("ip6-localhost").?, "::1");
    expect_eq(hosts.lookupIPv4("ip6-localhost") == null, true);

    // 3. DnsCache<16> TTL hit/miss & eviction
    let cache = DnsCache<16>.init();
    expect_eq(cache.put("api.internal", 0xC0A80A19, 8080, 1000, 5000), true);
    expect_eq(cache.lookup("api.internal", 8080, 3000).?, 0xC0A80A19 as u32);
    expect_eq(cache.lookup("api.internal", 8080, 6500) == null, true);
    expect_eq(cache.activeCount(), 0 as usize);
    expect_eq(CaresStatus.codeName(ARES_SUCCESS), "SUCCESS");
    expect_eq(CaresStatus.isRetryable(ARES_ETIMEOUT), true);
    expect_eq(CaresStatus.isRetryable(ARES_ENOTFOUND), false);
}

fn main() {
    test("1. cap:crypto — SHA-256/512/1/MD5, streaming Hash/HMAC & PBKDF2 key derivation", () => {
        try testCryptoHashHmacPbkdf2();
    });
    test("2. cap:crypto — AES-256-GCM, ChaCha20-Poly1305 AEAD, UUIDv4 & RFC 6125 TlsHostnameVerifier", () => {
        try testCryptoAeadUuidAndTlsVerifier();
    });
    test("3. cap:sql — Parameterized SqlStatement, Postgres v3 Startup/Auth/DataRow & MySQL 4.1 Wire", () => {
        try testSqlPostgresAndMySqlWire();
    });
    test("4. cap:sql — Embedded SQLite3 (:memory:) DDL/DML execution & zero-copy row cursor", () => {
        try testSqliteInMemoryDatabase();
    });
    test("5. cap:dns — RFC 1035 DNS Header/Codec, /etc/hosts HostsTable parser & DnsCache<CAP>", () => {
        try testDnsPacketHostsAndCache();
    });
}
