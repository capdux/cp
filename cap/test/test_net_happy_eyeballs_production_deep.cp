// Production Deep Test Suite for cap:net RFC 8305 Happy Eyeballs v2 (lib/net/happy_eyeballs.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    HE_MAX_CANDIDATES,
    HE_DEFAULT_RESOLUTION_DELAY_MS,
    HE_DEFAULT_ATTEMPT_DELAY_MS,
    HE_INVALID_ATTEMPT_ID,
    HeAddressFamily,
    HeActionKind,
    HeCandidate,
    HeConfig,
    HappyEyeballsEngine,
} from "cap:net";

fn makeV6(last_byte: u8, port: u16, id: u32): HeCandidate {
    const ip: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, last_byte];
    return HeCandidate.fromIpv6(ip, port, id);
}

fn makeV4(last_byte: u8, port: u16, id: u32): HeCandidate {
    const ip: [4]u8 = [192, 0, 2, last_byte];
    return HeCandidate.fromIpv4(ip, port, id);
}

// ============================================================================
// Test 1: RFC 8305 dual-stack interleaving & first_family_count preference
// ============================================================================
test("happy_eyeballs: RFC 8305 dual-stack interleaving & first_family_count preference", fn() {
    // Default IPv6-first 1:1 interleaving
    let he = HappyEyeballsEngine.init(HeConfig.defaults(), 1000) ?? return;
    const v6s: [3]HeCandidate = [makeV6(1, 443, 101), makeV6(2, 443, 102), makeV6(3, 443, 103)];
    const v4s: [2]HeCandidate = [makeV4(1, 443, 201), makeV4(2, 443, 202)];

    expect_eq(he.addResolvedAddresses(HeAddressFamily.ipv6, v6s[0..], true, 1000), 3 as usize);
    expect_eq(he.addResolvedAddresses(HeAddressFamily.ipv4, v4s[0..], true, 1000), 2 as usize);

    let order: [8]HeCandidate = [HeCandidate{}; 8];
    let order_s: []HeCandidate = order[0..];
    const n = he.previewInterleavedOrder(*order_s);
    expect_eq(n, 5 as usize);
    // Expected: v6(101), v4(201), v6(102), v4(202), v6(103)
    expect_eq(order[0].addr_id, 101 as u32);
    expect_eq(order[1].addr_id, 201 as u32);
    expect_eq(order[2].addr_id, 102 as u32);
    expect_eq(order[3].addr_id, 202 as u32);
    expect_eq(order[4].addr_id, 103 as u32);

    // Custom first_family_count = 2 -> v6, v6, v4, v6, v4
    let cfg2 = HeConfig.defaults();
    cfg2.first_family_count = 2;
    let he2 = HappyEyeballsEngine.init(cfg2, 1000) ?? return;
    * = he2.addResolvedAddresses(HeAddressFamily.ipv6, v6s[0..], true, 1000);
    * = he2.addResolvedAddresses(HeAddressFamily.ipv4, v4s[0..], true, 1000);
    const n2 = he2.previewInterleavedOrder(*order_s);
    expect_eq(n2, 5 as usize);
    expect_eq(order[0].addr_id, 101 as u32);
    expect_eq(order[1].addr_id, 102 as u32);
    expect_eq(order[2].addr_id, 201 as u32);
    expect_eq(order[3].addr_id, 103 as u32);
    expect_eq(order[4].addr_id, 202 as u32);
});

// ============================================================================
// Test 2: Resolution Delay (50ms) when IPv4 arrives before IPv6 & early wake
// ============================================================================
test("happy_eyeballs: Resolution Delay (50ms) when IPv4 arrives first & early wake on AAAA", fn() {
    let he = HappyEyeballsEngine.init(HeConfig.defaults(), 1000) ?? return;

    // IPv4 arrives at t=1000 while AAAA is still pending -> must wait 50ms until t=1050
    const v4s: [1]HeCandidate = [makeV4(10, 443, 201)];
    expect_eq(he.addResolvedAddresses(HeAddressFamily.ipv4, v4s[0..], true, 1000), 1 as usize);

    const w1 = he.nextAction(1000);
    expect_eq(w1.kind, HeActionKind.wait_until);
    expect_eq(w1.deadline_ms, 1000 + HE_DEFAULT_RESOLUTION_DELAY_MS);

    // AAAA arrives early at t=1020 -> immediately wakes up and starts IPv6!
    const v6s: [1]HeCandidate = [makeV6(10, 443, 101)];
    expect_eq(he.addResolvedAddresses(HeAddressFamily.ipv6, v6s[0..], true, 1020), 1 as usize);

    const a1 = he.nextAction(1020);
    expect_eq(a1.kind, HeActionKind.start_attempt);
    expect_eq(a1.candidate.family, HeAddressFamily.ipv6);
    expect_eq(a1.candidate.addr_id, 101 as u32);

    // Second scenario: AAAA never arrives within 50ms -> IPv4 starts at t=1050
    let he_timeout = HappyEyeballsEngine.init(HeConfig.defaults(), 2000) ?? return;
    * = he_timeout.addResolvedAddresses(HeAddressFamily.ipv4, v4s[0..], true, 2000);
    expect_eq(he_timeout.nextAction(2049).kind, HeActionKind.wait_until);
    const a_v4 = he_timeout.nextAction(2050);
    expect_eq(a_v4.kind, HeActionKind.start_attempt);
    expect_eq(a_v4.candidate.family, HeAddressFamily.ipv4);
});

