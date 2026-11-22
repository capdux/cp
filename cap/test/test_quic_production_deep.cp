// Source: cp/cap/test/test_quic_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:quic & cap.quic

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_VARINT,
    QUIC_V1,
    QuicError,
    LongPacketType,
    LongHeader,
    ShortHeader,
    AckRange,
    AckFrame,
    CryptoFrame,
    StreamFrame,
    ResetStreamFrame,
    ConnectionCloseFrame,
    QuicFrame,
    encodeVarint,
    decodeVarint,
    encodeLongHeader,
    decodeLongHeader,
    encodeShortHeader,
    decodeShortHeader,
    encodeQuicFrame,
    decodeFrames,
    freeFrames,
} from "cap:net";

fn runTestVarint(): !void {
    const alloc = std.testing.allocator;

    // RFC 9000 Appendix A.1 Sample Variable-Length Integers:
    // 1-byte: 37 (0x25)
    const v1 = encodeVarint(alloc, 37)?;
    defer alloc.free(v1);
    expect_eq(v1.len, 1 as usize);
    expect_eq(v1[0], 0x25 as u8);
    const d1 = decodeVarint(v1)?;
    expect_eq(d1.value, 37 as u64);
    expect_eq(d1.len, 1 as usize);

    // 2-byte: 15293 (0x7bbd)
    const v2 = encodeVarint(alloc, 15293)?;
    defer alloc.free(v2);
    expect_eq(v2.len, 2 as usize);
    expect_eq(v2[0], 0x7b as u8);
    expect_eq(v2[1], 0xbd as u8);
    const d2 = decodeVarint(v2)?;
    expect_eq(d2.value, 15293 as u64);
    expect_eq(d2.len, 2 as usize);

    // 4-byte: 494878333 (0x9d7f3e7d)
    const v4 = encodeVarint(alloc, 494878333)?;
    defer alloc.free(v4);
    expect_eq(v4.len, 4 as usize);
    expect_eq(v4[0], 0x9d as u8);
    expect_eq(v4[1], 0x7f as u8);
    expect_eq(v4[2], 0x3e as u8);
    expect_eq(v4[3], 0x7d as u8);
    const d4 = decodeVarint(v4)?;
    expect_eq(d4.value, 494878333 as u64);
    expect_eq(d4.len, 4 as usize);

    // 8-byte: 151288809941952652 (0xc2197c5eff14e88c)
    const v8 = encodeVarint(alloc, 151288809941952652)?;
    defer alloc.free(v8);
    expect_eq(v8.len, 8 as usize);
    expect_eq(v8[0], 0xc2 as u8);
    expect_eq(v8[7], 0x8c as u8);
    const d8 = decodeVarint(v8)?;
    expect_eq(d8.value, 151288809941952652 as u64);
    expect_eq(d8.len, 8 as usize);

    // Boundary MAX_VARINT (2^62 - 1) and overflow (> 2^62 - 1)
    const vmax = encodeVarint(alloc, MAX_VARINT)?;
    defer alloc.free(vmax);
    expect_eq(vmax.len, 8 as usize);
    expect_eq((decodeVarint(vmax)?).value, MAX_VARINT);

    let overflow_hit = false;
    * = encodeVarint(alloc, MAX_VARINT + 1) catch |e| {
        if (e == QuicError.VarintOutOfRange) overflow_hit = true;
    };
    expect_eq(overflow_hit, true);

    // Truncated varint -> UnexpectedEof
    let eof_hit = false;
    const trunc_buf: [3]u8 = [0xc2, 0x19, 0x7c];
    * = decodeVarint(&trunc_buf) catch |e| {
        if (e == QuicError.UnexpectedEof) eof_hit = true;
    };
    expect_eq(eof_hit, true);
}

