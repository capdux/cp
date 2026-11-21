// Source: cp/cap/test/test_dns_wire_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DnsWireError,
    DnsRecordType,
    WireDnsClass as DnsClass,
    WireDnsHeader as DnsHeader,
    WireDnsQuestion as DnsQuestion,
    DnsRdata,
    WireResourceRecord as DnsResourceRecord,
    DnsPacket,
    MxData,
    SrvData,
    SoaData,
    decodeDnsName,
    encodeDnsPacket,
    decodeDnsPacket,
} from "cap:dns";

test("1. DnsHeader 12-byte big-endian flags and counts roundtrip") {
    const hdr = DnsHeader{
        .id = 0xBEEF,
        .qr = true,
        .opcode = 0,
        .aa = true,
        .tc = false,
        .rd = true,
        .ra = true,
        .ad = true,
        .cd = false,
        .rcode = 3,
        .qd_count = 1,
        .an_count = 2,
        .ns_count = 1,
        .ar_count = 1,
    };
    const raw = hdr.encode();
    const out_hdr = try DnsHeader.decode(&raw);
    expect_eq(out_hdr.id, 0xBEEF);
    expect_eq(out_hdr.qr, true);
    expect_eq(out_hdr.aa, true);
    expect_eq(out_hdr.tc, false);
    expect_eq(out_hdr.rd, true);
    expect_eq(out_hdr.ra, true);
    expect_eq(out_hdr.ad, true);
    expect_eq(out_hdr.rcode, 3);
    expect_eq(out_hdr.an_count, 2);
}

test("2. Full DNS query/response packet with suffix compression roundtrip") {
    const alloc = std.testing.allocator;
    let qs: [1]DnsQuestion = [
        DnsQuestion{ .name = "api.example.com", .qtype = @intFromEnum(DnsRecordType.A), .qclass = @intFromEnum(DnsClass.IN) },
    ];
    let txt_chunks: [2][]const u8 = [ "v=spf1 ", "include:example.com" ];
    let a_ip: [4]u8 = [ 93, 184, 216, 34 ];
    let aaaa_ip: [16]u8 = [ 0x26, 0x06, 0x28, 0x00, 0x02, 0x20, 0x00, 0x01, 0, 0, 0, 0, 0, 0, 0x02, 0x48 ];
    let ans: [5]DnsResourceRecord = [
        DnsResourceRecord{ .name = "api.example.com", .rtype = @intFromEnum(DnsRecordType.CNAME), .ttl = 300, .rdata = DnsRdata{ .cname = "edge.example.com" } },
        DnsResourceRecord{ .name = "edge.example.com", .rtype = @intFromEnum(DnsRecordType.A), .ttl = 60, .rdata = DnsRdata{ .a = a_ip } },
        DnsResourceRecord{ .name = "edge.example.com", .rtype = @intFromEnum(DnsRecordType.AAAA), .ttl = 60, .rdata = DnsRdata{ .aaaa = aaaa_ip } },
        DnsResourceRecord{ .name = "example.com", .rtype = @intFromEnum(DnsRecordType.MX), .ttl = 600, .rdata = DnsRdata{ .mx = MxData{ .preference = 10, .exchange = "mail.example.com" } } },
        DnsResourceRecord{ .name = "example.com", .rtype = @intFromEnum(DnsRecordType.TXT), .ttl = 120, .rdata = DnsRdata{ .txt = &txt_chunks } },
    ];
    let empty_rrs: [0]DnsResourceRecord = [];
    const pkt = DnsPacket{
        .header = DnsHeader{ .id = 0x1234, .qr = true, .rd = true, .ra = true },
        .questions = &qs,
        .answers = &ans,
        .authorities = &empty_rrs,
        .additionals = &empty_rrs,
    };

    const uncompressed = try encodeDnsPacket(alloc, pkt, false);
    defer alloc.free(uncompressed);
    const compressed = try encodeDnsPacket(alloc, pkt, true);
    defer alloc.free(compressed);
    try std.testing.expect(compressed.len < uncompressed.len);

    let decoded = try decodeDnsPacket(alloc, compressed);
    defer decoded.deinit(alloc);
    expect_eq(decoded.questions.len, 1);
    try std.testing.expectEqualStrings("api.example.com", decoded.questions[0].name);
    expect_eq(decoded.answers.len, 5);
    try std.testing.expectEqualStrings("edge.example.com", decoded.answers[0].rdata.cname);
    expect_eq(decoded.answers[1].rdata.a[0], 93);
    expect_eq(decoded.answers[3].rdata.mx.preference, 10);
    try std.testing.expectEqualStrings("mail.example.com", decoded.answers[3].rdata.mx.exchange);
    try std.testing.expectEqualStrings("v=spf1 ", decoded.answers[4].rdata.txt[0]);
}

