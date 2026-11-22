// Origin: Pure-CP Stdlib Test Suite (lib/crypto/shamir_secret.cp) | Created: 2026-11-22
// Production Deep Verification Suite for GF(256) Zero-Alloc Shamir (k, n) Secret Sharing:
//   1. Exhaustive GF(256) field axiom verification (AES 0x11B test vectors + all 255 non-zero inverses)
//   2. (3, 5) split & all C(5,3)=10 subsets + superset (4/5, 5/5) exact reconstruction with SHA-256 tag
//   3. Zero-trust entropy injection (`splitSecretWithEntropy`) & below-threshold secrecy check
//   4. Tampered / forged share detection via constant-time SHA-256 commitment tag (`tag_mismatch`)
//   5. Extreme boundaries: 1-byte (2, 32) and max 64-byte (32, 32) reconstruction
//   6. Coordinate guards: `x == 0` (`invalid_x_zero`) & duplicate `x` (`duplicate_x`)
//   7. Split parameter guards: k < 2, k > n, n > 32, empty/>64B secret, insufficient entropy, small out_shares
//   8. Combine guards: insufficient shares, mismatched lengths, small out_secret & tagless mode

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SHAMIR_MAX_SECRET_LEN,
    SHAMIR_MIN_THRESHOLD,
    SHAMIR_MAX_SHARES,
    SHAMIR_TAG_LEN,
    gf256Mul,
    gf256Inv,
    gf256Div,
    ShamirStatus,
    ShamirShare,
    ShamirSplitResult,
    ShamirCombineResult,
    shamirRequiredEntropyBytes,
    computeSecretTagSha256,
    verifyReconstructedSecret,
    splitSecretWithEntropy,
    splitSecretDeterministic,
    combineShamirShares,
} from "cap:crypto";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Exhaustive GF(256) Field Axioms (AES Polynomial 0x11B)
// ============================================================================

fn testGf256FieldAxiomsExhaustive()! {
    // Classic FIPS-197 AES GF(2^8) multiplication vectors
    expect_eq(cap.crypto.gf256Mul(0x57, 0x83), 0xC1);
    expect_eq(gf256Mul(0x57, 0x13), 0xFE);
    expect_eq(gf256Mul(0x00, 0xFF), 0x00);
    expect_eq(gf256Inv(0), 0);
    expect_eq(gf256Div(0x57, 0) == null, true);

    // Every non-zero element a in 1..255 satisfies a * inv(a) == 1 and a / a == 1
    let a: usize = 1;
    while (a <= 255) : (a += 1) {
        const av: u8 = a as u8;
        const inv: u8 = gf256Inv(av);
        expect_eq(gf256Mul(av, inv), 1);
        expect_eq(gf256Div(av, av) ?? 0, 1);
        const prod: u8 = gf256Mul(av, 0x53);
        expect_eq(gf256Div(prod, 0x53) ?? 0, av);
    }
}

// ============================================================================
// Test 2: (3, 5) Split & All 10 Subsets + Superset Reconstruction
// ============================================================================

fn test3Of5AllSubsetsAndSupersets()! {
    const secret: [32]u8 = [
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0x01, 0x02, 0x03, 0x04, 0xF0, 0xE0, 0xD0, 0xC0,
    ];

    let shares: [5]ShamirShare = [ShamirShare{}; 5];
    let shares_s: []ShamirShare = shares[0..];
    const split_res: ShamirSplitResult = splitSecretDeterministic(secret[0..], 3, 5, 424242, true, *shares_s);
    expect_eq(split_res.status == ShamirStatus.ok, true);
    expect_eq(split_res.shares_written, 5);
    expect_eq(verifyReconstructedSecret(secret[0..], split_res.secret_tag), true);

    // Exhaustively test all C(5, 3) = 10 distinct 3-share subsets (in reverse order too)
    let out: [32]u8 = [0; 32];
    let out_s: []u8 = out[0..];
    let tested: usize = 0;

    let i: usize = 0;
    while (i < 5) : (i += 1) {
        let j: usize = i + 1;
        while (j < 5) : (j += 1) {
            let k: usize = j + 1;
            while (k < 5) : (k += 1) {
                const sub: [3]ShamirShare = [shares[k], shares[i], shares[j]];
                const comb: ShamirCombineResult = combineShamirShares(sub[0..], *out_s);
                expect_eq(comb.status == ShamirStatus.ok, true);
                expect_eq(comb.secret_len, 32);
                expect_eq(comb.tag_verified, true);
                expect_eq(bytesEq(out[0..32], secret[0..]), true);
                tested += 1;
            }
        }
    }
    expect_eq(tested, 10);

    // Superset: 4-of-5 and 5-of-5 reconstruct identically
    const comb4 = combineShamirShares(shares[0..4], *out_s);
    expect_eq(comb4.status == ShamirStatus.ok, true);
    expect_eq(bytesEq(out[0..32], secret[0..]), true);

    const comb5 = combineShamirShares(shares[0..5], *out_s);
    expect_eq(comb5.status == ShamirStatus.ok, true);
    expect_eq(bytesEq(out[0..32], secret[0..]), true);
}

