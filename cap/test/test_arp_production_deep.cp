// Source: cp/cap/test/test_arp_production_deep.cp (Production deep tests for cap:arp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ARP_ETHERTYPE,
    ARP_MIN_HEADER_SIZE,
    ARP_IPV4_PACKET_SIZE,
    ARP_HTYPE_ETHERNET,
    ARP_PTYPE_IPV4,
    ARP_HLEN_ETHERNET,
    ARP_PLEN_IPV4,
    ArpOp,
    arpOpFromU16,
    ArpPacket,
    ArpRawHeader,
    isArpIpv4,
    parseArpIpv4,
    parseArpRaw,
    encodeArpIpv4,
    encodeArpRequest,
    encodeArpReply,
    encodeArpProbe,
    encodeGratuitousArp,
} from "cap:net";

test("1. ARP Request and Reply 28-byte encode/decode roundtrip", () => {
    expect_eq(ARP_ETHERTYPE, 0x0806 as u16);
    expect_eq(ARP_IPV4_PACKET_SIZE, 28);
    expect_eq(ARP_MIN_HEADER_SIZE, 8);

    const smac: [6]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01];
    const sip: [4]u8 = [10, 0, 0, 5];
    const tmac: [6]u8 = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66];
    const tip: [4]u8 = [10, 0, 0, 1];

    // Request roundtrip
    let req_buf: [28]u8 = std.mem.zeroes([28]u8);
    const req_n = encodeArpRequest(smac, sip, tip, req_buf[0..]) ?? unreachable;
    expect_eq(req_n, 28);
    expect_eq(isArpIpv4(req_buf[0..]), true);

    const req = parseArpIpv4(req_buf[0..]) ?? unreachable;
    expect_eq(req.htype, ARP_HTYPE_ETHERNET);
    expect_eq(req.ptype, ARP_PTYPE_IPV4);
    expect_eq(req.hlen, ARP_HLEN_ETHERNET);
    expect_eq(req.plen, ARP_PLEN_IPV4);
    expect_eq(req.op, ArpOp.request);
    expect_eq(req.raw_op, 1 as u16);
    expect_eq(req.isRequest(), true);
    expect_eq(req.isReply(), false);
    expect_eq(req.isProbe(), false);
    expect_eq(req.isGratuitous(), false);
    expect_eq(req.isAnnouncement(), false);
    expect_eq(std.mem.eql(u8, req.sender_mac[0..], smac[0..]), true);
    expect_eq(std.mem.eql(u8, req.sender_ip[0..], sip[0..]), true);
    expect_eq(req.target_mac[0], 0 as u8);
    expect_eq(std.mem.eql(u8, req.target_ip[0..], tip[0..]), true);

    // Reply roundtrip
    let rep_buf: [28]u8 = std.mem.zeroes([28]u8);
    const rep_n = encodeArpReply(tmac, tip, smac, sip, rep_buf[0..]) ?? unreachable;
    expect_eq(rep_n, 28);

    const rep = parseArpIpv4(rep_buf[0..]) ?? unreachable;
    expect_eq(rep.op, ArpOp.reply);
    expect_eq(rep.isReply(), true);
    expect_eq(rep.isRequest(), false);
    expect_eq(std.mem.eql(u8, rep.sender_mac[0..], tmac[0..]), true);
    expect_eq(std.mem.eql(u8, rep.sender_ip[0..], tip[0..]), true);
    expect_eq(std.mem.eql(u8, rep.target_mac[0..], smac[0..]), true);
    expect_eq(std.mem.eql(u8, rep.target_ip[0..], sip[0..]), true);
});

