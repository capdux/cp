// Origin: Pure-CP Stdlib Test Suite (lib/crypto/keyring.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Multi-Version KeyRing Rotation:
//   1. Graceful Active -> VerifyOnly rotation & dual-version verification
//   2. Revocation enforcement & in-memory secret zeroization
//   3. Expiration enforcement (refuses signing & verification) + stats(now_ms)
//   4. Single-bit payload/tag tamper, cross-kid confusion & short-tag rejection
//   5. Duplicate kid, invalid expiry, full-slot & no-active-key boundary guards
//   6. Undersized out_tag_buf, empty payload & clear() zeroize/reuse

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KEYRING_TAG_LEN,
    KEYRING_SECRET_LEN,
    KeyVersionState,
    KeyRingEntry,
    KeyRingStats,
    KeyRing,
} from "cap:crypto";

// ============================================================================
// Test 1: Graceful Rotation (Active -> VerifyOnly) & Dual-Version Verification
// ============================================================================

fn testGracefulRotationAndDualVerify()! {
    expect_eq(KEYRING_TAG_LEN, 32);
    expect_eq(KEYRING_SECRET_LEN, 32);

    let slots: [4]KeyRingEntry = [KeyRingEntry{}; 4];
    let ring = cap.crypto.KeyRing.init(slots[0..]);

    const secret_v1: [32]u8 = [0x11; 32];
    const secret_v2: [32]u8 = [0x22; 32];

    expect_eq(ring.addKey(101, secret_v1, true, 1000, 10000), true);
    let tag1: [32]u8 = [0; 32];
    const kid1 = ring.signEnvelope("session:alice:role=admin", 2000, tag1[0..]) ?? return error.SignFailed;
    expect_eq(kid1, 101);
    expect_eq(ring.verifyEnvelope(kid1, "session:alice:role=admin", tag1[0..], 2500), true);

    // Rotate to v2: v1 must automatically demote to VerifyOnly
    expect_eq(ring.addKey(102, secret_v2, true, 3000, 20000), true);
    const act = ring.activeKey(3500) ?? return error.NoActiveKey;
    expect_eq(act.kid, 102);

    const old_k = ring.findVerifiableKey(101, 3500) ?? return error.OldKeyMissing;
    expect_eq(old_k.state == KeyVersionState.VerifyOnly, true);

    // New envelope signed with v2; BOTH v1 and v2 verify during grace window
    let tag2: [32]u8 = [0; 32];
    const kid2 = ring.signEnvelope("session:bob:role=agent", 3500, tag2[0..]) ?? return error.SignFailed;
    expect_eq(kid2, 102);
    expect_eq(ring.verifyEnvelope(kid1, "session:alice:role=admin", tag1[0..], 3500), true);
    expect_eq(ring.verifyEnvelope(kid2, "session:bob:role=agent", tag2[0..], 3500), true);
}

// ============================================================================
// Test 2: Revocation Enforcement & In-Memory Secret Zeroization
// ============================================================================

fn testRevocationAndZeroization()! {
    let slots: [2]KeyRingEntry = [KeyRingEntry{}; 2];
    let ring = KeyRing.init(slots[0..]);

    const s1: [32]u8 = [0xAB; 32];
    * = ring.addKey(1, s1, true, 100, 0);

    let tag: [32]u8 = [0; 32];
    const kid = ring.signEnvelope("payload", 200, tag[0..]) ?? return error.SignFailed;
    expect_eq(ring.verifyEnvelope(kid, "payload", tag[0..], 200), true);

    // Revoke key 1 -> immediately rejected for both verify and sign, and secret bytes zeroed
    expect_eq(ring.revokeKey(1), true);
    expect_eq(ring.revokeKey(999), false);
    expect_eq(ring.verifyEnvelope(kid, "payload", tag[0..], 200), false);
    expect_eq(ring.findVerifiableKey(1, 200) == null, true);
    expect_eq(ring.activeKey(200) == null, true);
    expect_eq(ring.signEnvelope("payload2", 200, tag[0..]) == null, true);

    // Verify in-memory secret zeroization
    let all_zero = true;
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        if (slots[0].secret[i] != 0) all_zero = false;
    }
    expect_eq(all_zero, true);
}

// ============================================================================
// Test 3: Expiration Enforcement & Stats Snapshot
// ============================================================================

fn testExpirationAndStats()! {
    let slots: [4]KeyRingEntry = [KeyRingEntry{}; 4];
    let ring = KeyRing.init(slots[0..]);

    const s1: [32]u8 = [0x31; 32];
    const s2: [32]u8 = [0x32; 32];
    const s3: [32]u8 = [0x33; 32];
    * = ring.addKey(1, s1, true, 1000, 5000);  // expires at 5000
    * = ring.addKey(2, s2, true, 2000, 8000);  // active, expires at 8000
    * = ring.addKey(3, s3, false, 2000, 0);    // verify-only, never expires
    * = ring.revokeKey(3);

    let tag1: [32]u8 = [0; 32];
    let tag2: [32]u8 = [0; 32];
    // Sign with key 2 at t=3000
    const kid2 = ring.signEnvelope("msg", 3000, tag2[0..]) ?? return error.SignFailed;
    expect_eq(kid2, 2);

    // At t=6000: key 1 is expired, key 2 is still active & valid
    expect_eq(ring.findVerifiableKey(1, 6000) == null, true);
    expect_eq(ring.verifyEnvelope(2, "msg", tag2[0..], 6000), true);
    const st6000: KeyRingStats = ring.stats(6000);
    expect_eq(st6000.total_keys, 3);
    expect_eq(st6000.active_count, 1);
    expect_eq(st6000.verify_only_count, 1);
    expect_eq(st6000.revoked_count, 1);
    expect_eq(st6000.expired_count, 1);

    // At t=8000: key 2 also expires -> neither signing nor verification allowed
    expect_eq(ring.activeKey(8000) == null, true);
    expect_eq(ring.signEnvelope("msg", 8000, tag1[0..]) == null, true);
    expect_eq(ring.verifyEnvelope(2, "msg", tag2[0..], 8000), false);
    expect_eq(ring.stats(8000).expired_count, 2);
}

