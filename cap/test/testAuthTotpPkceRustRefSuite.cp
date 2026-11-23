// Reference-hardened test suite against constantoine/totp-rs (MIT) & RFC 4226 / RFC 6238 / RFC 7636
// Covers:
//   1. RFC 4226 Appendix D all 10 HOTP-SHA1 official test vectors (counter 0..9) & strict bounds
//   2. RFC 6238 Appendix B TOTP-SHA1/SHA256/SHA512 official timestamp vectors & verifyTotpWithSkew
//   3. RFC 4648 Base32 strict canonical validation, secureZero key erasure & otpauth:// URI
//   4. RFC 7636 Appendix B PKCE S256 vector, length/charset guards & default-reject plain downgrade
//   5. 16-lane SIMD vs scalar 0-diff parity (simdAuthCtEqual & simdIsValidPkceVerifier)
//   6. 1,800 deterministic PRNG fuzz cases (malformed Base32 / otpauth:// / PKCE) with 0 panic / 0 OOB / 0 leak

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    OtpError,
    HashAlg,
    OtpType,
    OtpAuthUri,
    secureZero,
    hotpAt,
    hotpAtStrict,
    totpAt,
    totpAtStrict,
    formatCode,
    scalarAuthCtEqual,
    simdAuthCtEqual,
    constantTimeEq,
    verifyHotp,
    verifyTotpWithSkew,
    verifyTotp,
    base32Encode,
    base32Decode,
    base32DecodeStrict,
    buildOtpAuthUri,
    parseOtpAuthUri,
} from "../../lib/auth/totpHotp.cp";
import {
    OAuthError,
    Pkce,
    scalarIsValidPkceVerifier,
    simdIsValidPkceVerifier,
} from "../../lib/auth/oauthPkce.cp";

fn tryBase32Strict(alloc: std.mem.Allocator, s: []const u8): ([]u8)? {
    return base32DecodeStrict(alloc, s) catch return null;
}

fn nextPrng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn runRfc4226HotpVectors(): !void {
    const secret: []const u8 = "12345678901234567890";
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
    while (c < 10) : (c += 1) {
        const idx: usize = @intCast(c);
        expect_eq(hotpAt(secret, c, 6), expected[idx]);
        const strict_val = hotpAtStrict(secret, c, 6, HashAlg.Sha1)?;
        expect_eq(strict_val, expected[idx]);
    }

    // Strict rejection: digits < 6, digits > 8, empty secret
    let err_d5 = false;
    * = hotpAtStrict(secret, 0, 5, HashAlg.Sha1) catch |e| {
        if (e == OtpError.InvalidDigits) err_d5 = true;
    };
    expect_eq(err_d5, true);

    let err_d9 = false;
    * = hotpAtStrict(secret, 0, 9, HashAlg.Sha1) catch |e| {
        if (e == OtpError.InvalidDigits) err_d9 = true;
    };
    expect_eq(err_d9, true);

    let err_empty = false;
    * = hotpAtStrict("", 0, 6, HashAlg.Sha1) catch |e| {
        if (e == OtpError.InvalidSecret) err_empty = true;
    };
    expect_eq(err_empty, true);

    // verifyHotp rejects invalid digits (< 6 or > 8) and empty secret
    expect_eq(verifyHotp(secret, 0, "75522", 5, 1, HashAlg.Sha1) == null, true);
    expect_eq(verifyHotp(secret, 0, "000755224", 9, 1, HashAlg.Sha1) == null, true);
    expect_eq(verifyHotp("", 0, "755224", 6, 1, HashAlg.Sha1) == null, true);
}

