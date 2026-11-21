// Source: cp/cap/test/test_lldp_production_deep.cp
// Deep Production Test Suite for cap:lldp
// Covers:
//   1. Full switch/router LLDPDU round-trip (Chassis MAC + Port IfName + TTL 120 + PortDesc +
//      SystemName + SystemCaps + IEEE 802.1 & 802.3 Org-Specific TLVs + End-of-LLDPDU)
//   2. 9-bit length encoding (> 255B, e.g. 300B SystemDesc) round-trip verification
//   3. LldpTlvIter + parseLldpOrgTlv decoding IEEE 802.1 (00-80-C2) & 802.3 (00-12-0F) TLVs
//   4. Mandatory first 3 TLVs ordering enforcement (Port before Chassis, missing TTL, bad TTL len)
//   5. Non-zero length TLV_END, truncated header/payload, and malformed SystemCaps guards
//   6. LldpWriter overflow (> buffer capacity) & > 511B TLV length guards

import { test, expect_eq } from "cap:test";
import {
    LLDP_ETHERTYPE,
    TLV_END,
    TLV_CHASSIS_ID,
    TLV_PORT_ID,
    TLV_TTL,
    TLV_PORT_DESC,
    TLV_SYSTEM_NAME,
    TLV_SYSTEM_DESC,
    TLV_SYSTEM_CAPS,
    TLV_MGMT_ADDR,
    TLV_ORG_SPECIFIC,
    CAP_REPEATER,
    CAP_BRIDGE,
    CAP_WLAN_AP,
    CAP_ROUTER,
    CAP_TELEPHONE,
    CAP_STATION_ONLY,
    LldpTlv,
    LldpOrgTlv,
    LldpNeighbor,
    LldpTlvIter,
    LldpWriter,
    parseLldpOrgTlv,
    parseLldpdu,
} from "cap:lldp";

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

