// Wave 38 R4 Reference-Hardened Verification Suite
// Targets: lib/net/blockList.cp & lib/net/connTrack.cp
// Reference: krisprice/ipnet & smoltcp (MIT / 0BSD)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { Ipv4Addr } from "../../lib/net/ipaddr.cp";
import {
    BLOCKLIST_MAX_RULES,
    simdFwIpv6MatchesPrefix,
    scalarFwIpv6MatchesPrefix,
    BlockListFamily,
    BlockListError,
    BlockListAddr,
    BlockList,
} from "../../lib/net/blockList.cp";
import {
    CONNTRACK_MAX_FLOWS,
    CONNTRACK_PROTO_TCP,
    CONNTRACK_PROTO_UDP,
    CT_TCP_FIN,
    CT_TCP_SYN,
    CT_TCP_RST,
    CT_TCP_ACK,
    simdFwBytesEqual16,
    scalarFwBytesEqual16,
    ctElapsedMsSaturating,
    ConnTrackState,
    ConnTrackDir,
    ConnTrackAction,
    ConnTrackTuple,
    ConnTrackConfig,
    ConnTrackTable,
} from "../../lib/net/connTrack.cp";

struct XorShift64 {
    state: u64 = 0x57333852_34465701;

    static fn init(seed: u64): XorShift64 {
        const fallback: u64 = (@as(u64, 0x9E3779B9) << 32) | 0x7F4A7C15;
        return XorShift64{ state: if (seed == 0) fallback else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

fn fillRandomBytes16(rng: *XorShift64, out: *[16]u8) {
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        out[i] = @as(u8, @truncate(rng.next()));
    }
}

fn oracleV4MaskMatch(ip: u32, net: u32, prefix: u8): bool {
    if (prefix == 0) return true;
    if (prefix >= 32) return ip == net;
    const shift: u5 = @as(u5, @intCast(32 - prefix));
    return (ip >> shift) == (net >> shift);
}

fn oracleV6MaskMatch(ip: u128, net: u128, prefix: u8): bool {
    if (prefix == 0) return true;
    if (prefix >= 128) return ip == net;
    const shift: u7 = @as(u7, @intCast(128 - prefix));
    return (ip >> shift) == (net >> shift);
}

fn checkTest1BlockListBoundaries(): u32 {
    let bl = BlockList.init();

    // 1a. Single address IPv4 & IPv6 + IPv4-mapped IPv6 cross-family consistency
    bl.addAddress("192.168.10.5", BlockListFamily.Ipv4) catch return 1;
    bl.addAddress("2001:db8::42", BlockListFamily.Ipv6) catch return 2;
    bl.addAddress("::ffff:10.20.30.40", BlockListFamily.Ipv6) catch return 3;

    if (!bl.check("192.168.10.5", BlockListFamily.Ipv4)) return 4;
    if (!bl.check("::ffff:192.168.10.5", BlockListFamily.Ipv6)) return 5;
    if (!bl.check("10.20.30.40", BlockListFamily.Ipv4)) return 6;
    if (!bl.check("::ffff:10.20.30.40", BlockListFamily.Ipv6)) return 7;
    if (!bl.check("2001:db8::42", BlockListFamily.Ipv6)) return 8;
    if (bl.check("2001:db8::43", BlockListFamily.Ipv6)) return 9;
    if (bl.check("not-an-ip", BlockListFamily.Ipv4)) return 10;

    // 1b. Inclusive ranges: reject start > end and cross-family
    bl.addRange("10.0.0.10", "10.0.0.20", BlockListFamily.Ipv4) catch return 11;
    if (!bl.check("10.0.0.10", BlockListFamily.Ipv4)) return 12;
    if (!bl.check("10.0.0.15", BlockListFamily.Ipv4)) return 13;
    if (!bl.check("10.0.0.20", BlockListFamily.Ipv4)) return 14;
    if (!bl.check("::ffff:10.0.0.15", BlockListFamily.Ipv6)) return 15;
    if (bl.check("10.0.0.9", BlockListFamily.Ipv4)) return 16;
    if (bl.check("10.0.0.21", BlockListFamily.Ipv4)) return 17;

    let rev_rejected = false;
    bl.addRange("10.0.0.20", "10.0.0.10", BlockListFamily.Ipv4) catch |e| {
        if (e == @as(anyerror, error.InvalidRange)) rev_rejected = true;
    };
    if (!rev_rejected) return 18;

    let cross_rejected = false;
    bl.addRangeAuto("10.0.0.1", "2001:db8::1") catch |e| {
        if (e == @as(anyerror, error.FamilyMismatch)) cross_rejected = true;
    };
    if (!cross_rejected) return 19;

    // 1c. CIDR prefix bounds: IPv4 > 32, IPv6 > 128, leading zero rejected
    let p33_rejected = false;
    bl.addSubnet("10.0.0.0", 33, BlockListFamily.Ipv4) catch |e| {
        if (e == @as(anyerror, error.InvalidPrefix)) p33_rejected = true;
    };
    if (!p33_rejected) return 20;

    let p129_rejected = false;
    bl.addSubnet("2001:db8::", 129, BlockListFamily.Ipv6) catch |e| {
        if (e == @as(anyerror, error.InvalidPrefix)) p129_rejected = true;
    };
    if (!p129_rejected) return 21;

    let lz_rejected = false;
    bl.addSubnetCidr("10.0.0.0/08") catch |e| {
        if (e == @as(anyerror, error.InvalidPrefix)) lz_rejected = true;
    };
    if (!lz_rejected) return 22;

    // IPv4-mapped IPv6 CIDR matching native IPv4
    bl.addSubnetCidr("::ffff:172.16.0.0/108") catch return 23; // /12 in IPv4
    if (!bl.check("172.16.5.9", BlockListFamily.Ipv4)) return 24;
    if (!bl.check("172.31.255.254", BlockListFamily.Ipv4)) return 25;
    if (bl.check("172.32.0.1", BlockListFamily.Ipv4)) return 26;

    // 1d. Prefix == 0 full match boundary (IPv4 & IPv6)
    let bl_all4 = BlockList.init();
    bl_all4.addSubnetCidr("0.0.0.0/0") catch return 27;
    if (!bl_all4.check("1.2.3.4", BlockListFamily.Ipv4)) return 28;
    if (!bl_all4.check("::ffff:203.0.113.7", BlockListFamily.Ipv6)) return 29;
    if (bl_all4.check("2001:db8::1", BlockListFamily.Ipv6)) return 30;

    let bl_all6 = BlockList.init();
    bl_all6.addSubnetCidr("::/0") catch return 31;
    if (!bl_all6.check("fe80::1", BlockListFamily.Ipv6)) return 32;
    if (!bl_all6.check("::ffff:1.2.3.4", BlockListFamily.Ipv6)) return 33;

    // 1e. Capacity cap (256 rules) & mergeFrom overflow guard
    let bl_cap = BlockList.init();
    let i: usize = 0;
    while (i < BLOCKLIST_MAX_RULES) : (i += 1) {
        bl_cap.addAddress("127.0.0.1", BlockListFamily.Ipv4) catch return 34;
    }
    let cap_rejected = false;
    bl_cap.addAddress("127.0.0.2", BlockListFamily.Ipv4) catch |e| {
        if (e == @as(anyerror, error.CapacityFull)) cap_rejected = true;
    };
    if (!cap_rejected) return 35;

    let merge_rejected = false;
    bl_cap.mergeFrom(bl_all4) catch |e| {
        if (e == @as(anyerror, error.CapacityFull)) merge_rejected = true;
    };
    if (!merge_rejected) return 36;

    return 0;
}

fn checkTest2ConnTrackBoundaries(): u32 {
    // 2a. Invalid config rejected
    let bad_cfg = ConnTrackConfig.defaults();
    bad_cfg.max_half_open = 0;
    if (ConnTrackTable.init(bad_cfg) != null) return 1;

    let cfg = ConnTrackConfig{
        max_half_open: 3,
        syn_timeout_ms: 5_000,
        established_timeout_ms: 60_000,
        fin_wait_timeout_ms: 10_000,
        time_wait_timeout_ms: 4_000,
        udp_unreplied_timeout_ms: 5_000,
        udp_assured_timeout_ms: 30_000,
    };
    let ct = ConnTrackTable.init(cfg) ?? return 2;

    // 2b. Bidirectional 5-tuple symmetry (IPv6) & full TCP handshake + FIN/TIME_WAIT
    const v6a: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    const v6b: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2];
    const c2s = ConnTrackTuple.fromIpv6(CONNTRACK_PROTO_TCP, v6a, 50000, v6b, 443);
    const s2c = c2s.reverse();
    if (c2s.symmetricHash() != s2c.symmetricHash()) return 3;

    const r1 = ct.observePacket(c2s, CT_TCP_SYN, 60, 1000);
    if (r1.action != ConnTrackAction.created || r1.state != ConnTrackState.syn_sent) return 4;
    const r2 = ct.observePacket(s2c, CT_TCP_SYN | CT_TCP_ACK, 64, 1010);
    if (r2.action != ConnTrackAction.updated || r2.direction != ConnTrackDir.reply) return 5;
    if (r2.flow_id != r1.flow_id || r2.slot_idx != r1.slot_idx || r2.state != ConnTrackState.syn_recv) return 6;
    const r3 = ct.observePacket(c2s, CT_TCP_ACK, 52, 1020);
    if (r3.state != ConnTrackState.established || r3.flow_id != r1.flow_id) return 7;
    if (ct.stats().half_open_flows != 0) return 8;

    // 2c. Half-open quota guard (max_half_open = 3)
    const ip10_1: [4]u8 = [10, 0, 0, 1];
    const ip10_2: [4]u8 = [10, 0, 0, 2];
    let p: u16 = 10000;
    while (p < 10003) : (p += 1) {
        const t = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, ip10_1, p, ip10_2, 80);
        const o = ct.observePacket(t, CT_TCP_SYN, 60, 2000);
        if (o.action != ConnTrackAction.created) return 9;
    }
    const t_over = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, ip10_1, 10099, ip10_2, 80);
    const o_over = ct.observePacket(t_over, CT_TCP_SYN, 60, 2000);
    if (o_over.action != ConnTrackAction.rejected_half_open_quota) return 10;

    // 2d. Clock rollback saturation (now_ms < last_seen_ms) must NOT reap or panic
    if (ctElapsedMsSaturating(500, 2000) != 0) return 11;
    if (ctElapsedMsSaturating(1000, -1000) != 2000) return 12;
    const reaped_rollback = ct.reapExpired(100);
    if (reaped_rollback != 0) return 13;

    // Idle timeout reaping: at t=7000, 3 half-open SYN flows (created at 2000, timeout 5000) expire
    const reaped_syn = ct.reapExpired(7000);
    if (reaped_syn != 3 || ct.stats().half_open_flows != 0 || ct.stats().active_flows != 1) return 14;

    // 2e. Full table (128 slots): reclamation of closed slot + rejected_table_full when all established
    ct.reset();
    const ip172_1: [4]u8 = [172, 16, 0, 1];
    const ip172_2: [4]u8 = [172, 16, 0, 2];
    let k: u16 = 1;
    while (k <= @as(u16, CONNTRACK_MAX_FLOWS)) : (k += 1) {
        const t = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_UDP, ip172_1, k, ip172_2, 53);
        const o = ct.observePacket(t, 0, 64, 1000 + @as(i64, k));
        if (o.action != ConnTrackAction.created) return 15;
    }
    const t_extra = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_UDP, ip172_1, 9999, ip172_2, 53);
    const o_full = ct.observePacket(t_extra, 0, 64, 5000);
    if (o_full.action != ConnTrackAction.rejected_table_full) return 16;

    // Remove one, insert TCP and close it with RST, then verify full-table reclamation reuses closed slot
    if (!ct.removeFlow(1)) return 17;
    const ip10_11: [4]u8 = [10, 1, 1, 1];
    const ip10_12: [4]u8 = [10, 1, 1, 2];
    const t_tcp = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, ip10_11, 4000, ip10_12, 443);
    const o_rst = ct.observePacket(t_tcp, CT_TCP_RST, 40, 5001);
    if (o_rst.action != ConnTrackAction.created || o_rst.state != ConnTrackState.closed) return 18;
    const o_rec = ct.observePacket(t_extra, 0, 64, 5002);
    if (o_rec.action != ConnTrackAction.created || !o_rec.reclaimed_slot) return 19;
    if (!ct.verifyInvariants()) return 20;

    return 0;
}

