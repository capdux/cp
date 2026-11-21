// Source: cp/cap/test/test_s3_signing_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:s3 (sigv4) & cap.s3

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    S3SignError,
    SigV4Credentials,
    SigV4Header,
    SigV4QueryParam,
    SigV4SignedResult,
    awsUriEncode,
    deriveSigningKey,
    signRequest,
    presignUrl,
} from "cap:s3";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn emptySignedResult(): SigV4SignedResult {
    return SigV4SignedResult{};
}

// Test 1: AWS Official SigV4 Test Vector (get-vanilla: AKIDEXAMPLE / 20150830T123600Z / us-east-1 / service)
fn runTest1(): !void {
    const alloc = std.testing.allocator;

    const creds = SigV4Credentials{
        .access_key_id = "AKIDEXAMPLE",
        .secret_access_key = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY",
    };

    const headers = [
        SigV4Header{ .name = "Host", .value = "example.amazonaws.com" },
        SigV4Header{ .name = "X-Amz-Date", .value = "20150830T123600Z" },
    ];
    const query: []const SigV4QueryParam = [];

    let res = signRequest(
        alloc,
        "GET",
        "/",
        &query,
        &headers,
        "",
        creds,
        "us-east-1",
        "service",
        "20150830T123600Z",
    )?;
    defer res.deinit(alloc);

    const expected_sig = "5fa00fa31553b73ebf1942676e86291e8372ff2a2260956d9b8aae1d763fbf31";
    const expected_payload_sha = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    const expected_auth = "AWS4-HMAC-SHA256 Credential=AKIDEXAMPLE/20150830/us-east-1/service/aws4_request, SignedHeaders=host;x-amz-date, Signature=5fa00fa31553b73ebf1942676e86291e8372ff2a2260956d9b8aae1d763fbf31";

    expect_eq(std.mem.eql(u8, &res.signature, expected_sig), true);
    expect_eq(std.mem.eql(u8, &res.payload_sha256, expected_payload_sha), true);
    expect_eq(std.mem.eql(u8, &res.amz_date, "20150830T123600Z"), true);
    expect_eq(std.mem.eql(u8, res.authorization, expected_auth), true);

    // Verify derived signing key matches AWS SigV4 specification vector
    const key = deriveSigningKey("wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY", "20150830", "us-east-1", "service");
    const expected_key: [32]u8 = [
        0xc4, 0xaf, 0xb1, 0xcc, 0x57, 0x71, 0xd8, 0x71,
        0x76, 0x3a, 0x39, 0x3e, 0x44, 0xb7, 0x03, 0x57,
        0x1b, 0x55, 0xcc, 0x28, 0x42, 0x4d, 0x1a, 0x5e,
        0x86, 0xda, 0x6d, 0x3c, 0x1e, 0x2a, 0x4b, 0x3b,
    ];
    expect_eq(std.mem.eql(u8, &key, &expected_key), true);
}

// Test 2: awsUriEncode (slash preserve vs encode), multi-header/query sorting & session_token injection
fn runTest2(): !void {
    const alloc = std.testing.allocator;

    const enc_keep = awsUriEncode(alloc, "/my bucket/hello world~1_2-3.txt", false)?;
    defer alloc.free(enc_keep);
    expect_eq(std.mem.eql(u8, enc_keep, "/my%20bucket/hello%20world~1_2-3.txt"), true);

    const enc_slash = awsUriEncode(alloc, "a/b c+d", true)?;
    defer alloc.free(enc_slash);
    expect_eq(std.mem.eql(u8, enc_slash, "a%2Fb%20c%2Bd"), true);

    const creds = SigV4Credentials{
        .access_key_id = "AKIATEST123",
        .secret_access_key = "secretkey123",
        .session_token = "tok+abc/def==",
    };

    const headers = [
        SigV4Header{ .name = "X-Amz-Meta-Z", .value = "  zeta   value  " },
        SigV4Header{ .name = "Host", .value = "s3.us-west-2.amazonaws.com" },
        SigV4Header{ .name = "X-Amz-Meta-A", .value = "alpha" },
    ];
    const query = [
        SigV4QueryParam{ .name = "z_param", .value = "last" },
        SigV4QueryParam{ .name = "a_param", .value = "first value" },
    ];

    let res = signRequest(
        alloc,
        "PUT",
        "/bucket/my file.txt",
        &query,
        &headers,
        "UNSIGNED-PAYLOAD",
        creds,
        "us-west-2",
        "s3",
        "20250601T102030Z",
    )?;
    defer res.deinit(alloc);

    // CanonicalRequest must contain sorted query, sorted lowercase headers with x-amz-date & x-amz-security-token
    expect_eq(std.mem.indexOf(u8, res.canonical_request, "PUT\n/bucket/my%20file.txt\na_param=first%20value&z_param=last\n") != null, true);
    expect_eq(std.mem.indexOf(u8, res.canonical_request, "host:s3.us-west-2.amazonaws.com\nx-amz-date:20250601T102030Z\nx-amz-meta-a:alpha\nx-amz-meta-z:zeta value\nx-amz-security-token:tok+abc/def==\n") != null, true);
    expect_eq(std.mem.indexOf(u8, res.canonical_request, "\nhost;x-amz-date;x-amz-meta-a;x-amz-meta-z;x-amz-security-token\nUNSIGNED-PAYLOAD") != null, true);
    expect_eq(std.mem.indexOf(u8, res.authorization, "SignedHeaders=host;x-amz-date;x-amz-meta-a;x-amz-meta-z;x-amz-security-token") != null, true);
}

