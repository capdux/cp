// Origin: Pure-CP Stdlib Test Suite (A2A Multi-Agent Capability Negotiation, Modality Intersection & SLA Matcher) | Created: 2026-11-22
// Deep Production Verification Suite for lib/a2a/negotiation.cp
// Covers:
//   1. Exact classification & priority of all 7 hard-constraint rejection reasons vs Compatible
//   2. Modality & feature bitmask intersection, high-bit sanitization & unconstrained passthrough
//   3. Multi-peer composite SLA scoring, descending Top-K ranking & deterministic tie-breaking
//   4. Offline/stale-version filtering, unbounded SLA (0 caps) & large u64 cost overflow safety
//   5. Empty offer list, zero-length output buffer truncation safety & reliability_bps clamping

import { test, expect_eq } from "cap:test";
import {
    A2aModalityMask,
    A2aFeatureMask,
    A2aRequirement,
    A2aPeerOffer,
    NegotiationRejectReason,
    NegotiationMatch,
    checkPeerCompatibility,
    scoreCompatiblePeer,
    negotiateBestPeers,
} from "../../lib/a2a/index.cp";

fn makeBaselineOffer(): A2aPeerOffer {
    return A2aPeerOffer{
        peer_id: 1,
        in_modalities: A2aModalityMask.TEXT | A2aModalityMask.JSON | A2aModalityMask.CODE,
        out_modalities: A2aModalityMask.TEXT | A2aModalityMask.JSON,
        features: A2aFeatureMask.STREAMING | A2aFeatureMask.STATE_HISTORY | A2aFeatureMask.AUTH_MTLS,
        p95_latency_ms: 250,
        cost_micro_usd: 12000,
        protocol_minor: 3,
        reliability_bps: 9950,
        online: true,
    };
}

fn makeBaselineReq(): A2aRequirement {
    return A2aRequirement{
        required_in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE,
        required_out_modalities: A2aModalityMask.JSON,
        required_features: A2aFeatureMask.STREAMING,
        preferred_features: A2aFeatureMask.STATE_HISTORY | A2aFeatureMask.AUTH_MTLS,
        max_latency_ms: 500,
        max_cost_micro_usd: 20000,
        min_protocol_minor: 2,
    };
}

fn testHardConstraintRejectClassification()! {
    const req = makeBaselineReq();

    // 1. Baseline offer is fully compatible
    const ok_offer = makeBaselineOffer();
    expect_eq(checkPeerCompatibility(req, ok_offer), NegotiationRejectReason.Compatible);

    // 2. Offline takes highest priority even when other constraints also fail
    let offline = makeBaselineOffer();
    offline.online = false;
    offline.protocol_minor = 0;
    offline.p95_latency_ms = 99999;
    expect_eq(checkPeerCompatibility(req, offline), NegotiationRejectReason.Offline);

    // 3. VersionTooOld
    let old_ver = makeBaselineOffer();
    old_ver.protocol_minor = 1;
    expect_eq(checkPeerCompatibility(req, old_ver), NegotiationRejectReason.VersionTooOld);

    // 4. MissingInputModality (missing CODE)
    let miss_in = makeBaselineOffer();
    miss_in.in_modalities = A2aModalityMask.TEXT | A2aModalityMask.IMAGE;
    expect_eq(checkPeerCompatibility(req, miss_in), NegotiationRejectReason.MissingInputModality);

    // 5. MissingOutputModality (missing JSON)
    let miss_out = makeBaselineOffer();
    miss_out.out_modalities = A2aModalityMask.TEXT;
    expect_eq(checkPeerCompatibility(req, miss_out), NegotiationRejectReason.MissingOutputModality);

    // 6. MissingFeature (missing STREAMING)
    let miss_feat = makeBaselineOffer();
    miss_feat.features = A2aFeatureMask.STATE_HISTORY | A2aFeatureMask.AUTH_MTLS;
    expect_eq(checkPeerCompatibility(req, miss_feat), NegotiationRejectReason.MissingFeature);

    // 7. LatencyExceeded (501 > 500)
    let slow = makeBaselineOffer();
    slow.p95_latency_ms = 501;
    expect_eq(checkPeerCompatibility(req, slow), NegotiationRejectReason.LatencyExceeded);

    // 8. CostExceeded (20001 > 20000)
    let pricey = makeBaselineOffer();
    pricey.cost_micro_usd = 20001;
    expect_eq(checkPeerCompatibility(req, pricey), NegotiationRejectReason.CostExceeded);

    // 9. Exact boundary values (latency == max, cost == max, minor == min) are Compatible
    let edge = makeBaselineOffer();
    edge.protocol_minor = 2;
    edge.p95_latency_ms = 500;
    edge.cost_micro_usd = 20000;
    expect_eq(checkPeerCompatibility(req, edge), NegotiationRejectReason.Compatible);
}