// ============================================================================
// Test 4: Tamper, Cross-Kid Confusion & Short-Tag Rejection
// ============================================================================

fn testTamperAndCrossKidRejection()! {
    let slots: [2]KeyRingEntry = [KeyRingEntry{}; 2];
    let ring = KeyRing.init(slots[0..]);

    // Even if two kids accidentally share identical secret bytes, kid domain binding rejects swap
    const same_secret: [32]u8 = [0x77; 32];
    * = ring.addKey(10, same_secret, true, 100, 0);
    * = ring.addKey(20, same_secret, false, 100, 0);

    let tag: [32]u8 = [0; 32];
    const kid = ring.signEnvelope("transfer:100", 200, tag[0..]) ?? return error.SignFailed;
    expect_eq(kid, 10);

    // 4a. Cross-kid verification MUST fail despite identical secret
    expect_eq(ring.verifyEnvelope(20, "transfer:100", tag[0..], 200), false);

    // 4b. Single-byte payload modification fails
    expect_eq(ring.verifyEnvelope(10, "transfer:101", tag[0..], 200), false);

    // 4c. Single-bit tag flip fails
    tag[31] = tag[31] ^ 0x01;
    expect_eq(ring.verifyEnvelope(10, "transfer:100", tag[0..], 200), false);
    tag[31] = tag[31] ^ 0x01;
    expect_eq(ring.verifyEnvelope(10, "transfer:100", tag[0..], 200), true);

    // 4d. Short tag (< 32 bytes) rejected
    expect_eq(ring.verifyEnvelope(10, "transfer:100", tag[0..31], 200), false);
}

// ============================================================================
// Test 5: Duplicate kid, Invalid Expiry, Full Capacity & No-Active-Key Guards
// ============================================================================

fn testBoundaryRegistrationGuards()! {
    let slots: [2]KeyRingEntry = [KeyRingEntry{}; 2];
    let ring = KeyRing.init(slots[0..]);
    const s: [32]u8 = [0x55; 32];

    // Invalid expiry (expires_ms <= created_ms)
    expect_eq(ring.addKey(1, s, true, 1000, 1000), false);
    expect_eq(ring.addKey(1, s, true, 1000, 500), false);

    // Add VerifyOnly key -> no active key for signing
    expect_eq(ring.addKey(1, s, false, 1000, 0), true);
    let tag: [32]u8 = [0; 32];
    expect_eq(ring.signEnvelope("data", 1500, tag[0..]) == null, true);

    // Duplicate kid rejected
    expect_eq(ring.addKey(1, s, true, 1000, 0), false);

    // Fill 2nd slot, 3rd rejected
    expect_eq(ring.addKey(2, s, true, 1000, 0), true);
    expect_eq(ring.addKey(3, s, true, 1000, 0), false);
    expect_eq(ring.count(), 2);
    expect_eq(ring.capacity(), 2);
}

// ============================================================================
// Test 6: Undersized Tag Buffer, Empty Payload & Clear Reuse
// ============================================================================

fn testUndersizedTagEmptyPayloadAndClear()! {
    let slots: [2]KeyRingEntry = [KeyRingEntry{}; 2];
    let ring = KeyRing.init(slots[0..]);
    const s: [32]u8 = [0x99; 32];
    * = ring.addKey(7, s, true, 100, 0);

    // Undersized output tag buffer -> null
    let small_tag: [16]u8 = [0; 16];
    expect_eq(ring.signEnvelope("abc", 200, small_tag[0..]) == null, true);

    // Empty payload signs & verifies deterministically
    let tag: [32]u8 = [0; 32];
    const kid = ring.signEnvelope("", 200, tag[0..]) ?? return error.SignFailed;
    expect_eq(kid, 7);
    expect_eq(ring.verifyEnvelope(7, "", tag[0..], 200), true);

    // Clear zeroes secrets & allows reuse
    ring.clear();
    expect_eq(ring.count(), 0);
    expect_eq(slots[0].secret[0], 0);
    expect_eq(ring.addKey(7, s, true, 300, 0), true);
    expect_eq(ring.count(), 1);
}

test("crypto.keyring: graceful Active -> VerifyOnly rotation & dual-version verify", testGracefulRotationAndDualVerify);
test("crypto.keyring: revocation enforcement & in-memory secret zeroization", testRevocationAndZeroization);
test("crypto.keyring: expiration enforcement & stats snapshot", testExpirationAndStats);
test("crypto.keyring: tamper, cross-kid confusion & short-tag rejection", testTamperAndCrossKidRejection);
test("crypto.keyring: duplicate kid, invalid expiry, full capacity & no-active guards", testBoundaryRegistrationGuards);
test("crypto.keyring: undersized tag buffer, empty payload & clear reuse", testUndersizedTagEmptyPayloadAndClear);