fn runTestHeaders(): !void {
    const alloc = std.testing.allocator;

    // 1. Initial LongHeader with token
    const dcid: [8]u8 = [0x83, 0x94, 0xc8, 0xf0, 0x3e, 0x51, 0x57, 0x08];
    const scid: [4]u8 = [0x01, 0x02, 0x03, 0x04];
    const tok = "retry-token-v1";

    const init_hdr = LongHeader {
        packet_type: .Initial,
        version: QUIC_V1,
        dcid: &dcid,
        scid: &scid,
        token: tok,
        length: 1200,
        packet_number_len: 2,
    };
    const init_wire = encodeLongHeader(alloc, init_hdr)?;
    defer alloc.free(init_wire);
    const init_dec = decodeLongHeader(init_wire)?;
    expect_eq(init_dec.packet_type, LongPacketType.Initial);
    expect_eq(init_dec.version, QUIC_V1);
    expect_eq(std.mem.eql(u8, init_dec.dcid, &dcid), true);
    expect_eq(std.mem.eql(u8, init_dec.scid, &scid), true);
    expect_eq(std.mem.eql(u8, init_dec.token orelse unreachable, tok), true);
    expect_eq(init_dec.length, 1200 as u64);
    expect_eq(init_dec.packet_number_len, 2 as u8);

    // 2. Handshake LongHeader
    const hs_hdr = LongHeader {
        packet_type: .Handshake,
        version: QUIC_V1,
        dcid: &dcid,
        scid: &scid,
        length: 512,
        packet_number_len: 4,
    };
    const hs_wire = encodeLongHeader(alloc, hs_hdr)?;
    defer alloc.free(hs_wire);
    const hs_dec = decodeLongHeader(hs_wire)?;
    expect_eq(hs_dec.packet_type, LongPacketType.Handshake);
    expect_eq(hs_dec.token == null, true);
    expect_eq(hs_dec.length, 512 as u64);
    expect_eq(hs_dec.packet_number_len, 4 as u8);

    // 3. ZeroRtt LongHeader
    const zr_hdr = LongHeader {
        packet_type: .ZeroRtt,
        version: QUIC_V1,
        dcid: &dcid,
        scid: &scid,
        length: 64,
        packet_number_len: 1,
    };
    const zr_wire = encodeLongHeader(alloc, zr_hdr)?;
    defer alloc.free(zr_wire);
    const zr_dec = decodeLongHeader(zr_wire)?;
    expect_eq(zr_dec.packet_type, LongPacketType.ZeroRtt);
    expect_eq(zr_dec.length, 64 as u64);

    // 4. ShortHeader (1-RTT)
    const sh_hdr = ShortHeader {
        spin_bit: true,
        key_phase: true,
        dcid: &dcid,
        packet_number_len: 3,
    };
    const sh_wire = encodeShortHeader(alloc, sh_hdr)?;
    defer alloc.free(sh_wire);
    const sh_dec = decodeShortHeader(sh_wire, dcid.len)?;
    expect_eq(sh_dec.spin_bit, true);
    expect_eq(sh_dec.key_phase, true);
    expect_eq(std.mem.eql(u8, sh_dec.dcid, &dcid), true);
    expect_eq(sh_dec.packet_number_len, 3 as u8);
}

