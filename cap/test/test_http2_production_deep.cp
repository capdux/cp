// Source: cp/cap/test/test_http2_production_deep.cp
// Deep Production Verification Suite for cap:http2 & cap.http2
// Covers:
//   1. SETTINGS (with ACK validation), PING, GOAWAY, RST_STREAM, WINDOW_UPDATE & DATA (with PADDED) roundtrip
//   2. FrameStreamDecoder incremental 1..7-byte chunking (partial & sticky frames) & CONNECTION_PREFACE check
//   3. FlowWindow consume/replenish, FlowControlError overdraw, 0 / 2^31 overflow guard & Http2Stream states
//   4. FrameSizeError (> max_frame_size), InvalidSettingsLength (non-multiple of 6) & ProtocolError (SETTINGS stream_id != 0)
//   5. `import cap from "cap"; cap.http2` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CONNECTION_PREFACE,
    DEFAULT_MAX_FRAME_SIZE,
    DEFAULT_INITIAL_WINDOW_SIZE,
    MAX_WINDOW_SIZE,
    Http2Error,
    FrameType,
    FrameFlags,
    Http2ErrorCode,
    SettingId,
    SettingParam,
    Http2Stream,
    StreamState,
    Http2Frame,
    FrameStreamDecoder,
    FlowWindow,
    hasConnectionPreface,
    encodeFrame,
    encodeSettings,
    decodeSettings,
    encodePing,
    decodePing,
    encodeGoaway,
    decodeGoaway,
    encodeWindowUpdate,
    decodeWindowUpdate,
    encodeRstStream,
    decodeRstStream,
    encodeDataFrame,
    decodeDataPayload,
} from "cap:http2";