test("2. RFC 5227 ARP Probe (0.0.0.0) and Gratuitous ARP / Announcement detection", () => {
    const mac: [6]u8 = [0x02, 0x42, 0xAC, 0x11, 0x00, 0x02];
    const ip: [4]u8 = [172, 17, 0, 2];

    // ARP Probe
    let probe_buf: [28]u8 = std.mem.zeroes([28]u8);
    const pn = encodeArpProbe(mac, ip, probe_buf[0..]) ?? unreachable;
    expect_eq(pn, 28);
    const probe = parseArpIpv4(probe_buf[0..]) ?? unreachable;
    expect_eq(probe.isRequest(), true);
    expect_eq(probe.isProbe(), true);
    expect_eq(probe.isGratuitous(), false);
    expect_eq(probe.isAnnouncement(), false);
    expect_eq(probe.sender_ip[0], 0 as u8);
    expect_eq(std.mem.eql(u8, probe.target_ip[0..], ip[0..]), true);

    // Gratuitous ARP Announcement (request, sender_ip == target_ip != 0.0.0.0)
    let garp_buf: [28]u8 = std.mem.zeroes([28]u8);
    const gn = encodeGratuitousArp(mac, ip, garp_buf[0..]) ?? unreachable;
    expect_eq(gn, 28);
    const garp = parseArpIpv4(garp_buf[0..]) ?? unreachable;
    expect_eq(garp.isRequest(), true);
    expect_eq(garp.isProbe(), false);
    expect_eq(garp.isGratuitous(), true);
    expect_eq(garp.isAnnouncement(), true);
    expect_eq(std.mem.eql(u8, garp.sender_ip[0..], ip[0..]), true);
    expect_eq(std.mem.eql(u8, garp.target_ip[0..], ip[0..]), true);

    // Gratuitous ARP Reply (reply, sender_ip == target_ip != 0.0.0.0)
    const bcast_mac: [6]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF];
    let grep_buf: [28]u8 = std.mem.zeroes([28]u8);
    * = encodeArpIpv4(ArpOp.reply, mac, ip, bcast_mac, ip, grep_buf[0..]) ?? unreachable;
    const grep = parseArpIpv4(grep_buf[0..]) ?? unreachable;
    expect_eq(grep.isReply(), true);
    expect_eq(grep.isGratuitous(), true);
    expect_eq(grep.isAnnouncement(), false);
});

test("3. Generic variable-length parseArpRaw (InfiniBand 20B hw / IPv6 16B proto style)", () => {
    // Custom htype=32 (InfiniBand), ptype=0x86DD, hlen=8, plen=16 -> total = 8 + 16 + 32 = 56B
    let raw: [56]u8 = std.mem.zeroes([56]u8);
    raw[0] = 0x00;
    raw[1] = 32;   // htype = 32
    raw[2] = 0x86;
    raw[3] = 0xDD; // ptype = 0x86DD
    raw[4] = 8;    // hlen = 8
    raw[5] = 16;   // plen = 16
    raw[6] = 0x00;
    raw[7] = 0x01; // request
    raw[8] = 0xAA;  // sha[0]
    raw[15] = 0xBB; // sha[7]
    raw[16] = 0xFE; // spa[0]
    raw[31] = 0x01; // spa[15]
    raw[32] = 0xCC; // tha[0]
    raw[40] = 0xFE; // tpa[0]
    raw[55] = 0x02; // tpa[15]

    // Not Ethernet/IPv4
    expect_eq(isArpIpv4(raw[0..]), false);
    expect_eq(parseArpIpv4(raw[0..]) == null, true);

    const parsed = parseArpRaw(raw[0..]) ?? unreachable;
    expect_eq(parsed.htype, 32 as u16);
    expect_eq(parsed.ptype, 0x86DD as u16);
    expect_eq(parsed.hlen, 8 as u8);
    expect_eq(parsed.plen, 16 as u8);
    expect_eq(parsed.op, ArpOp.request);
    expect_eq(parsed.sha.len, 8);
    expect_eq(parsed.spa.len, 16);
    expect_eq(parsed.tha.len, 8);
    expect_eq(parsed.tpa.len, 16);
    expect_eq(parsed.sha[0], 0xAA as u8);
    expect_eq(parsed.sha[7], 0xBB as u8);
    expect_eq(parsed.spa[0], 0xFE as u8);
    expect_eq(parsed.spa[15], 0x01 as u8);
    expect_eq(parsed.tha[0], 0xCC as u8);
    expect_eq(parsed.tpa[15], 0x02 as u8);
});

