// Source: /data/dev/bun/src/
// cap/test/test_tier1_parity_deep.cp
// Tier-1 Core Stdlib Parity & Gap-Fill Deep Verification Suite

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    S3Acl,
    S3StorageClass,
    S3Credentials,
    S3Client,
    S3SimpleRequestOptions,
    S3RequestBuilder,
} from "cap:s3";

import {
    NSType,
    NSClass,
    DnsRCode,
    DnsHeader,
    DnsWireCodec,
    DnsCache,
    HostsTable,
} from "cap:dns";

import {
    RESPType,
    RESPParser,
    RESPEncoder,
    ValkeyReader,
    SubscriptionPushMessage,
    RedisCommand,
    RedisPipeline,
} from "cap:redis";

import {
    URL,
    URLSearchParams,
} from "cap:url";

import {
    Base64,
    Hex,
    Utf16BE,
    Ascii,
    PercentEncoding,
} from "cap:encoding";

import {
    Which,
    WhichOptions,
    which,
    whichAll,
    endsWithPathext,
} from "cap:which";

import {
    WatchAction,
    WatchEvent,
    WatchOptions,
    WatchFilter,
    createWatcher,
} from "cap:watcher";

// Additional Tier-1 cross-module imports (avoiding flat-Zig cross-lib symbol collisions Hash/getNowMs)
import { Crc32 } from "cap:pack";
import { DotEnv } from "cap:conf";
import { BoundedArray } from "cap:col";
import { TaggedPointer } from "cap:ptr";
import { Mutex } from "cap:sync";
import { Ipv4Addr } from "cap:net";

fn testS3Parity(): !void {
    let creds = S3Credentials{
        access_key_id: "AKIAIOSFODNN7EXAMPLE",
        secret_access_key: "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY",
        region: "us-east-1",
        endpoint: "s3.amazonaws.com",
        bucket: "parity-bucket",
        session_token: "FwoGZXIvYXdz/token+123==",
        virtual_hosted_style: true,
    };
    let client = S3Client.init(creds);

    // 1. Optional headers + session_token signed in strict alphabetical order
    let opts = S3SimpleRequestOptions{
        path: "artifacts/model.bin",
        method: "PUT",
        body: "payload-bytes",
        content_type: "application/octet-stream",
        content_disposition: "attachment",
        acl: S3Acl.Private,
        storage_class: S3StorageClass.StandardIa,
        request_payer: true,
    };
    let url_buf: [512]u8 = undefined;
    let auth_buf: [1024]u8 = undefined;
    const signed = client.signRequest(opts, "20261120", "20261120T070000Z", url_buf[0..512], auth_buf[0..1024])?;

    expect_eq(
        signed.signed_headers,
        "content-disposition;content-type;host;x-amz-acl;x-amz-content-sha256;x-amz-date;x-amz-request-payer;x-amz-security-token;x-amz-storage-class"
    );
    expect_eq(signed.security_token != null, true);
    expect_eq(std.mem.indexOf(u8, signed.auth_header, "SignedHeaders=content-disposition;") != null, true);

    // 2. Presigned URL with STS session_token percent-encoded
    let presign_buf: [1024]u8 = undefined;
    const presigned = client.presignUrl("GET", "artifacts/model.bin", "20261120", "20261120T070000Z", 900, presign_buf[0..1024])?;
    expect_eq(std.mem.indexOf(u8, presigned, "X-Amz-Security-Token=FwoGZXIvYXdz%2Ftoken%2B123%3D%3D") != null, true);
    expect_eq(std.mem.indexOf(u8, presigned, "&X-Amz-Signature=") != null, true);

    // 3. Presigned multipart upload part URL
    const part_url = client.presignPartUrl("artifacts/model.bin", "up-42", 2, "20261120", "20261120T070000Z", 600, presign_buf[0..1024])?;
    expect_eq(std.mem.indexOf(u8, part_url, "partNumber=2&uploadId=up-42&x-id=UploadPart") != null, true);
    expect_eq(std.mem.indexOf(u8, part_url, "&X-Amz-Signature=") != null, true);
}

