// Origin: Pure-CP Stdlib Test Suite (lib/a2a/gossipHealth.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc SWIM-Style Multi-Agent Gossip Health State Machine:
//   1. SWIM incarnation precedence & equal-incarnation convergence (`dead > suspect > alive`)
//   2. Local self-refutation on `suspect`/`dead` rumor & two-node gossip round-trip convergence
//   3. Bounded piggyback dissemination queue: least-transmitted & newest-first scheduling, deduplication & retirement
//   4. Suspicion timeout promotion (`reapSuspects`), clock-skew guard & `purgeDead` slot reclamation
//   5. Fisher-Yates shuffled round-robin `selectProbeTarget` (uniform coverage, excludes local & dead)
//   6. Dynamic capability broadcast, `listCapableAlive` bitmask filtering & 64-member capacity/invalid-id guards
//   7. Telemetry stats snapshot accuracy & `cap.a2a` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    GOSSIP_MAX_MEMBERS,
    GossipMemberState,
    gossipStateRank,
    GossipApplyOutcome,
    GossipConfig,
    GossipMember,
    GossipUpdate,
    GossipHealthMachine,
} from "cap:a2a";

test("GossipHealth: SWIM incarnation conflict resolution & equal-incarnation dead > suspect > alive") {
    let node = GossipHealthMachine.init(1, 0b001, 1000, GossipConfig{}) ?? return;

    expect_true(gossipStateRank(GossipMemberState.dead) > gossipStateRank(GossipMemberState.suspect));
    expect_true(gossipStateRank(GossipMemberState.suspect) > gossipStateRank(GossipMemberState.alive));

    // Discover peer 2 as alive@1
    expect_eq(node.applyUpdate(GossipUpdate{
        agent_id: 2,
        state: GossipMemberState.alive,
        incarnation: 1,
        capabilities: 0b010,
    }, 1100), GossipApplyOutcome.applied_new);

    // Duplicate alive@1 -> ignored_stale
    expect_eq(node.applyUpdate(GossipUpdate{
        agent_id: 2,
        state: GossipMemberState.alive,
        incarnation: 1,
        capabilities: 0b010,
    }, 1200), GossipApplyOutcome.ignored_stale);

    // Equal incarnation: suspect@1 overrides alive@1
    expect_eq(node.applyUpdate(GossipUpdate{
        agent_id: 2,
        state: GossipMemberState.suspect,
        incarnation: 1,
        capabilities: 0b010,
    }, 1300), GossipApplyOutcome.applied_updated);
    expect_eq((node.getMember(2) ?? GossipMember{}).state, GossipMemberState.suspect);

    // Equal incarnation reverse: alive@1 cannot override suspect@1
    expect_eq(node.applyUpdate(GossipUpdate{
        agent_id: 2,
        state: GossipMemberState.alive,
        incarnation: 1,
        capabilities: 0b010,
    }, 1400), GossipApplyOutcome.ignored_stale);

    // Equal incarnation: dead@1 overrides suspect@1
    expect_eq(node.applyUpdate(GossipUpdate{
        agent_id: 2,
        state: GossipMemberState.dead,
        incarnation: 1,
        capabilities: 0b010,
    }, 1500), GossipApplyOutcome.applied_updated);
    expect_eq((node.getMember(2) ?? GossipMember{}).state, GossipMemberState.dead);

    // Equal incarnation reverse: suspect@1 cannot override dead@1
    expect_eq(node.applyUpdate(GossipUpdate{
        agent_id: 2,
        state: GossipMemberState.suspect,
        incarnation: 1,
        capabilities: 0b010,
    }, 1600), GossipApplyOutcome.ignored_stale);

    // Higher incarnation alive@2 overrides dead@1
    expect_eq(node.applyUpdate(GossipUpdate{
        agent_id: 2,
        state: GossipMemberState.alive,
        incarnation: 2,
        capabilities: 0b110,
    }, 1700), GossipApplyOutcome.applied_updated);
    const m2 = node.getMember(2) ?? GossipMember{};
    expect_eq(m2.state, GossipMemberState.alive);
    expect_eq(m2.incarnation, 2 as u64);
    expect_eq(m2.capabilities, 0b110 as u64);

    // Lower incarnation dead@1 is ignored once at incarnation 2
    expect_eq(node.applyUpdate(GossipUpdate{
        agent_id: 2,
        state: GossipMemberState.dead,
        incarnation: 1,
        capabilities: 0b110,
    }, 1800), GossipApplyOutcome.ignored_stale);
}