// ============================================================================
// Test 3: Zero-Trust Entropy Injection & Below-Threshold Secrecy
// ============================================================================

fn testZeroTrustEntropyInjectionAndThresholdSecrecy()! {
    const secret: []const u8 = "master-key-2026!";
    expect_eq(shamirRequiredEntropyBytes(secret.len, 3), 32);

    let ent1: [32]u8 = [0; 32];
    let ent2: [32]u8 = [0; 32];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        ent1[i] = ((i * 37 + 13) & 0xff) as u8;
        ent2[i] = ((i * 73 + 91) & 0xff) as u8;
    }

    let s1: [4]ShamirShare = [ShamirShare{}; 4];
    let s2: [4]ShamirShare = [ShamirShare{}; 4];
    let s1_s: []ShamirShare = s1[0..];
    let s2_s: []ShamirShare = s2[0..];

    expect_eq(splitSecretWithEntropy(secret, 3, 4, ent1[0..], true, *s1_s).status == ShamirStatus.ok, true);
    expect_eq(splitSecretWithEntropy(secret, 3, 4, ent2[0..], true, *s2_s).status == ShamirStatus.ok, true);

    // Different entropy produces distinct shares for the same secret
    expect_eq(bytesEq(s1[0].payloadSlice(), s2[0].payloadSlice()), false);

    // Both reconstruct the exact same secret
    let out: [32]u8 = [0; 32];
    let out_s: []u8 = out[0..];
    expect_eq(combineShamirShares(s1[1..4], *out_s).status == ShamirStatus.ok, true);
    expect_eq(bytesEq(out[0..secret.len], secret), true);
    expect_eq(combineShamirShares(s2[0..3], *out_s).status == ShamirStatus.ok, true);
    expect_eq(bytesEq(out[0..secret.len], secret), true);

    // Below-threshold (2 shares when k=3) fails threshold check
    expect_eq(combineShamirShares(s1[0..2], *out_s).status == ShamirStatus.insufficient_shares, true);
}

// ============================================================================
// Test 4: Tampered & Forged Share Detection (`tag_mismatch`)
// ============================================================================

fn testTamperedShareDetectionViaSha256Tag()! {
    const secret: []const u8 = "top-secret-vault-seed";
    let shares: [4]ShamirShare = [ShamirShare{}; 4];
    let shares_s: []ShamirShare = shares[0..];
    const sp = splitSecretDeterministic(secret, 3, 4, 777, true, *shares_s);
    expect_eq(sp.status == ShamirStatus.ok, true);

    // Flip 1 bit in share 1 payload -> tag_mismatch
    let corrupted: [3]ShamirShare = [shares[0], shares[1], shares[2]];
    corrupted[1].y[5] = corrupted[1].y[5] ^ 0x01;
    let out: [32]u8 = [0; 32];
    let out_s: []u8 = out[0..];
    const res1 = combineShamirShares(corrupted[0..], *out_s);
    expect_eq(res1.status == ShamirStatus.tag_mismatch, true);
    expect_eq(res1.tag_verified, false);

    // Mix a share from a completely different secret -> tag_mismatch
    let other_shares: [3]ShamirShare = [ShamirShare{}; 3];
    let other_s: []ShamirShare = other_shares[0..];
    * = splitSecretDeterministic("other-secret-vault-00", 3, 3, 888, true, *other_s);
    const mixed: [3]ShamirShare = [shares[0], shares[1], other_shares[2]];
    expect_eq(combineShamirShares(mixed[0..], *out_s).status == ShamirStatus.tag_mismatch, true);

    // Direct verifyReconstructedSecret check
    const tag = computeSecretTagSha256(secret);
    expect_eq(verifyReconstructedSecret(secret, tag), true);
    expect_eq(verifyReconstructedSecret("top-secret-vault-seee", tag), false);
}

