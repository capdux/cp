// Origin: Pure-CP Stdlib Test Suite (lib/crypto/key_ratchet.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Forward-Secret Symmetric Key Ratchet:
//   1. TX/RX sequential derivation 100% byte-for-byte parity & per-step key uniqueness
//   2. Domain separation across different context_info & salt values
//   3. Out-of-order delivery (step 3 -> 1 -> 0 -> 2) exact match & strict one-time replay rejection
//   4. Excessive forward-skip (>32) atomic rejection preserving chain state & exact 32-skip boundary
//   5. Post-Compromise Security via rekey() entropy mixing & peer resynchronization
//   6. zeroize() full chain_key & skipped-key cache scrubbing verification
//   7. Empty root_secret initialization guard & RFC 5869 empty salt/info support

import { test, expect_eq } from "cap:test";
import {
    RATCHET_KEY_LEN,
    RATCHET_MAX_SKIPPED_KEYS,
    RatchetDerivedKey,
    RatchetStats,
    SymmetricKeyRatchet,
} from "cap:crypto";

fn eqBytes32(a: [32]u8, b: [32]u8): bool {
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn eqBytes12(a: [12]u8, b: [12]u8): bool {
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn eqDerived(a: RatchetDerivedKey, b: RatchetDerivedKey): bool {
    return a.step_index == b.step_index &&
        eqBytes32(a.key, b.key) &&
        eqBytes12(a.nonce_prefix, b.nonce_prefix);
}

fn isAllZero32(a: [32]u8): bool {
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        if (a[i] != 0) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Sequential TX/RX Byte Parity & Per-Step Uniqueness
// ============================================================================
test("key_ratchet: TX/RX sequential derivation parity & key uniqueness", () => {
    expect_eq(RATCHET_KEY_LEN, 32);
    expect_eq(RATCHET_MAX_SKIPPED_KEYS, 32);

    let tx = SymmetricKeyRatchet.init("master-root-secret-2026", "salt-alpha", "cp-agent-chan-1") ?? return;
    let rx = SymmetricKeyRatchet.init("master-root-secret-2026", "salt-alpha", "cp-agent-chan-1") ?? return;

    let prev_key: [32]u8 = [0; 32];
    let step: u64 = 0;
    while (step < 5) : (step += 1) {
        const sk = tx.next_send_key();
        const rk = rx.receive_key_for_step(step) ?? return;

        expect_eq(sk.step_index, step);
        expect_eq(eqDerived(sk, rk), true);
        expect_eq(isAllZero32(sk.key), false);
        if (step > 0) {
            expect_eq(eqBytes32(sk.key, prev_key), false);
        }
        prev_key = sk.key;
    }

    const st: RatchetStats = rx.stats();
    expect_eq(st.current_step, 5);
    expect_eq(st.total_derived, 5);
    expect_eq(st.cached_skipped_keys, 0);
});

// ============================================================================
// Test 2: Domain Separation Across Different context_info & salt
// ============================================================================
test("key_ratchet: different context_info or salt produces domain-separated keys", () => {
    let r_base = SymmetricKeyRatchet.init("shared-secret", "salt-1", "channel-A") ?? return;
    let r_info = SymmetricKeyRatchet.init("shared-secret", "salt-1", "channel-B") ?? return;
    let r_salt = SymmetricKeyRatchet.init("shared-secret", "salt-2", "channel-A") ?? return;

    const k_base = r_base.next_send_key();
    const k_info = r_info.next_send_key();
    const k_salt = r_salt.next_send_key();

    expect_eq(eqBytes32(k_base.key, k_info.key), false);
    expect_eq(eqBytes32(k_base.key, k_salt.key), false);
    expect_eq(eqBytes32(k_info.key, k_salt.key), false);
    expect_eq(eqBytes12(k_base.nonce_prefix, k_info.nonce_prefix), false);
});

// ============================================================================
// Test 3: Out-of-Order Arrival (3 -> 1 -> 0 -> 2) & Replay Rejection
// ============================================================================
test("key_ratchet: out-of-order delivery exact match & one-time replay rejection", () => {
    let tx = SymmetricKeyRatchet.init("ooo-secret", "ooo-salt", "ooo-info") ?? return;
    let rx = SymmetricKeyRatchet.init("ooo-secret", "ooo-salt", "ooo-info") ?? return;

    const s0 = tx.next_send_key();
    const s1 = tx.next_send_key();
    const s2 = tx.next_send_key();
    const s3 = tx.next_send_key();

    // Receive step 3 first -> skips & caches steps 0, 1, 2
    const r3 = rx.receive_key_for_step(3) ?? return;
    expect_eq(eqDerived(r3, s3), true);
    expect_eq(rx.stats().current_step, 4);
    expect_eq(rx.stats().cached_skipped_keys, 3);

    // Replay of step 3 MUST be rejected immediately
    expect_eq(rx.receive_key_for_step(3) == null, true);

    // Receive skipped steps out of order: 1 -> 0 -> 2
    const r1 = rx.receive_key_for_step(1) ?? return;
    expect_eq(eqDerived(r1, s1), true);
    expect_eq(rx.receive_key_for_step(1) == null, true); // replay rejected!

    const r0 = rx.receive_key_for_step(0) ?? return;
    expect_eq(eqDerived(r0, s0), true);
    expect_eq(rx.receive_key_for_step(0) == null, true); // replay rejected!

    const r2 = rx.receive_key_for_step(2) ?? return;
    expect_eq(eqDerived(r2, s2), true);
    expect_eq(rx.receive_key_for_step(2) == null, true); // replay rejected!

    const st = rx.stats();
    expect_eq(st.cached_skipped_keys, 0);
    expect_eq(st.total_out_of_order_hits, 3);
    expect_eq(st.total_derived, 4);
});

// ============================================================================
// Test 4: Excessive Forward-Skip Guard (>32) & Exact Boundary (==32)
// ============================================================================
test("key_ratchet: excessive forward skip rejected atomically & exact 32-skip succeeds", () => {
    let rx = SymmetricKeyRatchet.init("skip-secret", "skip-salt", "skip-info") ?? return;

    // Target step 33 from step 0 requires caching 33 skipped keys (> 32) -> rejected atomically!
    expect_eq(rx.receive_key_for_step(33) == null, true);
    expect_eq(rx.stats().current_step, 0);
    expect_eq(rx.stats().cached_skipped_keys, 0);

    // Exact max boundary: target step 32 caches 32 skipped keys (0..31) -> succeeds!
    const k32 = rx.receive_key_for_step(32) ?? return;
    expect_eq(k32.step_index, 32);
    expect_eq(rx.stats().current_step, 33);
    expect_eq(rx.stats().cached_skipped_keys, 32);

    // With cache full (32/32), skipping even 1 step (target 34) is rejected atomically
    expect_eq(rx.receive_key_for_step(34) == null, true);
    expect_eq(rx.stats().current_step, 33);
});

// ============================================================================
// Test 5: Post-Compromise Security via rekey() Entropy Mixing
// ============================================================================
test("key_ratchet: rekey mixes fresh entropy for post-compromise security", () => {
    let tx = SymmetricKeyRatchet.init("init-secret", "s", "i") ?? return;
    let rx = SymmetricKeyRatchet.init("init-secret", "s", "i") ?? return;
    let stale_clone = SymmetricKeyRatchet.init("init-secret", "s", "i") ?? return;

    // Step 0 identical across all three
    const k0_tx = tx.next_send_key();
    const k0_rx = rx.receive_key_for_step(0) ?? return;
    const k0_cl = stale_clone.next_send_key();
    expect_eq(eqDerived(k0_tx, k0_rx), true);
    expect_eq(eqDerived(k0_tx, k0_cl), true);

    // Empty entropy rejected
    expect_eq(tx.rekey("", "rekey-v1"), false);
    expect_eq(tx.stats().rekey_count, 0);

    // Both TX and RX mix fresh DH/CSPRNG entropy
    expect_eq(tx.rekey("fresh-ephemeral-dh-shared-secret", "epoch-2"), true);
    expect_eq(rx.rekey("fresh-ephemeral-dh-shared-secret", "epoch-2"), true);
    expect_eq(tx.stats().rekey_count, 1);

    // Step 1 matches between TX & RX, but completely diverges from un-rekeyed clone!
    const k1_tx = tx.next_send_key();
    const k1_rx = rx.receive_key_for_step(1) ?? return;
    const k1_cl = stale_clone.next_send_key();

    expect_eq(eqDerived(k1_tx, k1_rx), true);
    expect_eq(eqBytes32(k1_tx.key, k1_cl.key), false);
    expect_eq(eqBytes12(k1_tx.nonce_prefix, k1_cl.nonce_prefix), false);
});

// ============================================================================
// Test 6: zeroize() Scrubbing of Chain Key & Skipped Key Cache
// ============================================================================
test("key_ratchet: zeroize scrubs chain_key and all cached skipped keys", () => {
    let r = SymmetricKeyRatchet.init("wipe-secret", "salt", "info") ?? return;
    * = r.receive_key_for_step(3); // caches skipped keys 0, 1, 2
    expect_eq(r.stats().cached_skipped_keys, 3);
    expect_eq(isAllZero32(r.chain_key), false);

    r.zeroize();

    expect_eq(isAllZero32(r.chain_key), true);
    expect_eq(r.stats().cached_skipped_keys, 0);
    // Previously cached skipped keys are gone
    expect_eq(r.receive_key_for_step(0) == null, true);
    expect_eq(r.receive_key_for_step(1) == null, true);
    expect_eq(r.receive_key_for_step(2) == null, true);
});

// ============================================================================
// Test 7: Empty root_secret Guard & RFC 5869 Empty Salt/Info Support
// ============================================================================
test("key_ratchet: empty root_secret rejected & empty salt/info accepted", () => {
    expect_eq(SymmetricKeyRatchet.init("", "salt", "info") == null, true);

    // RFC 5869 allows empty salt and empty context info
    let r = SymmetricKeyRatchet.init("non-empty-secret", "", "") ?? return;
    const k0 = r.next_send_key();
    expect_eq(k0.step_index, 0);
    expect_eq(isAllZero32(k0.key), false);
});