fn testModalityAndFeatureBitmaskIntersection()! {
    // Sanitization helper checks unknown high bits are masked off
    const mod_mask = A2aModalityMask.fromBits(A2aModalityMask.TEXT | A2aModalityMask.AUDIO | 0xFFFF0000);
    expect_eq(mod_mask.bits, A2aModalityMask.TEXT | A2aModalityMask.AUDIO);
    expect_eq(mod_mask.hasAll(A2aModalityMask.TEXT), true);
    expect_eq(mod_mask.hasAll(A2aModalityMask.IMAGE), false);
    expect_eq(mod_mask.intersect(A2aModalityMask.AUDIO | A2aModalityMask.VIDEO), A2aModalityMask.AUDIO);

    const feat_mask = A2aFeatureMask.fromBits(A2aFeatureMask.STREAMING | A2aFeatureMask.BATCHING | 0xFF00);
    expect_eq(feat_mask.bits, A2aFeatureMask.STREAMING | A2aFeatureMask.BATCHING);
    expect_eq(feat_mask.hasAll(A2aFeatureMask.STREAMING), true);
    expect_eq(feat_mask.intersect(A2aFeatureMask.BATCHING | A2aFeatureMask.AUTH_MTLS), A2aFeatureMask.BATCHING);

    // Negotiated match computes exact intersections with req's required/preferred masks
    const req = A2aRequirement{
        required_in_modalities: A2aModalityMask.TEXT | A2aModalityMask.IMAGE,
        required_out_modalities: A2aModalityMask.JSON | A2aModalityMask.FILE,
        required_features: A2aFeatureMask.STREAMING,
        preferred_features: A2aFeatureMask.PUSH_NOTIFICATIONS | A2aFeatureMask.BATCHING,
        max_latency_ms: 1000,
        max_cost_micro_usd: 100000,
        min_protocol_minor: 1,
    };

    const offers: [1]A2aPeerOffer = [
        A2aPeerOffer{
            peer_id: 77,
            in_modalities: A2aModalityMask.TEXT | A2aModalityMask.IMAGE | A2aModalityMask.VIDEO | 0x8000,
            out_modalities: A2aModalityMask.JSON | A2aModalityMask.FILE | A2aModalityMask.AUDIO,
            features: A2aFeatureMask.STREAMING | A2aFeatureMask.BATCHING | A2aFeatureMask.AUTH_MTLS | 0x4000,
            p95_latency_ms: 200,
            cost_micro_usd: 10000,
            protocol_minor: 2,
            reliability_bps: 9900,
            online: true,
        },
    ];

    let out_buf: [2]NegotiationMatch = [NegotiationMatch{}; 2];
    let out: []NegotiationMatch = out_buf[0..];
    const count = negotiateBestPeers(req, offers[0..], out);
    expect_eq(count, 1);
    expect_eq(out[0].peer_id, 77);
    expect_eq(out[0].agreed_in_modalities, A2aModalityMask.TEXT | A2aModalityMask.IMAGE);
    expect_eq(out[0].agreed_out_modalities, A2aModalityMask.JSON | A2aModalityMask.FILE);
    // AUTH_MTLS was neither required nor preferred, so agreed_features = STREAMING | BATCHING
    expect_eq(out[0].agreed_features, A2aFeatureMask.STREAMING | A2aFeatureMask.BATCHING);

    // When req masks are 0 (wildcard), agreed masks pass through sanitized offer capabilities
    const wildcard_req = A2aRequirement{};
    const w_count = negotiateBestPeers(wildcard_req, offers[0..], out);
    expect_eq(w_count, 1);
    expect_eq(out[0].agreed_in_modalities, A2aModalityMask.TEXT | A2aModalityMask.IMAGE | A2aModalityMask.VIDEO);
    expect_eq(out[0].agreed_features, A2aFeatureMask.STREAMING | A2aFeatureMask.BATCHING | A2aFeatureMask.AUTH_MTLS);
}

