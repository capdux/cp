// Origin: Pure-CP Stdlib Test Suite (lib/net/stream_mux.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Yamux-Style Binary Stream Multiplexer:
//   1. 12-byte big-endian frame header & payload encode/decode round-trip (Data, WindowUpdate, Ping, GoAway)
//   2. Client-odd / Server-even stream ID allocation, peer parity validation & max_concurrent_streams cap
//   3. Bidirectional send_window / recv_window deduction, over-window rejection & WindowUpdate replenishment
//   4. Full-duplex half-close (LocalHalfClosed / RemoteHalfClosed) & immediate RST stream teardown
//   5. Graceful GoAway drain (rejects new streams while letting in-flight streams finish cleanly)
//   6. Malformed/truncated frames, invalid flags/version/stream_id, u32 overflow & init boundary guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MUX_PROTO_VERSION,
    MUX_HEADER_LEN,
    MUX_GOAWAY_NORMAL,
    MUX_GOAWAY_PROTOCOL_ERROR,
    MuxFrameType,
    MuxFlags,
    MuxCodecError,
    MuxFrameHeader,
    MuxDecodedFrame,
    encodeMuxFrameHeader,
    decodeMuxFrameHeader,
    encodeMuxFrame,
    decodeMuxFrame,
    MuxStreamState,
    MuxStreamSlot,
    MuxSession,
} from "cap:net";

// ============================================================================
// Test 1: 12-Byte Big-Endian Frame Encode/Decode Round-Trip
// ============================================================================

fn testWireCodecRoundTrip()! {
    let wire: [64]u8 = [0; 64];
    const body: []const u8 = "agent-rpc-payload";

    // 1. Data frame with SYN flag, stream_id = 0x01020305
    const data_hdr = MuxFrameHeader{
        version: MUX_PROTO_VERSION,
        frame_type: MuxFrameType.Data,
        flags: MuxFlags.SYN,
        stream_id: 0x01020305,
        length: 0, // overwritten by encodeMuxFrame with body.len
    };
    const n_data = cap.net.encodeMuxFrame(data_hdr, body, wire[0..])?;
    expect_eq(n_data, MUX_HEADER_LEN + body.len);

    // Verify exact big-endian wire bytes on header [0..12)
    expect_eq(wire[0], 0);      // version
    expect_eq(wire[1], 0);      // Data
    expect_eq(wire[2], 0x00);   // flags hi
    expect_eq(wire[3], 0x01);   // flags lo (SYN)
    expect_eq(wire[4], 0x01);   // stream_id BE
    expect_eq(wire[5], 0x02);
    expect_eq(wire[6], 0x03);
    expect_eq(wire[7], 0x05);
    expect_eq(wire[11], body.len as u8);

    const dec_data = decodeMuxFrame(wire[0..n_data])?;
    expect_eq(dec_data.total_bytes, n_data);
    expect_eq(dec_data.header.frame_type == MuxFrameType.Data, true);
    expect_eq(dec_data.header.flags, MuxFlags.SYN);
    expect_eq(dec_data.header.stream_id, 0x01020305);
    expect_eq(dec_data.header.length, body.len as u32);
    expect_eq(dec_data.payload.len, body.len);
    expect_eq(dec_data.payload[0], 'a' as u8);
    expect_eq(dec_data.payload[body.len - 1], 'd' as u8);

    // 2. WindowUpdate control frame (length = credit delta = 65536, empty payload)
    const empty_body: []const u8 = body[0..0];
    const wu_hdr = MuxFrameHeader{
        version: MUX_PROTO_VERSION,
        frame_type: MuxFrameType.WindowUpdate,
        flags: MuxFlags.ACK,
        stream_id: 7,
        length: 65536,
    };
    const n_wu = encodeMuxFrame(wu_hdr, empty_body, wire[0..])?;
    expect_eq(n_wu, MUX_HEADER_LEN);
    const dec_wu = decodeMuxFrame(wire[0..n_wu])?;
    expect_eq(dec_wu.header.frame_type == MuxFrameType.WindowUpdate, true);
    expect_eq(dec_wu.header.stream_id, 7);
    expect_eq(dec_wu.header.length, 65536);
    expect_eq(dec_wu.payload.len, 0);

    // 3. Ping control frame (stream_id == 0, length = opaque nonce)
    const ping_hdr = MuxFrameHeader{
        version: MUX_PROTO_VERSION,
        frame_type: MuxFrameType.Ping,
        flags: MuxFlags.SYN,
        stream_id: 0,
        length: 0xDEADBEEF,
    };
    const n_ping = encodeMuxFrameHeader(ping_hdr, wire[0..])?;
    expect_eq(n_ping, MUX_HEADER_LEN);
    const dec_ping = decodeMuxFrameHeader(wire[0..n_ping])?;
    expect_eq(dec_ping.frame_type == MuxFrameType.Ping, true);
    expect_eq(dec_ping.length, 0xDEADBEEF);

    // 4. GoAway control frame (stream_id == 0, length = status code)
    const ga_hdr = MuxFrameHeader{
        version: MUX_PROTO_VERSION,
        frame_type: MuxFrameType.GoAway,
        flags: MuxFlags.NONE,
        stream_id: 0,
        length: MUX_GOAWAY_PROTOCOL_ERROR,
    };
    const n_ga = encodeMuxFrame(ga_hdr, empty_body, wire[0..])?;
    const dec_ga = decodeMuxFrame(wire[0..n_ga])?;
    expect_eq(dec_ga.header.frame_type == MuxFrameType.GoAway, true);
    expect_eq(dec_ga.header.length, MUX_GOAWAY_PROTOCOL_ERROR);
}