// ============================================================================
// Test 3: Connection Attempt Delay (250ms), winner election & cancel collection
// ============================================================================
test("happy_eyeballs: Connection Attempt Delay (250ms), winner election & cancel pending", fn() {
    let he = HappyEyeballsEngine.init(HeConfig.defaults(), 5000) ?? return;
    const v6s: [2]HeCandidate = [makeV6(1, 443, 101), makeV6(2, 443, 102)];
    const v4s: [1]HeCandidate = [makeV4(1, 443, 201)];
    * = he.addResolvedAddresses(HeAddressFamily.ipv6, v6s[0..], true, 5000);
    * = he.addResolvedAddresses(HeAddressFamily.ipv4, v4s[0..], true, 5000);

    // t=5000: start IPv6 #1
    const a1 = he.nextAction(5000);
    expect_eq(a1.kind, HeActionKind.start_attempt);
    expect_eq(a1.candidate.addr_id, 101 as u32);

    // t=5100: must wait until 5250
    const w = he.nextAction(5100);
    expect_eq(w.kind, HeActionKind.wait_until);
    expect_eq(w.deadline_ms, 5000 + HE_DEFAULT_ATTEMPT_DELAY_MS);

    // t=5250: start IPv4 #1 (interleaved)
    const a2 = he.nextAction(5250);
    expect_eq(a2.kind, HeActionKind.start_attempt);
    expect_eq(a2.candidate.addr_id, 201 as u32);

    // t=5500: start IPv6 #2
    const a3 = he.nextAction(5500);
    expect_eq(a3.kind, HeActionKind.start_attempt);
    expect_eq(a3.candidate.addr_id, 102 as u32);

    expect_eq(he.stats().in_flight_attempts, 3 as usize);

    // IPv4 #1 (a2) wins -> a1 and a3 must be flagged for cancellation!
    expect_true(he.markAttemptSucceeded(a2.attempt_id));
    const done = he.nextAction(5510);
    expect_eq(done.kind, HeActionKind.connected);
    expect_eq(done.attempt_id, a2.attempt_id);
    expect_eq(done.candidate.addr_id, 201 as u32);

    // Collect cancellations in batches of 1
    let c_buf: [1]u16 = [HE_INVALID_ATTEMPT_ID; 1];
    let c_s: []u16 = c_buf[0..];
    expect_eq(he.collectAttemptsToCancel(*c_s), 1 as usize);
    const first_cancel: u16 = c_buf[0];
    expect_eq(he.collectAttemptsToCancel(*c_s), 1 as usize);
    const second_cancel: u16 = c_buf[0];
    expect_eq(he.collectAttemptsToCancel(*c_s), 0 as usize);
    expect_true(first_cancel != second_cancel);
    expect_true(first_cancel != a2.attempt_id && second_cancel != a2.attempt_id);
});

// ============================================================================
// Test 4: Immediate fallback on attempt failure (bypasses remaining 250ms delay)
// ============================================================================
test("happy_eyeballs: immediate fallback on attempt failure bypasses 250ms delay", fn() {
    let he = HappyEyeballsEngine.init(HeConfig.defaults(), 1000) ?? return;
    const v6s: [1]HeCandidate = [makeV6(1, 443, 101)];
    const v4s: [1]HeCandidate = [makeV4(1, 443, 201)];
    * = he.addResolvedAddresses(HeAddressFamily.ipv6, v6s[0..], true, 1000);
    * = he.addResolvedAddresses(HeAddressFamily.ipv4, v4s[0..], true, 1000);

    const a1 = he.nextAction(1000);
    expect_eq(a1.kind, HeActionKind.start_attempt);
    expect_eq(a1.candidate.family, HeAddressFamily.ipv6);

    // IPv6 fails fast (e.g. ENETUNREACH) at t=1005 -> nextAction(1005) immediately dials IPv4!
    expect_true(he.markAttemptFailed(a1.attempt_id, 1005));
    const a2 = he.nextAction(1005);
    expect_eq(a2.kind, HeActionKind.start_attempt);
    expect_eq(a2.candidate.family, HeAddressFamily.ipv4);
    expect_eq(a2.candidate.addr_id, 201 as u32);
    expect_eq(he.stats().failed_attempts, 1 as usize);
});

