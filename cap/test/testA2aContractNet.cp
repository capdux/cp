// Origin: Pure-CP Stdlib Test Suite (lib/a2a/contractNet.cp) | Created: 2026-11-23
// Deep production regression tests for cap:a2a FIPA Contract Net Protocol (`lib/a2a/contractNet.cp`):
//   1. Multi-objective utility scoring (cost + latency + confidence + reputation) & award completion
//   2. Strict bid admission filtering (caps, cost, latency, deadline, duplicate, non-open, invalid_arg)
//   3. Multi-hop runner-up fallback chain on winner failures without re-broadcasting CFP
//   4. Runner-up exhaustion cleanly transitions auction to `failed_no_bids`
//   5. `reapExpired()` auto-award, no-bid expiry, and execution-timeout -> immediate runner-up fallback
//   6. 16 bids/CFP & 32 concurrent CFPs capacity guards, `cancelCfp`, and `purgeTerminal()` reuse
//   7. Custom utility weights flipping winner, telemetry snapshot, `reset()`, and `cap.a2a` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CNP_MAX_AUCTIONS,
    CNP_MAX_BIDS_PER_CFP,
    CnpAuctionState,
    CnpBidSubmitStatus,
    CnpScoreWeights,
    ContractNetBroker,
} from "cap:a2a";

const CAP_CODE: u64 = 1 << 0;
const CAP_SQL: u64 = 1 << 1;
const CAP_GPU: u64 = 1 << 2;

test("contractNet: multi-objective utility scoring selects optimal winner and completes cleanly") {
    let broker = ContractNetBroker.init();
    expect_true(broker.openCfp(100, CAP_CODE | CAP_SQL, 1000, 2000, 1000, 5000));

    // Worker 1: expensive & slow, high confidence
    expect_true(broker.submitBid(100, 1, CAP_CODE | CAP_SQL, 900, 1800, 9000, 8000, 200) == CnpBidSubmitStatus.accepted);
    // Worker 2: cheap, fast, strong confidence & reputation -> clear winner
    expect_true(broker.submitBid(100, 2, CAP_CODE | CAP_SQL | CAP_GPU, 200, 400, 9200, 9000, 300) == CnpBidSubmitStatus.accepted);
    // Worker 3: mid cost & latency
    expect_true(broker.submitBid(100, 3, CAP_CODE | CAP_SQL, 500, 1000, 8000, 8000, 400) == CnpBidSubmitStatus.accepted);

    const award = broker.evaluateAndAward(100, 1000);
    expect_true(award.awarded);
    expect_eq(award.winner_worker_id, @as(u64, 2));
    expect_eq(award.rejected_count, @as(usize, 2));
    expect_eq(award.is_fallback, false);
    expect_true(award.winner_utility_score > 7000);

    // Non-winner reporting outcome is rejected; actual winner completes the CFP
    expect_eq(broker.reportExecutionOutcome(100, 1, true, 1500), false);
    expect_true(broker.reportExecutionOutcome(100, 2, true, 1500));

    if (view := broker.getAuction(100)) {
        expect_true(view.state == CnpAuctionState.completed);
        expect_eq(view.winner_worker_id, @as(u64, 2));
    } else {
        expect_true(false);
    }
    expect_eq(broker.stats().total_completed, @as(u64, 1));
}

