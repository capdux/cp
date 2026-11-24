// Reference-hardened against RustCrypto/KDFs & RustCrypto/MACs (MIT OR Apache-2.0) & RFC 5869/2104
// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Forward-Secret Symmetric Key Ratchet) | Created: 2026-11-22
// Pure CP zero-heap-allocation Signal/TLS-1.3 style Forward-Secret Symmetric Key Ratchet & Out-of-Order Key Cache:
//   - 16-lane @Vector(16, u8) SIMD branchless constant-time equality (`simdCryptoExtCtEqual`),
//     16-lane XOR (`simdCryptoExtXor`), 16-lane copy (`simdCryptoExtCopy`), and constant-time all-zero check
//   - RFC 5869 HKDF-SHA256 initialization (`Extract + Expand`) with salt & context_info domain separation
//   - One-way HMAC-SHA256 ratchet advancement (`0x01` msg_key, `0x03` 12B nonce_prefix, `0x02` next chain_key)
//     immediately overwriting prior chain_key and scrubbing intermediates via `std.crypto.secureZero`
//   - Fixed inline 32-slot skipped-key cache (`RATCHET_MAX_SKIPPED_KEYS`) supporting out-of-order delivery,
//     strict one-time consumption (replay rejection), and atomic DoS-resistant max-skip bounds
//   - Post-Compromise Security via HKDF entropy mixing (`rekey`/`rekeyStrict`) and `secureZero` scrubbing

const std = @import("std");

/// Module-local 16-lane byte vector alias for symmetric crypto extension hot paths.
type CryptoExtVec16u8 = @Vector(16, u8);

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Symmetric message & chain key byte length (256-bit).
pub const RATCHET_KEY_LEN: usize = 32;

/// Minimum recommended secret length (128-bit) enforced by `initStrict` / `rekeyStrict`.
pub const RATCHET_MIN_STRICT_SECRET_LEN: usize = 16;

/// Maximum RFC 5869 HKDF-SHA256 expanded output length (`255 * 32 = 8160` bytes).
pub const RATCHET_HKDF_MAX_OUTPUT_LEN: usize = 255 * 32;

/// Maximum skipped out-of-order message keys cached inline (anti-DoS bound).
pub const RATCHET_MAX_SKIPPED_KEYS: usize = 32;

/// Single derived message key & 96-bit AEAD nonce prefix for `step_index`.
pub struct RatchetDerivedKey {
    step_index: u64 = 0;
    key: [RATCHET_KEY_LEN]u8 = [0; RATCHET_KEY_LEN];
    nonce_prefix: [12]u8 = [0; 12];
}

/// Telemetry & state snapshot returned by `SymmetricKeyRatchet.stats()`.
pub struct RatchetStats {
    current_step: u64 = 0;
    cached_skipped_keys: usize = 0;
    total_derived: u64 = 0;
    total_out_of_order_hits: u64 = 0;
    rekey_count: u32 = 0;
}

/// Internal inline cache slot for a skipped out-of-order message key.
struct RatchetSkippedKeySlot {
    step_index: u64 = 0;
    key: [RATCHET_KEY_LEN]u8 = [0; RATCHET_KEY_LEN];
    nonce_prefix: [12]u8 = [0; 12];
    occupied: bool = false;
}

// ============================================================================
// 2. 16-Lane SIMD Branchless Constant-Time & Buffer Primitives (0-Diff)
// ============================================================================

/// Scalar reference: branchless constant-time byte-slice equality (no early exit on mismatch).
pub fn scalarCryptoExtCtEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

/// 16-lane SIMD branchless constant-time byte-slice equality (accumulates XOR diff across all lanes,
/// zero early exits on content mismatch).
pub fn simdCryptoExtCtEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let acc_vec: CryptoExtVec16u8 = @splat(0);
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: CryptoExtVec16u8 = a[i .. i + 16][0..16].*;
        const vb: CryptoExtVec16u8 = b[i .. i + 16][0..16].*;
        acc_vec = acc_vec | (va ^ vb);
        i += 16;
    }
    let diff: u8 = @reduce(.Or, acc_vec);
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

