// Source: cp/cap/test/test_ospf_production_deep.cp (Production deep tests for cap:ospf)

import { test, expect_eq } from "cap:test";
import {
    OSPF_IP_PROTO,
    OSPF_V2,
    OSPF_HEADER_SIZE,
    OSPF_LSA_HEADER_SIZE,
    AUTH_NONE,
    AUTH_SIMPLE_PW,
    AUTH_CRYPTO,
    OspfPacketType,
    OspfLsaType,
    OspfHeader,
    OspfPacket,
    OspfHello,
    OspfLsaHeader,
    OspfLsuEntry,
    OspfLsuIter,
    OspfLsackIter,
    ospf2Checksum,
    verifyOspf2Checksum,
    isOspf2,
    parseOspf2,
    parseOspfHello,
    parseOspfLsaHeader,
} from "cap:net";

fn writeBeU16(buf: *[]u8, off: usize, v: u16) {
    buf.*[off] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 1] = (v & 0xFF) as u8;
}

fn writeBeU32(buf: *[]u8, off: usize, v: u32) {
    buf.*[off] = ((v >> 24) & 0xFF) as u8;
    buf.*[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf.*[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 3] = (v & 0xFF) as u8;
}

fn writeIp4(buf: *[]u8, off: usize, a: u8, b: u8, c: u8, d: u8) {
    buf.*[off] = a;
    buf.*[off + 1] = b;
    buf.*[off + 2] = c;
    buf.*[off + 3] = d;
}

fn writeLsaHeader(
    buf: *[]u8,
    off: usize,
    ls_age: u16,
    options: u8,
    ls_type: u8,
    ls_id_last: u8,
    adv_last: u8,
    seq: u32,
    csum: u16,
    length: u16,
) {
    writeBeU16(buf, off, ls_age);
    buf.*[off + 2] = options;
    buf.*[off + 3] = ls_type;
    writeIp4(buf, off + 4, 10, 0, 0, ls_id_last);
    writeIp4(buf, off + 8, 1, 1, 1, adv_last);
    writeBeU32(buf, off + 12, seq);
    writeBeU16(buf, off + 16, csum);
    writeBeU16(buf, off + 18, length);
}

test("1. OSPFv2 Hello packet with 2 neighbors, DR/BDR & RFC 2328 checksum skipping 8B auth field", () => {
    expect_eq(OSPF_IP_PROTO, 89);
    expect_eq(OSPF_V2, 2);
    expect_eq(OSPF_HEADER_SIZE, 24);
    expect_eq(OSPF_LSA_HEADER_SIZE, 20);

    // 24B header + 20B Hello base + 8B (2 neighbors) = 52 bytes
    let raw: [52]u8 = [0; 52];
    let s: []u8 = raw[0..52];

    s[0] = OSPF_V2;
    s[1] = 1; // Hello
    writeBeU16(*s, 2, 52);
    writeIp4(*s, 4, 10, 1, 1, 1); // router_id
    writeIp4(*s, 8, 0, 0, 0, 0);  // area_id = 0.0.0.0
    writeBeU16(*s, 14, AUTH_SIMPLE_PW);
    // 填入非零 8 字节认证密码 "ospfpwd!"，校验和必须跳过此区域！
    s[16] = 111; s[17] = 115; s[18] = 112; s[19] = 102;
    s[20] = 112; s[21] = 119; s[22] = 100; s[23] = 33;

    // Hello body @ 24
    writeIp4(*s, 24, 255, 255, 255, 0); // network_mask
    writeBeU16(*s, 28, 10);             // hello_interval = 10s
    s[30] = 0x02;                       // options (E-bit)
    s[31] = 1;                          // router_priority
    writeBeU32(*s, 32, 40);             // dead_interval = 40s
    writeIp4(*s, 36, 10, 1, 1, 254);    // DR
    writeIp4(*s, 40, 10, 1, 1, 253);    // BDR
    writeIp4(*s, 44, 10, 1, 1, 2);      // Neighbor 0
    writeIp4(*s, 48, 10, 1, 1, 3);      // Neighbor 1

    // 计算并填入 RFC 2328 校验和
    const csum: u16 = ospf2Checksum(s[0..52]);
    writeBeU16(*s, 12, csum);

    expect_eq(isOspf2(s[0..52]), true);
    expect_eq(verifyOspf2Checksum(s[0..52]), true);

    // 修改 8B auth_data 中的字节，校验和必须依然通过（验证确实跳过了 16..24！）
    s[18] = 0xAA;
    expect_eq(verifyOspf2Checksum(s[0..52]), true);

    const pkt = parseOspf2(s[0..52]) ?? unreachable;
    expect_eq(pkt.header.version, 2);
    expect_eq(pkt.header.pkt_type == .hello, true);
    expect_eq(pkt.header.packet_len, 52);
    expect_eq(pkt.header.auth_type, AUTH_SIMPLE_PW);
    expect_eq(pkt.header.router_id[0], 10);
    expect_eq(pkt.header.router_id[3], 1);
    expect_eq(pkt.header.checksum, csum);

    const hello = parseOspfHello(pkt.body) ?? unreachable;
    expect_eq(hello.network_mask[0], 255);
    expect_eq(hello.network_mask[3], 0);
    expect_eq(hello.hello_interval, 10);
    expect_eq(hello.options, 0x02);
    expect_eq(hello.router_priority, 1);
    expect_eq(hello.dead_interval, 40);
    expect_eq(hello.designated_router[3], 254);
    expect_eq(hello.backup_dr[3], 253);
    expect_eq(hello.neighborCount(), 2);

    const n0 = hello.neighborAt(0) ?? unreachable;
    expect_eq(n0[3], 2);
    const n1 = hello.neighborAt(1) ?? unreachable;
    expect_eq(n1[3], 3);
    expect_eq(hello.neighborAt(2) == null, true);
});

test("2. OSPFv2 LSU (Link State Update) multi-LSA iteration (Router + Network + AS-External)", () => {
    // LSU body: 4B num_lsas (=3)
    // LSA 1: Router LSA (type=1), len=28 (20B hdr + 8B body)
    // LSA 2: Network LSA (type=2), len=24 (20B hdr + 4B body)
    // LSA 3: AS-External LSA (type=5), len=36 (20B hdr + 16B body)
    // Total body = 4 + 28 + 24 + 36 = 92B; Total packet = 24 + 92 = 116B
    let raw: [116]u8 = [0; 116];
    let s: []u8 = raw[0..116];

    s[0] = OSPF_V2;
    s[1] = 4; // LSU
    writeBeU16(*s, 2, 116);
    writeIp4(*s, 4, 2, 2, 2, 2);
    writeIp4(*s, 8, 0, 0, 0, 1);
    writeBeU16(*s, 14, AUTH_NONE);

    // LSU num_lsas = 3 @ 24
    writeBeU32(*s, 24, 3);

    // LSA 1 @ 28 (len 28)
    writeLsaHeader(*s, 28, 120, 0x02, 1, 10, 1, 0x80000001, 0x1122, 28);
    s[48] = 0xAA; s[55] = 0xBB;

    // LSA 2 @ 56 (len 24)
    writeLsaHeader(*s, 56, 300, 0x02, 2, 20, 2, 0x80000005, 0x3344, 24);
    s[76] = 0xCC;

    // LSA 3 @ 80 (len 36)
    writeLsaHeader(*s, 80, 3600, 0x02, 5, 30, 3, 0x80000009, 0x5566, 36);
    s[100] = 0xDD; s[115] = 0xEE;

    const csum: u16 = ospf2Checksum(s[0..116]);
    writeBeU16(*s, 12, csum);
    expect_eq(verifyOspf2Checksum(s[0..116]), true);

    const pkt = parseOspf2(s[0..116]) ?? unreachable;
    expect_eq(pkt.header.pkt_type == .lsu, true);

    let it = OspfLsuIter.init(pkt.body) ?? unreachable;
    expect_eq(it.num_lsas, 3);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.header.ls_type == .router, true);
    expect_eq(e1.header.ls_age, 120);
    expect_eq(e1.header.ls_seq_num, 0x80000001);
    expect_eq(e1.header.length, 28);
    expect_eq(e1.lsa_body.len, 8);
    expect_eq(e1.lsa_body[0], 0xAA);
    expect_eq(e1.lsa_body[7], 0xBB);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.header.ls_type == .network, true);
    expect_eq(e2.header.link_state_id[3], 20);
    expect_eq(e2.lsa_body.len, 4);
    expect_eq(e2.lsa_body[0], 0xCC);

    const e3 = it.next() ?? unreachable;
    expect_eq(e3.header.ls_type == .as_external, true);
    expect_eq(e3.header.ls_age, 3600);
    expect_eq(e3.header.advertising_router[3], 3);
    expect_eq(e3.lsa_body.len, 16);
    expect_eq(e3.lsa_body[0], 0xDD);
    expect_eq(e3.lsa_body[15], 0xEE);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("3. OSPFv2 LSAck (20B LSA header sequence) parsing", () => {
    // 24B header + 2 * 20B LSA headers = 64B
    let raw: [64]u8 = [0; 64];
    let s: []u8 = raw[0..64];

    s[0] = OSPF_V2;
    s[1] = 5; // LSAck
    writeBeU16(*s, 2, 64);
    writeIp4(*s, 4, 3, 3, 3, 3);
    writeIp4(*s, 8, 0, 0, 0, 0);

    writeLsaHeader(*s, 24, 15, 0x22, 3, 100, 1, 0x80000010, 0xABCD, 28); // summary_net
    writeLsaHeader(*s, 44, 25, 0x22, 10, 200, 2, 0x80000020, 0xEF01, 44); // opaque_area

    const csum: u16 = ospf2Checksum(s[0..64]);
    writeBeU16(*s, 12, csum);
    expect_eq(verifyOspf2Checksum(s[0..64]), true);

    const pkt = parseOspf2(s[0..64]) ?? unreachable;
    expect_eq(pkt.header.pkt_type == .lsack, true);

    let it = OspfLsackIter.init(pkt.body);
    const h1 = it.next() ?? unreachable;
    expect_eq(h1.ls_type == .summary_net, true);
    expect_eq(h1.link_state_id[3], 100);
    expect_eq(h1.length, 28);

    const h2 = it.next() ?? unreachable;
    expect_eq(h2.ls_type == .opaque_area, true);
    expect_eq(h2.link_state_id[3], 200);
    expect_eq(h2.length, 44);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("4. Tamper checksum detection on header & body bytes", () => {
    let raw: [44]u8 = [0; 44];
    let s: []u8 = raw[0..44];
    s[0] = OSPF_V2;
    s[1] = 1;
    writeBeU16(*s, 2, 44);
    writeIp4(*s, 4, 192, 168, 0, 1);
    writeIp4(*s, 24, 255, 255, 255, 0);
    writeBeU16(*s, 28, 10);
    writeBeU32(*s, 32, 40);

    const csum: u16 = ospf2Checksum(s[0..44]);
    writeBeU16(*s, 12, csum);
    expect_eq(verifyOspf2Checksum(s[0..44]), true);

    // 篡改 body 中 hello_interval
    s[29] = 11;
    expect_eq(verifyOspf2Checksum(s[0..44]), false);
});

test("5. Malformed packet & LSA bounds defense: non-v2, packet_len < 24, LSA length < 20, truncated LSU", () => {
    // 5.1 短于 24 字节
    const short23: [23]u8 = [0; 23];
    expect_eq(isOspf2(short23[0..23]), false);
    expect_eq(parseOspf2(short23[0..23]) == null, true);
    expect_eq(verifyOspf2Checksum(short23[0..23]), false);

    // 5.2 非 v2 (OSPFv3 = 3)
    let v3: [24]u8 = [0; 24];
    v3[0] = 3;
    v3[1] = 1;
    v3[3] = 24;
    expect_eq(isOspf2(v3[0..24]), false);
    expect_eq(parseOspf2(v3[0..24]) == null, true);

    // 5.3 packet_len < 24 或 > data.len
    let bad_len: [24]u8 = [0; 24];
    bad_len[0] = OSPF_V2;
    bad_len[1] = 1;
    bad_len[3] = 20; // < 24
    expect_eq(parseOspf2(bad_len[0..24]) == null, true);
    bad_len[3] = 40; // > 24
    expect_eq(parseOspf2(bad_len[0..24]) == null, true);

    // 5.4 Hello 邻居区非 4 字节倍数
    const bad_hello: [22]u8 = [0; 22];
    expect_eq(parseOspfHello(bad_hello[0..22]) == null, true);

    // 5.5 LSA length < 20 拒绝
    let bad_lsa: [20]u8 = [0; 20];
    bad_lsa[19] = 16; // length = 16 < 20
    expect_eq(parseOspfLsaHeader(bad_lsa[0..20]) == null, true);

    // 5.6 LSU 声明 num_lsas = 1 但 LSA 声明 length = 40 超出切片边界 -> had_error = true
    let trunc_lsu: [24]u8 = [0; 24];
    let ts: []u8 = trunc_lsu[0..24];
    writeBeU32(*ts, 0, 1);
    writeLsaHeader(*ts, 4, 10, 0x02, 1, 1, 1, 0x80000001, 0x1234, 40);
    let it = OspfLsuIter.init(ts[0..24]) ?? unreachable;
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, true);
});