// ============================================================================
// Test 2: Odd/Even Stream ID Allocation, Parity & Concurrency Limit
// ============================================================================

fn testStreamIdParityAndConcurrencyLimit()! {
    let c_slots: [4]MuxStreamSlot = [MuxStreamSlot{}; 4];
    let s_slots: [4]MuxStreamSlot = [MuxStreamSlot{}; 4];

    // Max 2 concurrent streams on client, 3 on server
    let client = MuxSession.init(c_slots[0..], true, 2, 1024) ?? return error.InitFailed;
    let server = MuxSession.init(s_slots[0..], false, 3, 1024) ?? return error.InitFailed;

    // Client allocates odd IDs: 1, 3; 3rd open blocked by max_concurrent_streams = 2
    expect_eq(client.openStream() ?? 0, 1);
    expect_eq(client.openStream() ?? 0, 3);
    expect_eq(client.openStream() == null, true);

    // Server allocates even IDs: 2, 4
    expect_eq(server.openStream() ?? 0, 2);
    expect_eq(server.openStream() ?? 0, 4);

    // Server accepts odd peer stream (1), rejects even peer stream (6) and duplicate (1)
    expect_eq(server.acceptRemoteStream(1), true);
    expect_eq(server.acceptRemoteStream(6), false); // wrong parity
    expect_eq(server.acceptRemoteStream(1), false); // duplicate active
    expect_eq(server.acceptRemoteStream(3), false); // concurrency limit 3 reached

    // Closing client stream 1 frees a slot so client can open stream 5
    expect_eq(client.resetStream(1), true);
    expect_eq(client.openStream() ?? 0, 5);
}

// ============================================================================
// Test 3: Bidirectional Credit Window & WindowUpdate Replenishment
// ============================================================================

