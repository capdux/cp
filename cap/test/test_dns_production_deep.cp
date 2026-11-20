// Source: cp/cap/test/test_dns_production_deep.cp
// Deep Production Verification Suite for cap:dns:
// 1. Safe Name Decoder (16-hop loop guard) & IPv4/IPv6 PTR .arpa Builder
// 2. SOA, SRV (RFC 2782 Priority/Weight Sort) & CAA (RFC 8659) Wire Codecs
// 3. RFC 8484 DoH GET URL / POST Headers & Cloudflare/Google JSON Response Parser
// 4. POSIX /etc/resolv.conf Parser, Host Loader & ndots Search-Domain Expander
// 5. Existing cap:dns Zero-Regression Compatibility

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    // New production modules
    DnsRecordError,
    DnsNameCodec,
    SrvRecord,
    SrvSelector,
    SoaRecord,
    CaaRecord,
    DohError,
    DohHeader,
    DohPostHeaders,
    DohQueryBuilder,
    DohJsonQuestion,
    DohJsonAnswer,
    DohJsonResponse,
    ResolvConf,
    // Existing cap:dns exports (zero-regression)
    ARES_SUCCESS,
    ARES_ETIMEOUT,
    ARES_ENOTFOUND,
    NSClass,
    NSType,
    CaresStatus,
    DnsRCode,
    DnsHeader,
    DnsWireCodec,
    DnsFamily,
    DnsCache,
    HostsTable,
} from "cap:dns";

