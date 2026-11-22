// Origin: Pure-CP Stdlib (A2A Multi-Agent Capability Negotiation, Modality Intersection & SLA Matcher) | Created: 2026-11-22
// Zero-heap-allocation Agent-to-Agent (A2A) capability negotiation engine with modality/feature bitmask
// intersection, hard SLA constraint classification, and deterministic Top-K peer ranking.

/// Bitmask constants and helpers for A2A input/output content modalities.
pub struct A2aModalityMask {
    bits: u32 = 0;

    pub static const NONE: u32 = 0;
    pub static const TEXT: u32 = 1;
    pub static const JSON: u32 = 2;
    pub static const IMAGE: u32 = 4;
    pub static const AUDIO: u32 = 8;
    pub static const VIDEO: u32 = 16;
    pub static const FILE: u32 = 32;
    pub static const CODE: u32 = 64;
    pub static const ALL_KNOWN: u32 = 127;

    /// Constructs a sanitized modality mask, masking out unknown high bits.
    static fn fromBits(raw: u32): A2aModalityMask {
        return A2aModalityMask{
            bits: raw & A2aModalityMask.ALL_KNOWN,
        };
    }

    /// Returns `true` iff all bits in `required` are set.
    fn hasAll(required: u32): bool {
        return (_.bits & required) == required;
    }

    /// Returns the sanitized intersection with another raw modality bitmask.
    fn intersect(other: u32): u32 {
        return _.bits & other & A2aModalityMask.ALL_KNOWN;
    }
}

/// Bitmask constants and helpers for A2A protocol capability features.
pub struct A2aFeatureMask {
    bits: u32 = 0;

    pub static const NONE: u32 = 0;
    pub static const STREAMING: u32 = 1;
    pub static const PUSH_NOTIFICATIONS: u32 = 2;
    pub static const STATE_HISTORY: u32 = 4;
    pub static const BATCHING: u32 = 8;
    pub static const AUTH_MTLS: u32 = 16;
    pub static const ALL_KNOWN: u32 = 31;

    /// Constructs a sanitized feature mask, masking out unknown high bits.
    static fn fromBits(raw: u32): A2aFeatureMask {
        return A2aFeatureMask{
            bits: raw & A2aFeatureMask.ALL_KNOWN,
        };
    }

    /// Returns `true` iff all bits in `required` are set.
    fn hasAll(required: u32): bool {
        return (_.bits & required) == required;
    }

    /// Returns the sanitized intersection with another raw feature bitmask.
    fn intersect(other: u32): u32 {
        return _.bits & other & A2aFeatureMask.ALL_KNOWN;
    }
}

/// Caller's hard constraints and soft preferences for A2A peer negotiation.
/// `max_latency_ms == 0` disables the latency cap; `max_cost_micro_usd == 0` disables the cost cap.
pub struct A2aRequirement {
    required_in_modalities: u32 = 0;
    required_out_modalities: u32 = 0;
    required_features: u32 = 0;
    preferred_features: u32 = 0;
    max_latency_ms: u32 = 0;
    max_cost_micro_usd: u64 = 0;
    min_protocol_minor: u16 = 0;
}

/// Advertised capabilities, SLA telemetry, and availability of a candidate A2A peer.
/// `reliability_bps` is measured in basis points (`0..10000`, where `10000 == 100.00%`).
pub struct A2aPeerOffer {
    peer_id: u32 = 0;
    in_modalities: u32 = 0;
    out_modalities: u32 = 0;
    features: u32 = 0;
    p95_latency_ms: u32 = 0;
    cost_micro_usd: u64 = 0;
    protocol_minor: u16 = 0;
    reliability_bps: u16 = 0;
    online: bool = false;
}

/// Deterministic hard-constraint evaluation outcome for a peer offer.
pub enum NegotiationRejectReason {
    Compatible,
    Offline,
    VersionTooOld,
    MissingInputModality,
    MissingOutputModality,
    MissingFeature,
    LatencyExceeded,
    CostExceeded,
}

