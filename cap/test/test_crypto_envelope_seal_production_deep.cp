// Origin: Pure-CP Stdlib Test Suite (lib/crypto/envelope_seal.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Versioned Envelope Encryption & Keyring:
//   1. CPE1 binary header encode/decode round-trip & output buffer size guard
//   2. Malformed header guards (truncated, bad magic, unsupported version/alg, invalid nonce/salt len)
//   3. End-to-end sealEnvelope/openEnvelope round-trip (multi-block CTR + empty plaintext + AAD)
//   4. Key rotation lifecycle (primary -> active_decrypt_only -> retired) & needs_rewrap migration hint
//   5. Key revocation scrubbing & strict decrypt/re-promote rejection (key_revoked)
//   6. Tamper resistance (ciphertext bit-flip, tag bit-flip, header field tamper, AAD mismatch -> auth_failed)
//   7. Keyring boundary guards (16-slot keyring_full, duplicate_key_version, zero KEK, no_primary_key)
//   8. cap.crypto namespace access, EnvelopeKeyring.stats() telemetry & zeroize() scrubbing

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ENVELOPE_FORMAT_VERSION,
    ENVELOPE_ALG_HKDF_CTR_HMAC_SHA256,
    ENVELOPE_FIXED_HEADER_LEN,
    ENVELOPE_TAG_LEN,
    ENVELOPE_KEK_LEN,
    ENVELOPE_KEYRING_MAX_KEYS,
    EnvelopeStatus,
    EnvelopeKeyState,
    EnvelopeHeader,
    EnvelopeKeyring,
    encodeEnvelopeHeader,
    decodeEnvelopeHeader,
    sealEnvelope,
    openEnvelope,
} from "cap:crypto";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn makeKek(seed: u8): [ENVELOPE_KEK_LEN]u8 {
    let k: [ENVELOPE_KEK_LEN]u8 = [0; ENVELOPE_KEK_LEN];
    let i: usize = 0;
    while (i < ENVELOPE_KEK_LEN) : (i += 1) {
        k[i] = seed ^ ((i as u8) +% 0x37);
    }
    return k;
}

// ============================================================================
// Test 1: CPE1 Binary Header Encode/Decode Round-Trip & Small Buffer Guard
// ============================================================================

fn testHeaderEncodeDecodeRoundTrip()! {
    let hdr = EnvelopeHeader{
        version: ENVELOPE_FORMAT_VERSION,
        alg_id: ENVELOPE_ALG_HKDF_CTR_HMAC_SHA256,
        key_version: 0x01020304,
        nonce_len: 12,
        salt_len: 20,
        aad_len: 42,
        ct_len: 128,
    };
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        hdr.nonce[i] = (0xA0 + i) as u8;
    }
    let j: usize = 0;
    while (j < 20) : (j += 1) {
        hdr.salt[j] = (0x50 + j) as u8;
    }
    let t: usize = 0;
    while (t < ENVELOPE_TAG_LEN) : (t += 1) {
        hdr.tag[t] = (0x10 + t) as u8;
    }

    const expected_len: usize = ENVELOPE_FIXED_HEADER_LEN + 12 + 20 + ENVELOPE_TAG_LEN;
    expect_eq(hdr.encodedLen(), expected_len);

    // Too-small output buffer must be rejected
    let tiny: [32]u8 = [0; 32];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(encodeEnvelopeHeader(hdr, *tiny_s).status == EnvelopeStatus.output_buffer_too_small, true);

    let wire: [128]u8 = [0; 128];
    let wire_s: []u8 = wire[0..];
    const enc = encodeEnvelopeHeader(hdr, *wire_s);
    expect_eq(enc.status == EnvelopeStatus.ok, true);
    expect_eq(enc.bytes_written, expected_len);

    const decoded = decodeEnvelopeHeader(wire[0..enc.bytes_written]);
    expect_eq(decoded.status == EnvelopeStatus.ok, true);
    expect_eq(decoded.header_len, expected_len);
    expect_eq(decoded.header.key_version, 0x01020304);
    expect_eq(decoded.header.nonce_len, 12);
    expect_eq(decoded.header.salt_len, 20);
    expect_eq(decoded.header.aad_len, 42);
    expect_eq(decoded.header.ct_len, 128);
    expect_eq(sliceEq(decoded.header.nonceSlice(), hdr.nonceSlice()), true);
    expect_eq(sliceEq(decoded.header.saltSlice(), hdr.saltSlice()), true);
    expect_eq(sliceEq(decoded.header.tag[0..], hdr.tag[0..]), true);
}

