// Source: cp/cap/test/test_vrrp_production_deep.cp (Production deep tests for cap:vrrp)

import { test, expect_eq } from "cap:test";
import {
    VRRP_IP_PROTO,
    VRRP_V2,
    VRRP_V3,
    VRRP_TYPE_ADVERTISEMENT,
    VRRP_HEADER_SIZE,
    VRRP_V2_AUTH_SIZE,
    VRRP_AUTH_NONE,
    VRRP_AUTH_SIMPLE,
    VRRP_AUTH_AH,
    VrrpHeader,
    VrrpIpv4Iter,
    VrrpIpv6Iter,
    VrrpPacket,
    isVrrp,
    parseVrrp,
    vrrp2Checksum,
    verifyVrrp2Checksum,
    vrrp3ChecksumIpv4,
    verifyVrrp3ChecksumIpv4,
    vrrp3ChecksumIpv6,
    verifyVrrp3ChecksumIpv6,
} from "cap:vrrp";

fn writeBeU16(buf: *[]u8, off: usize, v: u16) {
    buf.*[off] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 1] = (v & 0xFF) as u8;
}

test("1. VRRPv2 (RFC 3768) Advertisement with 2 IPv4 VIPs, Simple Auth & checksum verification", () => {
    expect_eq(VRRP_IP_PROTO, 112);
    expect_eq(VRRP_V2, 2);
    expect_eq(VRRP_V3, 3);
    expect_eq(VRRP_TYPE_ADVERTISEMENT, 1);
    expect_eq(VRRP_HEADER_SIZE, 8);
    expect_eq(VRRP_V2_AUTH_SIZE, 8);

    // 8B header + 2 * 4B IPv4 + 8B auth = 24 bytes
    let raw: [24]u8 = [0; 24];
    let s: []u8 = raw[0..24];
    s[0] = (VRRP_V2 << 4) | VRRP_TYPE_ADVERTISEMENT; // 0x21
    s[1] = 51;               // VRID = 51
    s[2] = 255;              // Priority = 255 (Address Owner)
    s[3] = 2;                // Count IP = 2
    s[4] = VRRP_AUTH_SIMPLE; // Auth Type = 1
    s[5] = 3;                // Adver Int = 3s

    // VIP 0: 192.168.10.1
    s[8] = 192; s[9] = 168; s[10] = 10; s[11] = 1;
    // VIP 1: 192.168.10.2
    s[12] = 192; s[13] = 168; s[14] = 10; s[15] = 2;
    // Auth Data: "vrrp2key"
    s[16] = 118; s[17] = 114; s[18] = 114; s[19] = 112;
    s[20] = 50;  s[21] = 107; s[22] = 101; s[23] = 121;

    const csum: u16 = vrrp2Checksum(s[0..24]);
    writeBeU16(*s, 6, csum);

    expect_eq(isVrrp(s[0..24]), true);
    expect_eq(verifyVrrp2Checksum(s[0..24]), true);

    const pkt = parseVrrp(s[0..24], false) ?? unreachable;
    expect_eq(pkt.header.version, VRRP_V2);
    expect_eq(pkt.header.pkt_type, VRRP_TYPE_ADVERTISEMENT);
    expect_eq(pkt.header.vrid, 51);
    expect_eq(pkt.header.priority, 255);
    expect_eq(pkt.header.isAddressOwner(), true);
    expect_eq(pkt.header.isMasterShutdown(), false);
    expect_eq(pkt.header.count_ip, 2);
    expect_eq(pkt.header.auth_type, VRRP_AUTH_SIMPLE);
    expect_eq(pkt.header.max_adver_int, 3);
    expect_eq(pkt.header.checksum, csum);
    expect_eq(pkt.auth_data.len, 8);
    expect_eq(pkt.auth_data[0], 118);
    expect_eq(pkt.total_bytes, 24);

    let it = pkt.ipv4Addrs();
    const vip0 = it.next() ?? unreachable;
    expect_eq(vip0[0], 192);
    expect_eq(vip0[3], 1);
    const vip1 = it.next() ?? unreachable;
    expect_eq(vip1[0], 192);
    expect_eq(vip1[3], 2);
    expect_eq(it.next() == null, true);

    // 篡改检测
    s[11] = 99;
    expect_eq(verifyVrrp2Checksum(s[0..24]), false);
});

