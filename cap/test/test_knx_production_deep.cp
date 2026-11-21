// Source: cp/cap/test/test_knx_production_deep.cp (Production deep tests for cap:knx)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    KNX_UDP_PORT,
    KNX_HEADER_SIZE,
    KNX_PROTOCOL_V10,
    KNX_HPAI_SIZE,
    SVC_SEARCH_REQ,
    SVC_TUNNELLING_REQ,
    SVC_TUNNELLING_ACK,
    SVC_ROUTING_INDICATION,
    HOST_PROTO_IPV4_UDP,
    HOST_PROTO_IPV4_TCP,
    CEMI_L_DATA_REQ,
    CEMI_L_DATA_IND,
    CEMI_L_DATA_CON,
    APCI_GROUP_READ,
    APCI_GROUP_RESPONSE,
    APCI_GROUP_WRITE,
    KnxHeader,
    KnxPacket,
    KnxHpai,
    KnxConnHeader,
    KnxIndividualAddr,
    KnxGroupAddr,
    decodeIndividualAddr,
    encodeIndividualAddr,
    decodeGroupAddr,
    encodeGroupAddr,
    KnxCemi,
    isKnx,
    parseKnx,
    encodeKnxHeader,
    parseKnxHpai,
    parseKnxConnHeader,
    parseKnxCemi,
    decodeDpt9,
} from "cap:knx";

fn expectApproxEq(actual: f64, expected: f64, eps: f64) {
    const diff = if (actual >= expected) actual - expected else expected - actual;
    expect_eq(diff <= eps, true);
}

test("1. KNXnet/IP Tunnelling Request + ConnHeader + cEMI L_Data.ind GroupValueWrite (DPT9 21.5C)", () => {
    expect_eq(KNX_UDP_PORT, 3671 as u16);
    expect_eq(KNX_HEADER_SIZE, 6);
    expect_eq(KNX_PROTOCOL_V10, 0x10 as u8);
    expect_eq(KNX_HPAI_SIZE, 8);

    // Build full Tunnelling Request (6B header + 4B conn header + 13B cEMI = 23 bytes):
    // cEMI: L_Data.ind(0x29), add_len=0, ctrl1=0xBC(std, prio=3 Low), ctrl2=0xE0(group addr),
    //       src=1.1.5 (0x1105), dst=2/1/10 (0x110A), npdu_len=3, tpci=0x00, apci=0x80 (GroupWrite),
    //       payload = DPT9 21.5C (0x0C, 0x33)
    const raw_pkt: [23]u8 = [
        0x06, 0x10, 0x04, 0x20, 0x00, 23,       // KNXnet/IP header: Tunnelling Req, total=23
        0x04, 0x07, 0x03, 0x00,                 // ConnHeader: len=4, ch=7, seq=3, status=0
        0x29, 0x00,                             // cEMI L_Data.ind, add_info_len=0
        0xBC, 0xE0,                             // ctrl1 (std=1, prio=3), ctrl2 (group=1)
        0x11, 0x05,                             // src 1.1.5
        0x11, 0x0A,                             // dst 2/1/10
        0x03,                                   // npdu_len = 3 (1B apci_lo + 2B payload)
        0x00, 0x80,                             // TPCI=0, APCI=GroupValueWrite (0x080)
        0x0C, 0x33,                             // DPT9 payload: 21.5°C
    ];

    expect_eq(isKnx(raw_pkt[0..]), true);
    const pkt = parseKnx(raw_pkt[0..]) ?? unreachable;
    expect_eq(pkt.header.service_type, SVC_TUNNELLING_REQ);
    expect_eq(pkt.header.total_len, 23 as u16);
    expect_eq(pkt.body.len, 17);

    const ch = parseKnxConnHeader(pkt.body) ?? unreachable;
    expect_eq(ch.channel_id, 7 as u8);
    expect_eq(ch.seq_counter, 3 as u8);
    expect_eq(ch.status, 0 as u8);

    const cemi = parseKnxCemi(pkt.body[4..]) ?? unreachable;
    expect_eq(cemi.msg_code, CEMI_L_DATA_IND);
    expect_eq(cemi.add_info.len, 0);
    expect_eq(cemi.isStandardFrame(), true);
    expect_eq(cemi.isGroupAddress(), true);
    expect_eq(cemi.priority(), 3 as u8);
    expect_eq(cemi.src_addr, 0x1105 as u16);
    expect_eq(cemi.dst_addr, 0x110A as u16);
    expect_eq(cemi.apci, APCI_GROUP_WRITE);
    expect_eq(cemi.payload.len, 2);

    const dpt9_raw: u16 = ((cemi.payload[0] as u16) << 8) | (cemi.payload[1] as u16);
    expectApproxEq(decodeDpt9(dpt9_raw), 21.5, 0.0001);
});

