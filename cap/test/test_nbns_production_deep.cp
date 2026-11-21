// Source: cp/cap/test/test_nbns_production_deep.cp (Production deep tests for cap:nbns)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NBNS_UDP_PORT,
    NBNS_HEADER_SIZE,
    NBNS_ENCODED_NAME_LEN,
    NBNS_RAW_NAME_LEN,
    NBNS_TYPE_NB,
    NBNS_TYPE_NBSTAT,
    NBNS_CLASS_IN,
    OP_QUERY,
    OP_REGISTRATION,
    OP_RELEASE,
    OP_WACK,
    OP_REFRESH,
    SUFFIX_WORKSTATION,
    SUFFIX_MESSENGER,
    SUFFIX_FILE_SERVER,
    SUFFIX_DOMAIN_MASTER,
    SUFFIX_DOMAIN_CTRL,
    SUFFIX_MASTER_BROWSER,
    SUFFIX_BROWSER_ELECTION,
    NbnsName,
    encodeNbnsName,
    decodeNbnsName,
    NbnsHeader,
    NbnsAddrEntry,
    NbnsAddrIter,
    NbnsStatEntry,
    NbnsStatIter,
    NbnsPacket,
    isNbns,
    parseNbns,
    encodeNbnsQuery,
} from "cap:nbns";

fn putU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn putU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

test("1. RFC 1001 section 14.1 standard vector FRED -> EGFCEFEE... first-level codec round-trip", () => {
    expect_eq(NBNS_UDP_PORT, 137 as u16);
    expect_eq(NBNS_HEADER_SIZE, 12 as usize);
    expect_eq(NBNS_ENCODED_NAME_LEN, 32 as usize);
    expect_eq(NBNS_RAW_NAME_LEN, 16 as usize);
    expect_eq(OP_QUERY, 0 as u8);
    expect_eq(OP_REGISTRATION, 5 as u8);
    expect_eq(OP_RELEASE, 6 as u8);
    expect_eq(OP_WACK, 7 as u8);
    expect_eq(OP_REFRESH, 8 as u8);
    expect_eq(SUFFIX_MESSENGER, 0x03 as u8);
    expect_eq(SUFFIX_DOMAIN_MASTER, 0x1B as u8);
    expect_eq(SUFFIX_DOMAIN_CTRL, 0x1C as u8);
    expect_eq(SUFFIX_MASTER_BROWSER, 0x1D as u8);
    expect_eq(SUFFIX_BROWSER_ELECTION, 0x1E as u8);

    // RFC 1001 standard vector: "fred" (auto-uppercased to "FRED") + 0x20 suffix
    let enc: [32]u8 = std.mem.zeroes([32]u8);
    const n = encodeNbnsName("fred", SUFFIX_FILE_SERVER, enc[0..]) ?? unreachable;
    expect_eq(n, 32 as usize);
    expect_eq(std.mem.eql(u8, enc[0..], "EGFCEFEECACACACACACACACACACACACA"), true);

    const decoded = decodeNbnsName(enc[0..]) ?? unreachable;
    expect_eq(decoded.suffix, SUFFIX_FILE_SERVER);
    expect_eq(decoded.trimmedLen(), 4 as usize);
    expect_eq(std.mem.eql(u8, decoded.nameSlice(), "FRED"), true);

    // Workstation suffix (0x00) round-trip
    const n2 = encodeNbnsName("FILESRV01", SUFFIX_WORKSTATION, enc[0..]) ?? unreachable;
    expect_eq(n2, 32 as usize);
    const dec2 = decodeNbnsName(enc[0..]) ?? unreachable;
    expect_eq(dec2.suffix, SUFFIX_WORKSTATION);
    expect_eq(std.mem.eql(u8, dec2.nameSlice(), "FILESRV01"), true);
});

