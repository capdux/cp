// Source: /data/dev/bun/src/s3_signing/ & /data/dev/bun/src/csrf/csrf.cp & /data/dev/bun/src/valkey/valkey_protocol.cp
// cap/test/test_s3_security_redis.cp
// Comprehensive Full E2E Regression Suite for cap:s3, cap:security, and cap:redis

import { test, expect_eq } from "cap:test";
import {
    S3Acl,
    S3StorageClass,
    S3SignErrorCode,
    S3Error,
    S3Credentials,
    SigV4Signer,
    S3Client,
    S3SimpleRequestOptions,
    S3RequestBuilder,
    S3ListObjectsOptions,
    formatListObjectsQuery,
    parseS3ListObjectsResult,
    UploadPartResult,
    formatCompleteMultipartUploadXML,
    parseInitiateMultipartUploadResult,
    parseCompleteMultipartUploadResult,
    formatUploadPartQuery,
    formatCompleteMultipartQuery,
    formatAbortMultipartQuery,
    formatRangeHeader,
    parseContentRange,
    calculateChunkCount,
} from "cap:s3";
import {
    CSRF,
    TokenFormat,
    DEFAULT_EXPIRATION_MS,
    constantTimeEql,
} from "cap:security";
import {
    RESPType,
    RESPFrame,
    RESPParser,
    ValkeyReader,
    RESPEncoder,
    RedisCommand,
    RedisPipeline,
    VerbatimString,
    SubscriptionPushMessage,
} from "cap:redis";
const std = @import("std");

fn testS3SigV4AndPresign(): !void {
    let creds = S3Credentials{
        access_key_id: "AKIAIOSFODNN7EXAMPLE",
        secret_access_key: "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY",
        region: "us-east-1",
        endpoint: "s3.amazonaws.com",
        bucket: "examplebucket",
        virtual_hosted_style: true,
    };
    expect_eq(creds.validate() == S3SignErrorCode.Ok, true);

    let client = S3Client.init(creds);

    // 1. AWS Official SigV4 Presigned URL Test Vector:
    // GET https://examplebucket.s3.amazonaws.com/test.txt (Date: 20130524T000000Z, Expires: 86400)
    let presign_buf: [512]u8 = undefined;
    const presigned = client.presignUrl(
        "GET",
        "test.txt",
        "20130524",
        "20130524T000000Z",
        86400,
        &presign_buf
    )?;

    const expected_presigned =
        "https://examplebucket.s3.amazonaws.com/test.txt" +
        "?X-Amz-Algorithm=AWS4-HMAC-SHA256" +
        "&X-Amz-Credential=AKIAIOSFODNN7EXAMPLE%2F20130524%2Fus-east-1%2Fs3%2Faws4_request" +
        "&X-Amz-Date=20130524T000000Z" +
        "&X-Amz-Expires=86400" +
        "&X-Amz-SignedHeaders=host" +
        "&X-Amz-Signature=aeeed9bbccd4d02ee5c0109b86d86835f995330da4c265957d157751f604d404";
    expect_eq(std.mem.eql(u8, presigned, expected_presigned), true);

    // 2. Header-based Signed Request via S3Client
    let url_buf: [256]u8 = undefined;
    let auth_buf: [512]u8 = undefined;
    let put_opts = client.upload("logs/2026.json", "{\"ok\":true}", S3SimpleRequestOptions{
        content_type: "application/json",
        acl: S3Acl.Private,
        storage_class: S3StorageClass.Standard,
    });
    const signed = client.signRequest(put_opts, "20261119", "20261119T080000Z", &url_buf, &auth_buf)?;
    expect_eq(signed.method, "PUT");
    expect_eq(signed.url, "https://examplebucket.s3.amazonaws.com/logs/2026.json");
    expect_eq(signed.host, "examplebucket.s3.amazonaws.com");
    expect_eq(signed.canonical_uri, "/logs/2026.json");
    expect_eq(signed.content_sha256.len, 64 as usize);
}