fn testMultiPeerTopKScoringAndRanking()! {
    const req = makeBaselineReq();

    // 5 compatible peers with distinct SLA profiles + 2 identical peers for tie-breaking
    const offers: [6]A2aPeerOffer = [
        // peer 10: mediocre reliability, 0 preferred features, high latency & cost
        A2aPeerOffer{
            peer_id: 10,
            in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE,
            out_modalities: A2aModalityMask.JSON,
            features: A2aFeatureMask.STREAMING,
            p95_latency_ms: 480,
            cost_micro_usd: 19000,
            protocol_minor: 3,
            reliability_bps: 9000,
            online: true,
        },
        // peer 20: top tier — 100% reliability, both preferred features, low latency & cost
        A2aPeerOffer{
            peer_id: 20,
            in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE,
            out_modalities: A2aModalityMask.JSON,
            features: A2aFeatureMask.STREAMING | A2aFeatureMask.STATE_HISTORY | A2aFeatureMask.AUTH_MTLS,
            p95_latency_ms: 50,
            cost_micro_usd: 2000,
            protocol_minor: 3,
            reliability_bps: 10000,
            online: true,
        },
        // peer 30: strong runner-up — 1 preferred feature, fast & cheap
        A2aPeerOffer{
            peer_id: 30,
            in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE,
            out_modalities: A2aModalityMask.JSON,
            features: A2aFeatureMask.STREAMING | A2aFeatureMask.STATE_HISTORY,
            p95_latency_ms: 100,
            cost_micro_usd: 4000,
            protocol_minor: 3,
            reliability_bps: 9900,
            online: true,
        },
        // peer 50 & peer 40: identical SLA stats -> peer 40 must beat peer 50 via peer_id tie-break
        A2aPeerOffer{
            peer_id: 50,
            in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE,
            out_modalities: A2aModalityMask.JSON,
            features: A2aFeatureMask.STREAMING | A2aFeatureMask.AUTH_MTLS,
            p95_latency_ms: 200,
            cost_micro_usd: 8000,
            protocol_minor: 2,
            reliability_bps: 9800,
            online: true,
        },
        A2aPeerOffer{
            peer_id: 40,
            in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE,
            out_modalities: A2aModalityMask.JSON,
            features: A2aFeatureMask.STREAMING | A2aFeatureMask.AUTH_MTLS,
            p95_latency_ms: 200,
            cost_micro_usd: 8000,
            protocol_minor: 2,
            reliability_bps: 9800,
            online: true,
        },
        // peer 60: slightly better than 40/50 (both preferred features, moderate latency)
        A2aPeerOffer{
            peer_id: 60,
            in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE,
            out_modalities: A2aModalityMask.JSON,
            features: A2aFeatureMask.STREAMING | A2aFeatureMask.STATE_HISTORY | A2aFeatureMask.AUTH_MTLS,
            p95_latency_ms: 250,
            cost_micro_usd: 10000,
            protocol_minor: 3,
            reliability_bps: 9800,
            online: true,
        },
    ];

    // Request Top-4 out of 6 compatible peers
    let top4_buf: [4]NegotiationMatch = [NegotiationMatch{}; 4];
    let top4: []NegotiationMatch = top4_buf[0..];
    const written = negotiateBestPeers(req, offers[0..], top4);
    expect_eq(written, 4);

    // Best must be peer 20, strictly descending or tied with ascending peer_id
    expect_eq(top4[0].peer_id, 20);
    expect_eq(top4[0].score > top4[1].score, true);
    expect_eq(top4[1].score >= top4[2].score, true);
    expect_eq(top4[2].score >= top4[3].score, true);

    // Verify full ranking of all 6 peers preserves tie-break: peer 40 precedes peer 50
    let all_buf: [6]NegotiationMatch = [NegotiationMatch{}; 6];
    let all: []NegotiationMatch = all_buf[0..];
    const all_written = negotiateBestPeers(req, offers[0..], all);
    expect_eq(all_written, 6);
    expect_eq(all[0].peer_id, 20);
    expect_eq(all[5].peer_id, 10);

    // Locate 40 and 50 in full sorted list
    let idx40: usize = 99;
    let idx50: usize = 99;
    let k: usize = 0;
    while (k < all_written) : (k += 1) {
        if (all[k].peer_id == 40) idx40 = k;
        if (all[k].peer_id == 50) idx50 = k;
    }
    expect_eq(idx40 < idx50, true);
    expect_eq(all[idx40].score, all[idx50].score);
}

