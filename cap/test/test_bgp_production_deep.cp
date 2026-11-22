// Source: cp/cap/test/test_bgp_production_deep.cp (Production deep tests for cap:bgp)

import { test, expect_eq } from "cap:test";
import {
    BGP_HEADER_SIZE,
    BGP_MARKER_LEN,
    BGP_MAX_MESSAGE_LEN,
    BGP_PORT,
    ATTR_ORIGIN,
    ATTR_AS_PATH,
    ATTR_NEXT_HOP,
    ATTR_MULTI_EXIT_DISC,
    ATTR_LOCAL_PREF,
    ATTR_ATOMIC_AGGREGATE,
    ATTR_AGGREGATOR,
    ATTR_COMMUNITIES,
    BgpMsgType,
    BgpHeader,
    BgpMessage,
    BgpOptParam,
    BgpOptParamIter,
    BgpOpen,
    BgpUpdate,
    BgpPrefix,
    BgpPrefixIter,
    BgpPathAttr,
    BgpPathAttrIter,
    BgpNotification,
    BgpStreamIter,
    parseBgpMessage,
    parseBgpOpen,
    parseBgpUpdate,
    parseBgpNotification,
} from "cap:net";

fn writeBgpHeader(buf: *[]u8, off: usize, length: u16, msg_type: u8) {
    let i: usize = 0;
    while (i < 16) {
        buf.*[off + i] = 0xFF;
        i += 1;
    }
    buf.*[off + 16] = ((length >> 8) & 0xFF) as u8;
    buf.*[off + 17] = (length & 0xFF) as u8;
    buf.*[off + 18] = msg_type;
}