// ============================================================================
// Test 5: Extreme Boundaries — 1-Byte (2, 32) and 64-Byte (32, 32)
// ============================================================================

fn testExtremeBoundaries1ByteAnd64Byte32Of32()! {
    // 1-byte secret split (2, 32): combine share 1 (x=1) and share 32 (x=32)
    const s1b: [1]u8 = [0xA5];
    let sh32: [32]ShamirShare = [ShamirShare{}; 32];
    let sh32_s: []ShamirShare = sh32[0..];
    expect_eq(splitSecretDeterministic(s1b[0..], 2, 32, 101, true, *sh32_s).status == ShamirStatus.ok, true);

    const pair: [2]ShamirShare = [sh32[0], sh32[31]];
    let out1: [1]u8 = [0; 1];
    let out1_s: []u8 = out1[0..];
    const c1 = combineShamirShares(pair[0..], *out1_s);
    expect_eq(c1.status == ShamirStatus.ok, true);
    expect_eq(c1.tag_verified, true);
    expect_eq(out1[0], 0xA5);

    // Max 64-byte secret split (32, 32): all 32 shares required and reconstruct bit-for-bit
    let s64: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        s64[i] = ((i * 19 + 7) & 0xff) as u8;
    }
    expect_eq(splitSecretDeterministic(s64[0..], 32, 32, 202, true, *sh32_s).status == ShamirStatus.ok, true);

    let out64: [64]u8 = [0; 64];
    let out64_s: []u8 = out64[0..];
    const c64 = combineShamirShares(sh32[0..32], *out64_s);
    expect_eq(c64.status == ShamirStatus.ok, true);
    expect_eq(c64.secret_len, 64);
    expect_eq(c64.tag_verified, true);
    expect_eq(bytesEq(out64[0..], s64[0..]), true);
}

// ============================================================================
// Test 6: Coordinate Guards (`x == 0` and Duplicate `x`)
// ============================================================================

fn testCoordinateGuardsZeroAndDuplicateX()! {
    let shares: [3]ShamirShare = [ShamirShare{}; 3];
    let shares_s: []ShamirShare = shares[0..];
    * = splitSecretDeterministic("abcd", 2, 3, 99, true, *shares_s);

    let out: [4]u8 = [0; 4];
    let out_s: []u8 = out[0..];

    // x == 0 rejected
    let bad_zero: [2]ShamirShare = [shares[0], shares[1]];
    bad_zero[0].x = 0;
    expect_eq(combineShamirShares(bad_zero[0..], *out_s).status == ShamirStatus.invalid_x_zero, true);

    // Duplicate x rejected
    const bad_dup: [2]ShamirShare = [shares[1], shares[1]];
    expect_eq(combineShamirShares(bad_dup[0..], *out_s).status == ShamirStatus.duplicate_x, true);
}

// ============================================================================
// Test 7: Split Parameter Boundary Guards
// ============================================================================

