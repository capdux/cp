// Wave 36 R9 Reference-Hardened Regression & Fuzz Suite
// Covers lib/net/{dhcp,bgp}.cp against smoltcp-rs/smoltcp (0BSD) & RFC 2131/4271

import { test, expect_eq } from "cap:test";
import {
    DHCP_MAX_OPTIONS,
    DHCP_MIN_PACKET_SIZE,
    DHCP_OP_REQUEST,
    DHCP_OP_REPLY,
    OPT_SUBNET_MASK,
    OPT_DNS,
    OPT_HOSTNAME,
    OPT_REQUESTED_IP,
    OPT_LEASE_TIME,
    OPT_MSG_TYPE,
    OPT_SERVER_ID,
    OPT_END,
    DhcpMsgType,
    DhcpWriter,
    parseDhcp,
    validateDhcpStrict,
    isDhcp,
} from "../../lib/net/dhcp.cp";
import {
    BGP_HEADER_SIZE,
    BGP_MARKER_LEN,
    scalarBgpIsAllFF16,
    simdBgpIsAllFF16,
    scalarNetCopyBytes16,
    simdNetCopyBytes16,
    BgpMsgType,
    BgpPrefixIter,
    BgpPathAttrIter,
    BgpStreamIter,
    parseBgpMessage,
    parseBgpOpen,
    parseBgpUpdate,
    parseBgpNotification,
    validateBgpStrict,
} from "../../lib/net/bgp.cp";

fn prngNext(state: u64): u64 {
    let z = state +% 0x6E37_79B9_7F4A_7C15;
    z = (z ^ (z >> 30)) *% 0x3F58_476D_1CE4_E5B9;
    z = (z ^ (z >> 27)) *% 0x14D0_49BB_1331_11EB;
    return z ^ (z >> 31);
}

fn checkDhcpVectors(): u32 {
    const mac: [6]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF];

    const ip_req: [4]u8 = [192, 168, 1, 50];
    const ip_yi: [4]u8 = [10, 0, 0, 42];
    const ip_mask: [4]u8 = [255, 255, 255, 0];
    const ip_srv: [4]u8 = [10, 0, 0, 1];

    // 1. DISCOVER round-trip with PAD + options
    let buf: [512]u8 = [0; 512];
    let w = DhcpWriter.init(buf[0..], DHCP_OP_REQUEST, 0x1234_5678, mac, true);
    if (!w.writeMsgType(DhcpMsgType.discover)) return 1;
    if (!w.writePad()) return 2;
    if (!w.writeIpOption(OPT_REQUESTED_IP, ip_req)) return 3;
    if (!w.writeOption(OPT_HOSTNAME, "cp-node-01")) return 4;
    const disc_pkt = w.finish() ?? return 5;

    if (!isDhcp(disc_pkt)) return 6;
    if (!validateDhcpStrict(disc_pkt)) return 7;
    const dp = parseDhcp(disc_pkt) ?? return 8;
    if (dp.header.xid != 0x1234_5678 || !dp.header.isBroadcast()) return 9;
    if (dp.header.macAddr()[5] != 0xFF) return 10;
    if (dp.msgType() != DhcpMsgType.discover) return 11;
    const req_ip = dp.requestedIp() ?? return 12;
    if (req_ip[0] != 192 || req_ip[3] != 50) return 13;
    const host = dp.hostname() ?? return 14;
    if (host.len != 10) return 15;

    // 2. OFFER / ACK round-trip
    let obuf: [512]u8 = [0; 512];
    let ow = DhcpWriter.init(obuf[0..], DHCP_OP_REPLY, 0xCAFE_BABE, mac, false);
    ow.setYiaddr(ip_yi);
    if (!ow.writeMsgType(DhcpMsgType.ack)) return 16;
    if (!ow.writeIpOption(OPT_SUBNET_MASK, ip_mask)) return 17;
    if (!ow.writeIpOption(OPT_SERVER_ID, ip_srv)) return 18;
    if (!ow.writeU32Option(OPT_LEASE_TIME, 86400)) return 19;
    const ack_pkt = ow.finish() ?? return 20;
    if (!validateDhcpStrict(ack_pkt)) return 21;
    const ap = parseDhcp(ack_pkt) ?? return 22;
    if (ap.msgType() != DhcpMsgType.ack) return 23;
    if ((ap.leaseTime() ?? 0) != 86400) return 24;
    if (ap.header.yiaddr[3] != 42) return 25;

    // 3. Reject short packet (< 240B)
    if (validateDhcpStrict(ack_pkt[0..239])) return 26;

    // 4. Reject bad Magic Cookie
    let bad_cookie: [512]u8 = obuf;
    bad_cookie[236] = 0x00;
    if (validateDhcpStrict(bad_cookie[0..ack_pkt.len])) return 27;

    // 5. Reject invalid op (0 or 3) and hlen > 16
    let bad_op: [512]u8 = obuf;
    bad_op[0] = 3;
    if (validateDhcpStrict(bad_op[0..ack_pkt.len])) return 28;
    let bad_hlen: [512]u8 = obuf;
    bad_hlen[2] = 17;
    if (validateDhcpStrict(bad_hlen[0..ack_pkt.len])) return 29;

    // 6. Reject truncated TLV option
    let trunc_opt: [244]u8 = [0; 244];
    let k: usize = 0;
    while (k < DHCP_MIN_PACKET_SIZE) : (k += 1) {
        trunc_opt[k] = obuf[k];
    }
    trunc_opt[240] = OPT_HOSTNAME;
    trunc_opt[241] = 10; // claims 10 bytes, only 2 follow
    trunc_opt[242] = 'a';
    trunc_opt[243] = 'b';
    if (validateDhcpStrict(trunc_opt[0..])) return 30;

    // 7. Reject malformed fixed-size option (OPT_LEASE_TIME len=2, OPT_DNS len=3)
    let bad_lease: [246]u8 = [0; 246];
    k = 0;
    while (k < DHCP_MIN_PACKET_SIZE) : (k += 1) {
        bad_lease[k] = obuf[k];
    }
    bad_lease[240] = OPT_LEASE_TIME;
    bad_lease[241] = 2;
    bad_lease[244] = OPT_END;
    if (validateDhcpStrict(bad_lease[0..245])) return 31;

    bad_lease[240] = OPT_DNS;
    bad_lease[241] = 3;
    bad_lease[245] = OPT_END;
    if (validateDhcpStrict(bad_lease[0..246])) return 32;

    // 8. Reject > DHCP_MAX_OPTIONS (65 options) resource-exhaustion guard
    let flood: [512]u8 = [0; 512];
    k = 0;
    while (k < DHCP_MIN_PACKET_SIZE) : (k += 1) {
        flood[k] = obuf[k];
    }
    let pos: usize = DHCP_MIN_PACKET_SIZE;
    let opt_i: usize = 0;
    while (opt_i < DHCP_MAX_OPTIONS + 1) : (opt_i += 1) {
        flood[pos] = 200; // private option code
        flood[pos + 1] = 1;
        flood[pos + 2] = 0x42;
        pos += 3;
    }
    flood[pos] = OPT_END;
    pos += 1;
    if (validateDhcpStrict(flood[0..pos])) return 33;

    return 0;
}

