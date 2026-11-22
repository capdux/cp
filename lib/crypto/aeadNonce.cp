// Origin: Pure-CP Stdlib (Zero-Alloc RFC 9001 AEAD Nonce Deriver, Reuse Guard & 128-Bit Anti-Replay Window) | Created: 2026-11-23
// Pure CP zero-heap-allocation TLS 1.3 / QUIC (RFC 9001 §5.3) AEAD nonce management & anti-replay engine:
//   - Deterministic 96-bit (12-byte) AEAD nonce derivation (`deriveAeadNonce`): XORs big-endian 64-bit sequence
//     number into the low 8 bytes of `base_iv`
//   - Monotonic `AeadNonceSender` with hard `rekey_limit` ceiling & `needsRekey()` warning watermark,
//     structurally eliminating catastrophic AES-GCM / ChaCha20-Poly1305 nonce reuse
//   - O(1) 128-bit sliding anti-replay bitmap (`AntiReplayWindow128`) supporting up to 128 out-of-order
//     packets, distinguishing `accepted | duplicate_replay | too_old_below_window`, with read-only pre-check
//     (`checkOnly`) and post-AEAD-verify commit (`checkAndMark`)
//   - Epoch-tracked `rekey(*_, new_base_iv)` on both sender and `AeadNonceReceiver`, plus zero-alloc telemetry

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Standard 96-bit AEAD nonce / IV length in bytes (AES-GCM / ChaCha20-Poly1305).
pub const AEAD_NONCE_LEN: usize = 12;

/// Width of the sliding anti-replay bitmap window in packets (128 out-of-order packets).
pub const AEAD_ANTI_REPLAY_WINDOW_BITS: u64 = 128;

/// Conservative default hard packet limit per key epoch (`2^23` packets, per RFC 9001 AES-GCM guidance).
pub const AEAD_DEFAULT_REKEY_LIMIT: u64 = 8_388_608;

/// Verdict returned by `AntiReplayWindow128.checkOnly` / `checkAndMark`.
pub enum AntiReplayVerdict {
    accepted,
    duplicate_replay,
    too_old_below_window,
}

/// Nonce ticket issued by `AeadNonceSender.nextNonce`.
pub struct AeadNonceTicket {
    seq: u64 = 0,
    key_epoch: u64 = 0,
    nonce: [AEAD_NONCE_LEN]u8 = [0; AEAD_NONCE_LEN],
}

/// Telemetry snapshot for `AeadNonceSender`.
pub struct AeadNonceSenderStats {
    key_epoch: u64 = 0,
    next_seq: u64 = 0,
    rekey_limit: u64 = 0,
    warn_threshold: u64 = 0,
    remaining_packets: u64 = 0,
    needs_rekey: bool = false,
    exhausted: bool = false,
    total_derived: u64 = 0,
    total_rekeys: u64 = 0,
    total_rejected_exhausted: u64 = 0,
}

/// Telemetry snapshot for `AntiReplayWindow128`.
pub struct AntiReplayStats {
    key_epoch: u64 = 0,
    has_received: bool = false,
    highest_seq: u64 = 0,
    window_size: u64 = AEAD_ANTI_REPLAY_WINDOW_BITS,
    total_accepted: u64 = 0,
    total_replays: u64 = 0,
    total_too_old: u64 = 0,
    total_rekeys: u64 = 0,
}

// ============================================================================
// 2. RFC 9001 / TLS 1.3 Deterministic 96-Bit Nonce Derivation
// ============================================================================