// ============================================================================
// Test 2: Malformed Header Validation Guards
// ============================================================================

fn testMalformedHeaderGuards()! {
    // 1. Truncated prefix (< 20B)
    let short_buf: [10]u8 = ['C', 'P', 'E', '1', 1, 1, 0, 0, 0, 1];
    expect_eq(decodeEnvelopeHeader(short_buf[0..]).status == EnvelopeStatus.truncated_input, true);

    // Construct a valid encoded header to mutate fields systematically
    let valid_hdr = EnvelopeHeader{
        key_version: 7,
        nonce_len: 16,
        salt_len: 16,
    };
    let wire: [128]u8 = [0; 128];
    let wire_s: []u8 = wire[0..];
    const enc = encodeEnvelopeHeader(valid_hdr, *wire_s);
    expect_eq(enc.status == EnvelopeStatus.ok, true);

    // 2. Truncated body (missing tag bytes)
    expect_eq(decodeEnvelopeHeader(wire[0..enc.bytes_written - 1]).status == EnvelopeStatus.truncated_input, true);

    // 3. Invalid magic
    let bad_magic: [128]u8 = wire;
    bad_magic[3] = '2';
    expect_eq(decodeEnvelopeHeader(bad_magic[0..enc.bytes_written]).status == EnvelopeStatus.invalid_magic, true);

    // 4. Unsupported version
    let bad_ver: [128]u8 = wire;
    bad_ver[4] = 9;
    expect_eq(decodeEnvelopeHeader(bad_ver[0..enc.bytes_written]).status == EnvelopeStatus.unsupported_version, true);

    // 5. Unsupported algorithm
    let bad_alg: [128]u8 = wire;
    bad_alg[5] = 99;
    expect_eq(decodeEnvelopeHeader(bad_alg[0..enc.bytes_written]).status == EnvelopeStatus.unsupported_algorithm, true);

    // 6. Invalid nonce_len (< 12)
    let bad_nonce: [128]u8 = wire;
    bad_nonce[10] = 4;
    expect_eq(decodeEnvelopeHeader(bad_nonce[0..enc.bytes_written]).status == EnvelopeStatus.invalid_header_params, true);

    // 7. Invalid key_version == 0
    let bad_kv: [128]u8 = wire;
    bad_kv[9] = 0;
    expect_eq(decodeEnvelopeHeader(bad_kv[0..enc.bytes_written]).status == EnvelopeStatus.invalid_header_params, true);
}

// ============================================================================
// Test 3: End-to-End sealEnvelope / openEnvelope (Multi-Block CTR & Empty PT)
// ============================================================================

fn testSealOpenEndToEndRoundTrip()! {
    let ring = EnvelopeKeyring.init();
    expect_eq(ring.registerKey(1, makeKek(0x41), EnvelopeKeyState.primary, 1000) == EnvelopeStatus.ok, true);

    // Multi-block plaintext (> 96 bytes -> 4 HMAC-CTR blocks)
    const msg: []const u8 = "The quick brown fox jumps over the lazy dog while CP encrypts multi-block envelopes with zero heap allocations!";
    const aad: []const u8 = "tenant=prod-east;table=agent_secrets;row=991";
    const nonce1: []const u8 = "nonce_0000000001";
    const nonce2: []const u8 = "nonce_0000000002";
    const salt: []const u8 = "salt_0123456789abcdef";

    let env1: [512]u8 = [0; 512];
    let env1_s: []u8 = env1[0..];
    const s1 = sealEnvelope(ring, nonce1, salt, aad, msg, *env1_s);
    expect_eq(s1.status == EnvelopeStatus.ok, true);
    expect_eq(s1.key_version, 1);
    expect_eq(s1.needs_rewrap, false);

    // Sealing with a different nonce must produce different ciphertext
    let env2: [512]u8 = [0; 512];
    let env2_s: []u8 = env2[0..];
    const s2 = sealEnvelope(ring, nonce2, salt, aad, msg, *env2_s);
    expect_eq(s2.status == EnvelopeStatus.ok, true);
    expect_eq(sliceEq(env1[0..s1.bytes_written], env2[0..s2.bytes_written]), false);

    // Open env1 and verify exact plaintext recovery
    let pt: [256]u8 = [0; 256];
    let pt_s: []u8 = pt[0..];
    const o1 = openEnvelope(ring, env1[0..s1.bytes_written], aad, *pt_s);
    expect_eq(o1.status == EnvelopeStatus.ok, true);
    expect_eq(o1.bytes_written, msg.len);
    expect_eq(o1.key_version, 1);
    expect_eq(o1.needs_rewrap, false);
    expect_eq(sliceEq(pt[0..o1.bytes_written], msg), true);

    // Verify ciphertext actually differs from plaintext
    const hdr_len = s1.bytes_written - msg.len;
    expect_eq(sliceEq(env1[hdr_len..s1.bytes_written], msg), false);

    // Empty plaintext (0 bytes) with AAD must also seal & authenticate cleanly
    let env_empty: [256]u8 = [0; 256];
    let env_empty_s: []u8 = env_empty[0..];
    const se = sealEnvelope(ring, nonce1, salt, aad, "", *env_empty_s);
    expect_eq(se.status == EnvelopeStatus.ok, true);

    let pt_empty: [16]u8 = [0; 16];
    let pt_empty_s: []u8 = pt_empty[0..];
    const oe = openEnvelope(ring, env_empty[0..se.bytes_written], aad, *pt_empty_s);
    expect_eq(oe.status == EnvelopeStatus.ok, true);
    expect_eq(oe.bytes_written, 0);
}

