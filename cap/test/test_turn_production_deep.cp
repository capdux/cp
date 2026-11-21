// Source: cp/cap/test/test_turn_production_deep.cp (Production deep tests for cap:turn)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TURN_UDP_PORT,
    TURNS_TLS_PORT,
    TURN_MAGIC_COOKIE,
    TURN_HEADER_SIZE,
    CHANNEL_DATA_HEADER_SIZE,
    CHANNEL_MIN,
    CHANNEL_MAX,
    METHOD_ALLOCATE,
    METHOD_REFRESH,
    METHOD_SEND,
    METHOD_DATA,
    METHOD_CREATE_PERMISSION,
    METHOD_CHANNEL_BIND,
    CLASS_REQUEST,
    CLASS_INDICATION,
    CLASS_SUCCESS,
    CLASS_ERROR,
    ATTR_ERROR_CODE,
    ATTR_CHANNEL_NUMBER,
    ATTR_LIFETIME,
    ATTR_XOR_PEER_ADDRESS,
    ATTR_DATA,
    ATTR_XOR_RELAYED_ADDRESS,
    ATTR_REQUESTED_ADDR_FAMILY,
    ATTR_EVEN_PORT,
    ATTR_REQUESTED_TRANSPORT,
    ATTR_DONT_FRAGMENT,
    ATTR_RESERVATION_TOKEN,
    TRANSPORT_TCP,
    TRANSPORT_UDP,
    encodeTurnMsgType,
    TurnHeader,
    TurnAttr,
    TurnAttrIter,
    TurnXorAddr,
    decodeXorAddr,
    encodeXorAddr,
    TurnMessage,
    isTurnMessage,
    parseTurnMessage,
    TurnChannelData,
    isChannelData,
    parseChannelData,
    encodeChannelData,
} from "cap:turn";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

test("1. ALLOCATE Request & Success with LIFETIME, REQUESTED-TRANSPORT, XOR-RELAYED-ADDRESS", () => {
    expect_eq(TURN_UDP_PORT, 3478 as u16);
    expect_eq(TURNS_TLS_PORT, 5349 as u16);
    expect_eq(TURN_MAGIC_COOKIE, 0x2112A442 as u32);
    expect_eq(TURN_HEADER_SIZE, 20);
    expect_eq(CHANNEL_DATA_HEADER_SIZE, 4);
    expect_eq(CHANNEL_MIN, 0x4000 as u16);
    expect_eq(CHANNEL_MAX, 0x7FFF as u16);
    expect_eq(encodeTurnMsgType(METHOD_ALLOCATE, CLASS_REQUEST), 0x0003 as u16);
    expect_eq(encodeTurnMsgType(METHOD_ALLOCATE, CLASS_SUCCESS), 0x0103 as u16);
    expect_eq(encodeTurnMsgType(METHOD_SEND, CLASS_INDICATION), 0x0016 as u16);
    expect_eq(encodeTurnMsgType(METHOD_DATA, CLASS_INDICATION), 0x0017 as u16);

    // ALLOCATE Request (0x0003) with REQUESTED-TRANSPORT=UDP(17) and LIFETIME=600
    const req: [36]u8 = [
        0x00, 0x03, 0x00, 16,             // ALLOCATE Request, len=16
        0x21, 0x12, 0xA4, 0x42,           // Magic Cookie
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
        // ATTR_REQUESTED_TRANSPORT (0x0019), len=4, proto=17(UDP)
        0x00, 0x19, 0x00, 0x04, 17, 0, 0, 0,
        // ATTR_LIFETIME (0x000D), len=4, lifetime=600 (0x00000258)
        0x00, 0x0D, 0x00, 0x04, 0x00, 0x00, 0x02, 0x58,
    ];

    expect_eq(isTurnMessage(req[0..]), true);
    const m_req = parseTurnMessage(req[0..]) ?? unreachable;
    expect_eq(m_req.header.method, METHOD_ALLOCATE);
    expect_eq(m_req.header.isRequest(), true);
    expect_eq(m_req.header.isSuccess(), false);
    expect_eq(m_req.requestedTransport() ?? 0, TRANSPORT_UDP);
    expect_eq(m_req.lifetime() ?? 0, 600 as u32);

    // ALLOCATE Success (0x0103) with XOR-RELAYED-ADDRESS (192.0.2.15:49152) + LIFETIME=600
    // 49152 (0xC000) ^ 0x2112 = 0xE112
    // [192,0,2,15] ^ [0x21,0x12,0xA4,0x42] = [0xE1, 0x12, 0xA6, 0x4D]
    const resp: [40]u8 = [
        0x01, 0x03, 0x00, 20,             // ALLOCATE Success, len=20
        0x21, 0x12, 0xA4, 0x42,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
        // ATTR_XOR_RELAYED_ADDRESS (0x0016), len=8
        0x00, 0x16, 0x00, 0x08, 0x00, 0x01, 0xE1, 0x12, 0xE1, 0x12, 0xA6, 0x4D,
        // ATTR_LIFETIME (0x000D), len=4, 600s
        0x00, 0x0D, 0x00, 0x04, 0x00, 0x00, 0x02, 0x58,
    ];
    const m_resp = parseTurnMessage(resp[0..]) ?? unreachable;
    expect_eq(m_resp.header.method, METHOD_ALLOCATE);
    expect_eq(m_resp.header.isSuccess(), true);
    expect_eq(m_resp.lifetime() ?? 0, 600 as u32);

    const relayed = m_resp.xorRelayedAddress() ?? unreachable;
    expect_eq(relayed.is_ipv6, false);
    expect_eq(relayed.port, 49152 as u16);
    const ip4 = relayed.ipv4Bytes();
    expect_eq(ip4[0], 192 as u8);
    expect_eq(ip4[1], 0 as u8);
    expect_eq(ip4[2], 2 as u8);
    expect_eq(ip4[3], 15 as u8);
});

