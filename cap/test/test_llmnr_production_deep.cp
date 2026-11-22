// Source: cp/cap/test/test_llmnr_production_deep.cp (Production deep tests for cap:llmnr)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LLMNR_PORT,
    LLMNR_MULTICAST_V4,
    LLMNR_MULTICAST_V6,
    LLMNR_HEADER_SIZE,
    LLMNR_DEFAULT_TTL,
    LLMNR_CLASS_IN,
    LLMNR_TYPE_A,
    LLMNR_TYPE_PTR,
    LLMNR_TYPE_AAAA,
    LLMNR_TYPE_ANY,
    FLAG_QR,
    FLAG_CONFLICT,
    LLMNR_FLAG_TC,
    FLAG_TENTATIVE,
    LlmnrHeader,
    LlmnrQuestion,
    LlmnrRecord,
    LlmnrRecordIter,
    LlmnrPacket,
    isLlmnr,
    parseLlmnr,
    encodeLlmnrQuery,
    encodeLlmnrResponseA,
    encodeLlmnrResponseAaaa,
} from "cap:iot";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

test("1. Protocol constants & LLMNR Query encode/parse round-trip", () => {
    expect_eq(LLMNR_PORT, 5355 as u16);
    expectBytesEq(LLMNR_MULTICAST_V4, "224.0.0.252");
    expectBytesEq(LLMNR_MULTICAST_V6, "ff02::1:3");
    expect_eq(LLMNR_HEADER_SIZE, 12);
    expect_eq(LLMNR_DEFAULT_TTL, 30 as u32);

    let buf: [128]u8 = [0; 128];
    const n = encodeLlmnrQuery(0xABCD, "nas-box", LLMNR_TYPE_A, buf[0..]) ?? unreachable;
    expect_eq(isLlmnr(buf[0..n]), true);

    const pkt = parseLlmnr(buf[0..n]) ?? unreachable;
    expect_eq(pkt.header.id, 0xABCD as u16);
    expect_eq(pkt.header.isResponse(), false);
    expect_eq(pkt.header.opcode(), 0 as u8);
    expect_eq(pkt.header.qd_count, 1 as u16);
    expect_eq(pkt.header.an_count, 0 as u16);

    const q = pkt.question() ?? unreachable;
    expect_eq(q.qtype, LLMNR_TYPE_A);
    expect_eq(q.qclass, LLMNR_CLASS_IN);

    let name_buf: [64]u8 = [0; 64];
    const name_len = pkt.decodeName(q.name_offset, name_buf[0..]) ?? unreachable;
    expectBytesEq(name_buf[0..name_len], "nas-box");
});

test("2. LLMNR IPv4 A & IPv6 AAAA Response encode/parse round-trip with 0xC0 compression pointer", () => {
    let wire: [128]u8 = [0; 128];
    const ip4: [4]u8 = [169, 254, 10, 20];
    const n4 = encodeLlmnrResponseA(0x1122, "printer.local", ip4, LLMNR_DEFAULT_TTL, wire[0..]) ?? unreachable;

    const pkt4 = parseLlmnr(wire[0..n4]) ?? unreachable;
    expect_eq(pkt4.header.isResponse(), true);
    expect_eq(pkt4.header.an_count, 1 as u16);

    let it4 = pkt4.answers();
    const rr4 = it4.next() ?? unreachable;
    expect_eq(rr4.rtype, LLMNR_TYPE_A);
    expect_eq(rr4.rclass, LLMNR_CLASS_IN);
    expect_eq(rr4.ttl, 30 as u32);
    const decoded_ip4 = rr4.asIpv4() ?? unreachable;
    expect_eq(decoded_ip4[0], 169 as u8);
    expect_eq(decoded_ip4[1], 254 as u8);
    expect_eq(decoded_ip4[2], 10 as u8);
    expect_eq(decoded_ip4[3], 20 as u8);
    expect_eq(rr4.asIpv6() == null, true);
    expect_eq(it4.next() == null, true);
    expect_eq(it4.hasError(), false);

    // Answer uses 0xC00C compression pointer -> decodeName resolves to "printer.local"
    let name_buf: [64]u8 = [0; 64];
    const an_name_len = pkt4.decodeName(rr4.name_offset, name_buf[0..]) ?? unreachable;
    expectBytesEq(name_buf[0..an_name_len], "printer.local");

    // IPv6 AAAA response round-trip
    const ip6: [16]u8 = [
        0xFE, 0x80, 0, 0, 0, 0, 0, 0,
        0x02, 0x11, 0x22, 0xFF, 0xFE, 0x33, 0x44, 0x55,
    ];
    const n6 = encodeLlmnrResponseAaaa(0x3344, "sensor6", ip6, 60, wire[0..]) ?? unreachable;
    const pkt6 = parseLlmnr(wire[0..n6]) ?? unreachable;
    let it6 = pkt6.answers();
    const rr6 = it6.next() ?? unreachable;
    expect_eq(rr6.rtype, LLMNR_TYPE_AAAA);
    expect_eq(rr6.ttl, 60 as u32);
    const decoded_ip6 = rr6.asIpv6() ?? unreachable;
    expectBytesEq(decoded_ip6[0..16], ip6[0..16]);
    expect_eq(rr6.asIpv4() == null, true);
});

