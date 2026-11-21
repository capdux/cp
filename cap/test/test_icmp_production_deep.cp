// Source: cp/cap/test/test_icmp_production_deep.cp (Production deep tests for cap:icmp)

import { test, expect_eq } from "cap:test";
import {
    ICMP_HEADER_SIZE,
    ICMP_IP_PROTO_V4,
    ICMP_IP_PROTO_V6,
    ICMP4_ECHO_REPLY,
    ICMP4_DEST_UNREACH,
    ICMP4_REDIRECT,
    ICMP4_ECHO_REQUEST,
    ICMP4_TIME_EXCEEDED,
    ICMP4_PARAM_PROBLEM,
    ICMP4_TIMESTAMP_REQ,
    ICMP4_TIMESTAMP_REPLY,
    ICMP6_DEST_UNREACH,
    ICMP6_PACKET_TOO_BIG,
    ICMP6_TIME_EXCEEDED,
    ICMP6_PARAM_PROBLEM,
    ICMP6_ECHO_REQUEST,
    ICMP6_ECHO_REPLY,
    ICMP6_ROUTER_SOLICIT,
    ICMP6_ROUTER_ADVERT,
    ICMP6_NEIGHBOR_SOLICIT,
    ICMP6_NEIGHBOR_ADVERT,
    IcmpPacket,
    icmp4Checksum,
    verifyIcmp4Checksum,
    icmp6Checksum,
    verifyIcmp6Checksum,
    parseIcmp,
    encodeIcmp4,
    encodeIcmp4Echo,
    encodeIcmp6,
    encodeIcmp6Echo,
} from "cap:icmp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. ICMPv4 Echo Request & Reply roundtrip + RFC 1071 checksum verification", () => {
    expect_eq(ICMP_HEADER_SIZE, 8);
    expect_eq(ICMP_IP_PROTO_V4, 1);
    expect_eq(ICMP_IP_PROTO_V6, 58);

    let buf: [64]u8 = [0; 64];
    let out: []u8 = buf[0..64];

    // Echo Request (even payload length = 8)
    const n_req = encodeIcmp4Echo(false, 0x1A2B, 42, "pingtest", *out) ?? unreachable;
    expect_eq(n_req, 16);
    expect_eq(verifyIcmp4Checksum(out[0..n_req]), true);

    const req = parseIcmp(out[0..n_req]) ?? unreachable;
    expect_eq(req.isEchoRequest4(), true);
    expect_eq(req.isEchoReply4(), false);
    expect_eq(req.echoId(), 0x1A2B);
    expect_eq(req.echoSeq(), 42);
    expect_eq(req.checksum, icmp4Checksum(out[0..n_req]));
    expect_eq(sliceEq(req.payload, "pingtest"), true);

    // Echo Reply
    const n_rep = encodeIcmp4Echo(true, 0x1A2B, 42, "pingtest", *out) ?? unreachable;
    expect_eq(n_rep, 16);
    expect_eq(verifyIcmp4Checksum(out[0..n_rep]), true);

    const rep = parseIcmp(out[0..n_rep]) ?? unreachable;
    expect_eq(rep.isEchoReply4(), true);
    expect_eq(rep.isEchoRequest4(), false);
    expect_eq(rep.echoId(), 0x1A2B);
    expect_eq(rep.echoSeq(), 42);
});

test("2. Odd-length payload (5B 'hello') RFC 1071 checksum exactness", () => {
    let buf: [32]u8 = [0; 32];
    let out: []u8 = buf[0..32];

    // Type=8, Code=0, Id=0x0001, Seq=0x0002, Payload="hello" (5 bytes: 0x68,0x65,0x6C,0x6C,0x6F)
    // Manual RFC 1071 sum:
    //   0x0800 + 0x0000 + 0x0001 + 0x0002 + 0x6865 + 0x6C6C + 0x6F00
    // = 0x0800 + 0x0003 + 0x6865 + 0x6C6C + 0x6F00
    // = 0x7068 + 0x6C6C = 0xDCD4; 0xDCD4 + 0x6F00 = 0x14BD4
    // fold: 0x4BD4 + 1 = 0x4BD5; ~0x4BD5 = 0xB42A
    const n = encodeIcmp4Echo(false, 1, 2, "hello", *out) ?? unreachable;
    expect_eq(n, 13);
    expect_eq(verifyIcmp4Checksum(out[0..n]), true);

    const pkt = parseIcmp(out[0..n]) ?? unreachable;
    expect_eq(pkt.checksum, 0xB42A);
    expect_eq(sliceEq(pkt.payload, "hello"), true);
});

