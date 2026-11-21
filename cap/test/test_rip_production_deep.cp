// Source: cp/cap/test/test_rip_production_deep.cp (Production deep tests for cap:rip)

import { test, expect_eq } from "cap:test";
import {
    RIP_UDP_PORT,
    RIPNG_UDP_PORT,
    RIP_HEADER_SIZE,
    RIP_RTE_SIZE,
    RIP_V1,
    RIP_V2,
    RIPNG_V1,
    RIP_CMD_REQUEST,
    RIP_CMD_RESPONSE,
    RIP_AFI_IP,
    RIP_AFI_AUTH,
    RIP_AUTH_SIMPLE,
    RIP_AUTH_CRYPTO,
    RIP_METRIC_INFINITY,
    RIPNG_NEXT_HOP_METRIC,
    RipHeader,
    RipRte,
    RipAuthSimple,
    RipRteIter,
    RipPacket,
    RipngRte,
    RipngRteIter,
    RipngPacket,
    isRip,
    parseRip,
    parseRipAuthSimple,
    parseRipng,
    encodeRipHeader,
    encodeRipRte,
} from "cap:rip";

test("1. RIPv2 multi-RTE encode & parse roundtrip (normal route, default route, unreachable metric=16)", () => {
    expect_eq(RIP_UDP_PORT, 520);
    expect_eq(RIPNG_UDP_PORT, 521);
    expect_eq(RIP_HEADER_SIZE, 4);
    expect_eq(RIP_RTE_SIZE, 20);
    expect_eq(RIP_METRIC_INFINITY, 16);

    // 4B header + 3 * 20B RTE = 64 bytes
    let buf: [64]u8 = [0; 64];
    let h_slice: []u8 = buf[0..4];
    expect_eq(encodeRipHeader(RIP_CMD_RESPONSE, RIP_V2, *h_slice) ?? 0, 4);

    let r0: []u8 = buf[4..24];
    expect_eq(encodeRipRte(RipRte {
        afi: RIP_AFI_IP,
        route_tag: 100,
        ip: [10, 20, 0, 0],
        mask: [255, 255, 0, 0],
        next_hop: [10, 0, 0, 1],
        metric: 3,
    }, *r0) ?? 0, 20);

    let r1: []u8 = buf[24..44];
    expect_eq(encodeRipRte(RipRte {
        afi: RIP_AFI_IP,
        route_tag: 0,
        ip: [0, 0, 0, 0],
        mask: [0, 0, 0, 0],
        next_hop: [192, 168, 1, 1],
        metric: 1,
    }, *r1) ?? 0, 20);

    let r2: []u8 = buf[44..64];
    expect_eq(encodeRipRte(RipRte {
        afi: RIP_AFI_IP,
        route_tag: 0,
        ip: [172, 16, 5, 0],
        mask: [255, 255, 255, 0],
        next_hop: [0, 0, 0, 0],
        metric: RIP_METRIC_INFINITY,
    }, *r2) ?? 0, 20);

    expect_eq(isRip(buf[0..64]), true);
    const pkt = parseRip(buf[0..64]) ?? unreachable;
    expect_eq(pkt.header.isResponse(), true);
    expect_eq(pkt.header.version, RIP_V2);
    expect_eq(pkt.rte_count, 3);

    let it = pkt.entries();

    const e0 = it.next() ?? unreachable;
    expect_eq(e0.afi, RIP_AFI_IP);
    expect_eq(e0.route_tag, 100);
    expect_eq(e0.ip[0], 10);
    expect_eq(e0.ip[1], 20);
    expect_eq(e0.next_hop[3], 1);
    expect_eq(e0.metric, 3);
    expect_eq(e0.isAuth(), false);
    expect_eq(e0.isDefaultRoute(), false);
    expect_eq(e0.isUnreachable(), false);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.isDefaultRoute(), true);
    expect_eq(e1.metric, 1);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.isUnreachable(), true);
    expect_eq(e2.metric, 16);

    expect_eq(it.next() == null, true);
});

test("2. RIPv2 AFI=0xFFFF Simple Password Authentication RTE parsing", () => {
    // 4B header + 20B Auth RTE + 20B IP RTE = 44 bytes
    let buf: [44]u8 = [0; 44];
    buf[0] = RIP_CMD_RESPONSE;
    buf[1] = RIP_V2;
    // Auth RTE @ 4..24
    buf[4] = 0xFF; buf[5] = 0xFF; // AFI = 0xFFFF
    buf[6] = 0x00; buf[7] = 0x02; // Auth Type = Simple Password (2)
    // password = "secret" (ASCII)
    buf[8] = 115; buf[9] = 101; buf[10] = 99; buf[11] = 114; buf[12] = 101; buf[13] = 116;

    // Normal RTE @ 24..44
    buf[24] = 0x00; buf[25] = 0x02; // AFI = IP
    buf[28] = 192; buf[29] = 168; buf[30] = 50; buf[31] = 0;
    buf[32] = 255; buf[33] = 255; buf[34] = 255; buf[35] = 0;
    buf[43] = 2; // metric = 2

    const pkt = parseRip(buf[0..44]) ?? unreachable;
    expect_eq(pkt.rte_count, 2);

    let it = pkt.entries();
    const auth_rte = it.next() ?? unreachable;
    expect_eq(auth_rte.isAuth(), true);
    expect_eq(auth_rte.route_tag, RIP_AUTH_SIMPLE);

    const raw0 = pkt.rawRteAt(0) ?? unreachable;
    const auth = parseRipAuthSimple(raw0) ?? unreachable;
    expect_eq(auth.password[0], 115); // 's'
    expect_eq(auth.password[5], 116); // 't'
    expect_eq(auth.password[6], 0);   // zero-padded

    // 非认证 RTE 传入 parseRipAuthSimple 必须返回 null
    const raw1 = pkt.rawRteAt(1) ?? unreachable;
    expect_eq(parseRipAuthSimple(raw1) == null, true);

    const route_rte = it.next() ?? unreachable;
    expect_eq(route_rte.ip[2], 50);
    expect_eq(route_rte.metric, 2);
});

