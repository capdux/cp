// Source: cp/cap/test/test_igmp_production_deep.cp
// Deep Production Test Suite for cap:igmp
// Covers:
//   1. IGMPv2 Join (0x16), Leave (0x17) & Query (0x11) 8B round-trip + RFC 1071 checksum
//   2. IGMPv3 Membership Query (S-bit, QRV, QQIC, 2 IPv4 source addresses) parsing & checksum
//   3. IGMPv3 Membership Report (0x22) with multiple Group Records, sourceAt() & Aux Data skipping
//   4. igmpDecodeFloatCode (< 128 linear & >= 128 exponent/mantissa RFC 3376 decoding)
//   5. Tampered checksum, truncated packet, invalid 9..11B query, and out-of-bounds num_sources guards
//   6. Malformed v3 Group Record iterator error propagation & encodeIgmpV2 argument guards

import { test, expect_eq } from "cap:test";
import {
    IGMP_IP_PROTO,
    IGMP_MIN_SIZE,
    IGMP_V3_QUERY_MIN_SIZE,
    IgmpType,
    IgmpV3RecordType,
    IgmpV3GroupRecord,
    IgmpPacket,
    IgmpV3RecordIter,
    igmpChecksum,
    verifyIgmpChecksum,
    igmpDecodeFloatCode,
    parseIgmp,
    encodeIgmpV2,
} from "cap:net";

