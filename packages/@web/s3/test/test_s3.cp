// Unit tests for @web/s3
const std = @import("std");
import { App, AppMethod, Request } from "../../../../lib/web/app.cp";
import {
    S3PluginConfig,
    s3NormalizeKey,
    s3BuildPresignedUrl,
    registerS3,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn strContains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (strEql(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

pub fn main() {
    // 1. s3NormalizeKey strips leading slashes & blocks traversal/backslash/control
    let kbuf: [256]u8 = [0; 256];
    const k1 = s3NormalizeKey("///avatars/u42.png", kbuf[0..]) orelse 0;
    assertTrue(strEql(kbuf[0..k1], "avatars/u42.png"), "leading slashes stripped");

    assertTrue(s3NormalizeKey("", kbuf[0..]) == null, "empty key rejected");
    assertTrue(s3NormalizeKey("///", kbuf[0..]) == null, "only-slash key rejected");
    assertTrue(s3NormalizeKey("../etc/passwd", kbuf[0..]) == null, "dot-dot traversal rejected");
    assertTrue(s3NormalizeKey("a/b/../../c", kbuf[0..]) == null, "nested dot-dot rejected");
    assertTrue(s3NormalizeKey("dir\\file.txt", kbuf[0..]) == null, "backslash rejected");
    assertTrue(s3NormalizeKey("dir/file\n.txt", kbuf[0..]) == null, "control char rejected");

    // 2. s3BuildPresignedUrl produces deterministic AWS SigV4 URL
    const cfg = S3PluginConfig{
        endpoint: "https://s3.us-east-1.amazonaws.com",
        bucket: "cp-assets",
        region: "us-east-1",
        access_key: "AKIAEXAMPLE123",
        secret_key: "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY",
        presign_path: "/upload/presign",
        default_expires_secs: 600,
    };

    let url1: [1024]u8 = [0; 1024];
    const u1_len = s3BuildPresignedUrl(cfg, "PUT", "/docs/report.pdf", 600, "20261125T103000Z", url1[0..]) orelse 0;
    assertTrue(u1_len > 100, "presigned URL generated");
    const u1 = url1[0..u1_len];
    assertTrue(strContains(u1, "https://s3.us-east-1.amazonaws.com/cp-assets/docs/report.pdf?"), "URL base + bucket + key");
    assertTrue(strContains(u1, "X-Amz-Algorithm=AWS4-HMAC-SHA256"), "contains X-Amz-Algorithm");
    assertTrue(strContains(u1, "X-Amz-Credential=AKIAEXAMPLE123%2F20261125%2Fus-east-1%2Fs3%2Faws4_request"), "contains X-Amz-Credential");
    assertTrue(strContains(u1, "X-Amz-Date=20261125T103000Z"), "contains X-Amz-Date");
    assertTrue(strContains(u1, "X-Amz-Expires=600"), "contains X-Amz-Expires");
    assertTrue(strContains(u1, "X-Amz-SignedHeaders=host"), "contains X-Amz-SignedHeaders");
    assertTrue(strContains(u1, "&X-Amz-Signature="), "contains X-Amz-Signature");

    // Determinism check: identical inputs -> identical signature
    let url2: [1024]u8 = [0; 1024];
    const u2_len = s3BuildPresignedUrl(cfg, "PUT", "docs/report.pdf", 600, "20261125T103000Z", url2[0..]) orelse 0;
    assertTrue(strEql(u1, url2[0..u2_len]), "SigV4 presigned URL is deterministic");

    // Different key -> different signature
    let url3: [1024]u8 = [0; 1024];
    const u3_len = s3BuildPresignedUrl(cfg, "PUT", "docs/other.pdf", 600, "20261125T103000Z", url3[0..]) orelse 0;
    assertTrue(!strEql(u1, url3[0..u3_len]), "different key yields different URL/signature");

    // Invalid key returns null
    assertTrue(s3BuildPresignedUrl(cfg, "PUT", "../secret", 600, "20261125T103000Z", url3[0..]) == null, "invalid key blocked in presigner");

    // 3. registerS3 endpoint tests
    let app = App.init();
    * = registerS3(*app, cfg);

    // Missing key -> 400
    const r_missing = app.dispatch(Request.init(AppMethod.GET, "/upload/presign"));
    assertTrue(r_missing.status == 400, "missing key returns 400");

    // Traversal key -> 400
    const r_bad = app.dispatch(Request.init(AppMethod.GET, "/upload/presign?key=../escape.txt"));
    assertTrue(r_bad.status == 400, "traversal key returns 400");

    // Valid key -> 200 JSON with bucket, key, url, expires
    const r_ok = app.dispatch(Request.init(AppMethod.GET, "/upload/presign?key=/images/logo.png"));
    assertTrue(r_ok.status == 200, "valid key returns 200");
    assertTrue(strContains(r_ok.body, "\"bucket\":\"cp-assets\""), "JSON contains bucket");
    assertTrue(strContains(r_ok.body, "\"key\":\"images/logo.png\""), "JSON contains normalized key");
    assertTrue(strContains(r_ok.body, "\"url\":\"https://s3.us-east-1.amazonaws.com/cp-assets/images/logo.png?"), "JSON contains presigned url");
    assertTrue(strContains(r_ok.body, "X-Amz-Signature="), "JSON URL contains SigV4 signature");
    assertTrue(strContains(r_ok.body, "\"expires\":600"), "JSON contains expires");

    std.debug.print("PASS: @web/s3 all tests passed!\n", .{});
}