test("3. ICMPv6 Echo Request & Reply roundtrip with 40B IPv6 pseudo-header checksum", () => {
    // 2001:db8::1 -> 2001:db8::2
    const src_ip: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    const dst_ip: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2];

    let buf: [64]u8 = [0; 64];
    let out: []u8 = buf[0..64];

    const n = encodeIcmp6Echo(src_ip, dst_ip, false, 0xBEEF, 7, "ipv6!", *out) ?? unreachable;
    expect_eq(n, 13);
    expect_eq(verifyIcmp6Checksum(src_ip, dst_ip, out[0..n]), true);

    // 目的地址变化时伪首部校验必须失败
    const wrong_dst: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9];
    expect_eq(verifyIcmp6Checksum(src_ip, wrong_dst, out[0..n]), false);

    const pkt = parseIcmp(out[0..n]) ?? unreachable;
    expect_eq(pkt.isEchoRequest6(), true);
    expect_eq(pkt.isEchoReply6(), false);
    expect_eq(pkt.echoId(), 0xBEEF);
    expect_eq(pkt.echoSeq(), 7);
    expect_eq(pkt.checksum, icmp6Checksum(src_ip, dst_ip, out[0..n]));

    // Echo Reply 6
    const n2 = encodeIcmp6Echo(dst_ip, src_ip, true, 0xBEEF, 7, "ipv6!", *out) ?? unreachable;
    expect_eq(verifyIcmp6Checksum(dst_ip, src_ip, out[0..n2]), true);
    const rep6 = parseIcmp(out[0..n2]) ?? unreachable;
    expect_eq(rep6.isEchoReply6(), true);
});

test("4. DestUnreach Fragmentation Needed (Next-Hop MTU), TimeExceeded & ICMPv6 Packet Too Big", () => {
    let buf: [64]u8 = [0; 64];
    let out: []u8 = buf[0..64];

    // 4.1 ICMPv4 DestUnreach (Type 3, Code 4: Fragmentation Needed), Next-Hop MTU = 1400
    const orig_hdr: [8]u8 = [0x45, 0x00, 0x05, 0xDC, 0x12, 0x34, 0x40, 0x00];
    const n1 = encodeIcmp4(ICMP4_DEST_UNREACH, 4, 1400, orig_hdr[0..8], *out) ?? unreachable;
    expect_eq(verifyIcmp4Checksum(out[0..n1]), true);

    const du = parseIcmp(out[0..n1]) ?? unreachable;
    expect_eq(du.icmp_type, ICMP4_DEST_UNREACH);
    expect_eq(du.code, 4);
    expect_eq(du.nextHopMtu(), 1400);
    expect_eq(du.payload.len, 8);

    // 4.2 ICMPv4 TimeExceeded (Type 11, Code 0: TTL Exceeded in Transit)
    const n2 = encodeIcmp4(ICMP4_TIME_EXCEEDED, 0, 0, orig_hdr[0..8], *out) ?? unreachable;
    expect_eq(verifyIcmp4Checksum(out[0..n2]), true);
    const te = parseIcmp(out[0..n2]) ?? unreachable;
    expect_eq(te.icmp_type, ICMP4_TIME_EXCEEDED);
    expect_eq(te.code, 0);

    // 4.3 ICMPv6 Packet Too Big (Type 2, Code 0, MTU = 1280)
    const src6: [16]u8 = [0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    const dst6: [16]u8 = [0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2];
    const n3 = encodeIcmp6(src6, dst6, ICMP6_PACKET_TOO_BIG, 0, 1280, orig_hdr[0..8], *out) ?? unreachable;
    expect_eq(verifyIcmp6Checksum(src6, dst6, out[0..n3]), true);
    const ptb = parseIcmp(out[0..n3]) ?? unreachable;
    expect_eq(ptb.icmp_type, ICMP6_PACKET_TOO_BIG);
    expect_eq(ptb.packetTooBigMtu(), 1280);
});

test("5. Tamper detection, <8B truncation & insufficient output buffer guards", () => {
    // 5.1 短于 8 字节
    const short_pkt: [7]u8 = [8, 0, 0, 0, 0, 1, 0];
    expect_eq(parseIcmp(short_pkt[0..7]) == null, true);
    expect_eq(verifyIcmp4Checksum(short_pkt[0..7]), false);
    const z16: [16]u8 = [0; 16];
    expect_eq(verifyIcmp6Checksum(z16, z16, short_pkt[0..7]), false);

    // 5.2 输出缓冲不足（需要 8 + 5 = 13 字节，仅提供 12 字节）
    let small_buf: [12]u8 = [0; 12];
    let small_out: []u8 = small_buf[0..12];
    expect_eq(encodeIcmp4Echo(false, 1, 1, "hello", *small_out) == null, true);
    expect_eq(encodeIcmp6Echo(z16, z16, false, 1, 1, "hello", *small_out) == null, true);

    // 5.3 单比特篡改检测
    let buf: [32]u8 = [0; 32];
    let out: []u8 = buf[0..32];
    const n = encodeIcmp4Echo(false, 9, 10, "integrity", *out) ?? unreachable;
    expect_eq(verifyIcmp4Checksum(out[0..n]), true);
    out[n - 1] = out[n - 1] ^ 0x01; // 翻转负载最后一位
    expect_eq(verifyIcmp4Checksum(out[0..n]), false);
});