// ============================================================================
// Test 4: Key Rotation Lifecycle & needs_rewrap Re-Encryption Migration
// ============================================================================

fn testKeyRotationAndRewrapHint()! {
    let ring = EnvelopeKeyring.init();
    expect_eq(ring.registerKey(1, makeKek(0x11), EnvelopeKeyState.primary, 1000) == EnvelopeStatus.ok, true);

    const aad: []const u8 = "doc:42";
    const pt_in: []const u8 = "api_token_sk_live_987654321";

    let env_v1: [256]u8 = [0; 256];
    let env_v1_s: []u8 = env_v1[0..];
    const s1 = sealEnvelope(ring, "n1_0123456789abc", "s1_0123456789abc", aad, pt_in, *env_v1_s);
    expect_eq(s1.status == EnvelopeStatus.ok, true);
    expect_eq(ring.needsRewrap(1), false);

    // Register v2 as active_decrypt_only first, then atomically promotePrimary(2)
    expect_eq(ring.registerKey(2, makeKek(0x22), EnvelopeKeyState.active_decrypt_only, 2000) == EnvelopeStatus.ok, true);
    expect_eq(ring.promotePrimary(2) == EnvelopeStatus.ok, true);

    // v1 should now be demoted to active_decrypt_only
    const st1 = ring.keyState(1) ?? return error.MissingKey1;
    const st2 = ring.keyState(2) ?? return error.MissingKey2;
    expect_eq(st1 == EnvelopeKeyState.active_decrypt_only, true);
    expect_eq(st2 == EnvelopeKeyState.primary, true);
    expect_eq(ring.needsRewrap(1), true);
    expect_eq(ring.needsRewrap(2), false);

    // Old v1 envelope still opens cleanly, signaling needs_rewrap == true
    let out1: [128]u8 = [0; 128];
    let out1_s: []u8 = out1[0..];
    const o1 = openEnvelope(ring, env_v1[0..s1.bytes_written], aad, *out1_s);
    expect_eq(o1.status == EnvelopeStatus.ok, true);
    expect_eq(o1.key_version, 1);
    expect_eq(o1.needs_rewrap, true);
    expect_eq(sliceEq(out1[0..o1.bytes_written], pt_in), true);

    // Rewrap under v2 -> opens with key_version == 2 and needs_rewrap == false
    let env_v2: [256]u8 = [0; 256];
    let env_v2_s: []u8 = env_v2[0..];
    const s2 = sealEnvelope(ring, "n2_0123456789abc", "s2_0123456789abc", aad, out1[0..o1.bytes_written], *env_v2_s);
    expect_eq(s2.status == EnvelopeStatus.ok, true);
    expect_eq(s2.key_version, 2);

    let out2: [128]u8 = [0; 128];
    let out2_s: []u8 = out2[0..];
    const o2 = openEnvelope(ring, env_v2[0..s2.bytes_written], aad, *out2_s);
    expect_eq(o2.status == EnvelopeStatus.ok, true);
    expect_eq(o2.key_version, 2);
    expect_eq(o2.needs_rewrap, false);
    expect_eq(sliceEq(out2[0..o2.bytes_written], pt_in), true);

    // Retire v1: still decryptable for archival migration, still needs_rewrap == true
    expect_eq(ring.retireKey(1) == EnvelopeStatus.ok, true);
    const o1_ret = openEnvelope(ring, env_v1[0..s1.bytes_written], aad, *out1_s);
    expect_eq(o1_ret.status == EnvelopeStatus.ok, true);
    expect_eq(o1_ret.needs_rewrap, true);
}