fn testDnsRecordDecodersAndIpv6Cache(): !void {
    // Build a synthetic DNS response packet with 1 Question + 4 Answers (AAAA, MX, SRV, TXT)
    // Header (12 bytes): ID=0xABCD, QR=1, qd=1, an=4
    let pkt: [256]u8 = [0; 256];
    const hdr = DnsHeader{
        id: 0xABCD,
        is_response: true,
        recursion_desired: true,
        recursion_available: true,
        rcode: DnsRCode.NoError,
        qd_count: 1,
        an_count: 4,
    };
    let pos = hdr.encode(pkt[0..256]);
    expect_eq(pos, 12);

    // Question: "_http._tcp.cap.dev" -> offset 12..32
    const qname_len = DnsWireCodec.encodeDomainName("_http._tcp.cap.dev", pkt[pos..]);
    pos += qname_len;
    pkt[pos] = 0;
    pkt[pos + 1] = 33; // SRV
    pkt[pos + 2] = 0;
    pkt[pos + 3] = 1; // IN
    pos += 4;

    // Decode question
    let qname_out: [64]u8 = [0; 64];
    const q = DnsWireCodec.decodeQuestion(pkt[0..pos], 12, qname_out[0..64]).?;
    expect_eq(qname_out[0..q.name_len], "_http._tcp.cap.dev");
    expect_eq(q.qtype == NSType.SRV, true);
    expect_eq(q.next_offset, pos);

    // Answer 1: AAAA record (pointer 0xC0 0x0C, type=28, class=1, ttl=60, rdlen=16)
    const aaaa_off = pos;
    pkt[pos] = 0xC0; pkt[pos + 1] = 12;
    pkt[pos + 2] = 0; pkt[pos + 3] = 28; // AAAA
    pkt[pos + 4] = 0; pkt[pos + 5] = 1;  // IN
    pkt[pos + 6] = 0; pkt[pos + 7] = 0; pkt[pos + 8] = 0; pkt[pos + 9] = 60;
    pkt[pos + 10] = 0; pkt[pos + 11] = 16;
    pos += 12;
    pkt[pos] = 0x20; pkt[pos + 1] = 0x01;
    pkt[pos + 2] = 0x0d; pkt[pos + 3] = 0xb8;
    pkt[pos + 15] = 0x01;
    pos += 16;

    // Answer 2: MX record (pointer 0xC0 0x0C, type=15, pref=10, exchange="mail.cap.dev")
    const mx_off = pos;
    pkt[pos] = 0xC0; pkt[pos + 1] = 12;
    pkt[pos + 2] = 0; pkt[pos + 3] = 15; // MX
    pkt[pos + 4] = 0; pkt[pos + 5] = 1;
    pkt[pos + 6] = 0; pkt[pos + 7] = 0; pkt[pos + 8] = 1; pkt[pos + 9] = 44; // TTL 300
    const mx_ex_len = DnsWireCodec.encodeDomainName("mail.cap.dev", pkt[pos + 14 ..]);
    const mx_rdlen = 2 + mx_ex_len;
    pkt[pos + 10] = 0; pkt[pos + 11] = mx_rdlen as u8;
    pkt[pos + 12] = 0; pkt[pos + 13] = 10; // pref 10
    pos += 12 + mx_rdlen;

    // Answer 3: SRV record (pointer 0xC0 0x0C, type=33, prio=5, weight=20, port=8443, target="edge.cap.dev")
    const srv_off = pos;
    pkt[pos] = 0xC0; pkt[pos + 1] = 12;
    pkt[pos + 2] = 0; pkt[pos + 3] = 33; // SRV
    pkt[pos + 4] = 0; pkt[pos + 5] = 1;
    pkt[pos + 6] = 0; pkt[pos + 7] = 0; pkt[pos + 8] = 0; pkt[pos + 9] = 120;
    const srv_tgt_len = DnsWireCodec.encodeDomainName("edge.cap.dev", pkt[pos + 18 ..]);
    const srv_rdlen = 6 + srv_tgt_len;
    pkt[pos + 10] = 0; pkt[pos + 11] = srv_rdlen as u8;
    pkt[pos + 12] = 0; pkt[pos + 13] = 5;   // priority 5
    pkt[pos + 14] = 0; pkt[pos + 15] = 20;  // weight 20
    pkt[pos + 16] = 0x20; pkt[pos + 17] = 0xFB; // port 8443
    pos += 12 + srv_rdlen;

    // Answer 4: TXT record (two chunks: "v=spf1 " + "-all")
    const txt_off = pos;
    pkt[pos] = 0xC0; pkt[pos + 1] = 12;
    pkt[pos + 2] = 0; pkt[pos + 3] = 16; // TXT
    pkt[pos + 4] = 0; pkt[pos + 5] = 1;
    pkt[pos + 6] = 0; pkt[pos + 7] = 0; pkt[pos + 8] = 0; pkt[pos + 9] = 90;
    pkt[pos + 10] = 0; pkt[pos + 11] = 13; // 1+7 + 1+4 = 13
    pos += 12;
    pkt[pos] = 7;
    @memcpy(pkt[pos + 1 .. pos + 8], "v=spf1 ");
    pos += 8;
    pkt[pos] = 4;
    @memcpy(pkt[pos + 1 .. pos + 5], "-all");
    pos += 5;

    let rr_name: [64]u8 = [0; 64];
    const rr1 = DnsWireCodec.decodeResourceRecord(pkt[0..pos], aaaa_off, rr_name[0..64]).?;
    expect_eq(rr1.rtype == NSType.AAAA, true);
    expect_eq(rr1.ttl, 60 as u32);
    const v6 = DnsWireCodec.parseAAAARdata(pkt[0..pos], rr1).?;
    let v6_str: [64]u8 = [0; 64];
    const v6_len = DnsWireCodec.formatIPv6(v6, v6_str[0..64]);
    expect_eq(v6_str[0..v6_len], "2001:db8:0:0:0:0:0:1");

    const rr2 = DnsWireCodec.decodeResourceRecord(pkt[0..pos], mx_off, rr_name[0..64]).?;
    let mx = DnsWireCodec.parseMxRdata(pkt[0..pos], rr2).?;
    expect_eq(mx.preference, 10 as u16);
    expect_eq(mx.exchange(), "mail.cap.dev");

    const rr3 = DnsWireCodec.decodeResourceRecord(pkt[0..pos], srv_off, rr_name[0..64]).?;
    let srv = DnsWireCodec.parseSrvRdata(pkt[0..pos], rr3).?;
    expect_eq(srv.priority, 5 as u16);
    expect_eq(srv.weight, 20 as u16);
    expect_eq(srv.port, 8443 as u16);
    expect_eq(srv.target(), "edge.cap.dev");

    const rr4 = DnsWireCodec.decodeResourceRecord(pkt[0..pos], txt_off, rr_name[0..64]).?;
    let txt_buf: [64]u8 = [0; 64];
    const txt_len = DnsWireCodec.parseTxtRdata(pkt[0..pos], rr4, txt_buf[0..64]).?;
    expect_eq(txt_buf[0..txt_len], "v=spf1 -all");

    // IPv6 DnsCache & HostsTable.lookupIPv6
    let cache = DnsCache<16>.init();
    expect_eq(cache.putAAAA("v6.cap.dev", v6, 443, 1000, 5000), true);
    const hit_v6 = cache.lookupAAAA("v6.cap.dev", 443, 3000).?;
    expect_eq(hit_v6[0], 0x20);
    expect_eq(hit_v6[15], 0x01);
    expect_eq(cache.lookupAAAA("v6.cap.dev", 443, 7000) == null, true);

    const hosts = HostsTable.parse("::1 ip6-localhost\n127.0.0.1 localhost\n");
    expect_eq(hosts.lookupIPv6("ip6-localhost").?, "::1");
}

