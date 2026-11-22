// Origin: Pure-CP Stdlib Test Suite (lib/crypto/certPin.cp) | Created: 2026-11-23
// Production Deep Verification Suite for TLS SPKI Public-Key Pinning & Backup Pin Rotation Engine:
//   1. Zero-alloc canonical RFC 4648 Base64 & `pin-sha256="..."` directive round-trip + malformed guard
//   2. Backup-pin rotation guard (`require_backup_pin`), zero/duplicate rejection & zero-downtime addPin/removePin
//   3. Constant-time chain verification: matched (leaf & intermediate), enforce vs report_only, expired & no_policy
//   4. Hostname normalization (case/root-dot), longest-parent `include_subdomains` inheritance & boundary safety
//   5. 32-host capacity saturation, slot reuse after removal & `rotatePins` replacement
//   6. Global telemetry stats, resetStats, constant-time equality & cap.crypto facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CERT_PIN_MAX_POLICIES,
    CERT_PIN_MAX_PINS_PER_HOST,
    CERT_PIN_HASH_LEN,
    CERT_PIN_B64_LEN,
    CERT_PIN_DIRECTIVE_LEN,
    CertPinMode,
    CertPinOpStatus,
    CertPinVerifyStatus,
    CertPinStats,
    certPinConstantTimeEq,
    encodePinSha256Base64,
    decodePinSha256Base64,
    formatPinDirective,
    parsePinDirective,
    CertPinStore,
} from "cap:crypto";

fn makePin(seed: u8): [32]u8 {
    let h: [32]u8 = [0; 32];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        h[i] = seed +% @as(u8, @intCast(i * 7 + 3));
    }
    return h;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("CertPin: canonical RFC 4648 Base64 & pin-sha256 directive round-trip + guards") {
    const pin = makePin(0x42);

    let b64_buf: [64]u8 = [0; 64];
    let b64_s: []u8 = b64_buf[0..];
    const b64_len = encodePinSha256Base64(pin, *b64_s) ?? 0;
    expect_eq(b64_len, CERT_PIN_B64_LEN);
    expect_eq(b64_buf[43], @as(u8, '='));

    const decoded = decodePinSha256Base64(b64_buf[0..b64_len]);
    expect_true(decoded != null);
    if (h := decoded) {
        expect_true(certPinConstantTimeEq(h, pin));
    }

    // Format and parse `pin-sha256="..."` directive (with surrounding whitespace tolerated on parse)
    let dir_buf: [64]u8 = [0; 64];
    let dir_s: []u8 = dir_buf[0..];
    const dir_len = formatPinDirective(pin, *dir_s) ?? 0;
    expect_eq(dir_len, CERT_PIN_DIRECTIVE_LEN);
    expect_true(bytesEq(dir_buf[0..12], "pin-sha256=\""));
    expect_eq(dir_buf[dir_len - 1], @as(u8, '"'));

    const parsed = parsePinDirective(dir_buf[0..dir_len]);
    expect_true(parsed != null);
    if (h2 := parsed) {
        expect_true(certPinConstantTimeEq(h2, pin));
    }

    // Reject malformed Base64, non-canonical trailing bits, wrong prefix, or tiny buffer
    expect_true(decodePinSha256Base64("short") == null);
    expect_true(decodePinSha256Base64("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA!") == null);
    // Non-canonical 43rd char ('B' = 1 -> low 2 bits != 0)
    expect_true(decodePinSha256Base64("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB=") == null);
    expect_true(parsePinDirective("pin-sha1=\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\"") == null);

    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(encodePinSha256Base64(pin, *tiny_s) == null);
    expect_true(formatPinDirective(pin, *tiny_s) == null);
}

test("CertPin: backup-pin rotation guard, duplicate/zero rejection & addPin/removePin") {
    let store = CertPinStore.init();
    const p1 = makePin(1);
    const p2 = makePin(2);
    const p3 = makePin(3);
    const zero_pin: [32]u8 = [0; 32];

    // Single pin rejected when require_backup_pin == true
    const single: [1][32]u8 = [p1];
    expect_eq(
        store.addPolicy("api.cap.dev", single[0..], false, true, CertPinMode.enforce, 2_000),
        CertPinOpStatus.backup_pin_required,
    );

    // Duplicate pins rejected
    const dup: [2][32]u8 = [p1, p1];
    expect_eq(
        store.addPolicy("api.cap.dev", dup[0..], false, true, CertPinMode.enforce, 2_000),
        CertPinOpStatus.duplicate_pin,
    );

    // All-zero placeholder pin rejected
    const with_zero: [2][32]u8 = [p1, zero_pin];
    expect_eq(
        store.addPolicy("api.cap.dev", with_zero[0..], false, true, CertPinMode.enforce, 2_000),
        CertPinOpStatus.invalid_pin_list,
    );

    // Valid primary + backup pin set accepted
    const pair: [2][32]u8 = [p1, p2];
    expect_eq(
        store.addPolicy("api.cap.dev", pair[0..], false, true, CertPinMode.enforce, 2_000),
        CertPinOpStatus.ok,
    );

    // Cannot remove p1 while only 2 pins exist (would violate require_backup_pin)
    expect_eq(store.removePin("api.cap.dev", p1), CertPinOpStatus.backup_pin_required);

    // Zero-downtime rotation: add p3 first, then safely remove retired p1
    expect_eq(store.addPin("api.cap.dev", p1), CertPinOpStatus.duplicate_pin);
    expect_eq(store.addPin("api.cap.dev", p3), CertPinOpStatus.ok);
    expect_eq(store.removePin("api.cap.dev", p1), CertPinOpStatus.ok);

    // Verify p1 no longer matches, p3 now matches
    const c_old: [1][32]u8 = [p1];
    const c_new: [1][32]u8 = [p3];
    expect_eq(store.verifyChain("api.cap.dev", c_old[0..], 1_500).status, CertPinVerifyStatus.mismatch_blocked);
    expect_eq(store.verifyChain("api.cap.dev", c_new[0..], 1_500).status, CertPinVerifyStatus.matched);
}