// ============================================================================
// Test 5: Pure IPv4-only / pure IPv6-only fallback & all_failed terminal state
// ============================================================================
test("happy_eyeballs: pure single-stack lists & all_failed terminal state", fn() {
    let he = HappyEyeballsEngine.init(HeConfig.defaults(), 0) ?? return;

    // AAAA completes empty; only IPv4 exists -> markDnsComplete immediately lifts resolution delay
    const v4s: [1]HeCandidate = [makeV4(5, 80, 301)];
    * = he.addResolvedAddresses(HeAddressFamily.ipv4, v4s[0..], true, 0);
    he.markDnsComplete(HeAddressFamily.ipv6, 5);

    const a1 = he.nextAction(5);
    expect_eq(a1.kind, HeActionKind.start_attempt);
    expect_eq(a1.candidate.family, HeAddressFamily.ipv4);

    // While in-flight, nextAction returns wait_until(0)
    const w = he.nextAction(10);
    expect_eq(w.kind, HeActionKind.wait_until);
    expect_eq(w.deadline_ms, 0 as u64);

    // Fail the only candidate -> terminal all_failed
    expect_true(he.markAttemptFailed(a1.attempt_id, 20));
    const fin = he.nextAction(20);
    expect_eq(fin.kind, HeActionKind.all_failed);
});

// ============================================================================
// Test 6: Address deduplication, capacity cap (32) & monotonic time guard
// ============================================================================
test("happy_eyeballs: deduplication, capacity limit (32) & monotonic time rollback guard", fn() {
    let he = HappyEyeballsEngine.init(HeConfig.defaults(), 1000) ?? return;

    // Duplicate candidates ignored
    const dup_v4: [2]HeCandidate = [makeV4(1, 443, 1), makeV4(1, 443, 99)];
    expect_eq(he.addResolvedAddresses(HeAddressFamily.ipv4, dup_v4[0..], false, 1000), 1 as usize);

    // Fill up to HE_MAX_CANDIDATES (32)
    let i: usize = 2;
    while (i <= HE_MAX_CANDIDATES + 5) : (i += 1) {
        const one: [1]HeCandidate = [makeV4(i as u8, 443, i as u32)];
        * = he.addResolvedAddresses(HeAddressFamily.ipv4, one[0..], false, 1000);
    }
    expect_eq(he.stats().total_candidates, HE_MAX_CANDIDATES);

    // Monotonic time rollback guard: passing now_ms < last_now_ms must not regress timers
    he.markDnsComplete(HeAddressFamily.ipv6, 1000);
    he.markDnsComplete(HeAddressFamily.ipv4, 1000);
    const a1 = he.nextAction(1100);
    expect_eq(a1.kind, HeActionKind.start_attempt);
    const rolled = he.nextAction(500); // regressed timestamp -> clamped to 1100 < 1350
    expect_eq(rolled.kind, HeActionKind.wait_until);
    expect_eq(rolled.deadline_ms, 1100 + HE_DEFAULT_ATTEMPT_DELAY_MS);
});

// ============================================================================
// Test 7: Strict boundary guards (invalid config, duplicate/unknown status)
// ============================================================================
test("happy_eyeballs: strict boundary guards (config, duplicate/unknown status reports)", fn() {
    let bad1 = HeConfig.defaults();
    bad1.first_family_count = 0;
    expect_true(HappyEyeballsEngine.init(bad1, 0) == null);

    let bad2 = HeConfig.defaults();
    bad2.connection_attempt_delay_ms = 0;
    expect_true(HappyEyeballsEngine.init(bad2, 0) == null);

    let bad3 = HeConfig.defaults();
    bad3.resolution_delay_ms = 999_999;
    expect_true(HappyEyeballsEngine.init(bad3, 0) == null);

    let he = HappyEyeballsEngine.init(HeConfig.defaults(), 0) ?? return;
    const v6s: [1]HeCandidate = [makeV6(1, 443, 1)];
    // Family mismatch candidate silently skipped
    expect_eq(he.addResolvedAddresses(HeAddressFamily.ipv4, v6s[0..], true, 0), 0 as usize);
    expect_eq(he.addResolvedAddresses(HeAddressFamily.ipv6, v6s[0..], true, 0), 1 as usize);

    // Cannot mark pending or out-of-bounds attempt as succeeded/failed
    expect_true(!he.markAttemptSucceeded(0));
    expect_true(!he.markAttemptFailed(0, 10));
    expect_true(!he.markAttemptSucceeded(99));
    expect_true(!he.markAttemptFailed(99, 10));

    // Start attempt 0, succeed once, reject duplicate succeed or subsequent fail
    const a = he.nextAction(10);
    expect_eq(a.kind, HeActionKind.start_attempt);
    expect_true(he.markAttemptSucceeded(a.attempt_id));
    expect_true(!he.markAttemptSucceeded(a.attempt_id));
    expect_true(!he.markAttemptFailed(a.attempt_id, 20));
});
