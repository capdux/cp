// Source: cp/cap/test/test_websocket_frame_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:websocket_frame & cap.websocket_frame

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    WS_GUID,
    WsOpcode,
    WsCloseCode,
    WsError,
    WsFrame,
    DecodedFrame,
    WsMessage,
    WsStreamAssembler,
    computeAcceptKey,
    applyMask,
    encodeFrame,
    encodeCloseFrame,
    decodeFrame,
} from "cap:websocket_frame";

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. RFC 6455 Section 4.2.2 official Sec-WebSocket-Accept vector
    const accept = computeAcceptKey("dGhlIHNhbXBsZSBub25jZQ==");
    expect_eq(std.mem.eql(u8, &accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="), true);
    expect_eq(std.mem.eql(u8, WS_GUID, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"), true);

    // 2. 7-bit masked frame round-trip
    const mask_key: [4]u8 = [0x37, 0xFA, 0x21, 0x3D];
    const f7 = encodeFrame(alloc, .Text, "Hello, RFC 6455!", true, mask_key)?;
    defer alloc.free(f7);
    expect_eq(f7[0], 0x81 as u8);
    expect_eq((f7[1] & 0x80) != 0, true);
    expect_eq((f7[1] & 0x7F), 16 as u8);

    const dec7 = decodeFrame(alloc, f7, true, 1024)?;
    let frame7 = dec7.frame;
    defer frame7.deinit(alloc);
    expect_eq(dec7.bytes_consumed, f7.len);
    expect_eq(frame7.fin, true);
    expect_eq(frame7.opcode == .Text, true);
    expect_eq(frame7.masked, true);
    expect_eq(std.mem.eql(u8, frame7.payload, "Hello, RFC 6455!"), true);

    // 3. 16-bit (126) payload length round-trip (300 bytes)
    const buf16 = try alloc.alloc(u8, 300);
    defer alloc.free(buf16);
    @memset(buf16, 0xAB as u8);
    const wire16 = encodeFrame(alloc, .Binary, buf16, true, null)?;
    defer alloc.free(wire16);
    expect_eq(wire16[0], 0x82 as u8);
    expect_eq(wire16[1], 126 as u8);
    expect_eq(std.mem.readInt(u16, wire16[2..4], .big), 300 as u16);

    const dec16 = decodeFrame(alloc, wire16, false, 4096)?;
    let frame16 = dec16.frame;
    defer frame16.deinit(alloc);
    expect_eq(dec16.bytes_consumed, wire16.len);
    expect_eq(std.mem.eql(u8, frame16.payload, buf16), true);

    // 4. 64-bit (127) payload length round-trip (70000 bytes)
    let buf64 = try alloc.alloc(u8, 70000);
    defer alloc.free(buf64);
    let idx: usize = 0;
    while (idx < buf64.len) : (idx += 1) {
        buf64[idx] = (idx & 0xFF) as u8;
    }
    const wire64 = encodeFrame(alloc, .Binary, buf64, true, mask_key)?;
    defer alloc.free(wire64);
    expect_eq(wire64[1] & 0x7F, 127 as u8);
    expect_eq(std.mem.readInt(u64, wire64[2..10], .big), 70000 as u64);

    const dec64 = decodeFrame(alloc, wire64, true, 128 * 1024)?;
    let frame64 = dec64.frame;
    defer frame64.deinit(alloc);
    expect_eq(dec64.bytes_consumed, wire64.len);
    expect_eq(std.mem.eql(u8, frame64.payload, buf64), true);

    // 5. Close frame encode/decode & applyMask idempotence
    const fclose = encodeCloseFrame(alloc, @intFromEnum(WsCloseCode.NormalClosure), "bye", mask_key)?;
    defer alloc.free(fclose);
    const dec_close = decodeFrame(alloc, fclose, true, 1024)?;
    let fc = dec_close.frame;
    defer fc.deinit(alloc);
    expect_eq(fc.opcode == .Close, true);
    expect_eq(std.mem.readInt(u16, fc.payload[0..2], .big), 1000 as u16);
    expect_eq(std.mem.eql(u8, fc.payload[2..], "bye"), true);

    let scratch: [7]u8 = [1, 2, 3, 4, 5, 6, 7];
    applyMask(&scratch, mask_key);
    applyMask(&scratch, mask_key);
    expect_eq(scratch[0], 1 as u8);
    expect_eq(scratch[6], 7 as u8);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    const mask_key: [4]u8 = [0x12, 0x34, 0x56, 0x78];

    // Pipelined 3 frames coalesced into a single buffer, then fed in 1..7 byte chunks
    const w1 = encodeFrame(alloc, .Text, "alpha", true, mask_key)?;
    defer alloc.free(w1);
    const w2 = encodeFrame(alloc, .Binary, "beta-payload-0123456789", true, mask_key)?;
    defer alloc.free(w2);
    const w3 = encodeFrame(alloc, .Pong, "ok", true, mask_key)?;
    defer alloc.free(w3);

    let stream: std.ArrayList(u8) = .empty;
    defer stream.deinit(alloc);
    try stream.appendSlice(alloc, w1);
    try stream.appendSlice(alloc, w2);
    try stream.appendSlice(alloc, w3);

    let assembler = WsStreamAssembler.init(alloc, true, 4096);
    defer assembler.deinit();

    // Feed variable 1..7 byte chunks
    let pos: usize = 0;
    let step: usize = 1;
    while (pos < stream.items.len) {
        const end = @min(pos + step, stream.items.len);
        assembler.feed(stream.items[pos..end])?;
        pos = end;
        step = (step % 7) + 1;
    }

    expect_eq(assembler.pendingCount(), 3 as usize);

    let m1 = assembler.nextMessage().?;
    defer m1.deinit(alloc);
    expect_eq(m1.opcode == .Text, true);
    expect_eq(std.mem.eql(u8, m1.payload, "alpha"), true);

    let m2 = assembler.nextMessage().?;
    defer m2.deinit(alloc);
    expect_eq(m2.opcode == .Binary, true);
    expect_eq(std.mem.eql(u8, m2.payload, "beta-payload-0123456789"), true);

    let m3 = assembler.nextMessage().?;
    defer m3.deinit(alloc);
    expect_eq(m3.opcode == .Pong, true);
    expect_eq(std.mem.eql(u8, m3.payload, "ok"), true);

    expect_eq(assembler.nextMessage() == null, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // Multi-fragment UTF-8 Text message split mid-codepoint with interleaved Ping & Pong
    // "你好世界" in UTF-8: E4 BD A0 E5 A5 BD E4 B8 96 E7 95 8C (12 bytes)
    const utf8_msg = "你好世界";
    const frag1 = encodeFrame(alloc, .Text, utf8_msg[0..4], false, null)?;
    defer alloc.free(frag1);
    const ping = encodeFrame(alloc, .Ping, "hb-1", true, null)?;
    defer alloc.free(ping);
    const frag2 = encodeFrame(alloc, .Continuation, utf8_msg[4..8], false, null)?;
    defer alloc.free(frag2);
    const close_f = encodeCloseFrame(alloc, 1001, "going", null)?;
    defer alloc.free(close_f);
    const frag3 = encodeFrame(alloc, .Continuation, utf8_msg[8..12], true, null)?;
    defer alloc.free(frag3);

    let assembler = WsStreamAssembler.init(alloc, false, 4096);
    defer assembler.deinit();

    assembler.feed(frag1)?;
    expect_eq(assembler.pendingCount(), 0 as usize);

    // Interleaved Ping mid-fragment must be emitted immediately
    assembler.feed(ping)?;
    expect_eq(assembler.pendingCount(), 1 as usize);
    let p_msg = assembler.nextMessage().?;
    defer p_msg.deinit(alloc);
    expect_eq(p_msg.opcode == .Ping, true);
    expect_eq(std.mem.eql(u8, p_msg.payload, "hb-1"), true);

    assembler.feed(frag2)?;
    assembler.feed(close_f)?;
    expect_eq(assembler.pendingCount(), 1 as usize);
    let c_msg = assembler.nextMessage().?;
    defer c_msg.deinit(alloc);
    expect_eq(c_msg.opcode == .Close, true);
    expect_eq(c_msg.close_code.?, 1001 as u16);

    assembler.feed(frag3)?;
    expect_eq(assembler.pendingCount(), 1 as usize);
    let t_msg = assembler.nextMessage().?;
    defer t_msg.deinit(alloc);
    expect_eq(t_msg.opcode == .Text, true);
    expect_eq(std.mem.eql(u8, t_msg.payload, utf8_msg), true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. RSV bits set -> ReservedBitsSet
    const rsv_frame: [2]u8 = [0xC1, 0x00];
    let got_rsv = false;
    * = decodeFrame(alloc, &rsv_frame, false, 1024) catch e => err_rsv: {
        got_rsv = (e == WsError.ReservedBitsSet);
        break :err_rsv DecodedFrame{ frame: WsFrame{}, bytes_consumed: 0 };
    };
    expect_eq(got_rsv, true);

    // 2. Invalid opcode (0x3) -> InvalidOpcode
    const bad_op: [2]u8 = [0x83, 0x00];
    let got_op = false;
    * = decodeFrame(alloc, &bad_op, false, 1024) catch e => err_op: {
        got_op = (e == WsError.InvalidOpcode);
        break :err_op DecodedFrame{ frame: WsFrame{}, bytes_consumed: 0 };
    };
    expect_eq(got_op, true);

    // 3. Control frame too large (Ping len=126) -> ControlFrameTooLarge
    const big_ctrl: [4]u8 = [0x89, 126, 0x00, 126];
    let got_ctrl_big = false;
    * = decodeFrame(alloc, &big_ctrl, false, 1024) catch e => err_cb: {
        got_ctrl_big = (e == WsError.ControlFrameTooLarge);
        break :err_cb DecodedFrame{ frame: WsFrame{}, bytes_consumed: 0 };
    };
    expect_eq(got_ctrl_big, true);

    // 4. Fragmented control frame (Ping FIN=0) -> ControlFrameFragmented
    const frag_ctrl: [2]u8 = [0x09, 0x00];
    let got_ctrl_frag = false;
    * = decodeFrame(alloc, &frag_ctrl, false, 1024) catch e => err_cf: {
        got_ctrl_frag = (e == WsError.ControlFrameFragmented);
        break :err_cf DecodedFrame{ frame: WsFrame{}, bytes_consumed: 0 };
    };
    expect_eq(got_ctrl_frag, true);

    // 5. Unmasked client frame when require_masked=true -> UnmaskedClientFrame
    const unmasked = encodeFrame(alloc, .Text, "hi", true, null)?;
    defer alloc.free(unmasked);
    let got_unmasked = false;
    * = decodeFrame(alloc, unmasked, true, 1024) catch e => err_um: {
        got_unmasked = (e == WsError.UnmaskedClientFrame);
        break :err_um DecodedFrame{ frame: WsFrame{}, bytes_consumed: 0 };
    };
    expect_eq(got_unmasked, true);

    // 6. Invalid UTF-8 in Text frame -> InvalidUtf8
    const bad_utf8: [2]u8 = [0xFF, 0xFE];
    const bad_text = encodeFrame(alloc, .Text, &bad_utf8, true, null)?;
    defer alloc.free(bad_text);
    let got_utf8 = false;
    * = decodeFrame(alloc, bad_text, false, 1024) catch e => err_u8: {
        got_utf8 = (e == WsError.InvalidUtf8);
        break :err_u8 DecodedFrame{ frame: WsFrame{}, bytes_consumed: 0 };
    };
    expect_eq(got_utf8, true);

    // 7. UnexpectedContinuation & FragmentedInProgress in WsStreamAssembler
    let assembler = WsStreamAssembler.init(alloc, false, 1024);
    defer assembler.deinit();

    const cont = encodeFrame(alloc, .Continuation, "orphan", true, null)?;
    defer alloc.free(cont);
    let got_unexp_cont = false;
    * = assembler.feed(cont) catch e => err_uc: {
        got_unexp_cont = (e == WsError.UnexpectedContinuation);
        break :err_uc;
    };
    expect_eq(got_unexp_cont, true);

    const start_frag = encodeFrame(alloc, .Text, "part1", false, null)?;
    defer alloc.free(start_frag);
    assembler.feed(start_frag)?;

    const new_data = encodeFrame(alloc, .Binary, "illegal", true, null)?;
    defer alloc.free(new_data);
    let got_frag_in_prog = false;
    * = assembler.feed(new_data) catch e => err_fp: {
        got_frag_in_prog = (e == WsError.FragmentedInProgress);
        break :err_fp;
    };
    expect_eq(got_frag_in_prog, true);

    // 8. Invalid close code (1005) -> InvalidClosePayload
    const bad_close_wire: [4]u8 = [0x88, 0x02, 0x03, 0xED]; // 1005 = 0x03ED
    let got_bad_close = false;
    * = decodeFrame(alloc, &bad_close_wire, false, 1024) catch e => err_bc: {
        got_bad_close = (e == WsError.InvalidClosePayload);
        break :err_bc DecodedFrame{ frame: WsFrame{}, bytes_consumed: 0 };
    };
    expect_eq(got_bad_close, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mount: import cap from "cap"; cap.websocket_frame
    const accept = cap.websocket_frame.computeAcceptKey("dGhlIHNhbXBsZSBub25jZQ==");
    expect_eq(std.mem.eql(u8, &accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="), true);

    const mask_key: [4]u8 = [0xAA, 0xBB, 0xCC, 0xDD];
    const encoded = cap.websocket_frame.encodeFrame(alloc, .Text, "dynamic-hub-ok", true, mask_key)?;
    defer alloc.free(encoded);

    let assembler = cap.websocket_frame.WsStreamAssembler.init(alloc, true, 4096);
    defer assembler.deinit();
    assembler.feed(encoded)?;

    let msg = assembler.nextMessage().?;
    defer msg.deinit(alloc);
    expect_eq(msg.opcode == .Text, true);
    expect_eq(std.mem.eql(u8, msg.payload, "dynamic-hub-ok"), true);
}

test("cap:websocket_frame RFC 6455 Sec-WebSocket-Accept & 7/16/64-bit frame masking round-trip") {
    runTest1() catch unreachable;
}

test("cap:websocket_frame 1..7 byte partial chunks & multi-frame pipelining") {
    runTest2() catch unreachable;
}

test("cap:websocket_frame multi-fragment reassembly with interleaved Ping/Close control frames") {
    runTest3() catch unreachable;
}

test("cap:websocket_frame RSV/opcode/control-frame/mask/UTF-8/fragmentation security guards") {
    runTest4() catch unreachable;
}

test("cap.websocket_frame dynamic mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