test("CertPin: constant-time chain match, enforce vs report_only, expiry & invalid input") {
    let store = CertPinStore.init();
    const p1 = makePin(11);
    const p2 = makePin(12);
    const p_other = makePin(99);
    const pins: [2][32]u8 = [p1, p2];

    expect_eq(
        store.addPolicy("strict.cap.dev", pins[0..], false, true, CertPinMode.enforce, 1_000),
        CertPinOpStatus.ok,
    );
    expect_eq(
        store.addPolicy("canary.cap.dev", pins[0..], false, true, CertPinMode.report_only, 1_000),
        CertPinOpStatus.ok,
    );

    // Match via intermediate CA (index 1 in chain matches backup pin index 1)
    const chain_hit: [3][32]u8 = [p_other, p2, makePin(55)];
    const r_hit = store.verifyChain("strict.cap.dev", chain_hit[0..], 900);
    expect_eq(r_hit.status, CertPinVerifyStatus.matched);
    expect_true(r_hit.allowed);
    expect_eq(r_hit.matched_chain_index, @as(usize, 1));
    expect_eq(r_hit.matched_pin_index, @as(usize, 1));

    // Mismatch on enforce -> blocked
    const chain_miss: [2][32]u8 = [p_other, makePin(77)];
    const r_blk = store.verifyChain("strict.cap.dev", chain_miss[0..], 900);
    expect_eq(r_blk.status, CertPinVerifyStatus.mismatch_blocked);
    expect_true(!r_blk.allowed);

    // Mismatch on report_only -> allowed + reported
    const r_rep = store.verifyChain("canary.cap.dev", chain_miss[0..], 900);
    expect_eq(r_rep.status, CertPinVerifyStatus.mismatch_reported);
    expect_true(r_rep.allowed);

    // Expired pin policy -> fail-open with pin_expired telemetry
    const r_exp = store.verifyChain("strict.cap.dev", chain_miss[0..], 1_001);
    expect_eq(r_exp.status, CertPinVerifyStatus.pin_expired);
    expect_true(r_exp.allowed);

    // Unpinned host -> no_pin_policy
    const r_none = store.verifyChain("unpinned.example.com", chain_miss[0..], 900);
    expect_eq(r_none.status, CertPinVerifyStatus.no_pin_policy);
    expect_true(r_none.allowed);

    // Empty chain or invalid hostname -> invalid_input
    const empty_chain: [0][32]u8 = [];
    expect_eq(store.verifyChain("strict.cap.dev", empty_chain[0..], 900).status, CertPinVerifyStatus.invalid_input);
    expect_eq(store.verifyChain("bad host!", chain_hit[0..], 900).status, CertPinVerifyStatus.invalid_input);
}

test("CertPin: hostname normalization, include_subdomains precedence & boundary guard") {
    let store = CertPinStore.init();
    const p_root1 = makePin(21);
    const p_root2 = makePin(22);
    const p_sub1 = makePin(31);
    const p_sub2 = makePin(32);

    const root_pins: [2][32]u8 = [p_root1, p_root2];
    const sub_pins: [2][32]u8 = [p_sub1, p_sub2];

    // Register root domain with include_subdomains=true, plus more specific child domain
    expect_eq(
        store.addPolicy("CAP.DEV.", root_pins[0..], true, true, CertPinMode.enforce, 5_000),
        CertPinOpStatus.ok,
    );
    expect_eq(
        store.addPolicy("payments.cap.dev", sub_pins[0..], true, true, CertPinMode.enforce, 5_000),
        CertPinOpStatus.ok,
    );

    const c_root: [1][32]u8 = [p_root1];
    const c_sub: [1][32]u8 = [p_sub1];

    // Subdomain inherits cap.dev (case-insensitive + trailing root dot)
    const r1 = store.verifyChain("API.CAP.DEV.", c_root[0..], 1_000);
    expect_eq(r1.status, CertPinVerifyStatus.matched);
    expect_true(r1.via_subdomain);

    // Longer parent `payments.cap.dev` overrides `cap.dev` for `eu.payments.cap.dev`
    const r2 = store.verifyChain("eu.payments.cap.dev", c_sub[0..], 1_000);
    expect_eq(r2.status, CertPinVerifyStatus.matched);
    expect_true(r2.via_subdomain);
    const r3 = store.verifyChain("eu.payments.cap.dev", c_root[0..], 1_000);
    expect_eq(r3.status, CertPinVerifyStatus.mismatch_blocked);

    // Strict DNS boundary: `evil-cap.dev` MUST NOT match `cap.dev`
    const r_evil = store.verifyChain("evil-cap.dev", c_sub[0..], 1_000);
    expect_eq(r_evil.status, CertPinVerifyStatus.no_pin_policy);
}

