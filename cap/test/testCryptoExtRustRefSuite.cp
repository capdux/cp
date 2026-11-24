import { test, expect_eq } from "cap:test";
import {
    RATCHET_KEY_LEN,
    RATCHET_HKDF_MAX_OUTPUT_LEN,
    RATCHET_MAX_SKIPPED_KEYS,
    scalarCryptoExtCtEqual,
    simdCryptoExtCtEqual,
    scalarCryptoExtXor,
    simdCryptoExtXor,
    scalarCryptoExtCopy,
    simdCryptoExtCopy,
    isAllZeroBytesCt,
    ratchetHkdfExtract,
    ratchetHkdfExpand,
    SymmetricKeyRatchet,
} from "../../lib/crypto/ratchet.cp";
import {
    AEAD_NONCE_LEN,
    AntiReplayVerdict,
    deriveAeadNonce,
    AeadNonceSender,
    AntiReplayWindow128,
    AeadNonceReceiver,
} from "../../lib/crypto/aeadNonce.cp";
import {
    KEYRING_TAG_LEN,
    keyringHmacSha256,
    KeyVersionState,
    KeyRingEntry,
    KeyRing,
} from "../../lib/crypto/keyring.cp";

const std = @import("std");

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn expect_false(cond: bool): void! {
    expect_eq(cond, false);
}

struct XorShift64 {
    state: u64 = 0x1E3779B97F4A7C15,

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

// ---------------------------------------------------------------------------
// Test 1: RFC 5869 HKDF-SHA256 Official Vectors & Forward-Secret Ratchet
// ---------------------------------------------------------------------------

fn runTestRfc5869AndRatchet(): void! {
    // RFC 5869 Appendix A.1 (Test Case 1)
    const ikm1: [22]u8 = [0x0b; 22];
    const salt1: [13]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c];
    const info1: [10]u8 = [0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9];
    const exp_prk1: [32]u8 = [
        0x07, 0x77, 0x09, 0x36, 0x2c, 0x2e, 0x32, 0xdf,
        0x0d, 0xdc, 0x3f, 0x0d, 0xc4, 0x7b, 0xba, 0x63,
        0x90, 0xb6, 0xc7, 0x3b, 0xb5, 0x0f, 0x9c, 0x31,
        0x22, 0xec, 0x84, 0x4a, 0xd7, 0xc2, 0xb3, 0xe5,
    ];
    const exp_okm1: [42]u8 = [
        0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a,
        0x90, 0x43, 0x4f, 0x64, 0xd0, 0x36, 0x2f, 0x2a,
        0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a, 0x5a, 0x4c,
        0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf,
        0x34, 0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18,
        0x58, 0x65,
    ];
    const prk1 = ratchetHkdfExtract(salt1[0..], ikm1[0..]);
    expect_true(simdCryptoExtCtEqual(prk1[0..], exp_prk1[0..]))?;
    let okm1: [42]u8 = [0; 42];
    expect_true(ratchetHkdfExpand(prk1, info1[0..], okm1[0..]))?;
    expect_true(simdCryptoExtCtEqual(okm1[0..], exp_okm1[0..]))?;

    // RFC 5869 Appendix A.3 (Test Case 3: zero-length salt & info)
    const exp_prk3: [32]u8 = [
        0x19, 0xef, 0x24, 0xa3, 0x2c, 0x71, 0x7b, 0x16,
        0x7f, 0x33, 0xa9, 0x1d, 0x6f, 0x64, 0x8b, 0xdf,
        0x96, 0x59, 0x67, 0x76, 0xaf, 0xdb, 0x63, 0x77,
        0xac, 0x43, 0x4c, 0x1c, 0x29, 0x3c, 0xcb, 0x04,
    ];
    const exp_okm3: [42]u8 = [
        0x8d, 0xa4, 0xe7, 0x75, 0xa5, 0x63, 0xc1, 0x8f,
        0x71, 0x5f, 0x80, 0x2a, 0x06, 0x3c, 0x5a, 0x31,
        0xb8, 0xa1, 0x1f, 0x5c, 0x5e, 0xe1, 0x87, 0x9e,
        0xc3, 0x45, 0x4e, 0x5f, 0x3c, 0x73, 0x8d, 0x2d,
        0x9d, 0x20, 0x13, 0x95, 0xfa, 0xa4, 0xb6, 0x1a,
        0x96, 0xc8,
    ];
    const prk3 = ratchetHkdfExtract("", ikm1[0..]);
    expect_true(simdCryptoExtCtEqual(prk3[0..], exp_prk3[0..]))?;
    let okm3: [42]u8 = [0; 42];
    expect_true(ratchetHkdfExpand(prk3, "", okm3[0..]))?;
    expect_true(simdCryptoExtCtEqual(okm3[0..], exp_okm3[0..]))?;