fn ip4Eq(a: [4]u8, b: [4]u8): bool {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

test("igmp: IGMPv2 Join / Leave / Query 8B round-trip + RFC 1071 checksum") {
    expect_eq(IGMP_IP_PROTO, 2);
    expect_eq(IGMP_MIN_SIZE, 8);
    expect_eq(IGMP_V3_QUERY_MIN_SIZE, 12);

    // 1. v2 Membership Report (Join 224.0.1.60)
    let buf1: [16]u8 = [0; 16];
    if (n := encodeIgmpV2(IgmpType.v2_report, 0, [224, 0, 1, 60], buf1[0..16])) {
        expect_eq(n, 8);
        expect_eq(verifyIgmpChecksum(buf1[0..n]), true);
        if (pkt := parseIgmp(buf1[0..n])) {
            expect_eq(pkt.msg_type == IgmpType.v2_report, true);
            expect_eq(pkt.raw_type, 0x16);
            expect_eq(pkt.max_resp_code, 0);
            expect_eq(pkt.is_v3_query, false);
            expect_eq(ip4Eq(pkt.group_addr, [224, 0, 1, 60]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 2. v2 Leave Group (239.255.255.250)
    let buf2: [16]u8 = [0; 16];
    if (n := encodeIgmpV2(IgmpType.v2_leave, 0, [239, 255, 255, 250], buf2[0..16])) {
        expect_eq(verifyIgmpChecksum(buf2[0..n]), true);
        if (pkt := parseIgmp(buf2[0..n])) {
            expect_eq(pkt.msg_type == IgmpType.v2_leave, true);
            expect_eq(ip4Eq(pkt.group_addr, [239, 255, 255, 250]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 3. v2 General Query (max_resp_code = 100 -> 10.0s, group = 0.0.0.0)
    let buf3: [16]u8 = [0; 16];
    if (n := encodeIgmpV2(IgmpType.membership_query, 100, [0, 0, 0, 0], buf3[0..16])) {
        expect_eq(verifyIgmpChecksum(buf3[0..n]), true);
        if (pkt := parseIgmp(buf3[0..n])) {
            expect_eq(pkt.msg_type == IgmpType.membership_query, true);
            expect_eq(pkt.max_resp_code, 100);
            expect_eq(pkt.is_v3_query, false);
            expect_eq(ip4Eq(pkt.group_addr, [0, 0, 0, 0]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("igmp: IGMPv3 Membership Query (S-bit, QRV, QQIC, 2 sources)") {
    // 12B header + 2 * 4B sources = 20B
    let q: [20]u8 = [
        0x11, 0x64, 0x00, 0x00, // type=0x11, max_resp=100, csum placeholder
        232, 1, 2, 3,           // group 232.1.2.3
        0x0A, 125, 0x00, 0x02,  // S=1 (0x08) | QRV=2 (0x02) = 0x0A, QQIC=125, num_sources=2
        10, 0, 0, 1,            // source[0] = 10.0.0.1
        10, 0, 0, 2,            // source[1] = 10.0.0.2
    ];
    const csum = igmpChecksum(q[0..20]);
    q[2] = ((csum >> 8) & 0xFF) as u8;
    q[3] = (csum & 0xFF) as u8;

    expect_eq(verifyIgmpChecksum(q[0..20]), true);
    if (pkt := parseIgmp(q[0..20])) {
        expect_eq(pkt.msg_type == IgmpType.membership_query, true);
        expect_eq(pkt.is_v3_query, true);
        expect_eq(pkt.max_resp_code, 100);
        expect_eq(ip4Eq(pkt.group_addr, [232, 1, 2, 3]), true);
        expect_eq(pkt.suppress_router_side, true);
        expect_eq(pkt.qrv, 2);
        expect_eq(pkt.qqic, 125);
        expect_eq(pkt.num_sources, 2);

        if (s0 := pkt.querySourceAt(0)) {
            expect_eq(ip4Eq(s0, [10, 0, 0, 1]), true);
        } else {
            expect_eq(true, false);
        }
        if (s1 := pkt.querySourceAt(1)) {
            expect_eq(ip4Eq(s1, [10, 0, 0, 2]), true);
        } else {
            expect_eq(true, false);
        }
        expect_eq(pkt.querySourceAt(2) == null, true);
    } else {
        expect_eq(true, false);
    }
}

test("igmp: IGMPv3 Membership Report multi Group Record + sourceAt + Aux skipping") {
    // 8B header + Rec1 (8B + 2*4B sources + 1*4B aux = 20B) + Rec2 (8B + 0 sources + 0 aux = 8B) = 36B
    let rep: [36]u8 = [
        0x22, 0x00, 0x00, 0x00, // type=0x22, resv, csum placeholder
        0x00, 0x00, 0x00, 0x02, // resv, num_group_records = 2
        // Record 1: ALLOW_NEW_SOURCES (5), aux_words=1 (4B), num_sources=2, group 232.10.20.30
        0x05, 0x01, 0x00, 0x02,
        232, 10, 20, 30,
        192, 168, 1, 10,        // source[0]
        192, 168, 1, 20,        // source[1]
        0xDE, 0xAD, 0xBE, 0xEF, // 1 word aux data
        // Record 2: CHANGE_TO_EXCLUDE (4), aux_words=0, num_sources=0, group 239.0.0.1
        0x04, 0x00, 0x00, 0x00,
        239, 0, 0, 1,
    ];
    const csum = igmpChecksum(rep[0..36]);
    rep[2] = ((csum >> 8) & 0xFF) as u8;
    rep[3] = (csum & 0xFF) as u8;

    expect_eq(verifyIgmpChecksum(rep[0..36]), true);
    if (pkt := parseIgmp(rep[0..36])) {
        expect_eq(pkt.msg_type == IgmpType.v3_report, true);
        expect_eq(pkt.num_group_records, 2);

        let it = pkt.v3Records();
        let count: u32 = 0;
        while (rec := it.next()) {
            count += 1;
            if (count == 1) {
                expect_eq(rec.rec_type == IgmpV3RecordType.allow_new_sources, true);
                expect_eq(rec.aux_data_words, 1);
                expect_eq(rec.aux_slice.len, 4);
                expect_eq(rec.num_sources, 2);
                expect_eq(ip4Eq(rec.multicast_addr, [232, 10, 20, 30]), true);
                if (s0 := rec.sourceAt(0)) {
                    expect_eq(ip4Eq(s0, [192, 168, 1, 10]), true);
                } else {
                    expect_eq(true, false);
                }
                if (s1 := rec.sourceAt(1)) {
                    expect_eq(ip4Eq(s1, [192, 168, 1, 20]), true);
                } else {
                    expect_eq(true, false);
                }
                expect_eq(rec.sourceAt(2) == null, true);
            } else if (count == 2) {
                expect_eq(rec.rec_type == IgmpV3RecordType.change_to_exclude, true);
                expect_eq(rec.aux_data_words, 0);
                expect_eq(rec.num_sources, 0);
                expect_eq(ip4Eq(rec.multicast_addr, [239, 0, 0, 1]), true);
            }
        }
        expect_eq(count, 2);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("igmp: igmpDecodeFloatCode RFC 3376 linear (<128) & exponential (>=128)") {
    expect_eq(igmpDecodeFloatCode(0), 0);
    expect_eq(igmpDecodeFloatCode(100), 100);
    expect_eq(igmpDecodeFloatCode(127), 127);

    // 0x80: exp=0, mant=0 -> (0 | 16) << 3 = 128
    expect_eq(igmpDecodeFloatCode(0x80), 128);
    // 0x8F: exp=0, mant=15 -> (15 | 16) << 3 = 31 * 8 = 248
    expect_eq(igmpDecodeFloatCode(0x8F), 248);
    // 0x90: exp=1, mant=0 -> 16 << 4 = 256
    expect_eq(igmpDecodeFloatCode(0x90), 256);
    // 0xFF: exp=7, mant=15 -> 31 << 10 = 31744
    expect_eq(igmpDecodeFloatCode(0xFF), 31744);
}

test("igmp: tampered checksum, truncated packet & out-of-bounds num_sources guards") {
    // 1. Tampered v2 packet fails verifyIgmpChecksum
    let buf: [8]u8 = [0; 8];
    if (n := encodeIgmpV2(IgmpType.v2_report, 0, [224, 0, 0, 22], buf[0..8])) {
        expect_eq(verifyIgmpChecksum(buf[0..n]), true);
        buf[7] = buf[7] ^ 0x01;
        expect_eq(verifyIgmpChecksum(buf[0..n]), false);
    } else {
        expect_eq(true, false);
    }

    // 2. Too short (< 8B)
    expect_eq(parseIgmp("\x16\x00\x00\x00\xE0\x00\x00") == null, true);
    expect_eq(verifyIgmpChecksum("\x16\x00\x00"), false);

    // 3. Truncated v3 Query (10 bytes: > 8 and < 12)
    let bad_q10: [10]u8 = [0x11, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(parseIgmp(bad_q10[0..10]) == null, true);

    // 4. v3 Query with num_sources=3 (needs 12+12=24B) in 16B buffer -> rejected
    let bad_q_src: [16]u8 = [
        0x11, 0, 0, 0, 232, 0, 0, 1,
        0x02, 125, 0x00, 0x03,
        10, 0, 0, 1,
    ];
    expect_eq(parseIgmp(bad_q_src[0..16]) == null, true);
}

test("igmp: malformed v3 Group Record iterator & encodeIgmpV2 guards") {
    // v3 Report claims 2 records but only has 4 trailing bytes -> iterator sets hasError()
    let bad_rep: [12]u8 = [
        0x22, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02,
        0x01, 0x00, 0x00, 0x00,
    ];
    if (pkt := parseIgmp(bad_rep[0..12])) {
        let it = pkt.v3Records();
        expect_eq(it.next() == null, true);
        expect_eq(it.hasError(), true);
    } else {
        expect_eq(true, false);
    }

    // encodeIgmpV2 rejects tiny buffer and v3_report / unknown types
    let tiny: [7]u8 = [0; 7];
    expect_eq(encodeIgmpV2(IgmpType.v2_report, 0, [224, 0, 0, 1], tiny[0..7]) == null, true);

    let ok_buf: [8]u8 = [0; 8];
    expect_eq(encodeIgmpV2(IgmpType.v3_report, 0, [0, 0, 0, 0], ok_buf[0..8]) == null, true);
    expect_eq(encodeIgmpV2(IgmpType.unknown, 0, [0, 0, 0, 0], ok_buf[0..8]) == null, true);
}