// Test 3: presignUrl complete S3 presigned URL with UNSIGNED-PAYLOAD, extra query params & session_token
fn runTest3(): !void {
    const alloc = std.testing.allocator;

    const creds = SigV4Credentials{
        .access_key_id = "AKIDEXAMPLE",
        .secret_access_key = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY",
        .session_token = "FwoGZXIvYXdzEBYa",
    };

    const extra = [
        SigV4QueryParam{ .name = "response-content-type", .value = "application/json" },
    ];

    const url = presignUrl(
        alloc,
        "https",
        "examplebucket.s3.amazonaws.com",
        "/test file.txt",
        "GET",
        3600,
        &extra,
        creds,
        "us-east-1",
        "s3",
        "20150830T123600Z",
    )?;
    defer alloc.free(url);

    expect_eq(std.mem.startsWith(u8, url, "https://examplebucket.s3.amazonaws.com/test%20file.txt?"), true);
    expect_eq(std.mem.indexOf(u8, url, "X-Amz-Algorithm=AWS4-HMAC-SHA256") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "X-Amz-Credential=AKIDEXAMPLE%2F20150830%2Fus-east-1%2Fs3%2Faws4_request") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "X-Amz-Date=20150830T123600Z") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "X-Amz-Expires=3600") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "X-Amz-Security-Token=FwoGZXIvYXdzEBYa") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "X-Amz-SignedHeaders=host") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "response-content-type=application%2Fjson") != null, true);
    expect_eq(std.mem.indexOf(u8, url, "&X-Amz-Signature=") != null, true);
}

// Test 4: Strict validation errors (InvalidCredentials, InvalidTimestamp, InvalidExpiration, InvalidPath)
fn runTest4(): !void {
    const alloc = std.testing.allocator;
    const empty_hdrs: []const SigV4Header = [];
    const empty_qry: []const SigV4QueryParam = [];

    // Empty credentials -> InvalidCredentials
    const bad_creds = SigV4Credentials{ .access_key_id = "", .secret_access_key = "secret" };
    let got_bad_creds = false;
    * = signRequest(alloc, "GET", "/", &empty_qry, &empty_hdrs, "", bad_creds, "us-east-1", "s3", "20150830T123600Z") catch e => err1: {
        got_bad_creds = (e == S3SignError.InvalidCredentials);
        break :err1 emptySignedResult();
    };
    expect_eq(got_bad_creds, true);

    const valid_creds = SigV4Credentials{ .access_key_id = "AKID", .secret_access_key = "SECRET" };

    // Bad timestamp -> InvalidTimestamp
    let got_bad_ts = false;
    * = signRequest(alloc, "GET", "/", &empty_qry, &empty_hdrs, "", valid_creds, "us-east-1", "s3", "2015-08-30") catch e => err2: {
        got_bad_ts = (e == S3SignError.InvalidTimestamp);
        break :err2 emptySignedResult();
    };
    expect_eq(got_bad_ts, true);

    // Expiration 0 -> InvalidExpiration
    let got_exp_zero = false;
    * = presignUrl(alloc, "https", "s3.amazonaws.com", "/k", "GET", 0, &empty_qry, valid_creds, "us-east-1", "s3", "20150830T123600Z") catch e => err3: {
        got_exp_zero = (e == S3SignError.InvalidExpiration);
        break :err3 emptyMutSlice();
    };
    expect_eq(got_exp_zero, true);

    // Expiration > 604800 -> InvalidExpiration
    let got_exp_over = false;
    * = presignUrl(alloc, "https", "s3.amazonaws.com", "/k", "GET", 604801, &empty_qry, valid_creds, "us-east-1", "s3", "20150830T123600Z") catch e => err4: {
        got_exp_over = (e == S3SignError.InvalidExpiration);
        break :err4 emptyMutSlice();
    };
    expect_eq(got_exp_over, true);

    // Invalid path (missing leading '/') -> InvalidPath
    let got_bad_path = false;
    * = signRequest(alloc, "GET", "no-leading-slash", &empty_qry, &empty_hdrs, "", valid_creds, "us-east-1", "s3", "20150830T123600Z") catch e => err5: {
        got_bad_path = (e == S3SignError.InvalidPath);
        break :err5 emptySignedResult();
    };
    expect_eq(got_bad_path, true);
}

// Test 5: Dynamic cap.s3_signing mounting & std.testing.allocator zero-leak verification
fn runTest5(): !void {
    const alloc = std.testing.allocator;

    const encoded = cap.s3.awsUriEncode(alloc, "dir/sub dir/file.bin", false)?;
    defer alloc.free(encoded);
    expect_eq(std.mem.eql(u8, encoded, "dir/sub%20dir/file.bin"), true);

    const creds = cap.s3.SigV4Credentials{
        .access_key_id = "AKIDHUB",
        .secret_access_key = "SECRETHUB",
    };
    const empty_qry: []const cap.s3.SigV4QueryParam = [];
    const url = cap.s3.presignUrl(
        alloc,
        "https",
        "hub.s3.us-east-1.amazonaws.com",
        "/object",
        "PUT",
        604800,
        &empty_qry,
        creds,
        "us-east-1",
        "s3",
        "20260101T000000Z",
    )?;
    defer alloc.free(url);
    expect_eq(std.mem.indexOf(u8, url, "X-Amz-Expires=604800") != null, true);
}

test("s3_signing: AWS official SigV4 vector & deriveSigningKey") {
    runTest1() catch unreachable;
}

test("s3_signing: URI encoding, header/query sorting & session token") {
    runTest2() catch unreachable;
}

test("s3_signing: presignUrl complete S3 presigned URL") {
    runTest3() catch unreachable;
}

test("s3_signing: invalid credentials, timestamp, expiration & path rejection") {
    runTest4() catch unreachable;
}

test("s3_signing: cap.s3_signing dynamic hub mount & zero memory leaks") {
    runTest5() catch unreachable;
}
