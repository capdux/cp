// Source: cp/cap/test/test_vxlan_production_deep.cp (Production deep tests for cap:vxlan)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VXLAN_HEADER_SIZE,
    VXLAN_DEFAULT_PORT,
    VXLAN_GPE_PORT,
    VXLAN_MAX_VNI,
    VXLAN_FLAG_I,
    VXLAN_GPE_FLAG_P,
    VXLAN_GPE_FLAG_O,
    VXLAN_GBP_FLAG_G,
    VxlanNextProto,
    vxlanNextProtoFromByte,
    VxlanHeader,
    VxlanPacket,
    vxlanVerifyHeader,
    parseVxlan,
    parseVxlanStrict,
    parseVxlanGpe,
    encodeVxlan,
    encodeVxlanGpe,
    encodeVxlanGbp,
} from "cap:vxlan";

test("1. Classic RFC 7348 VXLAN 24-bit VNI (0, 100, 16777215) roundtrip and zero-copy Ethernet payload", () => {
    expect_eq(VXLAN_HEADER_SIZE, 8);
    expect_eq(VXLAN_DEFAULT_PORT, 4789 as u16);
    expect_eq(VXLAN_GPE_PORT, 4790 as u16);
    expect_eq(VXLAN_MAX_VNI, 16777215 as u32);

    // Sample 14B Ethernet header + 4B payload
    const inner_eth: [18]u8 = [
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // dst MAC
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, // src MAC
        0x08, 0x00,                         // EtherType IPv4
        0x45, 0x00, 0x00, 0x14,             // IPv4 head bytes
    ];

    const vnis: [3]u32 = [0, 100, VXLAN_MAX_VNI];
    let idx: usize = 0;
    while (idx < 3) {
        const vni = vnis[idx];
        let out: [64]u8 = std.mem.zeroes([64]u8);
        const written = encodeVxlan(vni, inner_eth[0..], out[0..]) ?? unreachable;
        expect_eq(written, 8 + inner_eth.len);

        // Wire format: 0x08 00 00 00 <vni3> 00
        expect_eq(out[0], VXLAN_FLAG_I);
        expect_eq(out[1], 0 as u8);
        expect_eq(out[2], 0 as u8);
        expect_eq(out[3], 0 as u8);
        expect_eq(out[7], 0 as u8);
        expect_eq(vxlanVerifyHeader(out[0..written]), true);

        const pkt = parseVxlanStrict(out[0..written]) ?? unreachable;
        expect_eq(pkt.header.vni, vni);
        expect_eq(pkt.hasValidVni(), true);
        expect_eq(pkt.isGpe(), false);
        expect_eq(pkt.isOam(), false);
        expect_eq(pkt.hasGbp(), false);
        expect_eq(pkt.header.next_proto, VxlanNextProto.none);
        expect_eq(pkt.header.group_policy_id, 0 as u16);
        expect_eq(std.mem.eql(u8, pkt.payload, inner_eth[0..]), true);

        idx += 1;
    }
});

test("2. VXLAN-GPE IPv4, IPv6, Ethernet, NSH, MPLS and OAM flag encode/decode", () => {
    let out: [64]u8 = std.mem.zeroes([64]u8);
    const payload = "GPE-INNER";

    // IPv4 non-OAM
    const n1 = encodeVxlanGpe(0x12_3456, VxlanNextProto.ipv4, false, payload, out[0..]) ?? unreachable;
    expect_eq(out[0], (VXLAN_FLAG_I | VXLAN_GPE_FLAG_P) as u8);
    expect_eq(out[3], 1 as u8);
    const p1 = parseVxlanGpe(out[0..n1]) ?? unreachable;
    expect_eq(p1.header.vni, 0x12_3456 as u32);
    expect_eq(p1.header.next_proto, VxlanNextProto.ipv4);
    expect_eq(p1.isGpe(), true);
    expect_eq(p1.isOam(), false);
    expect_eq(std.mem.eql(u8, p1.payload, payload), true);

    // IPv6 with OAM bit set
    const n2 = encodeVxlanGpe(4096, VxlanNextProto.ipv6, true, payload, out[0..]) ?? unreachable;
    expect_eq(out[0], (VXLAN_FLAG_I | VXLAN_GPE_FLAG_P | VXLAN_GPE_FLAG_O) as u8);
    const p2 = parseVxlanGpe(out[0..n2]) ?? unreachable;
    expect_eq(p2.header.next_proto, VxlanNextProto.ipv6);
    expect_eq(p2.isOam(), true);

    // Ethernet, NSH, MPLS
    const n3 = encodeVxlanGpe(7, VxlanNextProto.ethernet, false, payload, out[0..]) ?? unreachable;
    const p3 = parseVxlanGpe(out[0..n3]) ?? unreachable;
    expect_eq(p3.header.next_proto, VxlanNextProto.ethernet);

    const n4 = encodeVxlanGpe(8, VxlanNextProto.nsh, false, payload, out[0..]) ?? unreachable;
    const p4 = parseVxlanGpe(out[0..n4]) ?? unreachable;
    expect_eq(p4.header.next_proto, VxlanNextProto.nsh);

    const n5 = encodeVxlanGpe(9, VxlanNextProto.mpls, false, payload, out[0..]) ?? unreachable;
    const p5 = parseVxlanGpe(out[0..n5]) ?? unreachable;
    expect_eq(p5.header.next_proto, VxlanNextProto.mpls);

    // Unknown Next Protocol byte maps cleanly to VxlanNextProto.unknown
    expect_eq(vxlanNextProtoFromByte(99), VxlanNextProto.unknown);
});

