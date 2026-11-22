// Source: cp/cap/test/test_h2stream_production_deep.cp (Production deep tests for cap:h2stream)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    H2_MAX_WINDOW_SIZE,
    H2_DEFAULT_INITIAL_WINDOW,
    H2_RST_STREAM_FRAME_LEN,
    H2_WINDOW_UPDATE_FRAME_LEN,
    StreamState,
    H2ErrorCode,
    errorCodeToU32,
    encodeRstStreamFrame,
    encodeWindowUpdateFrame,
    H2Stream,
} from "cap:h2stream";

fn readU32Be(b: []const u8, off: usize): u32 {
    return ((b[off] as u32) << 24) | ((b[off + 1] as u32) << 16) | ((b[off + 2] as u32) << 8) | (b[off + 3] as u32);
}

test("1. RFC 9113 stream lifecycle: Open -> HalfClosedLocal / HalfClosedRemote -> Closed & 100 Continue", () => {
    let st = H2Stream.init(3, H2_DEFAULT_INITIAL_WINDOW, H2_DEFAULT_INITIAL_WINDOW) ?? unreachable;
    expect_eq(st.state == StreamState.Open, true);
    expect_eq(st.localClosed(), false);
    expect_eq(st.remoteClosed(), false);
    expect_eq(st.canSendData(), true);

    // Expect: 100-continue blocks outbound DATA until 100 response arrives
    st.awaiting_continue = true;
    expect_eq(st.canSendData(), false);
    expect_eq(st.markHeadersReceived(100, false), true);
    expect_eq(st.awaiting_continue, false);
    expect_eq(st.headers_ready, false);
    expect_eq(st.canSendData(), true);

    // Local finishes sending -> HalfClosedLocal
    st.sentEndStream();
    expect_eq(st.state == StreamState.HalfClosedLocal, true);
    expect_eq(st.localClosed(), true);
    expect_eq(st.remoteClosed(), false);
    expect_eq(st.canSendData(), false);

    // Remote sends 200 OK with END_STREAM -> Closed
    expect_eq(st.markHeadersReceived(200, true), true);
    expect_eq(st.status_code, 200);
    expect_eq(st.headers_ready, true);
    expect_eq(st.isClosed(), true);
    expect_eq(st.markHeadersReceived(200, true), false);
});

test("2. Symmetric reverse closure: Open -> HalfClosedRemote -> Closed", () => {
    let st = H2Stream.init(5, 16384, 16384) ?? unreachable;
    st.recvEndStream();
    expect_eq(st.state == StreamState.HalfClosedRemote, true);
    expect_eq(st.remoteClosed(), true);
    expect_eq(st.localClosed(), false);
    expect_eq(st.recordReceivedData(1, false), false);

    st.sentEndStream();
    expect_eq(st.isClosed(), true);
});

test("3. Idempotent RST_STREAM emission and wire-format validation", () => {
    let st = H2Stream.init(9, 65535, 65535) ?? unreachable;
    let frame: [13]u8 = std.mem.zeroes([13]u8);
    let out: []u8 = frame[0..];

    const n1 = st.reset(H2ErrorCode.Cancel, &out) ?? unreachable;
    expect_eq(n1, H2_RST_STREAM_FRAME_LEN);
    expect_eq(st.rst_done, true);
    expect_eq(st.isClosed(), true);
    expect_eq(st.rst_error, errorCodeToU32(H2ErrorCode.Cancel));

    // Check 9-byte header + 4-byte error code (Cancel = 8)
    expect_eq(frame[2], 4);
    expect_eq(frame[3], 0x3);
    expect_eq(readU32Be(frame[0..], 5), 9);
    expect_eq(readU32Be(frame[0..], 9), 8);

    // Second reset is a no-op returning 0 bytes
    const n2 = st.reset(H2ErrorCode.InternalError, &out) ?? unreachable;
    expect_eq(n2, 0);
});