test("CertPin: 32-host capacity saturation, slot reuse & rotatePins") {
    let store = CertPinStore.init();
    const pins: [2][32]u8 = [makePin(41), makePin(42)];

    let hbuf: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < CERT_PIN_MAX_POLICIES) : (i += 1) {
        hbuf[0] = 'h';
        hbuf[1] = @as(u8, @intCast('0' + (i / 10)));
        hbuf[2] = @as(u8, @intCast('0' + (i % 10)));
        hbuf[3] = '.';
        hbuf[4] = 'i';
        hbuf[5] = 'o';
        expect_eq(
            store.addPolicy(hbuf[0..6], pins[0..], false, true, CertPinMode.enforce, 9_000),
            CertPinOpStatus.ok,
        );
    }
    expect_eq(store.stats().active_policies, CERT_PIN_MAX_POLICIES);
    expect_eq(
        store.addPolicy("h00.io", pins[0..], false, true, CertPinMode.enforce, 9_000),
        CertPinOpStatus.host_exists,
    );
    expect_eq(
        store.addPolicy("overflow.io", pins[0..], false, true, CertPinMode.enforce, 9_000),
        CertPinOpStatus.capacity_full,
    );

    // Rotate pins in place for h00.io
    const new_pins: [2][32]u8 = [makePin(61), makePin(62)];
    expect_eq(store.rotatePins("h00.io", new_pins[0..], 12_000), CertPinOpStatus.ok);
    const c_new: [1][32]u8 = [makePin(61)];
    expect_eq(store.verifyChain("h00.io", c_new[0..], 11_000).status, CertPinVerifyStatus.matched);

    // Remove and reuse freed slot
    expect_eq(store.removePolicy("h05.io"), CertPinOpStatus.ok);
    expect_eq(store.removePolicy("h05.io"), CertPinOpStatus.not_found);
    expect_eq(
        store.addPolicy("reused.io", pins[0..], false, true, CertPinMode.enforce, 9_000),
        CertPinOpStatus.ok,
    );
    expect_eq(store.stats().active_policies, CERT_PIN_MAX_POLICIES);
}

test("CertPin: global telemetry stats, reset & cap.crypto facade parity") {
    let store = cap.crypto.CertPinStore.init();
    const p1 = makePin(81);
    const p2 = makePin(82);
    const pins: [2][32]u8 = [p1, p2];
    * = store.addPolicy("a.dev", pins[0..], false, true, CertPinMode.enforce, 1_000);
    * = store.addPolicy("b.dev", pins[0..], false, true, CertPinMode.report_only, 1_000);

    const c_ok: [1][32]u8 = [p1];
    const c_bad: [1][32]u8 = [makePin(99)];

    * = store.verifyChain("a.dev", c_ok[0..], 500);   // matched
    * = store.verifyChain("a.dev", c_bad[0..], 500);  // mismatch_blocked
    * = store.verifyChain("b.dev", c_bad[0..], 500);  // mismatch_reported
    * = store.verifyChain("a.dev", c_bad[0..], 1_500); // pin_expired
    * = store.verifyChain("c.dev", c_bad[0..], 500);  // no_pin_policy
    * = store.verifyChain("", c_bad[0..], 500);       // invalid_input

    const st: CertPinStats = store.stats();
    expect_eq(st.active_policies, @as(usize, 2));
    expect_eq(st.policy_capacity, CERT_PIN_MAX_POLICIES);
    expect_eq(st.total_verifications, @as(u64, 6));
    expect_eq(st.matched_count, @as(u64, 1));
    expect_eq(st.mismatch_blocked_count, @as(u64, 1));
    expect_eq(st.mismatch_reported_count, @as(u64, 1));
    expect_eq(st.expired_count, @as(u64, 1));
    expect_eq(st.no_policy_count, @as(u64, 1));
    expect_eq(st.invalid_input_count, @as(u64, 1));

    store.resetStats();
    expect_eq(store.stats().total_verifications, @as(u64, 0));
    expect_eq(store.stats().active_policies, @as(usize, 2));
    expect_eq(CERT_PIN_MAX_PINS_PER_HOST, @as(usize, 8));
    expect_eq(CERT_PIN_HASH_LEN, @as(usize, 32));
}
