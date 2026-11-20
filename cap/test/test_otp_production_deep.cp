// Source: cp/cap/test/test_otp_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:otp & cap.otp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    OtpError,
    OtpAlgorithm,
    TotpVerifyResult,
    OtpAuthUri,
    base32Encode,
    base32Decode,
    hotp,
    hotpString,
    totp,
    totpString,
    verifyTotp,
    formatOtpAuthUri,
    parseOtpAuthUri,
} from "cap:otp";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn emptyVerifyResult(): TotpVerifyResult {
    return TotpVerifyResult{};
}

fn emptyAuthUri(): OtpAuthUri {
    return OtpAuthUri{};
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    const secret: []const u8 = "12345678901234567890";

    const expected_nums: [10]u32 = [
        755224, 287082, 359152, 969429, 338314,
        254676, 287922, 162583, 399871, 520489,
    ];
    const expected_strs: [10][]const u8 = [
        "755224", "287082", "359152", "969429", "338314",
        "254676", "287922", "162583", "399871", "520489",
    ];

    let c: u64 = 0;
    while (c < 10) : (c += 1) {
        const code_num = hotp(secret, c, 6, OtpAlgorithm.Sha1)?;
        expect_eq(code_num, expected_nums[c as usize]);

        const code_str = hotpString(alloc, secret, c, 6, OtpAlgorithm.Sha1)?;
        defer alloc.free(code_str);
        expect_eq(std.mem.eql(u8, code_str, expected_strs[c as usize]), true);
    }
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    const sec_sha1: []const u8 = "12345678901234567890";
    const sec_sha256: []const u8 = "12345678901234567890123456789012";
    const sec_sha512: []const u8 = "1234567890123456789012345678901234567890123456789012345678901234";

    const times: [6]u64 = [59, 1111111109, 1111111111, 1234567890, 2000000000, 20000000000];
    const exp_sha1: [6][]const u8 = [
        "94287082", "07081804", "14050471", "89005924", "69279037", "65353130",
    ];
    const exp_sha256: [6][]const u8 = [
        "46119246", "68084774", "67062674", "91819424", "90698825", "77737706",
    ];
    const exp_sha512: [6][]const u8 = [
        "90693936", "25091201", "99943326", "93441116", "38618901", "47863826",
    ];

    let i: usize = 0;
    while (i < 6) : (i += 1) {
        const s1 = totpString(alloc, sec_sha1, times[i], 30, 8, OtpAlgorithm.Sha1)?;
        defer alloc.free(s1);
        expect_eq(std.mem.eql(u8, s1, exp_sha1[i]), true);

        const s256 = totpString(alloc, sec_sha256, times[i], 30, 8, OtpAlgorithm.Sha256)?;
        defer alloc.free(s256);
        expect_eq(std.mem.eql(u8, s256, exp_sha256[i]), true);

        const s512 = totpString(alloc, sec_sha512, times[i], 30, 8, OtpAlgorithm.Sha512)?;
        defer alloc.free(s512);
        expect_eq(std.mem.eql(u8, s512, exp_sha512[i]), true);
    }

    // Verify integer totp() matches leading-zero vector at t=1111111109
    const int_lz = totp(sec_sha1, 1111111109, 30, 8, OtpAlgorithm.Sha1)?;
    expect_eq(int_lz, 7081804 as u32);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;
    const sec_sha1: []const u8 = "12345678901234567890";

    // t=1111111109 -> step = 37037036, 8-digit code = "07081804" (leading zero preserved!)
    const target_time: u64 = 1111111109;
    const expected_step: u64 = target_time / 30;
    const code = totpString(alloc, sec_sha1, target_time, 30, 8, OtpAlgorithm.Sha1)?;
    defer alloc.free(code);
    expect_eq(std.mem.eql(u8, code, "07081804"), true);

    // Exact match (delta = 0)
    const r0 = verifyTotp(sec_sha1, code, target_time, 30, 8, OtpAlgorithm.Sha1, 1)?;
    expect_eq(r0.valid, true);
    expect_eq(r0.matched_step, expected_step);
    expect_eq(r0.delta, 0 as i64);

    // Client behind by 1 step -> server is at +1 step, matched_step is target_step (delta = -1)
    const r_minus1 = verifyTotp(sec_sha1, code, target_time + 30, 30, 8, OtpAlgorithm.Sha1, 1)?;
    expect_eq(r_minus1.valid, true);
    expect_eq(r_minus1.matched_step, expected_step);
    expect_eq(r_minus1.delta, -1 as i64);

    // Client ahead by 2 steps -> server is at -2 steps, matched_step is target_step (delta = +2)
    const r_plus2 = verifyTotp(sec_sha1, code, target_time - 60, 30, 8, OtpAlgorithm.Sha1, 2)?;
    expect_eq(r_plus2.valid, true);
    expect_eq(r_plus2.matched_step, expected_step);
    expect_eq(r_plus2.delta, 2 as i64);

    // Outside ±1 skew window -> rejected
    const r_out = verifyTotp(sec_sha1, code, target_time + 60, 30, 8, OtpAlgorithm.Sha1, 1)?;
    expect_eq(r_out.valid, false);
    expect_eq(r_out.matched_step, 0 as u64);

    // Wrong code of same length -> rejected
    const r_wrong = verifyTotp(sec_sha1, "00000000", target_time, 30, 8, OtpAlgorithm.Sha1, 2)?;
    expect_eq(r_wrong.valid, false);

    // Malformed code -> InvalidCode
    let bad_code = false;
    * = verifyTotp(sec_sha1, "0708180A", target_time, 30, 8, OtpAlgorithm.Sha1, 1) catch e => errC: {
        bad_code = (e == OtpError.InvalidCode);
        break :errC emptyVerifyResult();
    };
    expect_eq(bad_code, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // RFC 4648 Base32 test vectors
    const b32_foobar = base32Encode(alloc, "foobar")?;
    defer alloc.free(b32_foobar);
    expect_eq(std.mem.eql(u8, b32_foobar, "MZXW6YTBOI======"), true);

    // Case-insensitive + spaces/hyphens ignored
    const dec_foobar = base32Decode(alloc, "mzxw-6ytb oi======")?;
    defer alloc.free(dec_foobar);
    expect_eq(std.mem.eql(u8, dec_foobar, "foobar"), true);

    // Unpadded Base32 secret
    const dec_unpadded = base32Decode(alloc, "JBSWY3DPEHPK3PXP")?;
    defer alloc.free(dec_unpadded);
    expect_eq(std.mem.eql(u8, dec_unpadded, "Hello!\xde\xad\xbe\xef"), true);

    // Invalid Base32 character ('1' or '8') -> InvalidSecret
    let bad_b32 = false;
    * = base32Decode(alloc, "JBSWY3DP1HPK3PXP") catch e => errB: {
        bad_b32 = (e == OtpError.InvalidSecret);
        break :errB emptyMutSlice();
    };
    expect_eq(bad_b32, true);

    // Invalid Base32 padding -> InvalidSecret
    let bad_pad = false;
    * = base32Decode(alloc, "MZXW6=TB") catch e => errP: {
        bad_pad = (e == OtpError.InvalidSecret);
        break :errP emptyMutSlice();
    };
    expect_eq(bad_pad, true);

    // Invalid digits -> InvalidDigits
    let bad_digits = false;
    * = totpString(alloc, "12345678901234567890", 59, 30, 9, OtpAlgorithm.Sha1) catch e => errD: {
        bad_digits = (e == OtpError.InvalidDigits);
        break :errD emptyMutSlice();
    };
    expect_eq(bad_digits, true);

    // otpauth://totp/... format & parse roundtrip
    const uri_in = OtpAuthUri{
        is_totp: true,
        label: "Acme Corp:alice@example.com",
        issuer: "Acme Corp",
        secret_b32: "JBSWY3DPEHPK3PXP",
        algo: OtpAlgorithm.Sha256,
        digits: 8,
        period: 60,
        counter: 0,
    };
    const uri_str = formatOtpAuthUri(alloc, uri_in)?;
    defer alloc.free(uri_str);

    let parsed = parseOtpAuthUri(alloc, uri_str)?;
    defer parsed.deinit(alloc);
    expect_eq(parsed.is_totp, true);
    expect_eq(std.mem.eql(u8, parsed.label, "Acme Corp:alice@example.com"), true);
    expect_eq(std.mem.eql(u8, parsed.issuer, "Acme Corp"), true);
    expect_eq(std.mem.eql(u8, parsed.secret_b32, "JBSWY3DPEHPK3PXP"), true);
    expect_eq(parsed.algo == OtpAlgorithm.Sha256, true);
    expect_eq(parsed.digits, 8 as u8);
    expect_eq(parsed.period, 60 as u64);

    // Invalid URI scheme -> InvalidUri
    let bad_uri = false;
    * = parseOtpAuthUri(alloc, "https://example.com/totp/a?secret=JBSWY3DPEHPK3PXP") catch e => errU: {
        bad_uri = (e == OtpError.InvalidUri);
        break :errU emptyAuthUri();
    };
    expect_eq(bad_uri, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    const raw_secret = cap.otp.base32Decode(alloc, "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ")?;
    defer alloc.free(raw_secret);
    expect_eq(std.mem.eql(u8, raw_secret, "12345678901234567890"), true);

    const h0 = cap.otp.hotp(raw_secret, 0, 6, cap.otp.OtpAlgorithm.Sha1)?;
    expect_eq(h0, 755224 as u32);

    const t_str = cap.otp.totpString(alloc, raw_secret, 1111111109, 30, 8, cap.otp.OtpAlgorithm.Sha1)?;
    defer alloc.free(t_str);
    expect_eq(std.mem.eql(u8, t_str, "07081804"), true);

    const v = cap.otp.verifyTotp(raw_secret, t_str, 1111111109, 30, 8, cap.otp.OtpAlgorithm.Sha1, 1)?;
    expect_eq(v.valid, true);
    expect_eq(v.delta, 0 as i64);

    let u = cap.otp.parseOtpAuthUri(alloc, "otpauth://hotp/Test:bob?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&counter=7&digits=6")?;
    defer u.deinit(alloc);
    expect_eq(u.is_totp, false);
    expect_eq(u.counter, 7 as u64);
    expect_eq(u.digits, 6 as u8);
}

test("1. RFC 4226 Appendix D official HOTP vectors counter=0..9", () => {
    runTest1() catch unreachable;
});

test("2. RFC 6238 Appendix B official SHA1/SHA256/SHA512 8-digit TOTP vectors", () => {
    runTest2() catch unreachable;
});

test("3. verifyTotp +-1/+-2 skew window, matched_step, out-of-window rejection & leading zero", () => {
    runTest3() catch unreachable;
});

test("4. RFC 4648 Base32 roundtrip, otpauth:// URI format/parse & error validation", () => {
    runTest4() catch unreachable;
});

test("5. import cap from \"cap\"; cap.otp dynamic mount and std.testing.allocator zero leak", () => {
    runTest5() catch unreachable;
});
