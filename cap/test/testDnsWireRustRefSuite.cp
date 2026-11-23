import { test, expect_eq } from "cap:test";
import {
    DNS_MAX_POINTER_JUMPS,
    DnsWireError,
    DnsHeader,
    DnsQuestion,
    MxData,
    SrvData,
    SoaData,
    CaaData,
    DnsRdata,
    DnsResourceRecord,
    DnsPacket,
    dnsNameEqlIgnoreCaseScalar,
    dnsNameEqlIgnoreCaseSimd,
    copyDnsBytesScalar,
    copyDnsBytesSimd,
    decodeDnsName,
    encodeDnsPacket,
    decodeDnsPacket,
    parseEdns0Opt,
    validateDnsPacket,
} from "../../lib/dns/wire.cp";

const std = @import("std");

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn runTestAllRecordTypesAndCompression(): void! {
    const alloc = std.heap.page_allocator;
    let questions: [1]DnsQuestion = [
        DnsQuestion{ name: "www.example.com", qtype: 1, qclass: 1 },
    ];
    let txt_chunks: [2][]const u8 = ["v=spf1 ", "~all"];
    let answers: [8]DnsResourceRecord = [
        DnsResourceRecord{ name: "www.example.com", rtype: 1, rclass: 1, ttl: 300, rdata: DnsRdata{ a: [93, 184, 216, 34] } },
        DnsResourceRecord{ name: "www.example.com", rtype: 28, rclass: 1, ttl: 300, rdata: DnsRdata{ aaaa: [0x26, 0x06, 0x28, 0x00, 0x02, 0x20, 0x00, 0x01, 0, 0, 0, 0, 0, 0, 0, 1] } },
        DnsResourceRecord{ name: "alias.example.com", rtype: 5, rclass: 1, ttl: 60, rdata: DnsRdata{ cname: "www.example.com" } },
        DnsResourceRecord{ name: "example.com", rtype: 15, rclass: 1, ttl: 600, rdata: DnsRdata{ mx: MxData{ preference: 10, exchange: "mail.example.com" } } },
        DnsResourceRecord{ name: "example.com", rtype: 16, rclass: 1, ttl: 120, rdata: DnsRdata{ txt: txt_chunks[0..] } },
        DnsResourceRecord{ name: "_https._tcp.example.com", rtype: 33, rclass: 1, ttl: 120, rdata: DnsRdata{ srv: SrvData{ priority: 10, weight: 20, port: 443, target: "www.example.com" } } },
        DnsResourceRecord{ name: "example.com", rtype: 6, rclass: 1, ttl: 3600, rdata: DnsRdata{ soa: SoaData{ mname: "ns1.example.com", rname: "hostmaster.example.com", serial: 2026112301, refresh: 7200, retry: 900, expire: 1209600, minimum: 300 } } },
        DnsResourceRecord{ name: "example.com", rtype: 257, rclass: 1, ttl: 300, rdata: DnsRdata{ caa: CaaData{ flags: 0, tag: "issue", value: "letsencrypt.org" } } },
    ];
    let additionals: [1]DnsResourceRecord = [
        // EDNS0 OPT: udp_payload_size=4096, ttl = (ext_rcode=0 << 24) | (version=0 << 16) | DO(0x8000)
        DnsResourceRecord{ name: "", rtype: 41, rclass: 4096, ttl: 0x00008000, rdata: DnsRdata{ opt: "" } },
    ];
    let empty_auth: []DnsResourceRecord = [];

    const pkt = DnsPacket{
        header: DnsHeader{ id: 0xABCD, qr: true, rd: true, ra: true, ad: true },
        questions: questions[0..],
        answers: answers[0..],
        authorities: empty_auth,
        additionals: additionals[0..],
    };

    const raw_uncomp = encodeDnsPacket(alloc, pkt, false) catch unreachable;
    defer alloc.free(raw_uncomp);
    const raw_comp = encodeDnsPacket(alloc, pkt, true) catch unreachable;
    defer alloc.free(raw_comp);
    expect_true(raw_comp.len < raw_uncomp.len)?;

    validateDnsPacket(raw_comp) catch unreachable;
    let decoded = decodeDnsPacket(alloc, raw_comp) catch unreachable;
    defer decoded.deinit(alloc);

    expect_eq(decoded.header.id, 0xABCD);
    expect_eq(decoded.questions.len, 1);
    expect_eq(decoded.answers.len, 8);
    expect_eq(decoded.additionals.len, 1);

    const opt_info = parseEdns0Opt(decoded.additionals[0]).?;
    expect_eq(opt_info.udp_payload_size, 4096);
    expect_eq(opt_info.version, 0);
    expect_eq(opt_info.dnssec_ok, true);

    switch (decoded.answers[7].rdata) {
        .caa c => {
            expect_eq(c.flags, 0);
            expect_true(bytesEqual(c.tag, "issue"))?;
            expect_true(bytesEqual(c.value, "letsencrypt.org"))?;
        },
        else => expect_true(false)?,
    }
}

