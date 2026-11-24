// Wave 36 R10 Reference-Hardened Regression & Fuzz Suite
// Covers lib/net/{icmp,arp}.cp against smoltcp-rs/smoltcp (0BSD) & RFC 792/4443/826

import { test, expect_eq } from "cap:test";
import {
    ICMP_HEADER_SIZE,
    ICMP4_DEST_UNREACH,
    ICMP4_ECHO_REQUEST,
    ICMP4_TIME_EXCEEDED,
    scalarIcmpChecksumFold,
    simdIcmpChecksumFold,
    icmp4Checksum,
    verifyIcmp4Checksum,
    verifyIcmp6Checksum,
    validateIcmpStrict,
    parseIcmp,
    encodeIcmp4,
    encodeIcmp4Echo,
    encodeIcmp6Echo,
} from "../../lib/net/icmp.cp";
import {
    ARP_IPV4_PACKET_SIZE,
    scalarArpBytesEqual,
    simdArpBytesEqual,
    ArpOp,
    isArpIpv4,
    validateArpStrict,
    parseArpIpv4,
    parseArpRaw,
    encodeArpRequest,
    encodeArpReply,
    encodeArpProbe,
    encodeGratuitousArp,
} from "../../lib/net/arp.cp";

fn prngNext(state: u64): u64 {
    let z = state +% 0x6E37_79B9_7F4A_7C15;
    z = (z ^ (z >> 30)) *% 0x3F58_476D_1CE4_E5B9;
    z = (z ^ (z >> 27)) *% 0x14D0_49BB_1331_11EB;
    return z ^ (z >> 31);
}

fn checkRfc1071AndIcmpVectors(): u32 {
    // 1. RFC 1071 Section 3 official test vector: sum = 0xDDF2, ~sum = 0x220D
    const rfc_vec: [8]u8 = [0x00, 0x01, 0xF2, 0x03, 0xF4, 0xF5, 0xF6, 0xF7];
    if (scalarIcmpChecksumFold(0, rfc_vec[0..]) != 0xDDF2) return 1;
    if (simdIcmpChecksumFold(0, rfc_vec[0..]) != 0xDDF2) return 2;

    // 2. All-0xFF even (64B) & odd (65B) carry-folding without overflow
    const ff65: [65]u8 = [0xFF; 65];
    if (simdIcmpChecksumFold(0, ff65[0..64]) != 0xFFFF) return 3;
    if (simdIcmpChecksumFold(0, ff65[0..65]) != scalarIcmpChecksumFold(0, ff65[0..65])) return 4;

    // 3. ICMPv4 Echo Request (odd payload len 19) encode -> parse -> verify -> single-bit tamper
    let buf: [128]u8 = [0; 128];
    const out_s: []u8 = buf[0..];
    const payload: []const u8 = "0123456789abcdefXYZ";
    const n = encodeIcmp4Echo(false, 0x1234, 0x0007, payload, out_s) ?? return 5;
    if (n != ICMP_HEADER_SIZE + payload.len) return 6;
    if (!verifyIcmp4Checksum(buf[0..n])) return 7;
    if (!validateIcmpStrict(buf[0..n])) return 8;
    const pkt = parseIcmp(buf[0..n]) ?? return 9;
    if (!pkt.isEchoRequest4() || pkt.echoId() != 0x1234 || pkt.echoSeq() != 7) return 10;

    // Every single-bit flip in packet must invalidate checksum & strict validator
    let i: usize = 0;
    while (i < n) : (i += 1) {
        let mut_b: [128]u8 = buf;
        mut_b[i] = mut_b[i] ^ 0x01;
        if (verifyIcmp4Checksum(mut_b[0..n])) return 11;
        if (validateIcmpStrict(mut_b[0..n])) return 12;
    }

    // 4. Reject < 8B, Echo with non-zero code, DestUnreach/TimeExceeded with payload < 28B
    if (validateIcmpStrict(buf[0..7])) return 13;
    let bad_echo: [16]u8 = [0; 16];
    const bad_echo_s: []u8 = bad_echo[0..];
    const bn = encodeIcmp4(ICMP4_ECHO_REQUEST, 1, 0, "abcd", bad_echo_s) ?? return 14;
    if (validateIcmpStrict(bad_echo[0..bn])) return 15;

    let du_short: [40]u8 = [0; 40];
    const du_short_s: []u8 = du_short[0..];
    const short_pl: [27]u8 = [0x45; 27];
    const dn1 = encodeIcmp4(ICMP4_DEST_UNREACH, 3, 0, short_pl[0..], du_short_s) ?? return 16;
    if (validateIcmpStrict(du_short[0..dn1])) return 17;

    let du_ok: [40]u8 = [0; 40];
    const du_ok_s: []u8 = du_ok[0..];
    const ok_pl: [28]u8 = [0x45; 28];
    const dn2 = encodeIcmp4(ICMP4_TIME_EXCEEDED, 0, 0, ok_pl[0..], du_ok_s) ?? return 18;
    if (!validateIcmpStrict(du_ok[0..dn2])) return 19;

    // 5. ICMPv6 Echo Request with 40B IPv6 pseudo-header
    const src6: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    let dst6: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2];
    let v6buf: [64]u8 = [0; 64];
    const v6s: []u8 = v6buf[0..];
    const v6n = encodeIcmp6Echo(src6, dst6, false, 42, 9, "hello-v6!", v6s) ?? return 20;
    if (!verifyIcmp6Checksum(src6, dst6, v6buf[0..v6n])) return 21;
    dst6[15] = 3;
    if (verifyIcmp6Checksum(src6, dst6, v6buf[0..v6n])) return 22;

    if (icmp4Checksum(buf[0..n]) != pkt.checksum) return 23;
    return 0;
}