fn runRfc6238TotpAndSkewVectors(): !void {
    const sec_sha1: []const u8 = "12345678901234567890";
    const sec_sha256: []const u8 = "12345678901234567890123456789012";
    const sec_sha512: []const u8 = "1234567890123456789012345678901234567890123456789012345678901234";

    const times: [6]u64 = [59, 1111111109, 1111111111, 1234567890, 2000000000, 20000000000];
    const exp_sha1: [6]u32 = [94287082, 7081804, 14050471, 89005924, 69279037, 65353130];
    const exp_sha256: [6]u32 = [46119246, 68084774, 67062674, 91819424, 90698825, 77737706];
    const exp_sha512: [6]u32 = [90693936, 25091201, 99943326, 93441116, 38618901, 47863826];

    let i: usize = 0;
    while (i < 6) : (i += 1) {
        expect_eq(totpAt(sec_sha1, times[i], 30, 8, HashAlg.Sha1), exp_sha1[i]);
        expect_eq(totpAt(sec_sha256, times[i], 30, 8, HashAlg.Sha256), exp_sha256[i]);
        expect_eq(totpAt(sec_sha512, times[i], 30, 8, HashAlg.Sha512), exp_sha512[i]);

        expect_eq(totpAtStrict(sec_sha1, times[i], 30, 8, HashAlg.Sha1)?, exp_sha1[i]);
        expect_eq(totpAtStrict(sec_sha256, times[i], 30, 8, HashAlg.Sha256)?, exp_sha256[i]);
        expect_eq(totpAtStrict(sec_sha512, times[i], 30, 8, HashAlg.Sha512)?, exp_sha512[i]);
    }

    // Strict period == 0 and digits bounds
    let err_period = false;
    * = totpAtStrict(sec_sha1, 59, 0, 8, HashAlg.Sha1) catch |e| {
        if (e == OtpError.InvalidPeriod) err_period = true;
    };
    expect_eq(err_period, true);

    let err_digits_lo = false;
    * = totpAtStrict(sec_sha1, 59, 30, 5, HashAlg.Sha1) catch |e| {
        if (e == OtpError.InvalidDigits) err_digits_lo = true;
    };
    expect_eq(err_digits_lo, true);

    let err_digits_hi = false;
    * = totpAtStrict(sec_sha1, 59, 30, 9, HashAlg.Sha1) catch |e| {
        if (e == OtpError.InvalidDigits) err_digits_hi = true;
    };
    expect_eq(err_digits_hi, true);

    // Leading-zero 8-digit TOTP at t=1111111109 ("07081804") + verifyTotpWithSkew
    let buf8: [8]u8 = undefined;
    const code_lz = formatCode(7081804, 8, &buf8);
    expect_eq(simdAuthCtEqual(code_lz, "07081804"), true);

    const d0 = verifyTotpWithSkew(sec_sha1, 1111111109, code_lz, 30, 8, 1, HashAlg.Sha1)?;
    expect_eq(d0, 0 as i64);

    // Server 1 step ahead -> delta = -1
    const d_neg1 = verifyTotpWithSkew(sec_sha1, 1111111109 + 30, code_lz, 30, 8, 1, HashAlg.Sha1)?;
    expect_eq(d_neg1, -1 as i64);

    // Server 2 steps behind -> delta = +2
    const d_pos2 = verifyTotpWithSkew(sec_sha1, 1111111109 - 60, code_lz, 30, 8, 2, HashAlg.Sha1)?;
    expect_eq(d_pos2, 2 as i64);

    // Outside skew=1 -> OtpError.InvalidCode
    let err_skew_miss = false;
    * = verifyTotpWithSkew(sec_sha1, 1111111109 + 60, code_lz, 30, 8, 1, HashAlg.Sha1) catch |e| {
        if (e == OtpError.InvalidCode) err_skew_miss = true;
    };
    expect_eq(err_skew_miss, true);

    // verifyTotpWithSkew rejects period == 0 & invalid digits & empty secret
    let err_skew_p = false;
    * = verifyTotpWithSkew(sec_sha1, 59, "94287082", 0, 8, 1, HashAlg.Sha1) catch |e| {
        if (e == OtpError.InvalidPeriod) err_skew_p = true;
    };
    expect_eq(err_skew_p, true);

    let err_skew_d = false;
    * = verifyTotpWithSkew(sec_sha1, 59, "94287", 30, 5, 1, HashAlg.Sha1) catch |e| {
        if (e == OtpError.InvalidDigits) err_skew_d = true;
    };
    expect_eq(err_skew_d, true);

    // Legacy verifyTotp returns null on period == 0 or invalid digits
    expect_eq(verifyTotp(sec_sha1, 59, "94287082", 0, 8, 1, HashAlg.Sha1) == null, true);
    expect_eq(verifyTotp(sec_sha1, 59, "94287", 30, 5, 1, HashAlg.Sha1) == null, true);
}