test("GossipHealth: local self-refutation on suspect rumor & two-node round-trip convergence") {
    let node_a = GossipHealthMachine.init(10, 0x0F, 1000, GossipConfig{ retransmit_limit: 3 }) ?? return;
    let node_b = GossipHealthMachine.init(20, 0xF0, 1000, GossipConfig{ retransmit_limit: 3 }) ?? return;

    * = node_a.upsertPeer(20, 0xF0, 1000);
    * = node_b.upsertPeer(10, 0x0F, 1000);

    // Drain initial join announcements from both nodes
    let drain_buf: [8]GossipUpdate = [GossipUpdate{}; 8];
    let drain_slice: []GossipUpdate = drain_buf[0..];
    let round: usize = 0;
    while (round < 3) : (round += 1) {
        * = node_a.buildGossipBatch(8, drain_slice);
        * = node_b.buildGossipBatch(8, drain_slice);
    }

    // Node B suspects Node A (at incarnation 1) and piggybacks the rumor to Node A
    expect_true(node_b.markSuspect(10, 2000));
    expect_eq((node_b.getMember(10) ?? GossipMember{}).state, GossipMemberState.suspect);

    let wire_b_to_a: [4]GossipUpdate = [GossipUpdate{}; 4];
    let wire_b_slice: []GossipUpdate = wire_b_to_a[0..];
    const sent_b = node_b.buildGossipBatch(4, wire_b_slice);
    expect_eq(sent_b, 1 as usize);
    expect_eq(wire_b_to_a[0].agent_id, 10 as u64);
    expect_eq(wire_b_to_a[0].state, GossipMemberState.suspect);

    // Node A receives suspect@1 about itself -> auto-refutes by bumping incarnation to 2!
    const outcome_a = node_a.applyUpdate(wire_b_to_a[0], 2100);
    expect_eq(outcome_a, GossipApplyOutcome.refuted_self);
    expect_eq(node_a.localMember().state, GossipMemberState.alive);
    expect_eq(node_a.localMember().incarnation, 2 as u64);

    // Node A piggybacks alive@2 refutation back to Node B
    let wire_a_to_b: [4]GossipUpdate = [GossipUpdate{}; 4];
    let wire_a_slice: []GossipUpdate = wire_a_to_b[0..];
    const sent_a = node_a.buildGossipBatch(4, wire_a_slice);
    expect_eq(sent_a, 1 as usize);
    expect_eq(wire_a_to_b[0].agent_id, 10 as u64);
    expect_eq(wire_a_to_b[0].state, GossipMemberState.alive);
    expect_eq(wire_a_to_b[0].incarnation, 2 as u64);

    // Node B applies batch -> Node A is restored to alive@2!
    const accepted_b = node_b.applyBatch(wire_a_to_b[0..sent_a], 2200);
    expect_eq(accepted_b, 1 as usize);
    const view_a_on_b = node_b.getMember(10) ?? GossipMember{};
    expect_eq(view_a_on_b.state, GossipMemberState.alive);
    expect_eq(view_a_on_b.incarnation, 2 as u64);

    // Higher-incarnation suspect@5 against local Node A jumps straight to incarnation 6
    expect_eq(node_a.applyUpdate(GossipUpdate{
        agent_id: 10,
        state: GossipMemberState.suspect,
        incarnation: 5,
    }, 2300), GossipApplyOutcome.refuted_self);
    expect_eq(node_a.localMember().incarnation, 6 as u64);

    // Stale suspect@3 against local Node A is ignored
    expect_eq(node_a.applyUpdate(GossipUpdate{
        agent_id: 10,
        state: GossipMemberState.suspect,
        incarnation: 3,
    }, 2400), GossipApplyOutcome.ignored_stale);
    expect_eq(node_a.localMember().incarnation, 6 as u64);
}

