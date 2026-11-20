// Source: cp/cap/test/test_http2_frame_production_deep.cp
const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    H2_PREFACE,
    H2_HEADER_LEN,
    H2_DEFAULT_MAX_FRAME_SIZE,
    H2_DEFAULT_INITIAL_WINDOW_SIZE,
    H2_MAX_WINDOW_SIZE,
    H2Error,
    H2FrameType,
    H2ErrorCode,
    H2FrameHeader,
    H2Setting,
    H2PrioritySpec,
    H2DataFrame,
    H2HeadersFrame,
    H2RstStreamFrame,
    H2SettingsFrame,
    H2PingFrame,
    H2GoAwayFrame,
    H2WindowUpdateFrame,
    H2Frame,
    validatePreface,
    decodeFrame,
    encodeFrame,
    freeDecodedFrame,
    H2StreamDecoder,
    H2StreamState,
    H2ConnectionWindow,
} from "cap:http2_frame";

test("http2_frame: SETTINGS/PING/WINDOW_UPDATE/GOAWAY/RST_STREAM roundtrip & preface") {
    const alloc = std.testing.allocator;
    try validatePreface(H2_PREFACE);
    try std.testing.expectError(H2Error.InvalidPreface, validatePreface("PRI * HTTP/1.1\r\n\r\nSM\r\n\r\n"));

    const settings_items: [2]H2Setting = [
        H2Setting{ .id = 0x3, .value = 100 },
        H2Setting{ .id = 0x4, .value = 65535 },
    ];
    const settings_wire = try encodeFrame(alloc, H2Frame{
        .settings = H2SettingsFrame{ .ack = false, .settings = &settings_items },
    });
    defer alloc.free(settings_wire);
    const s_hdr = try H2FrameHeader.parse(settings_wire[0..H2_HEADER_LEN]);
    try expect_eq(s_hdr.length, @as(u32, 12));
    try expect_eq(s_hdr.kind().?, H2FrameType.settings);
    const s_dec = try decodeFrame(alloc, s_hdr, settings_wire[H2_HEADER_LEN..]);
    defer freeDecodedFrame(alloc, s_dec);
    switch (s_dec) {
        .settings => |sf| {
            try expect_eq(sf.ack, false);
            try expect_eq(sf.settings.len, @as(usize, 2));
            try expect_eq(sf.settings[0].id, @as(u16, 0x3));
            try expect_eq(sf.settings[0].value, @as(u32, 100));
            try expect_eq(sf.settings[1].id, @as(u16, 0x4));
            try expect_eq(sf.settings[1].value, @as(u32, 65535));
        },
        else => return error.TestUnexpectedResult,
    }

    const ping_wire = try encodeFrame(alloc, H2Frame{
        .ping = H2PingFrame{ .ack = true, .opaque_data = [ 1, 2, 3, 4, 5, 6, 7, 8 ] },
    });
    defer alloc.free(ping_wire);
    const p_hdr = try H2FrameHeader.parse(ping_wire[0..H2_HEADER_LEN]);
    const p_dec = try decodeFrame(alloc, p_hdr, ping_wire[H2_HEADER_LEN..]);
    const expected_ping: [8]u8 = [ 1, 2, 3, 4, 5, 6, 7, 8 ];
    switch (p_dec) {
        .ping => |pf| {
            try expect_eq(pf.ack, true);
            try std.testing.expectEqualSlices(u8, &pf.opaque_data, &expected_ping);
        },
        else => return error.TestUnexpectedResult,
    }

    const wu_wire = try encodeFrame(alloc, H2Frame{
        .window_update = H2WindowUpdateFrame{ .stream_id = 7, .increment = 32768 },
    });
    defer alloc.free(wu_wire);
    const w_hdr = try H2FrameHeader.parse(wu_wire[0..H2_HEADER_LEN]);
    const w_dec = try decodeFrame(alloc, w_hdr, wu_wire[H2_HEADER_LEN..]);
    switch (w_dec) {
        .window_update => |wf| {
            try expect_eq(wf.stream_id, @as(u32, 7));
            try expect_eq(wf.increment, @as(u32, 32768));
        },
        else => return error.TestUnexpectedResult,
    }

    const ga_wire = try encodeFrame(alloc, H2Frame{
        .goaway = H2GoAwayFrame{
            .last_stream_id = 15,
            .error_code = @intFromEnum(H2ErrorCode.no_error),
            .debug_data = "graceful",
        },
    });
    defer alloc.free(ga_wire);
    const g_hdr = try H2FrameHeader.parse(ga_wire[0..H2_HEADER_LEN]);
    const g_dec = try decodeFrame(alloc, g_hdr, ga_wire[H2_HEADER_LEN..]);
    switch (g_dec) {
        .goaway => |gf| {
            try expect_eq(gf.last_stream_id, @as(u32, 15));
            try expect_eq(gf.error_code, @as(u32, 0));
            try std.testing.expectEqualStrings("graceful", gf.debug_data);
        },
        else => return error.TestUnexpectedResult,
    }

    const rst_wire = try encodeFrame(alloc, H2Frame{
        .rst_stream = H2RstStreamFrame{ .stream_id = 9, .error_code = @intFromEnum(H2ErrorCode.cancel) },
    });
    defer alloc.free(rst_wire);
    const r_hdr = try H2FrameHeader.parse(rst_wire[0..H2_HEADER_LEN]);
    const r_dec = try decodeFrame(alloc, r_hdr, rst_wire[H2_HEADER_LEN..]);
    switch (r_dec) {
        .rst_stream => |rf| {
            try expect_eq(rf.stream_id, @as(u32, 9));
            try expect_eq(rf.error_code, @as(u32, 0x8));
        },
        else => return error.TestUnexpectedResult,
    }
}