fn checkTest3SimdDifferential(): u32 {
    let rng = XorShift64.init(0x57333852_3453494D);

    // 3a. simdFwBytesEqual16 vs scalarFwBytesEqual16: identical, single-bit flips at every byte, random
    let a: [16]u8 = [0; 16];
    let b: [16]u8 = [0; 16];
    let iter: usize = 0;
    while (iter < 64) : (iter += 1) {
        fillRandomBytes16(&rng, &a);
        b = a;
        if (simdFwBytesEqual16(a, b) != scalarFwBytesEqual16(a, b) || !simdFwBytesEqual16(a, b)) {
            return 1;
        }
        let pos: usize = 0;
        while (pos < 16) : (pos += 1) {
            b[pos] ^= @as(u8, 1) << @as(u3, @truncate(iter));
            if (simdFwBytesEqual16(a, b) != scalarFwBytesEqual16(a, b) || simdFwBytesEqual16(a, b)) {
                return 2;
            }
            b[pos] = a[pos];
        }
    }

    // 3b. simdFwIpv6MatchesPrefix vs scalarFwIpv6MatchesPrefix across all prefixes 0..=132
    let ip: [16]u8 = [0; 16];
    let net: [16]u8 = [0; 16];
    let pref: usize = 0;
    while (pref <= 132) : (pref += 1) {
        const p8: u8 = @as(u8, @intCast(pref));
        let trial: usize = 0;
        while (trial < 8) : (trial += 1) {
            fillRandomBytes16(&rng, &net);
            ip = net;
            // Mutate a bit either inside or outside prefix
            const bit_idx: usize = @as(usize, @truncate(rng.next())) % 128;
            const byte_i: usize = bit_idx / 8;
            const bit_in_byte: u3 = @as(u3, @intCast(7 - (bit_idx % 8)));
            ip[byte_i] ^= (@as(u8, 1) << bit_in_byte);

            const r_simd = simdFwIpv6MatchesPrefix(ip, net, p8);
            const r_scalar = scalarFwIpv6MatchesPrefix(ip, net, p8);
            if (r_simd != r_scalar) return 3;
        }
    }

    return 0;
}