test("3. LLMNR Response C (Conflict), TC (Truncated), T (Tentative) flags decoding", () => {
    const resp_flags: u16 = FLAG_QR | FLAG_CONFLICT | LLMNR_FLAG_TC | FLAG_TENTATIVE | 0x0003;
    const raw: [24]u8 = [
        0x55, 0x66,                                 // id
        ((resp_flags >> 8) & 0xFF) as u8,
        (resp_flags & 0xFF) as u8,                  // flags
        0x00, 0x01,                                 // qd_count = 1
        0x00, 0x00,                                 // an_count = 0
        0x00, 0x00,                                 // ns_count = 0
        0x00, 0x00,                                 // ar_count = 0
        0x04, 'h', 'o', 's', 't', 0x00,             // "host"
        0x00, 0x01, 0x00, 0x01,                     // QTYPE=A, QCLASS=IN
        0x00, 0x00,
    ];

    const pkt = parseLlmnr(raw[0..]) ?? unreachable;
    expect_eq(pkt.header.isResponse(), true);
    expect_eq(pkt.header.isConflict(), true);
    expect_eq(pkt.header.isTruncated(), true);
    expect_eq(pkt.header.isTentative(), true);
    expect_eq(pkt.header.rcode(), 3 as u8);
});

test("4. Compression pointer loop protection (self-loop & mutual 2-hop cycle)", () => {
    // Self-loop compression pointer at offset 12 pointing to offset 12 (0xC0 0x0C)
    const self_loop: [18]u8 = [
        0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC0, 0x0C,
        0x00, 0x01, 0x00, 0x01,
    ];
    const pkt1 = parseLlmnr(self_loop[0..]) ?? unreachable;
    let out: [64]u8 = [0; 64];
    expect_eq(pkt1.decodeName(12, out[0..]) == null, true);

    // 2-hop mutual cycle: offset 12 -> 14 (0xC0 0x0E) and offset 14 -> 12 (0xC0 0x0C)
    const cycle: [20]u8 = [
        0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC0, 0x0E,
        0xC0, 0x0C,
        0x00, 0x01, 0x00, 0x01,
    ];
    const pkt2 = parseLlmnr(cycle[0..]) ?? unreachable;
    expect_eq(pkt2.decodeName(12, out[0..]) == null, true);
});

test("5. Strict RFC 4795 rejection: opcode!=0, query with C/TC/T, qd_count!=1, an_count!=0, truncation", () => {
    // Too short (< 12 bytes)
    const short_pkt: [8]u8 = [0; 8];
    expect_eq(isLlmnr(short_pkt[0..]), false);

    // Non-zero OPCODE (opcode = 1 -> flags = 0x0800)
    const bad_op: [12]u8 = [0, 1, 0x08, 0x00, 0, 1, 0, 0, 0, 0, 0, 0];
    expect_eq(parseLlmnr(bad_op[0..]) == null, true);

    // Query with C=1 (0x0400) or TC=1 (0x0200) or T=1 (0x0100)
    const q_conflict: [12]u8 = [0, 1, 0x04, 0x00, 0, 1, 0, 0, 0, 0, 0, 0];
    expect_eq(parseLlmnr(q_conflict[0..]) == null, true);
    const q_tc: [12]u8 = [0, 1, 0x02, 0x00, 0, 1, 0, 0, 0, 0, 0, 0];
    expect_eq(parseLlmnr(q_tc[0..]) == null, true);
    const q_tent: [12]u8 = [0, 1, 0x01, 0x00, 0, 1, 0, 0, 0, 0, 0, 0];
    expect_eq(parseLlmnr(q_tent[0..]) == null, true);

    // Query with qd_count != 1 or an_count != 0
    const q_qd2: [12]u8 = [0, 1, 0x00, 0x00, 0, 2, 0, 0, 0, 0, 0, 0];
    expect_eq(parseLlmnr(q_qd2[0..]) == null, true);
    const q_an1: [12]u8 = [0, 1, 0x00, 0x00, 0, 1, 0, 1, 0, 0, 0, 0];
    expect_eq(parseLlmnr(q_an1[0..]) == null, true);

    // Encoder guards: empty name, double dot, tiny buffer
    let tiny: [10]u8 = [0; 10];
    let ok_buf: [64]u8 = [0; 64];
    expect_eq(encodeLlmnrQuery(1, "", LLMNR_TYPE_A, ok_buf[0..]) == null, true);
    expect_eq(encodeLlmnrQuery(1, "a..b", LLMNR_TYPE_A, ok_buf[0..]) == null, true);
    expect_eq(encodeLlmnrQuery(1, "host", LLMNR_TYPE_A, tiny[0..]) == null, true);
});
