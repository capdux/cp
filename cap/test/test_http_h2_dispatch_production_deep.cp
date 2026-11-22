// Production Deep Tests for cap:http HTTP/2 Frame Dispatcher (lib/http/h2/dispatch.cp)
import { test, expect_eq } from "cap:test";
import {
    H2_DISPATCH_HEADER_SIZE,
    H2_DISPATCH_MAX_WINDOW_SIZE,
    H2DispatchError,
    H2DispatchEventKind,
    H2ParsedFrameHeader,
    stripH2Padding,
    H2DispatchEvent,
    H2FrameDispatcher,
} from "cap:http";

fn writeFrameHeader(out: []u8, length: u32, frame_type: u8, flags: u8, stream_id: u32) {
    out[0] = @intCast((length >> 16) & 0xff);
    out[1] = @intCast((length >> 8) & 0xff);
    out[2] = @intCast(length & 0xff);
    out[3] = frame_type;
    out[4] = flags;
    out[5] = @intCast((stream_id >> 24) & 0xff);
    out[6] = @intCast((stream_id >> 16) & 0xff);
    out[7] = @intCast((stream_id >> 8) & 0xff);
    out[8] = @intCast(stream_id & 0xff);
}

test("http h2 dispatch: enforces initial SETTINGS and validates SETTINGS bounds", fn() {
    let disp = H2FrameDispatcher.init();

    // Non-SETTINGS first frame (PING) must be rejected with ProtocolError
    const ping_hdr = H2ParsedFrameHeader{ length: 8, frame_type: 0x6, flags: 0, stream_id: 0 };
    const ping_body: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    let first_err = false;
    * = disp.dispatchOne(ping_hdr, ping_body[0..]) catch |e| {
        first_err = (e == H2DispatchError.ProtocolError);
        H2DispatchEvent{ kind: .unknown_frame }
    };
    expect_eq(first_err, true);

    // Valid SETTINGS frame with 2 parameters: MAX_CONCURRENT_STREAMS=256 (0x3), MAX_FRAME_SIZE=32768 (0x5)
    const set_payload: [12]u8 = [
        0x00, 0x03, 0x00, 0x00, 0x01, 0x00,
        0x00, 0x05, 0x00, 0x00, 0x80, 0x00,
    ];
    const set_hdr = H2ParsedFrameHeader{ length: 12, frame_type: 0x4, flags: 0, stream_id: 0 };
    const ev = disp.dispatchOne(set_hdr, set_payload[0..]) catch unreachable;
    expect_eq(ev.kind == H2DispatchEventKind.settings_applied, true);
    expect_eq(ev.needs_settings_ack, true);
    expect_eq(disp.settings_received, true);
    expect_eq(disp.remote_max_concurrent_streams, @as(u32, 256));
    expect_eq(disp.remote_max_frame_size, @as(u32, 32768));

    // SETTINGS ACK with non-zero length -> FrameSizeError
    const bad_ack = H2ParsedFrameHeader{ length: 6, frame_type: 0x4, flags: 0x1, stream_id: 0 };
    let ack_err = false;
    * = disp.dispatchOne(bad_ack, set_payload[0..6]) catch |e| {
        ack_err = (e == H2DispatchError.FrameSizeError);
        H2DispatchEvent{ kind: .unknown_frame }
    };
    expect_eq(ack_err, true);
});

test("http h2 dispatch: WINDOW_UPDATE zero increment and 2^31-1 overflow protection", fn() {
    let disp = H2FrameDispatcher.init();
    disp.settings_received = true;

    // Zero increment -> ProtocolError
    const zero_wu: [4]u8 = [0, 0, 0, 0];
    const wu_hdr = H2ParsedFrameHeader{ length: 4, frame_type: 0x8, flags: 0, stream_id: 0 };
    let zero_err = false;
    * = disp.dispatchOne(wu_hdr, zero_wu[0..]) catch |e| {
        zero_err = (e == H2DispatchError.ProtocolError);
        H2DispatchEvent{ kind: .unknown_frame }
    };
    expect_eq(zero_err, true);

    // Valid increment +1024 on connection window
    const ok_wu: [4]u8 = [0x00, 0x00, 0x04, 0x00];
    const ev = disp.dispatchOne(wu_hdr, ok_wu[0..]) catch unreachable;
    expect_eq(ev.kind == H2DispatchEventKind.window_update_conn, true);
    expect_eq(ev.window_increment, @as(u32, 1024));
    expect_eq(disp.conn_send_window, @as(i64, 65535 + 1024));

    // Overflowing MAX_WINDOW_SIZE (0x7fffffff) -> FlowControlError
    const huge_wu: [4]u8 = [0x7f, 0xff, 0xff, 0xff];
    let flow_err = false;
    * = disp.dispatchOne(wu_hdr, huge_wu[0..]) catch |e| {
        flow_err = (e == H2DispatchError.FlowControlError);
        H2DispatchEvent{ kind: .unknown_frame }
    };
    expect_eq(flow_err, true);
    expect_eq(H2_DISPATCH_MAX_WINDOW_SIZE, @as(u32, 0x7fffffff));
});