fn checkArpVectors(): u32 {
    const smac: [6]u8 = [0x00, 0x11, 0x22, 0x33, 0x44, 0x55];
    const tmac: [6]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF];
    const sip: [4]u8 = [192, 168, 1, 10];
    const tip: [4]u8 = [192, 168, 1, 1];

    // 1. ARP Request round-trip
    let buf: [64]u8 = [0; 64];
    const n1 = encodeArpRequest(smac, sip, tip, buf[0..]) ?? return 1;
    if (n1 != ARP_IPV4_PACKET_SIZE) return 2;
    if (!isArpIpv4(buf[0..n1]) || !validateArpStrict(buf[0..n1])) return 3;
    const p1 = parseArpIpv4(buf[0..n1]) ?? return 4;
    if (!p1.isRequest() || p1.isReply() || p1.isProbe() || p1.isGratuitous()) return 5;
    if (!simdArpBytesEqual(p1.sender_mac[0..], smac[0..])) return 6;

    // 2. ARP Reply round-trip
    const n2 = encodeArpReply(smac, sip, tmac, tip, buf[0..]) ?? return 7;
    if (!validateArpStrict(buf[0..n2])) return 8;
    const p2 = parseArpIpv4(buf[0..n2]) ?? return 9;
    if (!p2.isReply() || !simdArpBytesEqual(p2.target_mac[0..], tmac[0..])) return 10;

    // 3. ARP Probe & Gratuitous ARP (RFC 5227)
    const n3 = encodeArpProbe(smac, tip, buf[0..]) ?? return 11;
    const p3 = parseArpIpv4(buf[0..n3]) ?? return 12;
    if (!p3.isProbe() || !validateArpStrict(buf[0..n3])) return 13;

    const n4 = encodeGratuitousArp(smac, sip, buf[0..]) ?? return 14;
    const p4 = parseArpIpv4(buf[0..n4]) ?? return 15;
    if (!p4.isGratuitous() || !p4.isAnnouncement() || !validateArpStrict(buf[0..n4])) return 16;

    // 4. Generic variable-length ARP (htype=6, ptype=0x0800, hlen=8, plen=4, oper=1 -> 32B)
    let raw: [32]u8 = [0; 32];
    raw[0] = 0; raw[1] = 6;
    raw[2] = 0x08; raw[3] = 0x00;
    raw[4] = 8; raw[5] = 4;
    raw[6] = 0; raw[7] = 1;
    if (!validateArpStrict(raw[0..32])) return 17;
    if (validateArpStrict(raw[0..31])) return 18; // truncated by 1 byte
    const pr = parseArpRaw(raw[0..32]) ?? return 19;
    if (pr.op != ArpOp.request || pr.sha.len != 8 || pr.tpa.len != 4) return 20;

    // 5. Reject malformed frames: < 8B, < 28B for Eth+IPv4, oper=3, hlen=0, hlen=255 overflow
    if (validateArpStrict(buf[0..7])) return 21;
    if (validateArpStrict(buf[0..27])) return 22;
    let bad_op: [28]u8 = [0; 28];
    let k: usize = 0;
    while (k < 28) : (k += 1) { bad_op[k] = buf[k]; }
    bad_op[7] = 3; // RARP, not RFC 826 oper 1|2
    if (validateArpStrict(bad_op[0..])) return 23;

    bad_op[7] = 1;
    bad_op[4] = 0; // hlen == 0
    if (validateArpStrict(bad_op[0..])) return 24;

    bad_op[4] = 255; bad_op[5] = 255; // 8 + 510 + 510 = 1028 > 28
    if (validateArpStrict(bad_op[0..])) return 25;
    if (parseArpRaw(bad_op[0..]) != null) return 26;

    return 0;
}

