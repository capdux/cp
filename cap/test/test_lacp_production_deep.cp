// Source: cp/cap/test/test_lacp_production_deep.cp (Production deep tests for cap:lacp)

import { test, expect_eq } from "cap:test";
import {
    SLOW_PROTO_ETHERTYPE,
    LACP_SUBTYPE,
    LACP_VERSION,
    LACP_MIN_LEN,
    LACP_FULL_LEN,
    TLV_TERMINATOR,
    TLV_ACTOR,
    TLV_PARTNER,
    TLV_COLLECTOR,
    STATE_ACTIVITY,
    STATE_TIMEOUT,
    STATE_AGGREGATION,
    STATE_SYNC,
    STATE_COLLECTING,
    STATE_DISTRIBUTING,
    STATE_DEFAULTED,
    STATE_EXPIRED,
    LacpPeerInfo,
    LacpPdu,
    isLacp,
    parseLacp,
    encodeLacp,
} from "cap:lacp";

test("1. Standard LACPDU encode/parse roundtrip (110B) with Actor, Partner & Collector Max Delay", () => {
    expect_eq(SLOW_PROTO_ETHERTYPE, 0x8809);
    expect_eq(LACP_SUBTYPE, 0x01);
    expect_eq(LACP_VERSION, 0x01);
    expect_eq(LACP_MIN_LEN, 110);
    expect_eq(LACP_FULL_LEN, 124);

    const pdu = LacpPdu {
        subtype: LACP_SUBTYPE,
        version: LACP_VERSION,
        actor: LacpPeerInfo {
            system_priority: 32768,
            system_id: [0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E],
            key: 17,
            port_priority: 255,
            port_number: 3,
            state: STATE_ACTIVITY | STATE_AGGREGATION | STATE_SYNC | STATE_COLLECTING | STATE_DISTRIBUTING,
        },
        partner: LacpPeerInfo {
            system_priority: 4096,
            system_id: [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF],
            key: 25,
            port_priority: 128,
            port_number: 12,
            state: STATE_ACTIVITY | STATE_TIMEOUT | STATE_AGGREGATION | STATE_SYNC,
        },
        collector_max_delay: 8000,
    };

    let buf: [124]u8 = [0xFF; 124];
    let out: []u8 = buf[0..124];
    const n = encodeLacp(pdu, *out) ?? unreachable;
    expect_eq(n, LACP_MIN_LEN);

    // 验证所有 Reserved 尾部被正确清零
    expect_eq(out[58], TLV_TERMINATOR);
    expect_eq(out[59], 0);
    expect_eq(out[109], 0);

    expect_eq(isLacp(out[0..n]), true);
    const decoded = parseLacp(out[0..n]) ?? unreachable;
    expect_eq(decoded.subtype, LACP_SUBTYPE);
    expect_eq(decoded.version, LACP_VERSION);
    expect_eq(decoded.collector_max_delay, 8000);

    // Actor 字段校验
    expect_eq(decoded.actor.system_priority, 32768);
    expect_eq(decoded.actor.system_id[0], 0x00);
    expect_eq(decoded.actor.system_id[5], 0x5E);
    expect_eq(decoded.actor.key, 17);
    expect_eq(decoded.actor.port_priority, 255);
    expect_eq(decoded.actor.port_number, 3);

    // Partner 字段校验
    expect_eq(decoded.partner.system_priority, 4096);
    expect_eq(decoded.partner.system_id[0], 0xAA);
    expect_eq(decoded.partner.system_id[5], 0xFF);
    expect_eq(decoded.partner.key, 25);
    expect_eq(decoded.partner.port_priority, 128);
    expect_eq(decoded.partner.port_number, 12);
});

test("2. Exact verification of all 8 LACP Actor/Partner state flags", () => {
    const all_on = LacpPeerInfo {
        system_priority: 1,
        system_id: [0; 6],
        key: 1,
        port_priority: 1,
        port_number: 1,
        state: 0xFF,
    };
    expect_eq(all_on.isActive(), true);
    expect_eq(all_on.isShortTimeout(), true);
    expect_eq(all_on.isAggregatable(), true);
    expect_eq(all_on.isInSync(), true);
    expect_eq(all_on.isCollecting(), true);
    expect_eq(all_on.isDistributing(), true);
    expect_eq(all_on.isDefaulted(), true);
    expect_eq(all_on.isExpired(), true);

    const all_off = LacpPeerInfo {
        system_priority: 1,
        system_id: [0; 6],
        key: 1,
        port_priority: 1,
        port_number: 1,
        state: 0x00,
    };
    expect_eq(all_off.isActive(), false);
    expect_eq(all_off.isShortTimeout(), false);
    expect_eq(all_off.isAggregatable(), false);
    expect_eq(all_off.isInSync(), false);
    expect_eq(all_off.isCollecting(), false);
    expect_eq(all_off.isDistributing(), false);
    expect_eq(all_off.isDefaulted(), false);
    expect_eq(all_off.isExpired(), false);

    // 逐位独立验证
    const only_defaulted_expired = LacpPeerInfo {
        system_priority: 0,
        system_id: [0; 6],
        key: 0,
        port_priority: 0,
        port_number: 0,
        state: STATE_DEFAULTED | STATE_EXPIRED,
    };
    expect_eq(only_defaulted_expired.isActive(), false);
    expect_eq(only_defaulted_expired.isDistributing(), false);
    expect_eq(only_defaulted_expired.isDefaulted(), true);
    expect_eq(only_defaulted_expired.isExpired(), true);
});