fn testS3MultipartListAndRange(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Multipart Upload lifecycle
    const uid_opt = parseInitiateMultipartUploadResult(
        "<InitiateMultipartUploadResult><UploadId>mp-upload-2026</UploadId></InitiateMultipartUploadResult>"
    );
    expect_eq(uid_opt != null, true);
    expect_eq(uid_opt.?, "mp-upload-2026");

    let parts: [2]UploadPartResult = [
        UploadPartResult{ number: 1, etag: "\"etag-part-1\"" },
        UploadPartResult{ number: 2, etag: "\"etag-part-2\"" },
    ];
    let xml_buf: [512]u8 = undefined;
    const xml_len = formatCompleteMultipartUploadXML(parts[0..2], &xml_buf);
    expect_eq(xml_len > 100, true);

    const final_etag = parseCompleteMultipartUploadResult(
        "<CompleteMultipartUploadResult><ETag>\"final-combined-etag-2\"</ETag></CompleteMultipartUploadResult>"
    );
    expect_eq(final_etag.?, "\"final-combined-etag-2\"");

    // 2. ListObjectsV2 query & XML parsing
    let q_buf: [256]u8 = undefined;
    const q_len = formatListObjectsQuery(S3ListObjectsOptions{
        prefix: "checkpoints/",
        delimiter: "/",
        max_keys: 25,
    }, &q_buf);
    expect_eq(q_buf[0..q_len], "list-type=2&prefix=checkpoints/&delimiter=/&max-keys=25");

    const list_xml =
        "<ListBucketResult>" +
        "<Name>ai-bucket</Name><Prefix>checkpoints/</Prefix><KeyCount>1</KeyCount><MaxKeys>25</MaxKeys><IsTruncated>false</IsTruncated>" +
        "<Contents><Key>checkpoints/step-1000.pt</Key><ETag>\"e1\"</ETag><Size>1048576</Size><StorageClass>STANDARD</StorageClass></Contents>" +
        "<CommonPrefixes><Prefix>checkpoints/archive/</Prefix></CommonPrefixes>" +
        "</ListBucketResult>";
    let parsed_list = parseS3ListObjectsResult(alloc, list_xml)?;
    defer parsed_list.deinit(alloc);
    expect_eq(parsed_list.name, "ai-bucket");
    expect_eq(parsed_list.contents.len, 1 as usize);
    expect_eq(parsed_list.contents[0].key, "checkpoints/step-1000.pt");
    expect_eq(parsed_list.contents[0].object_size, 1048576 as i64);
    expect_eq(parsed_list.common_prefixes.len, 1 as usize);
    expect_eq(parsed_list.common_prefixes[0], "checkpoints/archive/");

    // 3. Range header & Content-Range parsing
    let r_buf: [64]u8 = undefined;
    const r_len = formatRangeHeader(4096, 1024, &r_buf);
    expect_eq(r_buf[0..r_len], "bytes=4096-5119");
    if (cr := parseContentRange("bytes 4096-5119/65536")) {
        expect_eq(cr.start, 4096 as usize);
        expect_eq(cr.end, 5119 as usize);
        expect_eq(cr.total, 65536 as usize);
    }
    expect_eq(calculateChunkCount(65536, 16384), 4 as usize);
}