fn testBidirectionalFlowControl()! {
    let slots: [4]MuxStreamSlot = [MuxStreamSlot{}; 4];
    let sess = MuxSession.init(slots[0..], true, 4, 100) ?? return error.InitFailed;

    const sid = sess.openStream() ?? return error.OpenFailed;
    expect_eq(sess.canSend(sid, 100), true);
    expect_eq(sess.canSend(sid, 101), false);

    // Deduct 60 + 40 = 100 bytes -> send_window reaches 0
    expect_eq(sess.consumeSendWindow(sid, 60), true);
    expect_eq(sess.consumeSendWindow(sid, 40), true);
    expect_eq(sess.consumeSendWindow(sid, 1), false); // Window exhausted!

    // Peer sends WindowUpdate(+50) -> restores send credit
    expect_eq(sess.onWindowUpdate(sid, 50, MuxFlags.NONE), true);
    expect_eq(sess.canSend(sid, 50), true);
    expect_eq(sess.consumeSendWindow(sid, 50), true);
    expect_eq((sess.getStream(sid) ?? return error.MissingStream).bytes_sent, 150);

    // Receive side: deduct 80 bytes -> 20 left; 25 bytes rejected (peer violated flow control!)
    expect_eq(sess.onRecvData(sid, 80, MuxFlags.NONE), true);
    expect_eq(sess.onRecvData(sid, 25, MuxFlags.NONE), false);

    // Application grants +80 recv credit -> returns outbound WindowUpdate header
    const wu_hdr = sess.grantRecvWindow(sid, 80) ?? return error.GrantFailed;
    expect_eq(wu_hdr.frame_type == MuxFrameType.WindowUpdate, true);
    expect_eq(wu_hdr.stream_id, sid);
    expect_eq(wu_hdr.length, 80);

    // Now 25 bytes succeeds cleanly (recv_window = 100 -> 75)
    expect_eq(sess.onRecvData(sid, 25, MuxFlags.NONE), true);
    expect_eq((sess.getStream(sid) ?? return error.MissingStream).recv_window, 75);
    expect_eq(sess.stats().total_bytes_received, 105);
}

// ============================================================================
// Test 4: Full-Duplex Half-Close & RST Teardown
// ============================================================================

fn testHalfCloseAndReset()! {
    let slots: [4]MuxStreamSlot = [MuxStreamSlot{}; 4];
    let sess = MuxSession.init(slots[0..], true, 4, 512) ?? return error.InitFailed;

    // Stream 1: Local FIN -> LocalHalfClosed -> still receives data -> Remote FIN -> Closed
    const s1 = sess.openStream() ?? return error.OpenFailed;
    expect_eq(sess.closeLocal(s1), true);
    expect_eq((sess.getStream(s1) ?? return error.MissingStream).state == MuxStreamState.LocalHalfClosed, true);
    expect_eq(sess.consumeSendWindow(s1, 10), false); // Local send closed
    expect_eq(sess.onRecvData(s1, 32, MuxFlags.NONE), true); // Remote receive still open
    expect_eq(sess.onRecvData(s1, 16, MuxFlags.FIN), true);  // Remote FIN completes close
    expect_eq((sess.getStream(s1) ?? return error.MissingStream).state == MuxStreamState.Closed, true);
    expect_eq(sess.stats().active_streams, 0);

    // Stream 3: Remote FIN -> RemoteHalfClosed -> still sends data -> Local FIN -> Closed
    const s3 = sess.openStream() ?? return error.OpenFailed;
    expect_eq(sess.onRecvData(s3, 10, MuxFlags.FIN), true);
    expect_eq((sess.getStream(s3) ?? return error.MissingStream).state == MuxStreamState.RemoteHalfClosed, true);
    expect_eq(sess.onRecvData(s3, 1, MuxFlags.NONE), false); // Remote closed
    expect_eq(sess.consumeSendWindow(s3, 20), true);         // Local send still open
    expect_eq(sess.closeLocal(s3), true);
    expect_eq((sess.getStream(s3) ?? return error.MissingStream).state == MuxStreamState.Closed, true);
    expect_eq(sess.stats().active_streams, 0);

    // Stream 5: Incoming RST immediately tears down Open stream
    const s5 = sess.openStream() ?? return error.OpenFailed;
    expect_eq(sess.onRecvData(s5, 0, MuxFlags.RST), true);
    expect_eq((sess.getStream(s5) ?? return error.MissingStream).state == MuxStreamState.Closed, true);
    expect_eq(sess.stats().active_streams, 0);
}