fn runTestSecurityBoundaries(): void! {
    const alloc = std.heap.page_allocator;
    expect_eq(DNS_MAX_POINTER_JUMPS, 16);

    // 1. Self-referential pointer loop at offset 12 (0xC0 0x0C)
    const self_loop: [18]u8 = [
        0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC0, 0x0C, 0x00, 0x01, 0x00, 0x01,
    ];
    let got_self = false;
    * = validateDnsPacket(self_loop[0..]) catch |e| {
        if (e == DnsWireError.CompressionLoop) got_self = true;
    };
    expect_true(got_self)?;

    // 2. Forward pointer (offset 12 points to offset 14 -> 0xC0 0x0E)
    const fwd_ptr: [20]u8 = [
        0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC0, 0x0E, 0x01, 'a', 0x00, 0x00, 0x01, 0x00,
    ];
    let got_fwd = false;
    * = validateDnsPacket(fwd_ptr[0..]) catch |e| {
        if (e == DnsWireError.CompressionLoop) got_fwd = true;
    };
    expect_true(got_fwd)?;

    // 3. >16 pointer hops rejected
    // Build chain: offset 12="a\0", offset 14->12, offset 16->14, ... 18 hops
    let chain: [64]u8 = [0; 64];
    chain[5] = 1; // qd_count = 1 (unused, we call decodeDnsName directly at end)
    chain[12] = 1;
    chain[13] = 'a';
    chain[14] = 0;
    let h: usize = 0;
    let cur_off: usize = 15;
    while (h < 18) : (h += 1) {
        const prev = if (h == 0) 12 else (cur_off - 2);
        chain[cur_off] = 0xC0;
        chain[cur_off + 1] = prev as u8;
        cur_off += 2;
    }
    let got_jumps = false;
    * = decodeDnsName(alloc, chain[0..], cur_off - 2) catch |e| {
        if (e == DnsWireError.CompressionLoop) got_jumps = true;
    };
    expect_true(got_jumps)?;

    // 4. >255B name rejected
    let long_pkt: [300]u8 = [0; 300];
    let lp: usize = 12;
    let seg: usize = 0;
    while (seg < 5) : (seg += 1) {
        long_pkt[lp] = 55;
        let ch: usize = 0;
        while (ch < 55) : (ch += 1) {
            long_pkt[lp + 1 + ch] = 'x';
        }
        lp += 56;
    }
    long_pkt[lp] = 0;
    let got_long = false;
    * = decodeDnsName(alloc, long_pkt[0..], 12) catch |e| {
        if (e == DnsWireError.NameTooLong) got_long = true;
    };
    expect_true(got_long)?;

    // 5. Invalid A record rdlen != 4
    const bad_a: [29]u8 = [
        0x00, 0x01, 0x81, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x01, 'a', 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x03,
        1, 2, 3, 0,
    ];
    let got_bad_a = false;
    * = validateDnsPacket(bad_a[0..]) catch |e| {
        if (e == DnsWireError.InvalidRdata) got_bad_a = true;
    };
    expect_true(got_bad_a)?;
}

fn runTestSimdParity(): void! {
    const n1 = "SubDomain.Example-Long-DNS-Name-For-16-Lane-SIMD.Org";
    const n2 = "subdomain.example-long-dns-name-for-16-lane-simd.org";
    const n3 = "subdomain.example-long-dns-name-f0r-16-lane-simd.org";
    expect_eq(dnsNameEqlIgnoreCaseSimd(n1, n2), dnsNameEqlIgnoreCaseScalar(n1, n2));
    expect_eq(dnsNameEqlIgnoreCaseSimd(n1, n2), true);
    expect_eq(dnsNameEqlIgnoreCaseSimd(n1, n3), dnsNameEqlIgnoreCaseScalar(n1, n3));
    expect_eq(dnsNameEqlIgnoreCaseSimd(n1, n3), false);

    let src: [129]u8 = [0; 129];
    let dst_sc: [129]u8 = [0; 129];
    let dst_sm: [129]u8 = [0; 129];
    let i: usize = 0;
    while (i < 129) : (i += 1) {
        src[i] = ((i * 31 + 11) & 0xFF) as u8;
    }
    const lens: [7]usize = [0, 1, 15, 16, 17, 64, 129];
    let li: usize = 0;
    while (li < lens.len) : (li += 1) {
        const n = lens[li];
        @memset(dst_sc[0..n], 0);
        @memset(dst_sm[0..n], 0);
        copyDnsBytesScalar(dst_sc[0..n], src[0..n]);
        copyDnsBytesSimd(dst_sm[0..n], src[0..n]);
        expect_true(bytesEqual(dst_sc[0..n], dst_sm[0..n]))?;
    }
}

fn tryDecodeDns(alloc: std.mem.Allocator, slice: []const u8): DnsPacket? {
    return decodeDnsPacket(alloc, slice) catch return null;
}

fn runTestMalformedFuzz(): void! {
    const alloc = std.heap.page_allocator;
    let seed: u64 = 0x428A2F98D728AE22;
    let buf: [256]u8 = [0; 256];
    let case_idx: usize = 0;
    while (case_idx < 1300) : (case_idx += 1) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        const len: usize = ((seed % 255) as usize) + 1;

        let j: usize = 0;
        while (j < len) : (j += 1) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            buf[j] = (seed & 0xFF) as u8;
        }
        if ((case_idx % 3) == 0 && len >= 12) {
            buf[4] = 0;
            buf[5] = 1;
        }

        const slice = buf[0..len];
        * = validateDnsPacket(slice) catch {};
        if (pkt := tryDecodeDns(alloc, slice)) {
            let p = pkt;
            p.deinit(alloc);
        }
    }
}

fn main() {
    test("1. RFC 1035/3596/6891/8659 all record types (A/AAAA/CNAME/MX/TXT/SRV/SOA/OPT/CAA) & compression", () => {
        runTestAllRecordTypesAndCompression()?;
    });
    test("2. Security boundaries: pointer loop, forward pointer, >16 jumps, >255B name & bad RDATA", () => {
        runTestSecurityBoundaries()?;
    });
    test("3. 16-lane SIMD vs Scalar 0-diff parity (dnsNameEqlIgnoreCase & copyDnsBytes)", () => {
        runTestSimdParity()?;
    });
    test("4. 1,300 deterministic PRNG malformed DNS wire fuzz cases (0 panic / 0 OOB)", () => {
        runTestMalformedFuzz()?;
    });
}
