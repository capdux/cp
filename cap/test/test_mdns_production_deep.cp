// Source: cp/cap/test/test_mdns_production_deep.cp (Production deep tests for cap:mdns)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MDNS_UDP_PORT,
    MDNS_MULTICAST_V4,
    MDNS_MULTICAST_V6,
    MDNS_HEADER_SIZE,
    MDNS_QU_BIT,
    MDNS_CACHE_FLUSH_BIT,
    CLASS_IN,
    TYPE_A,
    TYPE_PTR,
    TYPE_TXT,
    TYPE_AAAA,
    TYPE_SRV,
    TYPE_NSEC,
    TYPE_ANY,
    MdnsHeader,
    MdnsQuestion,
    MdnsQuestionIter,
    MdnsRecord,
    MdnsRecordIter,
    MdnsSrv,
    MdnsTxtEntry,
    MdnsTxtIter,
    MdnsPacket,
    isMdns,
    parseMdns,
    parseMdnsSrv,
    parseMdnsA,
    parseMdnsAaaa,
    encodeMdnsQuery,
} from "cap:mdns";

test("1. Protocol constants, encodeMdnsQuery + parseMdns roundtrip, and QU bit verification", () => {
    expect_eq(MDNS_UDP_PORT, 5353 as u16);
    expect_eq(std.mem.eql(u8, MDNS_MULTICAST_V4, "224.0.0.251"), true);
    expect_eq(std.mem.eql(u8, MDNS_MULTICAST_V6, "ff02::fb"), true);
    expect_eq(MDNS_HEADER_SIZE, 12 as usize);
    expect_eq(MDNS_QU_BIT, 0x8000 as u16);
    expect_eq(MDNS_CACHE_FLUSH_BIT, 0x8000 as u16);
    expect_eq(CLASS_IN, 1 as u16);

    let wire: [128]u8 = std.mem.zeroes([128]u8);
    const n = encodeMdnsQuery("_http._tcp.local", TYPE_PTR, true, wire[0..]) ?? unreachable;
    expect_eq(isMdns(wire[0..n]), true);

    const pkt = parseMdns(wire[0..n]) ?? unreachable;
    expect_eq(pkt.header.id, 0 as u16);
    expect_eq(pkt.header.isResponse(), false);
    expect_eq(pkt.header.qd_count, 1 as u16);
    expect_eq(pkt.header.an_count, 0 as u16);

    let qit = pkt.questions();
    const q = qit.next() ?? unreachable;
    expect_eq(q.qtype, TYPE_PTR);
    expect_eq(q.qclass, CLASS_IN);
    expect_eq(q.unicast_response, true);

    let name_buf: [128]u8 = std.mem.zeroes([128]u8);
    const name_len = pkt.decodeName(q.name_offset, name_buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, name_buf[0..name_len], "_http._tcp.local"), true);
    expect_eq(qit.next() == null, true);

    // QM (multicast response, qu=false) with trailing dot
    const n2 = encodeMdnsQuery("printer.local.", TYPE_A, false, wire[0..]) ?? unreachable;
    const pkt2 = parseMdns(wire[0..n2]) ?? unreachable;
    let qit2 = pkt2.questions();
    const q2 = qit2.next() ?? unreachable;
    expect_eq(q2.qtype, TYPE_A);
    expect_eq(q2.unicast_response, false);
});