fn testOfflineVersionFilteringAndUnboundedSla()! {
    const req = A2aRequirement{
        required_in_modalities: A2aModalityMask.TEXT,
        required_out_modalities: A2aModalityMask.TEXT,
        required_features: A2aFeatureMask.NONE,
        preferred_features: A2aFeatureMask.NONE,
        max_latency_ms: 0,      // Unbounded latency
        max_cost_micro_usd: 0,  // Unbounded cost
        min_protocol_minor: 3,
    };

    const offers: [4]A2aPeerOffer = [
        // Offline -> filtered
        A2aPeerOffer{
            peer_id: 1,
            in_modalities: A2aModalityMask.TEXT,
            out_modalities: A2aModalityMask.TEXT,
            features: A2aFeatureMask.ALL_KNOWN,
            p95_latency_ms: 10,
            cost_micro_usd: 100,
            protocol_minor: 5,
            reliability_bps: 10000,
            online: false,
        },
        // Stale protocol minor -> filtered
        A2aPeerOffer{
            peer_id: 2,
            in_modalities: A2aModalityMask.TEXT,
            out_modalities: A2aModalityMask.TEXT,
            features: A2aFeatureMask.ALL_KNOWN,
            p95_latency_ms: 10,
            cost_micro_usd: 100,
            protocol_minor: 2,
            reliability_bps: 10000,
            online: true,
        },
        // Compatible, slower & pricier
        A2aPeerOffer{
            peer_id: 3,
            in_modalities: A2aModalityMask.TEXT,
            out_modalities: A2aModalityMask.TEXT,
            features: A2aFeatureMask.NONE,
            p95_latency_ms: 3000,
            cost_micro_usd: 500000,
            protocol_minor: 3,
            reliability_bps: 9500,
            online: true,
        },
        // Compatible, faster & cheaper
        A2aPeerOffer{
            peer_id: 4,
            in_modalities: A2aModalityMask.TEXT,
            out_modalities: A2aModalityMask.TEXT,
            features: A2aFeatureMask.NONE,
            p95_latency_ms: 100,
            cost_micro_usd: 1000,
            protocol_minor: 4,
            reliability_bps: 9500,
            online: true,
        },
    ];

    let out_buf: [4]NegotiationMatch = [NegotiationMatch{}; 4];
    let out: []NegotiationMatch = out_buf[0..];
    const n = negotiateBestPeers(req, offers[0..], out);
    expect_eq(n, 2);
    expect_eq(out[0].peer_id, 4);
    expect_eq(out[1].peer_id, 3);
    expect_eq(out[0].score > out[1].score, true);

    // Large u64 cost budget (near u64 max) must not overflow when scoring
    const huge_req = A2aRequirement{
        max_cost_micro_usd: 100000000000000000, // 1e17
    };
    const huge_offer = A2aPeerOffer{
        peer_id: 9,
        cost_micro_usd: 10000000000000000, // 1e16 (10% of cap)
        reliability_bps: 10000,
        online: true,
    };
    const huge_score = scoreCompatiblePeer(huge_req, huge_offer);
    expect_eq(huge_score > 7000, true);
    expect_eq(huge_score <= 10000, true);
}

fn testEmptyInputsTruncationSafetyAndReliabilityClamping()! {
    const req = makeBaselineReq();

    // 1. Empty offers slice returns 0
    const empty_offers: [0]A2aPeerOffer = [];
    let out_buf: [2]NegotiationMatch = [NegotiationMatch{}; 2];
    let out: []NegotiationMatch = out_buf[0..];
    expect_eq(negotiateBestPeers(req, empty_offers[0..], out), 0);

    // 2. Zero-length output buffer returns 0 safely without out-of-bounds writes
    const one_offer: [1]A2aPeerOffer = [makeBaselineOffer()];
    let zero_buf: [0]NegotiationMatch = [];
    let zero_out: []NegotiationMatch = zero_buf[0..];
    expect_eq(negotiateBestPeers(req, one_offer[0..], zero_out), 0);

    // 3. All offers incompatible returns 0
    let bad = makeBaselineOffer();
    bad.online = false;
    const all_bad: [1]A2aPeerOffer = [bad];
    expect_eq(negotiateBestPeers(req, all_bad[0..], out), 0);

    // 4. Out-of-range reliability_bps (>10000) clamps cleanly to 10000 bps (score never exceeds 10000)
    let over_rel = makeBaselineOffer();
    over_rel.reliability_bps = 65535;
    over_rel.p95_latency_ms = 0;
    over_rel.cost_micro_usd = 0;
    const perfect_score = scoreCompatiblePeer(req, over_rel);
    expect_eq(perfect_score, 10000);
}

test("a2a negotiation: exact classification and priority of all 7 hard-constraint rejections", testHardConstraintRejectClassification);
test("a2a negotiation: modality and feature bitmask intersection and sanitization", testModalityAndFeatureBitmaskIntersection);
test("a2a negotiation: multi-peer composite SLA scoring, Top-K ranking and tie-breaking", testMultiPeerTopKScoringAndRanking);
test("a2a negotiation: offline/version filtering, unbounded SLA and large u64 cost safety", testOfflineVersionFilteringAndUnboundedSla);
test("a2a negotiation: empty inputs, zero-length output truncation safety and reliability clamping", testEmptyInputsTruncationSafetyAndReliabilityClamping);
