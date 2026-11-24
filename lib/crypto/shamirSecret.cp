// Reference-hardened against RustCrypto & GF(256) Shamir / SMT (MIT OR Apache-2.0)
// Origin: Pure-CP Stdlib (GF(256) Zero-Alloc Shamir (k, n) Threshold Secret Sharing + 16-Lane SIMD) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation, zero-C-FFI Shamir's Secret Sharing over GF(2^8):
//   - Branchless / constant-time GF(256) arithmetic under AES irreducible polynomial x^8 + x^4 + x^3 + x + 1 (0x11B)
//   - Supports 1..64 byte secrets split into 2 <= k <= n <= 32 shares
//   - Both zero-trust caller entropy injection (`splitSecretWithEntropy`) and deterministic SplitMix64 (`splitSecretDeterministic`)
//   - Exact Lagrange interpolation at x = 0 (`combineShamirShares`) with 16-lane SIMD GF(256) vector XOR accumulation
//   - Optional domain-separated SHA-256 secret commitment tag (`share_tag`) with 16-lane SIMD constant-time verification
//   - Full-path `std.crypto.secureZero` scrubbing of polynomial coefficients, Lagrange weights, and temporary buffers

const std = @import("std");

type ShamirSmtVec16u8 = @Vector(16, u8);

// ============================================================================
// Constants
// ============================================================================

/// Maximum secret payload length in bytes (`1..=64`).
pub const SHAMIR_MAX_SECRET_LEN: usize = 64;

/// Minimum reconstruction threshold (`k >= 2`).
pub const SHAMIR_MIN_THRESHOLD: u8 = 2;

/// Maximum number of shares per split (`n <= 32`).
pub const SHAMIR_MAX_SHARES: u8 = 32;

/// SHA-256 secret commitment tag length in bytes.
pub const SHAMIR_TAG_LEN: usize = 32;

const SHAMIR_SM64_INC: u64 = (@as(u64, 0x9e3779b9) << 32) | 0x7f4a7c15;
const SHAMIR_SM64_M1: u64 = (@as(u64, 0xbf58476d) << 32) | 0x1ce4e5b9;
const SHAMIR_SM64_M2: u64 = (@as(u64, 0x94d049bb) << 32) | 0x133111eb;

// ============================================================================
// 16-Lane @Vector(16, u8) SIMD Primitives + Scalar Differential References
// ============================================================================