fn testPointerLoopProtectionAndPtrArpa(): !void {
    const alloc = std.heap.page_allocator;
    let out_name: [256]u8 = undefined;

    // 1. Self-referential pointer loop at offset 12: 0xC0 0x0C
    let self_loop_pkt: [14]u8 = [0; 14];
    self_loop_pkt[12] = 0xC0;
    self_loop_pkt[13] = 0x0C;
    let got_self_loop = false;
    * = DnsNameCodec.decode(&self_loop_pkt, 12, &out_name) catch |e| {
        if (e == DnsRecordError.PointerLoop) got_self_loop = true;
    };
    expect_eq(got_self_loop, true);

    // 2. Mutual 2-node pointer cycle (offset 12 -> 14 -> 12) triggers MAX_JUMPS=16 PointerLoop
    let cycle_pkt: [16]u8 = [0; 16];
    cycle_pkt[12] = 0xC0;
    cycle_pkt[13] = 14;
    cycle_pkt[14] = 0xC0;
    cycle_pkt[15] = 12;
    let got_cycle_loop = false;
    * = DnsNameCodec.decode(&cycle_pkt, 12, &out_name) catch |e| {
        if (e == DnsRecordError.PointerLoop) got_cycle_loop = true;
    };
    expect_eq(got_cycle_loop, true);

    // 3. Valid 16-hop chain succeeds; 17-hop chain fails with PointerLoop
    // Layout: offsets 0..31 are 16 pointers (0->2, 2->4, ..., 30->32), offset 32 is "\x02cp\x00"
    let chain16: [36]u8 = [0; 36];
    let idx: usize = 0;
    while (idx < 16) {
        const target = (idx + 1) * 2;
        chain16[idx * 2] = 0xC0;
        chain16[idx * 2 + 1] = target as u8;
        idx += 1;
    }
    chain16[32] = 2;
    chain16[33] = 'c';
    chain16[34] = 'p';
    chain16[35] = 0;
    const dec16 = try DnsNameCodec.decode(&chain16, 0, &out_name);
    expect_eq(out_name[0..dec16.name_len], "cp");
    expect_eq(dec16.packet_bytes_consumed, 2 as usize);

    // 17-hop chain (0->2->...->32->34) exceeds MAX_JUMPS=16
    let chain17: [38]u8 = [0; 38];
    idx = 0;
    while (idx < 17) {
        const target = (idx + 1) * 2;
        chain17[idx * 2] = 0xC0;
        chain17[idx * 2 + 1] = target as u8;
        idx += 1;
    }
    chain17[34] = 2;
    chain17[35] = 'c';
    chain17[36] = 'p';
    chain17[37] = 0;
    let got_17_loop = false;
    * = DnsNameCodec.decode(&chain17, 0, &out_name) catch |e| {
        if (e == DnsRecordError.PointerLoop) got_17_loop = true;
    };
    expect_eq(got_17_loop, true);

    // 4. Out-of-bounds pointer and reserved label type (0x40) rejected
    let bad_ptr: [2]u8 = [0xC0, 99];
    let got_invalid_ptr = false;
    * = DnsNameCodec.decode(&bad_ptr, 0, &out_name) catch |e| {
        if (e == DnsRecordError.InvalidPointer) got_invalid_ptr = true;
    };
    expect_eq(got_invalid_ptr, true);

    let reserved_lbl: [3]u8 = [0x41, 'a', 0];
    let got_invalid_lbl = false;
    * = DnsNameCodec.decode(&reserved_lbl, 0, &out_name) catch |e| {
        if (e == DnsRecordError.InvalidLabel) got_invalid_lbl = true;
    };
    expect_eq(got_invalid_lbl, true);

    // 5. Reverse PTR .arpa builders (IPv4 & IPv6 with :: compression)
    let arpa_buf: [96]u8 = undefined;
    const ip4: [4]u8 = [192, 0, 2, 44];
    const ptr4 = try DnsNameCodec.ipv4ToArpa(ip4, &arpa_buf);
    expect_eq(ptr4, "44.2.0.192.in-addr.arpa");

    const ptr4_str = try DnsNameCodec.ipv4StrToArpa("8.8.4.4", &arpa_buf);
    expect_eq(ptr4_str, "4.4.8.8.in-addr.arpa");

    const ptr6_str = try DnsNameCodec.ipv6StrToArpa("2001:db8::567:89ab", &arpa_buf);
    expect_eq(
        ptr6_str,
        "b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa",
    );

    const ptr6_loopback = try DnsNameCodec.ipv6StrToArpa("::1", &arpa_buf);
    expect_eq(
        ptr6_loopback,
        "1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa",
    );

    const ip4_alloc: [4]u8 = [10, 20, 30, 40];
    const ptr4_alloc = try DnsNameCodec.ipv4ToArpaAlloc(alloc, ip4_alloc);
    defer alloc.free(ptr4_alloc);
    expect_eq(ptr4_alloc, "40.30.20.10.in-addr.arpa");
}

