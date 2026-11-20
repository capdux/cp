// Source: cp/cap/test/test_pcap_production_deep.cp
// Deep Production Test Suite for cap:pcap
// Covers:
//   1. Little/Big endian + Microsecond/Nanosecond magic PcapWriter & parsePcap multi-packet round-trip
//   2. Ethernet II (with 802.1Q VLAN tag) -> IPv4 (with options) -> TCP (SYN/ACK/PSH, seq/ack, options & payload)
//   3. Ethernet II -> IPv4 -> UDP DNS/RTP datagram dissection with padding trimming
//   4. Guards: InvalidMagic, UnsupportedVersion, TruncatedHeader/TruncatedPacket, PacketTooLarge, malformed L2/L3/L4
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.pcap` & std.testing.allocator zero-leak

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PcapWriter,
    PcapFile,
    PcapPacket,
    PcapGlobalHeader,
    PcapEndian,
    PcapPrecision,
    LinkType,
    EthernetFrame,
    Ipv4Packet,
    TcpSegment,
    UdpDatagram,
    PcapError,
    parsePcap,
    parsePcapWithMaxSnaplen,
    parseEthernet,
    parseIpv4,
    parseTcp,
    parseUdp,
} from "cap:pcap";
import cap from "cap";

fn verifyRoundTripCombo(
    alloc: std.mem.Allocator,
    endian: PcapEndian,
    precision: PcapPrecision,
    expected_magic: []const u8,
)! {
    let writer = PcapWriter.init(alloc, endian, precision, 65535, @intFromEnum(LinkType.Ethernet)) catch unreachable;
    defer writer.deinit();

    const pkt1 = "\xaa\xbb\xcc\xdd\xee\xff";
    const pkt2 = "\x01\x02\x03\x04\x05\x06\x07\x08";
    writer.addPacket(1700000001, 123456, 60, pkt1) catch unreachable;
    writer.addPacket(1700000002, 789012, 128, pkt2) catch unreachable;

    const raw = writer.finish(alloc) catch unreachable;
    defer alloc.free(raw);

    expect_eq(std.mem.eql(u8, raw[0..4], expected_magic), true);

    let parsed = parsePcap(alloc, raw) catch unreachable;
    defer parsed.deinit(alloc);

    expect_eq(parsed.header.version_major, 2 as u16);
    expect_eq(parsed.header.version_minor, 4 as u16);
    expect_eq(parsed.header.snaplen, 65535 as u32);
    expect_eq(parsed.header.link_type, 1 as u32);
    expect_eq(parsed.header.endian == endian, true);
    expect_eq(parsed.header.precision == precision, true);

    expect_eq(parsed.packets.len, 2 as usize);
    expect_eq(parsed.packets[0].ts_sec, 1700000001 as u32);
    expect_eq(parsed.packets[0].ts_subsec, 123456 as u32);
    expect_eq(parsed.packets[0].orig_len, 60 as u32);
    expect_eq(std.mem.eql(u8, parsed.packets[0].data, pkt1), true);

    expect_eq(parsed.packets[1].ts_sec, 1700000002 as u32);
    expect_eq(parsed.packets[1].ts_subsec, 789012 as u32);
    expect_eq(parsed.packets[1].orig_len, 128 as u32);
    expect_eq(std.mem.eql(u8, parsed.packets[1].data, pkt2), true);
}

test("1. Little/Big Endian & Microsecond/Nanosecond Magic PcapWriter/parsePcap Round-Trip") {
    const alloc = std.testing.allocator;

    verifyRoundTripCombo(alloc, .Little, .Microsecond, "\xd4\xc3\xb2\xa1")?;
    verifyRoundTripCombo(alloc, .Big, .Microsecond, "\xa1\xb2\xc3\xd4")?;
    verifyRoundTripCombo(alloc, .Little, .Nanosecond, "\x4d\x3c\xb2\xa1")?;
    verifyRoundTripCombo(alloc, .Big, .Nanosecond, "\xa1\xb2\x3c\x4d")?;
}

test("2. Ethernet II (802.1Q VLAN) -> IPv4 (Options) -> TCP Dissection") {
    const alloc = std.testing.allocator;

    // Build Ethernet (14) + 802.1Q VLAN (4, TPID=0x8100, TCI=0x0064, inner=0x0800)
    // + IPv4 (24 bytes, IHL=6 -> 24, proto=6 TCP, total_len=52, src=10.0.0.1, dst=10.0.0.2)
    // + TCP (24 bytes, data_offset=6 -> 24, src_port=51234, dst_port=443, flags=0x18 PSH|ACK)
    // + Payload "PING" (4 bytes)
    const frame: [70]u8 = [
        // dst MAC: 00:11:22:33:44:55
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
        // src MAC: 66:77:88:99:aa:bb
        0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
        // EtherType: 802.1Q (0x8100), VLAN ID 100, Inner EtherType: IPv4 (0x0800)
        0x81, 0x00, 0x00, 0x64, 0x08, 0x00,
        // IPv4: ver=4, ihl=6 (24B), tos=0, total_len=52 (0x0034)
        0x46, 0x00, 0x00, 0x34,
        0x12, 0x34, 0x40, 0x00,
        64, 6, 0x00, 0x00,
        10, 0, 0, 1,
        10, 0, 0, 2,
        // IPv4 4-byte option (NOPs)
        0x01, 0x01, 0x01, 0x01,
        // TCP: src=51234 (0xc822), dst=443 (0x01bb)
        0xc8, 0x22, 0x01, 0xbb,
        // seq = 0x11223344
        0x11, 0x22, 0x33, 0x44,
        // ack = 0x55667788
        0x55, 0x66, 0x77, 0x88,
        // data_offset=6 (24B -> 0x60), flags=0x18 (PSH|ACK), window=64240 (0xfaf0)
        0x60, 0x18, 0xfa, 0xf0,
        // checksum + urgent
        0x00, 0x00, 0x00, 0x00,
        // TCP 4-byte option
        0x02, 0x04, 0x05, 0xb4,
        // Payload: "PING"
        'P', 'I', 'N', 'G',
    ];

    let writer = PcapWriter.init(alloc, .Little, .Microsecond, 1514, @intFromEnum(LinkType.Ethernet)) catch unreachable;
    defer writer.deinit();
    writer.addPacket(1700000100, 500, 70, &frame) catch unreachable;
    const raw = writer.finish(alloc) catch unreachable;
    defer alloc.free(raw);

    let file = parsePcap(alloc, raw) catch unreachable;
    defer file.deinit(alloc);
    expect_eq(file.packets.len, 1 as usize);

    const eth = parseEthernet(file.packets[0].data) catch unreachable;
    expect_eq(std.mem.eql(u8, &eth.dst_mac, "\x00\x11\x22\x33\x44\x55"), true);
    expect_eq(std.mem.eql(u8, &eth.src_mac, "\x66\x77\x88\x99\xaa\xbb"), true);
    expect_eq(eth.ether_type, 0x0800 as u16);

    const ip = parseIpv4(eth.payload) catch unreachable;
    expect_eq(ip.ihl, 24 as u8);
    expect_eq(ip.protocol, 6 as u8);
    expect_eq(ip.ttl, 64 as u8);
    expect_eq(ip.total_len, 52 as u16);
    expect_eq(std.mem.eql(u8, &ip.src_ip, "\x0a\x00\x00\x01"), true);
    expect_eq(std.mem.eql(u8, &ip.dst_ip, "\x0a\x00\x00\x02"), true);

    const tcp = parseTcp(ip.payload) catch unreachable;
    expect_eq(tcp.src_port, 51234 as u16);
    expect_eq(tcp.dst_port, 443 as u16);
    expect_eq(tcp.seq, 0x11223344 as u32);
    expect_eq(tcp.ack, 0x55667788 as u32);
    expect_eq(tcp.data_offset, 24 as u8);
    expect_eq(tcp.flags, 0x18 as u8);
    expect_eq(tcp.window, 64240 as u16);
    expect_eq(std.mem.eql(u8, tcp.payload, "PING"), true);
}

test("3. Ethernet II -> IPv4 -> UDP DNS/RTP Datagram Dissection") {
    const alloc = std.testing.allocator;

    // Ethernet (14) + IPv4 (20, total_len=34, proto=17 UDP) + UDP (8, len=14) + "DNSQRY" (6) + 4B Ethernet padding
    const frame: [52]u8 = [
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60,
        0x08, 0x00,
        // IPv4: ver=4, ihl=5 (20B), total_len=34 (0x0022), ttl=128, proto=17
        0x45, 0x00, 0x00, 0x22,
        0x00, 0x01, 0x00, 0x00,
        128, 17, 0x00, 0x00,
        192, 168, 1, 10,
        8, 8, 8, 8,
        // UDP: src=5353 (0x14e9), dst=53 (0x0035), len=14 (0x000e)
        0x14, 0xe9, 0x00, 0x35,
        0x00, 0x0e, 0x00, 0x00,
        'D', 'N', 'S', 'Q', 'R', 'Y',
        // 4 bytes trailing link padding
        0x00, 0x00, 0x00, 0x00,
    ];

    const eth = parseEthernet(&frame) catch unreachable;
    expect_eq(eth.ether_type, 0x0800 as u16);

    const ip = parseIpv4(eth.payload) catch unreachable;
    expect_eq(ip.ihl, 20 as u8);
    expect_eq(ip.protocol, 17 as u8);
    expect_eq(ip.ttl, 128 as u8);
    expect_eq(ip.total_len, 34 as u16);
    expect_eq(std.mem.eql(u8, &ip.dst_ip, "\x08\x08\x08\x08"), true);

    const udp = parseUdp(ip.payload) catch unreachable;
    expect_eq(udp.src_port, 5353 as u16);
    expect_eq(udp.dst_port, 53 as u16);
    expect_eq(udp.length, 14 as u16);
    expect_eq(std.mem.eql(u8, udp.payload, "DNSQRY"), true);
    * = alloc;
}

test("4. Defensive Guards: InvalidMagic, UnsupportedVersion, Truncated, PacketTooLarge & Malformed L2-L4") {
    const alloc = std.testing.allocator;

    // 4a. Truncated global header (< 24 bytes)
    let trunc_hdr = false;
    * = parsePcap(alloc, "\xd4\xc3\xb2\xa1\x02\x00") catch |e| {
        if (e == PcapError.TruncatedHeader) trunc_hdr = true;
    };
    expect_eq(trunc_hdr, true);

    // 4b. Invalid magic
    const bad_magic: [24]u8 = [
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    ];
    let inv_magic = false;
    * = parsePcap(alloc, &bad_magic) catch |e| {
        if (e == PcapError.InvalidMagic) inv_magic = true;
    };
    expect_eq(inv_magic, true);

    // 4c. Unsupported version (2.3 instead of 2.4)
    const bad_ver: [24]u8 = [
        0xd4, 0xc3, 0xb2, 0xa1,
        0x02, 0x00, 0x03, 0x00,
        0, 0, 0, 0, 0, 0, 0, 0,
        0xff, 0xff, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
    ];
    let unsup_ver = false;
    * = parsePcap(alloc, &bad_ver) catch |e| {
        if (e == PcapError.UnsupportedVersion) unsup_ver = true;
    };
    expect_eq(unsup_ver, true);

    // 4d. PacketTooLarge via incl_len > snaplen & max_snaplen guard
    let writer = PcapWriter.init(alloc, .Little, .Microsecond, 64, @intFromEnum(LinkType.Ethernet)) catch unreachable;
    defer writer.deinit();
    writer.addPacket(1, 0, 10, "0123456789") catch unreachable;
    const valid_raw = writer.finish(alloc) catch unreachable;
    defer alloc.free(valid_raw);

    let too_large_max = false;
    * = parsePcapWithMaxSnaplen(alloc, valid_raw, 32) catch |e| {
        if (e == PcapError.PacketTooLarge) too_large_max = true;
    };
    expect_eq(too_large_max, true);

    // Mutate incl_len to 128 (> snaplen 64)
    let mutated = alloc.dupe(u8, valid_raw) catch unreachable;
    defer alloc.free(mutated);
    mutated[24 + 8] = 128;
    let too_large_incl = false;
    * = parsePcap(alloc, mutated) catch |e| {
        if (e == PcapError.PacketTooLarge) too_large_incl = true;
    };
    expect_eq(too_large_incl, true);

    // 4e. TruncatedPacket (cut off last 2 bytes of payload)
    let trunc_pkt = false;
    * = parsePcap(alloc, valid_raw[0 .. valid_raw.len - 2]) catch |e| {
        if (e == PcapError.TruncatedPacket) trunc_pkt = true;
    };
    expect_eq(trunc_pkt, true);

    // 4f. Malformed L2 / L3 / L4
    let bad_eth = false;
    * = parseEthernet("short") catch |e| {
        if (e == PcapError.InvalidEthernet) bad_eth = true;
    };
    expect_eq(bad_eth, true);

    const bad_ip_ver: [20]u8 = [0x60, 0, 0, 20, 0, 0, 0, 0, 64, 6, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2];
    let inv_ip = false;
    * = parseIpv4(&bad_ip_ver) catch |e| {
        if (e == PcapError.InvalidIpv4) inv_ip = true;
    };
    expect_eq(inv_ip, true);

    const bad_tcp_off: [20]u8 = [0, 80, 0, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0x30, 0, 0, 0, 0, 0, 0, 0];
    let inv_tcp = false;
    * = parseTcp(&bad_tcp_off) catch |e| {
        if (e == PcapError.InvalidTransport) inv_tcp = true;
    };
    expect_eq(inv_tcp, true);
}

test("5. Dynamic Lazy Namespace Mounting via cap.pcap & Zero-Leak") {
    const alloc = std.testing.allocator;

    let writer = cap.pcap.PcapWriter.init(
        alloc,
        .Big,
        .Nanosecond,
        1500,
        @intFromEnum(cap.pcap.LinkType.RawIp),
    ) catch unreachable;
    defer writer.deinit();

    const ip_udp: [28]u8 = [
        0x45, 0x00, 0x00, 0x1c,
        0x00, 0x01, 0x00, 0x00,
        64, 17, 0x00, 0x00,
        127, 0, 0, 1,
        127, 0, 0, 1,
        0x04, 0xd2, 0x16, 0x2e,
        0x00, 0x08, 0x00, 0x00,
    ];
    writer.addPacket(42, 999999999, 28, &ip_udp) catch unreachable;

    const raw = writer.finish(alloc) catch unreachable;
    defer alloc.free(raw);

    let file = cap.pcap.parsePcap(alloc, raw) catch unreachable;
    defer file.deinit(alloc);

    expect_eq(file.header.link_type, 101 as u32);
    expect_eq(file.packets.len, 1 as usize);

    const ip = cap.pcap.parseIpv4(file.packets[0].data) catch unreachable;
    const udp = cap.pcap.parseUdp(ip.payload) catch unreachable;
    expect_eq(udp.src_port, 1234 as u16);
    expect_eq(udp.dst_port, 5678 as u16);
    expect_eq(udp.payload.len, 0 as usize);
}