fn fillBgpMarker(buf: *[]u8) {
    let i: usize = 0;
    while (i < BGP_MARKER_LEN) : (i += 1) {
        buf[i] = 0xFF;
    }
}

fn checkBgpVectors(): u32 {
    // 1. Valid KEEPALIVE (exact 19B)
    let ka: [19]u8 = [0; 19];
    const ka_s: []u8 = ka[0..];
    fillBgpMarker(ka_s);
    ka[16] = 0;
    ka[17] = 19;
    ka[18] = 4; // KEEPALIVE
    if (!validateBgpStrict(ka[0..])) return 1;
    const ka_msg = parseBgpMessage(ka[0..]) ?? return 2;
    if (ka_msg.header.msg_type != BgpMsgType.keepalive || ka_msg.body.len != 0) return 3;

    // Reject KEEPALIVE with length == 20
    let ka_bad: [20]u8 = [0; 20];
    const ka_bad_s: []u8 = ka_bad[0..];
    fillBgpMarker(ka_bad_s);
    ka_bad[16] = 0;
    ka_bad[17] = 20;
    ka_bad[18] = 4;
    if (validateBgpStrict(ka_bad[0..])) return 4;

    // Reject corrupted Marker at every byte position 0..16
    let mpos: usize = 0;
    while (mpos < BGP_MARKER_LEN) : (mpos += 1) {
        let m_bad: [19]u8 = ka;
        m_bad[mpos] = 0xFE;
        if (validateBgpStrict(m_bad[0..])) return 5;
    }

    // 2. Valid OPEN (19B header + 10B fixed + 4B optional param TLV) = 33B
    let open_pkt: [33]u8 = [0; 33];
    const open_s: []u8 = open_pkt[0..];
    fillBgpMarker(open_s);
    open_pkt[16] = 0;
    open_pkt[17] = 33;
    open_pkt[18] = 1; // OPEN
    open_pkt[19] = 4; // version 4
    open_pkt[20] = 0xFD; open_pkt[21] = 0xE8; // AS 65000
    open_pkt[22] = 0; open_pkt[23] = 180; // hold time 180
    open_pkt[24] = 192; open_pkt[25] = 0; open_pkt[26] = 2; open_pkt[27] = 1; // BGP ID
    open_pkt[28] = 4; // opt_param_len = 4
    open_pkt[29] = 2; // Capability param type
    open_pkt[30] = 2; // param len = 2
    open_pkt[31] = 1; open_pkt[32] = 0;
    if (!validateBgpStrict(open_pkt[0..])) return 6;
    const om = parseBgpMessage(open_pkt[0..]) ?? return 7;
    const ob = parseBgpOpen(om.body) ?? return 8;
    if (ob.my_as != 65000 || ob.hold_time != 180) return 9;

    // Reject OPEN with truncated opt param TLV
    let open_bad: [33]u8 = open_pkt;
    open_bad[30] = 5; // claims 5 bytes, only 2 remain
    if (validateBgpStrict(open_bad[0..])) return 10;

    // 3. Valid UPDATE: 1 withdrawn (10.0.0.0/8), 1 ORIGIN path attr, 1 NLRI (192.0.2.0/24)
    // body = [0,2] + [8,10] + [0,4] + [0x40,1,1,0] + [24,192,0,2] = 14 bytes; total = 33B
    let upd: [33]u8 = [0; 33];
    const upd_s: []u8 = upd[0..];
    fillBgpMarker(upd_s);
    upd[16] = 0; upd[17] = 33; upd[18] = 2; // UPDATE
    upd[19] = 0; upd[20] = 2; // withdrawn_len = 2
    upd[21] = 8; upd[22] = 10; // 10.0.0.0/8
    upd[23] = 0; upd[24] = 4; // total_pa_len = 4
    upd[25] = 0x40; upd[26] = 1; upd[27] = 1; upd[28] = 0; // ORIGIN=IGP
    upd[29] = 24; upd[30] = 192; upd[31] = 0; upd[32] = 2; // 192.0.2.0/24
    if (!validateBgpStrict(upd[0..])) return 11;
    const um = parseBgpMessage(upd[0..]) ?? return 12;
    const ub = parseBgpUpdate(um.body) ?? return 13;
    let nit = BgpPrefixIter.init(ub.nlri);
    const pfx = nit.next() ?? return 14;
    if (pfx.prefix_len != 24 || pfx.addr[0] != 192 || pfx.addr[2] != 2) return 15;
    let pait = BgpPathAttrIter.init(ub.path_attrs);
    const pa = pait.next() ?? return 16;
    if (!pa.isTransitive() || pa.type_code != 1) return 17;

    // Reject UPDATE with NLRI prefix > 32
    let upd_bad_pfx: [33]u8 = upd;
    upd_bad_pfx[29] = 33;
    if (validateBgpStrict(upd_bad_pfx[0..])) return 18;

    // Reject UPDATE with withdrawn_len overflow/OOB
    let upd_bad_wlen: [33]u8 = upd;
    upd_bad_wlen[19] = 0xFF; upd_bad_wlen[20] = 0xFF;
    if (validateBgpStrict(upd_bad_wlen[0..])) return 19;

    // 4. Valid NOTIFICATION (21B)
    let notif: [21]u8 = [0; 21];
    const notif_s: []u8 = notif[0..];
    fillBgpMarker(notif_s);
    notif[16] = 0; notif[17] = 21; notif[18] = 3;
    notif[19] = 6; notif[20] = 2; // Cease / Admin Shutdown
    if (!validateBgpStrict(notif[0..])) return 20;
    const nm = parseBgpMessage(notif[0..]) ?? return 21;
    const nb = parseBgpNotification(nm.body) ?? return 22;
    if (nb.error_code != 6 || nb.error_subcode != 2) return 23;

    // Reject invalid length > 4096 and type outside 1..=4 in validateBgpStrict
    let rr: [23]u8 = [0; 23];
    const rr_s: []u8 = rr[0..];
    fillBgpMarker(rr_s);
    rr[16] = 0; rr[17] = 23; rr[18] = 5; // route_refresh (outside RFC 4271 1..=4)
    if (validateBgpStrict(rr[0..])) return 24;

    // 5. BgpStreamIter over concatenated KEEPALIVE + NOTIFICATION
    let stream: [40]u8 = [0; 40];
    const st_a: []u8 = stream[0..19];
    const st_b: []u8 = stream[19..40];
    * = simdNetCopyBytes16(st_a, ka[0..]);
    * = simdNetCopyBytes16(st_b, notif[0..]);
    let sit = BgpStreamIter.init(stream[0..]);
    const m1 = sit.next() ?? return 25;
    const m2 = sit.next() ?? return 26;
    if (m1.header.msg_type != BgpMsgType.keepalive || m2.header.msg_type != BgpMsgType.notification) return 27;
    if (sit.next() != null || sit.had_error) return 28;

    return 0;
}