test("3. VXLAN-GBP Group Policy ID extension encoding and lenient parsing", () => {
    let out: [64]u8 = std.mem.zeroes([64]u8);
    const inner = "GBP-FRAME";
    const n = encodeVxlanGbp(0x00AB_CDEF, 0x1A2B, inner, out[0..]) ?? unreachable;
    expect_eq(n, 8 + inner.len);
    expect_eq(out[0], (VXLAN_GBP_FLAG_G | VXLAN_FLAG_I) as u8);
    expect_eq(out[2], 0x1A as u8);
    expect_eq(out[3], 0x2B as u8);

    // Lenient parseVxlan decodes GBP ID
    const pkt = parseVxlan(out[0..n]) ?? unreachable;
    expect_eq(pkt.hasValidVni(), true);
    expect_eq(pkt.hasGbp(), true);
    expect_eq(pkt.isGpe(), false);
    expect_eq(pkt.header.vni, 0x00AB_CDEF as u32);
    expect_eq(pkt.header.group_policy_id, 0x1A2B as u16);
    expect_eq(std.mem.eql(u8, pkt.payload, inner), true);

    // Strict classic VXLAN rejects GBP header (non-zero G flag and non-zero bytes 2..4)
    expect_eq(parseVxlanStrict(out[0..n]) == null, true);
    // GPE parser rejects GBP header (missing P bit)
    expect_eq(parseVxlanGpe(out[0..n]) == null, true);
});

test("4. Strict mode rejects non-zero reserved bits/bytes and missing I-bit", () => {
    // Missing I-bit (flags = 0x00)
    const no_i: [8]u8 = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00];
    expect_eq(vxlanVerifyHeader(no_i[0..]), false);
    expect_eq(parseVxlan(no_i[0..]) == null, true);
    expect_eq(parseVxlanStrict(no_i[0..]) == null, true);
    expect_eq(parseVxlanGpe(no_i[0..]) == null, true);

    // Non-zero reserved byte 1
    const bad_r1: [8]u8 = [VXLAN_FLAG_I, 0x01, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00];
    expect_eq(parseVxlanStrict(bad_r1[0..]) == null, true);
    // Lenient accepts it
    const ok_lenient = parseVxlan(bad_r1[0..]) ?? unreachable;
    expect_eq(ok_lenient.header.vni, 100 as u32);

    // Non-zero reserved byte 7
    const bad_r7: [8]u8 = [VXLAN_FLAG_I, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0xFF];
    expect_eq(parseVxlanStrict(bad_r7[0..]) == null, true);

    // GPE with non-zero reserved version bit (0x10)
    const bad_gpe_ver: [8]u8 = [VXLAN_FLAG_I | VXLAN_GPE_FLAG_P | 0x10, 0, 0, 1, 0, 0, 1, 0];
    expect_eq(parseVxlanGpe(bad_gpe_ver[0..]) == null, true);
});

test("5. Out-of-range VNI (> 24-bit), truncated input, and insufficient buffer resilience", () => {
    let out: [32]u8 = std.mem.zeroes([32]u8);

    // VNI > 0x00FFFFFF must fail encoding
    expect_eq(encodeVxlan(0x0100_0000, "abc", out[0..]) == null, true);
    expect_eq(encodeVxlanGpe(0xFFFF_FFFF, VxlanNextProto.ipv4, false, "abc", out[0..]) == null, true);
    expect_eq(encodeVxlanGbp(0x0100_0001, 10, "abc", out[0..]) == null, true);

    // Insufficient output buffer
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(encodeVxlan(100, "1234", tiny[0..]) == null, true); // needs 12 > 10
    expect_eq(encodeVxlanGpe(100, VxlanNextProto.ipv4, false, "1234", tiny[0..]) == null, true);
    expect_eq(encodeVxlanGbp(100, 1, "1234", tiny[0..]) == null, true);
    // Exact fit (8 + 2 = 10) succeeds
    expect_eq(encodeVxlan(100, "OK", tiny[0..]) ?? 0, 10);

    // Truncated input (< 8 bytes)
    const trunc: [7]u8 = [VXLAN_FLAG_I, 0, 0, 0, 0, 0, 1];
    expect_eq(vxlanVerifyHeader(trunc[0..]), false);
    expect_eq(parseVxlan(trunc[0..]) == null, true);
    expect_eq(parseVxlanStrict(trunc[0..]) == null, true);
    expect_eq(parseVxlanGpe(trunc[0..]) == null, true);

    // Empty payload (8 bytes exact) is valid
    const empty_n = encodeVxlan(42, "", out[0..]) ?? unreachable;
    expect_eq(empty_n, 8);
    const p_empty = parseVxlanStrict(out[0..empty_n]) ?? unreachable;
    expect_eq(p_empty.header.vni, 42 as u32);
    expect_eq(p_empty.payload.len, 0);
});