/// Negotiated capability intersection and composite SLA score (`0..10000`) for a compatible peer.
pub struct NegotiationMatch {
    peer_id: u32 = 0;
    agreed_in_modalities: u32 = 0;
    agreed_out_modalities: u32 = 0;
    agreed_features: u32 = 0;
    score: u32 = 0;
}

fn popcount32(x: u32): u32 {
    let v: u32 = x;
    let count: u32 = 0;
    while (v != 0) {
        count += (v & 1);
        v = v >> 1;
    }
    return count;
}

/// Evaluates whether `offer` satisfies all hard constraints in `req`.
/// Checks in deterministic priority order:
/// `Offline` -> `VersionTooOld` -> `MissingInputModality` -> `MissingOutputModality`
/// -> `MissingFeature` -> `LatencyExceeded` -> `CostExceeded` -> `Compatible`.
pub fn checkPeerCompatibility(req: A2aRequirement, offer: A2aPeerOffer): NegotiationRejectReason {
    if (!offer.online) {
        return NegotiationRejectReason.Offline;
    }
    if (offer.protocol_minor < req.min_protocol_minor) {
        return NegotiationRejectReason.VersionTooOld;
    }
    if ((offer.in_modalities & req.required_in_modalities) != req.required_in_modalities) {
        return NegotiationRejectReason.MissingInputModality;
    }
    if ((offer.out_modalities & req.required_out_modalities) != req.required_out_modalities) {
        return NegotiationRejectReason.MissingOutputModality;
    }
    if ((offer.features & req.required_features) != req.required_features) {
        return NegotiationRejectReason.MissingFeature;
    }
    if (req.max_latency_ms > 0 && offer.p95_latency_ms > req.max_latency_ms) {
        return NegotiationRejectReason.LatencyExceeded;
    }
    if (req.max_cost_micro_usd > 0 && offer.cost_micro_usd > req.max_cost_micro_usd) {
        return NegotiationRejectReason.CostExceeded;
    }
    return NegotiationRejectReason.Compatible;
}

/// Computes a deterministic integer composite score in `[0, 10000]` for a compatible peer:
/// - Reliability (`reliability_bps` clamped to `10000`): up to `4000` pts
/// - Preferred feature hit ratio: up to `2500` pts (`2500` when `preferred_features == 0`)
/// - Latency efficiency: up to `2000` pts
/// - Cost efficiency: up to `1500` pts
pub fn scoreCompatiblePeer(req: A2aRequirement, offer: A2aPeerOffer): u32 {
    // 1. Reliability score (0..4000)
    const rel_bps: u32 = if (offer.reliability_bps > 10000) 10000 else (offer.reliability_bps as u32);
    const rel_score: u32 = (rel_bps * 4000) / 10000;

    // 2. Preferred feature hit score (0..2500)
    const pref_mask: u32 = req.preferred_features & A2aFeatureMask.ALL_KNOWN;
    let feat_score: u32 = 2500;
    if (pref_mask != 0) {
        const pref_total: u32 = popcount32(pref_mask);
        const pref_hits: u32 = popcount32(offer.features & pref_mask);
        if (pref_total > 0) {
            feat_score = (pref_hits * 2500) / pref_total;
        }
    }

    // 3. Latency score (0..2000)
    let lat_score: u32 = 0;
    if (req.max_latency_ms > 0) {
        if (offer.p95_latency_ms <= req.max_latency_ms) {
            const lat_headroom: u64 = (req.max_latency_ms - offer.p95_latency_ms) as u64;
            lat_score = ((lat_headroom * 2000) / (req.max_latency_ms as u64)) as u32;
        }
    } else {
        const denom: u64 = (offer.p95_latency_ms as u64) + 1000;
        const lat_num: u64 = 2000000;
        lat_score = (lat_num / denom) as u32;
    }

    // 4. Cost score (0..1500), overflow-safe for large u64 values
    let cost_score: u32 = 0;
    if (req.max_cost_micro_usd > 0) {
        if (offer.cost_micro_usd <= req.max_cost_micro_usd) {
            const diff: u64 = req.max_cost_micro_usd - offer.cost_micro_usd;
            // Scale down if diff * 1500 could overflow u64
            if (diff > 10000000000000000) {
                const scaled_diff: u64 = diff >> 12;
                const scaled_max: u64 = req.max_cost_micro_usd >> 12;
                if (scaled_max > 0) {
                    cost_score = ((scaled_diff * 1500) / scaled_max) as u32;
                }
            } else {
                cost_score = ((diff * 1500) / req.max_cost_micro_usd) as u32;
            }
        }
    } else {
        if (offer.cost_micro_usd <= 1000000000000) {
            const denom_cost: u64 = offer.cost_micro_usd + 10000;
            const cost_num: u64 = 15000000;
            cost_score = (cost_num / denom_cost) as u32;
        }
    }

    return rel_score + feat_score + lat_score + cost_score;
}