// ============================================================================
// Test 5: Graceful GoAway Drain
// ============================================================================

fn testGracefulGoAwayDrain()! {
    let slots: [4]MuxStreamSlot = [MuxStreamSlot{}; 4];
    let sess = MuxSession.init(slots[0..], true, 4, 256) ?? return error.InitFailed;

    const in_flight = sess.openStream() ?? return error.OpenFailed;

    // Local GoAway forbids new local and remote streams, while letting in_flight finish
    const ga_hdr = sess.sendGoAway(MUX_GOAWAY_NORMAL);
    expect_eq(ga_hdr.frame_type == MuxFrameType.GoAway, true);
    expect_eq(ga_hdr.stream_id, 0);
    expect_eq(ga_hdr.length, MUX_GOAWAY_NORMAL);

    expect_eq(sess.openStream() == null, true);
    expect_eq(sess.acceptRemoteStream(2), false);

    // In-flight stream continues normal bidirectional I/O and closes cleanly
    expect_eq(sess.consumeSendWindow(in_flight, 64), true);
    expect_eq(sess.onRecvData(in_flight, 32, MuxFlags.FIN), true);
    expect_eq(sess.closeLocal(in_flight), true);
    expect_eq(sess.stats().active_streams, 0);
    expect_eq(sess.stats().local_goaway, true);

    // Remote GoAway also blocks new local opens
    let slots2: [4]MuxStreamSlot = [MuxStreamSlot{}; 4];
    let sess2 = MuxSession.init(slots2[0..], false, 4, 256) ?? return error.InitFailed;
    const s_open = sess2.openStream() ?? return error.OpenFailed;
    sess2.onGoAway(MUX_GOAWAY_NORMAL);
    expect_eq(sess2.openStream() == null, true);
    expect_eq(sess2.consumeSendWindow(s_open, 10), true);
    expect_eq(sess2.stats().remote_goaway, true);
}

// ============================================================================
// Test 6: Malformed, Truncated, Overflow & Boundary Guards
// ============================================================================

