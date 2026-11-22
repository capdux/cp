// Origin: Pure-CP Stdlib Test Suite (lib/a2a/trustChain.cp) | Created: 2026-11-22
// Production Deep Verification Suite for A2A Capability-Attenuated Delegation Trust Chain:
//   1. Multi-hop strictly attenuated delegation chain (3 hops) & exact effective privilege computation
//   2. Privilege escalation guards (capability superset, cost inflation, time-window expansion)
//   3. Hop limit attenuation & max-chain-length (>16) guards
//   4. Cycle detection (self-loop, multi-hop cycle) & broken subject->issuer linkage detection
//   5. Time window (not_yet_valid / expired / invalid_time_window) & revocation (ID list + epoch)
//   6. Leaf task admission guards (missing_capability & cost_exceeded) with diagnostic effective bounds
//   7. Empty chain, invalid zero agent IDs, TrustChain builder guards & cap.a2a facade

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    TRUST_CHAIN_MAX_HOPS,
    TrustVerifyStatus,
    TrustDelegationLink,
    delegateLink,
    verifyTrustChain,
    verifyTrustChainWithEpoch,
    TrustChain,
} from "cap:a2a";

// ============================================================================
// Test 1: Multi-Hop Monotonic Attenuation & Effective Privilege Computation
// ============================================================================
test("trustChain: 3-hop attenuated delegation chain passes and computes exact leaf privileges", () => {
    const root = TrustDelegationLink{
        issuer_agent_id: 100,
        subject_agent_id: 200,
        capability_mask: 0b1111,
        max_cost_micro_usd: 1_000_000,
        not_before_ms: 1000,
        expires_ms: 10000,
        max_further_hops: 4,
        revocation_epoch: 2,
    };

    let tc = TrustChain.initRoot(root) ?? return;
    expect_true(tc.appendDelegation(300, 0b0111, 500_000, 1500, 8000, 2, 2));
    expect_true(tc.appendDelegation(400, 0b0011, 150_000, 2000, 6000, 1, 2));
    expect_eq(tc.len(), 3);

    const no_revoked: [0]u64 = [];
    const res = tc.verify(0b0010, 120_000, 3000, no_revoked[0..]);
    expect_true(res.isOk());
    expect_eq(res.status, TrustVerifyStatus.ok);
    expect_eq(res.effective_capabilities, 0b0011);
    expect_eq(res.effective_max_cost_micro_usd, 150_000);
    expect_eq(res.effective_not_before_ms, 2000);
    expect_eq(res.effective_expires_ms, 6000);
    expect_eq(res.remaining_further_hops, 1);
    expect_eq(res.root_issuer_id, 100);
    expect_eq(res.leaf_subject_id, 400);
    expect_eq(res.hop_count, 3);
});