test("2. Routing Indication with cEMI Additional Info, short 6-bit data, HPAI & encodeKnxHeader", () => {
    // Encode header for Search Request + 8B HPAI
    let hdr_buf: [14]u8 = [0; 14];
    const hlen = encodeKnxHeader(SVC_SEARCH_REQ, KNX_HPAI_SIZE, hdr_buf[0..]) ?? unreachable;
    expect_eq(hlen, 6);
    // Append HPAI: len=8, proto=1(UDP), ip=192.168.1.50, port=3671 (0x0E57)
    hdr_buf[6] = 8;
    hdr_buf[7] = HOST_PROTO_IPV4_UDP;
    hdr_buf[8] = 192;
    hdr_buf[9] = 168;
    hdr_buf[10] = 1;
    hdr_buf[11] = 50;
    hdr_buf[12] = 0x0E;
    hdr_buf[13] = 0x57;

    const search_pkt = parseKnx(hdr_buf[0..]) ?? unreachable;
    expect_eq(search_pkt.header.service_type, SVC_SEARCH_REQ);
    const hpai = parseKnxHpai(search_pkt.body) ?? unreachable;
    expect_eq(hpai.host_protocol, HOST_PROTO_IPV4_UDP);
    expect_eq(hpai.ip[0], 192 as u8);
    expect_eq(hpai.ip[1], 168 as u8);
    expect_eq(hpai.ip[2], 1 as u8);
    expect_eq(hpai.ip[3], 50 as u8);
    expect_eq(hpai.port, 3671 as u16);

    // Routing Indication (0x0530) with 2-byte cEMI Additional Info and 1-bit switch ON (short_data = 1)
    const routing_pkt: [19]u8 = [
        0x06, 0x10, 0x05, 0x30, 0x00, 19,
        0x29, 0x02, 0xAA, 0xBB,                 // L_Data.ind, 2B add_info
        0xB4, 0xE0,                             // std=1, prio=1 (Normal), group=1
        0x12, 0x01,                             // src
        0x08, 0x01,                             // dst
        0x01,                                   // npdu_len = 1 (short data only)
        0x00, 0x81,                             // APCI=GroupWrite (0x080) | short_data=1
    ];
    const rp = parseKnx(routing_pkt[0..]) ?? unreachable;
    expect_eq(rp.header.service_type, SVC_ROUTING_INDICATION);
    const rc = parseKnxCemi(rp.body) ?? unreachable;
    expect_eq(rc.add_info.len, 2);
    expect_eq(rc.add_info[0], 0xAA as u8);
    expect_eq(rc.priority(), 1 as u8);
    expect_eq(rc.apci, APCI_GROUP_WRITE);
    expect_eq(rc.short_data, 1 as u8);
    expect_eq(rc.payload.len, 0);
});

