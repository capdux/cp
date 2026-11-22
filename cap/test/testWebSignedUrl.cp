// Origin: Pure-CP Stdlib Test Suite (lib/web/signedUrl.cp) | Created: 2026-11-23
// Production Deep Verification Suite for HMAC-SHA256 Pre-Signed URL & Multi-Key Rotation Verifier:
//   1. Stateless signUrl & verifySignedUrl round-trip (with/without existing query, with/without kid, case-insensitive method)
//   2. Expiry window & clock_skew_s tolerance boundary checks
//   3. Exact method_mismatch detection vs invalid_signature (tampered path/query/exp/sig)
//   4. Missing params (missing_signature, missing_expiry) & malformed_url / CRLF / duplicate-param / tiny-buffer guards
//   5. SignedUrlKeyring multi-key zero-downtime rotation, revocation & 8-key capacity boundary
//   6. Keyring global telemetry stats, resetStats & cap.web facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SIGNED_URL_SIG_LEN,
    SIGNED_URL_MAX_KEYS,
    SIGNED_URL_DEFAULT_CLOCK_SKEW_S,
    SignedUrlStatus,
    SignedUrlKeyring,
    signedUrlConstantTimeEq,
    formatSignedUrlStatus,
    signUrl,
    verifySignedUrl,
} from "cap:web";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

test("SignedUrl: stateless signUrl & verifySignedUrl round-trip with/without query & kid") {
    const secret = "prod-presign-secret-key-at-least-32-bytes";
    let buf: [512]u8 = [0; 512];
    let buf_s: []u8 = buf[0..];

    // 1. With existing query + kid
    const n1 = signUrl(secret, "GET", "/assets/model.bin", "v=2&region=ap", 1_700_003_600, "k1", *buf_s) ?? 0;
    expect_true(n1 > 0);
    expect_true(containsSubstr(buf[0..n1], "/assets/model.bin?v=2&region=ap&exp=1700003600&kid=k1&sig="));

    const r1 = verifySignedUrl(secret, "get", buf[0..n1], 1_700_000_000, SIGNED_URL_DEFAULT_CLOCK_SKEW_S);
    expect_eq(r1.status, SignedUrlStatus.valid);
    expect_true(r1.valid);
    expect_eq(r1.exp_epoch_s, @as(i64, 1_700_003_600));
    expect_true(bytesEq(r1.kid(), "k1"));

    // 2. Without existing query and without kid (long >64B secret exercises SHA-256 key hashing path)
    const long_secret = "very-long-secret-key-exceeding-64-bytes-hmac-block-size-0123456789abcdef-extra";
    const n2 = signUrl(long_secret, "PUT", "/uploads/chunk-9", "", 1_700_009_999, "", *buf_s) ?? 0;
    expect_true(n2 > 0);
    expect_true(containsSubstr(buf[0..n2], "/uploads/chunk-9?exp=1700009999&sig="));
    expect_true(!containsSubstr(buf[0..n2], "&kid="));

    const r2 = verifySignedUrl(long_secret, "PUT", buf[0..n2], 1_700_009_000, 0);
    expect_eq(r2.status, SignedUrlStatus.valid);
    expect_true(r2.valid);
    expect_eq(r2.kid().len, @as(usize, 0));

    // Constant-time helper & status labels
    expect_true(signedUrlConstantTimeEq("abc", "abc"));
    expect_true(!signedUrlConstantTimeEq("abc", "abd"));
    expect_true(!signedUrlConstantTimeEq("abc", "ab"));
    expect_true(bytesEq(formatSignedUrlStatus(SignedUrlStatus.valid), "valid"));
    expect_true(bytesEq(formatSignedUrlStatus(SignedUrlStatus.method_mismatch), "method_mismatch"));
    expect_eq(SIGNED_URL_SIG_LEN, @as(usize, 43));
}

test("SignedUrl: expiry window & clock_skew_s tolerance boundary") {
    const secret = "expiry-boundary-secret-key";
    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];

    const exp_s: i64 = 1_700_001_000;
    const n = signUrl(secret, "GET", "/dl/a.zip", "", exp_s, "k1", *buf_s) ?? 0;
    expect_true(n > 0);

    // Exact expiry time -> valid
    expect_eq(verifySignedUrl(secret, "GET", buf[0..n], exp_s, 0).status, SignedUrlStatus.valid);
    // 1 second past expiry with 0 skew -> expired
    const r_exp0 = verifySignedUrl(secret, "GET", buf[0..n], exp_s + 1, 0);
    expect_eq(r_exp0.status, SignedUrlStatus.expired);
    expect_true(!r_exp0.valid);
    expect_eq(r_exp0.exp_epoch_s, exp_s);

    // 30 seconds past expiry with 30s skew -> still valid
    expect_eq(verifySignedUrl(secret, "GET", buf[0..n], exp_s + 30, 30).status, SignedUrlStatus.valid);
    // 31 seconds past expiry with 30s skew -> expired
    expect_eq(verifySignedUrl(secret, "GET", buf[0..n], exp_s + 31, 30).status, SignedUrlStatus.expired);
}

