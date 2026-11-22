// Source: cp/cap/test/test_rtp_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:rtp & cap.rtp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RtpError,
    RtpExtension,
    RtpPacket,
    RtcpType,
    RtcpSenderReport,
    RtcpBye,
    JitterBuffer,
    seqDiff,
} from "cap:net";

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Minimal 12-byte header + payload roundtrip
    const basic = RtpPacket {
        version: 2,
        padding: false,
        marker: true,
        payload_type: 111,
        sequence_number: 1024,
        timestamp: 96000,
        ssrc: 0xDEADBEEF,
        payload: "hello-rtp",
    };
    const basic_wire = basic.encode(alloc)?;
    defer alloc.free(basic_wire);
    expect_eq(basic_wire.len, (12 + 9) as usize);

    let basic_dec = RtpPacket.decode(alloc, basic_wire)?;
    defer basic_dec.deinit(alloc);
    expect_eq(basic_dec.version, 2 as u8);
    expect_eq(basic_dec.padding, false);
    expect_eq(basic_dec.marker, true);
    expect_eq(basic_dec.payload_type, 111 as u7);
    expect_eq(basic_dec.sequence_number, 1024 as u16);
    expect_eq(basic_dec.timestamp, 96000 as u32);
    expect_eq(basic_dec.ssrc, 0xDEADBEEF as u32);
    expect_eq(basic_dec.csrcs.len, 0 as usize);
    expect_eq(basic_dec.extension == null, true);
    expect_eq(std.mem.eql(u8, basic_dec.payload, "hello-rtp"), true);

    // 2. Full packet with CSRC list, Header Extension, and Padding
    const csrc_arr: [2]u32 = [0x11112222, 0x33334444];
    const ext_data: [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0x01, 0x02, 0x03, 0x04];
    const full = RtpPacket {
        version: 2,
        padding: true,
        marker: false,
        payload_type: 96,
        sequence_number: 42,
        timestamp: 123456,
        ssrc: 0xCAFEBABE,
        csrcs: &csrc_arr,
        extension: RtpExtension { profile: 0xBEDE, data: &ext_data },
        payload: "opus-audio-frame",
    };
    const full_wire = full.encode(alloc)?;
    defer alloc.free(full_wire);
    // Must be 4-byte aligned when padding is enabled
    expect_eq(full_wire.len % 4, 0 as usize);

    let full_dec = RtpPacket.decode(alloc, full_wire)?;
    defer full_dec.deinit(alloc);
    expect_eq(full_dec.padding, true);
    expect_eq(full_dec.marker, false);
    expect_eq(full_dec.payload_type, 96 as u7);
    expect_eq(full_dec.csrcs.len, 2 as usize);
    expect_eq(full_dec.csrcs[0], 0x11112222 as u32);
    expect_eq(full_dec.csrcs[1], 0x33334444 as u32);
    const ext = full_dec.extension orelse unreachable;
    expect_eq(ext.profile, 0xBEDE as u16);
    expect_eq(std.mem.eql(u8, ext.data, &ext_data), true);
    expect_eq(std.mem.eql(u8, full_dec.payload, "opus-audio-frame"), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // 1. RTCP Sender Report (PT=200)
    const sr = RtcpSenderReport {
        ssrc: 0x10203040,
        ntp_msw: 0xE1234567,
        ntp_lsw: 0x89ABCDEF,
        rtp_timestamp: 480000,
        packet_count: 500,
        octet_count: 64000,
    };
    const sr_wire = sr.encode(alloc)?;
    defer alloc.free(sr_wire);
    expect_eq(sr_wire.len, 28 as usize);
    expect_eq(sr_wire[1], RtcpType.SenderReport as u8);
    expect_eq(sr_wire[2], 0 as u8);
    expect_eq(sr_wire[3], 6 as u8);

    const sr_dec = RtcpSenderReport.decode(sr_wire)?;
    expect_eq(sr_dec.ssrc, 0x10203040 as u32);
    expect_eq(sr_dec.ntp_msw, 0xE1234567 as u32);
    expect_eq(sr_dec.ntp_lsw, 0x89ABCDEF as u32);
    expect_eq(sr_dec.rtp_timestamp, 480000 as u32);
    expect_eq(sr_dec.packet_count, 500 as u32);
    expect_eq(sr_dec.octet_count, 64000 as u32);

    // 2. RTCP BYE (PT=203) with multiple SSRCs and reason
    let bye_ssrcs: [2]u32 = [0xAABBCCDD, 0x11223344];
    const bye = RtcpBye {
        ssrc_list: &bye_ssrcs,
        reason: "session closed",
    };
    const bye_wire = bye.encode(alloc)?;
    defer alloc.free(bye_wire);
    expect_eq(bye_wire.len % 4, 0 as usize);
    expect_eq(bye_wire[1], RtcpType.Bye as u8);

    let bye_dec = RtcpBye.decode(alloc, bye_wire)?;
    defer bye_dec.deinit(alloc);
    expect_eq(bye_dec.ssrc_list.len, 2 as usize);
    expect_eq(bye_dec.ssrc_list[0], 0xAABBCCDD as u32);
    expect_eq(bye_dec.ssrc_list[1], 0x11223344 as u32);
    const r = bye_dec.reason orelse unreachable;
    expect_eq(std.mem.eql(u8, r, "session closed"), true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // Verify seqDiff wrap-around arithmetic
    expect_eq(seqDiff(0, 65535), 1 as i32);
    expect_eq(seqDiff(65535, 0), -1 as i32);
    expect_eq(seqDiff(1, 65534), 3 as i32);

    let jb = JitterBuffer.init(alloc, 8);
    defer jb.deinit();

    // Push first packet at 65534, then out-of-order across u16 wrap-around: 1, 65535, 0
    jb.push(65534, 1000, "p-65534")?;
    jb.push(1, 4000, "p-1")?;
    jb.push(65535, 2000, "p-65535")?;
    jb.push(0, 3000, "p-0")?;
    // Duplicate 65535 should be counted and ignored
    jb.push(65535, 2000, "dup-65535")?;

    let st1 = jb.stats();
    expect_eq(st1.buffered, 4 as usize);
    expect_eq(st1.duplicates, 1 as u64);

    // Pop in strict wrap-around order: 65534 -> 65535 -> 0 -> 1
    const expected_seqs: [4]u16 = [65534, 65535, 0, 1];
    let idx: usize = 0;
    while (idx < 4) : (idx += 1) {
        let item = jb.popNext() orelse unreachable;
        defer item.deinit(alloc);
        expect_eq(item.seq, expected_seqs[idx]);
    }
    expect_eq(jb.popNext() == null, true);

    // Late packet behind expected_seq (2) should be dropped
    jb.push(0, 3000, "late-0")?;

    // Simulate packet loss: 2 and 3 are lost, 4 arrives
    jb.push(4, 7000, "p-4")?;
    expect_eq(jb.popNext() == null, true);
    expect_eq(jb.skipMissing(), true);

    let item4 = jb.popNext() orelse unreachable;
    defer item4.deinit(alloc);
    expect_eq(item4.seq, 4 as u16);
    expect_eq(std.mem.eql(u8, item4.payload, "p-4"), true);

    const st2 = jb.stats();
    expect_eq(st2.emitted, 5 as u64);
    expect_eq(st2.duplicates, 1 as u64);
    expect_eq(st2.late_drops, 1 as u64);
    expect_eq(st2.lost_packets, 2 as u64);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. Invalid version (V=1) -> InvalidVersion
    const bad_ver: [12]u8 = [0x40, 96, 0, 1, 0, 0, 0, 10, 0, 0, 0, 1];
    let got_ver_err = false;
    * = RtpPacket.decode(alloc, &bad_ver) catch |e| {
        if (e == RtpError.InvalidVersion) {
            got_ver_err = true;
        }
    };
    expect_eq(got_ver_err, true);

    // 2. Truncated header (< 12 bytes) -> TruncatedPacket
    const short_hdr: [8]u8 = [0x80, 96, 0, 1, 0, 0, 0, 10];
    let got_short_err = false;
    * = RtpPacket.decode(alloc, &short_hdr) catch |e| {
        if (e == RtpError.TruncatedPacket) {
            got_short_err = true;
        }
    };
    expect_eq(got_short_err, true);

    // 3. Truncated CSRC list (CC=2 requires 20 bytes, only 16 provided) -> TruncatedPacket
    const trunc_csrc: [16]u8 = [0x82, 96, 0, 1, 0, 0, 0, 10, 0, 0, 0, 1, 0, 0, 0, 2];
    let got_csrc_err = false;
    * = RtpPacket.decode(alloc, &trunc_csrc) catch |e| {
        if (e == RtpError.TruncatedPacket) {
            got_csrc_err = true;
        }
    };
    expect_eq(got_csrc_err, true);

    // 4. Invalid Padding = 0 -> InvalidPadding
    const pad_zero: [14]u8 = [0xA0, 96, 0, 1, 0, 0, 0, 10, 0, 0, 0, 1, 0xAA, 0x00];
    let got_pad0_err = false;
    * = RtpPacket.decode(alloc, &pad_zero) catch |e| {
        if (e == RtpError.InvalidPadding) {
            got_pad0_err = true;
        }
    };
    expect_eq(got_pad0_err, true);

    // 5. Invalid Padding > remaining (pad_len = 5, remaining = 2) -> InvalidPadding
    const pad_over: [14]u8 = [0xA0, 96, 0, 1, 0, 0, 0, 10, 0, 0, 0, 1, 0xAA, 0x05];
    let got_pad_over_err = false;
    * = RtpPacket.decode(alloc, &pad_over) catch |e| {
        if (e == RtpError.InvalidPadding) {
            got_pad_over_err = true;
        }
    };
    expect_eq(got_pad_over_err, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    const pkt = cap.net.RtpPacket {
        version: 2,
        padding: false,
        marker: true,
        payload_type: 98,
        sequence_number: 555,
        timestamp: 77777,
        ssrc: 0x01020304,
        payload: "cap-rtp-mount",
    };
    const wire = pkt.encode(alloc)?;
    defer alloc.free(wire);

    let decoded = cap.net.RtpPacket.decode(alloc, wire)?;
    defer decoded.deinit(alloc);
    expect_eq(decoded.sequence_number, 555 as u16);
    expect_eq(std.mem.eql(u8, decoded.payload, "cap-rtp-mount"), true);

    let jb = cap.net.JitterBuffer.init(alloc, 4);
    defer jb.deinit();
    jb.push(decoded.sequence_number, decoded.timestamp, decoded.payload)?;
    let out = jb.popNext() orelse unreachable;
    defer out.deinit(alloc);
    expect_eq(out.seq, 555 as u16);
    expect_eq(cap.net.seqDiff(556, out.seq), 1 as i32);
}

test("cap:rtp Test 1: RtpPacket basic, CSRC, Header Extension & Padding encode/decode roundtrip") {
    runTest1() catch unreachable;
}

test("cap:rtp Test 2: RtcpSenderReport & RtcpBye 32-bit word length encode/decode roundtrip") {
    runTest2() catch unreachable;
}

test("cap:rtp Test 3: JitterBuffer out-of-order, u16 wrap-around, dedup & skipMissing loss stats") {
    runTest3() catch unreachable;
}

test("cap:rtp Test 4: InvalidVersion, TruncatedPacket (header/CSRC) & InvalidPadding (0 / overflow) rejection") {
    runTest4() catch unreachable;
}

test("cap:rtp Test 5: Dynamic cap.rtp mount & std.testing.allocator zero memory leak") {
    runTest5() catch unreachable;
}
