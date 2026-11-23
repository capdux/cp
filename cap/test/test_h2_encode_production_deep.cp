// Source: cp/cap/test/test_h2_encode_production_deep.cp (Production deep tests for cap:h2_encode)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    H2_CLIENT_PREFACE,
    H2_FRAME_DATA,
    H2_FRAME_HEADERS,
    H2_FRAME_SETTINGS,
    H2_FRAME_WINDOW_UPDATE,
    H2_FRAME_CONTINUATION,
    H2_FLAG_END_STREAM,
    H2_FLAG_END_HEADERS,
    RequestHeaderAction,
    DataWindowState,
    classifyRequestHeader,
    shouldSkipRequestHeader,
    isSensitiveHeader,
    lowercaseHeaderName,
    encodeHpackTableSizeUpdate,
    encodeFrameHeader,
    encodeClientPreface,
    encodeHeaderBlockFrames,
    encodeDataWindowed,
} from "cap:http";

fn readU24Be(b: []const u8, off: usize): u32 {
    return ((b[off] as u32) << 16) | ((b[off + 1] as u32) << 8) | (b[off + 2] as u32);
}

fn readU32Be(b: []const u8, off: usize): u32 {
    return ((b[off] as u32) << 24) | ((b[off + 1] as u32) << 16) | ((b[off + 2] as u32) << 8) | (b[off + 3] as u32);
}

test("1. RFC 9113 header classification, filtering, sensitive detection, and lowercase conversion", () => {
    expect_eq(classifyRequestHeader("Host") == RequestHeaderAction.Host, true);
    expect_eq(classifyRequestHeader("CONNECTION") == RequestHeaderAction.Drop, true);
    expect_eq(classifyRequestHeader("Transfer-Encoding") == RequestHeaderAction.Drop, true);
    expect_eq(classifyRequestHeader("Keep-Alive") == RequestHeaderAction.Drop, true);
    expect_eq(classifyRequestHeader("Upgrade") == RequestHeaderAction.Drop, true);
    expect_eq(classifyRequestHeader("Authorization") == RequestHeaderAction.ForwardNeverIndex, true);
    expect_eq(classifyRequestHeader("Cookie") == RequestHeaderAction.ForwardNeverIndex, true);
    expect_eq(classifyRequestHeader("User-Agent") == RequestHeaderAction.Forward, true);

    expect_eq(isSensitiveHeader("authorization"), true);
    expect_eq(isSensitiveHeader("Set-Cookie"), true);
    expect_eq(isSensitiveHeader("Accept"), false);

    // Filtering rules absorbed from Bun writeRequest
    expect_eq(shouldSkipRequestHeader("Connection", "keep-alive", false), true);
    expect_eq(shouldSkipRequestHeader("Host", "example.com", false), true);
    expect_eq(shouldSkipRequestHeader("TE", "  Trailers \t", false), false);
    expect_eq(shouldSkipRequestHeader("TE", "gzip", false), true);
    expect_eq(shouldSkipRequestHeader("Content-Length", "128", false), false);
    expect_eq(shouldSkipRequestHeader("Content-Length", "128", true), true);

    let lo_buf: [32]u8 = std.mem.zeroes([32]u8);
    let lo_s: []u8 = lo_buf[0..];
    const n = lowercaseHeaderName("X-Custom-Header", &lo_s) ?? unreachable;
    expect_eq(n, 15);
    expect_eq(std.mem.eql(u8, lo_buf[0..n], "x-custom-header"), true);
});

test("2. RFC 7541 HPACK dynamic table size update encoding (short < 31 and multi-byte >= 31)", () => {
    let buf: [8]u8 = std.mem.zeroes([8]u8);
    let out: []u8 = buf[0..];

    // value = 0 -> [0x20]
    const n0 = encodeHpackTableSizeUpdate(0, &out) ?? unreachable;
    expect_eq(n0, 1);
    expect_eq(buf[0], 0x20);

    // value = 30 -> [0x20 | 30 = 0x3E]
    const n30 = encodeHpackTableSizeUpdate(30, &out) ?? unreachable;
    expect_eq(n30, 1);
    expect_eq(buf[0], 0x3E);

    // value = 31 -> [0x3F, 0x00]
    const n31 = encodeHpackTableSizeUpdate(31, &out) ?? unreachable;
    expect_eq(n31, 2);
    expect_eq(buf[0], 0x3F);
    expect_eq(buf[1], 0x00);

    // value = 4096 -> 4096 - 31 = 4065 = 0xFE1 -> low7=0x61|0x80=0xE1, next=31 (0x1F)
    const n4k = encodeHpackTableSizeUpdate(4096, &out) ?? unreachable;
    expect_eq(n4k, 3);
    expect_eq(buf[0], 0x3F);
    expect_eq(buf[1], 0xE1);
    expect_eq(buf[2], 0x1F);
});