// ============================================================================
// Test 2: Privilege Escalation Guards (Capability, Cost & Time Window)
// ============================================================================
test("trustChain: rejects capability escalation, budget inflation & time-window expansion", () => {
    const parent = TrustDelegationLink{
        issuer_agent_id: 1,
        subject_agent_id: 2,
        capability_mask: 0b0101,
        max_cost_micro_usd: 200_000,
        not_before_ms: 2000,
        expires_ms: 8000,
        max_further_hops: 3,
        revocation_epoch: 1,
    };

    // delegateLink rejects all 4 forms of privilege escalation
    expect_true(delegateLink(parent, 3, 0b0111, 100_000, 2000, 7000, 2, 1) == null); // extra bit
    expect_true(delegateLink(parent, 3, 0b0101, 250_000, 2000, 7000, 2, 1) == null); // higher cost
    expect_true(delegateLink(parent, 3, 0b0101, 100_000, 1500, 7000, 2, 1) == null); // earlier nbf
    expect_true(delegateLink(parent, 3, 0b0101, 100_000, 2000, 9000, 2, 1) == null); // later exp

    // Direct forged child link passed to verifyTrustChain must be caught at hop index 1
    const no_revoked: [0]u64 = [];
    const forged_cap: [2]TrustDelegationLink = [
        parent,
        TrustDelegationLink{
            issuer_agent_id: 2,
            subject_agent_id: 3,
            capability_mask: 0b1101, // escalated bit 3!
            max_cost_micro_usd: 100_000,
            not_before_ms: 2500,
            expires_ms: 7000,
            max_further_hops: 2,
            revocation_epoch: 1,
        },
    ];
    const r_cap = verifyTrustChain(forged_cap[0..], 0b0001, 10_000, 3000, no_revoked[0..]);
    expect_eq(r_cap.status, TrustVerifyStatus.privilege_escalation);
    expect_eq(r_cap.failed_hop_index, 1);

    const forged_cost: [2]TrustDelegationLink = [
        parent,
        TrustDelegationLink{
            issuer_agent_id: 2,
            subject_agent_id: 3,
            capability_mask: 0b0101,
            max_cost_micro_usd: 300_000, // inflated cost!
            not_before_ms: 2500,
            expires_ms: 7000,
            max_further_hops: 2,
            revocation_epoch: 1,
        },
    ];
    const r_cost = verifyTrustChain(forged_cost[0..], 0b0001, 10_000, 3000, no_revoked[0..]);
    expect_eq(r_cost.status, TrustVerifyStatus.privilege_escalation);
    expect_eq(r_cost.failed_hop_index, 1);
});

// ============================================================================
// Test 3: Hop Limit Attenuation & Max Chain Length (>16) Guards
// ============================================================================
test("trustChain: enforces hop_limit_exceeded on non-decreasing hops & >16 chain length", () => {
    const no_revoked: [0]u64 = [];

    // Equal max_further_hops (2 -> 2) violates strict decrease
    const non_dec: [2]TrustDelegationLink = [
        TrustDelegationLink{ issuer_agent_id: 1, subject_agent_id: 2, capability_mask: 0b11, max_cost_micro_usd: 100, not_before_ms: 0, expires_ms: 1000, max_further_hops: 2 },
        TrustDelegationLink{ issuer_agent_id: 2, subject_agent_id: 3, capability_mask: 0b11, max_cost_micro_usd: 100, not_before_ms: 0, expires_ms: 1000, max_further_hops: 2 },
    ];
    expect_eq(verifyTrustChain(non_dec[0..], 0b01, 10, 500, no_revoked[0..]).status, TrustVerifyStatus.hop_limit_exceeded);

    // Parent max_further_hops == 0 cannot delegate further
    const zero_hop_parent = TrustDelegationLink{
        issuer_agent_id: 1, subject_agent_id: 2, capability_mask: 0b11,
        max_cost_micro_usd: 100, not_before_ms: 0, expires_ms: 1000, max_further_hops: 0,
    };
    expect_true(delegateLink(zero_hop_parent, 3, 0b01, 50, 0, 1000, 0, 0) == null);

    // Chain length 17 (> TRUST_CHAIN_MAX_HOPS == 16) rejected
    let too_long: [17]TrustDelegationLink = [TrustDelegationLink{}; 17];
    expect_eq(TRUST_CHAIN_MAX_HOPS, 16);
    expect_eq(verifyTrustChain(too_long[0..], 0, 0, 0, no_revoked[0..]).status, TrustVerifyStatus.hop_limit_exceeded);
});