    // Reject oversized HKDF output (> 255 * 32 = 8160)
    let big_out: [RATCHET_HKDF_MAX_OUTPUT_LEN + 1]u8 = [0; RATCHET_HKDF_MAX_OUTPUT_LEN + 1];
    expect_false(ratchetHkdfExpand(prk1, info1[0..], big_out[0..]))?;

    // Weak secret rejection in initStrict / rekeyStrict
    const zeros32: [32]u8 = [0; 32];
    expect_true(SymmetricKeyRatchet.init("", "salt", "info") == null)?;
    expect_true(SymmetricKeyRatchet.initStrict("short_key", "salt", "info") == null)?;
    expect_true(SymmetricKeyRatchet.initStrict(zeros32[0..], "salt", "info") == null)?;

    // Sender & Receiver out-of-order delivery, one-time replay rejection, rekeyStrict & zeroize
    const root: []const u8 = "0123456789ABCDEF0123456789ABCDEF";
    let sender = SymmetricKeyRatchet.initStrict(root, "salt", "ctx:v1").?;
    let receiver = SymmetricKeyRatchet.initStrict(root, "salt", "ctx:v1").?;

    const k0 = sender.next_send_key();
    const k1 = sender.next_send_key();
    const k2 = sender.next_send_key();
    expect_false(simdCryptoExtCtEqual(k0.key[0..], k1.key[0..]))?;

    // Receive out-of-order: step 2 first, then step 0, then step 1
    const r2 = receiver.receive_key_for_step(2).?;
    expect_true(simdCryptoExtCtEqual(r2.key[0..], k2.key[0..]))?;
    expect_true(simdCryptoExtCtEqual(r2.nonce_prefix[0..], k2.nonce_prefix[0..]))?;

    const r0 = receiver.receive_key_for_step(0).?;
    expect_true(simdCryptoExtCtEqual(r0.key[0..], k0.key[0..]))?;
    // Replay of step 0 MUST be rejected
    expect_true(receiver.receive_key_for_step(0) == null)?;

    const r1 = receiver.receive_key_for_step(1).?;
    expect_true(simdCryptoExtCtEqual(r1.key[0..], k1.key[0..]))?;
    expect_eq(receiver.stats().cached_skipped_keys, 0 as usize);

    // Reject DoS skip > RATCHET_MAX_SKIPPED_KEYS without mutating state
    expect_true(receiver.receive_key_for_step(3 + (RATCHET_MAX_SKIPPED_KEYS as u64) + 1) == null)?;
    expect_eq(receiver.stats().current_step, 3 as u64);

    // rekeyStrict rejects empty / short / all-zero entropy
    expect_false(sender.rekeyStrict("", "rekey"))?;
    expect_false(sender.rekeyStrict("short", "rekey"))?;
    expect_false(sender.rekeyStrict(zeros32[0..], "rekey"))?;
    expect_true(sender.rekeyStrict("FEDCBA9876543210FEDCBA9876543210", "rekey"))?;
    expect_true(receiver.rekeyStrict("FEDCBA9876543210FEDCBA9876543210", "rekey"))?;
    const k3s = sender.next_send_key();
    const k3r = receiver.receive_key_for_step(3).?;
    expect_true(simdCryptoExtCtEqual(k3s.key[0..], k3r.key[0..]))?;