test("lldp: full switch/router LLDPDU round-trip + Org TLVs + End-of-LLDPDU") {
    expect_eq(LLDP_ETHERTYPE, 0x88CC);

    let buf: [512]u8 = [0; 512];
    let w = LldpWriter.init(buf[0..512]);
    // Subtype 4 = MAC address, Subtype 5 = Interface name
    expect_eq(w.writeChassisId(4, "\xAA\xBB\xCC\x11\x22\x33"), true);
    expect_eq(w.writePortId(5, "Ethernet1/48"), true);
    expect_eq(w.writeTtl(120), true);
    expect_eq(w.writeStringTlv(TLV_PORT_DESC, "Uplink to Spine-01"), true);
    expect_eq(w.writeStringTlv(TLV_SYSTEM_NAME, "leaf-sw-01.dc1"), true);
    expect_eq(w.writeStringTlv(TLV_SYSTEM_DESC, "CapOS Network Switch 4.2"), true);
    expect_eq(
        w.writeSystemCaps(CAP_BRIDGE | CAP_ROUTER | CAP_WLAN_AP, CAP_BRIDGE | CAP_ROUTER),
        true,
    );
    // IEEE 802.1 Port VLAN ID (OUI 00-80-C2, subtype 1, VLAN 100 = 0x0064)
    expect_eq(w.writeOrgTlv([0x00, 0x80, 0xC2], 1, "\x00\x64"), true);
    // IEEE 802.3 Max Frame Size (OUI 00-12-0F, subtype 4, MTU 9216 = 0x2400)
    expect_eq(w.writeOrgTlv([0x00, 0x12, 0x0F], 4, "\x24\x00"), true);

    if (pdu := w.finish()) {
        // Verify trailing 2 bytes are 0x00 0x00 End-of-LLDPDU
        expect_eq(pdu[pdu.len - 2], 0);
        expect_eq(pdu[pdu.len - 1], 0);

        if (nbr := parseLldpdu(pdu)) {
            expect_eq(nbr.chassis_subtype, 4);
            expect_eq(strEq(nbr.chassis_id, "\xAA\xBB\xCC\x11\x22\x33"), true);
            expect_eq(nbr.port_subtype, 5);
            expect_eq(strEq(nbr.port_id, "Ethernet1/48"), true);
            expect_eq(nbr.ttl_sec, 120);
            expect_eq(strEq(nbr.port_desc, "Uplink to Spine-01"), true);
            expect_eq(strEq(nbr.system_name, "leaf-sw-01.dc1"), true);
            expect_eq(strEq(nbr.system_desc, "CapOS Network Switch 4.2"), true);
            expect_eq(nbr.caps_supported, CAP_BRIDGE | CAP_ROUTER | CAP_WLAN_AP);
            expect_eq(nbr.caps_enabled, CAP_BRIDGE | CAP_ROUTER);
            expect_eq(nbr.isBridge(), true);
            expect_eq(nbr.isRouter(), true);
            expect_eq(nbr.isWlanAp(), false);
            expect_eq(nbr.has_end_tlv, true);
            expect_eq(nbr.org_tlv_count, 2);
        } else {
            expect_eq(true, false);
        }

        // Also iterate and decode Org-Specific TLVs directly
        let it = LldpTlvIter.init(pdu);
        let org_seen: u32 = 0;
        while (tlv := it.next()) {
            if (tlv.tlv_type == TLV_ORG_SPECIFIC) {
                org_seen += 1;
                if (org := parseLldpOrgTlv(tlv)) {
                    if (org_seen == 1) {
                        expect_eq(org.oui[0], 0x00);
                        expect_eq(org.oui[1], 0x80);
                        expect_eq(org.oui[2], 0xC2);
                        expect_eq(org.subtype, 1);
                        expect_eq(strEq(org.info, "\x00\x64"), true);
                    } else if (org_seen == 2) {
                        expect_eq(org.oui[0], 0x00);
                        expect_eq(org.oui[1], 0x12);
                        expect_eq(org.oui[2], 0x0F);
                        expect_eq(org.subtype, 4);
                        expect_eq(strEq(org.info, "\x24\x00"), true);
                    }
                } else {
                    expect_eq(true, false);
                }
            }
        }
        expect_eq(org_seen, 2);
        expect_eq(it.hasEndTlv(), true);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("lldp: 9-bit length encoding (> 255B) round-trip for 300-byte SystemDesc") {
    let long_desc: [300]u8 = [0x58; 300]; // 300 'X' bytes (0x012C -> bit 8 = 1, low byte = 0x2C)
    long_desc[0] = 0x41; // 'A'
    long_desc[299] = 0x5A; // 'Z'

    let buf: [512]u8 = [0; 512];
    let w = LldpWriter.init(buf[0..512]);
    expect_eq(w.writeChassisId(4, "\x01\x02\x03\x04\x05\x06"), true);
    expect_eq(w.writePortId(5, "eth0"), true);
    expect_eq(w.writeTtl(60), true);
    expect_eq(w.writeStringTlv(TLV_SYSTEM_DESC, long_desc[0..300]), true);

    if (pdu := w.finish()) {
        if (nbr := parseLldpdu(pdu)) {
            expect_eq(nbr.system_desc.len, 300);
            expect_eq(nbr.system_desc[0], 0x41);
            expect_eq(nbr.system_desc[150], 0x58);
            expect_eq(nbr.system_desc[299], 0x5A);
            expect_eq(nbr.has_end_tlv, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("lldp: parseLldpOrgTlv rejects non-org or short TLV") {
    const non_org = LldpTlv {
        tlv_type: TLV_SYSTEM_NAME,
        length: 5,
        value: "hello",
    };
    expect_eq(parseLldpOrgTlv(non_org) == null, true);

    const short_org = LldpTlv {
        tlv_type: TLV_ORG_SPECIFIC,
        length: 3,
        value: "\x00\x80\xC2",
    };
    expect_eq(parseLldpOrgTlv(short_org) == null, true);
}

test("lldp: mandatory first 3 TLVs ordering enforcement") {
    // 1. Out-of-order: Port ID before Chassis ID -> rejected
    let buf1: [64]u8 = [0; 64];
    let w1 = LldpWriter.init(buf1[0..64]);
    expect_eq(w1.writePortId(5, "eth0"), true);
    expect_eq(w1.writeChassisId(4, "\x01\x02\x03\x04\x05\x06"), true);
    expect_eq(w1.writeTtl(120), true);
    if (pdu := w1.finish()) {
        expect_eq(parseLldpdu(pdu) == null, true);
    } else {
        expect_eq(true, false);
    }

    // 2. Missing TTL (Chassis -> Port -> End) -> rejected
    let buf2: [64]u8 = [0; 64];
    let w2 = LldpWriter.init(buf2[0..64]);
    expect_eq(w2.writeChassisId(4, "\x01\x02\x03\x04\x05\x06"), true);
    expect_eq(w2.writePortId(5, "eth0"), true);
    if (pdu := w2.finish()) {
        expect_eq(parseLldpdu(pdu) == null, true);
    } else {
        expect_eq(true, false);
    }

    // 3. Invalid TTL length (3 bytes instead of 2) -> rejected
    let buf3: [64]u8 = [0; 64];
    let w3 = LldpWriter.init(buf3[0..64]);
    expect_eq(w3.writeChassisId(4, "\x01\x02\x03\x04\x05\x06"), true);
    expect_eq(w3.writePortId(5, "eth0"), true);
    expect_eq(w3.writeStringTlv(TLV_TTL, "\x00\x00\x78"), true);
    if (pdu := w3.finish()) {
        expect_eq(parseLldpdu(pdu) == null, true);
    } else {
        expect_eq(true, false);
    }
}

test("lldp: non-zero length TLV_END, truncated header/payload & bad SystemCaps guards") {
    // 1. Non-zero length TLV_END: type=0, length=2 -> 0x00 0x02 0xAA 0xBB
    let it_bad_end = LldpTlvIter.init("\x00\x02\xAA\xBB");
    expect_eq(it_bad_end.next() == null, true);
    expect_eq(it_bad_end.hasError(), true);

    // 2. Truncated 1-byte header
    let it_trunc_hdr = LldpTlvIter.init("\x02");
    expect_eq(it_trunc_hdr.next() == null, true);
    expect_eq(it_trunc_hdr.hasError(), true);

    // 3. Truncated payload (type=5, length=10, only 3 bytes available)
    let it_trunc_val = LldpTlvIter.init("\x0A\x0Aabc");
    expect_eq(it_trunc_val.next() == null, true);
    expect_eq(it_trunc_val.hasError(), true);

    // 4. Valid first 3 TLVs followed by non-zero length TLV_END -> parseLldpdu returns null
    let buf: [64]u8 = [0; 64];
    let w = LldpWriter.init(buf[0..64]);
    expect_eq(w.writeChassisId(4, "\x01\x02"), true);
    expect_eq(w.writePortId(5, "p1"), true);
    expect_eq(w.writeTtl(30), true);
    if (pdu := w.finish()) {
        // Corrupt the End-of-LLDPDU length byte from 0 to 1
        let bad: [64]u8 = [0; 64];
        let i: usize = 0;
        while (i < pdu.len) {
            bad[i] = pdu[i];
            i += 1;
        }
        bad[pdu.len - 1] = 1;
        expect_eq(parseLldpdu(bad[0..pdu.len]) == null, true);
    } else {
        expect_eq(true, false);
    }

    // 5. Malformed SystemCaps (length = 2 instead of 4) -> parseLldpdu returns null
    let buf5: [64]u8 = [0; 64];
    let w5 = LldpWriter.init(buf5[0..64]);
    expect_eq(w5.writeChassisId(4, "\x01\x02"), true);
    expect_eq(w5.writePortId(5, "p1"), true);
    expect_eq(w5.writeTtl(30), true);
    expect_eq(w5.writeStringTlv(TLV_SYSTEM_CAPS, "\x00\x04"), true);
    if (pdu := w5.finish()) {
        expect_eq(parseLldpdu(pdu) == null, true);
    } else {
        expect_eq(true, false);
    }
}

test("lldp: LldpWriter overflow and > 511B length guards") {
    // 1. Buffer too small for finish End-of-LLDPDU
    let tiny: [5]u8 = [0; 5];
    let w_tiny = LldpWriter.init(tiny[0..5]);
    expect_eq(w_tiny.writeTtl(10), true); // 4 bytes written, 1 byte left
    expect_eq(w_tiny.finish() == null, true);
    expect_eq(w_tiny.hasError(), true);

    // 2. Payload > 511B exceeds 9-bit LLDP TLV length limit
    let huge: [520]u8 = [0x41; 520];
    let out: [600]u8 = [0; 600];
    let w_huge = LldpWriter.init(out[0..600]);
    expect_eq(w_huge.writeStringTlv(TLV_SYSTEM_DESC, huge[0..520]), false);
    expect_eq(w_huge.hasError(), true);
    expect_eq(w_huge.finish() == null, true);
}