test("3. Individual Address (area.line.device) & 3-level Group Address (main/middle/sub) round-trip", () => {
    const raw_ind = encodeIndividualAddr(1, 2, 34);
    expect_eq(raw_ind, 0x1222 as u16);
    const ind = decodeIndividualAddr(raw_ind);
    expect_eq(ind.area, 1 as u8);
    expect_eq(ind.line, 2 as u8);
    expect_eq(ind.device, 34 as u8);

    const raw_grp = encodeGroupAddr(5, 3, 200);
    // (5 << 11) | (3 << 8) | 200 = 0x2800 | 0x0300 | 0x00C8 = 0x2BC8
    expect_eq(raw_grp, 0x2BC8 as u16);
    const grp = decodeGroupAddr(raw_grp);
    expect_eq(grp.main, 5 as u8);
    expect_eq(grp.middle, 3 as u8);
    expect_eq(grp.sub, 200 as u8);
});

test("4. DPT9 2-byte floating point decoding (0.0, +21.5C, -10.24C, -0.01C, max)", () => {
    // 0.0 -> 0x0000
    expectApproxEq(decodeDpt9(0x0000), 0.0, 0.000001);

    // +21.5°C -> exp=1, mant=+1075 (0x0433) -> 0x0C33
    expectApproxEq(decodeDpt9(0x0C33), 21.5, 0.0001);

    // -10.24°C -> sign=1, exp=0, mant=-1024 (2048-1024=0x400) -> 0x8400
    expectApproxEq(decodeDpt9(0x8400), -10.24, 0.0001);

    // -0.01°C -> sign=1, exp=0, mant=-1 (0x7FF) -> 0x87FF
    expectApproxEq(decodeDpt9(0x87FF), -0.01, 0.000001);

    // -30.0°C -> sign=1, exp=1, mant=-1500 (2048-1500=548=0x224) -> (1<<15)|(1<<11)|0x224 = 0x8A24
    expectApproxEq(decodeDpt9(0x8A24), -30.0, 0.0001);
});

test("5. Malformed KNXnet/IP, HPAI, ConnHeader & cEMI truncation rejection", () => {
    // Wrong header_len (5 instead of 6)
    const bad_hlen: [6]u8 = [0x05, 0x10, 0x04, 0x20, 0x00, 0x06];
    expect_eq(isKnx(bad_hlen[0..]), false);
    expect_eq(parseKnx(bad_hlen[0..]) == null, true);

    // Wrong protocol version (0x20 instead of 0x10)
    const bad_ver: [6]u8 = [0x06, 0x20, 0x04, 0x20, 0x00, 0x06];
    expect_eq(parseKnx(bad_ver[0..]) == null, true);

    // total_len < 6 or total_len > data.len
    const small_tlen: [6]u8 = [0x06, 0x10, 0x04, 0x20, 0x00, 0x04];
    expect_eq(parseKnx(small_tlen[0..]) == null, true);
    const over_tlen: [6]u8 = [0x06, 0x10, 0x04, 0x20, 0x00, 0x10];
    expect_eq(parseKnx(over_tlen[0..]) == null, true);

    // Bad HPAI & ConnHeader
    const bad_hpai: [8]u8 = [8, 0x99, 127, 0, 0, 1, 0x0E, 0x57];
    expect_eq(parseKnxHpai(bad_hpai[0..]) == null, true);
    const bad_ch: [4]u8 = [5, 1, 2, 0];
    expect_eq(parseKnxConnHeader(bad_ch[0..]) == null, true);

    // Truncated cEMI & npdu_len exceeding slice
    const trunc_cemi: [8]u8 = [0x29, 0x00, 0xBC, 0xE0, 0x11, 0x05, 0x11, 0x0A];
    expect_eq(parseKnxCemi(trunc_cemi[0..]) == null, true);
    const over_npdu: [11]u8 = [0x29, 0x00, 0xBC, 0xE0, 0x11, 0x05, 0x11, 0x0A, 0x05, 0x00, 0x80];
    expect_eq(parseKnxCemi(over_npdu[0..]) == null, true);

    // Small output buffer on encodeKnxHeader
    let tiny: [4]u8 = [0; 4];
    expect_eq(encodeKnxHeader(SVC_TUNNELLING_ACK, 4, tiny[0..]) == null, true);
});