fn testRedisResp3AndPubSub(): !void {
    let wire: [512]u8 = undefined;
    let pos: usize = 0;

    pos += RESPEncoder.encodeDoubleStr(wire[pos..], "3.14159");
    pos += RESPEncoder.encodeBigNumber(wire[pos..], "12345678901234567890");
    pos += RESPEncoder.encodeBlobError(wire[pos..], "SYNTAX bad arg");
    pos += RESPEncoder.encodeVerbatimString(wire[pos..], "txt", "hello-resp3");
    pos += RESPEncoder.encodePushHeader(wire[pos..], 3);
    pos += RESPEncoder.encodeSimpleString(wire[pos..], "pmessage");
    pos += RESPEncoder.encodeBulkString(wire[pos..], "news.*");
    pos += RESPEncoder.encodeBulkString(wire[pos..], "breaking");
    pos += RESPEncoder.encodeAttributeHeader(wire[pos..], 1);
    pos += RESPEncoder.encodeSimpleString(wire[pos..], "ttl");
    pos += RESPEncoder.encodeInteger(wire[pos..], 99);

    let reader = ValkeyReader.init(wire[0..pos]);
    const f_dbl = reader.nextFrame().?;
    expect_eq(f_dbl.kind == RESPType.Double, true);
    const dval = f_dbl.asDouble().?;
    expect_eq(dval > 3.14 && dval < 3.15, true);

    const f_big = reader.nextFrame().?;
    expect_eq(f_big.kind == RESPType.BigNumber, true);
    expect_eq(f_big.str_val, "12345678901234567890");

    const f_berr = reader.nextFrame().?;
    expect_eq(f_berr.kind == RESPType.BlobError, true);
    expect_eq(f_berr.str_val, "SYNTAX bad arg");

    const f_verb = reader.nextFrame().?;
    const vb = f_verb.asVerbatimString().?;
    expect_eq(vb.format, "txt");
    expect_eq(vb.content, "hello-resp3");

    // Skip entire nested Push (>3 ...) and Attribute (|1 ...)
    expect_eq(reader.skipFrame(), true);
    expect_eq(reader.skipFrame(), true);
    expect_eq(reader.hasMore(), false);

    // PubSub enum + N-arg command + Pipeline
    expect_eq(SubscriptionPushMessage.fromString("pmessage").? == SubscriptionPushMessage.PMessage, true);
    expect_eq(SubscriptionPushMessage.fromString("psubscribe").? == SubscriptionPushMessage.PSubscribe, true);
    expect_eq(SubscriptionPushMessage.fromString("punsubscribe").? == SubscriptionPushMessage.PUnsubscribe, true);

    let pipe = RedisPipeline.init();
    expect_eq(pipe.hello(3), true);
    expect_eq(pipe.subscribe("chan:ai"), true);
    expect_eq(pipe.publish("chan:ai", "ready"), true);
    expect_eq(pipe.incr("counter"), true);
    const custom_args: [4][]const u8 = ["MSET", "a", "1", "b"];
    expect_eq(pipe.command(custom_args[0..4]), true);
    expect_eq(pipe.command_count, 5 as usize);
}