/// Scalar reference: XOR `a[0..n]` and `b[0..n]` into `dst[0..n]` where `n = min(dst.len, a.len, b.len)`.
pub fn scalarCryptoExtXor(dst: *[]u8, a: []const u8, b: []const u8) {
    let n = if (dst.len < a.len) dst.len else a.len;
    if (b.len < n) n = b.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = a[i] ^ b[i];
    }
}

/// 16-lane SIMD accelerated byte-wise XOR (`dst[i] = a[i] ^ b[i]`).
pub fn simdCryptoExtXor(dst: *[]u8, a: []const u8, b: []const u8) {
    let n = if (dst.len < a.len) dst.len else a.len;
    if (b.len < n) n = b.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const va: CryptoExtVec16u8 = a[i .. i + 16][0..16].*;
        const vb: CryptoExtVec16u8 = b[i .. i + 16][0..16].*;
        dst[i .. i + 16][0..16].* = va ^ vb;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = a[i] ^ b[i];
    }
}

/// Scalar reference: copy bytes from `src` into `dst`.
pub fn scalarCryptoExtCopy(dst: *[]u8, src: []const u8) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane SIMD accelerated byte copy from `src` into `dst`.
pub fn simdCryptoExtCopy(dst: *[]u8, src: []const u8) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v: CryptoExtVec16u8 = src[i .. i + 16][0..16].*;
        dst[i .. i + 16][0..16].* = v;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane SIMD branchless constant-time check returning `true` iff every byte in `buf` is `0x00`.
pub fn isAllZeroBytesCt(buf: []const u8): bool {
    let acc_vec: CryptoExtVec16u8 = @splat(0);
    let i: usize = 0;
    while (i + 16 <= buf.len) {
        const v: CryptoExtVec16u8 = buf[i .. i + 16][0..16].*;
        acc_vec = acc_vec | v;
        i += 16;
    }
    let acc: u8 = @reduce(.Or, acc_vec);
    while (i < buf.len) : (i += 1) {
        acc = acc | buf[i];
    }
    return acc == 0;
}

// ============================================================================
// 3. RFC 5869 HKDF-SHA256 & RFC 2104 HMAC-SHA256 Primitives
// ============================================================================

/// RFC 5869 HKDF-Extract using HMAC-SHA256 (`PRK = HMAC-SHA256(salt, ikm)`).
pub fn ratchetHkdfExtract(salt: []const u8, ikm: []const u8): [RATCHET_KEY_LEN]u8 {
    return std.crypto.kdf.hkdf.HkdfSha256.extract(salt, ikm);
}

/// RFC 5869 HKDF-Expand using HMAC-SHA256.
/// Rejects `out.len > 255 * 32` (`8160` bytes) and returns `false` without mutating `out`.
pub fn ratchetHkdfExpand(prk: [RATCHET_KEY_LEN]u8, info: []const u8, out: *[]u8): bool {
    if (out.len > RATCHET_HKDF_MAX_OUTPUT_LEN) {
        return false;
    }
    if (out.len == 0) {
        return true;
    }
    std.crypto.kdf.hkdf.HkdfSha256.expand(out, info, prk);
    return true;
}

fn ratchetHkdfDeriveChainKey(ikm: []const u8, salt: []const u8, info: []const u8): [RATCHET_KEY_LEN]u8 {
    let prk: [RATCHET_KEY_LEN]u8 = std.crypto.kdf.hkdf.HkdfSha256.extract(salt, ikm);
    let okm: [RATCHET_KEY_LEN]u8 = [0; RATCHET_KEY_LEN];
    std.crypto.kdf.hkdf.HkdfSha256.expand(okm[0..], info, prk);
    std.crypto.secureZero(u8, prk[0..]);
    return okm;
}

fn ratchetHmacChainTag(chain_key: [RATCHET_KEY_LEN]u8, tag: u8): [RATCHET_KEY_LEN]u8 {
    const msg: [1]u8 = [tag];
    let out: [RATCHET_KEY_LEN]u8 = [0; RATCHET_KEY_LEN];
    std.crypto.auth.hmac.sha2.HmacSha256.create(&out, msg[0..], chain_key[0..]);
    return out;
}

// ============================================================================
// 4. Zero-Alloc Forward-Secret Symmetric Key Ratchet
// ============================================================================

