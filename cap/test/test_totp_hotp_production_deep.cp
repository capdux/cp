// Source: native-cp-stdlib (RFC 4226 / RFC 6238 TOTP & HOTP)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HashAlg,
    OtpType,
    OtpAuthUri,
    hotpAt,
    hotpAtWithAlg,
    totpAt,
    formatCode,
    constantTimeEq,
    verifyHotp,
    verifyTotp,
    base32Encode,
    base32Decode,
    buildOtpAuthUri,
    parseOtpAuthUri,
} from "cap:auth";

test "RFC 4226 Appendix D official HOTP test vectors (SHA-1 counter 0..9)" {
    const secret = "12345678901234567890";
    const expected: [10]u32 = [
        755224,
        287082,
        359152,
        969429,
        338314,
        254676,
        287922,
        162583,
        399871,
        520489,
    ];

    let c: u64 = 0;
    while (c < 10) {
        const idx: usize = @intCast(c);
        const got = hotpAt(secret, c, 6);
        expect_eq(got, expected[idx]);
        c = c + 1;
    }
}

test "RFC 6238 Appendix B official TOTP test vectors (SHA-1 / SHA-256 / SHA-512)" {
    const secret_sha1 = "12345678901234567890";
    const secret_sha256 = "12345678901234567890123456789012";
    const secret_sha512 = "1234567890123456789012345678901234567890123456789012345678901234";

    const times: [6]u64 = [ 59, 1111111109, 1111111111, 1234567890, 2000000000, 20000000000 ];
    const exp_sha1: [6]u32 = [ 94287082, 7081804, 14050471, 89005924, 69279037, 65353130 ];
    const exp_sha256: [6]u32 = [ 46119246, 68084774, 67062674, 91819424, 90698825, 77737706 ];
    const exp_sha512: [6]u32 = [ 90693936, 25091201, 99943326, 93441116, 38618901, 47863826 ];

    let i: usize = 0;
    while (i < 6) {
        expect_eq(totpAt(secret_sha1, times[i], 30, 8, HashAlg.Sha1), exp_sha1[i]);
        expect_eq(totpAt(secret_sha256, times[i], 30, 8, HashAlg.Sha256), exp_sha256[i]);
        expect_eq(totpAt(secret_sha512, times[i], 30, 8, HashAlg.Sha512), exp_sha512[i]);
        i = i + 1;
    }
}

test "verifyHotp resync lookahead and verifyTotp time window drift and constant-time comparison" {
    const secret = "12345678901234567890";

    // HOTP counter=5 -> 254676, lookahead=3 starting from counter=3 should match and return 6
    let buf6: [6]u8 = undefined;
    const code5 = formatCode(254676, 6, &buf6);
    const resync = verifyHotp(secret, 3, code5, 6, 3, HashAlg.Sha1);
    expect_eq(resync != null, true);
    expect_eq(resync.?, 6);

    // Out of lookahead window
    const resync_miss = verifyHotp(secret, 0, code5, 6, 4, HashAlg.Sha1);
    expect_eq(resync_miss == null, true);

    // TOTP at unix_sec=59 (step 1) is 94287082 -> zero-padded 8 digits
    let buf8: [8]u8 = undefined;
    const code_t59 = formatCode(94287082, 8, &buf8);
    expect_eq(constantTimeEq(code_t59, "94287082"), true);

    // Check at unix_sec=89 (step 2) with window=1 -> drift should be -1
    const drift_neg = verifyTotp(secret, 89, code_t59, 30, 8, 1, HashAlg.Sha1);
    expect_eq(drift_neg != null, true);
    expect_eq(drift_neg.?, -1);

    // Check at unix_sec=30 (step 1) with window=1 -> drift should be 0
    const drift_zero = verifyTotp(secret, 30, code_t59, 30, 8, 1, HashAlg.Sha1);
    expect_eq(drift_zero != null, true);
    expect_eq(drift_zero.?, 0);

    // Check at unix_sec=120 (step 4) with window=1 -> miss
    const drift_miss = verifyTotp(secret, 120, code_t59, 30, 8, 1, HashAlg.Sha1);
    expect_eq(drift_miss == null, true);

    // Constant time eq checks
    expect_eq(constantTimeEq("123456", "123456"), true);
    expect_eq(constantTimeEq("123456", "123457"), false);
    expect_eq(constantTimeEq("123456", "12345"), false);
}

test "RFC 4648 Base32 encode/decode and otpauth URI build/parse with zero leaks" {
    const alloc = std.testing.allocator;

    const enc = base32Encode(alloc, "12345678901234567890", true).?;
    defer alloc.free(enc);
    expect_eq(std.mem.eql(u8, enc, "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"), true);

    // Case-insensitive + whitespace + padding
    const dec_bytes = base32Decode(alloc, "gezd gnbv gy3t qojq gezd gnbv gy3t qojq====").?;
    defer alloc.free(dec_bytes);
    expect_eq(std.mem.eql(u8, dec_bytes, "12345678901234567890"), true);

    const uri_in = OtpAuthUri{
        .otp_type = OtpType.Totp,
        .label = "Acme:alice@example.com",
        .issuer = "Acme",
        .account = "alice@example.com",
        .secret = enc,
        .algorithm = HashAlg.Sha256,
        .digits = 6,
        .period = 30,
        .counter = 0,
    };

    const built = buildOtpAuthUri(alloc, uri_in).?;
    defer alloc.free(built);

    let parsed = parseOtpAuthUri(alloc, built).?;
    defer parsed.deinit(alloc);

    expect_eq(parsed.otp_type == OtpType.Totp, true);
    expect_eq(std.mem.eql(u8, parsed.issuer, "Acme"), true);
    expect_eq(std.mem.eql(u8, parsed.account, "alice@example.com"), true);
    expect_eq(std.mem.eql(u8, parsed.secret, enc), true);
    expect_eq(parsed.algorithm == HashAlg.Sha256, true);
    expect_eq(parsed.digits, 6);
    expect_eq(parsed.period, 30);
}

test "cap.totp_hotp dynamic mount and std.testing.allocator zero leak check" {
    const alloc = std.testing.allocator;
    const secret = "12345678901234567890";

    const h0 = cap.auth.hotpAt(secret, 0, 6);
    expect_eq(h0, 755224);

    const t59 = cap.auth.totpAt(secret, 59, 30, 8, cap.auth.HashAlg.Sha1);
    expect_eq(t59, 94287082);

    const b32 = cap.auth.base32Encode(alloc, "foobar", true).?;
    defer alloc.free(b32);
    expect_eq(std.mem.eql(u8, b32, "MZXW6YTBOI======"), true);

    const raw = cap.auth.base32Decode(alloc, b32).?;
    defer alloc.free(raw);
    expect_eq(std.mem.eql(u8, raw, "foobar"), true);
}
