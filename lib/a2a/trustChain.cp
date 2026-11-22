// Origin: Pure-CP Stdlib (A2A Zero-Trust Capability-Attenuated Delegation Trust Chain) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation multi-agent delegation trust chain verifier:
//   - Strict monotonic attenuation across hops: capability bitmask subset, cost ceiling, time window, hop budget
//   - Cycle detection (self-signed & multi-hop loops), broken subject-issuer chain detection, revocation list & epoch
//   - End-to-end effective privilege computation and task admission verification

/// Maximum delegation chain length supported by `TrustChain` and `verifyTrustChain`.
pub const TRUST_CHAIN_MAX_HOPS: usize = 16;

// ============================================================================
// Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Detailed verification verdict returned by `verifyTrustChain`.
pub enum TrustVerifyStatus: u8 {
    ok = 0,
    empty_chain = 1,
    privilege_escalation = 2,
    hop_limit_exceeded = 3,
    cycle_detected = 4,
    expired = 5,
    not_yet_valid = 6,
    revoked = 7,
    cost_exceeded = 8,
    missing_capability = 9,
    broken_subject_link = 10,
    invalid_time_window = 11,
    invalid_agent_id = 12,
}

/// Single signed/attested delegation hop from `issuer_agent_id` to `subject_agent_id`.
pub struct TrustDelegationLink {
    issuer_agent_id: u64 = 0;
    subject_agent_id: u64 = 0;
    capability_mask: u64 = 0;
    max_cost_micro_usd: u64 = 0;
    not_before_ms: u64 = 0;
    expires_ms: u64 = 0;
    max_further_hops: u8 = 0;
    revocation_epoch: u32 = 0;
}

/// Verification outcome and computed effective leaf privileges.
pub struct TrustChainResult {
    status: TrustVerifyStatus = TrustVerifyStatus.empty_chain;
    failed_hop_index: usize = 0;
    effective_capabilities: u64 = 0;
    effective_max_cost_micro_usd: u64 = 0;
    effective_not_before_ms: u64 = 0;
    effective_expires_ms: u64 = 0;
    remaining_further_hops: u8 = 0;
    root_issuer_id: u64 = 0;
    leaf_subject_id: u64 = 0;
    hop_count: usize = 0;

    /// Returns `true` iff the entire delegation chain and task request passed verification.
    fn isOk(): bool {
        return _.status == TrustVerifyStatus.ok;
    }
}

// ============================================================================
// Internal Helpers
// ============================================================================

fn containsU64(slice: []const u64, target: u64): bool {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (slice[i] == target) {
            return true;
        }
    }
    return false;
}

fn makeFailResult(status: TrustVerifyStatus, failed_hop: usize, hop_count: usize): TrustChainResult {
    return TrustChainResult{
        status: status,
        failed_hop_index: failed_hop,
        effective_capabilities: 0,
        effective_max_cost_micro_usd: 0,
        effective_not_before_ms: 0,
        effective_expires_ms: 0,
        remaining_further_hops: 0,
        root_issuer_id: 0,
        leaf_subject_id: 0,
        hop_count: hop_count,
    };
}

// ============================================================================
// 1. Monotonic Attenuation Delegation Constructor
// ============================================================================

/// Creates a strictly attenuated child delegation link from `parent` to `subject_agent_id`.
/// Enforces all monotonic attenuation invariants:
/// - Non-zero IDs and no immediate self-loop / 2-hop reversal (`subject != parent.subject && subject != parent.issuer`)
/// - `parent.max_further_hops > 0` and `max_further_hops < parent.max_further_hops`
/// - `(capability_mask & ~parent.capability_mask) == 0` (strict subset, no privilege escalation)
/// - `max_cost_micro_usd <= parent.max_cost_micro_usd`
/// - `not_before_ms >= parent.not_before_ms`, `expires_ms <= parent.expires_ms`, `not_before_ms <= expires_ms`
/// Returns `null` if any attenuation rule is violated.
pub fn delegateLink(
    parent: TrustDelegationLink,
    subject_agent_id: u64,
    capability_mask: u64,
    max_cost_micro_usd: u64,
    not_before_ms: u64,
    expires_ms: u64,
    max_further_hops: u8,
    revocation_epoch: u32,
): TrustDelegationLink? {
    if (parent.issuer_agent_id == 0 || parent.subject_agent_id == 0 || subject_agent_id == 0) {
        return null;
    }
    if (subject_agent_id == parent.subject_agent_id || subject_agent_id == parent.issuer_agent_id) {
        return null;
    }
    if (parent.max_further_hops == 0 || max_further_hops >= parent.max_further_hops) {
        return null;
    }
    if ((capability_mask & (~parent.capability_mask)) != 0) {
        return null;
    }
    if (max_cost_micro_usd > parent.max_cost_micro_usd) {
        return null;
    }
    if (not_before_ms < parent.not_before_ms || expires_ms > parent.expires_ms || not_before_ms > expires_ms) {
        return null;
    }

    return TrustDelegationLink{
        issuer_agent_id: parent.subject_agent_id,
        subject_agent_id: subject_agent_id,
        capability_mask: capability_mask,
        max_cost_micro_usd: max_cost_micro_usd,
        not_before_ms: not_before_ms,
        expires_ms: expires_ms,
        max_further_hops: max_further_hops,
        revocation_epoch: revocation_epoch,
    };
}

