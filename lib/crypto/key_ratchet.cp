// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Forward-Secret Symmetric Key Ratchet) | Created: 2026-11-22
// Pure CP zero-heap-allocation Signal/TLS-1.3 style Forward-Secret Symmetric Key Ratchet & Out-of-Order Key Cache:
//   - RFC 5869 HKDF-SHA256 initialization (`Extract + Expand`) with salt & context_info domain separation
//   - One-way HMAC-SHA256 ratchet advancement (`0x01` msg_key, `0x03` 12B nonce_prefix, `0x02` next chain_key)
//     immediately overwriting prior chain_key for strict Forward Secrecy
//   - Fixed inline 32-slot skipped-key cache (`RATCHET_MAX_SKIPPED_KEYS`) supporting out-of-order delivery,
//     strict one-time consumption (replay rejection), and atomic DoS-resistant max-skip bounds
//   - Post-Compromise Security via HKDF entropy mixing (`rekey`) and explicit memory scrubbing (`zeroize`)

const std = @import("std");

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Symmetric message & chain key byte length (256-bit).
pub const RATCHET_KEY_LEN: usize = 32;

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
struct SkippedKeySlot {
    step_index: u64 = 0;
    key: [RATCHET_KEY_LEN]u8 = [0; RATCHET_KEY_LEN];
    nonce_prefix: [12]u8 = [0; 12];
    occupied: bool = false;
}

// ============================================================================
// 2. Internal Zero-Alloc HKDF-SHA256 & HMAC-SHA256 Helpers
// ============================================================================

fn hkdfDeriveChainKey(ikm: []const u8, salt: []const u8, info: []const u8): [RATCHET_KEY_LEN]u8 {
    const prk: [RATCHET_KEY_LEN]u8 = std.crypto.kdf.hkdf.HkdfSha256.extract(salt, ikm);
    let okm: [RATCHET_KEY_LEN]u8 = [0; RATCHET_KEY_LEN];
    std.crypto.kdf.hkdf.HkdfSha256.expand(okm[0..], info, prk);
    return okm;
}

fn hmacChainTag(chain_key: [RATCHET_KEY_LEN]u8, tag: u8): [RATCHET_KEY_LEN]u8 {
    const msg: [1]u8 = [tag];
    let out: [RATCHET_KEY_LEN]u8 = [0; RATCHET_KEY_LEN];
    std.crypto.auth.hmac.sha2.HmacSha256.create(&out, msg[0..], chain_key[0..]);
    return out;
}

// ============================================================================
// 3. Zero-Alloc Forward-Secret Symmetric Key Ratchet
// ============================================================================

/// Zero-heap-allocation Forward-Secret Symmetric Key Ratchet with inline 32-slot
/// out-of-order skipped-key cache and one-time replay protection.
pub struct SymmetricKeyRatchet {
    chain_key: [RATCHET_KEY_LEN]u8 = [0; RATCHET_KEY_LEN];
    current_step: u64 = 0;
    skipped: [RATCHET_MAX_SKIPPED_KEYS]SkippedKeySlot = [SkippedKeySlot{}; RATCHET_MAX_SKIPPED_KEYS];
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
        const init_ck: [RATCHET_KEY_LEN]u8 = hkdfDeriveChainKey(root_secret, salt, context_info);
        return SymmetricKeyRatchet{
            chain_key: init_ck,
            current_step: 0,
            skipped: [SkippedKeySlot{}; RATCHET_MAX_SKIPPED_KEYS],
            cached_skipped_count: 0,
            total_derived: 0,
            total_out_of_order_hits: 0,
            rekey_count: 0,
        };
    }

    /// Internal helper: advances the ratchet by exactly one step, overwriting `chain_key`.
    fn advanceOneStep(*_): RatchetDerivedKey {
        const step: u64 = _.current_step;
        const msg_key: [RATCHET_KEY_LEN]u8 = hmacChainTag(_.chain_key, 0x01);
        const nonce_full: [RATCHET_KEY_LEN]u8 = hmacChainTag(_.chain_key, 0x03);
        const next_ck: [RATCHET_KEY_LEN]u8 = hmacChainTag(_.chain_key, 0x02);

        // Immediately overwrite previous chain key to enforce Forward Secrecy
        _.chain_key = next_ck;

        let np: [12]u8 = [0; 12];
        let i: usize = 0;
        while (i < 12) : (i += 1) {
            np[i] = nonce_full[i];
        }

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
    ///   - `target_step < current_step`: looks up `target_step` in the skipped-key cache, zeroizes and
    ///     removes it upon return (one-time consumption prevents replay); returns `null` if absent.
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
                    // One-time consumption + immediate zeroization (anti-replay)
                    let k: usize = 0;
                    while (k < RATCHET_KEY_LEN) : (k += 1) {
                        _.skipped[i].key[k] = 0;
                    }
                    let n: usize = 0;
                    while (n < 12) : (n += 1) {
                        _.skipped[i].nonce_prefix[n] = 0;
                    }
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
                    _.skipped[slot_idx] = SkippedKeySlot{
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
        const mixed: [RATCHET_KEY_LEN]u8 = hkdfDeriveChainKey(new_entropy, _.chain_key[0..], info);
        _.chain_key = mixed;
        _.rekey_count += 1;
        return true;
    }

    /// Securely scrubs `chain_key` and all cached skipped keys in place to all-zero bytes.
    fn zeroize(*_) {
        let i: usize = 0;
        while (i < RATCHET_KEY_LEN) : (i += 1) {
            _.chain_key[i] = 0;
        }
        let s: usize = 0;
        while (s < RATCHET_MAX_SKIPPED_KEYS) : (s += 1) {
            let k: usize = 0;
            while (k < RATCHET_KEY_LEN) : (k += 1) {
                _.skipped[s].key[k] = 0;
            }
            let n: usize = 0;
            while (n < 12) : (n += 1) {
                _.skipped[s].nonce_prefix[n] = 0;
            }
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