// ============================================================================
// Test 4: Cycle Detection (Self-Loop & Multi-Hop) & Broken Subject Link
// ============================================================================
test("trustChain: detects self-signed loop, multi-hop cycle and broken subject-issuer chain", () => {
    const no_revoked: [0]u64 = [];

    // Self-loop: 10 -> 10
    const self_loop: [1]TrustDelegationLink = [
        TrustDelegationLink{ issuer_agent_id: 10, subject_agent_id: 10, capability_mask: 0b11, max_cost_micro_usd: 100, not_before_ms: 0, expires_ms: 1000, max_further_hops: 2 },
    ];
    expect_eq(verifyTrustChain(self_loop[0..], 0b01, 10, 500, no_revoked[0..]).status, TrustVerifyStatus.cycle_detected);

    // 3-hop cycle: 1 -> 2 -> 3 -> 1
    const tri_cycle: [3]TrustDelegationLink = [
        TrustDelegationLink{ issuer_agent_id: 1, subject_agent_id: 2, capability_mask: 0b11, max_cost_micro_usd: 100, not_before_ms: 0, expires_ms: 1000, max_further_hops: 4 },
        TrustDelegationLink{ issuer_agent_id: 2, subject_agent_id: 3, capability_mask: 0b11, max_cost_micro_usd: 100, not_before_ms: 0, expires_ms: 1000, max_further_hops: 3 },
        TrustDelegationLink{ issuer_agent_id: 3, subject_agent_id: 1, capability_mask: 0b11, max_cost_micro_usd: 100, not_before_ms: 0, expires_ms: 1000, max_further_hops: 2 },
    ];
    const r_cyc = verifyTrustChain(tri_cycle[0..], 0b01, 10, 500, no_revoked[0..]);
    expect_eq(r_cyc.status, TrustVerifyStatus.cycle_detected);
    expect_eq(r_cyc.failed_hop_index, 2);

    // Broken link: link[0].subject (2) != link[1].issuer (99)
    const broken: [2]TrustDelegationLink = [
        TrustDelegationLink{ issuer_agent_id: 1, subject_agent_id: 2, capability_mask: 0b11, max_cost_micro_usd: 100, not_before_ms: 0, expires_ms: 1000, max_further_hops: 3 },
        TrustDelegationLink{ issuer_agent_id: 99, subject_agent_id: 3, capability_mask: 0b11, max_cost_micro_usd: 100, not_before_ms: 0, expires_ms: 1000, max_further_hops: 2 },
    ];
    const r_brk = verifyTrustChain(broken[0..], 0b01, 10, 500, no_revoked[0..]);
    expect_eq(r_brk.status, TrustVerifyStatus.broken_subject_link);
    expect_eq(r_brk.failed_hop_index, 1);
});

// ============================================================================
// Test 5: Time Validity (not_yet_valid / expired) & Revocation (ID & Epoch)
// ============================================================================
test("trustChain: enforces not_yet_valid, expired, revoked agent IDs & revocation epoch", () => {
    const chain: [2]TrustDelegationLink = [
        TrustDelegationLink{ issuer_agent_id: 10, subject_agent_id: 20, capability_mask: 0b111, max_cost_micro_usd: 500, not_before_ms: 1000, expires_ms: 5000, max_further_hops: 2, revocation_epoch: 3 },
        TrustDelegationLink{ issuer_agent_id: 20, subject_agent_id: 30, capability_mask: 0b011, max_cost_micro_usd: 300, not_before_ms: 2000, expires_ms: 4000, max_further_hops: 1, revocation_epoch: 3 },
    ];
    const no_revoked: [0]u64 = [];

    // Before leaf not_before_ms (1500 < 2000) -> not_yet_valid at hop 1
    const r_nbf = verifyTrustChain(chain[0..], 0b001, 100, 1500, no_revoked[0..]);
    expect_eq(r_nbf.status, TrustVerifyStatus.not_yet_valid);
    expect_eq(r_nbf.failed_hop_index, 1);

    // After leaf expires_ms (4500 > 4000) -> expired at hop 1
    const r_exp = verifyTrustChain(chain[0..], 0b001, 100, 4500, no_revoked[0..]);
    expect_eq(r_exp.status, TrustVerifyStatus.expired);
    expect_eq(r_exp.failed_hop_index, 1);

    // Revoked intermediary agent 20 -> revoked
    const revoked_mid: [1]u64 = [20];
    expect_eq(verifyTrustChain(chain[0..], 0b001, 100, 3000, revoked_mid[0..]).status, TrustVerifyStatus.revoked);

    // Epoch floor 4 > link revocation_epoch 3 -> revoked
    expect_eq(verifyTrustChainWithEpoch(chain[0..], 0b001, 100, 3000, no_revoked[0..], 4).status, TrustVerifyStatus.revoked);
    expect_eq(verifyTrustChainWithEpoch(chain[0..], 0b001, 100, 3000, no_revoked[0..], 3).status, TrustVerifyStatus.ok);
});