test("2. Full DNS-SD response (PTR, SRV, TXT, A) with 0xC0 compression pointers and Cache-Flush bit", () => {
    // Wire packet layout:
    // 0..12:  Header: id=0, flags=0x8400 (Response + Authoritative), qd=0, an=2 (PTR, SRV), ns=0, ar=2 (TXT, A)
    // 12..30: Name "_http._tcp.local" (18B: 5 _http 4 _tcp 5 local 0)
    //   12: 5 '_','h','t','t','p' (6B)
    //   18: 4 '_','t','c','p' (5B)
    //   23: 5 'l','o','c','a','l' (6B)
    //   29: 0 (1B)
    // 30..40: PTR RR header (type=12, class=0x0001 (no cache-flush), ttl=4500, rdlen=7)
    // 40..47: PTR rdata: 4 'w','e','b','1', 0xC0, 0x0C (points to "_http._tcp.local" at 12)
    // 47..49: SRV Name: 0xC0, 0x28 (points to "web1._http._tcp.local" at 40)
    // 49..59: SRV RR header (type=33, class=0x8001 (cache-flush=true), ttl=120, rdlen=14)
    // 59..73: SRV rdata: prio=0, weight=0, port=8080 (0x1F90), target: 5 'm','y','p','c','1', 0xC0, 0x17 (points to "local" at 23)
    // 73..75: TXT Name: 0xC0, 0x28 (points to "web1._http._tcp.local" at 40)
    // 75..85: TXT RR header (type=16, class=0x8001, ttl=120, rdlen=12)
    // 85..97: TXT rdata: [6, 'p','a','t','h','=','/'], [4, 't','l','s','0'] -> 7 + 5 = 12B
    // 97..99: A Name: 0xC0, 0x41 (points to "mypc1.local" at 65 = 59 + 6)
    // 99..109: A RR header (type=1, class=0x8001, ttl=120, rdlen=4)
    // 109..113: A rdata: 192.168.1.42
    let pkt_bytes: [113]u8 = [
        // Header (0..12)
        0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
        // PTR Name at 12: "_http._tcp.local"
        5, '_', 'h', 't', 't', 'p',
        4, '_', 't', 'c', 'p',
        5, 'l', 'o', 'c', 'a', 'l',
        0,
        // PTR RR meta (30..40)
        0x00, 0x0C, 0x00, 0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0x07,
        // PTR rdata at 40: "web1._http._tcp.local"
        4, 'w', 'e', 'b', '1', 0xC0, 0x0C,
        // SRV Name at 47 -> ptr 40 (0x28)
        0xC0, 0x28,
        // SRV RR meta (49..59): cache-flush=1, rdlen=14
        0x00, 0x21, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x0E,
        // SRV rdata at 59: prio=0, weight=0, port=8080, target at 65 (0x41) = "mypc1.local"
        0x00, 0x00, 0x00, 0x00, 0x1F, 0x90,
        5, 'm', 'y', 'p', 'c', '1', 0xC0, 0x17,
        // TXT Name at 73 -> ptr 40 (0x28)
        0xC0, 0x28,
        // TXT RR meta (75..85): cache-flush=1, rdlen=12
        0x00, 0x10, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x0C,
        // TXT rdata at 85 (12B)
        6, 'p', 'a', 't', 'h', '=', '/',
        4, 't', 'l', 's', '0',
        // A Name at 97 -> ptr 65 (0x41)
        0xC0, 0x41,
        // A RR meta (99..109): cache-flush=1, rdlen=4
        0x00, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x04,
        // A rdata at 109
        192, 168, 1, 42,
    ];

    const pkt = parseMdns(pkt_bytes[0..]) ?? unreachable;
    expect_eq(pkt.header.isResponse(), true);
    expect_eq(pkt.header.isAuthoritative(), true);
    expect_eq(pkt.header.isTruncated(), false);

    let rit = pkt.records();
    let name_buf: [128]u8 = std.mem.zeroes([128]u8);

    // 1. PTR
    const r_ptr = rit.next() ?? unreachable;
    expect_eq(r_ptr.rtype, TYPE_PTR);
    expect_eq(r_ptr.cache_flush, false);
    expect_eq(r_ptr.ttl, 4500 as u32);
    const ptr_target_len = pkt.decodeName(r_ptr.rdata_offset, name_buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, name_buf[0..ptr_target_len], "web1._http._tcp.local"), true);

    // 2. SRV
    const r_srv = rit.next() ?? unreachable;
    expect_eq(r_srv.rtype, TYPE_SRV);
    expect_eq(r_srv.cache_flush, true);
    const srv = parseMdnsSrv(r_srv) ?? unreachable;
    expect_eq(srv.priority, 0 as u16);
    expect_eq(srv.weight, 0 as u16);
    expect_eq(srv.port, 8080 as u16);
    const srv_target_len = pkt.decodeName(srv.target_offset, name_buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, name_buf[0..srv_target_len], "mypc1.local"), true);

    // 3. TXT
    const r_txt = rit.next() ?? unreachable;
    expect_eq(r_txt.rtype, TYPE_TXT);
    expect_eq(r_txt.cache_flush, true);

    // 4. A
    const r_a = rit.next() ?? unreachable;
    expect_eq(r_a.rtype, TYPE_A);
    expect_eq(r_a.cache_flush, true);
    const a_name_len = pkt.decodeName(r_a.name_offset, name_buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, name_buf[0..a_name_len], "mypc1.local"), true);
    const ip = parseMdnsA(r_a) ?? unreachable;
    expect_eq(ip[0], 192 as u8);
    expect_eq(ip[1], 168 as u8);
    expect_eq(ip[2], 1 as u8);
    expect_eq(ip[3], 42 as u8);

    expect_eq(rit.next() == null, true);
});