test("3. RIPng (RFC 2080) IPv6 Prefix & Next-Hop (metric=0xFF) RTE iteration", () => {
    // 4B header + 2 * 20B RIPng RTE = 44 bytes
    let buf: [44]u8 = [0; 44];
    buf[0] = RIP_CMD_RESPONSE;
    buf[1] = RIPNG_V1;

    // RTE 0: Next-Hop RTE (fe80::1, metric = 0xFF)
    buf[4] = 0xfe; buf[5] = 0x80; buf[19] = 0x01;
    buf[23] = RIPNG_NEXT_HOP_METRIC;

    // RTE 1: Prefix 2001:db8:abcd::/48, tag=0x1234, metric=4
    buf[24] = 0x20; buf[25] = 0x01; buf[26] = 0x0d; buf[27] = 0xb8;
    buf[28] = 0xab; buf[29] = 0xcd;
    buf[40] = 0x12; buf[41] = 0x34; // route_tag
    buf[42] = 48;                   // prefix_len
    buf[43] = 4;                    // metric

    const pkt = parseRipng(buf[0..44]) ?? unreachable;
    expect_eq(pkt.header.command, RIP_CMD_RESPONSE);
    expect_eq(pkt.header.version, RIPNG_V1);
    expect_eq(pkt.rte_count, 2);

    let it = pkt.entries();
    const nh = it.next() ?? unreachable;
    expect_eq(nh.isNextHop(), true);
    expect_eq(nh.prefix[0], 0xfe);
    expect_eq(nh.prefix[15], 0x01);

    const rt = it.next() ?? unreachable;
    expect_eq(rt.isNextHop(), false);
    expect_eq(rt.isUnreachable(), false);
    expect_eq(rt.route_tag, 0x1234);
    expect_eq(rt.prefix_len, 48);
    expect_eq(rt.metric, 4);
    expect_eq(rt.prefix[4], 0xab);

    expect_eq(it.next() == null, true);
});

test("4. RIPv1 Request & encoder buffer bounds checks", () => {
    let buf: [24]u8 = [0; 24];
    let h: []u8 = buf[0..4];
    expect_eq(encodeRipHeader(RIP_CMD_REQUEST, RIP_V1, *h) ?? 0, 4);

    // 缓冲不足拒绝
    let tiny3: [3]u8 = [0; 3];
    let ts3: []u8 = tiny3[0..3];
    expect_eq(encodeRipHeader(RIP_CMD_REQUEST, RIP_V1, *ts3) == null, true);

    let tiny19: [19]u8 = [0; 19];
    let ts19: []u8 = tiny19[0..19];
    expect_eq(encodeRipRte(RipRte {
        afi: 0,
        route_tag: 0,
        ip: [0; 4],
        mask: [0; 4],
        next_hop: [0; 4],
        metric: 16,
    }, *ts19) == null, true);

    // 合法 4B header + 20B all-routes request RTE
    let r: []u8 = buf[4..24];
    expect_eq(encodeRipRte(RipRte {
        afi: 0,
        route_tag: 0,
        ip: [0; 4],
        mask: [0; 4],
        next_hop: [0; 4],
        metric: 16,
    }, *r) ?? 0, 20);

    const pkt = parseRip(buf[0..24]) ?? unreachable;
    expect_eq(pkt.header.isRequest(), true);
    expect_eq(pkt.header.version, RIP_V1);
    expect_eq(pkt.rte_count, 1);
});

test("5. Malformed packet defense: bad command/version, non-zero reserved, non-20B aligned body", () => {
    // 5.1 短于 4B
    const short3: [3]u8 = [1, 2, 0];
    expect_eq(isRip(short3[0..3]), false);
    expect_eq(parseRip(short3[0..3]) == null, true);
    expect_eq(parseRipng(short3[0..3]) == null, true);

    // 5.2 非法 command (0 or 3)
    const bad_cmd: [24]u8 = [3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(parseRip(bad_cmd[0..24]) == null, true);
    expect_eq(parseRipng(bad_cmd[0..24]) == null, true);

    // 5.3 非法 version (RIP version=3; RIPng version=2)
    const bad_ver: [24]u8 = [2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(parseRip(bad_ver[0..24]) == null, true);
    const ripng_v2: [24]u8 = [2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(parseRipng(ripng_v2[0..24]) == null, true);

    // 5.4 非零 reserved 字段
    const bad_rsvd: [24]u8 = [2, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(isRip(bad_rsvd[0..24]), false);
    expect_eq(parseRip(bad_rsvd[0..24]) == null, true);
    const bad_rsvd_ng: [24]u8 = [2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(parseRipng(bad_rsvd_ng[0..24]) == null, true);

    // 5.5 非 20B 对齐 (4 + 19 = 23 bytes)
    let unaligned: [23]u8 = [0; 23];
    unaligned[0] = 2; unaligned[1] = 2;
    expect_eq(isRip(unaligned[0..23]), false);
    expect_eq(parseRip(unaligned[0..23]) == null, true);
    unaligned[1] = 1;
    expect_eq(parseRipng(unaligned[0..23]) == null, true);
});