fn checkSimdParity(): u32 {
    // 1. simdIcmpChecksumFold vs scalarIcmpChecksumFold across lengths 0..=257
    let data: [257]u8 = [0; 257];
    let i: usize = 0;
    while (i < 257) : (i += 1) {
        data[i] = @as(u8, @intCast((i * 53 + 17) & 0xFF));
    }
    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        const s1 = simdIcmpChecksumFold(0, data[0..len]);
        const c1 = scalarIcmpChecksumFold(0, data[0..len]);
        if (s1 != c1) return 1;
        const s2 = simdIcmpChecksumFold(0x12345, data[0..len]);
        const c2 = scalarIcmpChecksumFold(0x12345, data[0..len]);
        if (s2 != c2) return 2;
    }

    // 2. simdArpBytesEqual vs scalarArpBytesEqual across all 256 byte values & lengths 0..=257
    let a1: [1]u8 = [0];
    let b1: [1]u8 = [0];
    let bv: usize = 0;
    while (bv < 256) : (bv += 1) {
        a1[0] = @as(u8, @intCast(bv));
        b1[0] = a1[0] ^ 0x01;
        if (simdArpBytesEqual(a1[0..], a1[0..]) != scalarArpBytesEqual(a1[0..], a1[0..])) return 3;
        if (simdArpBytesEqual(a1[0..], b1[0..]) != scalarArpBytesEqual(a1[0..], b1[0..])) return 4;
    }

    let copy_buf: [257]u8 = data;
    len = 0;
    while (len <= 257) : (len += 1) {
        if (simdArpBytesEqual(data[0..len], copy_buf[0..len]) != scalarArpBytesEqual(data[0..len], copy_buf[0..len])) return 5;
        if (len > 0) {
            copy_buf[len - 1] = copy_buf[len - 1] ^ 0x80;
            if (simdArpBytesEqual(data[0..len], copy_buf[0..len]) != scalarArpBytesEqual(data[0..len], copy_buf[0..len])) return 6;
            copy_buf[len - 1] = data[len - 1];
        }
    }
    if (simdArpBytesEqual(data[0..28], copy_buf[0..27])) return 7;
    return 0;
}

fn checkPrngFuzz1600(): u32 {
    let rng: u64 = 0x3610_CAFE_1234_5678;

    // 1. 800 ICMP fuzz cases
    let i: usize = 0;
    while (i < 800) : (i += 1) {
        let buf: [160]u8 = [0; 160];
        rng = prngNext(rng);
        const len = @as(usize, @intCast(rng % 160));
        let k: usize = 0;
        while (k < len) : (k += 1) {
            rng = prngNext(rng);
            buf[k] = @as(u8, @intCast(rng & 0xFF));
        }
        // Half of cases patch valid RFC 1071 checksum to exercise type/code/payload guards deeply
        if (len >= ICMP_HEADER_SIZE && (i % 2 == 0)) {
            const csum = icmp4Checksum(buf[0..len]);
            buf[2] = @as(u8, @intCast((csum >> 8) & 0xFF));
            buf[3] = @as(u8, @intCast(csum & 0xFF));
        }
        const ok = validateIcmpStrict(buf[0..len]);
        if (ok && len < ICMP_HEADER_SIZE) return 1;
        if (simdIcmpChecksumFold(0, buf[0..len]) != scalarIcmpChecksumFold(0, buf[0..len])) return 2;
    }

    // 2. 800 ARP fuzz cases
    let j: usize = 0;
    while (j < 800) : (j += 1) {
        let buf: [96]u8 = [0; 96];
        rng = prngNext(rng);
        const len = @as(usize, @intCast(rng % 96));
        let k: usize = 0;
        while (k < len) : (k += 1) {
            rng = prngNext(rng);
            buf[k] = @as(u8, @intCast(rng & 0xFF));
        }
        if (len >= 8 && (j % 2 == 0)) {
            buf[0] = 0; buf[1] = 1;
            buf[2] = 0x08; buf[3] = 0x00;
            buf[6] = 0; buf[7] = @as(u8, @intCast((j % 2) + 1));
        }
        const ok = validateArpStrict(buf[0..len]);
        const raw_opt = parseArpRaw(buf[0..len]);
        if (ok && raw_opt == null) return 3;
    }

    return 0;
}

fn main() {
    test("w36r10_rfc1071_icmp_vectors", () => {
        expect_eq(checkRfc1071AndIcmpVectors(), @as(u32, 0));
    });
    test("w36r10_arp_vectors", () => {
        expect_eq(checkArpVectors(), @as(u32, 0));
    });
    test("w36r10_simd_parity", () => {
        expect_eq(checkSimdParity(), @as(u32, 0));
    });
    test("w36r10_prng_fuzz_1600", () => {
        expect_eq(checkPrngFuzz1600(), @as(u32, 0));
    });
}