fn runTestFrames(): !void {
    const alloc = std.testing.allocator;

    let ranges: [2]AckRange = [
        AckRange { gap: 1, ack_range_len: 3 },
        AckRange { gap: 0, ack_range_len: 2 },
    ];

    const f_pad = encodeQuicFrame(alloc, QuicFrame { padding: 3 })?;
    defer alloc.free(f_pad);
    const f_ping = encodeQuicFrame(alloc, QuicFrame.ping)?;
    defer alloc.free(f_ping);
    const f_ack = encodeQuicFrame(alloc, QuicFrame {
        ack: AckFrame {
            largest_acked: 100,
            ack_delay: 25,
            first_ack_range: 5,
            ranges: &ranges,
        },
    })?;
    defer alloc.free(f_ack);
    const f_crypto = encodeQuicFrame(alloc, QuicFrame {
        crypto: CryptoFrame {
            offset: 0,
            data: "ClientHelloBytes",
        },
    })?;
    defer alloc.free(f_crypto);
    const f_stream = encodeQuicFrame(alloc, QuicFrame {
        stream: StreamFrame {
            stream_id: 4,
            offset: 1024,
            fin: true,
            data: "GET / HTTP/3",
        },
    })?;
    defer alloc.free(f_stream);
    const f_max = encodeQuicFrame(alloc, QuicFrame { max_data: 65536 })?;
    defer alloc.free(f_max);
    const f_close = encodeQuicFrame(alloc, QuicFrame {
        connection_close: ConnectionCloseFrame {
            is_app_error: false,
            error_code: 0x0a,
            frame_type: 0x08,
            reason_phrase: "flow control limit",
        },
    })?;
    defer alloc.free(f_close);

    let combined: std.ArrayList(u8) = .empty;
    defer combined.deinit(alloc);
    try combined.appendSlice(alloc, f_pad);
    try combined.appendSlice(alloc, f_ping);
    try combined.appendSlice(alloc, f_ack);
    try combined.appendSlice(alloc, f_crypto);
    try combined.appendSlice(alloc, f_stream);
    try combined.appendSlice(alloc, f_max);
    try combined.appendSlice(alloc, f_close);

    const decoded = decodeFrames(alloc, combined.items)?;
    defer freeFrames(alloc, decoded);

    expect_eq(decoded.len, 7 as usize);

    switch (decoded[0]) {
        .padding => |n| expect_eq(n, 3 as usize),
        else => unreachable,
    }
    switch (decoded[1]) {
        .ping => {},
        else => unreachable,
    }
    switch (decoded[2]) {
        .ack => |ack| {
            expect_eq(ack.largest_acked, 100 as u64);
            expect_eq(ack.ack_delay, 25 as u64);
            expect_eq(ack.first_ack_range, 5 as u64);
            expect_eq(ack.ranges.len, 2 as usize);
            expect_eq(ack.ranges[0].gap, 1 as u64);
            expect_eq(ack.ranges[0].ack_range_len, 3 as u64);
            expect_eq(ack.ranges[1].gap, 0 as u64);
            expect_eq(ack.ranges[1].ack_range_len, 2 as u64);
        },
        else => unreachable,
    }
    switch (decoded[3]) {
        .crypto => |cry| {
            expect_eq(cry.offset, 0 as u64);
            expect_eq(std.mem.eql(u8, cry.data, "ClientHelloBytes"), true);
        },
        else => unreachable,
    }
    switch (decoded[4]) {
        .stream => |st| {
            expect_eq(st.stream_id, 4 as u64);
            expect_eq(st.offset, 1024 as u64);
            expect_eq(st.fin, true);
            expect_eq(std.mem.eql(u8, st.data, "GET / HTTP/3"), true);
        },
        else => unreachable,
    }
    switch (decoded[5]) {
        .max_data => |md| expect_eq(md, 65536 as u64),
        else => unreachable,
    }
    switch (decoded[6]) {
        .connection_close => |cc| {
            expect_eq(cc.is_app_error, false);
            expect_eq(cc.error_code, 0x0a as u64);
            expect_eq(cc.frame_type, 0x08 as u64);
            expect_eq(std.mem.eql(u8, cc.reason_phrase, "flow control limit"), true);
        },
        else => unreachable,
    }
}