test("GossipHealth: bounded piggyback queue priority (least tx_count, newest seq) & budget retirement") {
    let node = GossipHealthMachine.init(1, 0b1, 1000, GossipConfig{ retransmit_limit: 2 }) ?? return;

    // Drain initial self announcement (2 rounds)
    let tmp: [4]GossipUpdate = [GossipUpdate{}; 4];
    let tmp_slice: []GossipUpdate = tmp[0..];
    * = node.buildGossipBatch(4, tmp_slice);
    * = node.buildGossipBatch(4, tmp_slice);
    expect_eq(node.stats().pending_disseminations, 0 as usize);

    // Enqueue 3 distinct peer discoveries in order: 2, 3, 4 (all have tx_count == 0, seq: 4 > 3 > 2)
    * = node.upsertPeer(2, 0b1, 1100);
    * = node.upsertPeer(3, 0b1, 1110);
    * = node.upsertPeer(4, 0b1, 1120);
    expect_eq(node.stats().pending_disseminations, 3 as usize);

    // Batch 1 (max_items = 2): must pick newest with tx_count == 0 -> agent 4, then agent 3
    let out1: [2]GossipUpdate = [GossipUpdate{}; 2];
    let out1_slice: []GossipUpdate = out1[0..];
    expect_eq(node.buildGossipBatch(2, out1_slice), 2 as usize);
    expect_eq(out1[0].agent_id, 4 as u64);
    expect_eq(out1[1].agent_id, 3 as u64);

    // Batch 2 (max_items = 2): agent 2 has tx_count == 0 (highest priority!), then agent 4 (tx_count == 1, newer than 3)
    let out2: [2]GossipUpdate = [GossipUpdate{}; 2];
    let out2_slice: []GossipUpdate = out2[0..];
    expect_eq(node.buildGossipBatch(2, out2_slice), 2 as usize);
    expect_eq(out2[0].agent_id, 2 as u64);
    expect_eq(out2[1].agent_id, 4 as u64); // agent 4 now hits retransmits_left == 0 and retires!
    expect_eq(node.stats().pending_disseminations, 2 as usize);

    // Deduplication test: marking agent 3 suspect replaces its existing queue entry with fresh tx_count == 0
    expect_true(node.markSuspect(3, 1200));
    expect_eq(node.stats().pending_disseminations, 2 as usize);

    let out3: [2]GossipUpdate = [GossipUpdate{}; 2];
    let out3_slice: []GossipUpdate = out3[0..];
    expect_eq(node.buildGossipBatch(2, out3_slice), 2 as usize);
    // Agent 3 (tx_count == 0) precedes Agent 2 (tx_count == 1)
    expect_eq(out3[0].agent_id, 3 as u64);
    expect_eq(out3[0].state, GossipMemberState.suspect);
    expect_eq(out3[1].agent_id, 2 as u64); // agent 2 now retires!
    expect_eq(node.stats().pending_disseminations, 1 as usize);

    // Final send retires agent 3
    let out4: [2]GossipUpdate = [GossipUpdate{}; 2];
    let out4_slice: []GossipUpdate = out4[0..];
    expect_eq(node.buildGossipBatch(2, out4_slice), 1 as usize);
    expect_eq(out4[0].agent_id, 3 as u64);
    expect_eq(node.stats().pending_disseminations, 0 as usize);
}

