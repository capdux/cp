// Origin: Pure-CP Stdlib Test Suite (lib/crypto/aeadNonce.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc RFC 9001 AEAD Nonce Deriver & 128-Bit Anti-Replay Window:
//   1. RFC 9001 §5.3 / TLS 1.3 deterministic 96-bit XOR nonce derivation exact vectors
//   2. `AeadNonceSender` hard `rekey_limit` reuse prevention, `needsRekey()` warning & invalid-arg guards
//   3. Epoch-tracked `rekey(*_, new_base_iv)` on sender & receiver resetting seq & anti-replay window
//   4. 128-bit anti-replay exact boundary tests (delta = 0, 63, 64, 127, 128) & read-only `checkOnly`
//   5. Cross-64-bit bitmap carry shifting (`shift < 64`, `64..127`) & large sequence jump (`>= 128`) window reset
//   6. End-to-end sender/receiver out-of-order delivery & telemetry stats verification
//   7. `cap.crypto` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    AEAD_NONCE_LEN,
    AEAD_ANTI_REPLAY_WINDOW_BITS,
    AntiReplayVerdict,
    AeadNonceTicket,
    deriveAeadNonce,
    AeadNonceSender,
    AntiReplayWindow128,
    AeadNonceReceiver,
} from "cap:crypto";

