// Origin: Pure-CP Stdlib Test Suite (lib/net/connTrack.cp) | Created: 2026-11-23
// Deep production regression tests for cap:net L4 bidirectional conntrack table (`lib/net/connTrack.cp`):
//   1. Symmetric 5-tuple hashing, full TCP handshake/teardown lifecycle & bidirectional packet/byte counters
//   2. IPv6 UDP `udp_unreplied` -> `udp_assured` promotion & TCP RST immediate `closed` transition
//   3. Half-open (`syn_sent`/`syn_recv`) quota enforcement & release upon handshake completion
//   4. Per-state idle timeout sweeping (`reapExpired`) across SYN / UDP / Established lifetimes
//   5. Full 128-slot table priority reclamation (`closed` -> `time_wait` -> `rejected_table_full`)
//   6. Invalid config, unsupported protocol rejection, bidirectional `lookup` & `removeFlow`
//   7. Zero-heap `ConnTrackStats`, `reset()`, and `cap.net` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CONNTRACK_MAX_FLOWS,
    CONNTRACK_PROTO_TCP,
    CONNTRACK_PROTO_UDP,
    CT_TCP_FIN,
    CT_TCP_SYN,
    CT_TCP_RST,
    CT_TCP_ACK,
    ConnTrackState,
    ConnTrackDir,
    ConnTrackAction,
    ConnTrackTuple,
    ConnTrackConfig,
    ConnTrackTable,
} from "cap:net";

test("connTrack: full TCP lifecycle, symmetric hash & bidirectional packet/byte accounting") {
    let ct = ConnTrackTable.init(ConnTrackConfig.defaults()) ?? return;
    const c2s = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [192, 168, 1, 10], 54321, [10, 0, 0, 5], 443);
    const s2c = c2s.reverse();

    // Commutative symmetric hash must be identical for both directions
    expect_eq(c2s.symmetricHash(), s2c.symmetricHash());

    // 1. SYN (original) -> created, syn_sent, half_open=1
    const o1 = ct.observePacket(c2s, CT_TCP_SYN, 60, 1000);
    expect_true(o1.action == ConnTrackAction.created);
    expect_true(o1.direction == ConnTrackDir.original);
    expect_true(o1.state == ConnTrackState.syn_sent);
    expect_eq(ct.stats().half_open_flows, @as(usize, 1));

    // 2. SYN+ACK (reply) -> updated on same slot, syn_recv, half_open still 1
    const o2 = ct.observePacket(s2c, CT_TCP_SYN | CT_TCP_ACK, 64, 1010);
    expect_true(o2.action == ConnTrackAction.updated);
    expect_true(o2.direction == ConnTrackDir.reply);
    expect_true(o2.state == ConnTrackState.syn_recv);
    expect_eq(o2.flow_id, o1.flow_id);
    expect_eq(o2.slot_idx, o1.slot_idx);
    expect_eq(ct.stats().half_open_flows, @as(usize, 1));

    // 3. ACK (original) -> established, half_open drops to 0
    const o3 = ct.observePacket(c2s, CT_TCP_ACK, 52, 1020);
    expect_true(o3.state == ConnTrackState.established);
    expect_eq(ct.stats().half_open_flows, @as(usize, 0));

    // 4. Data packets in both directions + FIN handshake -> fin_wait -> time_wait
    * = ct.observePacket(c2s, CT_TCP_ACK, 1200, 1030);
    * = ct.observePacket(s2c, CT_TCP_ACK, 1400, 1040);
    const o_fin1 = ct.observePacket(c2s, CT_TCP_FIN | CT_TCP_ACK, 52, 1050);
    expect_true(o_fin1.state == ConnTrackState.fin_wait);
    const o_fin2 = ct.observePacket(s2c, CT_TCP_FIN | CT_TCP_ACK, 52, 1060);
    expect_true(o_fin2.state == ConnTrackState.time_wait);

    // Bidirectional lookup from reply tuple returns exact cumulative counters
    if (entry := ct.lookup(s2c)) {
        expect_eq(entry.orig_packets, @as(u64, 4));
        expect_eq(entry.orig_bytes, @as(u64, 60 + 52 + 1200 + 52));
        expect_eq(entry.reply_packets, @as(u64, 3));
        expect_eq(entry.reply_bytes, @as(u64, 64 + 1400 + 52));
        expect_true(entry.state == ConnTrackState.time_wait);
    } else {
        expect_true(false);
    }
}