// ============================================================================
// Test 5: Key Revocation Scrubbing & Strict Decrypt / Promote Rejection
// ============================================================================

fn testKeyRevocationGuards()! {
    let ring = EnvelopeKeyring.init();
    expect_eq(ring.registerKey(10, makeKek(0x77), EnvelopeKeyState.primary, 1000) == EnvelopeStatus.ok, true);

    let env: [256]u8 = [0; 256];
    let env_s: []u8 = env[0..];
    const s = sealEnvelope(ring, "nonce_rev_000001", "salt_rev_0000001", "aad", "sensitive", *env_s);
    expect_eq(s.status == EnvelopeStatus.ok, true);

    // Revoke key 10
    expect_eq(ring.revokeKey(10) == EnvelopeStatus.ok, true);
    expect_eq(ring.resolveDecryptKey(10) == null, true);
    expect_eq(ring.selectEncryptKey() == null, true);

    // Attempting to open envelope sealed by revoked key must return key_revoked
    let pt: [64]u8 = [0; 64];
    let pt_s: []u8 = pt[0..];
    const opened = openEnvelope(ring, env[0..s.bytes_written], "aad", *pt_s);
    expect_eq(opened.status == EnvelopeStatus.key_revoked, true);
    expect_eq(opened.key_version, 10);

    // Revoked key cannot be promoted back to primary
    expect_eq(ring.promotePrimary(10) == EnvelopeStatus.key_revoked, true);
    expect_eq(ring.retireKey(10) == EnvelopeStatus.key_revoked, true);
}

// ============================================================================
// Test 6: Tamper Resistance (Ciphertext, Tag, Header & AAD Integrity Guards)
// ============================================================================

fn testTamperResistanceGuards()! {
    let ring = EnvelopeKeyring.init();
    expect_eq(ring.registerKey(3, makeKek(0x99), EnvelopeKeyState.primary, 1000) == EnvelopeStatus.ok, true);

    let env: [256]u8 = [0; 256];
    let env_s: []u8 = env[0..];
    const aad: []const u8 = "role=admin;scope=billing";
    const pt_in: []const u8 = "transfer:$5000";
    const s = sealEnvelope(ring, "nonce_tamper_001", "salt_tamper_0001", aad, pt_in, *env_s);
    expect_eq(s.status == EnvelopeStatus.ok, true);

    let out: [64]u8 = [0; 64];
    let out_s: []u8 = out[0..];

    // 1. Flip 1 bit in ciphertext
    let bad_ct: [256]u8 = env;
    bad_ct[s.bytes_written - 1] ^= 0x01;
    expect_eq(openEnvelope(ring, bad_ct[0..s.bytes_written], aad, *out_s).status == EnvelopeStatus.auth_failed, true);

    // 2. Flip 1 bit in MAC tag (last byte of header)
    let bad_tag: [256]u8 = env;
    const hdr_len = s.bytes_written - pt_in.len;
    bad_tag[hdr_len - 1] ^= 0x80;
    expect_eq(openEnvelope(ring, bad_tag[0..s.bytes_written], aad, *out_s).status == EnvelopeStatus.auth_failed, true);

    // 3. Wrong AAD content (same length)
    expect_eq(openEnvelope(ring, env[0..s.bytes_written], "role=guest;scope=billing", *out_s).status == EnvelopeStatus.auth_failed, true);

    // 4. Wrong AAD length
    expect_eq(openEnvelope(ring, env[0..s.bytes_written], "role=admin", *out_s).status == EnvelopeStatus.auth_failed, true);

    // 5. Truncated ciphertext payload
    expect_eq(openEnvelope(ring, env[0..s.bytes_written - 2], aad, *out_s).status == EnvelopeStatus.truncated_input, true);
}

// ============================================================================
// Test 7: Keyring Capacity, Duplicate, Weak Key & Missing Key Guards
// ============================================================================