fn nonceEq(a: [AEAD_NONCE_LEN]u8, b: [AEAD_NONCE_LEN]u8): bool {
    let i: usize = 0;
    while (i < AEAD_NONCE_LEN) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("AeadNonce: RFC 9001 / TLS 1.3 deterministic 96-bit XOR derivation exact vectors") {
    const base_iv: [12]u8 = [
        0x11, 0x22, 0x33, 0x44,
        0xFF, 0xEE, 0xDD, 0xCC,
        0xBB, 0xAA, 0x99, 0x88,
    ];

    // seq == 0 yields base_iv unchanged
    expect_true(nonceEq(deriveAeadNonce(base_iv, 0), base_iv));

    // seq == 0x0102_0304_0506_0708 XORs big-endian into base_iv[4..12]
    const expected: [12]u8 = [
        0x11, 0x22, 0x33, 0x44,
        0xFF ^ 0x01, 0xEE ^ 0x02, 0xDD ^ 0x03, 0xCC ^ 0x04,
        0xBB ^ 0x05, 0xAA ^ 0x06, 0x99 ^ 0x07, 0x88 ^ 0x08,
    ];
    const derived = deriveAeadNonce(base_iv, 0x0102_0304_0506_0708);
    expect_true(nonceEq(derived, expected));
}

test("AeadNonce: sender hard rekey_limit reuse guard, needsRekey watermark & invalid init") {
    const iv: [12]u8 = [0xAA; 12];
    expect_true(AeadNonceSender.init(iv, 0, 0) == null);
    expect_true(AeadNonceSender.init(iv, 10, 11) == null);

    let tx = AeadNonceSender.init(iv, 5, 4) ?? return;
    expect_eq(tx.remaining(), 5 as u64);
    expect_true(!tx.needsRekey());
    expect_true(!tx.isExhausted());

    // Issue packets 0, 1, 2 -> below warn_threshold (4)
    let i: u64 = 0;
    while (i < 3) : (i += 1) {
        const t = tx.nextNonce() ?? return;
        expect_eq(t.seq, i);
        expect_eq(t.key_epoch, 0 as u64);
        expect_true(nonceEq(t.nonce, deriveAeadNonce(iv, i)));
    }
    expect_true(!tx.needsRekey());

    // Issue packet 3 -> next_seq becomes 4 -> needsRekey() triggers!
    * = tx.nextNonce();
    expect_true(tx.needsRekey());
    expect_true(!tx.isExhausted());

    // Issue packet 4 -> next_seq becomes 5 -> exhausted!
    * = tx.nextNonce();
    expect_true(tx.isExhausted());
    expect_eq(tx.remaining(), 0 as u64);

    // 6th call is strictly blocked to prevent catastrophic nonce reuse
    expect_true(tx.nextNonce() == null);
    expect_eq(tx.stats().total_rejected_exhausted, 1 as u64);
    expect_eq(tx.stats().total_derived, 5 as u64);
}

test("AeadNonce: key rotation rekey() increments key_epoch & resets seq and replay window") {
    const iv1: [12]u8 = [0x10; 12];
    const iv2: [12]u8 = [0x20; 12];

    let tx = AeadNonceSender.init(iv1, 2, 1) ?? return;
    let rx = AeadNonceReceiver.init(iv1);

    const t0 = tx.nextNonce() ?? return;
    const t1 = tx.nextNonce() ?? return;
    expect_true(tx.nextNonce() == null);

    expect_eq(rx.checkAndMark(t0.seq), AntiReplayVerdict.accepted);
    expect_eq(rx.checkAndMark(t1.seq), AntiReplayVerdict.accepted);
    expect_eq(rx.checkAndMark(t0.seq), AntiReplayVerdict.duplicate_replay);

    // Rotate both sender and receiver to epoch 1 with fresh iv2
    tx.rekey(iv2);
    rx.rekey(iv2);

    const t_new = tx.nextNonce() ?? return;
    expect_eq(t_new.key_epoch, 1 as u64);
    expect_eq(t_new.seq, 0 as u64);
    expect_true(nonceEq(t_new.nonce, rx.deriveForSeq(0)));
    expect_true(!nonceEq(t_new.nonce, t0.nonce));

    // seq 0 is freshly accepted in epoch 1
    expect_eq(rx.checkAndMark(0), AntiReplayVerdict.accepted);
    expect_eq(rx.stats().key_epoch, 1 as u64);
    expect_eq(rx.stats().total_rekeys, 1 as u64);
    expect_eq(tx.stats().total_rekeys, 1 as u64);
}

test("AeadNonce: 128-bit AntiReplayWindow exact boundaries (delta 0, 63, 64, 127, 128) & checkOnly") {
    let win = AntiReplayWindow128.init();

    // Receive seq 200 first
    expect_eq(win.checkAndMark(200), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(200), AntiReplayVerdict.duplicate_replay);

    // delta == 128 (seq 72) is strictly below the 128-bit window
    expect_eq(win.checkOnly(72), AntiReplayVerdict.too_old_below_window);
    expect_eq(win.checkAndMark(72), AntiReplayVerdict.too_old_below_window);

    // delta == 127 (seq 73, highest bit of bitmap_hi) -> accepted once, then replay
    expect_eq(win.checkOnly(73), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(73), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(73), AntiReplayVerdict.duplicate_replay);

    // delta == 64 (seq 136, lowest bit of bitmap_hi)
    expect_eq(win.checkAndMark(136), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(136), AntiReplayVerdict.duplicate_replay);

    // delta == 63 (seq 137, highest bit of bitmap_lo)
    expect_eq(win.checkAndMark(137), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(137), AntiReplayVerdict.duplicate_replay);

    // checkOnly must never mutate window state
    expect_eq(win.checkOnly(150), AntiReplayVerdict.accepted);
    expect_eq(win.checkOnly(150), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(150), AntiReplayVerdict.accepted);
    expect_eq(win.checkOnly(150), AntiReplayVerdict.duplicate_replay);
}

test("AeadNonce: cross-64-bit bitmap carry shifting & large sequence jump reset") {
    let win = AntiReplayWindow128.init();

    // Mark 100 and 95
    expect_eq(win.checkAndMark(100), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(95), AntiReplayVerdict.accepted);

    // Advance by shift = 40 (< 64) to 140 -> 100 at delta 40, 95 at delta 45
    expect_eq(win.checkAndMark(140), AntiReplayVerdict.accepted);
    // Advance by shift = 50 (< 64) to 190 -> 95 carries from bitmap_lo into bitmap_hi at delta 95!
    expect_eq(win.checkAndMark(190), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(95), AntiReplayVerdict.duplicate_replay);
    expect_eq(win.checkAndMark(100), AntiReplayVerdict.duplicate_replay);
    expect_eq(win.checkAndMark(140), AntiReplayVerdict.duplicate_replay);
    // Unseen seq 96 (delta 94) is still accepted
    expect_eq(win.checkAndMark(96), AntiReplayVerdict.accepted);

    // Advance by shift = 70 (64 <= shift < 128) to 260 -> 190 moves into bitmap_hi at delta 70
    expect_eq(win.checkAndMark(260), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(190), AntiReplayVerdict.duplicate_replay);
    expect_eq(win.checkAndMark(140), AntiReplayVerdict.duplicate_replay); // delta 120 < 128!

    // Large jump by shift = 300 (>= 128) to 560 clears old bitmap cleanly
    expect_eq(win.checkAndMark(560), AntiReplayVerdict.accepted);
    expect_eq(win.checkAndMark(432), AntiReplayVerdict.too_old_below_window); // delta 128
    expect_eq(win.checkAndMark(433), AntiReplayVerdict.accepted);             // delta 127
}

test("AeadNonce: end-to-end out-of-order delivery & telemetry stats") {
    const iv: [12]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C];
    let tx = AeadNonceSender.initDefault(iv);
    let rx = AeadNonceReceiver.init(iv);

    let tickets: [6]AeadNonceTicket = [AeadNonceTicket{}; 6];
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        tickets[i] = tx.nextNonce() ?? return;
    }

    // Deliver out of order: 5, 2, 0, 4, 1, 3
    const order: [6]usize = [5, 2, 0, 4, 1, 3];
    let k: usize = 0;
    while (k < 6) : (k += 1) {
        const t = tickets[order[k]];
        expect_true(nonceEq(t.nonce, rx.deriveForSeq(t.seq)));
        expect_eq(rx.checkAndMark(t.seq), AntiReplayVerdict.accepted);
    }

    // Replaying any of 0..5 is blocked
    expect_eq(rx.checkAndMark(3), AntiReplayVerdict.duplicate_replay);
    const st = rx.stats();
    expect_eq(st.highest_seq, 5 as u64);
    expect_eq(st.window_size, AEAD_ANTI_REPLAY_WINDOW_BITS);
    expect_eq(st.total_accepted, 6 as u64);
    expect_eq(st.total_replays, 1 as u64);
}

test("AeadNonce: cap.crypto namespace facade parity") {
    const iv: [12]u8 = [0x5A; 12];
    let tx = cap.crypto.AeadNonceSender.init(iv, 100, 80) ?? return;
    let rx = cap.crypto.AeadNonceReceiver.init(iv);

    const t = tx.nextNonce() ?? return;
    expect_true(nonceEq(t.nonce, cap.crypto.deriveAeadNonce(iv, 0)));
    expect_eq(rx.checkAndMark(t.seq), cap.crypto.AntiReplayVerdict.accepted);
    expect_eq(rx.checkAndMark(t.seq), cap.crypto.AntiReplayVerdict.duplicate_replay);
}
