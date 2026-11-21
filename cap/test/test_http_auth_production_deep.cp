import { test, expect } from "cap:test";
import {
    AuthScheme,
    AuthHeader,
    decodeBasicAuth,
    encodeBasicAuth,
    encodeBearerAuth,
} from "cap:http";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("http_auth: Basic auth encode/decode roundtrip (RFC 7617)", () => {
    let enc_buf: [128]u8 = [0; 128];
    let dec_buf: [128]u8 = [0; 128];

    // RFC 7617 test vector: Aladdin:open sesame -> QWxhZGRpbjpvcGVuIHNlc2FtZQ==
    const hdr = encodeBasicAuth("Aladdin", "open sesame", enc_buf[0..]).?;
    expect(strEq(hdr, "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="));

    const creds = decodeBasicAuth(hdr, dec_buf[0..]).?;
    expect(strEq(creds.username, "Aladdin") && strEq(creds.password, "open sesame"));

    // Password containing ':' preserved
    const hdr2 = encodeBasicAuth("user", "p:a:s:s", enc_buf[0..]).?;
    const creds2 = decodeBasicAuth(hdr2, dec_buf[0..]).?;
    expect(strEq(creds2.username, "user") && strEq(creds2.password, "p:a:s:s"));

    expect(decodeBasicAuth("Bearer tok", dec_buf[0..]) == null);
    expect(decodeBasicAuth("Basic @@@", dec_buf[0..]) == null);
});

test("http_auth: Bearer and custom scheme parsing", () => {
    let buf: [128]u8 = [0; 128];
    const b = encodeBearerAuth("eyJhbGciOiJIUzI1NiJ9.payload.sig", buf[0..]).?;
    const parsed = AuthHeader.parse(b).?;
    expect(parsed.scheme == AuthScheme.bearer);
    expect(strEq(parsed.bearerToken().?, "eyJhbGciOiJIUzI1NiJ9.payload.sig"));

    const aws = AuthHeader.parse("AWS4-HMAC-SHA256 Credential=AKIA/20261122/us-east-1/s3/aws4_request").?;
    expect(aws.scheme == AuthScheme.aws4_hmac_sha256);

    expect(AuthHeader.parse("") == null);
    expect(AuthHeader.parse("Basic") == null);
});