test("4. RARP, InARP, and unknown opcode decoding", () => {
    expect_eq(arpOpFromU16(3), ArpOp.rarp_request);
    expect_eq(arpOpFromU16(4), ArpOp.rarp_reply);
    expect_eq(arpOpFromU16(8), ArpOp.inarp_request);
    expect_eq(arpOpFromU16(9), ArpOp.inarp_reply);
    expect_eq(arpOpFromU16(999), ArpOp.unknown);

    const mac: [6]u8 = [1, 2, 3, 4, 5, 6];
    const ip: [4]u8 = [192, 168, 0, 1];
    let buf: [28]u8 = std.mem.zeroes([28]u8);

    * = encodeArpIpv4(ArpOp.rarp_request, mac, ip, mac, ip, buf[0..]) ?? unreachable;
    const p_rarp = parseArpIpv4(buf[0..]) ?? unreachable;
    expect_eq(p_rarp.op, ArpOp.rarp_request);
    expect_eq(p_rarp.raw_op, 3 as u16);

    * = encodeArpIpv4(ArpOp.inarp_reply, mac, ip, mac, ip, buf[0..]) ?? unreachable;
    const p_inarp = parseArpIpv4(buf[0..]) ?? unreachable;
    expect_eq(p_inarp.op, ArpOp.inarp_reply);
    expect_eq(p_inarp.raw_op, 9 as u16);

    // Unknown raw opcode 0x0042 preserved in raw_op
    buf[6] = 0x00;
    buf[7] = 0x42;
    const p_unk = parseArpIpv4(buf[0..]) ?? unreachable;
    expect_eq(p_unk.op, ArpOp.unknown);
    expect_eq(p_unk.raw_op, 0x0042 as u16);
});

test("5. Truncated, non-Ethernet, non-IPv4, out-of-bounds raw, and insufficient buffer resilience", () => {
    // Truncated (< 28B) for IPv4 ARP
    const short27: [27]u8 = [0; 27];
    expect_eq(isArpIpv4(short27[0..]), false);
    expect_eq(parseArpIpv4(short27[0..]) == null, true);

    // Truncated (< 8B) for raw ARP
    const short7: [7]u8 = [0; 7];
    expect_eq(parseArpRaw(short7[0..]) == null, true);

    // Raw ARP declaring hlen=6, plen=4 (needs 28B) but only 20B supplied
    const oob_raw: [20]u8 = [
        0x00, 0x01, 0x08, 0x00, 6, 4, 0x00, 0x01,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    expect_eq(parseArpRaw(oob_raw[0..]) == null, true);

    // Wrong htype (2 instead of 1)
    let bad_htype: [28]u8 = std.mem.zeroes([28]u8);
    bad_htype[1] = 2;
    bad_htype[2] = 0x08;
    bad_htype[4] = 6;
    bad_htype[5] = 4;
    expect_eq(parseArpIpv4(bad_htype[0..]) == null, true);

    // Wrong ptype (0x86DD instead of 0x0800)
    let bad_ptype: [28]u8 = std.mem.zeroes([28]u8);
    bad_ptype[1] = 1;
    bad_ptype[2] = 0x86;
    bad_ptype[3] = 0xDD;
    bad_ptype[4] = 6;
    bad_ptype[5] = 4;
    expect_eq(parseArpIpv4(bad_ptype[0..]) == null, true);

    // Insufficient output buffer (< 28B)
    let tiny: [27]u8 = std.mem.zeroes([27]u8);
    const mac: [6]u8 = [0; 6];
    const ip: [4]u8 = [10, 0, 0, 1];
    expect_eq(encodeArpRequest(mac, ip, ip, tiny[0..]) == null, true);
    expect_eq(encodeArpReply(mac, ip, mac, ip, tiny[0..]) == null, true);
    expect_eq(encodeArpProbe(mac, ip, tiny[0..]) == null, true);
    expect_eq(encodeGratuitousArp(mac, ip, tiny[0..]) == null, true);
});