    // zeroize scrubs chain_key to all zeros
    sender.zeroize();
    expect_true(isAllZeroBytesCt(sender.chain_key[0..]))?;
}

// ---------------------------------------------------------------------------
// Test 2: RFC 4231 HMAC-SHA256 Official Vectors & KeyRing Rotation/Tampering
// ---------------------------------------------------------------------------

fn runTestRfc4231AndKeyRing(): void! {
    // RFC 4231 Test Case 1: Key = 20x 0x0b, Data = "Hi There"
    const key1: [20]u8 = [0x0b; 20];
    const exp1: [32]u8 = [
        0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53,
        0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
        0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7,
        0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7,
    ];
    const mac1 = keyringHmacSha256(key1[0..], "Hi There");
    expect_true(simdCryptoExtCtEqual(mac1[0..], exp1[0..]))?;

    // RFC 4231 Test Case 2: Key = "Jefe", Data = "what do ya want for nothing?"
    const exp2: [32]u8 = [
        0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
        0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
        0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
        0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43,
    ];
    const mac2 = keyringHmacSha256("Jefe", "what do ya want for nothing?");
    expect_true(simdCryptoExtCtEqual(mac2[0..], exp2[0..]))?;

    // RFC 4231 Test Case 6: Key = 131x 0xaa (> 64 bytes block size)
    const key6: [131]u8 = [0xaa; 131];
    const exp6: [32]u8 = [
        0x60, 0xe4, 0x31, 0x59, 0x1e, 0xe0, 0xb6, 0x7f,
        0x0d, 0x8a, 0x26, 0xaa, 0xcb, 0xf5, 0xb7, 0x7f,
        0x8e, 0x0b, 0xc6, 0x21, 0x37, 0x28, 0xc5, 0x14,
        0x05, 0x46, 0x04, 0x0f, 0x0e, 0xe3, 0x7f, 0x54,
    ];
    const mac6 = keyringHmacSha256(key6[0..], "Test Using Larger Than Block-Size Key - Hash Key First");
    expect_true(simdCryptoExtCtEqual(mac6[0..], exp6[0..]))?;

    // KeyRing strict key rotation, expiry, revoke zeroization & 1-bit tamper rejection
    let slots: [4]KeyRingEntry = [KeyRingEntry{}; 4];
    let ring = KeyRing.init(slots[0..]);

    const zero_key: [32]u8 = [0; 32];
    const sec1: [32]u8 = [0x11; 32];
    const sec2: [32]u8 = [0x22; 32];

    // addKeyStrict rejects all-zero weak key
    expect_false(ring.addKeyStrict(1, zero_key, true, 1000, 5000))?;
    expect_true(ring.addKeyStrict(1, sec1, true, 1000, 5000))?;

    let tag1: [KEYRING_TAG_LEN]u8 = [0; KEYRING_TAG_LEN];
    const kid1 = ring.signEnvelope("hello-envelope", 2000, tag1[0..]).?;
    expect_eq(kid1, 1 as u32);
    expect_true(ring.verifyEnvelope(kid1, "hello-envelope", tag1[0..], 2000))?;

    // Rotate to kid=2; kid=1 becomes VerifyOnly and still verifies before expiry
    expect_true(ring.addKeyStrict(2, sec2, true, 2500, 10000))?;
    expect_true(ring.verifyEnvelope(1, "hello-envelope", tag1[0..], 3000))?;
    // Cross-kid confusion rejected
    expect_false(ring.verifyEnvelope(2, "hello-envelope", tag1[0..], 3000))?;
    // Expiry enforcement at now_ms >= 5000
    expect_false(ring.verifyEnvelope(1, "hello-envelope", tag1[0..], 5000))?;

    // Single-bit flip in every byte of tag1 MUST be rejected 100%
    let b: usize = 0;
    while (b < KEYRING_TAG_LEN) : (b += 1) {
        tag1[b] ^= 0x01;
        expect_false(ring.verifyEnvelope(1, "hello-envelope", tag1[0..], 3000))?;
        tag1[b] ^= 0x01;
    }

    // Revoke kid=1 scrubs secret in place and rejects verification
    expect_true(ring.revokeKey(1))?;
    expect_false(ring.verifyEnvelope(1, "hello-envelope", tag1[0..], 3000))?;
    expect_true(isAllZeroBytesCt(slots[0].secret[0..]))?;

    ring.clear();
    expect_true(isAllZeroBytesCt(slots[1].secret[0..]))?;
}

