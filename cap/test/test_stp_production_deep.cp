// Source: cp/cap/test/test_stp_production_deep.cp (Production deep tests for cap:stp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    STP_PROTOCOL_ID,
    STP_VERSION_STP,
    STP_VERSION_RSTP,
    STP_VERSION_MSTP,
    BPDU_TYPE_CONFIG,
    BPDU_TYPE_RST,
    BPDU_TYPE_TCN,
    STP_TCN_SIZE,
    STP_CONFIG_SIZE,
    STP_RST_SIZE,
    FLAG_TC,
    FLAG_PROPOSAL,
    FLAG_LEARNING,
    FLAG_FORWARDING,
    FLAG_AGREEMENT,
    FLAG_TCA,
    StpPortRole,
    stpPortRoleFromFlags,
    StpBridgeId,
    parseBridgeId,
    encodeBridgeId,
    StpBpdu,
    isBpdu,
    parseBpdu,
    encodeConfigBpdu,
} from "cap:stp";

test("1. Classic 35B IEEE 802.1D STP Config BPDU roundtrip and 1/256s timer conversions", () => {
    expect_eq(STP_PROTOCOL_ID, 0 as u16);
    expect_eq(STP_TCN_SIZE, 4);
    expect_eq(STP_CONFIG_SIZE, 35);
    expect_eq(STP_RST_SIZE, 36);

    const root = StpBridgeId{
        .priority = 4096,
        .system_id_ext = 1,
        .mac = [0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E],
    };
    const bridge = StpBridgeId{
        .priority = 32768,
        .system_id_ext = 1,
        .mac = [0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE],
    };
    const bpdu_in = StpBpdu{
        .version = STP_VERSION_STP,
        .bpdu_type = BPDU_TYPE_CONFIG,
        .is_tcn = false,
        .flags = FLAG_TC | FLAG_TCA,
        .root_id = root,
        .root_path_cost = 19,
        .bridge_id = bridge,
        .port_id = 0x8001, // priority = 128, port = 1
        .message_age_ticks = 256,      // 1.0 s
        .max_age_ticks = 5120,         // 20.0 s
        .hello_time_ticks = 512,       // 2.0 s
        .forward_delay_ticks = 3840,   // 15.0 s
        .version_1_len = 0,
    };

    let out: [64]u8 = std.mem.zeroes([64]u8);
    const n = encodeConfigBpdu(bpdu_in, out[0..]) ?? unreachable;
    expect_eq(n, 35);
    expect_eq(isBpdu(out[0..n]), true);

    const parsed = parseBpdu(out[0..n]) ?? unreachable;
    expect_eq(parsed.version, STP_VERSION_STP);
    expect_eq(parsed.bpdu_type, BPDU_TYPE_CONFIG);
    expect_eq(parsed.isTcn(), false);
    expect_eq(parsed.isRstp(), false);
    expect_eq(parsed.isTopologyChange(), true);
    expect_eq(parsed.isTopologyChangeAck(), true);
    expect_eq(parsed.root_path_cost, 19 as u32);
    expect_eq(parsed.portPriority(), 128 as u8);
    expect_eq(parsed.portNumber(), 1 as u16);

    // 1/256s timer conversions
    expect_eq(parsed.messageAgeSec() > 0.99 && parsed.messageAgeSec() < 1.01, true);
    expect_eq(parsed.maxAgeSec() > 19.99 && parsed.maxAgeSec() < 20.01, true);
    expect_eq(parsed.helloTimeSec() > 1.99 && parsed.helloTimeSec() < 2.01, true);
    expect_eq(parsed.forwardDelaySec() > 14.99 && parsed.forwardDelaySec() < 15.01, true);
});

test("2. 36B IEEE 802.1w RSTP BPDU (designated & root port roles, proposal/agreement/forwarding)", () => {
    const bid = StpBridgeId{
        .priority = 8192,
        .system_id_ext = 10,
        .mac = [0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01],
    };

    // Designated role = 3 -> (3 << 2) = 0x0C
    const desig_flags: u8 = (0x0C as u8) | FLAG_PROPOSAL | FLAG_LEARNING | FLAG_FORWARDING;
    const rstp_desig = StpBpdu{
        .version = STP_VERSION_RSTP,
        .bpdu_type = BPDU_TYPE_RST,
        .is_tcn = false,
        .flags = desig_flags,
        .root_id = bid,
        .root_path_cost = 0,
        .bridge_id = bid,
        .port_id = 0x8018, // priority 128, port 24
        .message_age_ticks = 0,
        .max_age_ticks = 5120,
        .hello_time_ticks = 512,
        .forward_delay_ticks = 3840,
        .version_1_len = 0,
    };

    let out: [64]u8 = std.mem.zeroes([64]u8);
    const n = encodeConfigBpdu(rstp_desig, out[0..]) ?? unreachable;
    expect_eq(n, 36);
    expect_eq(out[35], 0 as u8);

    const d1 = parseBpdu(out[0..n]) ?? unreachable;
    expect_eq(d1.isRstp(), true);
    expect_eq(d1.portRole(), StpPortRole.designated);
    expect_eq(d1.isProposal(), true);
    expect_eq(d1.isAgreement(), false);
    expect_eq(d1.isLearning(), true);
    expect_eq(d1.isForwarding(), true);
    expect_eq(d1.portNumber(), 24 as u16);

    // Root role = 2 -> (2 << 2) = 0x08 + Agreement
    const root_flags: u8 = (0x08 as u8) | FLAG_AGREEMENT | FLAG_FORWARDING;
    expect_eq(stpPortRoleFromFlags(root_flags), StpPortRole.root);
    out[4] = root_flags;
    const d2 = parseBpdu(out[0..n]) ?? unreachable;
    expect_eq(d2.portRole(), StpPortRole.root);
    expect_eq(d2.isAgreement(), true);
    expect_eq(d2.isProposal(), false);
    expect_eq(d2.isForwarding(), true);

    // Alternate/Backup role = 1 -> (1 << 2) = 0x04
    expect_eq(stpPortRoleFromFlags(0x04), StpPortRole.alternate_backup);
    expect_eq(stpPortRoleFromFlags(0x00), StpPortRole.unknown);
});