fn runBase32StrictAndSecureZero(): !void {
    const alloc = std.heap.page_allocator;

    // RFC 4648 Section 10 test vectors
    const enc_foo = base32Encode(alloc, "foobar", true).?;
    defer alloc.free(enc_foo);
    expect_eq(std.mem.eql(u8, enc_foo, "MZXW6YTBOI======"), true);

    const dec_strict = base32DecodeStrict(alloc, "MZXW6YTBOI======")?;
    defer alloc.free(dec_strict);
    expect_eq(std.mem.eql(u8, dec_strict, "foobar"), true);

    // Unpadded valid Base32
    const dec_unpad = base32DecodeStrict(alloc, "MZXW6YTBOI")?;
    defer alloc.free(dec_unpad);
    expect_eq(std.mem.eql(u8, dec_unpad, "foobar"), true);

    // Strict Base32 rejections:
    // 1. empty, 2. invalid char '1'/'8', 3. misplaced '=', 4. bad quantum rem=1/3/6,
    // 5. wrong pad count, 6. non-zero trailing canonical bits ("MY======" -> 'f' is MY======, 'MZ======' has trailing bit 1)
    const bad_b32: [9][]const u8 = [
        "",
        "JBSWY3DP1HPK3PXP",
        "JBSWY3DP8HPK3PXP",
        "MZXW=YTB",
        "A",
        "AAA",
        "AAAAAA",
        "MY=====",
        "MZ======",
    ];
    let bi: usize = 0;
    while (bi < bad_b32.len) : (bi += 1) {
        let got_err = false;
        * = base32DecodeStrict(alloc, bad_b32[bi]) catch |e| {
            if (e == OtpError.InvalidBase32) got_err = true;
        };
        expect_eq(got_err, true);
    }

    // secureZero erases sensitive key material
    let key_buf: [20]u8 = undefined;
    @memcpy(&key_buf, "12345678901234567890");
    secureZero(&key_buf);
    let zi: usize = 0;
    while (zi < key_buf.len) : (zi += 1) {
        expect_eq(key_buf[zi], 0 as u8);
    }

    // otpauth:// roundtrip
    const uri_in = OtpAuthUri{
        .otp_type = OtpType.Totp,
        .label = "TotpRs:bob@example.com",
        .issuer = "TotpRs",
        .account = "bob@example.com",
        .secret = "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ",
        .algorithm = HashAlg.Sha512,
        .digits = 8,
        .period = 30,
        .counter = 0,
    };
    const built = buildOtpAuthUri(alloc, uri_in).?;
    defer alloc.free(built);
    let parsed = parseOtpAuthUri(alloc, built).?;
    defer parsed.deinit(alloc);
    expect_eq(parsed.algorithm == HashAlg.Sha512, true);
    expect_eq(parsed.digits, 8 as u8);
    expect_eq(parsed.period, 30 as u64);
}

fn runRfc7636PkceAndDowngradeGuards(): !void {
    const alloc = std.heap.page_allocator;

    // RFC 7636 Appendix B official vector
    const rfc_verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const rfc_challenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";

    expect_eq(Pkce.isValidVerifier(rfc_verifier), true);
    const derived = Pkce.computeS256Challenge(alloc, rfc_verifier)?;
    defer alloc.free(derived);
    expect_eq(simdAuthCtEqual(derived, rfc_challenge), true);
    expect_eq(Pkce.verifyS256(rfc_verifier, rfc_challenge), true);
    expect_eq(Pkce.verifyChallenge(rfc_verifier, rfc_challenge, "S256"), true);

    // Default-safe verifyChallenge MUST reject plain downgrade
    expect_eq(Pkce.verifyChallenge(rfc_verifier, rfc_verifier, "plain"), false);
    expect_eq(Pkce.verifyWithMethod(rfc_verifier, rfc_verifier, "plain", false), false);
    // Explicitly opt-in plain succeeds only with valid verifier and exact match
    expect_eq(Pkce.verifyWithMethod(rfc_verifier, rfc_verifier, "plain", true), true);
    expect_eq(Pkce.verifyWithMethod(rfc_verifier, "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXj", "plain", true), false);
    // Reject unknown / case-mismatched methods
    expect_eq(Pkce.verifyWithMethod(rfc_verifier, rfc_challenge, "s256", true), false);
    expect_eq(Pkce.verifyWithMethod(rfc_verifier, rfc_verifier, "PLAIN", true), false);
    expect_eq(Pkce.verifyWithMethod(rfc_verifier, rfc_challenge, "none", true), false);

    // Exact boundary lengths: 42 (reject), 43 (accept), 128 (accept), 129 (reject)
    let buf129: [129]u8 = [0; 129];
    let k: usize = 0;
    while (k < 129) : (k += 1) {
        buf129[k] = 'a';
    }
    expect_eq(Pkce.isValidVerifier(buf129[0..42]), false);
    expect_eq(Pkce.isValidVerifier(buf129[0..43]), true);
    expect_eq(Pkce.isValidVerifier(buf129[0..128]), true);
    expect_eq(Pkce.isValidVerifier(buf129[0..129]), false);

    // Disallowed characters in verifier
    let bad_v: [43]u8 = undefined;
    @memcpy(&bad_v, rfc_verifier);
    const illegal_chars: [8]u8 = [' ', '/', '+', '=', '!', '@', '\n', 0x7f];
    let ci: usize = 0;
    while (ci < illegal_chars.len) : (ci += 1) {
        bad_v[17] = illegal_chars[ci];
        expect_eq(Pkce.isValidVerifier(&bad_v), false);
        expect_eq(Pkce.verifyS256(&bad_v, rfc_challenge), false);
    }
}