fn checkTest4PrngFuzz1600(): u32 {
    let rng = XorShift64.init(0x57333852_3446555A);

    // 4a. 800 BlockList differential model checks (IPv4, IPv6, IPv4-mapped IPv6)
    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        let bl = BlockList.init();
        const mode = rng.next() % 4;

        if (mode == 0) {
            // IPv4 CIDR vs native IPv4 & mapped IPv6
            const net_u32: u32 = @as(u32, @truncate(rng.next()));
            const q_u32: u32 = @as(u32, @truncate(rng.next()));
            const p: u8 = @as(u8, @intCast(rng.next() % 33));
            let nbuf: [32]u8 = [0; 32];
            const nstr = Ipv4Addr.fromU32(net_u32).format(nbuf[0..]) ?? return 1;
            bl.addSubnet(nstr, p, BlockListFamily.Ipv4) catch return 2;

            const expected = oracleV4MaskMatch(q_u32, net_u32, p);
            const got_v4 = bl.checkAddr(BlockListAddr.fromV4(Ipv4Addr.fromU32(q_u32)));
            const mapped_u128: u128 = (@as(u128, 0xFFFF) << 32) | @as(u128, q_u32);
            const got_mapped = bl.checkAddr(BlockListAddr{ family: BlockListFamily.Ipv6, v4: 0, v6: mapped_u128 });
            if (got_v4 != expected || got_mapped != expected) return 3;
        } else if (mode == 1) {
            // IPv4 Range [lo, hi]
            let a_u32: u32 = @as(u32, @truncate(rng.next()));
            let b_u32: u32 = @as(u32, @truncate(rng.next()));
            if (a_u32 > b_u32) {
                const tmp = a_u32;
                a_u32 = b_u32;
                b_u32 = tmp;
            }
            const q_u32: u32 = @as(u32, @truncate(rng.next()));
            let sbuf: [32]u8 = [0; 32];
            let ebuf: [32]u8 = [0; 32];
            const sstr = Ipv4Addr.fromU32(a_u32).format(sbuf[0..]) ?? return 4;
            const estr = Ipv4Addr.fromU32(b_u32).format(ebuf[0..]) ?? return 5;
            bl.addRange(sstr, estr, BlockListFamily.Ipv4) catch return 6;

            const expected = (q_u32 >= a_u32 && q_u32 <= b_u32);
            const got = bl.checkAddr(BlockListAddr.fromV4(Ipv4Addr.fromU32(q_u32)));
            if (got != expected) return 7;
        } else if (mode == 2) {
            // IPv6 CIDR (SIMD path) vs pure u128 oracle
            const hi_n: u64 = rng.next();
            const lo_n: u64 = rng.next();
            const net_u128: u128 = (@as(u128, hi_n) << 64) | @as(u128, lo_n);
            const q_u128: u128 = if ((rng.next() & 1) == 0)
                net_u128 ^ (@as(u128, 1) << @as(u7, @intCast(rng.next() % 128)))
            else
                (@as(u128, rng.next()) << 64) | @as(u128, rng.next());
            const p: u8 = @as(u8, @intCast(rng.next() % 129));
            let nbuf: [64]u8 = [0; 64];
            const naddr = BlockListAddr{ family: BlockListFamily.Ipv6, v4: 0, v6: net_u128 };
            const nstr = naddr.format(nbuf[0..]) ?? return 8;
            bl.addSubnet(nstr, p, BlockListFamily.Ipv6) catch return 9;

            const expected = oracleV6MaskMatch(q_u128, net_u128, p);
            const got = bl.checkAddr(BlockListAddr{ family: BlockListFamily.Ipv6, v4: 0, v6: q_u128 });
            if (got != expected) return 10;
        } else {
            // IPv6 single address exact match (SIMD path)
            const net_u128: u128 = (@as(u128, rng.next()) << 64) | @as(u128, rng.next());
            const same = (rng.next() & 1) == 0;
            const q_u128: u128 = if (same) net_u128 else net_u128 ^ 1;
            let nbuf: [64]u8 = [0; 64];
            const naddr = BlockListAddr{ family: BlockListFamily.Ipv6, v4: 0, v6: net_u128 };
            const nstr = naddr.format(nbuf[0..]) ?? return 11;
            bl.addAddress(nstr, BlockListFamily.Ipv6) catch return 12;

            const got = bl.checkAddr(BlockListAddr{ family: BlockListFamily.Ipv6, v4: 0, v6: q_u128 });
            if (got != same) return 13;
        }
    }

    // 4b. 800 ConnTrack randomized state machine steps with continuous invariant verification
    let ct = ConnTrackTable.init(ConnTrackConfig{
        max_half_open: 16,
        syn_timeout_ms: 2_000,
        established_timeout_ms: 15_000,
        fin_wait_timeout_ms: 4_000,
        time_wait_timeout_ms: 2_000,
        udp_unreplied_timeout_ms: 3_000,
        udp_assured_timeout_ms: 10_000,
    }) ?? return 14;

    let now_ms: i64 = 10_000;
    let step: usize = 0;
    while (step < 800) : (step += 1) {
        const op = rng.next() % 10;
        if (op == 0) {
            // Forward clock jump + reap
            now_ms += @as(i64, @intCast(rng.next() % 5000));
            * = ct.reapExpired(now_ms);
        } else if (op == 1) {
            // Clock rollback probe
            const back_ms = now_ms - @as(i64, @intCast(rng.next() % 2000));
            * = ct.reapExpired(back_ms);
        } else if (op == 2) {
            // Random removeFlow
            * = ct.removeFlow(1 + (rng.next() % 64));
        } else {
            const is_tcp = (rng.next() & 1) == 0;
            const proto: u8 = if (is_tcp) CONNTRACK_PROTO_TCP else CONNTRACK_PROTO_UDP;
            const flow_key: u16 = (1000 as u16) + @as(u16, @intCast(rng.next() % 160));
            const src_v4: [4]u8 = [10, 0, 0, @as(u8, @truncate(flow_key & 0x0F))];
            const dst_v4: [4]u8 = [10, 1, 0, 1];
            const base = ConnTrackTuple.fromIpv4(
                proto,
                src_v4,
                flow_key,
                dst_v4,
                if (is_tcp) 443 else 53,
            );
            const pkt = if ((rng.next() & 1) == 0) base else base.reverse();
            let flags: u8 = 0;
            if (is_tcp) {
                const fm = rng.next() % 5;
                if (fm == 0) flags = CT_TCP_SYN
                else if (fm == 1) flags = CT_TCP_SYN | CT_TCP_ACK
                else if (fm == 2) flags = CT_TCP_ACK
                else if (fm == 3) flags = CT_TCP_FIN | CT_TCP_ACK
                else flags = CT_TCP_RST;
            }
            const out = ct.observePacket(pkt, flags, @as(u32, @truncate(rng.next() % 1500)), now_ms);
            if (out.action == ConnTrackAction.created || out.action == ConnTrackAction.updated) {
                if (out.slot_idx >= CONNTRACK_MAX_FLOWS) return 15;
                if (ct.lookup(pkt) == null || ct.lookup(pkt.reverse()) == null) return 16;
            }
        }
        if (!ct.verifyInvariants()) return 17;
    }

    return 0;
}

fn main() {
    test("1. BlockList IPv4/IPv6 Address, Range, CIDR, IPv4-Mapped & Cap Guards", () => {
        expect_eq(checkTest1BlockListBoundaries(), 0 as u32);
    });

    test("2. ConnTrack Bidirectional Symmetry, Half-Open Quota, Rollback & Full-Table Guards", () => {
        expect_eq(checkTest2ConnTrackBoundaries(), 0 as u32);
    });

    test("3. 16-Lane SIMD vs Scalar 0-Diff Differential Parity", () => {
        expect_eq(checkTest3SimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG Fuzzing (800 BlockList + 800 ConnTrack = 1,600 Cases)", () => {
        expect_eq(checkTest4PrngFuzz1600(), 0 as u32);
    });
}