test("contractNet: strict bid validation rejects invalid, late, under-capable, over-budget & duplicate bids") {
    let broker = ContractNetBroker.init();
    expect_true(broker.openCfp(200, CAP_CODE | CAP_GPU, 500, 1000, 1000, 5000));

    expect_true(broker.submitBid(200, 0, CAP_CODE | CAP_GPU, 100, 100, 5000, 5000, 100) == CnpBidSubmitStatus.invalid_arg);
    expect_true(broker.submitBid(200, 1, CAP_CODE | CAP_GPU, 100, 100, 10001, 5000, 100) == CnpBidSubmitStatus.invalid_arg);
    expect_true(broker.submitBid(999, 1, CAP_CODE | CAP_GPU, 100, 100, 5000, 5000, 100) == CnpBidSubmitStatus.cfp_not_found);
    expect_true(broker.submitBid(200, 1, CAP_CODE | CAP_GPU, 100, 100, 5000, 5000, 1001) == CnpBidSubmitStatus.deadline_passed);
    expect_true(broker.submitBid(200, 1, CAP_CODE, 100, 100, 5000, 5000, 500) == CnpBidSubmitStatus.capability_mismatch);
    expect_true(broker.submitBid(200, 1, CAP_CODE | CAP_GPU, 501, 100, 5000, 5000, 500) == CnpBidSubmitStatus.cost_exceeded);
    expect_true(broker.submitBid(200, 1, CAP_CODE | CAP_GPU, 100, 1001, 5000, 5000, 500) == CnpBidSubmitStatus.latency_exceeded);

    // Valid bid accepted, duplicate from same worker rejected
    expect_true(broker.submitBid(200, 1, CAP_CODE | CAP_GPU, 200, 200, 8000, 8000, 500) == CnpBidSubmitStatus.accepted);
    expect_true(broker.submitBid(200, 1, CAP_CODE | CAP_GPU, 150, 150, 9000, 9000, 600) == CnpBidSubmitStatus.duplicate_worker);

    // After award, further bids rejected with cfp_not_open
    * = broker.evaluateAndAward(200, 1000);
    expect_true(broker.submitBid(200, 2, CAP_CODE | CAP_GPU, 100, 100, 9000, 9000, 900) == CnpBidSubmitStatus.cfp_not_open);

    const st = broker.stats();
    expect_eq(st.total_bids_accepted, @as(u64, 1));
    expect_eq(st.total_invalid_bids_rejected, @as(u64, 9));
}

test("contractNet: multi-hop runner-up fallback chain recovers without re-broadcasting CFP") {
    let broker = ContractNetBroker.init();
    expect_true(broker.openCfp(300, CAP_SQL, 1000, 1000, 1000, 3000));

    // Ranked order: worker 10 (best) > worker 20 (second) > worker 30 (third)
    expect_true(broker.submitBid(300, 10, CAP_SQL, 100, 100, 9500, 9500, 100) == CnpBidSubmitStatus.accepted);
    expect_true(broker.submitBid(300, 20, CAP_SQL, 300, 300, 8500, 8500, 100) == CnpBidSubmitStatus.accepted);
    expect_true(broker.submitBid(300, 30, CAP_SQL, 600, 600, 7500, 7500, 100) == CnpBidSubmitStatus.accepted);

    const a1 = broker.evaluateAndAward(300, 1000);
    expect_eq(a1.winner_worker_id, @as(u64, 10));

    // Winner 10 fails -> fallback promotes runner-up 20
    expect_true(broker.reportExecutionOutcome(300, 10, false, 1200));
    const fb1 = broker.fallbackToRunnerUp(300, 4000, 1250);
    expect_true(fb1.awarded);
    expect_true(fb1.is_fallback);
    expect_eq(fb1.winner_worker_id, @as(u64, 20));

    // Runner-up 20 fails -> fallback promotes worker 30
    expect_true(broker.reportExecutionOutcome(300, 20, false, 1500));
    const fb2 = broker.fallbackToRunnerUp(300, 5000, 1550);
    expect_true(fb2.awarded);
    expect_eq(fb2.winner_worker_id, @as(u64, 30));

    // Worker 30 succeeds!
    expect_true(broker.reportExecutionOutcome(300, 30, true, 1800));

    const st = broker.stats();
    expect_eq(st.total_completed, @as(u64, 1));
    expect_eq(st.total_fallback_awards, @as(u64, 2));
    expect_eq(st.total_fallback_successes, @as(u64, 1));
    if (v := broker.getAuction(300)) {
        expect_eq(v.fallback_count, @as(u32, 2));
        expect_true(v.state == CnpAuctionState.completed);
    } else {
        expect_true(false);
    }
}

test("contractNet: exhausting all runner-up candidates transitions auction to failed_no_bids") {
    let broker = ContractNetBroker.init();
    expect_true(broker.openCfp(400, CAP_CODE, 500, 500, 1000, 3000));
    * = broker.submitBid(400, 1, CAP_CODE, 100, 100, 9000, 9000, 100);

    const a = broker.evaluateAndAward(400, 1000);
    expect_eq(a.winner_worker_id, @as(u64, 1));
    expect_true(broker.reportExecutionOutcome(400, 1, false, 1500));

    // No remaining rejected bids -> fallback fails cleanly
    const fb = broker.fallbackToRunnerUp(400, 4000, 1600);
    expect_eq(fb.awarded, false);
    expect_true(fb.state == CnpAuctionState.failed_no_bids);
    expect_eq(broker.stats().total_no_bids_or_failed, @as(u64, 1));
}