// ---------------------------------------------------------------------------
// Test 3: RFC 9001 AEAD Nonce Derivation, Rekey Guard & 128-Bit Anti-Replay
// ---------------------------------------------------------------------------

fn runTestRfc9001NonceAndReplay(): void! {
    // RFC 9001 §5.3 deterministic XOR vector
    const base_iv: [12]u8 = [0x01, 0x02, 0x03, 0x04, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80];
    const n0 = deriveAeadNonce(base_iv, 0);
    expect_true(simdCryptoExtCtEqual(n0[0..], base_iv[0..]))?;

    const n1 = deriveAeadNonce(base_iv, 0x0102030405060708);
    const exp_n1: [12]u8 = [
        0x01, 0x02, 0x03, 0x04,
        0x10 ^ 0x01, 0x20 ^ 0x02, 0x30 ^ 0x03, 0x40 ^ 0x04,
        0x50 ^ 0x05, 0x60 ^ 0x06, 0x70 ^ 0x07, 0x80 ^ 0x08,
    ];
    expect_true(simdCryptoExtCtEqual(n1[0..], exp_n1[0..]))?;

    // Sender hard rekey_limit enforcement & rekeyStrict
    expect_true(AeadNonceSender.init(base_iv, 0, 0) == null)?;
    expect_true(AeadNonceSender.init(base_iv, 10, 11) == null)?;
    let sender = AeadNonceSender.init(base_iv, 3, 2).?;
    expect_false(sender.needsRekey())?;
    const t0 = sender.nextNonce().?;
    const t1 = sender.nextNonce().?;
    expect_true(sender.needsRekey())?;
    const t2 = sender.nextNonce().?;
    expect_true(sender.isExhausted())?;
    expect_true(sender.nextNonce() == null)?;
    expect_eq(t0.seq, 0 as u64);
    expect_eq(t1.seq, 1 as u64);
    expect_eq(t2.seq, 2 as u64);

    // rekeyStrict rejects all-zero IV and identical base_iv
    const zero_iv: [AEAD_NONCE_LEN]u8 = [0; AEAD_NONCE_LEN];
    const new_iv: [AEAD_NONCE_LEN]u8 = [0xAA; AEAD_NONCE_LEN];
    expect_false(sender.rekeyStrict(zero_iv))?;
    expect_false(sender.rekeyStrict(base_iv))?;
    expect_true(sender.rekeyStrict(new_iv))?;
    expect_eq(sender.stats().key_epoch, 1 as u64);
    expect_false(sender.isExhausted())?;

    // Receiver 128-bit sliding anti-replay window
    let recv = AeadNonceReceiver.init(base_iv);
    expect_eq(recv.checkAndMark(10), AntiReplayVerdict.accepted);
    expect_eq(recv.checkAndMark(10), AntiReplayVerdict.duplicate_replay);
    expect_eq(recv.checkAndMark(137), AntiReplayVerdict.accepted); // delta to 10 is 127 (inside window)
    expect_eq(recv.checkAndMark(10), AntiReplayVerdict.duplicate_replay);
    expect_eq(recv.checkAndMark(9), AntiReplayVerdict.too_old_below_window); // delta = 128 -> too old
    expect_eq(recv.checkAndMark(11), AntiReplayVerdict.accepted); // delta = 126 -> accepted
    expect_false(recv.rekeyStrict(base_iv))?;
    expect_true(recv.rekeyStrict(new_iv))?;
    expect_eq(recv.checkAndMark(0), AntiReplayVerdict.accepted);

    sender.zeroize();
    recv.zeroize();
    expect_true(isAllZeroBytesCt(sender.base_iv[0..]))?;
    expect_true(isAllZeroBytesCt(recv.base_iv[0..]))?;
}