test("2. IPv4 and IPv6 XOR-PEER-ADDRESS encode/decode round-trip", () => {
    const tx_id: [12]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88];

    // IPv4 round-trip: 203.0.113.42:3478
    let v4_addr: [16]u8 = [0; 16];
    v4_addr[0] = 203;
    v4_addr[1] = 0;
    v4_addr[2] = 113;
    v4_addr[3] = 42;
    const orig_v4 = TurnXorAddr {
        is_ipv6: false,
        port: 3478,
        addr: v4_addr,
    };
    let enc4: [8]u8 = [0; 8];
    const n4 = encodeXorAddr(orig_v4, tx_id, enc4[0..]) ?? unreachable;
    expect_eq(n4, 8);
    const dec4 = decodeXorAddr(enc4[0..n4], tx_id) ?? unreachable;
    expect_eq(dec4.is_ipv6, false);
    expect_eq(dec4.port, 3478 as u16);
    expectBytesEq(dec4.addr[0..4], orig_v4.addr[0..4]);

    // IPv6 round-trip: 2001:db8::1234:5678 port 5349
    const v6_addr: [16]u8 = [
        0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78,
    ];
    const orig_v6 = TurnXorAddr {
        is_ipv6: true,
        port: 5349,
        addr: v6_addr,
    };
    let enc6: [20]u8 = [0; 20];
    const n6 = encodeXorAddr(orig_v6, tx_id, enc6[0..]) ?? unreachable;
    expect_eq(n6, 20);
    const dec6 = decodeXorAddr(enc6[0..n6], tx_id) ?? unreachable;
    expect_eq(dec6.is_ipv6, true);
    expect_eq(dec6.port, 5349 as u16);
    expectBytesEq(dec6.addr[0..16], v6_addr[0..16]);

    // Invalid family or short buffer rejected
    let bad: [8]u8 = [0, 0x03, 0, 0, 0, 0, 0, 0];
    expect_eq(decodeXorAddr(bad[0..], tx_id) == null, true);
    let tiny: [4]u8 = [0; 4];
    expect_eq(encodeXorAddr(orig_v4, tx_id, tiny[0..]) == null, true);
});