fn shamirSmtLoadVec16(data: []const u8, off: usize): ShamirSmtVec16u8 {
    const v: ShamirSmtVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` non-short-circuiting constant-time byte slice equality check.
/// Hot path for SMT root hash comparison and Shamir commitment tag verification.
pub fn simdShamirSmtCtEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let acc_v: ShamirSmtVec16u8 = @splat(0);
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va = shamirSmtLoadVec16(a, i);
        const vb = shamirSmtLoadVec16(b, i);
        acc_v = acc_v | (va ^ vb);
        i += 16;
    }
    let diff: u8 = @reduce(.Or, acc_v);
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

/// Scalar reference for `simdShamirSmtCtEqual` (differential verification).
pub fn scalarShamirSmtCtEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

/// 16-lane `@Vector(16, u8)` in-place byte XOR (`dst[i] ^= src[i]`).
/// Hot path for GF(256) vector addition and Shamir Lagrange share accumulation.
pub fn simdShamirSmtXor(dst: []u8, src: []const u8) {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const vd = shamirSmtLoadVec16(dst, i);
        const vs = shamirSmtLoadVec16(src, i);
        const xr: [16]u8 = vd ^ vs;
        @memcpy(dst[i .. i + 16], xr[0..16]);
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = dst[i] ^ src[i];
    }
}

/// Scalar reference for `simdShamirSmtXor` (differential verification).
pub fn scalarShamirSmtXor(dst: []u8, src: []const u8) {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = dst[i] ^ src[i];
    }
}

// ============================================================================
// Internal Helpers (PRNG & Constant-Time Tag Comparison)
// ============================================================================

fn shamirSplitMix64Next(state: *u64): u64 {
    state.* = state.* +% SHAMIR_SM64_INC;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% SHAMIR_SM64_M1;
    z = (z ^ (z >> 27)) *% SHAMIR_SM64_M2;
    return z ^ (z >> 31);
}

fn shamirCtEqual32(a: [SHAMIR_TAG_LEN]u8, b: [SHAMIR_TAG_LEN]u8): bool {
    return simdShamirSmtCtEqual(a[0..], b[0..]);
}

// ============================================================================
// 1. Constant-Time GF(256) Arithmetic (AES Polynomial 0x11B)
// ============================================================================

/// Branchless constant-time multiplication in GF(2^8) modulo `x^8 + x^4 + x^3 + x + 1` (`0x11B`).
pub fn gf256Mul(a: u8, b: u8): u8 {
    let aa: u8 = a;
    let bb: u8 = b;
    let acc: u8 = 0;
    let bit: usize = 0;
    while (bit < 8) : (bit += 1) {
        const mask_b: u8 = (0 as u8) -% (bb & 1);
        acc = acc ^ (aa & mask_b);
        const hi_bit: u8 = (aa >> 7) & 1;
        const mask_hi: u8 = (0 as u8) -% hi_bit;
        aa = (aa << 1) ^ ((0x1B as u8) & mask_hi);
        bb = bb >> 1;
    }
    return acc;
}

/// Branchless constant-time multiplicative inverse in GF(2^8) via Fermat's Little Theorem (`a^254`).
/// Returns `0` for `a == 0`.
pub fn gf256Inv(a: u8): u8 {
    let p: u8 = gf256Mul(a, a);   // a^2
    let res: u8 = p;              // a^2
    p = gf256Mul(p, p);           // a^4
    res = gf256Mul(res, p);       // a^6
    p = gf256Mul(p, p);           // a^8
    res = gf256Mul(res, p);       // a^14
    p = gf256Mul(p, p);           // a^16
    res = gf256Mul(res, p);       // a^30
    p = gf256Mul(p, p);           // a^32
    res = gf256Mul(res, p);       // a^62
    p = gf256Mul(p, p);           // a^64
    res = gf256Mul(res, p);       // a^126
    p = gf256Mul(p, p);           // a^128
    res = gf256Mul(res, p);       // a^254
    return res;
}

/// Exact division `a / b` in GF(2^8). Returns `null` if `b == 0`.
pub fn gf256Div(a: u8, b: u8): u8? {
    if (b == 0) return null;
    return gf256Mul(a, gf256Inv(b));
}

// ============================================================================
// 2. Enums, Share & Result Structs
// ============================================================================

/// Status code for Shamir secret splitting and reconstruction operations.
pub enum ShamirStatus: u8 {
    ok = 0,
    invalid_threshold = 1,
    invalid_share_count = 2,
    invalid_secret_len = 3,
    insufficient_entropy = 4,
    insufficient_shares = 5,
    invalid_x_zero = 6,
    duplicate_x = 7,
    mismatched_share_len = 8,
    output_too_small = 9,
    tag_mismatch = 10,
}

/// Fixed-capacity Shamir share storing coordinate `x in 1..=255`, payload bytes `y[0..secret_len]`,
/// and an optional SHA-256 secret commitment tag for tamper/corruption detection.
pub struct ShamirShare {
    x: u8 = 0;
    threshold: u8 = 0;
    secret_len: u8 = 0;
    has_tag: bool = false;
    share_tag: [SHAMIR_TAG_LEN]u8 = [0; SHAMIR_TAG_LEN];
    y: [SHAMIR_MAX_SECRET_LEN]u8 = [0; SHAMIR_MAX_SECRET_LEN];

    /// Returns a zero-copy slice of this share's evaluated payload bytes.
    pub fn payloadSlice(): []const u8 {
        const n: usize = _.secret_len as usize;
        const clamped: usize = if (n > SHAMIR_MAX_SECRET_LEN) SHAMIR_MAX_SECRET_LEN else n;
        return _.y[0..clamped];
    }

    /// Securely scrubs all sensitive share payload and commitment tag bytes.
    pub fn zeroize(*_) {
        std.crypto.secureZero(u8, _.y[0..]);
        std.crypto.secureZero(u8, _.share_tag[0..]);
        _.x = 0;
        _.threshold = 0;
        _.secret_len = 0;
        _.has_tag = false;
    }
}

/// Outcome returned by `splitSecretWithEntropy` / `splitSecretDeterministic`.
pub struct ShamirSplitResult {
    status: ShamirStatus = ShamirStatus.ok;
    shares_written: usize = 0;
    secret_tag: [SHAMIR_TAG_LEN]u8 = [0; SHAMIR_TAG_LEN];
}

/// Outcome returned by `combineShamirShares`.
pub struct ShamirCombineResult {
    status: ShamirStatus = ShamirStatus.ok;
    secret_len: usize = 0;
    tag_verified: bool = false;
}

// ============================================================================
// 3. Commitment Tag & Secret Splitting / Reconstruction
// ============================================================================

/// Returns the exact number of random coefficient bytes needed by `splitSecretWithEntropy`:
/// `secret_len * (threshold - 1)`. Returns `0` on invalid parameters.
pub fn shamirRequiredEntropyBytes(secret_len: usize, threshold: u8): usize {
    if (secret_len == 0 || secret_len > SHAMIR_MAX_SECRET_LEN) return 0;
    if (threshold < SHAMIR_MIN_THRESHOLD || threshold > SHAMIR_MAX_SHARES) return 0;
    return secret_len * ((threshold as usize) - 1);
}

/// Computes a domain-separated SHA-256 commitment digest over `secret` (zero C FFI, zero heap).
pub fn computeSecretTagSha256(secret: []const u8): [SHAMIR_TAG_LEN]u8 {
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    h.update("cp:shamir:v1:");
    h.update(secret);
    let out: [SHAMIR_TAG_LEN]u8 = [0; SHAMIR_TAG_LEN];
    h.final(&out);
    return out;
}

/// Constant-time verification that `secret` matches `expected_tag` (16-lane SIMD + secureZero).
pub fn verifyReconstructedSecret(secret: []const u8, expected_tag: [SHAMIR_TAG_LEN]u8): bool {
    let actual = computeSecretTagSha256(secret);
    defer std.crypto.secureZero(u8, actual[0..]);
    return shamirCtEqual32(actual, expected_tag);
}

/// Splits `secret` (`1..=64` bytes) into `total_shares` (`2 <= threshold <= total_shares <= 32`)
/// using caller-provided cryptographic `entropy_bytes` (must have length `>= secret.len * (threshold - 1)`).
///
/// For each secret byte `b`, constructs the degree `threshold - 1` polynomial:
///   `f_b(x) = secret[b] ^ c_1*x ^ c_2*x^2 ^ ... ^ c_{k-1}*x^{k-1}`
/// and evaluates it at `x = 1, 2, ..., total_shares` via Horner's method.
pub fn splitSecretWithEntropy(
    secret: []const u8,
    threshold: u8,
    total_shares: u8,
    entropy_bytes: []const u8,
    attach_tag: bool,
    out_shares: *[]ShamirShare,
): ShamirSplitResult {
    if (secret.len == 0 || secret.len > SHAMIR_MAX_SECRET_LEN) {
        return ShamirSplitResult{ status: ShamirStatus.invalid_secret_len };
    }
    if (threshold < SHAMIR_MIN_THRESHOLD || threshold > SHAMIR_MAX_SHARES) {
        return ShamirSplitResult{ status: ShamirStatus.invalid_threshold };
    }
    if (total_shares < threshold || total_shares > SHAMIR_MAX_SHARES) {
        return ShamirSplitResult{ status: ShamirStatus.invalid_share_count };
    }
    if (out_shares.len < (total_shares as usize)) {
        return ShamirSplitResult{ status: ShamirStatus.output_too_small };
    }

    const degree: usize = (threshold as usize) - 1;
    const req_entropy: usize = secret.len * degree;
    if (entropy_bytes.len < req_entropy) {
        return ShamirSplitResult{ status: ShamirStatus.insufficient_entropy };
    }

    const tag: [SHAMIR_TAG_LEN]u8 = if (attach_tag) computeSecretTagSha256(secret) else [0; SHAMIR_TAG_LEN];

    let s_idx: usize = 0;
    while (s_idx < (total_shares as usize)) : (s_idx += 1) {
        const x: u8 = (s_idx + 1) as u8;
        let share = ShamirShare{
            x: x,
            threshold: threshold,
            secret_len: secret.len as u8,
            has_tag: attach_tag,
            share_tag: tag,
            y: [0; SHAMIR_MAX_SECRET_LEN],
        };

        let b: usize = 0;
        while (b < secret.len) : (b += 1) {
            const coeff_base: usize = b * degree;
            let acc: u8 = 0;
            let c: usize = degree;
            while (c > 0) : (c -= 1) {
                acc = gf256Mul(acc, x) ^ entropy_bytes[coeff_base + c - 1];
            }
            acc = gf256Mul(acc, x) ^ secret[b];
            share.y[b] = acc;
        }

        out_shares[s_idx] = share;
    }

    return ShamirSplitResult{
        status: ShamirStatus.ok,
        shares_written: total_shares as usize,
        secret_tag: tag,
    };
}

/// Deterministic variant of `splitSecretWithEntropy` deriving polynomial coefficients from
/// SplitMix64 seeded with `rng_seed`, scrubbing all temporary coefficient buffers via `secureZero`.
pub fn splitSecretDeterministic(
    secret: []const u8,
    threshold: u8,
    total_shares: u8,
    rng_seed: u64,
    attach_tag: bool,
    out_shares: *[]ShamirShare,
): ShamirSplitResult {
    if (secret.len == 0 || secret.len > SHAMIR_MAX_SECRET_LEN) {
        return ShamirSplitResult{ status: ShamirStatus.invalid_secret_len };
    }
    if (threshold < SHAMIR_MIN_THRESHOLD || threshold > SHAMIR_MAX_SHARES) {
        return ShamirSplitResult{ status: ShamirStatus.invalid_threshold };
    }
    if (total_shares < threshold || total_shares > SHAMIR_MAX_SHARES) {
        return ShamirSplitResult{ status: ShamirStatus.invalid_share_count };
    }

    const req_entropy: usize = secret.len * ((threshold as usize) - 1);
    let ent_buf: [SHAMIR_MAX_SECRET_LEN * 31]u8 = [0; SHAMIR_MAX_SECRET_LEN * 31];
    defer std.crypto.secureZero(u8, ent_buf[0..]);

    let state: u64 = if (rng_seed == 0) SHAMIR_SM64_INC else rng_seed;

    let i: usize = 0;
    while (i < req_entropy) : (i += 1) {
        ent_buf[i] = (shamirSplitMix64Next(&state) & 0xff) as u8;
    }

    return splitSecretWithEntropy(
        secret,
        threshold,
        total_shares,
        ent_buf[0..req_entropy],
        attach_tag,
        out_shares,
    );
}

/// Reconstructs the original secret at `x = 0` via Lagrange interpolation in GF(2^8) from `shares`.
///
/// Hardened guarantees:
///   - Enforces `shares.len >= max(2, max_declared_threshold)` and `shares.len <= SHAMIR_MAX_SHARES`
///   - Rejects `x == 0`, duplicate `x` coordinates, and mismatched `secret_len`
///   - Uses 16-lane SIMD `simdShamirSmtXor` for GF(256) share accumulation
///   - Uses 16-lane SIMD `simdShamirSmtCtEqual` for commitment tag verification
///   - Scrubs Lagrange weights, temporary accumulators, and failed `out_secret` via `std.crypto.secureZero`
pub fn combineShamirShares(
    shares: []const ShamirShare,
    out_secret: *[]u8,
): ShamirCombineResult {
    if (shares.len < (SHAMIR_MIN_THRESHOLD as usize)) {
        return ShamirCombineResult{ status: ShamirStatus.insufficient_shares };
    }
    if (shares.len > (SHAMIR_MAX_SHARES as usize)) {
        return ShamirCombineResult{ status: ShamirStatus.invalid_share_count };
    }

    const slen: usize = shares[0].secret_len as usize;
    if (slen == 0 || slen > SHAMIR_MAX_SECRET_LEN) {
        return ShamirCombineResult{ status: ShamirStatus.invalid_secret_len };
    }
    if (out_secret.len < slen) {
        return ShamirCombineResult{ status: ShamirStatus.output_too_small };
    }

    // Validate all coordinates (x != 0, pairwise distinct), max threshold, lengths, and optional tag.
    let has_any_tag: bool = false;
    let ref_tag: [SHAMIR_TAG_LEN]u8 = [0; SHAMIR_TAG_LEN];
    defer std.crypto.secureZero(u8, ref_tag[0..]);
    let req_threshold: usize = SHAMIR_MIN_THRESHOLD as usize;

    let i: usize = 0;
    while (i < shares.len) : (i += 1) {
        if (shares[i].x == 0) {
            return ShamirCombineResult{ status: ShamirStatus.invalid_x_zero };
        }
        if ((shares[i].secret_len as usize) != slen) {
            return ShamirCombineResult{ status: ShamirStatus.mismatched_share_len };
        }
        if (shares[i].threshold >= SHAMIR_MIN_THRESHOLD) {
            const t_usize: usize = shares[i].threshold as usize;
            if (t_usize > req_threshold) {
                req_threshold = t_usize;
            }
        }
        if (shares[i].has_tag) {
            if (!has_any_tag) {
                has_any_tag = true;
                ref_tag = shares[i].share_tag;
            } else if (!shamirCtEqual32(ref_tag, shares[i].share_tag)) {
                return ShamirCombineResult{ status: ShamirStatus.tag_mismatch };
            }
        }
        let j: usize = i + 1;
        while (j < shares.len) : (j += 1) {
            if (shares[i].x == shares[j].x) {
                return ShamirCombineResult{ status: ShamirStatus.duplicate_x };
            }
        }
    }

    if (shares.len < req_threshold) {
        return ShamirCombineResult{ status: ShamirStatus.insufficient_shares };
    }

    // Precompute Lagrange basis coefficients lambda_i(0) = prod_{j != i} (x_j / (x_i ^ x_j))
    let lambdas: [SHAMIR_MAX_SHARES]u8 = [0; SHAMIR_MAX_SHARES];
    defer std.crypto.secureZero(u8, lambdas[0..]);

    let li: usize = 0;
    while (li < shares.len) : (li += 1) {
        let num: u8 = 1;
        let den: u8 = 1;
        let lj: usize = 0;
        while (lj < shares.len) : (lj += 1) {
            if (li == lj) continue;
            num = gf256Mul(num, shares[lj].x);
            den = gf256Mul(den, shares[li].x ^ shares[lj].x);
        }
        lambdas[li] = gf256Mul(num, gf256Inv(den));
    }

    // Interpolate secret bytes using 16-lane SIMD GF(256) vector XOR accumulation
    let acc_buf: [SHAMIR_MAX_SECRET_LEN]u8 = [0; SHAMIR_MAX_SECRET_LEN];
    defer std.crypto.secureZero(u8, acc_buf[0..]);
    let term_buf: [SHAMIR_MAX_SECRET_LEN]u8 = [0; SHAMIR_MAX_SECRET_LEN];
    defer std.crypto.secureZero(u8, term_buf[0..]);

    let si: usize = 0;
    while (si < shares.len) : (si += 1) {
        const lam = lambdas[si];
        let b: usize = 0;
        while (b < slen) : (b += 1) {
            term_buf[b] = gf256Mul(shares[si].y[b], lam);
        }
        simdShamirSmtXor(acc_buf[0..slen], term_buf[0..slen]);
    }

    @memcpy(out_secret[0..slen], acc_buf[0..slen]);

    if (has_any_tag) {
        if (!verifyReconstructedSecret(out_secret[0..slen], ref_tag)) {
            std.crypto.secureZero(u8, out_secret[0..slen]);
            return ShamirCombineResult{
                status: ShamirStatus.tag_mismatch,
                secret_len: slen,
                tag_verified: false,
            };
        }
        return ShamirCombineResult{
            status: ShamirStatus.ok,
            secret_len: slen,
            tag_verified: true,
        };
    }

    return ShamirCombineResult{
        status: ShamirStatus.ok,
        secret_len: slen,
        tag_verified: false,
    };
}