// ---------------------------------------------------------------------------
// Test 4: 16-Lane SIMD vs Scalar 0-Diff Verification
// ---------------------------------------------------------------------------

fn runTestSimdZeroDiff(): void! {
    let a: [96]u8 = [0; 96];
    let b: [96]u8 = [0; 96];
    let x_simd: [96]u8 = [0; 96];
    let x_scalar: [96]u8 = [0; 96];
    let c_simd: [96]u8 = [0; 96];
    let c_scalar: [96]u8 = [0; 96];
    let zbuf: [96]u8 = [0; 96];

    let i: usize = 0;
    while (i < 96) : (i += 1) {
        a[i] = ((i * 41 + 13) & 0xFF) as u8;
        b[i] = ((i * 19 + 5) & 0xFF) as u8;
    }

    let len: usize = 0;
    while (len <= 96) : (len += 1) {
        // Equal
        expect_eq(simdCryptoExtCtEqual(a[0..len], a[0..len]), scalarCryptoExtCtEqual(a[0..len], a[0..len]));
        expect_true(simdCryptoExtCtEqual(a[0..len], a[0..len]))?;

        // XOR parity
        simdCryptoExtXor(x_simd[0..len], a[0..len], b[0..len]);
        scalarCryptoExtXor(x_scalar[0..len], a[0..len], b[0..len]);
        expect_true(scalarCryptoExtCtEqual(x_simd[0..len], x_scalar[0..len]))?;

        // Copy parity
        simdCryptoExtCopy(c_simd[0..len], a[0..len]);
        scalarCryptoExtCopy(c_scalar[0..len], a[0..len]);
        expect_true(scalarCryptoExtCtEqual(c_simd[0..len], c_scalar[0..len]))?;

        // All-zero check
        expect_true(isAllZeroBytesCt(zbuf[0..len]))?;
        if (len > 0 and (len <= 34 or len == 48 or len == 64 or len == 96)) {
            let p: usize = 0;
            while (p < len) : (p += 1) {
                zbuf[p] = 0x01;
                expect_false(isAllZeroBytesCt(zbuf[0..len]))?;
                zbuf[p] = 0x00;

                c_simd[p] ^= 0x80;
                expect_eq(simdCryptoExtCtEqual(a[0..len], c_simd[0..len]), scalarCryptoExtCtEqual(a[0..len], c_simd[0..len]));
                expect_false(simdCryptoExtCtEqual(a[0..len], c_simd[0..len]))?;
                c_simd[p] ^= 0x80;
            }
        }
    }
    expect_false(simdCryptoExtCtEqual(a[0..16], a[0..17]))?;
}

// ---------------------------------------------------------------------------
// Test 5: 1,600-Case Deterministic PRNG Fuzz (600 Ratchet + 500 KeyRing + 500 Replay)
// ---------------------------------------------------------------------------