fn checkSimdParity(): u32 {
    // 1. simdBgpIsAllFF16 vs scalarBgpIsAllFF16
    let mk: [20]u8 = [0xFF; 20];
    let slen: usize = 0;
    while (slen <= 20) : (slen += 1) {
        if (simdBgpIsAllFF16(mk[0..slen]) != scalarBgpIsAllFF16(mk[0..slen])) return 1;
    }
    let pos: usize = 0;
    while (pos < 16) : (pos += 1) {
        let bv: usize = 0;
        while (bv < 256) : (bv += 1) {
            mk[pos] = @as(u8, @intCast(bv));
            if (simdBgpIsAllFF16(mk[0..16]) != scalarBgpIsAllFF16(mk[0..16])) return 2;
        }
        mk[pos] = 0xFF;
    }

    // 2. simdNetCopyBytes16 vs scalarNetCopyBytes16 across lengths 0..=257
    let src: [257]u8 = [0; 257];
    let dst_simd: [257]u8 = [0; 257];
    let dst_scal: [257]u8 = [0; 257];
    let i: usize = 0;
    while (i < 257) : (i += 1) {
        src[i] = @as(u8, @intCast((i * 37 + 11) & 0xFF));
    }

    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        const s_dst: []u8 = dst_simd[0..len];
        const c_dst: []u8 = dst_scal[0..len];
        const n1 = simdNetCopyBytes16(s_dst, src[0..len]);
        const n2 = scalarNetCopyBytes16(c_dst, src[0..len]);
        if (n1 != n2 || n1 != len) return 3;
        let k: usize = 0;
        while (k < len) : (k += 1) {
            if (dst_simd[k] != dst_scal[k] || dst_simd[k] != src[k]) return 4;
        }
    }
    return 0;
}