fn testSplitParameterGuards()! {
    let shares: [4]ShamirShare = [ShamirShare{}; 4];
    let shares_s: []ShamirShare = shares[0..];
    const ent_ok: [16]u8 = [0x33; 16];
    const ent_short: [2]u8 = [0x11; 2];

    // Empty & >64B secret
    expect_eq(splitSecretWithEntropy("", 2, 3, ent_ok[0..], true, *shares_s).status == ShamirStatus.invalid_secret_len, true);
    const big65: [65]u8 = [0xAA; 65];
    expect_eq(splitSecretDeterministic(big65[0..], 2, 3, 1, true, *shares_s).status == ShamirStatus.invalid_secret_len, true);

    // k < 2, k > n, n > 32
    expect_eq(splitSecretDeterministic("abcd", 1, 3, 1, true, *shares_s).status == ShamirStatus.invalid_threshold, true);
    expect_eq(splitSecretDeterministic("abcd", 4, 3, 1, true, *shares_s).status == ShamirStatus.invalid_share_count, true);
    expect_eq(splitSecretDeterministic("abcd", 2, 33, 1, true, *shares_s).status == ShamirStatus.invalid_share_count, true);

    // Insufficient entropy
    expect_eq(splitSecretWithEntropy("abcd", 3, 4, ent_short[0..], true, *shares_s).status == ShamirStatus.insufficient_entropy, true);

    // Output shares slice too small
    let tiny_out: [2]ShamirShare = [ShamirShare{}; 2];
    let tiny_s: []ShamirShare = tiny_out[0..];
    expect_eq(splitSecretDeterministic("abcd", 2, 3, 1, true, *tiny_s).status == ShamirStatus.output_too_small, true);

    expect_eq(SHAMIR_MAX_SECRET_LEN, 64);
    expect_eq(SHAMIR_MIN_THRESHOLD, 2);
    expect_eq(SHAMIR_MAX_SHARES, 32);
    expect_eq(SHAMIR_TAG_LEN, 32);
}

// ============================================================================
// Test 8: Combine Guards & Tagless Mode
// ============================================================================

fn testCombineGuardsAndTaglessMode()! {
    let shares: [3]ShamirShare = [ShamirShare{}; 3];
    let shares_s: []ShamirShare = shares[0..];
    expect_eq(splitSecretDeterministic("hello-world", 2, 3, 55, false, *shares_s).status == ShamirStatus.ok, true);

    let out: [16]u8 = [0; 16];
    let out_s: []u8 = out[0..];

    // Tagless mode succeeds with tag_verified == false
    const ok = combineShamirShares(shares[0..2], *out_s);
    expect_eq(ok.status == ShamirStatus.ok, true);
    expect_eq(ok.tag_verified, false);
    expect_eq(bytesEq(out[0..11], "hello-world"), true);

    // Single share -> insufficient_shares
    expect_eq(combineShamirShares(shares[0..1], *out_s).status == ShamirStatus.insufficient_shares, true);

    // Mismatched secret_len across shares
    let mismatch: [2]ShamirShare = [shares[0], shares[1]];
    mismatch[1].secret_len = 5;
    expect_eq(combineShamirShares(mismatch[0..], *out_s).status == ShamirStatus.mismatched_share_len, true);

    // Output buffer too small
    let small_out: [4]u8 = [0; 4];
    let small_s: []u8 = small_out[0..];
    expect_eq(combineShamirShares(shares[0..2], *small_s).status == ShamirStatus.output_too_small, true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("crypto.shamir_secret: exhaustive GF(256) AES 0x11B field axioms & 255 inverses", testGf256FieldAxiomsExhaustive);
test("crypto.shamir_secret: (3, 5) all C(5,3)=10 subsets & supersets reconstruction", test3Of5AllSubsetsAndSupersets);
test("crypto.shamir_secret: zero-trust entropy injection & below-threshold secrecy", testZeroTrustEntropyInjectionAndThresholdSecrecy);
test("crypto.shamir_secret: tampered & forged share detection via SHA-256 tag", testTamperedShareDetectionViaSha256Tag);
test("crypto.shamir_secret: extreme 1B (2, 32) & 64B (32, 32) reconstruction", testExtremeBoundaries1ByteAnd64Byte32Of32);
test("crypto.shamir_secret: x == 0 and duplicate x coordinate guards", testCoordinateGuardsZeroAndDuplicateX);
test("crypto.shamir_secret: split parameter & entropy boundary guards", testSplitParameterGuards);
test("crypto.shamir_secret: combine guards & tagless mode", testCombineGuardsAndTaglessMode);