fn testSecurityCsrfAndTimingGuard(): !void {
    const alloc = std.heap.page_allocator;
    const secret = "cp_enterprise_csrf_hmac_secret_2026";

    // 1. Constant-time comparison
    expect_eq(constantTimeEql("digest_abc_123", "digest_abc_123"), true);
    expect_eq(constantTimeEql("digest_abc_123", "digest_abc_124"), false);
    expect_eq(constantTimeEql("short", "longer_slice"), false);

    // 2. Deterministic raw token generation, expiration & clock-skew protection
    let raw_tok: [64]u8 = undefined;
    const t0: u64 = 1700000000000;
    const ttl: u64 = 60000; // 60s
    const raw_len = CSRF.generateRawAt(secret, t0, ttl, &raw_tok)?;
    expect_eq(raw_len, 64 as usize);

    // Valid at t0 + 30s
    expect_eq(CSRF.verifyRawAt(&raw_tok, secret, t0 + 30000, ttl), true);
    // Expired at t0 + 61s
    expect_eq(CSRF.verifyRawAt(&raw_tok, secret, t0 + 61000, ttl), false);
    // Expired when caller passes stricter max_age_ms (10s) at t0 + 15s
    expect_eq(CSRF.verifyRawAt(&raw_tok, secret, t0 + 15000, 10000), false);
    // Clock skew rejection: token timestamp is > 60s in the future relative to now_ms
    expect_eq(CSRF.verifyRawAt(&raw_tok, secret, t0 - 120000, ttl), false);
    // Wrong secret rejection
    expect_eq(CSRF.verifyRawAt(&raw_tok, "wrong_secret", t0 + 1000, ttl), false);

    // 3. Base64Url, Base64, and Hex encoded tokens
    const tok_b64url = CSRF.generate(secret, DEFAULT_EXPIRATION_MS, TokenFormat.Base64Url, alloc)?;
    defer alloc.free(tok_b64url);
    expect_eq(CSRF.verify(tok_b64url, secret, DEFAULT_EXPIRATION_MS, TokenFormat.Base64Url, alloc), true);

    const tok_b64 = CSRF.generate(secret, DEFAULT_EXPIRATION_MS, TokenFormat.Base64, alloc)?;
    defer alloc.free(tok_b64);
    expect_eq(CSRF.verify(tok_b64, secret, DEFAULT_EXPIRATION_MS, TokenFormat.Base64, alloc), true);

    const tok_hex = CSRF.generate(secret, DEFAULT_EXPIRATION_MS, TokenFormat.Hex, alloc)?;
    defer alloc.free(tok_hex);
    expect_eq(tok_hex.len, 128 as usize);
    expect_eq(CSRF.verify(tok_hex, secret, DEFAULT_EXPIRATION_MS, TokenFormat.Hex, alloc), true);
}