test("cap:http2 - SETTINGS (incl ACK), PING, GOAWAY, RST_STREAM, WINDOW_UPDATE & PADDED DATA roundtrip") {
    const alloc = std.testing.allocator;

    let decoder = FrameStreamDecoder.init(alloc, DEFAULT_MAX_FRAME_SIZE);
    defer decoder.deinit();

    // 1. SETTINGS frame with 2 parameters
    const params = [
        SettingParam.init(SettingId.MAX_CONCURRENT_STREAMS, 128),
        SettingParam.init(SettingId.INITIAL_WINDOW_SIZE, 131072),
    ];
    const settings_wire = encodeSettings(alloc, params[0..], false) catch unreachable;
    defer alloc.free(settings_wire);
    expect_eq(settings_wire.len, 9 + 12);

    // 2. SETTINGS ACK frame (must be empty payload)
    const empty_params: [0]SettingParam = [];
    const ack_wire = encodeSettings(alloc, empty_params[0..], true) catch unreachable;
    defer alloc.free(ack_wire);
    expect_eq(ack_wire.len, 9);

    // 3. PING frame
    const ping_bytes: [8]u8 = [0x01, 0x02, 0x03, 0x04, 0xaa, 0xbb, 0xcc, 0xdd];
    const ping_wire = encodePing(alloc, ping_bytes, true) catch unreachable;
    defer alloc.free(ping_wire);

    // 4. GOAWAY frame
    const goaway_wire = encodeGoaway(alloc, 17, Http2ErrorCode.EnhanceYourCalm, "slow down") catch unreachable;
    defer alloc.free(goaway_wire);

    // 5. RST_STREAM frame
    const rst_wire = encodeRstStream(alloc, 5, Http2ErrorCode.Cancel) catch unreachable;
    defer alloc.free(rst_wire);

    // 6. WINDOW_UPDATE frame
    const wu_wire = encodeWindowUpdate(alloc, 5, 4096) catch unreachable;
    defer alloc.free(wu_wire);

    // 7. PADDED DATA frame
    const data_wire = encodeDataFrame(alloc, 3, "hello http2", true, 4) catch unreachable;
    defer alloc.free(data_wire);

    decoder.feed(settings_wire) catch unreachable;
    decoder.feed(ack_wire) catch unreachable;
    decoder.feed(ping_wire) catch unreachable;
    decoder.feed(goaway_wire) catch unreachable;
    decoder.feed(rst_wire) catch unreachable;
    decoder.feed(wu_wire) catch unreachable;
    decoder.feed(data_wire) catch unreachable;

    // Decode & verify SETTINGS
    let f1 = (decoder.nextFrame() catch unreachable).?;
    defer f1.deinit(alloc);
    expect_eq(f1.frame_type == FrameType.Settings, true);
    const decoded_params = decodeSettings(alloc, f1) catch unreachable;
    defer alloc.free(decoded_params);
    expect_eq(decoded_params.len, 2);
    expect_eq(decoded_params[0].id, SettingId.MAX_CONCURRENT_STREAMS);
    expect_eq(decoded_params[0].value, 128);
    expect_eq(decoded_params[1].id, SettingId.INITIAL_WINDOW_SIZE);
    expect_eq(decoded_params[1].value, 131072);

    // Decode & verify SETTINGS ACK
    let f2 = (decoder.nextFrame() catch unreachable).?;
    defer f2.deinit(alloc);
    expect_eq(f2.hasFlag(FrameFlags.ACK), true);
    const ack_params = decodeSettings(alloc, f2) catch unreachable;
    defer alloc.free(ack_params);
    expect_eq(ack_params.len, 0);

    // Decode & verify PING
    let f3 = (decoder.nextFrame() catch unreachable).?;
    defer f3.deinit(alloc);
    expect_eq(f3.hasFlag(FrameFlags.ACK), true);
    const ping_out = decodePing(f3) catch unreachable;
    expect_eq(std.mem.eql(u8, ping_out[0..], ping_bytes[0..]), true);

    // Decode & verify GOAWAY
    let f4 = (decoder.nextFrame() catch unreachable).?;
    defer f4.deinit(alloc);
    const ga = decodeGoaway(f4) catch unreachable;
    expect_eq(ga.last_stream_id, 17);
    expect_eq(ga.error_code, Http2ErrorCode.EnhanceYourCalm as u32);
    expect_eq(std.mem.eql(u8, ga.debug_data, "slow down"), true);

    // Decode & verify RST_STREAM
    let f5 = (decoder.nextFrame() catch unreachable).?;
    defer f5.deinit(alloc);
    expect_eq(f5.stream_id, 5);
    expect_eq(decodeRstStream(f5) catch unreachable, Http2ErrorCode.Cancel as u32);

    // Decode & verify WINDOW_UPDATE
    let f6 = (decoder.nextFrame() catch unreachable).?;
    defer f6.deinit(alloc);
    expect_eq(f6.stream_id, 5);
    expect_eq(decodeWindowUpdate(f6) catch unreachable, 4096);

    // Decode & verify PADDED DATA
    let f7 = (decoder.nextFrame() catch unreachable).?;
    defer f7.deinit(alloc);
    expect_eq(f7.stream_id, 3);
    expect_eq(f7.hasFlag(FrameFlags.END_STREAM), true);
    expect_eq(f7.hasFlag(FrameFlags.PADDED), true);
    const app_data = decodeDataPayload(f7) catch unreachable;
    expect_eq(std.mem.eql(u8, app_data, "hello http2"), true);

    // No more frames
    expect_eq((decoder.nextFrame() catch unreachable) == null, true);
}