test("http2_frame: DATA (PADDED) + HEADERS (PRIORITY) & 1..7B incremental stream decoding") {
    const alloc = std.testing.allocator;

    const h_wire = try encodeFrame(alloc, H2Frame{
        .headers = H2HeadersFrame{
            .stream_id = 3,
            .end_stream = false,
            .end_headers = true,
            .pad_len = 2,
            .priority = H2PrioritySpec{ .exclusive = true, .stream_dependency = 1, .weight = 64 },
            .header_block = "\x82\x84\x86",
        },
    });
    defer alloc.free(h_wire);

    const d_wire = try encodeFrame(alloc, H2Frame{
        .data = H2DataFrame{
            .stream_id = 3,
            .end_stream = true,
            .pad_len = 4,
            .payload = "hello-h2",
        },
    });
    defer alloc.free(d_wire);

    const combined = try alloc.alloc(u8, h_wire.len + d_wire.len);
    defer alloc.free(combined);
    std.mem.copyForwards(u8, combined[0..h_wire.len], h_wire);
    std.mem.copyForwards(u8, combined[h_wire.len..], d_wire);

    let decoder = try H2StreamDecoder.init(alloc, H2_DEFAULT_MAX_FRAME_SIZE);
    defer decoder.deinit();

    let decoded_count: usize = 0;
    let pos: usize = 0;
    let step_idx: usize = 0;
    const steps: [7]usize = [ 1, 3, 2, 7, 5, 4, 6 ];

    while (pos < combined.len) {
        const step = steps[step_idx % steps.len];
        step_idx += 1;
        const end = @min(pos + step, combined.len);
        try decoder.feed(combined[pos..end]);
        pos = end;

        while (maybe_f := try decoder.nextFrame()) {
            let f = maybe_f;
            defer f.deinit(alloc);
            if (decoded_count == 0) {
                switch (f.frame) {
                    .headers => |hf| {
                        try expect_eq(hf.stream_id, @as(u32, 3));
                        try expect_eq(hf.end_stream, false);
                        try expect_eq(hf.end_headers, true);
                        try expect_eq(hf.pad_len, @as(u8, 2));
                        try expect_eq(hf.priority.?.exclusive, true);
                        try expect_eq(hf.priority.?.stream_dependency, @as(u32, 1));
                        try expect_eq(hf.priority.?.weight, @as(u8, 64));
                        try std.testing.expectEqualSlices(u8, "\x82\x84\x86", hf.header_block);
                    },
                    else => return error.TestUnexpectedResult,
                }
            } else if (decoded_count == 1) {
                switch (f.frame) {
                    .data => |df| {
                        try expect_eq(df.stream_id, @as(u32, 3));
                        try expect_eq(df.end_stream, true);
                        try expect_eq(df.pad_len, @as(u8, 4));
                        try std.testing.expectEqualStrings("hello-h2", df.payload);
                    },
                    else => return error.TestUnexpectedResult,
                }
            }
            decoded_count += 1;
        }
    }
    try expect_eq(decoded_count, @as(usize, 2));
    try expect_eq(decoder.bufferedLen(), @as(usize, 0));
}