fn testMalformedAndBoundaryGuards()! {
    // 1. Truncated header (< 12 bytes)
    const short_hdr: [8]u8 = [0; 8];
    let err_trunc_hdr: bool = false;
    * = decodeMuxFrameHeader(short_hdr[0..]) catch |e| {
        err_trunc_hdr = (e == MuxCodecError.TruncatedHeader);
        MuxFrameHeader{}
    };
    expect_eq(err_trunc_hdr, true);

    // 2. Unsupported protocol version
    let bad_ver: [12]u8 = [0; 12];
    bad_ver[0] = 1;
    bad_ver[7] = 1;
    let err_ver: bool = false;
    * = decodeMuxFrameHeader(bad_ver[0..]) catch |e| {
        err_ver = (e == MuxCodecError.UnsupportedVersion);
        MuxFrameHeader{}
    };
    expect_eq(err_ver, true);

    // 3. Unknown frame type (4)
    let bad_ft: [12]u8 = [0; 12];
    bad_ft[1] = 4;
    let err_ft: bool = false;
    * = decodeMuxFrameHeader(bad_ft[0..]) catch |e| {
        err_ft = (e == MuxCodecError.UnknownFrameType);
        MuxFrameHeader{}
    };
    expect_eq(err_ft, true);

    // 4. Invalid flag bits (0x0010)
    let bad_flags: [12]u8 = [0; 12];
    bad_flags[3] = 0x10;
    bad_flags[7] = 1;
    let err_flags: bool = false;
    * = decodeMuxFrameHeader(bad_flags[0..]) catch |e| {
        err_flags = (e == MuxCodecError.InvalidFlags);
        MuxFrameHeader{}
    };
    expect_eq(err_flags, true);

    // 5. Invalid stream_id: Data with stream_id == 0, Ping with stream_id != 0
    let out12: [12]u8 = [0; 12];
    let err_sid1: bool = false;
    * = encodeMuxFrameHeader(MuxFrameHeader{ frame_type: MuxFrameType.Data, stream_id: 0 }, out12[0..]) catch |e| {
        err_sid1 = (e == MuxCodecError.InvalidStreamId);
        0
    };
    expect_eq(err_sid1, true);

    let err_sid2: bool = false;
    * = encodeMuxFrameHeader(MuxFrameHeader{ frame_type: MuxFrameType.Ping, stream_id: 1 }, out12[0..]) catch |e| {
        err_sid2 = (e == MuxCodecError.InvalidStreamId);
        0
    };
    expect_eq(err_sid2, true);

    // 6. Control frame with non-empty payload rejected
    const dummy: []const u8 = "x";
    let err_ctrl_pay: bool = false;
    * = encodeMuxFrame(MuxFrameHeader{ frame_type: MuxFrameType.Ping, stream_id: 0, length: 42 }, dummy, out12[0..]) catch |e| {
        err_ctrl_pay = (e == MuxCodecError.PayloadNotAllowed);
        0
    };
    expect_eq(err_ctrl_pay, true);

    // 7. Truncated Data payload rejected
    let trunc_data: [14]u8 = [0; 14];
    trunc_data[7] = 1;  // stream_id = 1
    trunc_data[11] = 8; // claims 8 bytes payload, only 2 bytes follow
    let err_trunc_pay: bool = false;
    * = decodeMuxFrame(trunc_data[0..]) catch |e| {
        err_trunc_pay = (e == MuxCodecError.TruncatedPayload);
        MuxDecodedFrame{ header: MuxFrameHeader{}, payload: trunc_data[0..0], total_bytes: 0 }
    };
    expect_eq(err_trunc_pay, true);

    // 8. BufferTooSmall on encode
    let tiny: [6]u8 = [0; 6];
    let err_small: bool = false;
    * = encodeMuxFrameHeader(MuxFrameHeader{ frame_type: MuxFrameType.Ping, stream_id: 0 }, tiny[0..]) catch |e| {
        err_small = (e == MuxCodecError.BufferTooSmall);
        0
    };
    expect_eq(err_small, true);

    // 9. Invalid MuxSession.init & u32 window overflow guard
    let slots: [2]MuxStreamSlot = [MuxStreamSlot{}; 2];
    let empty_slots: []MuxStreamSlot = slots[0..0];
    expect_eq(MuxSession.init(empty_slots, true, 1, 1024) == null, true);
    expect_eq(MuxSession.init(slots[0..], true, 0, 1024) == null, true);
    expect_eq(MuxSession.init(slots[0..], true, 3, 1024) == null, true); // > buf.len
    expect_eq(MuxSession.init(slots[0..], true, 2, 0) == null, true);    // zero window

    let sess = MuxSession.init(slots[0..], true, 2, 100) ?? return error.InitFailed;
    const sid = sess.openStream() ?? return error.OpenFailed;
    expect_eq(sess.onWindowUpdate(sid, std.math.maxInt(u32), MuxFlags.NONE), false); // overflow rejected
    expect_eq(sess.grantRecvWindow(sid, 0) == null, true);
    expect_eq(sess.grantRecvWindow(sid, std.math.maxInt(u32)) == null, true);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("net/stream_mux: 12-byte big-endian frame encode/decode round-trip", testWireCodecRoundTrip);
test("net/stream_mux: odd/even stream ID parity & concurrency limit", testStreamIdParityAndConcurrencyLimit);
test("net/stream_mux: bidirectional send/recv flow control & WindowUpdate", testBidirectionalFlowControl);
test("net/stream_mux: full-duplex half-close & RST stream teardown", testHalfCloseAndReset);
test("net/stream_mux: graceful GoAway drain", testGracefulGoAwayDrain);
test("net/stream_mux: malformed, truncated, overflow & init boundary guards", testMalformedAndBoundaryGuards);