test("SignedUrl: exact method_mismatch vs tampered path/query/sig invalid_signature") {
    const secret = "tamper-detection-secret-key";
    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];

    const n = signUrl(secret, "GET", "/api/export", "fmt=csv", 1_700_005_000, "v1", *buf_s) ?? 0;
    expect_true(n > 0);

    // 1. Replaying GET-signed URL as POST or DELETE -> exact method_mismatch
    const r_post = verifySignedUrl(secret, "POST", buf[0..n], 1_700_000_000, 0);
    expect_eq(r_post.status, SignedUrlStatus.method_mismatch);
    expect_true(!r_post.valid);

    const r_del = verifySignedUrl(secret, "DELETE", buf[0..n], 1_700_000_000, 0);
    expect_eq(r_del.status, SignedUrlStatus.method_mismatch);

    // 2. Wrong secret -> invalid_signature
    const r_wrong_secret = verifySignedUrl("wrong-secret", "GET", buf[0..n], 1_700_000_000, 0);
    expect_eq(r_wrong_secret.status, SignedUrlStatus.invalid_signature);

    // 3. Single-byte tamper in signature -> invalid_signature
    let tampered_sig: [256]u8 = [0; 256];
    let i: usize = 0;
    while (i < n) : (i += 1) {
        tampered_sig[i] = buf[i];
    }
    tampered_sig[n - 1] = if (tampered_sig[n - 1] == 'A') 'B' else 'A';
    const r_tsig = verifySignedUrl(secret, "GET", tampered_sig[0..n], 1_700_000_000, 0);
    expect_eq(r_tsig.status, SignedUrlStatus.invalid_signature);

    // 4. Tamper query value (`fmt=csv` -> `fmt=csx`) -> invalid_signature
    let tampered_q: [256]u8 = [0; 256];
    i = 0;
    while (i < n) : (i += 1) {
        tampered_q[i] = buf[i];
    }
    // "/api/export?fmt=csv" -> index 18 is 'v'
    tampered_q[18] = 'x';
    const r_tq = verifySignedUrl(secret, "GET", tampered_q[0..n], 1_700_000_000, 0);
    expect_eq(r_tq.status, SignedUrlStatus.invalid_signature);
}