/// Derives a 96-bit AEAD nonce per RFC 9001 §5.3 / RFC 8446 §5.3:
/// copies `base_iv[0..4]` verbatim and XORs the 64-bit big-endian `seq` into `base_iv[4..12]`.
pub fn deriveAeadNonce(base_iv: [AEAD_NONCE_LEN]u8, seq: u64): [AEAD_NONCE_LEN]u8 {
    return [
        base_iv[0],
        base_iv[1],
        base_iv[2],
        base_iv[3],
        base_iv[4] ^ (((seq >> 56) & 0xFF) as u8),
        base_iv[5] ^ (((seq >> 48) & 0xFF) as u8),
        base_iv[6] ^ (((seq >> 40) & 0xFF) as u8),
        base_iv[7] ^ (((seq >> 32) & 0xFF) as u8),
        base_iv[8] ^ (((seq >> 24) & 0xFF) as u8),
        base_iv[9] ^ (((seq >> 16) & 0xFF) as u8),
        base_iv[10] ^ (((seq >> 8) & 0xFF) as u8),
        base_iv[11] ^ ((seq & 0xFF) as u8),
    ];
}

// ============================================================================
// 3. Monotonic Nonce Sender with Hard Reuse Guard & Rekey Epoch
// ============================================================================

/// Monotonic AEAD nonce generator with hard `rekey_limit` enforcement and key-epoch tracking.
pub struct AeadNonceSender {
    base_iv: [AEAD_NONCE_LEN]u8 = [0; AEAD_NONCE_LEN],
    key_epoch: u64 = 0,
    next_seq: u64 = 0,
    rekey_limit: u64 = AEAD_DEFAULT_REKEY_LIMIT,
    warn_threshold: u64 = (AEAD_DEFAULT_REKEY_LIMIT * 80) / 100,
    total_derived: u64 = 0,
    total_rekeys: u64 = 0,
    total_rejected_exhausted: u64 = 0,

    /// Initializes a sender with explicit `rekey_limit` and `warn_threshold`.
    /// Returns `null` if `rekey_limit == 0` or `warn_threshold > rekey_limit`.
    /// Passing `warn_threshold == 0` automatically selects 80% of `rekey_limit`.
    pub fn init(
        base_iv: [AEAD_NONCE_LEN]u8,
        rekey_limit: u64,
        warn_threshold: u64,
    ): AeadNonceSender? {
        if (rekey_limit == 0 || warn_threshold > rekey_limit) return null;
        const resolved_warn: u64 = if (warn_threshold == 0)
            ((rekey_limit * 80) / 100)
        else
            warn_threshold;

        return AeadNonceSender{
            base_iv: base_iv,
            rekey_limit: rekey_limit,
            warn_threshold: resolved_warn,
        };
    }

    /// Initializes a sender with RFC-conservative defaults (`rekey_limit = 2^23`, 80% warning).
    pub static fn initDefault(base_iv: [AEAD_NONCE_LEN]u8): AeadNonceSender {
        return AeadNonceSender{
            base_iv: base_iv,
            rekey_limit: AEAD_DEFAULT_REKEY_LIMIT,
            warn_threshold: (AEAD_DEFAULT_REKEY_LIMIT * 80) / 100,
        };
    }

    /// Issues the next unique nonce ticket, advancing `next_seq` monotonically.
    /// Returns `null` once `next_seq >= rekey_limit` to prevent nonce reuse.
    pub fn nextNonce(*_): AeadNonceTicket? {
        if (_.next_seq >= _.rekey_limit) {
            _.total_rejected_exhausted += 1;
            return null;
        }
        const seq = _.next_seq;
        _.next_seq += 1;
        _.total_derived += 1;
        return AeadNonceTicket{
            seq: seq,
            key_epoch: _.key_epoch,
            nonce: deriveAeadNonce(_.base_iv, seq),
        };
    }

    /// Returns `true` when `next_seq` has reached or exceeded the proactive rekey warning threshold.
    pub fn needsRekey(): bool {
        return _.next_seq >= _.warn_threshold;
    }

    /// Returns `true` when `next_seq` has hit the hard `rekey_limit` ceiling.
    pub fn isExhausted(): bool {
        return _.next_seq >= _.rekey_limit;
    }