test("2. VRRPv3 over IPv4 (RFC 5798): 12-bit centisecond Max Adver Int & IPv4 pseudo-header checksum", () => {
    const src_ip: [4]u8 = [10, 0, 1, 2];
    const dst_ip: [4]u8 = [224, 0, 0, 18]; // All VRRP Routers multicast

    // 8B header + 1 * 4B IPv4 = 12 bytes (no v2 auth trailer!)
    let raw: [12]u8 = [0; 12];
    let s: []u8 = raw[0..12];
    s[0] = (VRRP_V3 << 4) | VRRP_TYPE_ADVERTISEMENT; // 0x31
    s[1] = 10;  // VRID
    s[2] = 100; // Priority
    s[3] = 1;   // Count IP = 1
    // rsvd = 0, max_adver_int = 300 centiseconds (0x012C = 3.00s)
    s[4] = 0x01;
    s[5] = 0x2C;
    s[8] = 10; s[9] = 0; s[10] = 1; s[11] = 254;

    const csum: u16 = vrrp3ChecksumIpv4(src_ip, dst_ip, s[0..12]);
    writeBeU16(*s, 6, csum);

    expect_eq(verifyVrrp3ChecksumIpv4(src_ip, dst_ip, s[0..12]), true);
    // 伪首部源 IP 变动必须校验失败
    const wrong_src: [4]u8 = [10, 0, 1, 9];
    expect_eq(verifyVrrp3ChecksumIpv4(wrong_src, dst_ip, s[0..12]), false);

    const pkt = parseVrrp(s[0..12], false) ?? unreachable;
    expect_eq(pkt.header.version, VRRP_V3);
    expect_eq(pkt.header.max_adver_int, 300);
    expect_eq(pkt.auth_data.len, 0);
    expect_eq(pkt.total_bytes, 12);

    let it = pkt.ipv4Addrs();
    const vip = it.next() ?? unreachable;
    expect_eq(vip[3], 254);
    expect_eq(it.next() == null, true);
});