fn testSoaSrvCaaRecordsAndSorting(): !void {
    // 1. SOA encode/decode roundtrip + compressed RNAME pointer decoding
    const soa_in = SoaRecord.init(
        "ns1.cp-lang.org",
        "hostmaster.cp-lang.org",
        2026112001,
        7200,
        900,
        1209600,
        300,
    );
    let rdata_buf: [256]u8 = undefined;
    const soa_len = try soa_in.encodeRdata(&rdata_buf);
    let soa_out = try SoaRecord.parseWire(rdata_buf[0..soa_len], 0, soa_len as u16);
    expect_eq(soa_out.mname(), "ns1.cp-lang.org");
    expect_eq(soa_out.rname(), "hostmaster.cp-lang.org");
    expect_eq(soa_out.serial, 2026112001 as u32);
    expect_eq(soa_out.refresh, 7200 as u32);
    expect_eq(soa_out.retry, 900 as u32);
    expect_eq(soa_out.expire, 1209600 as u32);
    expect_eq(soa_out.minimum, 300 as u32);

    // SOA with compression pointer inside packet (offset 0 = "\x07cp-lang\x03org\x00")
    let comp_pkt: [64]u8 = [0; 64];
    const base_len = DnsNameCodec.encode("cp-lang.org", &comp_pkt); // 13 bytes
    const rdata_off = base_len;
    // mname: "\x03ns2" + pointer(0)
    comp_pkt[rdata_off] = 3;
    comp_pkt[rdata_off + 1] = 'n';
    comp_pkt[rdata_off + 2] = 's';
    comp_pkt[rdata_off + 3] = '2';
    comp_pkt[rdata_off + 4] = 0xC0;
    comp_pkt[rdata_off + 5] = 0;
    // rname: "\x05admin" + pointer(0)
    comp_pkt[rdata_off + 6] = 5;
    comp_pkt[rdata_off + 7] = 'a';
    comp_pkt[rdata_off + 8] = 'd';
    comp_pkt[rdata_off + 9] = 'm';
    comp_pkt[rdata_off + 10] = 'i';
    comp_pkt[rdata_off + 11] = 'n';
    comp_pkt[rdata_off + 12] = 0xC0;
    comp_pkt[rdata_off + 13] = 0;
    // 20-byte trailer: serial=42, refresh=60, retry=30, expire=600, minimum=15
    comp_pkt[rdata_off + 17] = 42;
    comp_pkt[rdata_off + 21] = 60;
    comp_pkt[rdata_off + 25] = 30;
    comp_pkt[rdata_off + 28] = 0x02;
    comp_pkt[rdata_off + 29] = 0x58; // 600
    comp_pkt[rdata_off + 33] = 15;
    let soa_comp = try SoaRecord.parseWire(comp_pkt[0 .. rdata_off + 34], rdata_off, 34);
    expect_eq(soa_comp.mname(), "ns2.cp-lang.org");
    expect_eq(soa_comp.rname(), "admin.cp-lang.org");
    expect_eq(soa_comp.serial, 42 as u32);
    expect_eq(soa_comp.expire, 600 as u32);
    expect_eq(soa_comp.minimum, 15 as u32);

    // 2. SRV encode/decode & RFC 2782 SrvSelector.sortByPriorityAndWeight
    const srv_in = SrvRecord.init(10, 60, 5060, "sip1.cp-lang.org");
    const srv_len = try srv_in.encodeRdata(&rdata_buf);
    let srv_out = try SrvRecord.parseWire(rdata_buf[0..srv_len], 0, srv_len as u16);
    expect_eq(srv_out.priority, 10 as u16);
    expect_eq(srv_out.weight, 60 as u16);
    expect_eq(srv_out.port, 5060 as u16);
    expect_eq(srv_out.target(), "sip1.cp-lang.org");

    let srv_list: [5]SrvRecord = [
        SrvRecord.init(30, 100, 443, "dr.cp-lang.org"),
        SrvRecord.init(10, 0, 443, "p10-w0.cp-lang.org"),
        SrvRecord.init(20, 50, 443, "p20-w50.cp-lang.org"),
        SrvRecord.init(10, 80, 443, "p10-w80.cp-lang.org"),
        SrvRecord.init(10, 20, 443, "p10-w20.cp-lang.org"),
    ];
    SrvSelector.sortByPriorityAndWeight(&srv_list);
    expect_eq(srv_list[0].target(), "p10-w80.cp-lang.org");
    expect_eq(srv_list[1].target(), "p10-w20.cp-lang.org");
    expect_eq(srv_list[2].target(), "p10-w0.cp-lang.org");
    expect_eq(srv_list[3].target(), "p20-w50.cp-lang.org");
    expect_eq(srv_list[4].target(), "dr.cp-lang.org");

    // Verify weighted shuffle preserves priority tier ordering and places weight-0 at tier end
    SrvSelector.shuffleWeighted(&srv_list, 0xCAFEBABE);
    expect_eq(srv_list[0].priority, 10 as u16);
    expect_eq(srv_list[1].priority, 10 as u16);
    expect_eq(srv_list[2].priority, 10 as u16);
    expect_eq(srv_list[2].weight, 0 as u16);
    expect_eq(srv_list[3].priority, 20 as u16);
    expect_eq(srv_list[4].priority, 30 as u16);

    // 3. CAA (RFC 8659) encode/decode & critical flag
    const caa_in = CaaRecord.init(0x80, "issue", "letsencrypt.org");
    const caa_len = try caa_in.encodeRdata(&rdata_buf);
    let caa_out = try CaaRecord.parseWire(rdata_buf[0..caa_len], 0, caa_len as u16);
    expect_eq(caa_out.flags, 0x80 as u8);
    expect_eq(caa_out.isCritical(), true);
    expect_eq(caa_out.tag(), "issue");
    expect_eq(caa_out.value(), "letsencrypt.org");

    const caa_noncrit = CaaRecord.init(0, "iodef", "mailto:sec@cp-lang.org");
    const caa2_len = try caa_noncrit.encodeRdata(&rdata_buf);
    let caa2_out = try CaaRecord.parseWire(rdata_buf[0..caa2_len], 0, caa2_len as u16);
    expect_eq(caa2_out.isCritical(), false);
    expect_eq(caa2_out.tag(), "iodef");
    expect_eq(caa2_out.value(), "mailto:sec@cp-lang.org");

    // Invalid CAA tag (contains '-') rejected
    let bad_caa: [6]u8 = [0, 3, 'a', '-', 'b', 'x'];
    let got_bad_caa = false;
    * = CaaRecord.parseWire(&bad_caa, 0, 6) catch { got_bad_caa = true; };
    expect_eq(got_bad_caa, true);
}