test("1. Multi-message TCP stream BgpStreamIter: OPEN + KEEPALIVE + UPDATE + NOTIFICATION", () => {
    expect_eq(BGP_HEADER_SIZE, 19);
    expect_eq(BGP_MARKER_LEN, 16);
    expect_eq(BGP_MAX_MESSAGE_LEN, 4096);
    expect_eq(BGP_PORT, 179);

    // 拼接 4 条完整 BGP 报文：
    // 1) OPEN: 19 + 14 (10B base + 4B opt param) = 33B @ 0..33
    // 2) KEEPALIVE: 19B @ 33..52
    // 3) UPDATE: 19 + 4 (empty) = 23B @ 52..75
    // 4) NOTIFICATION: 19 + 3 (code=6 Cease, subcode=2, data=[0xAA]) = 22B @ 75..97
    let stream: [97]u8 = [0; 97];
    let s: []u8 = stream[0..97];

    // 1) OPEN @ 0
    writeBgpHeader(*s, 0, 33, 1);
    s[19] = 4;                  // version 4
    s[20] = 0xFB; s[21] = 0xF0; // my_as = 64496
    s[22] = 0x00; s[23] = 0xB4; // hold_time = 180
    s[24] = 192; s[25] = 0; s[26] = 2; s[27] = 1; // bgp_id = 192.0.2.1
    s[28] = 4;                  // opt_param_len = 4
    s[29] = 2; s[30] = 2; s[31] = 0x01; s[32] = 0x00; // Capability TLV

    // 2) KEEPALIVE @ 33
    writeBgpHeader(*s, 33, 19, 4);

    // 3) UPDATE @ 52 (minimal empty update)
    writeBgpHeader(*s, 52, 23, 2);
    // withdrawn=0, pa=0 (already zeroed)

    // 4) NOTIFICATION @ 75
    writeBgpHeader(*s, 75, 22, 3);
    s[94] = 6;    // Cease
    s[95] = 2;    // Administrative Shutdown
    s[96] = 0xAA; // diagnostic byte

    let it = BgpStreamIter.init(stream[0..97]);

    const m1 = it.next() ?? unreachable;
    expect_eq(m1.header.msg_type == .open, true);
    const open = parseBgpOpen(m1.body) ?? unreachable;
    expect_eq(open.version, 4);
    expect_eq(open.my_as, 64496);
    expect_eq(open.hold_time, 180);
    expect_eq(open.bgp_id[0], 192);
    expect_eq(open.bgp_id[3], 1);
    let oit = open.optParams();
    const op = oit.next() ?? unreachable;
    expect_eq(op.param_type, 2);
    expect_eq(op.value.len, 2);
    expect_eq(oit.next() == null, true);
    expect_eq(oit.had_error, false);

    const m2 = it.next() ?? unreachable;
    expect_eq(m2.header.msg_type == .keepalive, true);
    expect_eq(m2.body.len, 0);

    const m3 = it.next() ?? unreachable;
    expect_eq(m3.header.msg_type == .update, true);
    const upd = parseBgpUpdate(m3.body) ?? unreachable;
    expect_eq(upd.withdrawn_routes.len, 0);
    expect_eq(upd.path_attrs.len, 0);
    expect_eq(upd.nlri.len, 0);

    const m4 = it.next() ?? unreachable;
    expect_eq(m4.header.msg_type == .notification, true);
    const notif = parseBgpNotification(m4.body) ?? unreachable;
    expect_eq(notif.error_code, 6);
    expect_eq(notif.error_subcode, 2);
    expect_eq(notif.data.len, 1);
    expect_eq(notif.data[0], 0xAA);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("2. BgpPrefixIter compressed IPv4 CIDR decoding (/0, /8, /16, /24, /32)", () => {
    const nlri: [15]u8 = [
        0,                            // 0.0.0.0/0
        8, 10,                        // 10.0.0.0/8
        16, 172, 16,                  // 172.16.0.0/16
        24, 192, 168, 1,              // 192.168.1.0/24
        32, 203, 0, 113, 42,          // 203.0.113.42/32
    ];

    let pit = BgpPrefixIter.init(nlri[0..15]);

    const p0 = pit.next() ?? unreachable;
    expect_eq(p0.prefix_len, 0);
    expect_eq(p0.addr[0], 0);
    expect_eq(p0.addr[3], 0);

    const p8 = pit.next() ?? unreachable;
    expect_eq(p8.prefix_len, 8);
    expect_eq(p8.addr[0], 10);
    expect_eq(p8.addr[1], 0);

    const p16 = pit.next() ?? unreachable;
    expect_eq(p16.prefix_len, 16);
    expect_eq(p16.addr[0], 172);
    expect_eq(p16.addr[1], 16);
    expect_eq(p16.addr[2], 0);

    const p24 = pit.next() ?? unreachable;
    expect_eq(p24.prefix_len, 24);
    expect_eq(p24.addr[0], 192);
    expect_eq(p24.addr[1], 168);
    expect_eq(p24.addr[2], 1);
    expect_eq(p24.addr[3], 0);

    const p32 = pit.next() ?? unreachable;
    expect_eq(p32.prefix_len, 32);
    expect_eq(p32.addr[0], 203);
    expect_eq(p32.addr[3], 42);

    expect_eq(pit.next() == null, true);
    expect_eq(pit.had_error, false);
});

test("3. BgpPathAttrIter standard 1B and Extended Length 2B attributes (ORIGIN/AS_PATH/NEXT_HOP/LOCAL_PREF)", () => {
    const pa: [26]u8 = [
        // 1) ORIGIN (flags=0x40 Transitive, type=1, len=1, val=0 IGP)
        0x40, ATTR_ORIGIN, 1, 0,
        // 2) AS_PATH (flags=0x50 Transitive+ExtendedLen, type=2, len=0x0004, val=[2,1,0xFB,0xF0])
        0x50, ATTR_AS_PATH, 0x00, 0x04, 2, 1, 0xFB, 0xF0,
        // 3) NEXT_HOP (flags=0x40, type=3, len=4, val=198.51.100.1)
        0x40, ATTR_NEXT_HOP, 4, 198, 51, 100, 1,
        // 4) LOCAL_PREF (flags=0xC0 Optional+Transitive, type=5, len=4, val=100)
        0xC0, ATTR_LOCAL_PREF, 4, 0, 0, 0, 100,
    ];

    let ait = BgpPathAttrIter.init(pa[0..26]);

    const a1 = ait.next() ?? unreachable;
    expect_eq(a1.type_code, ATTR_ORIGIN);
    expect_eq(a1.isOptional(), false);
    expect_eq(a1.isTransitive(), true);
    expect_eq(a1.isExtendedLen(), false);
    expect_eq(a1.value.len, 1);
    expect_eq(a1.value[0], 0);

    const a2 = ait.next() ?? unreachable;
    expect_eq(a2.type_code, ATTR_AS_PATH);
    expect_eq(a2.isExtendedLen(), true);
    expect_eq(a2.value.len, 4);
    expect_eq(a2.value[2], 0xFB);

    const a3 = ait.next() ?? unreachable;
    expect_eq(a3.type_code, ATTR_NEXT_HOP);
    expect_eq(a3.value.len, 4);
    expect_eq(a3.value[0], 198);

    const a4 = ait.next() ?? unreachable;
    expect_eq(a4.type_code, ATTR_LOCAL_PREF);
    expect_eq(a4.isOptional(), true);
    expect_eq(a4.isTransitive(), true);
    expect_eq(a4.value[3], 100);

    expect_eq(ait.next() == null, true);
    expect_eq(ait.had_error, false);
});

test("4. Full UPDATE with withdrawn routes, path attributes, and NLRI combined", () => {
    // withdrawn = [24, 10, 1, 2] (4B)
    // path_attrs = [0x40, 1, 1, 0] (4B)
    // nlri = [24, 10, 2, 3] (4B)
    const upd_body: [16]u8 = [
        0x00, 0x04, 24, 10, 1, 2,
        0x00, 0x04, 0x40, ATTR_ORIGIN, 1, 0,
        24, 10, 2, 3,
    ];
    const u = parseBgpUpdate(upd_body[0..16]) ?? unreachable;
    let wit = BgpPrefixIter.init(u.withdrawn_routes);
    const w = wit.next() ?? unreachable;
    expect_eq(w.prefix_len, 24);
    expect_eq(w.addr[0], 10);
    expect_eq(w.addr[2], 2);

    let nit = BgpPrefixIter.init(u.nlri);
    const n = nit.next() ?? unreachable;
    expect_eq(n.prefix_len, 24);
    expect_eq(n.addr[2], 3);
});

test("5. Malformed Marker, length < 19, bad KEEPALIVE length, prefix_len > 32, and truncated payloads", () => {
    let bad_marker: [19]u8 = [0xFF; 19];
    bad_marker[7] = 0xFE;
    bad_marker[16] = 0; bad_marker[17] = 19; bad_marker[18] = 4;
    expect_eq(parseBgpMessage(bad_marker[0..19]) == null, true);

    let short_len: [19]u8 = [0xFF; 19];
    short_len[16] = 0; short_len[17] = 18; short_len[18] = 4;
    expect_eq(parseBgpMessage(short_len[0..19]) == null, true);

    // KEEPALIVE length != 19 必须拒绝
    let bad_ka: [20]u8 = [0xFF; 20];
    bad_ka[16] = 0; bad_ka[17] = 20; bad_ka[18] = 4;
    expect_eq(parseBgpMessage(bad_ka[0..20]) == null, true);

    // prefix_len > 32
    const bad_pfx: [5]u8 = [33, 1, 2, 3, 4];
    let pit = BgpPrefixIter.init(bad_pfx[0..5]);
    expect_eq(pit.next() == null, true);
    expect_eq(pit.had_error, true);

    // 截断 Path Attribute
    const trunc_pa: [4]u8 = [0x40, ATTR_NEXT_HOP, 4, 198];
    let ait = BgpPathAttrIter.init(trunc_pa[0..4]);
    expect_eq(ait.next() == null, true);
    expect_eq(ait.had_error, true);
});