test("2. NBNS Query zero-alloc encoding and parsing round-trip (broadcast & unicast)", () => {
    let pkt_buf: [64]u8 = std.mem.zeroes([64]u8);
    const n = encodeNbnsQuery(0x1337, "WORKGROUP", SUFFIX_WORKSTATION, true, pkt_buf[0..]) ?? unreachable;
    expect_eq(n, 50 as usize);
    expect_eq(isNbns(pkt_buf[0..n]), true);

    const pkt = parseNbns(pkt_buf[0..n]) ?? unreachable;
    expect_eq(pkt.header.tr_id, 0x1337 as u16);
    expect_eq(pkt.header.isResponse(), false);
    expect_eq(pkt.header.opcode(), OP_QUERY);
    expect_eq(pkt.header.isBroadcast(), true);
    expect_eq(pkt.header.qd_count, 1 as u16);
    expect_eq(pkt.header.an_count, 0 as u16);
    expect_eq(pkt.qtype, NBNS_TYPE_NB);
    expect_eq(pkt.qclass, NBNS_CLASS_IN);
    expect_eq(pkt.name.suffix, SUFFIX_WORKSTATION);
    expect_eq(std.mem.eql(u8, pkt.name.nameSlice(), "WORKGROUP"), true);

    // Unicast query
    const n2 = encodeNbnsQuery(0x0042, "DC01", SUFFIX_FILE_SERVER, false, pkt_buf[0..]) ?? unreachable;
    const pkt2 = parseNbns(pkt_buf[0..n2]) ?? unreachable;
    expect_eq(pkt2.header.isBroadcast(), false);
    expect_eq(std.mem.eql(u8, pkt2.name.nameSlice(), "DC01"), true);
});

test("3. NB (0x0020) multi-IP response with Group/ONT flags parsing", () => {
    // 12B header + 34B encoded name (0x20 + 32B + 0x00) + 10B RR header + 12B RDATA (2 entries) = 68B
    let resp: [68]u8 = std.mem.zeroes([68]u8);
    putU16Be(resp[0..], 0, 0xABCD);
    putU16Be(resp[0..], 2, 0x8500); // R=1, OP=0, AA=1, RD=1
    putU16Be(resp[0..], 4, 0); // qd_count=0
    putU16Be(resp[0..], 6, 1); // an_count=1

    resp[12] = 32;
    * = encodeNbnsName("NAS01", SUFFIX_FILE_SERVER, resp[13..45]) ?? unreachable;
    resp[45] = 0;

    putU16Be(resp[0..], 46, NBNS_TYPE_NB);
    putU16Be(resp[0..], 48, NBNS_CLASS_IN);
    putU32Be(resp[0..], 50, 300000); // TTL
    putU16Be(resp[0..], 54, 12); // RDLENGTH = 2 * 6

    // Entry 0: Unique (G=0), H-node (ONT=3 -> 0x6000), IP 192.168.1.10
    putU16Be(resp[0..], 56, 0x6000);
    resp[58] = 192;
    resp[59] = 168;
    resp[60] = 1;
    resp[61] = 10;

    // Entry 1: Group (G=1), B-node (ONT=0 -> 0x8000), IP 10.0.0.5
    putU16Be(resp[0..], 62, 0x8000);
    resp[64] = 10;
    resp[65] = 0;
    resp[66] = 0;
    resp[67] = 5;

    const pkt = parseNbns(resp[0..]) ?? unreachable;
    expect_eq(pkt.header.isResponse(), true);
    expect_eq(pkt.header.isAuthoritative(), true);
    expect_eq(pkt.header.isTruncated(), false);
    expect_eq(pkt.header.rcode(), 0 as u8);
    expect_eq(pkt.ttl, 300000 as u32);
    expect_eq(std.mem.eql(u8, pkt.name.nameSlice(), "NAS01"), true);

    let it = pkt.addrs();
    const e0 = it.next() ?? unreachable;
    expect_eq(e0.is_group, false);
    expect_eq(e0.ont, 3 as u8);
    expect_eq(e0.ip[0], 192 as u8);
    expect_eq(e0.ip[3], 10 as u8);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.is_group, true);
    expect_eq(e1.ont, 0 as u8);
    expect_eq(e1.ip[0], 10 as u8);
    expect_eq(e1.ip[3], 5 as u8);

    expect_eq(it.next() == null, true);
});