fn testDohGetPostAndJsonResponse(): !void {
    const alloc = std.heap.page_allocator;

    // 1. DoH GET URL construction (unpadded Base64URL) + roundtrip decode
    let wire_pkt: [64]u8 = [0; 64];
    const pkt_len = DnsWireCodec.buildQueryPacket(0x1337, "api.cp-lang.org", NSType.A, &wire_pkt);
    expect_eq(pkt_len > 12, true);

    const url1 = try DohQueryBuilder.buildGetUrl(alloc, "https://dns.google/dns-query", wire_pkt[0..pkt_len]);
    defer alloc.free(url1);
    expect_eq(std.mem.startsWith(u8, url1, "https://dns.google/dns-query?dns="), true);

    const param_pos = std.mem.indexOf(u8, url1, "dns=").?;
    const b64_part = url1[param_pos + 4 ..];
    // Must not contain '=' padding
    expect_eq(std.mem.indexOfScalar(u8, b64_part, '=') == null, true);

    const decoded_pkt = try DohQueryBuilder.decodeDnsParam(alloc, b64_part);
    defer alloc.free(decoded_pkt);
    expect_eq(decoded_pkt, wire_pkt[0..pkt_len]);

    // Endpoint with existing query string uses '&dns='
    const url2 = try DohQueryBuilder.buildDnsQueryGetUrl(
        alloc,
        "https://cloudflare-dns.com/dns-query?ct=application/dns-message",
        0x2244,
        "cp-lang.org",
        NSType.AAAA,
    );
    defer alloc.free(url2);
    expect_eq(
        std.mem.startsWith(u8, url2, "https://cloudflare-dns.com/dns-query?ct=application/dns-message&dns="),
        true,
    );

    // 2. DoH POST headers
    const post_hdrs = DohQueryBuilder.buildPostHeaders();
    expect_eq(post_hdrs.content_type, "application/dns-message");
    expect_eq(post_hdrs.accept, "application/dns-message");
    expect_eq(post_hdrs.headers[0].name, "Content-Type");
    expect_eq(post_hdrs.headers[0].value, "application/dns-message");
    expect_eq(post_hdrs.headers[1].name, "Accept");
    expect_eq(post_hdrs.headers[1].value, "application/dns-message");

    // 3. DoH JSON Response Parser (Cloudflare/Google schema)
    const doh_json =
        "{\"Status\":0,\"TC\":false,\"RD\":true,\"RA\":true,\"AD\":true,\"CD\":false," +
        "\"Question\":[{\"name\":\"cp-lang.org.\",\"type\":1}]," +
        "\"Answer\":[" +
        "{\"name\":\"cp-lang.org.\",\"type\":1,\"TTL\":300,\"data\":\"104.21.10.20\"}," +
        "{\"name\":\"cp-lang.org.\",\"type\":28,\"TTL\":120,\"data\":\"2606:4700:3030::6815:a14\"}" +
        "]}";

    let resp = try DohJsonResponse.parse(alloc, doh_json);
    defer resp.deinit(alloc);

    expect_eq(resp.status, 0 as u16);
    expect_eq(resp.rcode() == DnsRCode.NoError, true);
    expect_eq(resp.tc, false);
    expect_eq(resp.rd, true);
    expect_eq(resp.ra, true);
    expect_eq(resp.ad, true);
    expect_eq(resp.cd, false);
    expect_eq(resp.questions.len, 1 as usize);
    expect_eq(resp.questions[0].name, "cp-lang.org.");
    expect_eq(resp.questions[0].rtype, 1 as u16);
    expect_eq(resp.answers.len, 2 as usize);
    expect_eq(resp.answers[0].name, "cp-lang.org.");
    expect_eq(resp.answers[0].rtype, 1 as u16);
    expect_eq(resp.answers[0].nsType() == NSType.A, true);
    expect_eq(resp.answers[0].ttl, 300 as u32);
    expect_eq(resp.answers[0].data, "104.21.10.20");
    expect_eq(resp.answers[1].nsType() == NSType.AAAA, true);
    expect_eq(resp.answers[1].ttl, 120 as u32);
    expect_eq(resp.answers[1].data, "2606:4700:3030::6815:a14");

    // Malformed JSON or missing Status rejected cleanly
    let bad_json_err = false;
    * = DohJsonResponse.parse(alloc, "{\"RD\":true}") catch { bad_json_err = true; };
    expect_eq(bad_json_err, true);
}