test("connTrack: IPv6 UDP unreplied -> assured promotion and TCP RST close") {
    let ct = ConnTrackTable.init(ConnTrackConfig.defaults()) ?? return;

    const v6_a: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    const v6_b: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2];
    const udp_req = ConnTrackTuple.fromIpv6(CONNTRACK_PROTO_UDP, v6_a, 53000, v6_b, 53);
    const udp_rep = udp_req.reverse();

    const u1 = ct.observePacket(udp_req, 0, 80, 100);
    expect_true(u1.action == ConnTrackAction.created);
    expect_true(u1.state == ConnTrackState.udp_unreplied);

    const u2 = ct.observePacket(udp_rep, 0, 140, 120);
    expect_true(u2.action == ConnTrackAction.updated);
    expect_true(u2.direction == ConnTrackDir.reply);
    expect_true(u2.state == ConnTrackState.udp_assured);

    // TCP RST immediately transitions established flow to closed
    const tcp_t = ConnTrackTuple.fromIpv6(CONNTRACK_PROTO_TCP, v6_a, 40000, v6_b, 80);
    * = ct.observePacket(tcp_t, CT_TCP_SYN, 60, 200);
    const rst_out = ct.observePacket(tcp_t.reverse(), CT_TCP_RST, 40, 210);
    expect_true(rst_out.state == ConnTrackState.closed);
    expect_eq(ct.stats().half_open_flows, @as(usize, 0));
}

test("connTrack: half-open quota protection blocks excess SYN flows until handshakes complete") {
    let cfg = ConnTrackConfig.defaults();
    cfg.max_half_open = 4;
    let ct = ConnTrackTable.init(cfg) ?? return;

    // 4 distinct SYN flows succeed
    let p: u16 = 10000;
    while (p < 10004) {
        const t = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 1, 0, 1], p, [10, 2, 0, 1], 80);
        expect_true(ct.observePacket(t, CT_TCP_SYN, 60, 1000).action == ConnTrackAction.created);
        p += 1;
    }
    expect_eq(ct.stats().half_open_flows, @as(usize, 4));

    // 5th SYN is blocked by half-open quota
    const t5 = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 1, 0, 1], 10004, [10, 2, 0, 1], 80);
    expect_true(ct.observePacket(t5, CT_TCP_SYN, 60, 1000).action == ConnTrackAction.rejected_half_open_quota);
    expect_eq(ct.stats().total_half_open_rejected, @as(u64, 1));

    // Complete handshake on flow #1 (port 10000) -> half_open drops 4 -> 3 -> t5 is now admitted!
    const t0 = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 1, 0, 1], 10000, [10, 2, 0, 1], 80);
    * = ct.observePacket(t0.reverse(), CT_TCP_SYN | CT_TCP_ACK, 60, 1010);
    * = ct.observePacket(t0, CT_TCP_ACK, 52, 1020);
    expect_eq(ct.stats().half_open_flows, @as(usize, 3));

    expect_true(ct.observePacket(t5, CT_TCP_SYN, 60, 1030).action == ConnTrackAction.created);
    expect_eq(ct.stats().half_open_flows, @as(usize, 4));
}

test("connTrack: per-state idle timeout sweeping via reapExpired") {
    let cfg = ConnTrackConfig.defaults();
    cfg.syn_timeout_ms = 1000;
    cfg.udp_unreplied_timeout_ms = 2000;
    cfg.udp_assured_timeout_ms = 5000;
    cfg.established_timeout_ms = 10000;
    let ct = ConnTrackTable.init(cfg) ?? return;

    // 1. syn_sent (timeout 1000)
    const f_syn = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 0, 0, 1], 1001, [10, 0, 0, 2], 80);
    * = ct.observePacket(f_syn, CT_TCP_SYN, 60, 1000);

    // 2. udp_unreplied (timeout 2000)
    const f_u1 = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_UDP, [10, 0, 0, 1], 2001, [10, 0, 0, 2], 53);
    * = ct.observePacket(f_u1, 0, 60, 1000);

    // 3. udp_assured (timeout 5000)
    const f_u2 = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_UDP, [10, 0, 0, 1], 2002, [10, 0, 0, 2], 53);
    * = ct.observePacket(f_u2, 0, 60, 1000);
    * = ct.observePacket(f_u2.reverse(), 0, 60, 1000);

    // 4. established (timeout 10000)
    const f_est = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 0, 0, 1], 1002, [10, 0, 0, 2], 443);
    * = ct.observePacket(f_est, CT_TCP_SYN, 60, 1000);
    * = ct.observePacket(f_est.reverse(), CT_TCP_SYN | CT_TCP_ACK, 60, 1000);
    * = ct.observePacket(f_est, CT_TCP_ACK, 52, 1000);

    expect_eq(ct.stats().active_flows, @as(usize, 4));

    // t=2000 (idle 1000): only syn_sent expires
    expect_eq(ct.reapExpired(2000), @as(usize, 1));
    expect_eq(ct.stats().half_open_flows, @as(usize, 0));
    expect_eq(ct.stats().active_flows, @as(usize, 3));

    // t=3000 (idle 2000): udp_unreplied expires
    expect_eq(ct.reapExpired(3000), @as(usize, 1));
    expect_eq(ct.stats().active_flows, @as(usize, 2));

    // t=6000 (idle 5000): udp_assured expires, established remains
    expect_eq(ct.reapExpired(6000), @as(usize, 1));
    expect_eq(ct.stats().active_flows, @as(usize, 1));
    expect_true(ct.lookup(f_est) != null);
}