fn testRedisResp2Resp3AndPipeline(): !void {
    // 1. RESP2 & RESP3 encoding and ValkeyReader streaming zero-copy deserialization
    let wire_buf: [512]u8 = undefined;
    let pos: usize = 0;

    pos += RESPEncoder.encodeSimpleString(wire_buf[pos..], "PONG");
    pos += RESPEncoder.encodeError(wire_buf[pos..], "ERR Auth required");
    pos += RESPEncoder.encodeInteger(wire_buf[pos..], -98765);
    pos += RESPEncoder.encodeBoolean(wire_buf[pos..], true);
    pos += RESPEncoder.encodeNull(wire_buf[pos..]);
    pos += RESPEncoder.encodeNullBulkString(wire_buf[pos..]);
    pos += RESPEncoder.encodeBulkString(wire_buf[pos..], "valkey_payload");
    pos += RESPEncoder.encodeMapHeader(wire_buf[pos..], 2);
    pos += RESPEncoder.encodeSetHeader(wire_buf[pos..], 3);

    let reader = ValkeyReader.init(wire_buf[0..pos]);
    expect_eq(reader.hasMore(), true);

    // Frame 1: SimpleString (+PONG\r\n)
    if (f1 := reader.nextFrame()) {
        expect_eq(f1.kind == RESPType.SimpleString, true);
        expect_eq(f1.str_val, "PONG");
    } else {
        expect_eq(true, false);
    }

    // Frame 2: Error (-ERR Auth required\r\n)
    if (f2 := reader.nextFrame()) {
        expect_eq(f2.kind == RESPType.Error, true);
        expect_eq(f2.str_val, "ERR Auth required");
    } else {
        expect_eq(true, false);
    }

    // Frame 3: Integer (:-98765\r\n)
    if (f3 := reader.nextFrame()) {
        expect_eq(f3.kind == RESPType.Integer, true);
        expect_eq(f3.int_val, -98765 as i64);
    } else {
        expect_eq(true, false);
    }

    // Frame 4: RESP3 Boolean (#t\r\n)
    if (f4 := reader.nextFrame()) {
        expect_eq(f4.kind == RESPType.Boolean, true);
        expect_eq(f4.bool_val, true);
    } else {
        expect_eq(true, false);
    }

    // Frame 5: RESP3 Null (_\r\n)
    if (f5 := reader.nextFrame()) {
        expect_eq(f5.kind == RESPType.Null, true);
        expect_eq(f5.is_null, true);
    } else {
        expect_eq(true, false);
    }

    // Frame 6: RESP2 Null BulkString ($-1\r\n)
    if (f6 := reader.nextFrame()) {
        expect_eq(f6.kind == RESPType.BulkString, true);
        expect_eq(f6.is_null, true);
    } else {
        expect_eq(true, false);
    }

    // Frame 7: BulkString ($14\r\nvalkey_payload\r\n)
    if (f7 := reader.nextFrame()) {
        expect_eq(f7.kind == RESPType.BulkString, true);
        expect_eq(f7.is_null, false);
        expect_eq(f7.str_val, "valkey_payload");
    } else {
        expect_eq(true, false);
    }

    // Frame 8: RESP3 Map Header (%2\r\n)
    if (f8 := reader.nextFrame()) {
        expect_eq(f8.kind == RESPType.Map, true);
        expect_eq(f8.array_count, 2 as i64);
    } else {
        expect_eq(true, false);
    }

    // Frame 9: RESP3 Set Header (~3\r\n)
    if (f9 := reader.nextFrame()) {
        expect_eq(f9.kind == RESPType.Set, true);
        expect_eq(f9.array_count, 3 as i64);
    } else {
        expect_eq(true, false);
    }

    expect_eq(reader.hasMore(), false);

    // 2. RESP3 VerbatimString & Pub/Sub Push Message helpers
    if (vf := RESPParser.parseFrame("=15\r\nmkd:# Title Doc\r\n")) {
        expect_eq(vf.kind == RESPType.VerbatimString, true);
        if (vb := VerbatimString.parse(vf.str_val)) {
            expect_eq(vb.format, "mkd");
            expect_eq(vb.content, "# Title Doc");
        }
    }
    expect_eq(SubscriptionPushMessage.fromString("message") == SubscriptionPushMessage.Message, true);
    expect_eq(SubscriptionPushMessage.fromString("subscribe") == SubscriptionPushMessage.Subscribe, true);

    // 3. RedisCommand & RedisPipeline batching
    let pipe = RedisPipeline.init();
    expect_eq(pipe.ping(), true);
    expect_eq(pipe.set("k1", "v1"), true);
    expect_eq(pipe.get("k1"), true);
    expect_eq(pipe.expire("k1", 120), true);
    expect_eq(pipe.hset("h1", "f1", "val1"), true);
    expect_eq(pipe.hget("h1", "f1"), true);
    expect_eq(pipe.lpush("q1", "job1"), true);
    expect_eq(pipe.rpop("q1"), true);
    expect_eq(pipe.del("k1"), true);
    expect_eq(pipe.command_count, 9 as usize);
    expect_eq(pipe.bytes().len > 150, true);
}

fn main() {
    test("1. cap:s3 — AWS SigV4 official Presigned URL vector & signed request builder", () => {
        try testS3SigV4AndPresign();
    });
    test("2. cap:s3 — Multipart Upload XML, ListObjectsV2 XML parser & HTTP Range streaming", () => {
        try testS3MultipartListAndRange();
    });
    test("3. cap:security — CSRF 64-byte token lifecycle, TTL expiration, clock-skew & constantTimeEql", () => {
        try testSecurityCsrfAndTimingGuard();
    });
    test("4. cap:redis — RESP2/RESP3 frame encoder, ValkeyReader zero-copy stream & RedisPipeline", () => {
        try testRedisResp2Resp3AndPipeline();
    });
}
