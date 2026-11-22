// Origin: Pure-CP Stdlib (GF(256) Zero-Alloc Shamir (k, n) Threshold Secret Sharing & Lagrange Reconstruction) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation, zero-C-FFI Shamir's Secret Sharing over GF(2^8):
//   - Branchless / constant-time GF(256) arithmetic under AES irreducible polynomial x^8 + x^4 + x^3 + x + 1 (0x11B)
//   - Supports 1..64 byte secrets split into 2 <= k <= n <= 32 shares
//   - Both zero-trust caller entropy injection (`splitSecretWithEntropy`) and deterministic SplitMix64 (`splitSecretDeterministic`)
//   - Exact Lagrange interpolation at x = 0 (`combineShamirShares`) for any k (or >= k) distinct valid shares
//   - Optional domain-separated SHA-256 secret commitment tag (`share_tag`) with constant-time integrity verification

const std = @import("std");

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

// ============================================================================
// Internal Helpers (PRNG & Constant-Time Tag Comparison)
// ============================================================================

fn splitMix64Next(state: *u64): u64 {
    state.* = state.* +% 0x9e3779b97f4a7c15;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

fn ctEqual32(a: [SHAMIR_TAG_LEN]u8, b: [SHAMIR_TAG_LEN]u8): bool {
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < SHAMIR_TAG_LEN) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
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
    fn payloadSlice(): []const u8 {
        const n: usize = _.secret_len as usize;
        const clamped: usize = if (n > SHAMIR_MAX_SECRET_LEN) SHAMIR_MAX_SECRET_LEN else n;
        return _.y[0..clamped];
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

/// Constant-time verification that `secret` matches `expected_tag`.
pub fn verifyReconstructedSecret(secret: []const u8, expected_tag: [SHAMIR_TAG_LEN]u8): bool {
    const actual = computeSecretTagSha256(secret);
    return ctEqual32(actual, expected_tag);
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
            // Evaluate polynomial at x via Horner's rule:
            // acc = (((c_{k-1} * x ^ c_{k-2}) * x ^ ...) * x ^ secret[b])
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
/// SplitMix64 seeded with `rng_seed` (ideal for reproducible testing and deterministic pipelines).
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
    let state: u64 = if (rng_seed == 0) 0x9e3779b97f4a7c15 else rng_seed;

    let i: usize = 0;
    while (i < req_entropy) : (i += 1) {
        ent_buf[i] = (splitMix64Next(&state) & 0xff) as u8;
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
/// Validates:
///   - `shares.len >= 2` and `shares.len <= SHAMIR_MAX_SHARES`
///   - If `shares[0].threshold >= 2`, enforces `shares.len >= shares[0].threshold`
///   - Every share has `x != 0`, no duplicate `x` coordinates, and identical `secret_len in 1..=64`
///   - `out_secret.len >= secret_len`
///   - If any share carries `has_tag == true`, automatically verifies the reconstructed secret
///     against `share_tag` in constant time and returns `tag_mismatch` on tampering/corruption.
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
    if (shares[0].threshold >= SHAMIR_MIN_THRESHOLD && shares.len < (shares[0].threshold as usize)) {
        return ShamirCombineResult{ status: ShamirStatus.insufficient_shares };
    }
    if (out_secret.len < slen) {
        return ShamirCombineResult{ status: ShamirStatus.output_too_small };
    }

    // Validate all coordinates (x != 0, pairwise distinct), lengths, and collect optional tag.
    let has_any_tag: bool = false;
    let ref_tag: [SHAMIR_TAG_LEN]u8 = [0; SHAMIR_TAG_LEN];

    let i: usize = 0;
    while (i < shares.len) : (i += 1) {
        if (shares[i].x == 0) {
            return ShamirCombineResult{ status: ShamirStatus.invalid_x_zero };
        }
        if ((shares[i].secret_len as usize) != slen) {
            return ShamirCombineResult{ status: ShamirStatus.mismatched_share_len };
        }
        if (shares[i].has_tag) {
            if (!has_any_tag) {
                has_any_tag = true;
                ref_tag = shares[i].share_tag;
            } else if (!ctEqual32(ref_tag, shares[i].share_tag)) {
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

    // Precompute Lagrange basis coefficients lambda_i(0) = prod_{j != i} (x_j / (x_i ^ x_j))
    let lambdas: [SHAMIR_MAX_SHARES]u8 = [0; SHAMIR_MAX_SHARES];
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

    // Interpolate each secret byte independently in GF(2^8)
    let b: usize = 0;
    while (b < slen) : (b += 1) {
        let acc: u8 = 0;
        let si: usize = 0;
        while (si < shares.len) : (si += 1) {
            acc = acc ^ gf256Mul(shares[si].y[b], lambdas[si]);
        }
        out_secret[b] = acc;
    }

    if (has_any_tag) {
        if (!verifyReconstructedSecret(out_secret[0..slen], ref_tag)) {
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