test("3. SEND/DATA Indication with 4-byte padded DATA attribute and CHANNEL-BIND", () => {
    // DATA Indication (0x0017):
    // Attr 1: ATTR_DATA (0x0013), len=5 ("hello") + 3 bytes padding => 12 bytes
    // Attr 2: ATTR_CHANNEL_NUMBER (0x000C), len=4, ch=0x4001 => 8 bytes
    // Total attrs_len = 20 bytes
    const pkt: [40]u8 = [
        0x00, 0x17, 0x00, 20,             // DATA Indication, len=20
        0x21, 0x12, 0xA4, 0x42,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        // ATTR_DATA len=5 ("hello") + 3B zero pad
        0x00, 0x13, 0x00, 0x05, 'h', 'e', 'l', 'l', 'o', 0, 0, 0,
        // ATTR_CHANNEL_NUMBER len=4, channel=0x4001
        0x00, 0x0C, 0x00, 0x04, 0x40, 0x01, 0x00, 0x00,
    ];

    const msg = parseTurnMessage(pkt[0..]) ?? unreachable;
    expect_eq(msg.header.method, METHOD_DATA);
    expect_eq(msg.header.isIndication(), true);
    expectBytesEq(msg.dataAttr() ?? unreachable, "hello");
    expect_eq(msg.channelNumber() ?? 0, 0x4001 as u16);

    let it = msg.attrs();
    const a0 = it.next() ?? unreachable;
    expect_eq(a0.attr_type, ATTR_DATA);
    expect_eq(a0.length, 5 as u16);
    expectBytesEq(a0.value, "hello");

    const a1 = it.next() ?? unreachable;
    expect_eq(a1.attr_type, ATTR_CHANNEL_NUMBER);
    expect_eq(a1.length, 4 as u16);
    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("4. ChannelData (0x4000..0x7FFF) encode and parse round-trip", () => {
    const payload = "TURN-relayed-UDP-datagram";
    let wire: [64]u8 = [0; 64];

    const n_min = encodeChannelData(CHANNEL_MIN, payload, wire[0..]) ?? unreachable;
    expect_eq(n_min, 4 + payload.len);
    expect_eq(isChannelData(wire[0..n_min]), true);
    expect_eq(isTurnMessage(wire[0..n_min]), false);

    const cd_min = parseChannelData(wire[0..n_min]) ?? unreachable;
    expect_eq(cd_min.channel, CHANNEL_MIN);
    expect_eq(cd_min.length, payload.len as u16);
    expectBytesEq(cd_min.payload, payload);

    // Upper bound channel 0x7FFF with empty payload
    const n_max = encodeChannelData(CHANNEL_MAX, "", wire[0..]) ?? unreachable;
    expect_eq(n_max, 4);
    const cd_max = parseChannelData(wire[0..n_max]) ?? unreachable;
    expect_eq(cd_max.channel, CHANNEL_MAX);
    expect_eq(cd_max.length, 0 as u16);
    expect_eq(cd_max.payload.len, 0);
});

test("5. Malformed TURN/ChannelData rejection (cookie, alignment, channel bounds, truncation)", () => {
    // 1. Wrong magic cookie
    const bad_cookie: [20]u8 = [
        0x00, 0x03, 0x00, 0x00,
        0xDE, 0xAD, 0xBE, 0xEF,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    expect_eq(isTurnMessage(bad_cookie[0..]), false);
    expect_eq(parseTurnMessage(bad_cookie[0..]) == null, true);

    // 2. Non-zero top 2 bits in msg_type
    const bad_bits: [20]u8 = [
        0x40, 0x03, 0x00, 0x00,
        0x21, 0x12, 0xA4, 0x42,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    expect_eq(parseTurnMessage(bad_bits[0..]) == null, true);

    // 3. Length not 4-byte aligned
    const bad_align: [24]u8 = [
        0x00, 0x03, 0x00, 0x02,
        0x21, 0x12, 0xA4, 0x42,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0,
    ];
    expect_eq(parseTurnMessage(bad_align[0..]) == null, true);

    // 4. Truncated TURN message (length says 8, only 4 bytes follow)
    const trunc_turn: [24]u8 = [
        0x00, 0x03, 0x00, 0x08,
        0x21, 0x12, 0xA4, 0x42,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0,
    ];
    expect_eq(parseTurnMessage(trunc_turn[0..]) == null, true);

    // 5. ChannelData out-of-range channel numbers and truncated payload
    let out_buf: [16]u8 = [0; 16];
    expect_eq(encodeChannelData(0x3FFF, "x", out_buf[0..]) == null, true);
    expect_eq(encodeChannelData(0x8000, "x", out_buf[0..]) == null, true);
    expect_eq(encodeChannelData(0x4000, "too-long-payload!", out_buf[0..]) == null, true);

    const bad_ch_low: [6]u8 = [0x3F, 0xFF, 0x00, 0x02, 1, 2];
    expect_eq(parseChannelData(bad_ch_low[0..]) == null, true);
    const bad_ch_hi: [6]u8 = [0x80, 0x00, 0x00, 0x02, 1, 2];
    expect_eq(parseChannelData(bad_ch_hi[0..]) == null, true);
    const trunc_cd: [5]u8 = [0x40, 0x00, 0x00, 0x04, 1];
    expect_eq(parseChannelData(trunc_cd[0..]) == null, true);
});