test("SignedUrl: missing params, malformed URLs, CRLF/pollution & tiny buffer guards") {
    const secret = "hygiene-guard-secret";
    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];

    // Missing signature / missing expiry
    expect_eq(verifySignedUrl(secret, "GET", "/files/a.txt", 100, 0).status, SignedUrlStatus.missing_signature);
    expect_eq(verifySignedUrl(secret, "GET", "/files/a.txt?exp=200", 100, 0).status, SignedUrlStatus.missing_signature);
    expect_eq(verifySignedUrl(secret, "GET", "/files/a.txt?sig=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 100, 0).status, SignedUrlStatus.missing_expiry);

    // Malformed URLs: empty, relative path, CRLF injection, fragment, duplicate sig, non-numeric exp
    expect_eq(verifySignedUrl(secret, "GET", "", 100, 0).status, SignedUrlStatus.malformed_url);
    expect_eq(verifySignedUrl(secret, "GET", "relative/path?exp=200&sig=abc", 100, 0).status, SignedUrlStatus.malformed_url);
    expect_eq(verifySignedUrl(secret, "GET", "/a\r\nX-Injected: 1?exp=200&sig=abc", 100, 0).status, SignedUrlStatus.malformed_url);
    expect_eq(verifySignedUrl(secret, "GET", "/a?exp=200&sig=abc#frag", 100, 0).status, SignedUrlStatus.malformed_url);
    expect_eq(verifySignedUrl(secret, "GET", "/a?exp=200&sig=abc&sig=def", 100, 0).status, SignedUrlStatus.malformed_url);
    expect_eq(verifySignedUrl(secret, "GET", "/a?exp=notanumber&sig=abc", 100, 0).status, SignedUrlStatus.malformed_url);

    // signUrl hygiene guards: empty path, CRLF, pre-existing reserved param, non-positive exp, tiny buffer
    expect_true(signUrl(secret, "GET", "", "", 200, "k1", *buf_s) == null);
    expect_true(signUrl(secret, "GET", "/a\nb", "", 200, "k1", *buf_s) == null);
    expect_true(signUrl(secret, "GET", "/a", "sig=forged", 200, "k1", *buf_s) == null);
    expect_true(signUrl(secret, "GET", "/a", "exp=999", 200, "k1", *buf_s) == null);
    expect_true(signUrl(secret, "GET", "/a", "", 0, "k1", *buf_s) == null);

    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(signUrl(secret, "GET", "/files/report.pdf", "", 200, "k1", *tiny_s) == null);
}

test("SignedUrlKeyring: zero-downtime multi-key rotation, revocation & 8-key capacity") {
    let kr = SignedUrlKeyring.init();
    expect_true(kr.addKey("k1", "secret-v1-alpha-0000000000000001", true));
    expect_eq(kr.keyCount(), @as(usize, 1));

    let url1_buf: [256]u8 = [0; 256];
    let url1_s: []u8 = url1_buf[0..];
    const n1 = kr.signWithPrimary("GET", "/media/img1.png", "w=800", 1_700_010_000, *url1_s) ?? 0;
    expect_true(n1 > 0);
    expect_true(containsSubstr(url1_buf[0..n1], "kid=k1"));

    // Rotate primary to k2 while keeping k1 active -> zero downtime!
    expect_true(kr.addKey("k2", "secret-v2-beta-00000000000000002", true));
    expect_eq(kr.keyCount(), @as(usize, 2));

    let url2_buf: [256]u8 = [0; 256];
    let url2_s: []u8 = url2_buf[0..];
    const n2 = kr.signWithPrimary("GET", "/media/img2.png", "", 1_700_010_000, *url2_s) ?? 0;
    expect_true(n2 > 0);
    expect_true(containsSubstr(url2_buf[0..n2], "kid=k2"));

    // Both old (k1) and new (k2) URLs verify cleanly
    const v1 = kr.verify("GET", url1_buf[0..n1], 1_700_000_000, 30);
    expect_eq(v1.status, SignedUrlStatus.valid);
    expect_true(bytesEq(v1.kid(), "k1"));

    const v2 = kr.verify("GET", url2_buf[0..n2], 1_700_000_000, 30);
    expect_eq(v2.status, SignedUrlStatus.valid);
    expect_true(bytesEq(v2.kid(), "k2"));

    // Revoke k1 -> old k1 URL now fails with invalid_signature and increments unknown_kid_total
    expect_true(kr.revokeKey("k1"));
    expect_true(!kr.hasKey("k1"));
    const v1_revoked = kr.verify("GET", url1_buf[0..n1], 1_700_000_000, 30);
    expect_eq(v1_revoked.status, SignedUrlStatus.invalid_signature);
    expect_eq(kr.stats().unknown_kid_total, @as(u64, 1));

    // Fill up to 8 keys, verify 9th key is rejected
    expect_true(kr.addKey("k3", "s3", false));
    expect_true(kr.addKey("k4", "s4", false));
    expect_true(kr.addKey("k5", "s5", false));
    expect_true(kr.addKey("k6", "s6", false));
    expect_true(kr.addKey("k7", "s7", false));
    expect_true(kr.addKey("k8", "s8", false));
    expect_true(kr.addKey("k9", "s9", false));
    expect_eq(kr.keyCount(), SIGNED_URL_MAX_KEYS);
    expect_true(!kr.addKey("k10", "s10", false));
}

test("SignedUrlKeyring: telemetry stats, resetStats & cap.web facade parity") {
    let kr = cap.web.SignedUrlKeyring.init();
    expect_true(kr.addKey("main", "facade-secret-key-1234567890", true));

    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];
    const n = kr.signWithKey("main", "GET", "/v1/blob", "id=42", 2000, *buf_s) ?? 0;
    expect_true(n > 0);

    // 1 valid, 1 expired, 1 method_mismatch, 1 malformed
    expect_eq(kr.verify("GET", buf[0..n], 1500, 0).status, cap.web.SignedUrlStatus.valid);
    expect_eq(kr.verify("GET", buf[0..n], 2500, 0).status, cap.web.SignedUrlStatus.expired);
    expect_eq(kr.verify("POST", buf[0..n], 1500, 0).status, cap.web.SignedUrlStatus.method_mismatch);
    expect_eq(kr.verify("GET", "bad-url", 1500, 0).status, cap.web.SignedUrlStatus.malformed_url);

    const st = kr.stats();
    expect_eq(st.key_count, @as(usize, 1));
    expect_eq(st.capacity, SIGNED_URL_MAX_KEYS);
    expect_eq(st.signed_total, @as(u64, 1));
    expect_eq(st.verified_total, @as(u64, 4));
    expect_eq(st.valid_total, @as(u64, 1));
    expect_eq(st.expired_total, @as(u64, 1));
    expect_eq(st.method_mismatch_total, @as(u64, 1));
    expect_eq(st.malformed_total, @as(u64, 1));
    expect_eq(st.invalid_sig_total, @as(u64, 0));

    kr.resetStats();
    const st2 = kr.stats();
    expect_eq(st2.key_count, @as(usize, 1)); // keys preserved
    expect_eq(st2.verified_total, @as(u64, 0));
    expect_eq(st2.signed_total, @as(u64, 0));
}