test("3. RFC 6763 TXT key/value iterator (with key=val, empty val, and boolean flag without '=')", () => {
    let txt_rdata: [29]u8 = [
        7, 't', 'x', 't', 'v', '=', '1', '0',
        9, 'p', 'a', 't', 'h', '=', '/', 'a', 'p', 'i',
        4, 'i', 'p', 'v', '6',
        5, 'e', 'm', 'p', 't', '=',
    ];

    let it = MdnsTxtIter.init(txt_rdata[0..]);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "txtv"), true);
    expect_eq(std.mem.eql(u8, e1.value, "10"), true);
    expect_eq(e1.has_value, true);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "path"), true);
    expect_eq(std.mem.eql(u8, e2.value, "/api"), true);
    expect_eq(e2.has_value, true);

    const e3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.key, "ipv6"), true);
    expect_eq(e3.value.len, 0 as usize);
    expect_eq(e3.has_value, false);

    const e4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e4.key, "empt"), true);
    expect_eq(e4.value.len, 0 as usize);
    expect_eq(e4.has_value, true);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("4. AAAA record extraction (16-byte IPv6 address)", () => {
    // Header + 1 Answer AAAA record for "h.local"
    let pkt_bytes: [47]u8 = [
        0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        1, 'h', 5, 'l', 'o', 'c', 'a', 'l', 0,
        0x00, 0x1C, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x10,
        // fe80::1
        0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    ];
    const pkt = parseMdns(pkt_bytes[0..]) ?? unreachable;
    let rit = pkt.records();
    const rec = rit.next() ?? unreachable;
    expect_eq(rec.rtype, TYPE_AAAA);
    const v6 = parseMdnsAaaa(rec) ?? unreachable;
    expect_eq(v6[0], 0xFE as u8);
    expect_eq(v6[1], 0x80 as u8);
    expect_eq(v6[15], 0x01 as u8);
    expect_eq(parseMdnsA(rec) == null, true);
});

test("5. Compression pointer loop guard (self-pointing & 2-node cycle) and truncation rejection", () => {
    // Self-pointing compression pointer at offset 12 -> 0xC0 0x0C
    let self_loop: [18]u8 = [
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC0, 0x0C,
        0x00, 0x01, 0x00, 0x01,
    ];
    const pkt_self = parseMdns(self_loop[0..]) ?? unreachable;
    let name_buf: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(pkt_self.decodeName(12, name_buf[0..]) == null, true);

    // 2-node mutually recursive compression pointer cycle: offset 12 -> 14, offset 14 -> 12
    let cycle_pkt: [20]u8 = [
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC0, 0x0E,
        0xC0, 0x0C,
        0x00, 0x01, 0x00, 0x01,
    ];
    const pkt_cyc = parseMdns(cycle_pkt[0..]) ?? unreachable;
    expect_eq(pkt_cyc.decodeName(12, name_buf[0..]) == null, true);

    // Truncated packet (< 12 bytes) and truncated RR rdata rejected
    let too_short: [8]u8 = [0, 0, 0, 0, 0, 1, 0, 0];
    expect_eq(isMdns(too_short[0..]), false);
    expect_eq(parseMdns(too_short[0..]) == null, true);

    let trunc_rr: [24]u8 = [
        0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        1, 'a', 0,
        0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, // truncated rdlength/rdata
    ];
    expect_eq(parseMdns(trunc_rr[0..]) == null, true);

    // Invalid query inputs: empty name, double dot "..", tiny buffer
    let small: [16]u8 = std.mem.zeroes([16]u8);
    expect_eq(encodeMdnsQuery("", TYPE_A, false, name_buf[0..]) == null, true);
    expect_eq(encodeMdnsQuery("bad..local", TYPE_A, false, name_buf[0..]) == null, true);
    expect_eq(encodeMdnsQuery("_http._tcp.local", TYPE_PTR, false, small[0..]) == null, true);

    // Truncated TXT entry sets hasError()
    let bad_txt: [4]u8 = [10, 'a', '=', '1'];
    let txt_it = MdnsTxtIter.init(bad_txt[0..]);
    expect_eq(txt_it.next() == null, true);
    expect_eq(txt_it.hasError(), true);
});