test("4. Outbound flow control: consumeSendWindow, WINDOW_UPDATE overflow, and negative SETTINGS delta", () => {
    let st = H2Stream.init(1, 1000, 1000) ?? unreachable;
    expect_eq(st.consumeSendWindow(600), true);
    expect_eq(st.send_window, 400);
    expect_eq(st.data_bytes_sent, 600);
    expect_eq(st.consumeSendWindow(401), false);

    // RFC 9113 §6.9.2: peer lowers SETTINGS_INITIAL_WINDOW_SIZE by 1000 -> send_window legally becomes -600
    expect_eq(st.applySettingsInitialWindowDelta(-1000), true);
    expect_eq(st.send_window, -600);
    expect_eq(st.canSendData(), false);
    expect_eq(st.consumeSendWindow(1), false);

    // Peer sends WINDOW_UPDATE(+800) -> window recovers to +200
    expect_eq(st.applySendWindowUpdate(800), true);
    expect_eq(st.send_window, 200);
    expect_eq(st.canSendData(), true);

    // Overflow guard (> 2^31 - 1) and zero-increment guard
    expect_eq(st.applySendWindowUpdate(0), false);
    expect_eq(st.applySendWindowUpdate(H2_MAX_WINDOW_SIZE as u32), false);
});

test("5. Inbound flow control & threshold-triggered WINDOW_UPDATE replenishment", () => {
    let st = H2Stream.init(11, 65535, 10000) ?? unreachable;
    let frame: [13]u8 = std.mem.zeroes([13]u8);
    let out: []u8 = frame[0..];

    expect_eq(st.recordReceivedData(3000, false), true);
    expect_eq(st.recv_window, 7000);
    expect_eq(st.unacked_bytes, 3000);
    // Below threshold 5000 -> emits 0 bytes
    expect_eq(st.maybeEmitWindowUpdate(5000, &out) ?? unreachable, 0);

    expect_eq(st.recordReceivedData(2500, false), true);
    expect_eq(st.recv_window, 4500);
    expect_eq(st.unacked_bytes, 5500);
    // Reaches threshold -> emits 13-byte WINDOW_UPDATE for 5500 bytes and restores recv_window to 10000
    const n = st.maybeEmitWindowUpdate(5000, &out) ?? unreachable;
    expect_eq(n, H2_WINDOW_UPDATE_FRAME_LEN);
    expect_eq(frame[3], 0x8);
    expect_eq(readU32Be(frame[0..], 5), 11);
    expect_eq(readU32Be(frame[0..], 9), 5500);
    expect_eq(st.recv_window, 10000);
    expect_eq(st.unacked_bytes, 0);

    // Exceeding recv_window fails cleanly
    expect_eq(st.recordReceivedData(10001, false), false);
});

test("6. Defensive guards: invalid stream IDs, negative initial windows, zero/oversized increments, short buffers", () => {
    expect_eq(H2Stream.init(0, 65535, 65535) == null, true);
    expect_eq(H2Stream.init(0x80000000, 65535, 65535) == null, true);
    expect_eq(H2Stream.init(1, -1, 65535) == null, true);
    expect_eq(H2Stream.init(1, 65535, -1) == null, true);

    let tiny: [8]u8 = std.mem.zeroes([8]u8);
    let tiny_s: []u8 = tiny[0..];
    let ok_buf: [13]u8 = std.mem.zeroes([13]u8);
    let ok_s: []u8 = ok_buf[0..];

    expect_eq(encodeRstStreamFrame(0, H2ErrorCode.NoError, &ok_s) == null, true);
    expect_eq(encodeRstStreamFrame(1, H2ErrorCode.NoError, &tiny_s) == null, true);
    expect_eq(encodeWindowUpdateFrame(0, 0, &ok_s) == null, true);
    expect_eq(encodeWindowUpdateFrame(0, 0x80000000, &ok_s) == null, true);
    expect_eq(encodeWindowUpdateFrame(0, 1024, &tiny_s) == null, true);
});