fn checkPrngFuzz1600(): u32 {
    let rng: u64 = 0x3609_BEEF_1234_5678;

    // 1. 800 DHCP fuzz cases
    let i: usize = 0;
    while (i < 800) : (i += 1) {
        let buf: [320]u8 = [0; 320];
        rng = prngNext(rng);
        const len = @as(usize, @intCast(rng % 320));
        let k: usize = 0;
        while (k < len) : (k += 1) {
            rng = prngNext(rng);
            buf[k] = @as(u8, @intCast(rng & 0xFF));
        }
        // Half of cases inject valid header + Magic Cookie to exercise TLV parser deeply
        if (len >= DHCP_MIN_PACKET_SIZE && (i % 2 == 0)) {
            buf[0] = DHCP_OP_REQUEST;
            buf[2] = 6;
            buf[236] = 0x63; buf[237] = 0x82; buf[238] = 0x53; buf[239] = 0x63;
        }
        const ok = validateDhcpStrict(buf[0..len]);
        if (ok && len < DHCP_MIN_PACKET_SIZE) return 1;
    }

    // 2. 800 BGP fuzz cases
    let j: usize = 0;
    while (j < 800) : (j += 1) {
        let buf: [256]u8 = [0; 256];
        rng = prngNext(rng);
        const len = @as(usize, @intCast(rng % 256));
        let k: usize = 0;
        while (k < len) : (k += 1) {
            rng = prngNext(rng);
            buf[k] = @as(u8, @intCast(rng & 0xFF));
        }
        // Half of cases inject valid 16B 0xFF Marker & bounded length to exercise OPEN/UPDATE/NOTIFICATION deeply
        if (len >= BGP_HEADER_SIZE && (j % 2 == 0)) {
            let m: usize = 0;
            while (m < BGP_MARKER_LEN) : (m += 1) {
                buf[m] = 0xFF;
            }
            buf[16] = @as(u8, @intCast((len >> 8) & 0xFF));
            buf[17] = @as(u8, @intCast(len & 0xFF));
            buf[18] = @as(u8, @intCast((j % 4) + 1));
        }
        const ok = validateBgpStrict(buf[0..len]);
        if (ok && len < BGP_HEADER_SIZE) return 2;
    }

    return 0;
}

fn main() {
    test("w36r9_dhcp_vectors", () => {
        expect_eq(checkDhcpVectors(), @as(u32, 0));
    });
    test("w36r9_bgp_vectors", () => {
        expect_eq(checkBgpVectors(), @as(u32, 0));
    });
    test("w36r9_simd_parity", () => {
        expect_eq(checkSimdParity(), @as(u32, 0));
    });
    test("w36r9_prng_fuzz_1600", () => {
        expect_eq(checkPrngFuzz1600(), @as(u32, 0));
    });
}