test("3. VRRPv3 over IPv6 (RFC 5798): 2 IPv6 VIPs, Priority 0 shutdown & 40B IPv6 pseudo-header checksum", () => {
    const src6: [16]u8 = [0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    const dst6: [16]u8 = [0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x12]; // ff02::12

    // 8B header + 2 * 16B IPv6 = 40 bytes
    let raw: [40]u8 = [0; 40];
    let s: []u8 = raw[0..40];
    s[0] = 0x31;
    s[1] = 77;
    s[2] = 0;    // Master shutdown (priority = 0)
    s[3] = 2;    // Count IPv6 = 2
    s[4] = 0x00;
    s[5] = 0x64; // 100 centiseconds (1.00s)

    // VIP6[0] = fe80::100
    s[8] = 0xfe; s[9] = 0x80; s[22] = 0x01; s[23] = 0x00;
    // VIP6[1] = 2001:db8::1
    s[24] = 0x20; s[25] = 0x01; s[26] = 0x0d; s[27] = 0xb8; s[39] = 0x01;

    const csum: u16 = vrrp3ChecksumIpv6(src6, dst6, s[0..40]);
    writeBeU16(*s, 6, csum);

    expect_eq(verifyVrrp3ChecksumIpv6(src6, dst6, s[0..40]), true);

    const pkt = parseVrrp(s[0..40], true) ?? unreachable;
    expect_eq(pkt.header.version, VRRP_V3);
    expect_eq(pkt.header.isMasterShutdown(), true);
    expect_eq(pkt.header.max_adver_int, 100);
    expect_eq(pkt.is_ipv6, true);
    expect_eq(pkt.total_bytes, 40);

    // ipv4Addrs() on IPv6 packet yields empty iterator safely
    let v4it = pkt.ipv4Addrs();
    expect_eq(v4it.next() == null, true);

    let v6it = pkt.ipv6Addrs();
    const a0 = v6it.next() ?? unreachable;
    expect_eq(a0[0], 0xfe);
    expect_eq(a0[14], 0x01);
    const a1 = v6it.next() ?? unreachable;
    expect_eq(a1[0], 0x20);
    expect_eq(a1[15], 0x01);
    expect_eq(v6it.next() == null, true);

    // 篡改单字节导致 IPv6 校验和失败
    s[39] = 0x02;
    expect_eq(verifyVrrp3ChecksumIpv6(src6, dst6, s[0..40]), false);
});

test("4. VRRPv2 rejects IPv6 mode & VRRPv3 max 12-bit Adver Int (4095 cs)", () => {
    let v2: [20]u8 = [0; 20];
    v2[0] = 0x21;
    v2[1] = 1;
    v2[2] = 100;
    v2[3] = 1;
    // VRRPv2 不支持 IPv6
    expect_eq(parseVrrp(v2[0..20], true) == null, true);
    // VRRPv2 IPv4 正常解析
    const ok_v2 = parseVrrp(v2[0..20], false) ?? unreachable;
    expect_eq(ok_v2.total_bytes, 20);

    // VRRPv3 12-bit 全 1 (4095 centiseconds) + rsvd = 0x0A
    let v3: [12]u8 = [0; 12];
    v3[0] = 0x31;
    v3[1] = 2;
    v3[2] = 200;
    v3[3] = 1;
    v3[4] = 0xAF; // rsvd = 0xA, high 4 bits = 0xF
    v3[5] = 0xFF; // low 8 bits = 0xFF -> 0x0FFF = 4095
    const ok_v3 = parseVrrp(v3[0..12], false) ?? unreachable;
    expect_eq(ok_v3.header.auth_type, 0x0A);
    expect_eq(ok_v3.header.max_adver_int, 4095);
});

test("5. Malformed packet defense: short buffer, bad version/type, count_ip == 0, truncated VIP list", () => {
    // 5.1 短于 8B
    const short7: [7]u8 = [0x21, 1, 100, 1, 0, 1, 0];
    expect_eq(isVrrp(short7[0..7]), false);
    expect_eq(parseVrrp(short7[0..7], false) == null, true);
    expect_eq(verifyVrrp2Checksum(short7[0..7]), false);

    // 5.2 非法版本 (v1 = 0x11, v4 = 0x41)
    let bad_ver: [20]u8 = [0; 20];
    bad_ver[0] = 0x11; bad_ver[3] = 1;
    expect_eq(isVrrp(bad_ver[0..20]), false);
    expect_eq(parseVrrp(bad_ver[0..20], false) == null, true);
    bad_ver[0] = 0x41;
    expect_eq(parseVrrp(bad_ver[0..20], false) == null, true);

    // 5.3 非法类型 (type = 2 -> 0x22)
    let bad_type: [20]u8 = [0; 20];
    bad_type[0] = 0x22; bad_type[3] = 1;
    expect_eq(parseVrrp(bad_type[0..20], false) == null, true);

    // 5.4 count_ip == 0 拒绝
    let zero_cnt: [20]u8 = [0; 20];
    zero_cnt[0] = 0x21; zero_cnt[3] = 0;
    expect_eq(isVrrp(zero_cnt[0..20]), false);
    expect_eq(parseVrrp(zero_cnt[0..20], false) == null, true);

    // 5.5 count_ip 越界（VRRPv2 声明 3 个 IP 需 8+12+8=28B，仅给 20B；VRRPv3 IPv6 声明 2 个 IP 需 40B，仅给 24B）
    let trunc_v2: [20]u8 = [0; 20];
    trunc_v2[0] = 0x21; trunc_v2[3] = 3;
    expect_eq(parseVrrp(trunc_v2[0..20], false) == null, true);

    let trunc_v3_6: [24]u8 = [0; 24];
    trunc_v3_6[0] = 0x31; trunc_v3_6[3] = 2;
    expect_eq(parseVrrp(trunc_v3_6[0..24], true) == null, true);
});