test("3. SRV, SOA and OPT (EDNS0) records roundtrip") {
    const alloc = std.testing.allocator;
    let auths: [1]DnsResourceRecord = [
        DnsResourceRecord{
            .name = "example.org",
            .rtype = @intFromEnum(DnsRecordType.SOA),
            .ttl = 3600,
            .rdata = DnsRdata{ .soa = SoaData{
                .mname = "ns1.example.org",
                .rname = "hostmaster.example.org",
                .serial = 2026112101,
                .refresh = 7200,
                .retry = 900,
                .expire = 1209600,
                .minimum = 300,
            } },
        },
    ];
    let empty_bytes: [0]u8 = [];
    let adds: [2]DnsResourceRecord = [
        DnsResourceRecord{
            .name = "_grpc._tcp.example.org",
            .rtype = @intFromEnum(DnsRecordType.SRV),
            .ttl = 60,
            .rdata = DnsRdata{ .srv = SrvData{ .priority = 5, .weight = 100, .port = 8443, .target = "svc.example.org" } },
        },
        DnsResourceRecord{
            .name = "",
            .rtype = @intFromEnum(DnsRecordType.OPT),
            .rclass = 4096,
            .ttl = 0,
            .rdata = DnsRdata{ .opt = &empty_bytes },
        },
    ];
    let empty_qs: [0]DnsQuestion = [];
    let empty_rrs: [0]DnsResourceRecord = [];
    const pkt = DnsPacket{
        .header = DnsHeader{ .id = 0x7777, .qr = true },
        .questions = &empty_qs,
        .answers = &empty_rrs,
        .authorities = &auths,
        .additionals = &adds,
    };

    const wire = try encodeDnsPacket(alloc, pkt, true);
    defer alloc.free(wire);
    let decoded_pkt = try decodeDnsPacket(alloc, wire);
    defer decoded_pkt.deinit(alloc);

    expect_eq(decoded_pkt.authorities[0].rdata.soa.serial, 2026112101);
    try std.testing.expectEqualStrings("ns1.example.org", decoded_pkt.authorities[0].rdata.soa.mname);
    expect_eq(decoded_pkt.additionals[0].rdata.srv.port, 8443);
    expect_eq(decoded_pkt.additionals[1].rtype, @intFromEnum(DnsRecordType.OPT));
    expect_eq(decoded_pkt.additionals[1].rclass, 4096);
}

test("4. Compression pointer loop and truncated/malformed packet protection") {
    const alloc = std.testing.allocator;
    const loop_pkt: [18]u8 = [
        0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC0, 0x0C, 0x00, 0x01, 0x00, 0x01,
    ];
    try std.testing.expectError(DnsWireError.CompressionLoop, decodeDnsName(alloc, &loop_pkt, 12));
    try std.testing.expectError(DnsWireError.CompressionLoop, decodeDnsPacket(alloc, &loop_pkt));

    try std.testing.expectError(DnsWireError.UnexpectedEof, DnsHeader.decode(loop_pkt[0..5]));

    const bad_tag: [20]u8 = [
        0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
        0x40, 0x01, 'a', 0, 0, 1, 0, 1,
    ];
    try std.testing.expectError(DnsWireError.InvalidLabel, decodeDnsPacket(alloc, &bad_tag));
}

test("5. Dynamic cap.dns_wire namespace and zero-leak check") {
    const alloc = std.testing.allocator;
    const hdr = cap.dns_wire.DnsHeader{ .id = 0xCAFE, .rd = true };
    let qs: [1]cap.dns_wire.DnsQuestion = [
        cap.dns_wire.DnsQuestion{ .name = "cap-lang.org", .qtype = 1, .qclass = 1 },
    ];
    let empty_rrs: [0]cap.dns_wire.DnsResourceRecord = [];
    const pkt = cap.dns_wire.DnsPacket{
        .header = hdr,
        .questions = &qs,
        .answers = &empty_rrs,
        .authorities = &empty_rrs,
        .additionals = &empty_rrs,
    };
    const wire = try cap.dns_wire.encodeDnsPacket(alloc, pkt, false);
    defer alloc.free(wire);
    let decoded_pkt = try cap.dns_wire.decodeDnsPacket(alloc, wire);
    defer decoded_pkt.deinit(alloc);
    expect_eq(decoded_pkt.header.id, 0xCAFE);
    try std.testing.expectEqualStrings("cap-lang.org", decoded_pkt.questions[0].name);
}