/// Zero-heap-allocation Forward-Secret Symmetric Key Ratchet with inline 32-slot
/// out-of-order skipped-key cache and one-time replay protection.
pub struct SymmetricKeyRatchet {
    chain_key: [RATCHET_KEY_LEN]u8 = [0; RATCHET_KEY_LEN];
    current_step: u64 = 0;
    skipped: [RATCHET_MAX_SKIPPED_KEYS]RatchetSkippedKeySlot = [RatchetSkippedKeySlot{}; RATCHET_MAX_SKIPPED_KEYS];
    cached_skipped_count: usize = 0;
    total_derived: u64 = 0;
    total_out_of_order_hits: u64 = 0;
    rekey_count: u32 = 0;

    /// Derives the initial 32-byte `chain_key` via RFC 5869 HKDF-SHA256 (`Extract + Expand`).
    /// Returns `null` if `root_secret` is empty.
    static fn init(
        root_secret: []const u8,
        salt: []const u8,
        context_info: []const u8,
    ): SymmetricKeyRatchet? {
        if (root_secret.len == 0) return null;
        const init_ck: [RATCHET_KEY_LEN]u8 = ratchetHkdfDeriveChainKey(root_secret, salt, context_info);
        return SymmetricKeyRatchet{
            chain_key: init_ck,
            current_step: 0,
            skipped: [RatchetSkippedKeySlot{}; RATCHET_MAX_SKIPPED_KEYS],
            cached_skipped_count: 0,
            total_derived: 0,
            total_out_of_order_hits: 0,
            rekey_count: 0,
        };
    }

    /// Strict initializer rejecting weak secrets (`< 16` bytes or all-zero key material).
    static fn initStrict(
        root_secret: []const u8,
        salt: []const u8,
        context_info: []const u8,
    ): SymmetricKeyRatchet? {
        if (root_secret.len < RATCHET_MIN_STRICT_SECRET_LEN) return null;
        if (isAllZeroBytesCt(root_secret)) return null;
        return SymmetricKeyRatchet.init(root_secret, salt, context_info);
    }

    /// Internal helper: advances the ratchet by exactly one step, overwriting `chain_key`
    /// and scrubbing intermediate nonce derivation bytes via `std.crypto.secureZero`.
    fn advanceOneStep(*_): RatchetDerivedKey {
        const step: u64 = _.current_step;
        const msg_key: [RATCHET_KEY_LEN]u8 = ratchetHmacChainTag(_.chain_key, 0x01);
        let nonce_full: [RATCHET_KEY_LEN]u8 = ratchetHmacChainTag(_.chain_key, 0x03);
        let next_ck: [RATCHET_KEY_LEN]u8 = ratchetHmacChainTag(_.chain_key, 0x02);

        // Immediately overwrite previous chain key to enforce Forward Secrecy
        std.crypto.secureZero(u8, _.chain_key[0..]);
        simdCryptoExtCopy(_.chain_key[0..], next_ck[0..]);
        std.crypto.secureZero(u8, next_ck[0..]);

        let np: [12]u8 = [0; 12];
        simdCryptoExtCopy(np[0..], nonce_full[0..12]);
        std.crypto.secureZero(u8, nonce_full[0..]);

        _.current_step += 1;
        _.total_derived += 1;

        return RatchetDerivedKey{
            step_index: step,
            key: msg_key,
            nonce_prefix: np,
        };
    }

    /// Advances the ratchet by one step for sequential sending and returns the derived key & nonce prefix.
    fn next_send_key(*_): RatchetDerivedKey {
        return _.advanceOneStep();
    }

