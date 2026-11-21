// Source: cp/cap/test/test_cdp_production_deep.cp (Production deep tests for cap:cdp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CDP_SNAP_PID,
    CDP_V1,
    CDP_V2,
    CDP_HEADER_SIZE,
    CDP_TLV_HEADER_SIZE,
    TLV_DEVICE_ID,
    TLV_ADDRESSES,
    TLV_PORT_ID,
    TLV_CAPABILITIES,
    TLV_VERSION,
    TLV_PLATFORM,
    TLV_VTP_DOMAIN,
    TLV_NATIVE_VLAN,
    TLV_DUPLEX,
    TLV_MGMT_ADDR,
    CAP_ROUTER,
    CAP_TRANSPARENT_BRIDGE,
    CAP_SOURCE_ROUTE_BRIDGE,
    CAP_SWITCH,
    CAP_HOST,
    CAP_IGMP,
    CAP_REPEATER,
    cdpChecksum,
    verifyCdpChecksum,
    CdpHeader,
    CdpTlv,
    CdpTlvIter,
    CdpPacket,
    isCdp,
    parseCdp,
    CdpWriter,
} from "cap:cdp";

test("1. CDPv2 full TLV suite encode/decode roundtrip and checksum verification", () => {
    expect_eq(CDP_SNAP_PID, 0x2000 as u16);
    expect_eq(CDP_HEADER_SIZE, 4);
    expect_eq(CDP_TLV_HEADER_SIZE, 4);

    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let w = CdpWriter.init(buf[0..], CDP_V2, 180);
    expect_eq(w.writeTlv(TLV_DEVICE_ID, "edge-rtr-01"), true);
    expect_eq(w.writeTlv(TLV_PORT_ID, "Eth1/1"), true);
    expect_eq(w.writeTlv(TLV_PLATFORM, "Cisco N9K"), true);
    expect_eq(w.writeTlv(TLV_VERSION, "NX-OS 10.3"), true);
    expect_eq(w.writeTlv(TLV_VTP_DOMAIN, "dc-east"), true);
    expect_eq(w.writeCapabilities(CAP_ROUTER | CAP_SWITCH | CAP_IGMP), true);
    expect_eq(w.writeNativeVlan(200), true);
    expect_eq(w.writeDuplex(true), true);

    const wire = w.finish() ?? unreachable;
    expect_eq(isCdp(wire), true);
    expect_eq(verifyCdpChecksum(wire), true);

    const pkt = parseCdp(wire) ?? unreachable;
    expect_eq(pkt.header.version, CDP_V2);
    expect_eq(pkt.header.ttl, 180 as u8);
    expect_eq(pkt.header.checksum, cdpChecksum(wire));

    const dev = pkt.deviceId() ?? unreachable;
    expect_eq(std.mem.eql(u8, dev, "edge-rtr-01"), true);
    const port = pkt.portId() ?? unreachable;
    expect_eq(std.mem.eql(u8, port, "Eth1/1"), true);
    const plat = pkt.platform() ?? unreachable;
    expect_eq(std.mem.eql(u8, plat, "Cisco N9K"), true);
    const ver = pkt.softwareVersion() ?? unreachable;
    expect_eq(std.mem.eql(u8, ver, "NX-OS 10.3"), true);
    const vtp = pkt.vtpDomain() ?? unreachable;
    expect_eq(std.mem.eql(u8, vtp, "dc-east"), true);
    expect_eq(pkt.capabilities() ?? 0, (CAP_ROUTER | CAP_SWITCH | CAP_IGMP));
    expect_eq(pkt.nativeVlan() ?? 0, 200 as u16);
    expect_eq(pkt.isFullDuplex() ?? false, true);

    // Count all 8 TLVs via iterator
    let it = pkt.tlvs();
    let count: usize = 0;
    while (true) {
        if (tlv := it.next()) {
            * = tlv;
            count += 1;
        } else {
            break;
        }
    }
    expect_eq(count, 8);
    expect_eq(it.hasError(), false);
});

test("2. Capabilities bitmasks, Native VLAN, and Half/Full Duplex decoding", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = CdpWriter.init(buf[0..], CDP_V1, 120);
    const all_caps = CAP_ROUTER | CAP_TRANSPARENT_BRIDGE | CAP_SOURCE_ROUTE_BRIDGE |
        CAP_SWITCH | CAP_HOST | CAP_IGMP | CAP_REPEATER;
    expect_eq(w.writeCapabilities(all_caps), true);
    expect_eq(w.writeNativeVlan(4094), true);
    expect_eq(w.writeDuplex(false), true);

    const wire = w.finish() ?? unreachable;
    const pkt = parseCdp(wire) ?? unreachable;
    expect_eq(pkt.header.version, CDP_V1);
    expect_eq(pkt.capabilities() ?? 0, all_caps);
    expect_eq(pkt.nativeVlan() ?? 0, 4094 as u16);
    expect_eq(pkt.isFullDuplex() ?? true, false);
    expect_eq(pkt.deviceId() == null, true);
});