test("http2_frame: FrameTooLarge, invalid control frame lengths, and stream id guards") {
    const alloc = std.testing.allocator;

    // 1. FrameTooLarge in H2StreamDecoder
    let decoder = try H2StreamDecoder.init(alloc, H2_DEFAULT_MAX_FRAME_SIZE);
    defer decoder.deinit();
    let big_hdr: [9]u8 = undefined;
    const hdr = H2FrameHeader{
        .length = H2_DEFAULT_MAX_FRAME_SIZE + 1,
        .frame_type = @intFromEnum(H2FrameType.data),
        .flags = 0,
        .stream_id = 1,
    };
    try hdr.encode(&big_hdr);
    try decoder.feed(&big_hdr);
    try std.testing.expectError(H2Error.FrameTooLarge, decoder.nextFrame());

    // 2. PING length != 8
    const bad_ping_hdr = H2FrameHeader{ .length = 7, .frame_type = 6, .flags = 0, .stream_id = 0 };
    try std.testing.expectError(H2Error.InvalidFrameLength, decodeFrame(alloc, bad_ping_hdr, "1234567"));

    // 3. WINDOW_UPDATE increment == 0
    const bad_wu_hdr = H2FrameHeader{ .length = 4, .frame_type = 8, .flags = 0, .stream_id = 0 };
    const zero_inc: [4]u8 = [ 0, 0, 0, 0 ];
    try std.testing.expectError(H2Error.InvalidIncrement, decodeFrame(alloc, bad_wu_hdr, &zero_inc));

    // 4. RST_STREAM stream_id == 0 & wrong length
    const bad_rst_sid = H2FrameHeader{ .length = 4, .frame_type = 3, .flags = 0, .stream_id = 0 };
    try std.testing.expectError(H2Error.InvalidStreamId, decodeFrame(alloc, bad_rst_sid, &zero_inc));
    const bad_rst_len = H2FrameHeader{ .length = 2, .frame_type = 3, .flags = 0, .stream_id = 1 };
    try std.testing.expectError(H2Error.InvalidFrameLength, decodeFrame(alloc, bad_rst_len, "ab"));
}

test("http2_frame: H2ConnectionWindow stream state machine & flow control overflow") {
    const alloc = std.testing.allocator;
    let win = try H2ConnectionWindow.init(alloc, H2_DEFAULT_INITIAL_WINDOW_SIZE);
    defer win.deinit();

    try expect_eq(win.getStreamState(1), H2StreamState.idle);
    try win.transitionOnSendHeaders(1, false);
    try expect_eq(win.getStreamState(1), H2StreamState.open);

    try win.consumeSendWindow(1, 10000);
    try expect_eq(win.getSendWindow(0), H2_DEFAULT_INITIAL_WINDOW_SIZE - 10000);
    try expect_eq(win.getSendWindow(1), H2_DEFAULT_INITIAL_WINDOW_SIZE - 10000);

    try win.applyWindowUpdate(1, 5000);
    try expect_eq(win.getSendWindow(1), H2_DEFAULT_INITIAL_WINDOW_SIZE - 5000);

    // Overflow check (> 2^31 - 1)
    try std.testing.expectError(H2Error.FlowControlError, win.applyWindowUpdate(1, @as(u32, @intCast(H2_MAX_WINDOW_SIZE))));

    // HalfClosedLocal -> Closed
    try win.transitionOnSendEndStream(1);
    try expect_eq(win.getStreamState(1), H2StreamState.half_closed_local);
    try win.transitionOnRecvEndStream(1);
    try expect_eq(win.getStreamState(1), H2StreamState.closed);
    try std.testing.expectError(H2Error.InvalidStreamState, win.consumeSendWindow(1, 1));
}

test("http2_frame: cap.http2_frame dynamic mount & zero leak verification") {
    const alloc = std.testing.allocator;
    try cap.http2_frame.validatePreface(cap.http2_frame.H2_PREFACE);

    const wire = try cap.http2_frame.encodeFrame(alloc, cap.http2_frame.H2Frame{
        .ping = cap.http2_frame.H2PingFrame{ .ack = false, .opaque_data = [ 9, 8, 7, 6, 5, 4, 3, 2 ] },
    });
    defer alloc.free(wire);

    let decoder = try cap.http2_frame.H2StreamDecoder.init(alloc, cap.http2_frame.H2_DEFAULT_MAX_FRAME_SIZE);
    defer decoder.deinit();
    try decoder.feed(wire);
    const maybe_frame = try decoder.nextFrame();
    try std.testing.expect(maybe_frame != null);
    let f = maybe_frame.?;
    defer f.deinit(alloc);
    try expect_eq(f.header.length, @as(u32, 8));
}