// ============================================================================
// 2. End-to-End Trust Chain Verifier
// ============================================================================

/// Verifies an A2A delegation trust chain with revocation list and minimum revocation epoch,
/// enforcing monotonic attenuation, cycle-freedom, unbroken issuer->subject linkage,
/// time validity, required capability coverage, and cost budget ceiling.
pub fn verifyTrustChainWithEpoch(
    chain: []const TrustDelegationLink,
    required_caps: u64,
    est_cost: u64,
    now_ms: u64,
    revoked_ids: []const u64,
    min_revocation_epoch: u32,
): TrustChainResult {
    if (chain.len == 0) {
        return makeFailResult(TrustVerifyStatus.empty_chain, 0, 0);
    }
    if (chain.len > TRUST_CHAIN_MAX_HOPS) {
        return makeFailResult(TrustVerifyStatus.hop_limit_exceeded, TRUST_CHAIN_MAX_HOPS, chain.len);
    }

    let seen_agents: [TRUST_CHAIN_MAX_HOPS + 1]u64 = [0; TRUST_CHAIN_MAX_HOPS + 1];
    let seen_len: usize = 0;

    let i: usize = 0;
    while (i < chain.len) : (i += 1) {
        const cur = chain[i];

        if (cur.issuer_agent_id == 0 || cur.subject_agent_id == 0) {
            return makeFailResult(TrustVerifyStatus.invalid_agent_id, i, chain.len);
        }
        if (cur.issuer_agent_id == cur.subject_agent_id) {
            return makeFailResult(TrustVerifyStatus.cycle_detected, i, chain.len);
        }

        if (i == 0) {
            seen_agents[0] = cur.issuer_agent_id;
            seen_agents[1] = cur.subject_agent_id;
            seen_len = 2;
        } else {
            const prev = chain[i - 1];

            // Unbroken subject -> issuer linkage
            if (cur.issuer_agent_id != prev.subject_agent_id) {
                return makeFailResult(TrustVerifyStatus.broken_subject_link, i, chain.len);
            }

            // Cycle detection across all prior participants in the chain
            if (containsU64(seen_agents[0..seen_len], cur.subject_agent_id)) {
                return makeFailResult(TrustVerifyStatus.cycle_detected, i, chain.len);
            }
            seen_agents[seen_len] = cur.subject_agent_id;
            seen_len += 1;

            // Strict hop budget attenuation
            if (prev.max_further_hops == 0 || cur.max_further_hops >= prev.max_further_hops) {
                return makeFailResult(TrustVerifyStatus.hop_limit_exceeded, i, chain.len);
            }

            // Strict capability, cost, and time-window monotonic attenuation
            if ((cur.capability_mask & (~prev.capability_mask)) != 0) {
                return makeFailResult(TrustVerifyStatus.privilege_escalation, i, chain.len);
            }
            if (cur.max_cost_micro_usd > prev.max_cost_micro_usd) {
                return makeFailResult(TrustVerifyStatus.privilege_escalation, i, chain.len);
            }
            if (cur.not_before_ms < prev.not_before_ms || cur.expires_ms > prev.expires_ms) {
                return makeFailResult(TrustVerifyStatus.privilege_escalation, i, chain.len);
            }
        }

        if (cur.not_before_ms > cur.expires_ms) {
            return makeFailResult(TrustVerifyStatus.invalid_time_window, i, chain.len);
        }

        // Revocation checks (agent IDs & epoch)
        if (cur.revocation_epoch < min_revocation_epoch ||
            containsU64(revoked_ids, cur.issuer_agent_id) ||
            containsU64(revoked_ids, cur.subject_agent_id))
        {
            return makeFailResult(TrustVerifyStatus.revoked, i, chain.len);
        }

        // Active time window check
        if (now_ms < cur.not_before_ms) {
            return makeFailResult(TrustVerifyStatus.not_yet_valid, i, chain.len);
        }
        if (now_ms > cur.expires_ms) {
            return makeFailResult(TrustVerifyStatus.expired, i, chain.len);
        }
    }

    const first = chain[0];
    const leaf = chain[chain.len - 1];

    // Effective privileges equal leaf privileges (guaranteed by strict monotonic attenuation)
    if ((required_caps & (~leaf.capability_mask)) != 0) {
        return TrustChainResult{
            status: TrustVerifyStatus.missing_capability,
            failed_hop_index: chain.len - 1,
            effective_capabilities: leaf.capability_mask,
            effective_max_cost_micro_usd: leaf.max_cost_micro_usd,
            effective_not_before_ms: leaf.not_before_ms,
            effective_expires_ms: leaf.expires_ms,
            remaining_further_hops: leaf.max_further_hops,
            root_issuer_id: first.issuer_agent_id,
            leaf_subject_id: leaf.subject_agent_id,
            hop_count: chain.len,
        };
    }

    if (est_cost > leaf.max_cost_micro_usd) {
        return TrustChainResult{
            status: TrustVerifyStatus.cost_exceeded,
            failed_hop_index: chain.len - 1,
            effective_capabilities: leaf.capability_mask,
            effective_max_cost_micro_usd: leaf.max_cost_micro_usd,
            effective_not_before_ms: leaf.not_before_ms,
            effective_expires_ms: leaf.expires_ms,
            remaining_further_hops: leaf.max_further_hops,
            root_issuer_id: first.issuer_agent_id,
            leaf_subject_id: leaf.subject_agent_id,
            hop_count: chain.len,
        };
    }

    return TrustChainResult{
        status: TrustVerifyStatus.ok,
        failed_hop_index: 0,
        effective_capabilities: leaf.capability_mask,
        effective_max_cost_micro_usd: leaf.max_cost_micro_usd,
        effective_not_before_ms: leaf.not_before_ms,
        effective_expires_ms: leaf.expires_ms,
        remaining_further_hops: leaf.max_further_hops,
        root_issuer_id: first.issuer_agent_id,
        leaf_subject_id: leaf.subject_agent_id,
        hop_count: chain.len,
    };
}