test("GossipHealth: suspect timeout reapSuspects, clock-skew safety & purgeDead reclamation") {
    let node = GossipHealthMachine.init(1, 0b1, 1000, GossipConfig{
        suspect_timeout_ms: 5000,
        retransmit_limit: 2,
    }) ?? return;

    * = node.upsertPeer(2, 0b1, 1000);
    * = node.upsertPeer(3, 0b1, 1000);

    expect_true(node.markSuspect(2, 2000));
    expect_true(node.markSuspect(3, 4000));
    // Marking local or already-suspect peer returns false
    expect_true(!node.markSuspect(1, 2000));
    expect_true(!node.markSuspect(2, 2100));

    // Clock skew (now_ms < state_changed_ms) must never falsely reap
    expect_eq(node.reapSuspects(1500), 0 as usize);

    // At t = 6999, peer 2 elapsed 4999ms (< 5000ms) -> not yet dead
    expect_eq(node.reapSuspects(6999), 0 as usize);
    expect_eq((node.getMember(2) ?? GossipMember{}).state, GossipMemberState.suspect);

    // At t = 7000, peer 2 reaches 5000ms -> promoted to dead; peer 3 only 3000ms -> remains suspect
    expect_eq(node.reapSuspects(7000), 1 as usize);
    expect_eq((node.getMember(2) ?? GossipMember{}).state, GossipMemberState.dead);
    expect_eq((node.getMember(3) ?? GossipMember{}).state, GossipMemberState.suspect);

    // Direct ACK on dead peer returns false; on alive local/peer returns true
    expect_true(!node.recordDirectAck(2, 7100));
    expect_true(node.recordDirectAck(1, 7100));

    // At t = 9000, peer 3 also reaps to dead
    expect_eq(node.reapSuspects(9000), 1 as usize);
    expect_eq((node.getMember(3) ?? GossipMember{}).state, GossipMemberState.dead);

    // Purge dead peers older than 3000ms at t = 10500: peer 2 (dead since 7000, age 3500) purged; peer 3 (age 1500) kept
    expect_eq(node.purgeDead(10500, 3000), 1 as usize);
    expect_true(node.getMember(2) == null);
    expect_true(node.getMember(3) != null);
    expect_eq(node.stats().member_count, 2 as usize);
}

test("GossipHealth: Fisher-Yates shuffled round-robin selectProbeTarget uniform coverage") {
    let node = GossipHealthMachine.init(1, 0b1, 1000, GossipConfig{
        prng_seed: 0x1234_5678_9ABC_DEF0,
    }) ?? return;

    // Single-node cluster has no remote probe target
    expect_true(node.selectProbeTarget() == null);

    // Add 5 remote peers (11..15) and mark peer 13 dead
    * = node.upsertPeer(11, 0b1, 1000);
    * = node.upsertPeer(12, 0b1, 1000);
    * = node.upsertPeer(13, 0b1, 1000);
    * = node.upsertPeer(14, 0b1, 1000);
    * = node.upsertPeer(15, 0b1, 1000);
    * = node.applyUpdate(GossipUpdate{
        agent_id: 13,
        state: GossipMemberState.dead,
        incarnation: 1,
    }, 1100);

    // A full round of 4 probes must visit each of {11, 12, 14, 15} exactly once, never 1 or 13
    let seen_11: usize = 0;
    let seen_12: usize = 0;
    let seen_14: usize = 0;
    let seen_15: usize = 0;

    let step: usize = 0;
    while (step < 4) : (step += 1) {
        const target = node.selectProbeTarget() ?? 0;
        expect_true(target != 0 && target != 1 && target != 13);
        if (target == 11) seen_11 += 1;
        if (target == 12) seen_12 += 1;
        if (target == 14) seen_14 += 1;
        if (target == 15) seen_15 += 1;
    }

    expect_eq(seen_11, 1 as usize);
    expect_eq(seen_12, 1 as usize);
    expect_eq(seen_14, 1 as usize);
    expect_eq(seen_15, 1 as usize);

    // Next probe starts a fresh shuffled round and still returns a valid probeable peer
    const next_t = node.selectProbeTarget() ?? 0;
    expect_true(next_t == 11 || next_t == 12 || next_t == 14 || next_t == 15);
}