test("contractNet: reapExpired handles no-bid expiry, auto-award, and exec timeout -> fallback") {
    let broker = ContractNetBroker.init();

    // CFP 501: open with 0 bids, bid_deadline=1000
    expect_true(broker.openCfp(501, CAP_CODE, 500, 500, 1000, 3000));
    // CFP 502: open with 1 bid, bid_deadline=1000 -> should auto-award
    expect_true(broker.openCfp(502, CAP_CODE, 500, 500, 1000, 3000));
    * = broker.submitBid(502, 11, CAP_CODE, 200, 200, 8000, 8000, 500);
    // CFP 503: already awarded to worker 21 (exec_deadline=1200) with runner-up 22
    expect_true(broker.openCfp(503, CAP_CODE, 500, 500, 800, 1200));
    * = broker.submitBid(503, 21, CAP_CODE, 100, 100, 9000, 9000, 500);
    * = broker.submitBid(503, 22, CAP_CODE, 300, 300, 8000, 8000, 500);
    * = broker.evaluateAndAward(503, 800);

    // Reap at t=1300: 501 fails no-bids, 502 auto-awards to 11, 503 times out worker 21!
    const rep = broker.reapExpired(1300);
    expect_eq(rep.open_expired_no_bids, @as(usize, 1));
    expect_eq(rep.open_auto_awarded, @as(usize, 1));
    expect_eq(rep.exec_timed_out, @as(usize, 1));

    // Immediately fallback CFP 503 to runner-up 22
    const fb = broker.fallbackToRunnerUp(503, 2500, 1300);
    expect_true(fb.awarded);
    expect_eq(fb.winner_worker_id, @as(u64, 22));
}

test("contractNet: 16-bid cap, 32-auction cap, cancelCfp, and purgeTerminal slot reuse") {
    let broker = ContractNetBroker.init();
    expect_true(broker.openCfp(1, CAP_CODE, 1000, 1000, 2000, 5000));

    // Submit 16 distinct workers -> 17th rejected with bids_full
    let w: u64 = 1;
    while (w <= @as(u64, CNP_MAX_BIDS_PER_CFP)) {
        expect_true(broker.submitBid(1, w, CAP_CODE, 100, 100, 8000, 8000, 500) == CnpBidSubmitStatus.accepted);
        w += 1;
    }
    expect_true(broker.submitBid(1, 999, CAP_CODE, 100, 100, 8000, 8000, 500) == CnpBidSubmitStatus.bids_full);

    // Cancel CFP 1
    expect_true(broker.cancelCfp(1));
    expect_eq(broker.cancelCfp(1), false);

    // Fill remaining 31 auction slots up to 32
    let cid: u64 = 2;
    while (cid <= @as(u64, CNP_MAX_AUCTIONS)) {
        expect_true(broker.openCfp(cid, CAP_CODE, 500, 500, 1000, 2000));
        cid += 1;
    }
    expect_eq(broker.openCfp(999, CAP_CODE, 500, 500, 1000, 2000), false);

    // Purge cancelled CFP 1 -> allows opening CFP 999
    expect_eq(broker.purgeTerminal(), @as(usize, 1));
    expect_true(broker.openCfp(999, CAP_CODE, 500, 500, 1000, 2000));
}

test("contractNet: custom weights flip winner, reset, and cap.a2a facade parity") {
    // Pure latency-focused broker
    const lat_weights = cap.a2a.CnpScoreWeights{
        cost_weight: 0,
        latency_weight: 10000,
        confidence_weight: 0,
        reputation_weight: 0,
    };
    let bad_weights = CnpScoreWeights{ cost_weight: 0, latency_weight: 0, confidence_weight: 0, reputation_weight: 0 };
    expect_true(ContractNetBroker.initWithWeights(bad_weights) == null);

    let broker = cap.a2a.ContractNetBroker.initWithWeights(lat_weights) ?? return;
    expect_true(broker.openCfp(700, CAP_SQL, 1000, 1000, 1000, 5000));
    // Worker 1: ultra cheap (50) but slower (600ms)
    * = broker.submitBid(700, 1, CAP_SQL, 50, 600, 9000, 9000, 100);
    // Worker 2: expensive (900) but ultra fast (50ms) -> wins under latency-only weights!
    * = broker.submitBid(700, 2, CAP_SQL, 900, 50, 5000, 5000, 100);

    const aw = broker.evaluateAndAward(700, 1000);
    expect_eq(aw.winner_worker_id, @as(u64, 2));

    broker.reset();
    const st = broker.stats();
    expect_eq(st.active_auctions, @as(usize, 0));
    expect_eq(st.total_cfps, @as(u64, 0));
}