test("cap:http2 - FrameStreamDecoder arbitrary 1..7-byte chunking & CONNECTION_PREFACE") {
    const alloc = std.testing.allocator;

    expect_eq(CONNECTION_PREFACE.len, 24);
    expect_eq(hasConnectionPreface(CONNECTION_PREFACE), true);
    expect_eq(hasConnectionPreface("PRI * HTTP/1.1\r\n\r\nSM\r\n\r\n"), false);

    // Build a stream with 3 concatenated frames
    const w1 = encodeDataFrame(alloc, 1, "chunk-alpha", false, 0) catch unreachable;
    defer alloc.free(w1);
    const w2 = encodeDataFrame(alloc, 1, "chunk-beta", true, 3) catch unreachable;
    defer alloc.free(w2);
    const w3 = encodeWindowUpdate(alloc, 0, 65536) catch unreachable;
    defer alloc.free(w3);

    let combined = std.ArrayList(u8).empty;
    defer combined.deinit(alloc);
    combined.appendSlice(alloc, w1) catch unreachable;
    combined.appendSlice(alloc, w2) catch unreachable;
    combined.appendSlice(alloc, w3) catch unreachable;

    let decoder = FrameStreamDecoder.init(alloc, DEFAULT_MAX_FRAME_SIZE);
    defer decoder.deinit();

    // Feed in varying small chunk sizes (1..7 bytes) to stress partial headers & sticky frames
    let pos: usize = 0;
    let step: usize = 1;
    let decoded_count: usize = 0;

    while (pos < combined.items.len) {
        const end = @min(pos + step, combined.items.len);
        decoder.feed(combined.items[pos..end]) catch unreachable;
        pos = end;
        step = if (step >= 7) 1 else step + 1;

        while (true) {
            const maybe_frame = decoder.nextFrame() catch unreachable;
            if (maybe_frame == null) {
                break;
            }
            let frame = maybe_frame.?;
            defer frame.deinit(alloc);

            if (decoded_count == 0) {
                expect_eq(frame.stream_id, 1);
                expect_eq(std.mem.eql(u8, decodeDataPayload(frame) catch unreachable, "chunk-alpha"), true);
            } else if (decoded_count == 1) {
                expect_eq(frame.stream_id, 1);
                expect_eq(frame.hasFlag(FrameFlags.END_STREAM), true);
                expect_eq(std.mem.eql(u8, decodeDataPayload(frame) catch unreachable, "chunk-beta"), true);
            } else if (decoded_count == 2) {
                expect_eq(frame.stream_id, 0);
                expect_eq(decodeWindowUpdate(frame) catch unreachable, 65536);
            }
            decoded_count += 1;
        }
    }

    expect_eq(decoded_count, 3);
    expect_eq(decoder.bufferedLen(), 0);
}

test("cap:http2 - FlowWindow consume/replenish, FlowControlError, 0/2^31 overflow & Http2Stream") {
    let win = FlowWindow.init(DEFAULT_INITIAL_WINDOW_SIZE);
    expect_eq(win.available(), 65535);

    win.consume(60000) catch unreachable;
    expect_eq(win.available(), 5535);

    // Overdrawing window must trigger FlowControlError
    let overdraw_err = false;
    win.consume(5536) catch |e| {
        expect_eq(e == Http2Error.FlowControlError, true);
        overdraw_err = true;
    };
    expect_eq(overdraw_err, true);
    expect_eq(win.available(), 5535);

    // Zero or > 0x7FFFFFFF replenish must trigger InvalidWindowIncrement
    let zero_inc_err = false;
    win.replenish(0) catch |e| {
        expect_eq(e == Http2Error.InvalidWindowIncrement, true);
        zero_inc_err = true;
    };
    expect_eq(zero_inc_err, true);

    let big_inc_err = false;
    win.replenish(0x80000000) catch |e| {
        expect_eq(e == Http2Error.InvalidWindowIncrement, true);
        big_inc_err = true;
    };
    expect_eq(big_inc_err, true);

    // Replenishing past 2^31 - 1 must trigger FlowControlError
    let overflow_err = false;
    win.replenish(MAX_WINDOW_SIZE) catch |e| {
        expect_eq(e == Http2Error.FlowControlError, true);
        overflow_err = true;
    };
    expect_eq(overflow_err, true);

    // Valid replenish
    win.replenish(10000) catch unreachable;
    expect_eq(win.available(), 15535);

    // Stream state lifecycle check
    let st = Http2Stream.init(1);
    expect_eq(st.state == StreamState.Idle, true);
    st.sendHeaders(false) catch unreachable;
    expect_eq(st.state == StreamState.Open, true);
    st.recvHeaders(true) catch unreachable;
    expect_eq(st.state == StreamState.HalfClosedRemote, true);
    st.sendHeaders(true) catch unreachable;
    expect_eq(st.state == StreamState.Closed, true);
}