test("3. 4B TCN (Topology Change Notification) BPDU encode and decode", () => {
    const tcn_in = StpBpdu{
        .version = STP_VERSION_STP,
        .bpdu_type = BPDU_TYPE_TCN,
        .is_tcn = true,
        .flags = 0,
        .root_id = StpBridgeId.zero(),
        .root_path_cost = 0,
        .bridge_id = StpBridgeId.zero(),
        .port_id = 0,
        .message_age_ticks = 0,
        .max_age_ticks = 0,
        .hello_time_ticks = 0,
        .forward_delay_ticks = 0,
        .version_1_len = 0,
    };

    let buf: [8]u8 = std.mem.zeroes([8]u8);
    const n = encodeConfigBpdu(tcn_in, buf[0..]) ?? unreachable;
    expect_eq(n, 4);
    expect_eq(buf[0], 0x00 as u8);
    expect_eq(buf[1], 0x00 as u8);
    expect_eq(buf[2], STP_VERSION_STP);
    expect_eq(buf[3], BPDU_TYPE_TCN);
    expect_eq(isBpdu(buf[0..n]), true);

    const tcn = parseBpdu(buf[0..n]) ?? unreachable;
    expect_eq(tcn.isTcn(), true);
    expect_eq(tcn.isRstp(), false);
    expect_eq(tcn.bpdu_type, BPDU_TYPE_TCN);
});

test("4. IEEE 802.1t StpBridgeId (priority=32768, VLAN ID=100, MAC) roundtrip", () => {
    const bid = StpBridgeId{
        .priority = 32768,
        .system_id_ext = 100,
        .mac = [0x00, 0x1B, 0x21, 0x3C, 0x4D, 0x5F],
    };
    let raw: [8]u8 = std.mem.zeroes([8]u8);
    expect_eq(encodeBridgeId(bid, raw[0..]), true);

    // 32768 (0x8000) | 100 (0x0064) = 0x8064
    expect_eq(raw[0], 0x80 as u8);
    expect_eq(raw[1], 0x64 as u8);
    expect_eq(raw[2], 0x00 as u8);
    expect_eq(raw[7], 0x5F as u8);

    const parsed = parseBridgeId(raw[0..]) ?? unreachable;
    expect_eq(parsed.priority, 32768 as u16);
    expect_eq(parsed.system_id_ext, 100 as u16);
    expect_eq(std.mem.eql(u8, parsed.mac[0..], bid.mac[0..]), true);

    // Too short buffer
    expect_eq(parseBridgeId(raw[0..7]) == null, true);
    expect_eq(encodeBridgeId(bid, raw[0..7]), false);
});

test("5. Non-zero protocol_id, truncated BPDU, unknown bpdu_type, and short output buffer resilience", () => {
    // Non-zero protocol_id
    const bad_proto: [4]u8 = [0x00, 0x01, 0x00, BPDU_TYPE_TCN];
    expect_eq(isBpdu(bad_proto[0..]), false);
    expect_eq(parseBpdu(bad_proto[0..]) == null, true);

    // Unknown bpdu_type (0x7F)
    const bad_type: [36]u8 = [0, 0, 0, 0x7F, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(isBpdu(bad_type[0..]), false);
    expect_eq(parseBpdu(bad_type[0..]) == null, true);

    // Truncated Config BPDU (34B < 35B)
    let trunc_cfg: [34]u8 = std.mem.zeroes([34]u8);
    trunc_cfg[3] = BPDU_TYPE_CONFIG;
    expect_eq(isBpdu(trunc_cfg[0..]), false);
    expect_eq(parseBpdu(trunc_cfg[0..]) == null, true);

    // Truncated RSTP BPDU (35B < 36B)
    let trunc_rst: [35]u8 = std.mem.zeroes([35]u8);
    trunc_rst[2] = STP_VERSION_RSTP;
    trunc_rst[3] = BPDU_TYPE_RST;
    expect_eq(isBpdu(trunc_rst[0..]), false);
    expect_eq(parseBpdu(trunc_rst[0..]) == null, true);

    // Truncated (< 4B)
    const tiny3: [3]u8 = [0, 0, 0];
    expect_eq(isBpdu(tiny3[0..]), false);
    expect_eq(parseBpdu(tiny3[0..]) == null, true);

    // Encoder buffer too small for RSTP (35B < 36B)
    const rst_bpdu = StpBpdu{
        .version = STP_VERSION_RSTP,
        .bpdu_type = BPDU_TYPE_RST,
        .is_tcn = false,
        .flags = 0,
        .root_id = StpBridgeId.zero(),
        .root_path_cost = 0,
        .bridge_id = StpBridgeId.zero(),
        .port_id = 0,
        .message_age_ticks = 0,
        .max_age_ticks = 0,
        .hello_time_ticks = 0,
        .forward_delay_ticks = 0,
        .version_1_len = 0,
    };
    let short_out: [35]u8 = std.mem.zeroes([35]u8);
    expect_eq(encodeConfigBpdu(rst_bpdu, short_out[0..]) == null, true);
});