test("3. Even-length and odd-length RFC 1071 checksum verification and tamper detection", () => {
    // Even-length packet: 4 (hdr) + 4 + 4 ("ABCD") = 12B
    let even_buf: [64]u8 = std.mem.zeroes([64]u8);
    let w_even = CdpWriter.init(even_buf[0..], CDP_V2, 60);
    expect_eq(w_even.writeTlv(TLV_DEVICE_ID, "ABCD"), true);
    const wire_even = w_even.finish() ?? unreachable;
    expect_eq(wire_even.len % 2, 0);
    expect_eq(verifyCdpChecksum(wire_even), true);

    // Tamper 1 byte -> checksum fails
    even_buf[8] = even_buf[8] ^ 0x01;
    expect_eq(verifyCdpChecksum(even_buf[0..wire_even.len]), false);
    even_buf[8] = even_buf[8] ^ 0x01;
    expect_eq(verifyCdpChecksum(even_buf[0..wire_even.len]), true);

    // Odd-length packet: 4 (hdr) + 4 + 5 ("ABCDE") = 13B
    let odd_buf: [64]u8 = std.mem.zeroes([64]u8);
    let w_odd = CdpWriter.init(odd_buf[0..], CDP_V2, 60);
    expect_eq(w_odd.writeTlv(TLV_DEVICE_ID, "ABCDE"), true);
    const wire_odd = w_odd.finish() ?? unreachable;
    expect_eq(wire_odd.len % 2, 1);
    expect_eq(verifyCdpChecksum(wire_odd), true);

    // Tamper odd trailing byte -> checksum fails
    odd_buf[wire_odd.len - 1] = odd_buf[wire_odd.len - 1] ^ 0x01;
    expect_eq(verifyCdpChecksum(odd_buf[0..wire_odd.len]), false);
});

test("4. Management Address and Addresses TLV lookup via findTlv", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let w = CdpWriter.init(buf[0..], CDP_V2, 180);
    const addr_val: [4]u8 = [0, 0, 0, 1];
    const mgmt_val: [4]u8 = [10, 1, 2, 3];
    expect_eq(w.writeTlv(TLV_ADDRESSES, addr_val[0..]), true);
    expect_eq(w.writeTlv(TLV_MGMT_ADDR, mgmt_val[0..]), true);
    const wire = w.finish() ?? unreachable;

    const pkt = parseCdp(wire) ?? unreachable;
    const t_addr = pkt.findTlv(TLV_ADDRESSES) ?? unreachable;
    expect_eq(t_addr.length, 8 as u16);
    expect_eq(t_addr.value[3], 1 as u8);

    const t_mgmt = pkt.findTlv(TLV_MGMT_ADDR) ?? unreachable;
    expect_eq(t_mgmt.value[0], 10 as u8);
    expect_eq(t_mgmt.value[3], 3 as u8);
    expect_eq(pkt.findTlv(0x9999) == null, true);
});

test("5. Malformed, truncated, invalid version, and TLV length < 4 / out-of-bounds rejection", () => {
    // 1. Shorter than 4B header
    const short3: [3]u8 = [CDP_V2, 180, 0];
    expect_eq(isCdp(short3[0..]), false);
    expect_eq(parseCdp(short3[0..]) == null, true);
    expect_eq(verifyCdpChecksum(short3[0..]), false);

    // 2. Invalid version (0 and 3)
    const bad_v0: [4]u8 = [0, 180, 0, 0];
    const bad_v3: [4]u8 = [3, 180, 0, 0];
    expect_eq(isCdp(bad_v0[0..]), false);
    expect_eq(parseCdp(bad_v0[0..]) == null, true);
    expect_eq(isCdp(bad_v3[0..]), false);
    expect_eq(parseCdp(bad_v3[0..]) == null, true);

    // 3. TLV with length < 4 (length = 3)
    const bad_tlv_len: [8]u8 = [CDP_V2, 180, 0, 0, 0x00, 0x01, 0x00, 0x03];
    expect_eq(parseCdp(bad_tlv_len[0..]) == null, true);

    // 4. TLV claiming length beyond packet bounds (length = 20, only 4B left)
    const oob_tlv: [8]u8 = [CDP_V2, 180, 0, 0, 0x00, 0x01, 0x00, 20];
    expect_eq(parseCdp(oob_tlv[0..]) == null, true);

    // 5. Trailing truncated TLV header (2 bytes after 4B CDP header)
    const trunc_tlv: [6]u8 = [CDP_V2, 180, 0, 0, 0x00, 0x01];
    expect_eq(parseCdp(trunc_tlv[0..]) == null, true);

    // 6. CdpWriter invalid version & buffer overflow protection
    let tiny: [3]u8 = [0; 3];
    let w_tiny = CdpWriter.init(tiny[0..], CDP_V2, 10);
    expect_eq(w_tiny.finish() == null, true);

    let small: [8]u8 = [0; 8];
    let w_badv = CdpWriter.init(small[0..], 9, 10);
    expect_eq(w_badv.finish() == null, true);

    let w_small = CdpWriter.init(small[0..], CDP_V2, 10);
    expect_eq(w_small.writeTlv(TLV_DEVICE_ID, "12345"), false); // needs 9B > 4B remaining
});