test("cap:http2 - FrameSizeError, InvalidSettingsLength, ProtocolError & InvalidWindowIncrement guards") {
    const alloc = std.testing.allocator;

    // 1. FrameSizeError when payload length exceeds decoder max_frame_size (16384)
    let big_payload = alloc.alloc(u8, 16385) catch unreachable;
    defer alloc.free(big_payload);
    @memset(big_payload, 0x41);
    const big_frame_wire = encodeFrame(alloc, FrameType.Data, 0, 1, big_payload) catch unreachable;
    defer alloc.free(big_frame_wire);

    let dec1 = FrameStreamDecoder.init(alloc, DEFAULT_MAX_FRAME_SIZE);
    defer dec1.deinit();
    dec1.feed(big_frame_wire) catch unreachable;
    let hit_frame_size_err = false;
    * = dec1.nextFrame() catch |e| {
        expect_eq(e == Http2Error.FrameSizeError, true);
        hit_frame_size_err = true;
    };
    expect_eq(hit_frame_size_err, true);

    // Raising max_frame_size up to 16777215 allows decoding the 16385-byte frame
    let dec_large = FrameStreamDecoder.init(alloc, 32768);
    defer dec_large.deinit();
    dec_large.feed(big_frame_wire) catch unreachable;
    let ok_big = (dec_large.nextFrame() catch unreachable).?;
    defer ok_big.deinit(alloc);
    expect_eq(ok_big.payload.len, 16385);

    // 2. InvalidSettingsLength when SETTINGS payload is not a multiple of 6
    const bad_settings_payload: [5]u8 = [0, 1, 0, 0, 1];
    const bad_settings_wire = encodeFrame(alloc, FrameType.Settings, 0, 0, bad_settings_payload[0..]) catch unreachable;
    defer alloc.free(bad_settings_wire);

    let dec2 = FrameStreamDecoder.init(alloc, DEFAULT_MAX_FRAME_SIZE);
    defer dec2.deinit();
    dec2.feed(bad_settings_wire) catch unreachable;
    let hit_settings_len_err = false;
    * = dec2.nextFrame() catch |e| {
        expect_eq(e == Http2Error.InvalidSettingsLength, true);
        hit_settings_len_err = true;
    };
    expect_eq(hit_settings_len_err, true);

    // 3. ProtocolError when SETTINGS has non-zero stream_id
    const valid_settings_payload: [6]u8 = [0, 3, 0, 0, 0, 100];
    const bad_sid_settings = encodeFrame(alloc, FrameType.Settings, 0, 1, valid_settings_payload[0..]) catch unreachable;
    defer alloc.free(bad_sid_settings);

    let dec3 = FrameStreamDecoder.init(alloc, DEFAULT_MAX_FRAME_SIZE);
    defer dec3.deinit();
    dec3.feed(bad_sid_settings) catch unreachable;
    let hit_proto_err = false;
    * = dec3.nextFrame() catch |e| {
        expect_eq(e == Http2Error.ProtocolError, true);
        hit_proto_err = true;
    };
    expect_eq(hit_proto_err, true);

    // 4. InvalidWindowIncrement when encodeWindowUpdate receives 0
    let hit_wu_err = false;
    * = encodeWindowUpdate(alloc, 0, 0) catch |e| {
        expect_eq(e == Http2Error.InvalidWindowIncrement, true);
        hit_wu_err = true;
    };
    expect_eq(hit_wu_err, true);
}

test("cap.http2 - dynamic mounting via import cap from \"cap\" with std.testing.allocator") {
    const alloc = std.testing.allocator;

    expect_eq(cap.http2.CONNECTION_PREFACE.len, 24);
    let decoder = cap.http2.FrameStreamDecoder.init(alloc, cap.http2.DEFAULT_MAX_FRAME_SIZE);
    defer decoder.deinit();

    const params = [
        cap.http2.SettingParam.init(cap.http2.SettingId.HEADER_TABLE_SIZE, 8192),
    ];
    const wire = cap.http2.encodeSettings(alloc, params[0..], false) catch unreachable;
    defer alloc.free(wire);

    decoder.feed(wire) catch unreachable;
    let frame = (decoder.nextFrame() catch unreachable).?;
    defer frame.deinit(alloc);

    const decoded = cap.http2.decodeSettings(alloc, frame) catch unreachable;
    defer alloc.free(decoded);
    expect_eq(decoded.len, 1);
    expect_eq(decoded[0].id, cap.http2.SettingId.HEADER_TABLE_SIZE);
    expect_eq(decoded[0].value, 8192);

    let win = cap.http2.FlowWindow.init(cap.http2.DEFAULT_INITIAL_WINDOW_SIZE);
    win.consume(1024) catch unreachable;
    expect_eq(win.available(), 64511);
}