fn runTestValidationErrors(): !void {
    const alloc = std.testing.allocator;

    // 1. DCID > 20 bytes -> InvalidConnectionId
    const long_cid = alloc.alloc(u8, 21) catch unreachable;
    defer alloc.free(long_cid);
    @memset(long_cid, 0xaa);
    let cid_err = false;
    * = encodeLongHeader(alloc, LongHeader {
        packet_type: .Initial,
        version: QUIC_V1,
        dcid: long_cid,
        scid: "",
        length: 10,
    }) catch |e| {
        if (e == QuicError.InvalidConnectionId) cid_err = true;
    };
    expect_eq(cid_err, true);

    // 2. Missing fixed bit (0x40) -> InvalidHeader
    const bad_hdr_bytes: [8]u8 = [0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00];
    let hdr_err = false;
    * = decodeLongHeader(&bad_hdr_bytes) catch |e| {
        if (e == QuicError.InvalidHeader) hdr_err = true;
    };
    expect_eq(hdr_err, true);

    // 3. Truncated STREAM frame -> TruncatedFrame
    const trunc_stream: [5]u8 = [0x0a, 0x04, 0x10, 0x61, 0x62]; // stream_id=4, len=16, only 2 bytes
    let trunc_err = false;
    * = decodeFrames(alloc, &trunc_stream) catch |e| {
        if (e == QuicError.TruncatedFrame) trunc_err = true;
    };
    expect_eq(trunc_err, true);

    // 4. Truncated ACK after allocating ranges -> zero leak verified by testing.allocator
    const trunc_ack: [7]u8 = [0x02, 0x0a, 0x01, 0x02, 0x01, 0x00, 0x01]; // 2 ranges declared, 2nd missing
    let ack_trunc_err = false;
    * = decodeFrames(alloc, &trunc_ack) catch |e| {
        if (e == QuicError.TruncatedFrame) ack_trunc_err = true;
    };
    expect_eq(ack_trunc_err, true);
}

fn runTestDynamicCapMount(): !void {
    const alloc = std.testing.allocator;

    const encoded = cap.net.encodeVarint(alloc, 16383)?;
    defer alloc.free(encoded);
    expect_eq(encoded.len, 2 as usize);
    const decoded = cap.net.decodeVarint(encoded)?;
    expect_eq(decoded.value, 16383 as u64);

    const frame_wire = cap.net.encodeQuicFrame(alloc, cap.net.QuicFrame {
        reset_stream: cap.net.ResetStreamFrame {
            stream_id: 8,
            error_code: 42,
            final_size: 2048,
        },
    })?;
    defer alloc.free(frame_wire);

    const frames = cap.net.decodeFrames(alloc, frame_wire)?;
    defer cap.net.freeFrames(alloc, frames);
    expect_eq(frames.len, 1 as usize);
    switch (frames[0]) {
        .reset_stream => |rst| {
            expect_eq(rst.stream_id, 8 as u64);
            expect_eq(rst.error_code, 42 as u64);
            expect_eq(rst.final_size, 2048 as u64);
        },
        else => unreachable,
    }
}

test("1. RFC 9000 A.1 varint 1B/2B/4B/8B round-trip & overflow/EOF guards") {
    runTestVarint() catch |e| {
        std.debug.print("varint test failed: {}\n", .{e});
        unreachable;
    };
}

test("2. QUIC LongHeader (Initial/Handshake/ZeroRtt) & ShortHeader codec") {
    runTestHeaders() catch |e| {
        std.debug.print("headers test failed: {}\n", .{e});
        unreachable;
    };
}

test("3. QUIC compound frames (PADDING/PING/ACK/CRYPTO/STREAM/MAX_DATA/CONNECTION_CLOSE)") {
    runTestFrames() catch |e| {
        std.debug.print("frames test failed: {}\n", .{e});
        unreachable;
    };
}

test("4. QUIC header/CID/truncation validation & error-path zero-leak") {
    runTestValidationErrors() catch |e| {
        std.debug.print("validation test failed: {}\n", .{e});
        unreachable;
    };
}

test("5. cap.quic dynamic lazy-mount & std.testing.allocator zero-leak") {
    runTestDynamicCapMount() catch |e| {
        std.debug.print("cap.quic test failed: {}\n", .{e});
        unreachable;
    };
}