test("http h2 dispatch: PING, GOAWAY, and RST_STREAM decoding & validation", fn() {
    let disp = H2FrameDispatcher.init();
    disp.settings_received = true;

    // PING request -> needs_ping_ack == true, echoes 8 bytes
    const ping_hdr = H2ParsedFrameHeader{ length: 8, frame_type: 0x6, flags: 0, stream_id: 0 };
    const ping_body: [8]u8 = [0xaa, 0xbb, 0xcc, 0xdd, 1, 2, 3, 4];
    const pev = disp.dispatchOne(ping_hdr, ping_body[0..]) catch unreachable;
    expect_eq(pev.kind == H2DispatchEventKind.ping_received, true);
    expect_eq(pev.needs_ping_ack, true);
    expect_eq(pev.ping_data[0], @as(u8, 0xaa));
    expect_eq(pev.ping_data[7], @as(u8, 4));

    // GOAWAY with reserved bit set on last_stream_id (0x80000005 -> 5), error_code=2, debug="bye"
    const goaway_body: [11]u8 = [0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 'b', 'y', 'e'];
    const goaway_hdr = H2ParsedFrameHeader{ length: 11, frame_type: 0x7, flags: 0, stream_id: 0 };
    const gev = disp.dispatchOne(goaway_hdr, goaway_body[0..]) catch unreachable;
    expect_eq(gev.kind == H2DispatchEventKind.goaway_received, true);
    expect_eq(gev.last_stream_id, @as(u32, 5));
    expect_eq(gev.error_code, @as(u32, 2));
    expect_eq(gev.payload_slice, "bye");
    expect_eq(disp.goaway_received, true);

    // RST_STREAM on stream 3, error_code=8 (CANCEL)
    const rst_body: [4]u8 = [0, 0, 0, 8];
    const rst_hdr = H2ParsedFrameHeader{ length: 4, frame_type: 0x3, flags: 0, stream_id: 3 };
    const rev = disp.dispatchOne(rst_hdr, rst_body[0..]) catch unreachable;
    expect_eq(rev.kind == H2DispatchEventKind.rst_stream_received, true);
    expect_eq(rev.stream_id, @as(u32, 3));
    expect_eq(rev.error_code, @as(u32, 8));
});

test("http h2 dispatch: HEADERS PADDED+PRIORITY stripping and CONTINUATION sequence lock", fn() {
    let disp = H2FrameDispatcher.init();
    disp.settings_received = true;

    // HEADERS on stream 1: PADDED (0x8) | PRIORITY (0x20), NO END_HEADERS
    // pad_len=2, priority=5 bytes, fragment="hdr1", padding=2 bytes -> total 1+5+4+2 = 12
    const h_payload: [12]u8 = [2, 0, 0, 0, 0, 16, 'h', 'd', 'r', '1', 0, 0];
    const h_hdr = H2ParsedFrameHeader{ length: 12, frame_type: 0x1, flags: 0x28, stream_id: 1 };
    const hev = disp.dispatchOne(h_hdr, h_payload[0..]) catch unreachable;
    expect_eq(hev.kind == H2DispatchEventKind.headers_fragment, true);
    expect_eq(hev.payload_slice, "hdr1");
    expect_eq(disp.expecting_continuation, @as(u32, 1));

    // Interleaved PING while expecting CONTINUATION -> ProtocolError
    const ping_hdr = H2ParsedFrameHeader{ length: 8, frame_type: 0x6, flags: 0, stream_id: 0 };
    const ping_body: [8]u8 = [0; 8];
    let seq_err = false;
    * = disp.dispatchOne(ping_hdr, ping_body[0..]) catch |e| {
        seq_err = (e == H2DispatchError.ProtocolError);
        H2DispatchEvent{ kind: .unknown_frame }
    };
    expect_eq(seq_err, true);

    // Matching CONTINUATION with END_HEADERS (0x4) unlocks continuation state
    const c_payload = "hdr2";
    const c_hdr = H2ParsedFrameHeader{ length: 4, frame_type: 0x9, flags: 0x4, stream_id: 1 };
    const cev = disp.dispatchOne(c_hdr, c_payload) catch unreachable;
    expect_eq(cev.kind == H2DispatchEventKind.continuation_fragment, true);
    expect_eq(cev.payload_slice, "hdr2");
    expect_eq(disp.expecting_continuation, @as(u32, 0));
});