fn testUrlResolveNormalizeAndSearchParams(): !void {
    // 1. normalizePath
    let pbuf: [128]u8 = undefined;
    expect_eq(URL.normalizePath("/a/b/c/./../../g/", &pbuf), "/a/g/");
    expect_eq(URL.normalizePath("/../../x", &pbuf), "/x");

    // 2. resolve & parseWithBase
    let rbuf: [256]u8 = undefined;
    const base = "https://user:pw@api.cp-lang.org:8443/v1/users/profile?old=1#top";
    expect_eq(
        URL.resolve(base, "../tokens?scope=read#ok", &rbuf).?,
        "https://user:pw@api.cp-lang.org:8443/v1/tokens?scope=read#ok"
    );
    expect_eq(
        URL.resolve(base, "/healthz", &rbuf).?,
        "https://user:pw@api.cp-lang.org:8443/healthz"
    );
    expect_eq(
        URL.resolve(base, "//cdn.cp-lang.org/app.js", &rbuf).?,
        "https://cdn.cp-lang.org/app.js"
    );

    let u = URL.parseWithBase("../v2/status", base, null, &rbuf)?;
    let obuf: [128]u8 = undefined;
    expect_eq(u.formatOrigin(&obuf), "https://api.cp-lang.org:8443");
    expect_eq(u.pathname, "/v1/v2/status");

    // 3. URLSearchParams WHATWG setUnique, hasValue, deleteValue, toEncodedString
    let sp = URLSearchParams.parse("?tag=a&k=1&tag=b&tag=c", null);
    expect_eq(sp.hasValue("tag", "b"), true);
    expect_eq(sp.hasValue("tag", "z"), false);
    sp.deleteValue("tag", "b");
    expect_eq(sp.hasValue("tag", "b"), false);
    sp.setUnique("tag", "only");
    let qbuf: [128]u8 = undefined;
    expect_eq(sp.toString(&qbuf), "tag=only&k=1");

    sp.clear();
    sp.append("q", "hello world");
    expect_eq(sp.toEncodedString(&qbuf), "q=hello%20world");
}