test("GossipHealth: capability bitmask broadcast, filtering & 64-member capacity/invalid-id guards") {
    // agent_id == 0 is rejected at init
    expect_true(GossipHealthMachine.init(0, 0, 1000, GossipConfig{}) == null);

    const CAP_LLM: u64 = 1 << 0;
    const CAP_SQL: u64 = 1 << 1;
    const CAP_GPU: u64 = 1 << 2;

    let node = GossipHealthMachine.init(1, CAP_LLM, 1000, GossipConfig{}) ?? return;
    expect_eq(node.applyUpdate(GossipUpdate{ agent_id: 0, state: GossipMemberState.alive, incarnation: 1 }, 1000), GossipApplyOutcome.ignored_stale);

    // Update local capabilities -> incarnation bumps from 1 to 2
    const new_inc = node.updateLocalCapabilities(CAP_LLM | CAP_GPU, 1100);
    expect_eq(new_inc, 2 as u64);
    expect_eq(node.localMember().capabilities, CAP_LLM | CAP_GPU);

    * = node.upsertPeer(2, CAP_LLM | CAP_SQL, 1100);
    * = node.upsertPeer(3, CAP_LLM | CAP_GPU, 1100);
    * = node.upsertPeer(4, CAP_LLM | CAP_GPU, 1100);
    expect_true(node.markSuspect(4, 1200)); // suspect peer must be excluded from listCapableAlive

    let ids_buf: [8]u64 = [0; 8];
    let ids_slice: []u64 = ids_buf[0..];
    const n_all = node.listCapableAlive(CAP_LLM | CAP_GPU, true, ids_slice);
    expect_eq(n_all, 2 as usize); // local(1) and peer(3)
    const n_remote = node.listCapableAlive(CAP_LLM | CAP_GPU, false, ids_slice);
    expect_eq(n_remote, 1 as usize); // only peer(3)
    expect_eq(ids_buf[0], 3 as u64);

    // Fill remaining slots up to GOSSIP_MAX_MEMBERS (64 total including local)
    let id: u64 = 5;
    while (id <= GOSSIP_MAX_MEMBERS as u64) : (id += 1) {
        expect_eq(node.upsertPeer(id, CAP_LLM, 1300), GossipApplyOutcome.applied_new);
    }
    expect_eq(node.stats().member_count, GOSSIP_MAX_MEMBERS);

    // 65th distinct agent returns capacity_full
    expect_eq(node.upsertPeer(999, CAP_LLM, 1400), GossipApplyOutcome.capacity_full);
}

test("GossipHealth: telemetry stats snapshot & cap.a2a namespace facade parity") {
    let node = cap.a2a.GossipHealthMachine.init(100, 0xAA, 5000, cap.a2a.GossipConfig{
        suspect_timeout_ms: 1000,
        retransmit_limit: 2,
    }) ?? return;

    * = node.upsertPeer(101, 0x01, 5000);
    * = node.upsertPeer(102, 0x02, 5000);
    expect_true(node.markSuspect(101, 5100));
    expect_true(node.markSuspect(102, 5500));
    expect_eq(node.reapSuspects(6100), 1 as usize); // 101 -> dead, 102 stays suspect

    // Trigger self-refutation
    expect_eq(node.applyUpdate(cap.a2a.GossipUpdate{
        agent_id: 100,
        state: cap.a2a.GossipMemberState.suspect,
        incarnation: 1,
    }, 6200), cap.a2a.GossipApplyOutcome.refuted_self);

    let out: [4]cap.a2a.GossipUpdate = [cap.a2a.GossipUpdate{}; 4];
    let out_slice: []cap.a2a.GossipUpdate = out[0..];
    const sent = node.buildGossipBatch(4, out_slice);
    expect_true(sent >= 1);

    const st = node.stats();
    expect_eq(st.local_agent_id, 100 as u64);
    expect_eq(st.local_incarnation, 2 as u64);
    expect_eq(st.member_count, 3 as usize);
    expect_eq(st.capacity, GOSSIP_MAX_MEMBERS);
    expect_eq(st.alive_count, 1 as usize);
    expect_eq(st.suspect_count, 1 as usize);
    expect_eq(st.dead_count, 1 as usize);
    expect_eq(st.total_self_refutations, 1 as u64);
    expect_eq(st.total_suspects_reaped, 1 as u64);
    expect_eq(st.total_gossip_sent, sent as u64);
}