fn testResolvConfAndSearchExpansion(): !void {
    const alloc = std.heap.page_allocator;

    const resolv_text =
        "# Generated by NetworkManager\n" +
        "; Legacy semicolon comment\n" +
        "domain old.example.com\n" +
        "search svc.cluster.local cluster.local corp.internal # inline comment\n" +
        "nameserver 1.1.1.1 ; Cloudflare\n" +
        "nameserver 2606:4700:4700::1111\n" +
        "nameserver 8.8.8.8\n" +
        "options ndots:2 timeout:3 attempts:4 rotate edns0 trust-ad use-vc\n";

    let rc = ResolvConf.parse(resolv_text);
    const nss = rc.getNameservers();
    expect_eq(nss.len, 3 as usize);
    expect_eq(nss[0], "1.1.1.1");
    expect_eq(nss[1], "2606:4700:4700::1111");
    expect_eq(nss[2], "8.8.8.8");

    const sdoms = rc.getSearchDomains();
    expect_eq(sdoms.len, 3 as usize);
    expect_eq(sdoms[0], "svc.cluster.local");
    expect_eq(sdoms[1], "cluster.local");
    expect_eq(sdoms[2], "corp.internal");

    expect_eq(rc.ndots, 2 as u8);
    expect_eq(rc.timeout, 3 as u16);
    expect_eq(rc.attempts, 4 as u8);
    expect_eq(rc.rotate, true);
    expect_eq(rc.edns0, true);
    expect_eq(rc.trust_ad, true);
    expect_eq(rc.use_vc, true);

    // 1. dots < ndots (1 < 2): search domains tried BEFORE raw host
    const exp1 = try rc.expandSearchDomains(alloc, "redis.cache");
    defer ResolvConf.freeExpandedDomains(alloc, exp1);
    expect_eq(exp1.len, 4 as usize);
    expect_eq(exp1[0], "redis.cache.svc.cluster.local");
    expect_eq(exp1[1], "redis.cache.cluster.local");
    expect_eq(exp1[2], "redis.cache.corp.internal");
    expect_eq(exp1[3], "redis.cache");

    // 2. dots >= ndots (2 >= 2): raw host tried FIRST, then search domains
    const exp2 = try rc.expandSearchDomains(alloc, "api.cp-lang.org");
    defer ResolvConf.freeExpandedDomains(alloc, exp2);
    expect_eq(exp2.len, 4 as usize);
    expect_eq(exp2[0], "api.cp-lang.org");
    expect_eq(exp2[1], "api.cp-lang.org.svc.cluster.local");

    // 3. Trailing-dot FQDN bypasses search expansion
    const exp3 = try rc.expandSearchDomains(alloc, "db.internal.");
    defer ResolvConf.freeExpandedDomains(alloc, exp3);
    expect_eq(exp3.len, 1 as usize);
    expect_eq(exp3[0], "db.internal.");

    // 4. Host /etc/resolv.conf load test (non-fatal if sandbox lacks file, otherwise verified)
    if (sys_rc := ResolvConf.loadSystemOrNull(alloc)) {
        let mut_sys = sys_rc;
        defer mut_sys.deinit(alloc);
        expect_eq(mut_sys.ndots <= 15, true);
    }
}