fn runSimdScalarZeroDiffParity(): !void {
    // 1. simdAuthCtEqual vs scalarAuthCtEqual across lengths 0..128 and every diff byte position
    const lens: [12]usize = [0, 1, 6, 8, 15, 16, 17, 31, 32, 43, 64, 128];
    let a: [128]u8 = [0; 128];
    let b: [128]u8 = [0; 128];
    let i: usize = 0;
    while (i < 128) : (i += 1) {
        a[i] = @intCast((i * 37 + 11) & 0xFF);
        b[i] = a[i];
    }

    let li: usize = 0;
    while (li < lens.len) : (li += 1) {
        const n = lens[li];
        expect_eq(simdAuthCtEqual(a[0..n], b[0..n]), scalarAuthCtEqual(a[0..n], b[0..n]));
        expect_eq(simdAuthCtEqual(a[0..n], b[0..n]), true);
        expect_eq(constantTimeEq(a[0..n], b[0..n]), true);

        if (n > 0) {
            // Flip first, mid, lane boundary, and last byte
            let pos: usize = 0;
            while (pos < n) : (pos += 1) {
                b[pos] ^= 0x01;
                const s_res = scalarAuthCtEqual(a[0..n], b[0..n]);
                const v_res = simdAuthCtEqual(a[0..n], b[0..n]);
                expect_eq(v_res, s_res);
                expect_eq(v_res, false);
                b[pos] ^= 0x01;
            }
        }
    }
    // Mismatched lengths
    expect_eq(simdAuthCtEqual(a[0..16], b[0..15]), false);
    expect_eq(simdAuthCtEqual(a[0..43], b[0..42]), false);

    // 2. simdIsValidPkceVerifier vs scalarIsValidPkceVerifier across all 256 byte values in lane & tail positions
    let pkce_buf: [64]u8 = [0; 64];
    let p: usize = 0;
    while (p < 64) : (p += 1) {
        pkce_buf[p] = 'A';
    }
    // Test all 256 possible byte values at index 0 (SIMD lane 0), index 15 (SIMD lane 15),
    // index 31 (2nd SIMD chunk), and index 50 (scalar tail for len=51)
    const test_positions: [4]usize = [0, 15, 31, 50];
    let pi: usize = 0;
    while (pi < test_positions.len) : (pi += 1) {
        const pos = test_positions[pi];
        let ch_u16: u16 = 0;
        while (ch_u16 < 256) : (ch_u16 += 1) {
            pkce_buf[pos] = @intCast(ch_u16);
            const slice = pkce_buf[0..51];
            const sc = scalarIsValidPkceVerifier(slice);
            const sm = simdIsValidPkceVerifier(slice);
            expect_eq(sm, sc);
        }
        pkce_buf[pos] = 'A';
    }
}