test("connTrack: full 128-slot table prioritizes closed then time_wait reclamation before drop") {
    let ct = ConnTrackTable.init(ConnTrackConfig.defaults()) ?? return;

    // Fill all 128 slots with established TCP flows (ACK mid-stream)
    let k: u16 = 0;
    while (k < @as(u16, @intCast(CONNTRACK_MAX_FLOWS))) {
        const t = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 0, 1, 1], 20000 + k, [10, 0, 2, 1], 443);
        expect_true(ct.observePacket(t, CT_TCP_ACK, 64, 1000).action == ConnTrackAction.created);
        k += 1;
    }
    expect_eq(ct.stats().active_flows, CONNTRACK_MAX_FLOWS);

    // Mark port 20005 as time_wait (bidirectional FIN) and port 20010 as closed (RST)
    const t_tw = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 0, 1, 1], 20005, [10, 0, 2, 1], 443);
    * = ct.observePacket(t_tw, CT_TCP_FIN | CT_TCP_ACK, 52, 1010);
    * = ct.observePacket(t_tw.reverse(), CT_TCP_FIN | CT_TCP_ACK, 52, 1020);

    const t_cl = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 0, 1, 1], 20010, [10, 0, 2, 1], 443);
    * = ct.observePacket(t_cl, CT_TCP_RST, 40, 1030);

    // New flow A reclaims `closed` slot (port 20010) first!
    const t_new_a = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_UDP, [10, 9, 9, 1], 40001, [10, 9, 9, 2], 53);
    const oa = ct.observePacket(t_new_a, 0, 64, 1040);
    expect_true(oa.action == ConnTrackAction.created);
    expect_true(oa.reclaimed_slot);
    expect_true(ct.lookup(t_cl) == null);
    expect_true(ct.lookup(t_tw) != null);

    // New flow B reclaims `time_wait` slot (port 20005) next!
    const t_new_b = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_UDP, [10, 9, 9, 1], 40002, [10, 9, 9, 2], 53);
    const ob = ct.observePacket(t_new_b, 0, 64, 1050);
    expect_true(ob.action == ConnTrackAction.created);
    expect_true(ob.reclaimed_slot);
    expect_true(ct.lookup(t_tw) == null);

    // New flow C finds all 128 slots active -> rejected_table_full
    const t_new_c = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_UDP, [10, 9, 9, 1], 40003, [10, 9, 9, 2], 53);
    const oc = ct.observePacket(t_new_c, 0, 64, 1060);
    expect_true(oc.action == ConnTrackAction.rejected_table_full);
    expect_eq(ct.stats().total_reclaimed_closed_tw, @as(u64, 2));
    expect_eq(ct.stats().total_table_full_dropped, @as(u64, 1));
}

test("connTrack: invalid config, unsupported protocol rejection & removeFlow") {
    let bad = ConnTrackConfig.defaults();
    bad.max_half_open = 0;
    expect_true(ConnTrackTable.init(bad) == null);
    bad.max_half_open = CONNTRACK_MAX_FLOWS + 1;
    expect_true(ConnTrackTable.init(bad) == null);

    let ct = ConnTrackTable.init(ConnTrackConfig.defaults()) ?? return;
    // Protocol 1 (ICMP) is not L4 TCP/UDP -> rejected_invalid
    const icmp = ConnTrackTuple.fromIpv4(1, [10, 0, 0, 1], 0, [10, 0, 0, 2], 0);
    expect_true(ct.observePacket(icmp, 0, 64, 100).action == ConnTrackAction.rejected_invalid);

    const tcp = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 0, 0, 1], 1234, [10, 0, 0, 2], 80);
    const out = ct.observePacket(tcp, CT_TCP_SYN, 60, 100);
    expect_true(ct.removeFlow(out.flow_id));
    expect_eq(ct.removeFlow(out.flow_id), false);
    expect_eq(ct.stats().active_flows, @as(usize, 0));
    expect_eq(ct.stats().half_open_flows, @as(usize, 0));
}

test("connTrack: stats snapshot, reset, and cap.net facade parity") {
    let ct = cap.net.ConnTrackTable.init(cap.net.ConnTrackConfig.defaults()) ?? return;
    const t = cap.net.ConnTrackTuple.fromIpv4(cap.net.CONNTRACK_PROTO_UDP, [172, 16, 0, 1], 5000, [172, 16, 0, 2], 5001);
    * = ct.observePacket(t, 0, 100, 1000);
    * = ct.observePacket(t.reverse(), 0, 200, 1010);

    const st = ct.stats();
    expect_eq(st.capacity, CONNTRACK_MAX_FLOWS);
    expect_eq(st.active_flows, @as(usize, 1));
    expect_eq(st.total_created, @as(u64, 1));
    expect_eq(st.total_hits, @as(u64, 1));

    ct.reset();
    const st2 = ct.stats();
    expect_eq(st2.active_flows, @as(usize, 0));
    expect_eq(st2.total_created, @as(u64, 0));
    expect_eq(st2.total_hits, @as(u64, 0));
}