fn testExistingCapDnsZeroRegression(): !void {
    // Verify all existing packet.cp, cache.cp, and _cares.cp APIs work seamlessly
    let pkt: [64]u8 = [0; 64];
    const q_len = DnsWireCodec.buildQueryPacket(0xBEEF, "dns.cp-lang.org", NSType.SRV, &pkt);
    expect_eq(q_len, 33 as usize);

    const hdr = DnsHeader.decode(pkt[0..12]).?;
    expect_eq(hdr.id, 0xBEEF as u16);
    expect_eq(hdr.qd_count, 1 as u16);

    let hosts = HostsTable.parse("127.0.0.1 localhost\n10.0.0.5 api.local\n");
    expect_eq(hosts.lookupIPv4("api.local").?, 0x0A000005 as u32);

    let cache = DnsCache<8>.init();
    expect_eq(cache.put("api.local", 0x0A000005, 80, 100, 1000), true);
    expect_eq(cache.lookup("api.local", 80, 500).?, 0x0A000005 as u32);
    expect_eq(cache.lookup("api.local", 80, 1500) == null, true);

    expect_eq(CaresStatus.codeName(ARES_SUCCESS), "SUCCESS");
    expect_eq(CaresStatus.isRetryable(ARES_ETIMEOUT), true);
    expect_eq(CaresStatus.isRetryable(ARES_ENOTFOUND), false);
    expect_eq(DnsFamily.fromString("IPv6") == DnsFamily.IPv6, true);
    expect_eq((NSClass.In as u16), 1 as u16);
}

fn main() {
    test("1. cap:dns — DnsNameCodec 16-hop pointer loop guard & IPv4/IPv6 PTR .arpa builder", () => {
        try testPointerLoopProtectionAndPtrArpa();
    });
    test("2. cap:dns — SOA, SRV (RFC 2782 sort/shuffle) & CAA (RFC 8659) wire codecs", () => {
        try testSoaSrvCaaRecordsAndSorting();
    });
    test("3. cap:dns — RFC 8484 DoH GET URL / POST headers & JSON response parser", () => {
        try testDohGetPostAndJsonResponse();
    });
    test("4. cap:dns — POSIX /etc/resolv.conf parser, system loader & ndots search expansion", () => {
        try testResolvConfAndSearchExpansion();
    });
    test("5. cap:dns — Existing DnsWireCodec, HostsTable, DnsCache & c-ares zero-regression", () => {
        try testExistingCapDnsZeroRegression();
    });
}