test("http h2 dispatch: DATA padding stripping and invalid padding rejection", fn() {
    let disp = H2FrameDispatcher.init();
    disp.settings_received = true;

    // PADDED DATA: pad_len=3, body="hello", 3 pad bytes -> total 9
    const padded: [9]u8 = [3, 'h', 'e', 'l', 'l', 'o', 0, 0, 0];
    expect_eq(stripH2Padding(padded[0..]) ?? "", "hello");

    const d_hdr = H2ParsedFrameHeader{ length: 9, frame_type: 0x0, flags: 0x8, stream_id: 1 };
    const dev = disp.dispatchOne(d_hdr, padded[0..]) catch unreachable;
    expect_eq(dev.kind == H2DispatchEventKind.data_received, true);
    expect_eq(dev.payload_slice, "hello");

    // Malformed padding: pad_len=10 > remaining 2 bytes -> ProtocolError
    const bad_pad: [3]u8 = [10, 'a', 'b'];
    expect_eq(stripH2Padding(bad_pad[0..]) == null, true);
    const bad_hdr = H2ParsedFrameHeader{ length: 3, frame_type: 0x0, flags: 0x8, stream_id: 1 };
    let pad_err = false;
    * = disp.dispatchOne(bad_hdr, bad_pad[0..]) catch |e| {
        pad_err = (e == H2DispatchError.ProtocolError);
        H2DispatchEvent{ kind: .unknown_frame }
    };
    expect_eq(pad_err, true);
});

test("http h2 dispatch: streaming parseNext handles partial frames and multi-frame buffers", fn() {
    let disp = H2FrameDispatcher.init();
    // Build 2 concatenated frames: empty SETTINGS (9 bytes) + PING (9+8 = 17 bytes) -> 26 bytes
    let wire: [26]u8 = [0; 26];
    writeFrameHeader(wire[0..9], 0, 0x4, 0, 0);
    writeFrameHeader(wire[9..18], 8, 0x6, 0, 0);
    wire[18] = 0xde;
    wire[25] = 0xad;

    // Partial header (5 bytes) -> null, consumed=0
    let consumed: usize = 99;
    const partial_hdr = disp.parseNext(wire[0..5], &consumed) catch unreachable;
    expect_eq(partial_hdr == null, true);
    expect_eq(consumed, @as(usize, 0));

    // Frame 1 (SETTINGS)
    const ev1 = (disp.parseNext(wire[0..], &consumed) catch unreachable) ?? unreachable;
    expect_eq(ev1.kind == H2DispatchEventKind.settings_applied, true);
    expect_eq(consumed, H2_DISPATCH_HEADER_SIZE);

    // Partial Frame 2 (header + 4 of 8 payload bytes) -> null
    let consumed2: usize = 99;
    const partial_body = disp.parseNext(wire[9..22], &consumed2) catch unreachable;
    expect_eq(partial_body == null, true);
    expect_eq(consumed2, @as(usize, 0));

    // Full Frame 2 (PING)
    const ev2 = (disp.parseNext(wire[9..], &consumed2) catch unreachable) ?? unreachable;
    expect_eq(ev2.kind == H2DispatchEventKind.ping_received, true);
    expect_eq(consumed2, @as(usize, 17));
    expect_eq(ev2.ping_data[0], @as(u8, 0xde));
    expect_eq(ev2.ping_data[7], @as(u8, 0xad));
});