/// Standard entry point verifying an A2A delegation trust chain against `required_caps`,
/// `est_cost`, `now_ms`, and `revoked_ids` (no minimum epoch floor).
pub fn verifyTrustChain(
    chain: []const TrustDelegationLink,
    required_caps: u64,
    est_cost: u64,
    now_ms: u64,
    revoked_ids: []const u64,
): TrustChainResult {
    return verifyTrustChainWithEpoch(chain, required_caps, est_cost, now_ms, revoked_ids, 0);
}

// ============================================================================
// 3. Fixed-Capacity Stack TrustChain Builder
// ============================================================================

/// Zero-heap-allocation fixed-capacity builder & container for up to `TRUST_CHAIN_MAX_HOPS` links.
pub struct TrustChain {
    links: [TRUST_CHAIN_MAX_HOPS]TrustDelegationLink = [TrustDelegationLink{}; TRUST_CHAIN_MAX_HOPS];
    count: usize = 0;

    /// Initializes a 1-hop trust chain from a validated root delegation link.
    /// Returns `null` on zero agent IDs, self-loop, or inverted time window.
    static fn initRoot(root: TrustDelegationLink): TrustChain? {
        if (root.issuer_agent_id == 0 || root.subject_agent_id == 0) {
            return null;
        }
        if (root.issuer_agent_id == root.subject_agent_id) {
            return null;
        }
        if (root.not_before_ms > root.expires_ms) {
            return null;
        }
        let tc = TrustChain{};
        tc.links[0] = root;
        tc.count = 1;
        return tc;
    }

    /// Appends a strictly attenuated delegation hop from the current leaf agent to `subject_agent_id`.
    /// Rejects capacity overflow, cycles across all prior agents, and any attenuation violation.
    fn appendDelegation(
        *_,
        subject_agent_id: u64,
        capability_mask: u64,
        max_cost_micro_usd: u64,
        not_before_ms: u64,
        expires_ms: u64,
        max_further_hops: u8,
        revocation_epoch: u32,
    ): bool {
        if (_.count == 0 || _.count >= TRUST_CHAIN_MAX_HOPS) {
            return false;
        }
        // Check cycle against all existing participants in the chain
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.links[i].issuer_agent_id == subject_agent_id || _.links[i].subject_agent_id == subject_agent_id) {
                return false;
            }
        }
        const parent = _.links[_.count - 1];
        const child = delegateLink(
            parent,
            subject_agent_id,
            capability_mask,
            max_cost_micro_usd,
            not_before_ms,
            expires_ms,
            max_further_hops,
            revocation_epoch,
        ) ?? return false;
        _.links[_.count] = child;
        _.count += 1;
        return true;
    }

    /// Returns a read-only slice over the active delegation links.
    fn asSlice(): []const TrustDelegationLink {
        return _.links[0.._.count];
    }

    /// Returns the number of hops in this chain.
    fn len(): usize {
        return _.count;
    }

    /// Verifies this chain against task requirements and revocation list.
    fn verify(required_caps: u64, est_cost: u64, now_ms: u64, revoked_ids: []const u64): TrustChainResult {
        return verifyTrustChain(_.links[0.._.count], required_caps, est_cost, now_ms, revoked_ids);
    }
}