    /// Returns the number of packets remaining before `rekey_limit` is reached.
    pub fn remaining(): u64 {
        return if (_.next_seq >= _.rekey_limit) 0 else (_.rekey_limit - _.next_seq);
    }

    /// Rotates to a fresh key epoch with `new_base_iv`, incrementing `key_epoch` and resetting `next_seq` to 0.
    pub fn rekey(*_, new_base_iv: [AEAD_NONCE_LEN]u8) {
        _.base_iv = new_base_iv;
        _.key_epoch += 1;
        _.next_seq = 0;
        _.total_rekeys += 1;
    }

    /// Returns a complete sender telemetry snapshot.
    pub fn stats(): AeadNonceSenderStats {
        return AeadNonceSenderStats{
            key_epoch: _.key_epoch,
            next_seq: _.next_seq,
            rekey_limit: _.rekey_limit,
            warn_threshold: _.warn_threshold,
            remaining_packets: _.remaining(),
            needs_rekey: _.needsRekey(),
            exhausted: _.isExhausted(),
            total_derived: _.total_derived,
            total_rekeys: _.total_rekeys,
            total_rejected_exhausted: _.total_rejected_exhausted,
        };
    }
}

// ============================================================================
// 4. 128-Bit Sliding Anti-Replay Window & Receiver Helper
// ============================================================================

/// Zero-heap-allocation 128-bit sliding anti-replay bitmap (O(1) duplicate & stale detection).
pub struct AntiReplayWindow128 {
    key_epoch: u64 = 0,
    has_received: bool = false,
    highest_seq: u64 = 0,
    bitmap_lo: u64 = 0,
    bitmap_hi: u64 = 0,
    total_accepted: u64 = 0,
    total_replays: u64 = 0,
    total_too_old: u64 = 0,
    total_rekeys: u64 = 0,

    /// Creates a fresh empty 128-bit anti-replay window at `key_epoch = 0`.
    pub fn init(): AntiReplayWindow128 {
        return AntiReplayWindow128{};
    }

    /// Pure read-only check suitable for pre-filtering packets BEFORE AEAD tag verification.
    pub fn checkOnly(seq: u64): AntiReplayVerdict {
        if (!_.has_received) return AntiReplayVerdict.accepted;
        if (seq > _.highest_seq) return AntiReplayVerdict.accepted;

        const delta: u64 = _.highest_seq - seq;
        if (delta >= AEAD_ANTI_REPLAY_WINDOW_BITS) {
            return AntiReplayVerdict.too_old_below_window;
        }
        if (delta < 64) {
            const bit: u64 = (1 as u64) << (delta as u6);
            return if ((_.bitmap_lo & bit) != 0) AntiReplayVerdict.duplicate_replay else AntiReplayVerdict.accepted;
        }
        const hi_bit: u64 = (1 as u64) << ((delta - 64) as u6);
        return if ((_.bitmap_hi & hi_bit) != 0) AntiReplayVerdict.duplicate_replay else AntiReplayVerdict.accepted;
    }