    /// Retrieves the derived key for `target_step`, supporting both in-order and out-of-order delivery:
    ///   - `target_step == current_step`: advances one step and returns immediately.
    ///   - `target_step > current_step`: atomically checks `skip <= RATCHET_MAX_SKIPPED_KEYS` and free cache
    ///     capacity; caches all skipped intermediate keys and returns `target_step`'s key (returns `null`
    ///     with zero state mutation if the skip exceeds bounds).
    ///   - `target_step < current_step`: looks up `target_step` in the skipped-key cache, scrubs via
    ///     `std.crypto.secureZero` and removes it upon return (one-time consumption prevents replay).
    fn receive_key_for_step(*_, target_step: u64): RatchetDerivedKey? {
        if (target_step == _.current_step) {
            return _.advanceOneStep();
        }

        if (target_step < _.current_step) {
            let i: usize = 0;
            while (i < RATCHET_MAX_SKIPPED_KEYS) : (i += 1) {
                if (_.skipped[i].occupied && _.skipped[i].step_index == target_step) {
                    const found = RatchetDerivedKey{
                        step_index: _.skipped[i].step_index,
                        key: _.skipped[i].key,
                        nonce_prefix: _.skipped[i].nonce_prefix,
                    };
                    // One-time consumption + secureZero scrubbing (anti-replay)
                    std.crypto.secureZero(u8, _.skipped[i].key[0..]);
                    std.crypto.secureZero(u8, _.skipped[i].nonce_prefix[0..]);
                    _.skipped[i].step_index = 0;
                    _.skipped[i].occupied = false;
                    _.cached_skipped_count -= 1;
                    _.total_out_of_order_hits += 1;
                    return found;
                }
            }
            return null;
        }

        // Forward skip: atomic anti-DoS and cache capacity pre-flight guard
        const skip_u64: u64 = target_step - _.current_step;
        if (skip_u64 > (RATCHET_MAX_SKIPPED_KEYS as u64)) return null;
        const skip_count: usize = @intCast(skip_u64);
        const free_slots: usize = RATCHET_MAX_SKIPPED_KEYS - _.cached_skipped_count;
        if (skip_count > free_slots) return null;

        let s: usize = 0;
        while (s < skip_count) : (s += 1) {
            const skipped_key = _.advanceOneStep();
            let slot_idx: usize = 0;
            while (slot_idx < RATCHET_MAX_SKIPPED_KEYS) : (slot_idx += 1) {
                if (!_.skipped[slot_idx].occupied) {
                    _.skipped[slot_idx] = RatchetSkippedKeySlot{
                        step_index: skipped_key.step_index,
                        key: skipped_key.key,
                        nonce_prefix: skipped_key.nonce_prefix,
                        occupied: true,
                    };
                    _.cached_skipped_count += 1;
                    break;
                }
            }
        }

        return _.advanceOneStep();
    }

    /// Mixes fresh `new_entropy` into the current `chain_key` via HKDF-SHA256 for Post-Compromise Security,
    /// incrementing `rekey_count`. Rejects empty `new_entropy`, returning `false`.
    fn rekey(*_, new_entropy: []const u8, info: []const u8): bool {
        if (new_entropy.len == 0) return false;
        let mixed: [RATCHET_KEY_LEN]u8 = ratchetHkdfDeriveChainKey(new_entropy, _.chain_key[0..], info);
        std.crypto.secureZero(u8, _.chain_key[0..]);
        simdCryptoExtCopy(_.chain_key[0..], mixed[0..]);
        std.crypto.secureZero(u8, mixed[0..]);
        _.rekey_count += 1;
        return true;
    }

    /// Strict Post-Compromise Security rekey rejecting weak (`< 16` bytes) or all-zero `new_entropy`.
    fn rekeyStrict(*_, new_entropy: []const u8, info: []const u8): bool {
        if (new_entropy.len < RATCHET_MIN_STRICT_SECRET_LEN) return false;
        if (isAllZeroBytesCt(new_entropy)) return false;
        return _.rekey(new_entropy, info);
    }

    /// Securely scrubs `chain_key` and all cached skipped keys in place via `std.crypto.secureZero`.
    fn zeroize(*_) {
        std.crypto.secureZero(u8, _.chain_key[0..]);
        let s: usize = 0;
        while (s < RATCHET_MAX_SKIPPED_KEYS) : (s += 1) {
            std.crypto.secureZero(u8, _.skipped[s].key[0..]);
            std.crypto.secureZero(u8, _.skipped[s].nonce_prefix[0..]);
            _.skipped[s].step_index = 0;
            _.skipped[s].occupied = false;
        }
        _.cached_skipped_count = 0;
    }

    /// Returns a cumulative telemetry & state snapshot.
    fn stats(): RatchetStats {
        return RatchetStats{
            current_step: _.current_step,
            cached_skipped_keys: _.cached_skipped_count,
            total_derived: _.total_derived,
            total_out_of_order_hits: _.total_out_of_order_hits,
            rekey_count: _.rekey_count,
        };
    }
}