test("3. Client preface sequence: 24B preface + 18B SETTINGS + optional WINDOW_UPDATE", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let out: []u8 = buf[0..];

    // With window > 65535 -> 24 + (9+18) + (9+4) = 64 bytes
    const len_with_wu = encodeClientPreface(262144, 65536, &out) ?? unreachable;
    expect_eq(len_with_wu, 64);
    expect_eq(std.mem.eql(u8, buf[0..24], H2_CLIENT_PREFACE), true);
    // SETTINGS header at offset 24
    expect_eq(readU24Be(buf[0..], 24), 18);
    expect_eq(buf[27], H2_FRAME_SETTINGS);
    expect_eq(readU32Be(buf[0..], 29), 0);
    // WINDOW_UPDATE header at offset 51
    expect_eq(readU24Be(buf[0..], 51), 4);
    expect_eq(buf[54], H2_FRAME_WINDOW_UPDATE);
    expect_eq(readU32Be(buf[0..], 60), 262144 - 65535);

    // With default window 65535 -> no WINDOW_UPDATE, total 24 + 27 = 51 bytes
    const len_no_wu = encodeClientPreface(65535, 32768, &out) ?? unreachable;
    expect_eq(len_no_wu, 51);
});

test("4. HEADERS + CONTINUATION automatic fragmentation with exact flag placement", () => {
    const block = "0123456789ABCDEF"; // 16 bytes
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let out: []u8 = buf[0..];

    // max_frame_size = 6 -> 3 frames: 6B HEADERS, 6B CONTINUATION, 4B CONTINUATION
    const res = encodeHeaderBlockFrames(7, block, 6, true, &out) ?? unreachable;
    expect_eq(res.frame_count, 3);
    expect_eq(res.bytes_written, 3 * 9 + 16);

    // Frame 0: HEADERS, END_STREAM only (no END_HEADERS), stream 7, len 6
    expect_eq(readU24Be(buf[0..], 0), 6);
    expect_eq(buf[3], H2_FRAME_HEADERS);
    expect_eq(buf[4], H2_FLAG_END_STREAM);
    expect_eq(readU32Be(buf[0..], 5), 7);
    expect_eq(std.mem.eql(u8, buf[9..15], "012345"), true);

    // Frame 1: CONTINUATION, flags = 0, len 6
    expect_eq(readU24Be(buf[0..], 15), 6);
    expect_eq(buf[18], H2_FRAME_CONTINUATION);
    expect_eq(buf[19], 0);
    expect_eq(std.mem.eql(u8, buf[24..30], "6789AB"), true);

    // Frame 2: CONTINUATION, flags = END_HEADERS, len 4
    expect_eq(readU24Be(buf[0..], 30), 4);
    expect_eq(buf[33], H2_FRAME_CONTINUATION);
    expect_eq(buf[34], H2_FLAG_END_HEADERS);
    expect_eq(std.mem.eql(u8, buf[39..43], "CDEF"), true);
});

test("5. Flow-controlled DATA encoding respects stream/conn windows, max_frame_size, and send_cap", () => {
    const payload = "ABCDEFGHIJKLMNO"; // 15 bytes
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let out: []u8 = buf[0..];

    // conn_window = 10 limits total sent to 10 bytes across two frames (6B + 4B), END_STREAM NOT yet sent
    let win = DataWindowState { stream_window: 100, conn_window: 10 };
    const r1 = encodeDataWindowed(3, payload, 6, true, 100, &win, &out) ?? unreachable;
    expect_eq(r1.payload_consumed, 10);
    expect_eq(r1.frame_count, 2);
    expect_eq(r1.sent_end_stream, false);
    expect_eq(win.conn_window, 0);
    expect_eq(win.stream_window, 90);
    expect_eq(buf[3], H2_FRAME_DATA);
    expect_eq(buf[4], 0);

    // Replenish conn_window and send remaining 5 bytes -> sets END_STREAM
    win.conn_window = 20;
    const r2 = encodeDataWindowed(3, payload[10..], 6, true, 100, &win, &out) ?? unreachable;
    expect_eq(r2.payload_consumed, 5);
    expect_eq(r2.frame_count, 1);
    expect_eq(r2.sent_end_stream, true);
    expect_eq(buf[4], H2_FLAG_END_STREAM);
    expect_eq(win.conn_window, 15);
    expect_eq(win.stream_window, 85);
});

test("6. Defensive guards: invalid stream IDs, oversized frames, and insufficient output buffers", () => {
    let tiny_arr: [4]u8 = std.mem.zeroes([4]u8);
    let tiny: []u8 = tiny_arr[0..];
    let ok_arr: [32]u8 = std.mem.zeroes([32]u8);
    let ok_buf: []u8 = ok_arr[0..];

    expect_eq(encodeFrameHeader(10, H2_FRAME_DATA, 0, 1, &tiny) == null, true);
    expect_eq(encodeFrameHeader(0x1000000, H2_FRAME_DATA, 0, 1, &ok_buf) == null, true);
    expect_eq(encodeFrameHeader(10, H2_FRAME_DATA, 0, 0x80000000, &ok_buf) == null, true);
    expect_eq(encodeClientPreface(65535, 65535, &ok_buf) == null, true);
    expect_eq(encodeHeaderBlockFrames(0, "abc", 16, true, &ok_buf) == null, true);
    expect_eq(encodeHeaderBlockFrames(1, "abc", 0, true, &ok_buf) == null, true);
    expect_eq(lowercaseHeaderName("Too-Long-For-Tiny", &tiny) == null, true);

    let win = DataWindowState { stream_window: 100, conn_window: 100 };
    expect_eq(encodeDataWindowed(0, "abc", 16, true, 100, &win, &ok_buf) == null, true);
    expect_eq(encodeDataWindowed(1, "abc", 0, true, 100, &win, &ok_buf) == null, true);
    expect_eq(encodeDataWindowed(1, "abc", 16, true, 100, &win, &tiny) == null, true);
});