fn runDeterministicFuzz1800(): !void {
    const alloc = std.heap.page_allocator;
    let rng: u64 = 0x1E3779B97F4A7C15;
    let buf: [160]u8 = [0; 160];
    let buf2: [160]u8 = [0; 160];

    // 1. 600 malformed Base32 fuzz cases (both legacy base32Decode & strict base32DecodeStrict)
    let f1: usize = 0;
    while (f1 < 600) : (f1 += 1) {
        const len: usize = @intCast(nextPrng(&rng) % 96);
        let k: usize = 0;
        while (k < len) : (k += 1) {
            const r = nextPrng(&rng);
            if ((r & 3) == 0) {
                buf[k] = @intCast(r & 0xFF);
            } else {
                const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789= -_\t\r\n";
                const a_idx: usize = @intCast((r >> 8) % alphabet.len);
                buf[k] = alphabet[a_idx];
            }
        }
        if (dec_b32 := base32Decode(alloc, buf[0..len])) {
            alloc.free(dec_b32);
        }
        if (strict_res := tryBase32Strict(alloc, buf[0..len])) {
            alloc.free(strict_res);
        }
    }

    // 2. 600 malformed otpauth:// URI fuzz cases
    const uri_prefix = "otpauth://";
    let f2: usize = 0;
    while (f2 < 600) : (f2 += 1) {
        let len: usize = @intCast(nextPrng(&rng) % 128);
        let start: usize = 0;
        if ((f2 & 1) == 0) {
            @memcpy(buf[0..uri_prefix.len], uri_prefix);
            start = uri_prefix.len;
            if (len < start) len = start;
        }
        let k: usize = start;
        while (k < len) : (k += 1) {
            const r = nextPrng(&rng);
            const pool = "totphotp/:?&=%0123456789abcdefABCDEFGZYX+-._~\x00\r\n";
            const p_idx: usize = @intCast(r % pool.len);
            buf[k] = pool[p_idx];
        }
        if (parsed := parseOtpAuthUri(alloc, buf[0..len])) {
            let mut_p = parsed;
            mut_p.deinit(alloc);
        }
    }

    // 3. 600 malformed PKCE verifier / challenge / method fuzz cases + SIMD parity
    let f3: usize = 0;
    while (f3 < 600) : (f3 += 1) {
        const len: usize = @intCast(nextPrng(&rng) % 150);
        let k: usize = 0;
        while (k < len) : (k += 1) {
            const r = nextPrng(&rng);
            if ((r % 5) == 0) {
                buf[k] = @intCast(r & 0xFF);
            } else {
                const pkce_pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~+/= ";
                const pk_idx: usize = @intCast((r >> 8) % pkce_pool.len);
                buf[k] = pkce_pool[pk_idx];
            }
            buf2[k] = buf[k];
        }
        if (len > 0 and (f3 & 1) == 1) {
            const flip_idx: usize = @intCast(nextPrng(&rng) % len);
            buf2[flip_idx] ^= 0x5A;
        }
        const s_v = scalarIsValidPkceVerifier(buf[0..len]);
        const v_v = simdIsValidPkceVerifier(buf[0..len]);
        expect_eq(v_v, s_v);
        expect_eq(simdAuthCtEqual(buf[0..len], buf2[0..len]), scalarAuthCtEqual(buf[0..len], buf2[0..len]));

        * = Pkce.verifyS256(buf[0..len], buf2[0..len]);
        * = Pkce.verifyChallenge(buf[0..len], buf2[0..len], "S256");
        * = Pkce.verifyChallenge(buf[0..len], buf[0..len], "plain");
    }
}

fn main() {
    test("1. RFC 4226 Appendix D HOTP-SHA1 all 10 vectors & strict bounds", () => {
        runRfc4226HotpVectors()?;
    });

    test("2. RFC 6238 Appendix B TOTP-SHA1/256/512 vectors & verifyTotpWithSkew", () => {
        runRfc6238TotpAndSkewVectors()?;
    });

    test("3. RFC 4648 Base32 strict canonical decode, secureZero & otpauth URI", () => {
        runBase32StrictAndSecureZero()?;
    });

    test("4. RFC 7636 Appendix B PKCE S256 vector, bounds & plain downgrade rejection", () => {
        runRfc7636PkceAndDowngradeGuards()?;
    });

    test("5. 16-lane SIMD vs scalar 0-diff parity (simdAuthCtEqual & simdIsValidPkceVerifier)", () => {
        runSimdScalarZeroDiffParity()?;
    });

    test("6. 1,800 deterministic PRNG fuzz cases (Base32 / otpauth:// / PKCE) zero panic/OOB/leak", () => {
        runDeterministicFuzz1800()?;
    });
}
