// Wave 32 R3: JWT / JWS Rust Reference Hardening Suite (Keats/jsonwebtoken MIT)
import { test, expect_eq } from "cap:test";
import {
    JwtAlgorithm,
    JwtError,
    JwtValidation,
    JwtVerified,
    JwtParts,
    JwtHeader,
    JwtClaims,
    ClaimStatus,
    jwtCtEqualScalar,
    jwtCtEqualSimd,
    jwtFindDotScalar,
    jwtFindDotSimd,
    jwtIsValidBase64UrlScalar,
    jwtIsValidBase64UrlSimd,
    encodeBase64Url,
    decodeBase64UrlStrict,
    splitJwtStrict,
    signJwtHmac,
    verifyJwtHmac,
} from "../../lib/jwt/claims.cp";

fn check(cond: bool): void! {
    if (!cond) return error.TestFailed;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn checkRoundtrip(alg: JwtAlgorithm, hdr: []const u8, payload: []const u8, secret: []const u8): void! {
    let tok_buf: [1024]u8 = undefined;
    let tok = signJwtHmac(alg, hdr, payload, secret, &tok_buf) catch {
        return error.SignFailed;
    };

    let cfg = JwtValidation{
        expected_alg: alg,
        allow_none_alg: false,
        leeway_sec: 60,
        validate_exp: true,
        validate_nbf: true,
        expected_iss: "cp-auth",
        expected_sub: "user-42",
        expected_aud: "api",
    };

    let h_buf: [256]u8 = undefined;
    let p_buf: [512]u8 = undefined;
    let verified = verifyJwtHmac(tok, secret, 1500, cfg, &h_buf, &p_buf) catch {
        return error.VerifyFailed;
    };
    check((verified.claims.exp orelse 0) == 2000)?;
    check((verified.claims.nbf orelse 0) == 1000)?;
    check(bytesEq(verified.claims.sub orelse "", "user-42"))?;
    check(bytesEq(verified.claims.iss orelse "", "cp-auth"))?;
    check(bytesEq(verified.claims.aud orelse "", "api"))?;
}

fn runTest1(): void! {
    // 1. HS256 / HS384 / HS512 sign + verify roundtrip
    let secret = "super-secret-jwt-key-2026";
    let payload = "{\"sub\":\"user-42\",\"iss\":\"cp-auth\",\"aud\":\"api\",\"exp\":2000,\"nbf\":1000}";

    checkRoundtrip(JwtAlgorithm.HS256, "{\"alg\":\"HS256\",\"typ\":\"JWT\"}", payload, secret)?;
    checkRoundtrip(JwtAlgorithm.HS384, "{\"alg\":\"HS384\",\"typ\":\"JWT\"}", payload, secret)?;
    checkRoundtrip(JwtAlgorithm.HS512, "{\"alg\":\"HS512\",\"typ\":\"JWT\"}", payload, secret)?;

    // 2. RFC 7515 Appendix A.1 HS256 test vector
    let rfc_tok = "eyJ0eXAiOiJKV1QiLA0KICJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGFtcGxlLmNvbS9pc19yb290Ijp0cnVlfQ.dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    let rfc_key_b64 = "AyM1SysPpbyDfgZld3umj1qzKObwVMkoqQ-EstJQLr_T-1qS0gZH75aKtMN3Yj0iPS4hcgUuTwjAzZr1Z9CAow";
    let rfc_key_buf: [128]u8 = undefined;
    let rfc_key = decodeBase64UrlStrict(rfc_key_b64, &rfc_key_buf) orelse {
        return error.RfcKeyDecodeFailed;
    };

    let rfc_cfg = JwtValidation{
        expected_alg: JwtAlgorithm.HS256,
        allow_none_alg: false,
        leeway_sec: 0,
        validate_exp: true,
        validate_nbf: false,
        expected_iss: "joe",
        expected_sub: null,
        expected_aud: null,
    };
    let rh_buf: [256]u8 = undefined;
    let rp_buf: [512]u8 = undefined;
    let rfc_res = verifyJwtHmac(rfc_tok, rfc_key, 1300819300, rfc_cfg, &rh_buf, &rp_buf) catch {
        return error.RfcVerifyFailed;
    };
    check((rfc_res.claims.exp orelse 0) == 1300819380)?;
    check(bytesEq(rfc_res.claims.iss orelse "", "joe"))?;
}

fn runTest2(): void! {
    let secret = "key-for-security-checks";
    let payload = "{\"sub\":\"alice\",\"iss\":\"auth\",\"aud\":\"svc\",\"exp\":2000,\"nbf\":1000}";

    // 1. alg: "none" attack rejected with NoneAlgorithmForbidden
    let h_none_buf: [128]u8 = undefined;
    let p_b64_buf: [256]u8 = undefined;
    let h_none_b64 = encodeBase64Url("{\"alg\":\"none\",\"typ\":\"JWT\"}", &h_none_buf) orelse {
        return error.EncodeFailed;
    };
    let p_b64 = encodeBase64Url(payload, &p_b64_buf) orelse {
        return error.EncodeFailed;
    };
    let none_tok_buf: [512]u8 = undefined;
    let pos: usize = 0;
    @memcpy(none_tok_buf[pos .. pos + h_none_b64.len], h_none_b64);
    pos += h_none_b64.len;
    none_tok_buf[pos] = '.';
    pos += 1;
    @memcpy(none_tok_buf[pos .. pos + p_b64.len], p_b64);
    pos += p_b64.len;
    none_tok_buf[pos] = '.';
    pos += 1;
    let none_tok = none_tok_buf[0..pos];

    let cfg_hs256 = JwtValidation{
        expected_alg: JwtAlgorithm.HS256,
        allow_none_alg: false,
        leeway_sec: 60,
        validate_exp: true,
        validate_nbf: true,
        expected_iss: null,
        expected_sub: null,
        expected_aud: null,
    };
    let hb: [256]u8 = undefined;
    let pb: [512]u8 = undefined;

    let got_none_err = false;
    * = verifyJwtHmac(none_tok, secret, 1500, cfg_hs256, &hb, &pb) catch |e| {
        if (e == JwtError.NoneAlgorithmForbidden) got_none_err = true;
    };
    check(got_none_err)?;

    // 2. Algorithm confusion: HS256 token verified with expected_alg = HS512 -> AlgorithmMismatch
    let tok_buf: [1024]u8 = undefined;
    let valid_tok = signJwtHmac(JwtAlgorithm.HS256, "{\"alg\":\"HS256\",\"typ\":\"JWT\"}", payload, secret, &tok_buf) catch {
        return error.SignFailed;
    };
    let cfg_hs512 = JwtValidation{
        expected_alg: JwtAlgorithm.HS512,
        allow_none_alg: false,
        leeway_sec: 60,
        validate_exp: true,
        validate_nbf: true,
        expected_iss: null,
        expected_sub: null,
        expected_aud: null,
    };
    let got_alg_err = false;
    * = verifyJwtHmac(valid_tok, secret, 1500, cfg_hs512, &hb, &pb) catch |e| {
        if (e == JwtError.AlgorithmMismatch) got_alg_err = true;
    };
    check(got_alg_err)?;

    // 3. Single-bit signature tamper -> InvalidSignature
    let tampered: [1024]u8 = undefined;
    @memcpy(tampered[0..valid_tok.len], valid_tok);
    let last = valid_tok.len - 1;
    if (tampered[last] == 'A') {
        tampered[last] = 'B';
    } else {
        tampered[last] = 'A';
    }
    let got_sig_err = false;
    * = verifyJwtHmac(tampered[0..valid_tok.len], secret, 1500, cfg_hs256, &hb, &pb) catch |e| {
        if (e == JwtError.InvalidSignature) got_sig_err = true;
    };
    check(got_sig_err)?;

    // 4. exp / nbf with leeway_sec
    // exp = 2000, leeway = 60: now=2059 valid, now=2060 expired
    * = verifyJwtHmac(valid_tok, secret, 2059, cfg_hs256, &hb, &pb) catch {
        return error.LeewayExpFailed;
    };
    let got_exp_err = false;
    * = verifyJwtHmac(valid_tok, secret, 2060, cfg_hs256, &hb, &pb) catch |e| {
        if (e == JwtError.TokenExpired) got_exp_err = true;
    };
    check(got_exp_err)?;

    // nbf = 1000, leeway = 60: now=940 valid, now=939 not yet valid
    * = verifyJwtHmac(valid_tok, secret, 940, cfg_hs256, &hb, &pb) catch {
        return error.LeewayNbfFailed;
    };
    let got_nbf_err = false;
    * = verifyJwtHmac(valid_tok, secret, 939, cfg_hs256, &hb, &pb) catch |e| {
        if (e == JwtError.TokenNotYetValid) got_nbf_err = true;
    };
    check(got_nbf_err)?;

    // 5. iss / sub / aud mismatch
    let cfg_iss = JwtValidation{
        expected_alg: JwtAlgorithm.HS256,
        allow_none_alg: false,
        leeway_sec: 60,
        validate_exp: true,
        validate_nbf: true,
        expected_iss: "wrong-iss",
        expected_sub: null,
        expected_aud: null,
    };
    let got_iss_err = false;
    * = verifyJwtHmac(valid_tok, secret, 1500, cfg_iss, &hb, &pb) catch |e| {
        if (e == JwtError.IssuerMismatch) got_iss_err = true;
    };
    check(got_iss_err)?;

    let cfg_sub = JwtValidation{
        expected_alg: JwtAlgorithm.HS256,
        allow_none_alg: false,
        leeway_sec: 60,
        validate_exp: true,
        validate_nbf: true,
        expected_iss: null,
        expected_sub: "bob",
        expected_aud: null,
    };
    let got_sub_err = false;
    * = verifyJwtHmac(valid_tok, secret, 1500, cfg_sub, &hb, &pb) catch |e| {
        if (e == JwtError.SubjectMismatch) got_sub_err = true;
    };
    check(got_sub_err)?;

    let cfg_aud = JwtValidation{
        expected_alg: JwtAlgorithm.HS256,
        allow_none_alg: false,
        leeway_sec: 60,
        validate_exp: true,
        validate_nbf: true,
        expected_iss: null,
        expected_sub: null,
        expected_aud: "other-svc",
    };
    let got_aud_err = false;
    * = verifyJwtHmac(valid_tok, secret, 1500, cfg_aud, &hb, &pb) catch |e| {
        if (e == JwtError.AudienceMismatch) got_aud_err = true;
    };
    check(got_aud_err)?;

    // 6. 2-part and 4-part malformed tokens rejected by splitJwtStrict
    check(splitJwtStrict("abc.def") == null)?;
    check(splitJwtStrict("a.b.c.d") == null)?;
    check(splitJwtStrict(".b.c") == null)?;
    check(splitJwtStrict("a..c") == null)?;

    // 7. Strict Base64URL: rejects '=', '+', '/', and non-zero canonical trailing bits
    let out_b: [64]u8 = undefined;
    check(decodeBase64UrlStrict("YQ==", &out_b) == null)?;
    check(decodeBase64UrlStrict("ab+c", &out_b) == null)?;
    check(decodeBase64UrlStrict("ab/c", &out_b) == null)?;
    check(decodeBase64UrlStrict("a", &out_b) == null)?;
    // "YR": bottom 4 bits of 'R'(17=0b010001) are non-zero, non-canonical
    check(decodeBase64UrlStrict("YR", &out_b) == null)?;
    // "YQ": 'a' (0x61), bottom 4 bits of 'Q'(16=0b010000) are 0, canonical
    let ok_dec = decodeBase64UrlStrict("YQ", &out_b) orelse {
        return error.CanonicalBase64Failed;
    };
    check(bytesEq(ok_dec, "a"))?;
}

fn runTest3(): void! {
    // SIMD vs Scalar 0-diff parity across lengths 0..64
    let a_buf: [64]u8 = undefined;
    let b_buf: [64]u8 = undefined;
    let k: usize = 0;
    while (k < 64) : (k += 1) {
        a_buf[k] = @as(u8, 'A') + @as(u8, @intCast(k % 26));
        b_buf[k] = a_buf[k];
    }

    let len: usize = 0;
    while (len <= 64) : (len += 1) {
        let sa = a_buf[0..len];
        let sb = b_buf[0..len];
        check(jwtCtEqualSimd(sa, sb) == jwtCtEqualScalar(sa, sb))?;
        check(jwtIsValidBase64UrlSimd(sa) == jwtIsValidBase64UrlScalar(sa))?;
        check(jwtFindDotSimd(sa) == jwtFindDotScalar(sa))?;

        if (len > 0) {
            let mod_buf: [64]u8 = undefined;
            @memcpy(mod_buf[0..len], sa);
            mod_buf[len - 1] = '.';
            let sm = mod_buf[0..len];
            check(jwtCtEqualSimd(sa, sm) == jwtCtEqualScalar(sa, sm))?;
            check(jwtIsValidBase64UrlSimd(sm) == jwtIsValidBase64UrlScalar(sm))?;
            check(jwtFindDotSimd(sm) == jwtFindDotScalar(sm))?;

            mod_buf[0] = '+';
            check(jwtIsValidBase64UrlSimd(mod_buf[0..len]) == jwtIsValidBase64UrlScalar(mod_buf[0..len]))?;
        }
    }
    check(!jwtCtEqualSimd("abc", "abcd"))?;
    check(jwtFindDotSimd("aaaaaaaaaaaaaaaa.bbbbbbbbbbbbbbbb") == 16)?;
    check(jwtIsValidBase64UrlSimd("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"))?;
    check(!jwtIsValidBase64UrlSimd("eyJhbGciOiJIUzI1NiIs=InR5cCI6IkpXVCJ9"))?;
}

fn runTest4(): void! {
    // >= 1,300 deterministic PRNG malformed JWT / Base64URL / JSON fuzz cases
    let seed: u64 = 0x428A2F98D728AE22;
    let alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_+.=/\":{}[], 0123456789";
    let secret = "fuzz-secret";
    let cfg = JwtValidation{
        expected_alg: JwtAlgorithm.HS256,
        allow_none_alg: false,
        leeway_sec: 60,
        validate_exp: true,
        validate_nbf: true,
        expected_iss: null,
        expected_sub: null,
        expected_aud: null,
    };

    let iter: usize = 0;
    while (iter < 1350) : (iter += 1) {
        let fuzz_buf: [192]u8 = undefined;
        let len: usize = iter % 192;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            seed = seed *% 6364136223846793005 +% 1442695040888963407;
            let ch_idx = @as(usize, @intCast((seed >> 32) % alphabet.len));
            fuzz_buf[j] = alphabet[ch_idx];
        }
        let sample = fuzz_buf[0..len];

        * = splitJwtStrict(sample);

        let dec_buf: [256]u8 = undefined;
        * = decodeBase64UrlStrict(sample, &dec_buf);

        check(jwtIsValidBase64UrlSimd(sample) == jwtIsValidBase64UrlScalar(sample))?;
        check(jwtFindDotSimd(sample) == jwtFindDotScalar(sample))?;

        let hb: [256]u8 = undefined;
        let pb: [256]u8 = undefined;
        * = verifyJwtHmac(sample, secret, 1500, cfg, &hb, &pb) catch |e| {
            * = e;
            continue;
        };
    }
}

fn main() {
    test("W32 R3: HS256/384/512 sign/verify roundtrip and RFC 7515 A.1 vector", () => {
        runTest1() catch {
            expect_eq(true, false);
        };
    });

    test("W32 R3: JWT security boundaries (none alg, alg confusion, tamper, exp/nbf leeway, iss/sub/aud, strict b64)", () => {
        runTest2() catch {
            expect_eq(true, false);
        };
    });

    test("W32 R3: SIMD vs Scalar 0-diff parity (ctEqual, findDot, isValidBase64Url)", () => {
        runTest3() catch {
            expect_eq(true, false);
        };
    });

    test("W32 R3: 1350 deterministic PRNG malformed JWT fuzz cases", () => {
        runTest4() catch {
            expect_eq(true, false);
        };
    });
}