    /// Checks `seq` and atomically marks it received if accepted (call after AEAD verification succeeds).
    pub fn checkAndMark(*_, seq: u64): AntiReplayVerdict {
        if (!_.has_received) {
            _.has_received = true;
            _.highest_seq = seq;
            _.bitmap_lo = 1;
            _.bitmap_hi = 0;
            _.total_accepted += 1;
            return AntiReplayVerdict.accepted;
        }

        if (seq > _.highest_seq) {
            const shift: u64 = seq - _.highest_seq;
            if (shift >= AEAD_ANTI_REPLAY_WINDOW_BITS) {
                _.bitmap_lo = 1;
                _.bitmap_hi = 0;
            } else if (shift >= 64) {
                const s: u6 = (shift - 64) as u6;
                _.bitmap_hi = _.bitmap_lo << s;
                _.bitmap_lo = 1;
            } else {
                const s: u6 = shift as u6;
                const carry_s: u6 = (64 - shift) as u6;
                _.bitmap_hi = (_.bitmap_hi << s) | (_.bitmap_lo >> carry_s);
                _.bitmap_lo = (_.bitmap_lo << s) | 1;
            }
            _.highest_seq = seq;
            _.total_accepted += 1;
            return AntiReplayVerdict.accepted;
        }

        const delta: u64 = _.highest_seq - seq;
        if (delta >= AEAD_ANTI_REPLAY_WINDOW_BITS) {
            _.total_too_old += 1;
            return AntiReplayVerdict.too_old_below_window;
        }

        if (delta < 64) {
            const bit: u64 = (1 as u64) << (delta as u6);
            if ((_.bitmap_lo & bit) != 0) {
                _.total_replays += 1;
                return AntiReplayVerdict.duplicate_replay;
            }
            _.bitmap_lo |= bit;
            _.total_accepted += 1;
            return AntiReplayVerdict.accepted;
        }

        const hi_bit: u64 = (1 as u64) << ((delta - 64) as u6);
        if ((_.bitmap_hi & hi_bit) != 0) {
            _.total_replays += 1;
            return AntiReplayVerdict.duplicate_replay;
        }
        _.bitmap_hi |= hi_bit;
        _.total_accepted += 1;
        return AntiReplayVerdict.accepted;
    }

    /// Resets the sliding window for a new key epoch, incrementing `key_epoch`.
    pub fn rekey(*_) {
        _.key_epoch += 1;
        _.has_received = false;
        _.highest_seq = 0;
        _.bitmap_lo = 0;
        _.bitmap_hi = 0;
        _.total_rekeys += 1;
    }

    /// Returns a telemetry snapshot of the anti-replay window.
    pub fn stats(): AntiReplayStats {
        return AntiReplayStats{
            key_epoch: _.key_epoch,
            has_received: _.has_received,
            highest_seq: _.highest_seq,
            window_size: AEAD_ANTI_REPLAY_WINDOW_BITS,
            total_accepted: _.total_accepted,
            total_replays: _.total_replays,
            total_too_old: _.total_too_old,
            total_rekeys: _.total_rekeys,
        };
    }
}

/// Combined AEAD receiver pairing `base_iv` nonce derivation with a 128-bit `AntiReplayWindow128`.
pub struct AeadNonceReceiver {
    base_iv: [AEAD_NONCE_LEN]u8 = [0; AEAD_NONCE_LEN],
    window: AntiReplayWindow128 = AntiReplayWindow128{},

    /// Initializes a receiver with `base_iv` at `key_epoch = 0`.
    pub fn init(base_iv: [AEAD_NONCE_LEN]u8): AeadNonceReceiver {
        return AeadNonceReceiver{
            base_iv: base_iv,
            window: AntiReplayWindow128.init(),
        };
    }

    /// Derives the expected 96-bit AEAD nonce for `seq` in the current key epoch.
    pub fn deriveForSeq(seq: u64): [AEAD_NONCE_LEN]u8 {
        return deriveAeadNonce(_.base_iv, seq);
    }

    /// Read-only pre-check for `seq` before AEAD decryption.
    pub fn checkOnly(seq: u64): AntiReplayVerdict {
        return _.window.checkOnly(seq);
    }

    /// Marks `seq` accepted in the anti-replay window (invoke after AEAD tag verification succeeds).
    pub fn checkAndMark(*_, seq: u64): AntiReplayVerdict {
        return _.window.checkAndMark(seq);
    }

    /// Rotates to `new_base_iv`, incrementing `key_epoch` and resetting the anti-replay window.
    pub fn rekey(*_, new_base_iv: [AEAD_NONCE_LEN]u8) {
        _.base_iv = new_base_iv;
        _.window.rekey();
    }

    /// Returns anti-replay window telemetry.
    pub fn stats(): AntiReplayStats {
        return _.window.stats();
    }
}