test("3. Parsing raw wire bytes (110B & padded 124B slice)", () => {
    let raw: [124]u8 = [0; 124];
    raw[0] = 0x01; // LACP subtype
    raw[1] = 0x01; // version 1
    // Actor TLV
    raw[2] = 0x01; raw[3] = 20;
    raw[4] = 0x80; raw[5] = 0x00; // sys_prio = 32768
    raw[6] = 0x10; raw[11] = 0x20;
    raw[12] = 0x00; raw[13] = 0x05; // key = 5
    raw[14] = 0x00; raw[15] = 0x80; // port_prio = 128
    raw[16] = 0x00; raw[17] = 0x02; // port = 2
    raw[18] = STATE_ACTIVITY | STATE_TIMEOUT;
    // Partner TLV
    raw[22] = 0x02; raw[23] = 20;
    raw[24] = 0x00; raw[25] = 0x01;
    // Collector TLV
    raw[42] = 0x03; raw[43] = 16;
    raw[44] = 0x01; raw[45] = 0xF4; // collector_max_delay = 500
    // Terminator TLV (already 0x00, 0x00)

    const p = parseLacp(raw[0..124]) ?? unreachable;
    expect_eq(p.actor.system_priority, 32768);
    expect_eq(p.actor.key, 5);
    expect_eq(p.actor.port_number, 2);
    expect_eq(p.actor.isActive(), true);
    expect_eq(p.actor.isShortTimeout(), true);
    expect_eq(p.actor.isInSync(), false);
    expect_eq(p.partner.system_priority, 1);
    expect_eq(p.collector_max_delay, 500);
});

test("4. Output buffer too small rejected by encodeLacp", () => {
    const pdu = LacpPdu {
        subtype: LACP_SUBTYPE,
        version: LACP_VERSION,
        actor: LacpPeerInfo {
            system_priority: 32768,
            system_id: [0; 6],
            key: 1,
            port_priority: 128,
            port_number: 1,
            state: STATE_ACTIVITY,
        },
        partner: LacpPeerInfo {
            system_priority: 0,
            system_id: [0; 6],
            key: 0,
            port_priority: 0,
            port_number: 0,
            state: 0,
        },
        collector_max_delay: 0,
    };
    let small: [109]u8 = [0; 109];
    let s: []u8 = small[0..109];
    expect_eq(encodeLacp(pdu, *s) == null, true);
});

test("5. Malformed LACPDU defense: truncated, bad subtype/version, corrupt TLV type/len", () => {
    // 5.1 截断 (< 110B)
    let trunc: [109]u8 = [0; 109];
    trunc[0] = 1; trunc[1] = 1; trunc[2] = 1; trunc[3] = 20;
    trunc[22] = 2; trunc[23] = 20; trunc[42] = 3; trunc[43] = 16;
    expect_eq(isLacp(trunc[0..109]), false);
    expect_eq(parseLacp(trunc[0..109]) == null, true);

    // 构造一份合法 110B 基线
    let base: [110]u8 = [0; 110];
    base[0] = 1; base[1] = 1;
    base[2] = 1; base[3] = 20;
    base[22] = 2; base[23] = 20;
    base[42] = 3; base[43] = 16;
    expect_eq(isLacp(base[0..110]), true);

    // 5.2 错误 subtype (Marker Protocol = 0x02)
    base[0] = 0x02;
    expect_eq(parseLacp(base[0..110]) == null, true);
    base[0] = 0x01;

    // 5.3 错误 version (0x02)
    base[1] = 0x02;
    expect_eq(parseLacp(base[0..110]) == null, true);
    base[1] = 0x01;

    // 5.4 错误 Actor TLV length (19 != 20)
    base[3] = 19;
    expect_eq(parseLacp(base[0..110]) == null, true);
    base[3] = 20;

    // 5.5 错误 Partner TLV type (0x05 != 0x02)
    base[22] = 0x05;
    expect_eq(parseLacp(base[0..110]) == null, true);
    base[22] = 0x02;

    // 5.6 错误 Collector TLV length (12 != 16)
    base[43] = 12;
    expect_eq(parseLacp(base[0..110]) == null, true);
    base[43] = 16;

    // 5.7 错误 Terminator TLV (非 0x00/0x00)
    base[59] = 1;
    expect_eq(parseLacp(base[0..110]) == null, true);
});