fn isBetterMatch(a: NegotiationMatch, b: NegotiationMatch): bool {
    if (a.score != b.score) {
        return a.score > b.score;
    }
    return a.peer_id < b.peer_id;
}

/// Filters `offers` against `req`, computes agreed modality/feature bitmask intersections,
/// scores each compatible peer, and writes up to `out_matches.len` top-ranked matches in
/// descending score order (tie-broken by smaller `peer_id`).
/// Zero heap allocations; safely handles empty `offers` or zero-length `out_matches`.
pub fn negotiateBestPeers(
    req: A2aRequirement,
    offers: []const A2aPeerOffer,
    out_matches: *[]NegotiationMatch,
): usize {
    if (offers.len == 0 || out_matches.len == 0) return 0;

    let written: usize = 0;
    const desired_features: u32 = (req.required_features | req.preferred_features) & A2aFeatureMask.ALL_KNOWN;

    let i: usize = 0;
    while (i < offers.len) : (i += 1) {
        const offer = offers[i];
        if (checkPeerCompatibility(req, offer) != NegotiationRejectReason.Compatible) {
            continue;
        }

        const agreed_in: u32 = if (req.required_in_modalities != 0)
            (offer.in_modalities & req.required_in_modalities & A2aModalityMask.ALL_KNOWN)
        else
            (offer.in_modalities & A2aModalityMask.ALL_KNOWN);

        const agreed_out: u32 = if (req.required_out_modalities != 0)
            (offer.out_modalities & req.required_out_modalities & A2aModalityMask.ALL_KNOWN)
        else
            (offer.out_modalities & A2aModalityMask.ALL_KNOWN);

        const agreed_feat: u32 = if (desired_features != 0)
            (offer.features & desired_features)
        else
            (offer.features & A2aFeatureMask.ALL_KNOWN);

        const m = NegotiationMatch{
            peer_id: offer.peer_id,
            agreed_in_modalities: agreed_in,
            agreed_out_modalities: agreed_out,
            agreed_features: agreed_feat,
            score: scoreCompatiblePeer(req, offer),
        };

        // Maintain sorted Top-K array in descending order
        if (written < out_matches.len) {
            let pos: usize = written;
            while (pos > 0 && isBetterMatch(m, out_matches[pos - 1])) {
                out_matches[pos] = out_matches[pos - 1];
                pos -= 1;
            }
            out_matches[pos] = m;
            written += 1;
        } else if (isBetterMatch(m, out_matches[written - 1])) {
            let pos: usize = written - 1;
            while (pos > 0 && isBetterMatch(m, out_matches[pos - 1])) {
                out_matches[pos] = out_matches[pos - 1];
                pos -= 1;
            }
            out_matches[pos] = m;
        }
    }

    return written;
}