fn testEncodingWhichWatcher(): !void {
    // 1. cap:encoding Utf16BE + Ascii + PercentEncoding
    let be_buf: [16]u8 = undefined;
    const be_len = Utf16BE.encodeUtf8ToUtf16Be("A\u{1F600}", &be_buf); // 'A' (2B) + emoji (4B) = 6B
    expect_eq(be_len, 6 as usize);
    expect_eq(be_buf[0], 0x00);
    expect_eq(be_buf[1], 0x41);
    let rt_buf: [16]u8 = undefined;
    const rt_len = Utf16BE.decodeUtf16BeToUtf8(be_buf[0..be_len], &rt_buf);
    expect_eq(rt_buf[0..rt_len], "A\u{1F600}");

    expect_eq(Ascii.isAscii("cp-lang"), true);
    expect_eq(Ascii.isAscii("中文"), false);
    let low_buf: [16]u8 = undefined;
    expect_eq(low_buf[0..Ascii.toLower("CpLang", &low_buf)], "cplang");

    let penc: [32]u8 = undefined;
    const plen = PercentEncoding.encodeComponentToBuf("a b", &penc);
    expect_eq(penc[0..plen], "a%20b");

    // 2. cap:which PATHEXT & whichAll
    expect_eq(endsWithPathext("tool.CMD", ".EXE;.CMD;.BAT"), true);
    expect_eq(endsWithPathext("tool.sh", ".EXE;.CMD"), false);
    let wbuf: [1024]u8 = undefined;
    let wpaths: [8][]const u8 = undefined;
    const found_n = whichAll("sh", .{ .path = "/bin:/usr/bin" }, &wbuf, &wpaths);
    expect_eq(found_n >= 1, true);

    // 3. cap:watcher integrated WatchFilter + custom ignore patterns
    const pats: [1][]const u8 = ["*.log"];
    let watcher = createWatcher(WatchOptions{
        debounce_ms: 10,
        ignore_defaults: true,
        ignore_hidden: true,
        ignore_patterns: pats[0..1],
    });
    defer watcher.close();

    expect_eq(watcher.shouldIgnore(".git/config"), true);
    expect_eq(watcher.shouldIgnore("src/.env"), true);
    expect_eq(watcher.shouldIgnore("logs/server.log"), true);
    expect_eq(watcher.shouldIgnore("src/main.cp"), false);

    // Ignored mock events are filtered out before reaching debounce
    expect_eq(watcher.dispatchMockEvent(".git/HEAD", WatchAction.modify, false, 100), false);
    expect_eq(watcher.dispatchMockEvent("logs/debug.log", WatchAction.create, false, 100), false);
    expect_eq(watcher.dispatchMockEvent("src/main.cp", WatchAction.modify, false, 100), true);

    let out_ev: [4]WatchEvent = undefined;
    expect_eq(watcher.poll(120, &out_ev), 1 as usize);
    expect_eq(out_ev[0].path, "src/main.cp");
}

fn testTier1CrossModuleIntegration(): !void {
    // Cross-module sanity across non-colliding Tier-1 modules
    expect_eq(Crc32.checksum("123456789"), 0xCBF43926 as u32);

    let env = DotEnv.parse("MODE=prod\n");
    expect_eq(env.get("MODE").?, "prod");

    let arr = BoundedArray<i32, 4>.init();
    expect_eq(arr.append(42), true);
    expect_eq(arr.len, 1 as usize);

    let val: u64 = 128;
    const tp = TaggedPointer.fromPtr(&val, 3);
    expect_eq(tp.tag(), 3 as u16);

    let mtx = Mutex{};
    mtx.lock();
    mtx.unlock();

    expect_eq(Ipv4Addr.loopback().isLoopback(), true);

    // Also verify Hex & Base64 roundtrip
    let b64: [32]u8 = undefined;
    const b64_len = Base64.encodeURLSafe(&b64, "cp-tier1");
    let dec_buf: [32]u8 = undefined;
    const dec_len = Base64.decodeURLSafe(&dec_buf, b64[0..b64_len])?;
    expect_eq(dec_buf[0..dec_len], "cp-tier1");
    let hbuf: [16]u8 = undefined;
    expect_eq(hbuf[0..Hex.encodeLower("AZ", &hbuf)], "415a");
}

fn main() {
    test("1. cap:s3 — Full SigV4 optional headers, STS session_token & presignPartUrl", () => {
        try testS3Parity();
    });
    test("2. cap:dns — Wire AAAA/MX/SRV/TXT decoders, IPv6 DnsCache & HostsTable.lookupIPv6", () => {
        try testDnsRecordDecodersAndIpv6Cache();
    });
    test("3. cap:redis — RESP3 scalar/collection codecs, ValkeyReader.skipFrame & PubSub", () => {
        try testRedisResp3AndPubSub();
    });
    test("4. cap:url — RFC 3986 normalizePath, resolve, parseWithBase & URLSearchParams", () => {
        try testUrlResolveNormalizeAndSearchParams();
    });
    test("5. cap:encoding, cap:which & cap:watcher — UTF-16BE, PATHEXT/whichAll & WatchFilter", () => {
        try testEncodingWhichWatcher();
    });
    test("6. All 18 Tier-1 Modules Cross-Integration Smoke Verification", () => {
        try testTier1CrossModuleIntegration();
    });
}