test("4. NBSTAT (0x0021) node status table + MAC unit_id parsing", () => {
    // RDATA: 1B count (2) + 2*18B entries (36B) + 6B MAC = 43B
    let rdata: [43]u8 = std.mem.zeroes([43]u8);
    rdata[0] = 2;

    // Entry 0: "HOST01         " + SUFFIX_WORKSTATION (0x00), flags = Active (0x0400)
    let i: usize = 0;
    while (i < 15) {
        rdata[1 + i] = 0x20;
        i += 1;
    }
    const h0 = "HOST01";
    i = 0;
    while (i < h0.len) {
        rdata[1 + i] = h0[i];
        i += 1;
    }
    rdata[1 + 15] = SUFFIX_WORKSTATION;
    putU16Be(rdata[0..], 1 + 16, 0x0400);

    // Entry 1: "WORKGROUP      " + SUFFIX_WORKSTATION (0x00), flags = Group | Active (0x8400)
    i = 0;
    while (i < 15) {
        rdata[19 + i] = 0x20;
        i += 1;
    }
    const h1 = "WORKGROUP";
    i = 0;
    while (i < h1.len) {
        rdata[19 + i] = h1[i];
        i += 1;
    }
    rdata[19 + 15] = SUFFIX_WORKSTATION;
    putU16Be(rdata[0..], 19 + 16, 0x8400);

    // 6-byte MAC address at offset 37
    rdata[37] = 0x00;
    rdata[38] = 0x1A;
    rdata[39] = 0x2B;
    rdata[40] = 0x3C;
    rdata[41] = 0x4D;
    rdata[42] = 0x5E;

    let sit = NbnsStatIter.init(rdata[0..]) ?? unreachable;
    expect_eq(sit.count(), 2 as u8);

    const s0 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s0.name.nameSlice(), "HOST01"), true);
    expect_eq(s0.name.suffix, SUFFIX_WORKSTATION);
    expect_eq(s0.isGroup(), false);
    expect_eq(s0.isActive(), true);

    const s1 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s1.name.nameSlice(), "WORKGROUP"), true);
    expect_eq(s1.isGroup(), true);
    expect_eq(s1.isActive(), true);

    expect_eq(sit.next() == null, true);

    const mac = sit.unitId() ?? unreachable;
    expect_eq(mac[0], 0x00 as u8);
    expect_eq(mac[1], 0x1A as u8);
    expect_eq(mac[5], 0x5E as u8);
});

test("5. Invalid nibble chars, overlong names, truncated packets & malformed RDATA rejection", () => {
    // Non-'A'..'P' character ('Q' and '0') and wrong length
    expect_eq(decodeNbnsName("QGFCEFEECACACACACACACACACACACACA") == null, true);
    expect_eq(decodeNbnsName("0GFCEFEECACACACACACACACACACACACA") == null, true);
    expect_eq(decodeNbnsName("SHORT") == null, true);

    // Name > 15 bytes or tiny output buffer
    let enc: [32]u8 = std.mem.zeroes([32]u8);
    expect_eq(encodeNbnsName("SIXTEEN_BYTES_16", 0, enc[0..]) == null, true);
    expect_eq(encodeNbnsName("OK", 0, enc[0..16]) == null, true);
    expect_eq(encodeNbnsQuery(1, "OK", 0, false, enc[0..]) == null, true);

    // Empty / truncated NBNS header
    const tiny: [8]u8 = [0, 1, 0, 0, 0, 1, 0, 0];
    expect_eq(parseNbns(tiny[0..]) == null, true);
    expect_eq(isNbns(tiny[0..]), false);

    // Header with 0 questions and 0 answers
    const empty_hdr: [12]u8 = [0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(parseNbns(empty_hdr[0..]) == null, true);

    // Truncated NBSTAT RDATA (claims 2 entries = 37B, only 10B provided)
    const bad_stat: [10]u8 = [2, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(NbnsStatIter.init(bad_stat[0..]) == null, true);
});