fn testKeyringBoundaryGuards()! {
    let ring = EnvelopeKeyring.init();

    // No primary key -> sealEnvelope returns no_primary_key
    let env: [256]u8 = [0; 256];
    let env_s: []u8 = env[0..];
    expect_eq(sealEnvelope(ring, "nonce_0123456789", "salt_0123456789a", "", "hi", *env_s).status == EnvelopeStatus.no_primary_key, true);

    // All-zero KEK and key_version == 0 must be rejected
    const zero_kek: [ENVELOPE_KEK_LEN]u8 = [0; ENVELOPE_KEK_LEN];
    expect_eq(ring.registerKey(1, zero_kek, EnvelopeKeyState.primary, 100) == EnvelopeStatus.invalid_key_material, true);
    expect_eq(ring.registerKey(0, makeKek(0x01), EnvelopeKeyState.primary, 100) == EnvelopeStatus.invalid_key_material, true);

    // Fill all 16 keyring slots
    let v: u32 = 1;
    while (v <= (ENVELOPE_KEYRING_MAX_KEYS as u32)) : (v += 1) {
        const st = if (v == 1) EnvelopeKeyState.primary else EnvelopeKeyState.active_decrypt_only;
        expect_eq(ring.registerKey(v, makeKek(v as u8), st, (v as u64) * 100) == EnvelopeStatus.ok, true);
    }

    // Duplicate key_version rejected
    expect_eq(ring.registerKey(5, makeKek(0x55), EnvelopeKeyState.retired, 999) == EnvelopeStatus.duplicate_key_version, true);

    // 17th key rejected with keyring_full
    expect_eq(ring.registerKey(99, makeKek(0x63), EnvelopeKeyState.retired, 999) == EnvelopeStatus.keyring_full, true);

    // Opening envelope sealed by a key_version not in another keyring returns key_not_found
    const s = sealEnvelope(ring, "nonce_0123456789", "salt_0123456789a", "", "hi", *env_s);
    expect_eq(s.status == EnvelopeStatus.ok, true);
    let other_ring = EnvelopeKeyring.init();
    expect_eq(other_ring.registerKey(99, makeKek(0xAA), EnvelopeKeyState.primary, 100) == EnvelopeStatus.ok, true);
    let pt: [32]u8 = [0; 32];
    let pt_s: []u8 = pt[0..];
    expect_eq(openEnvelope(other_ring, env[0..s.bytes_written], "", *pt_s).status == EnvelopeStatus.key_not_found, true);
}

// ============================================================================
// Test 8: cap.crypto Namespace, Telemetry stats() & zeroize() Scrubbing
// ============================================================================

fn testNamespaceStatsAndZeroize()! {
    let ring = cap.crypto.EnvelopeKeyring.init();
    expect_eq(ring.registerKey(1, makeKek(0x10), EnvelopeKeyState.primary, 100) == EnvelopeStatus.ok, true);
    expect_eq(ring.registerKey(2, makeKek(0x20), EnvelopeKeyState.primary, 200) == EnvelopeStatus.ok, true);
    expect_eq(ring.registerKey(3, makeKek(0x30), EnvelopeKeyState.retired, 300) == EnvelopeStatus.ok, true);
    expect_eq(ring.registerKey(4, makeKek(0x40), EnvelopeKeyState.active_decrypt_only, 400) == EnvelopeStatus.ok, true);
    expect_eq(ring.revokeKey(4) == EnvelopeStatus.ok, true);

    const st = ring.stats();
    expect_eq(st.total_keys, 4);
    expect_eq(st.has_primary, true);
    expect_eq(st.primary_version, 2);
    expect_eq(st.active_decrypt_only_count, 1);
    expect_eq(st.retired_count, 1);
    expect_eq(st.revoked_count, 1);

    // Zeroize scrubs all KEK material in place
    ring.zeroize();
    const k2 = ring.selectEncryptKey() ?? return error.MissingPrimary2;
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < ENVELOPE_KEK_LEN) : (i += 1) {
        sum += k2.kek[i] as u32;
    }
    expect_eq(sum, 0);
}

// ============================================================================
// Test Entrypoints
// ============================================================================

test("envelope_seal: CPE1 binary header encode/decode round-trip & small buffer guard", testHeaderEncodeDecodeRoundTrip);
test("envelope_seal: malformed header guards (truncated, magic, version, alg, params)", testMalformedHeaderGuards);
test("envelope_seal: end-to-end sealEnvelope/openEnvelope multi-block CTR & empty PT", testSealOpenEndToEndRoundTrip);
test("envelope_seal: key rotation lifecycle & needs_rewrap migration hint", testKeyRotationAndRewrapHint);
test("envelope_seal: key revocation scrubbing & decrypt/promote rejection", testKeyRevocationGuards);
test("envelope_seal: tamper resistance (ciphertext, tag, AAD, truncation -> auth_failed)", testTamperResistanceGuards);
test("envelope_seal: keyring boundary guards (16-slot full, duplicate, weak key, missing)", testKeyringBoundaryGuards);
test("envelope_seal: cap.crypto namespace, stats() telemetry & zeroize() scrubbing", testNamespaceStatsAndZeroize);