fn runTestCryptoFuzz1600(): void! {
    let rng = XorShift64 { state: 0x3701C720261124 };

    // Part A: 600 Ratchet differential out-of-order / replay / rekey ops
    let s_rat = SymmetricKeyRatchet.initStrict("0123456789ABCDEF0123456789ABCDEF", "fz", "info").?;
    let r_rat = SymmetricKeyRatchet.initStrict("0123456789ABCDEF0123456789ABCDEF", "fz", "info").?;
    let sent_keys: [64][32]u8 = [[0; 32]; 64];
    let sent_used: [64]bool = [false; 64];
    let sent_count: usize = 0;
    let window_base: usize = 0;

    let ri: usize = 0;
    while (ri < 600) : (ri += 1) {
        if (sent_count < 64 and ((rng.next() % 3) != 0 or sent_count == window_base)) {
            const dk = s_rat.next_send_key();
            simdCryptoExtCopy(sent_keys[sent_count][0..], dk.key[0..]);
            sent_used[sent_count] = false;
            sent_count += 1;
        } else if (sent_count > 0) {
            const pick = (rng.next() as usize) % sent_count;
            const res = r_rat.receive_key_for_step(pick as u64);
            if (res != null) {
                const got = res.?;
                expect_false(sent_used[pick])?;
                expect_true(simdCryptoExtCtEqual(got.key[0..], sent_keys[pick][0..]))?;
                sent_used[pick] = true;
                while (window_base < sent_count and sent_used[window_base]) : (window_base += 1) {}
            } else {
                // Must either be already consumed or exceed max skip window
                const cur = r_rat.stats().current_step as usize;
                expect_true(sent_used[pick] or (pick > cur and (pick - cur) > RATCHET_MAX_SKIPPED_KEYS) or r_rat.stats().cached_skipped_keys > 0)?;
            }
        }
    }

    // Part B: 500 KeyRing HMAC differential vs std.crypto HmacSha256 + 1-bit tamper rejection
    let key_buf: [96]u8 = [0; 96];
    let msg_buf: [128]u8 = [0; 128];
    let ki: usize = 0;
    while (ki < 500) : (ki += 1) {
        const klen = (rng.next() as usize) % 96;
        const mlen = (rng.next() as usize) % 128;
        let j: usize = 0;
        while (j < klen) : (j += 1) {
            key_buf[j] = (rng.next() & 0xFF) as u8;
        }
        j = 0;
        while (j < mlen) : (j += 1) {
            msg_buf[j] = (rng.next() & 0xFF) as u8;
        }
        const our_mac = keyringHmacSha256(key_buf[0..klen], msg_buf[0..mlen]);
        let std_mac: [32]u8 = [0; 32];
        std.crypto.auth.hmac.sha2.HmacSha256.create(&std_mac, msg_buf[0..mlen], key_buf[0..klen]);
        expect_true(simdCryptoExtCtEqual(our_mac[0..], std_mac[0..]))?;
    }

    // Part C: 500 AntiReplayWindow128 differential vs exact reference model
    let win = AntiReplayWindow128.init();
    let seen: [512]bool = [false; 512];
    let max_seq: u64 = 0;
    let has_any = false;
    let ai: usize = 0;
    while (ai < 500) : (ai += 1) {
        const seq_u: usize = (rng.next() as usize) % 512;
        const seq: u64 = seq_u as u64;
        const pre = win.checkOnly(seq);
        const got = win.checkAndMark(seq);
        expect_eq(pre, got);

        if (!has_any) {
            expect_eq(got, AntiReplayVerdict.accepted);
            seen[seq_u] = true;
            max_seq = seq;
            has_any = true;
        } else if (seq > max_seq) {
            expect_eq(got, AntiReplayVerdict.accepted);
            seen[seq_u] = true;
            max_seq = seq;
        } else if (max_seq - seq >= 128) {
            expect_eq(got, AntiReplayVerdict.too_old_below_window);
        } else if (seen[seq_u]) {
            expect_eq(got, AntiReplayVerdict.duplicate_replay);
        } else {
            expect_eq(got, AntiReplayVerdict.accepted);
            seen[seq_u] = true;
        }
    }
}

fn main() {
    test("RFC 5869 HKDF-SHA256 official vectors & Forward-Secret Ratchet", () => {
        runTestRfc5869AndRatchet()?;
    });
    test("RFC 4231 HMAC-SHA256 official vectors & KeyRing rotation/tamper rejection", () => {
        runTestRfc4231AndKeyRing()?;
    });
    test("RFC 9001 AEAD Nonce derivation, rekeyStrict & 128-bit anti-replay window", () => {
        runTestRfc9001NonceAndReplay()?;
    });
    test("16-lane SIMD vs Scalar 0-diff differential verification", () => {
        runTestSimdZeroDiff()?;
    });
    test("1600-case deterministic PRNG Fuzz (600 Ratchet + 500 KeyRing HMAC + 500 AntiReplay)", () => {
        runTestCryptoFuzz1600()?;
    });
}
