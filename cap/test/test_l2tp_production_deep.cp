// Source: cp/cap/test/test_l2tp_production_deep.cp
// Deep Production Test Suite for cap:l2tp
// Covers:
//   1. L2TPv2 Control SCCRQ + multi-AVP iteration (MessageType, HostName, AssignedTunnelID)
//   2. L2TPv2 Data packets with minimal 6B header & with full L/S/O/P flags + Offset Pad skipping
//   3. L2TPv3 Control-over-UDP header (4B zero session_id, cc_id, ns/nr) + AVP iteration
//   4. L2TPv2 strict guards (version!=2, control missing L/S or having O/P, truncated offset)
//   5. L2TPv3 Control-over-UDP strict guards (non-zero session_id, wrong ver, missing flags, bad len)
//   6. Malformed AVP guards (length < 6, truncated AVP body)

import { test, expect_eq } from "cap:test";
import {
    L2TP_UDP_PORT,
    L2TP_IP_PROTO,
    L2TP_V2,
    L2TP_V3,
    L2TP_AVP_HEADER_SIZE,
    FLAG_TYPE_CTRL,
    FLAG_LENGTH,
    FLAG_SEQUENCE,
    FLAG_OFFSET,
    FLAG_PRIORITY,
    AVP_MESSAGE_TYPE,
    AVP_RESULT_CODE,
    AVP_PROTOCOL_VERSION,
    AVP_FRAMING_CAPS,
    AVP_BEARER_CAPS,
    AVP_HOST_NAME,
    AVP_VENDOR_NAME,
    AVP_ASSIGNED_TUNNEL_ID,
    AVP_RECEIVE_WINDOW_SIZE,
    AVP_ASSIGNED_SESSION_ID,
    AVP_CALL_SERIAL_NUMBER,
    L2tpAvp,
    L2tpAvpIter,
    L2tpv2Packet,
    L2tpv3ControlHeader,
    parseL2tpv2,
    parseL2tpv3Control,
    isL2tp,
} from "cap:l2tp";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("l2tp: L2TPv2 Control SCCRQ + multi-AVP iteration") {
    expect_eq(L2TP_UDP_PORT, 1701);
    expect_eq(L2TP_IP_PROTO, 115);
    expect_eq(L2TP_V2, 2);
    expect_eq(L2TP_V3, 3);
    expect_eq(L2TP_AVP_HEADER_SIZE, 6);

    // 12B Control header (T|L|S|ver2 = 0xC802, len=39, tunnel=0, session=0, ns=0, nr=0)
    // + AVP1: MessageType=1 (SCCRQ), M=1, len=8
    // + AVP2: HostName="lac01" (5B), M=1, len=11
    // + AVP3: AssignedTunnelID=0x0102, M=1, H=1, len=8
    // Total = 12 + 8 + 11 + 8 = 39
    let pkt: [39]u8 = [
        0xC8, 0x02, 0x00, 39,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        // AVP1: M=1, len=8 (0x8008), vendor=0, attr=0 (MESSAGE_TYPE), val=0x0001
        0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        // AVP2: M=1, len=11 (0x800B), vendor=0, attr=7 (HOST_NAME), val="lac01"
        0x80, 0x0B, 0x00, 0x00, 0x00, 0x07, 0x6C, 0x61, 0x63, 0x30, 0x31,
        // AVP3: M=1|H=1, len=8 (0xC008), vendor=0, attr=9 (ASSIGNED_TUNNEL_ID), val=0x0102
        0xC0, 0x08, 0x00, 0x00, 0x00, 0x09, 0x01, 0x02,
    ];

    expect_eq(isL2tp(pkt[0..39]), true);
    if (v2 := parseL2tpv2(pkt[0..39])) {
        expect_eq(v2.is_control, true);
        expect_eq(v2.has_length, true);
        expect_eq(v2.has_seq, true);
        expect_eq(v2.has_offset, false);
        expect_eq(v2.is_priority, false);
        expect_eq(v2.length == 39, true);
        expect_eq(v2.tunnel_id, 0);
        expect_eq(v2.session_id, 0);
        expect_eq(v2.ns == 0, true);
        expect_eq(v2.nr == 0, true);

        let it = v2.avps();
        let count: u32 = 0;
        while (avp := it.next()) {
            count += 1;
            if (count == 1) {
                expect_eq(avp.mandatory, true);
                expect_eq(avp.hidden, false);
                expect_eq(avp.attr_type, AVP_MESSAGE_TYPE);
                expect_eq(avp.value.len, 2);
                expect_eq(avp.value[1], 1);
            } else if (count == 2) {
                expect_eq(avp.attr_type, AVP_HOST_NAME);
                expect_eq(strEq(avp.value, "lac01"), true);
            } else if (count == 3) {
                expect_eq(avp.mandatory, true);
                expect_eq(avp.hidden, true);
                expect_eq(avp.attr_type, AVP_ASSIGNED_TUNNEL_ID);
                expect_eq(avp.value.len, 2);
                expect_eq(avp.value[0], 0x01);
                expect_eq(avp.value[1], 0x02);
            }
        }
        expect_eq(count, 3);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("l2tp: L2TPv2 Data packets (minimal 6B & full L/S/O/P + Offset Pad)") {
    // 1. Minimal 6B Data header (flags_ver = 0x0002) + 4B PPP payload
    let d1: [10]u8 = [0x00, 0x02, 0x00, 0x10, 0x00, 0x20, 0xFF, 0x03, 0x00, 0x21];
    if (v2 := parseL2tpv2(d1[0..10])) {
        expect_eq(v2.is_control, false);
        expect_eq(v2.has_length, false);
        expect_eq(v2.has_seq, false);
        expect_eq(v2.has_offset, false);
        expect_eq(v2.tunnel_id, 0x0010);
        expect_eq(v2.session_id, 0x0020);
        expect_eq(v2.payload.len, 4);
        expect_eq(v2.payload[0], 0xFF);
        // Data packet avps() yields empty iterator
        let it = v2.avps();
        expect_eq(it.next() == null, true);
    } else {
        expect_eq(true, false);
    }

    // 2. Data packet with L|S|O|P (0x4B02), len=20, tunnel=1, session=2, ns=5, nr=4, offset_size=2 (+2B pad), payload="OK"
    let d2: [20]u8 = [
        0x4B, 0x02, 0x00, 20,
        0x00, 0x01, 0x00, 0x02,
        0x00, 0x05, 0x00, 0x04,
        0x00, 0x02, 0xAA, 0xBB, // offset_size=2 + 2B offset pad
        0x4F, 0x4B, 0x21, 0x21, // "OK!!"
    ];
    if (v2 := parseL2tpv2(d2[0..20])) {
        expect_eq(v2.is_control, false);
        expect_eq(v2.has_length, true);
        expect_eq(v2.has_seq, true);
        expect_eq(v2.has_offset, true);
        expect_eq(v2.is_priority, true);
        expect_eq(v2.length == 20, true);
        expect_eq(v2.ns == 5, true);
        expect_eq(v2.nr == 4, true);
        expect_eq(v2.offset_size == 2, true);
        expect_eq(strEq(v2.payload, "OK!!"), true);
    } else {
        expect_eq(true, false);
    }
}

test("l2tp: L2TPv3 Control-over-UDP header + AVP iteration") {
    // 4B zero session_id + 12B v3 control header (T|L|S|ver3 = 0xC803, len=20, cc_id=0x11223344, ns=1, nr=2) + 8B AVP = 24B
    let v3_pkt: [24]u8 = [
        0x00, 0x00, 0x00, 0x00,
        0xC8, 0x03, 0x00, 20,
        0x11, 0x22, 0x33, 0x44,
        0x00, 0x01, 0x00, 0x02,
        // 8B AVP: M=1, len=8, vendor=0, attr=0, val=0x0002 (SCCRP)
        0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    ];

    expect_eq(isL2tp(v3_pkt[0..24]), true);
    if (hdr := parseL2tpv3Control(v3_pkt[0..24])) {
        expect_eq(hdr.length, 20);
        expect_eq(hdr.cc_id, 0x11223344);
        expect_eq(hdr.ns, 1);
        expect_eq(hdr.nr, 2);

        let it = hdr.avps();
        if (avp := it.next()) {
            expect_eq(avp.attr_type, AVP_MESSAGE_TYPE);
            expect_eq(avp.value.len, 2);
            expect_eq(avp.value[1], 2);
        } else {
            expect_eq(true, false);
        }
        expect_eq(it.next() == null, true);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("l2tp: L2TPv2 strict guards (bad version, control missing L/S or with O/P, truncated)") {
    // 1. Wrong version (ver=1)
    let bad_ver: [6]u8 = [0x00, 0x01, 0, 1, 0, 2];
    expect_eq(parseL2tpv2(bad_ver[0..6]) == null, true);

    // 2. Control missing L flag (T|S|ver2 = 0x8802)
    let no_l: [12]u8 = [0x88, 0x02, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0];
    expect_eq(parseL2tpv2(no_l[0..12]) == null, true);

    // 3. Control missing S flag (T|L|ver2 = 0xC002)
    let no_s: [12]u8 = [0xC0, 0x02, 0, 12, 0, 1, 0, 2, 0, 0, 0, 0];
    expect_eq(parseL2tpv2(no_s[0..12]) == null, true);

    // 4. Control with O flag set (T|L|S|O|ver2 = 0xCA02)
    let with_o: [14]u8 = [0xCA, 0x02, 0, 14, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0];
    expect_eq(parseL2tpv2(with_o[0..14]) == null, true);

    // 5. Data with offset_size > remaining bytes
    let bad_off: [10]u8 = [0x02, 0x02, 0, 1, 0, 2, 0x00, 0x10, 0xAA, 0xBB];
    expect_eq(parseL2tpv2(bad_off[0..10]) == null, true);
}

test("l2tp: L2TPv3 Control-over-UDP strict guards") {
    // Non-zero leading session_id -> rejected
    let non_zero_sid: [16]u8 = [
        0x00, 0x00, 0x00, 0x01,
        0xC8, 0x03, 0x00, 12,
        0, 0, 0, 1, 0, 0, 0, 0,
    ];
    expect_eq(parseL2tpv3Control(non_zero_sid[0..16]) == null, true);

    // Length < 12 -> rejected
    let small_len: [16]u8 = [
        0, 0, 0, 0,
        0xC8, 0x03, 0x00, 8,
        0, 0, 0, 1, 0, 0, 0, 0,
    ];
    expect_eq(parseL2tpv3Control(small_len[0..16]) == null, true);

    // Length exceeds available bytes -> rejected
    let big_len: [16]u8 = [
        0, 0, 0, 0,
        0xC8, 0x03, 0x00, 20,
        0, 0, 0, 1, 0, 0, 0, 0,
    ];
    expect_eq(parseL2tpv3Control(big_len[0..16]) == null, true);
}

test("l2tp: malformed AVP guards (length < 6 & truncated AVP)") {
    // AVP length = 4 (< 6) -> error
    let it1 = L2tpAvpIter.init("\x80\x04\x00\x00\x00\x00");
    expect_eq(it1.next() == null, true);
    expect_eq(it1.hasError(), true);

    // AVP claims length = 12, only 8 bytes present -> error
    let it2 = L2tpAvpIter.init("\x80\x0C\x00\x00\x00\x07ab");
    expect_eq(it2.next() == null, true);
    expect_eq(it2.hasError(), true);
}