// ============================================================================
// Test 6: Leaf Task Admission (missing_capability & cost_exceeded)
// ============================================================================
test("trustChain: enforces required_caps and est_cost against attenuated leaf privileges", () => {
    const chain: [2]TrustDelegationLink = [
        TrustDelegationLink{ issuer_agent_id: 1, subject_agent_id: 2, capability_mask: 0b1111, max_cost_micro_usd: 1000, not_before_ms: 0, expires_ms: 9000, max_further_hops: 2 },
        TrustDelegationLink{ issuer_agent_id: 2, subject_agent_id: 3, capability_mask: 0b0011, max_cost_micro_usd: 250, not_before_ms: 0, expires_ms: 9000, max_further_hops: 1 },
    ];
    const no_revoked: [0]u64 = [];

    // Requesting bit 2 (0b0100) which was attenuated away at hop 1 -> missing_capability
    const r_cap = verifyTrustChain(chain[0..], 0b0101, 100, 1000, no_revoked[0..]);
    expect_eq(r_cap.status, TrustVerifyStatus.missing_capability);
    expect_eq(r_cap.effective_capabilities, 0b0011);

    // Requesting cost 300 > leaf max_cost 250 -> cost_exceeded
    const r_cost = verifyTrustChain(chain[0..], 0b0011, 300, 1000, no_revoked[0..]);
    expect_eq(r_cost.status, TrustVerifyStatus.cost_exceeded);
    expect_eq(r_cost.effective_max_cost_micro_usd, 250);
});

// ============================================================================
// Test 7: Empty Chain, Zero Agent IDs, Builder Cycle Guard & cap.a2a Facade
// ============================================================================
test("trustChain: boundary guards (empty chain, zero IDs, builder cycle) & cap.a2a facade", () => {
    const empty_links: [0]TrustDelegationLink = [];
    const no_revoked: [0]u64 = [];
    expect_eq(verifyTrustChain(empty_links[0..], 0, 0, 100, no_revoked[0..]).status, TrustVerifyStatus.empty_chain);

    const zero_id: [1]TrustDelegationLink = [
        TrustDelegationLink{ issuer_agent_id: 0, subject_agent_id: 2, capability_mask: 1, max_cost_micro_usd: 10, not_before_ms: 0, expires_ms: 1000, max_further_hops: 1 },
    ];
    expect_eq(verifyTrustChain(zero_id[0..], 1, 5, 100, no_revoked[0..]).status, TrustVerifyStatus.invalid_agent_id);

    // TrustChain builder rejects cycle back to root issuer
    const root = TrustDelegationLink{
        issuer_agent_id: 10, subject_agent_id: 20, capability_mask: 0b111,
        max_cost_micro_usd: 500, not_before_ms: 0, expires_ms: 5000, max_further_hops: 3,
    };
    let tc = cap.a2a.TrustChain.initRoot(root) ?? return;
    expect_true(tc.appendDelegation(30, 0b011, 300, 0, 4000, 2, 0));
    expect_true(!tc.appendDelegation(10, 0b001, 100, 0, 3000, 1, 0)); // cycle back to 10 rejected!
    expect_eq(tc.len(), 2);

    const res = cap.a2a.verifyTrustChain(tc.asSlice(), 0b010, 200, 1000, no_revoked[0..]);
    expect_true(res.isOk());
});
