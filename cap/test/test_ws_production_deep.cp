// ============================================================================
// cap/test/test_ws_production_deep.cp
// cap:ws 工业级 RFC 6455 / RFC 7692 WebSocket 协议栈深度回归测试套件
// 覆盖:
//   1. SubprotocolNegotiator 大小写精确协商、WsExtensionHeader 解析/序列化、CloseCodeValidator RFC 6455 §7.4.1 校验
//   2. PerMessageDeflate RFC 7692 协商、0x00 0x00 0xFF 0xFF 剥离/补回与压缩/解压往返及超限防护
//   3. WsSession 状态机（Connecting->Open->Closing->Closed、分片重组、控制帧交错、Auto-Pong、Close 握手）
//   4. WsSession 安全与资源耗尽防护（客户端/服务端掩码强制、max_frame_bytes、max_message_bytes、RSV 校验与压缩分片）
//   5. cap:ws 全量导出兼容性与既有帧/握手/重组 API 零回归
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { deflateRaw, inflateRaw } from "cap:pack";
import {
    // New production modules
    SubprotocolList,
    SubprotocolNegotiator,
    WsExtensionHeader,
    WsExtensionList,
    CloseCodeValidator,
    PMD_SYNC_TRAILER,
    PerMessageDeflateConfig,
    PerMessageDeflateNegotiation,
    PerMessageDeflate,
    WsSessionState,
    WsSessionConfig,
    WsSessionResult,
    WsSession,
    // Existing modules (100% backward compatibility)
    Opcode,
    CloseCode,
    Mask,
    FrameHeader,
    FrameBuilder,
    ClosePayload,
    WS_MAGIC_GUID,
    WebSocketUpgradeRequest,
    WebSocketUpgradeResponse,
    WebSocketHandshake,
    WebSocketMessageKind,
    WebSocketMessage,
    AssembleResult,
    WebSocketMessageAssembler,
    ws,
} from "cap:ws";

fn testSubprotocolExtClose(): !void {
    // Subprotocol parsing & exact case-sensitive selection (RFC 6455 §11.3.4)
    let offered = try SubprotocolNegotiator.parseOffered("  Chat , superchat , graphql-transport-ws ");
    expect_eq(offered.count(), 3);
    expect_eq(std.mem.eql(u8, offered.get(0).?, "Chat"), true);
    expect_eq(std.mem.eql(u8, offered.get(1).?, "superchat"), true);
    expect_eq(std.mem.eql(u8, offered.get(2).?, "graphql-transport-ws"), true);

    const supported: [2][]const u8 = ["chat", "superchat"];
    // "Chat" != "chat" (case-sensitive), so "superchat" is selected
    const chosen = SubprotocolNegotiator.select(offered.asSlice(), &supported);
    expect_eq(chosen != null, true);
    expect_eq(std.mem.eql(u8, chosen.?, "superchat"), true);

    const only_upper: [1][]const u8 = ["CHAT"];
    const lower_supported: [1][]const u8 = ["chat"];
    expect_eq(SubprotocolNegotiator.select(&only_upper, &lower_supported) == null, true);

    // Reject invalid / duplicate subprotocols
    let dup_failed = false;
    * = SubprotocolNegotiator.parseOffered("chat, chat") catch {
        dup_failed = true;
    };
    expect_eq(dup_failed, true);

    let bad_tok_failed = false;
    * = SubprotocolNegotiator.parseOffered("chat/v1, ok") catch {
        bad_tok_failed = true;
    };
    expect_eq(bad_tok_failed, true);

    // WsExtensionHeader parse & serialize
    let ext = try WsExtensionHeader.parse("permessage-deflate; client_max_window_bits=15; server_no_context_takeover");
    expect_eq(std.mem.eql(u8, ext.name, "permessage-deflate"), true);
    expect_eq(ext.hasParam("client_max_window_bits"), true);
    expect_eq(ext.getParamU8("client_max_window_bits").?, 15 as u8);
    expect_eq(ext.hasParam("server_no_context_takeover"), true);
    expect_eq(ext.getParam("server_no_context_takeover") == null, true);

    let ser_buf: [128]u8 = undefined;
    const ser = try ext.serialize(&ser_buf);
    expect_eq(
        std.mem.eql(u8, ser, "permessage-deflate; client_max_window_bits=15; server_no_context_takeover"),
        true,
    );

    let multi = try WsExtensionHeader.parseAll("x-custom; mode=\"fast,safe\", permessage-deflate; client_max_window_bits");
    expect_eq(multi.count(), 2);
    expect_eq(std.mem.eql(u8, multi.get(0).?.getParam("mode").?, "fast,safe"), true);
    expect_eq(std.mem.eql(u8, multi.get(1).?.name, "permessage-deflate"), true);

    // CloseCodeValidator (RFC 6455 §7.4.1)
    expect_eq(CloseCodeValidator.isValidWireCloseCode(0), false);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(999), false);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1000), true);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1003), true);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1004), false);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1005), false);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1006), false);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1007), true);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1011), true);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1014), true);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1015), false);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(1016), false);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(2999), false);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(3000), true);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(4999), true);
    expect_eq(CloseCodeValidator.isValidWireCloseCode(5000), false);

    expect_eq(CloseCodeValidator.isValidUtf8("Normal shutdown 正常关闭"), true);
    const bad_utf8: [2]u8 = [0xFF as u8, 0xFE as u8];
    expect_eq(CloseCodeValidator.isValidUtf8(&bad_utf8), false);
    const trunc_utf8: [2]u8 = [0xE4 as u8, 0xB8 as u8];
    expect_eq(CloseCodeValidator.isValidUtf8(&trunc_utf8), false);
}

fn testPerMessageDeflate(): !void {
    // Negotiate with valid client header
    let neg = try PerMessageDeflate.negotiate("permessage-deflate; client_max_window_bits=12; server_no_context_takeover");
    expect_eq(neg.accepted, true);
    expect_eq(neg.config.client_max_window_bits, 12 as u8);
    expect_eq(neg.config.server_no_context_takeover, true);
    const resp_hdr = neg.responseHeader();
    expect_eq(std.mem.indexOf(u8, resp_hdr, "permessage-deflate") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_hdr, "server_no_context_takeover") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_hdr, "client_max_window_bits=12") != null, true);

    // Fallback to second valid offer when first offer has unknown parameter
    let neg2 = try PerMessageDeflate.negotiate("permessage-deflate; unknown_param=1, permessage-deflate; client_no_context_takeover");
    expect_eq(neg2.accepted, true);
    expect_eq(neg2.config.client_no_context_takeover, true);

    // Reject invalid window bits
    let neg_bad = try PerMessageDeflate.negotiate("permessage-deflate; client_max_window_bits=7");
    expect_eq(neg_bad.accepted, false);

    // RFC 7692 §7.2.3.1 exact test vector for "Hello" -> f2 48 cd c9 c9 07 00
    const comp_hello = try PerMessageDeflate.compressPayload("Hello");
    expect_eq(PerMessageDeflate.hasSyncTrailer(comp_hello), false);
    expect_eq(comp_hello.len, 7);
    expect_eq(comp_hello[0], 0xf2 as u8);
    expect_eq(comp_hello[6], 0x00 as u8);

    const decomp_hello = try PerMessageDeflate.decompressPayload(comp_hello);
    expect_eq(std.mem.eql(u8, decomp_hello, "Hello"), true);

    // Cross-verify cap:pack deflateRaw -> PerMessageDeflate.decompressPayload compatibility
    const raw_hello = deflateRaw("Hello", 6);
    const decomp_raw = try PerMessageDeflate.decompressPayload(raw_hello);
    expect_eq(std.mem.eql(u8, decomp_raw, "Hello"), true);
    * = inflateRaw(raw_hello);

    // Empty payload roundtrip (RFC 7692 §7.2.3.6)
    const comp_empty = try PerMessageDeflate.compressPayload("");
    expect_eq(comp_empty.len, 1);
    expect_eq(comp_empty[0], 0x00 as u8);
    const decomp_empty = try PerMessageDeflate.decompressPayload(comp_empty);
    expect_eq(decomp_empty.len, 0);

    // Stateful codec + larger payload + max_bytes guard
    let pmd = try PerMessageDeflate.init(PerMessageDeflateConfig{
        server_no_context_takeover: true,
        client_no_context_takeover: false,
        compression_threshold: 32,
    });
    defer pmd.deinit();

    const big_text = "RFC 7692 permessage-deflate compression test! RFC 7692 permessage-deflate compression test! RFC 7692 permessage-deflate compression test!";
    expect_eq(pmd.config.shouldCompress(big_text.len), true);
    expect_eq(pmd.config.shouldCompress(16), false);

    const c1 = try pmd.compress(big_text);
    expect_eq(c1.len < big_text.len, true);
    const d1 = try pmd.decompress(c1);
    expect_eq(std.mem.eql(u8, d1, big_text), true);

    // Decompression limit guard
    let too_big_err = false;
    * = PerMessageDeflate.decompressPayload(c1, 32) catch e => {
        if (e == @as(anyerror, error.MessageTooBig)) {
            too_big_err = true;
        }
    };
    expect_eq(too_big_err, true);
    expect_eq(PMD_SYNC_TRAILER[2], 0xFF as u8);
}

fn testWsSessionStateMachine(): !void {
    const allocator: std.mem.Allocator = std.heap.page_allocator;
    let session = WsSession.init(allocator, WsSessionConfig{
        is_server: true,
        max_message_bytes: 4096,
        max_frame_bytes: 1024,
        auto_pong: true,
        heartbeat_interval_ms: 5000,
    });
    defer session.deinit();

    expect_eq(session.getState(), WsSessionState.Connecting);

    // Reject frames before Open
    let f_buf: [128]u8 = undefined;
    const client_mask: [4]u8 = [0x11 as u8, 0x22 as u8, 0x33 as u8, 0x44 as u8];
    const early_n = try FrameBuilder.buildText(&f_buf, "early", client_mask);
    let connecting_err = false;
    * = session.feedBytes(f_buf[0..early_n]) catch e => {
        if (e == @as(anyerror, error.InvalidSessionState)) connecting_err = true;
    };
    expect_eq(connecting_err, true);

    try session.markOpen();
    expect_eq(session.isOpen(), true);

    // Fragment 1 (Text, FIN=false) + interleaved Ping + Fragment 2 (Continue, FIN=true)
    let wire: [512]u8 = undefined;
    const n1 = try FrameBuilder.buildFragment(wire[0..], false, .Text, "Hello, ", client_mask);
    const n2 = try FrameBuilder.buildPing(wire[n1..], "ping-42", client_mask);
    const n3 = try FrameBuilder.buildContinue(wire[n1 + n2 ..], true, "WebSocket!", client_mask);
    const total = n1 + n2 + n3;

    let pos: usize = 0;
    const r1 = try session.feedBytes(wire[pos..total]);
    expect_eq(r1.bytes_consumed, n1);
    expect_eq(r1.message == null, true);
    pos += r1.bytes_consumed;

    const r2 = try session.feedBytes(wire[pos..total]);
    expect_eq(r2.bytes_consumed, n2);
    expect_eq(r2.message.?.kind, WebSocketMessageKind.Ping);
    expect_eq(std.mem.eql(u8, r2.message.?.data, "ping-42"), true);
    expect_eq(r2.needs_pong, true);
    pos += r2.bytes_consumed;

    let pong_buf: [128]u8 = undefined;
    const pong_n = try session.takePendingPongFrame(&pong_buf, null);
    expect_eq(pong_n > 2, true);
    const pong_hdr = try FrameHeader.decode(pong_buf[0..pong_n]);
    expect_eq(pong_hdr.opcode, Opcode.Pong);
    expect_eq(std.mem.eql(u8, pong_buf[pong_hdr.header_len .. pong_n], "ping-42"), true);
    expect_eq(session.hasPendingPong(), false);

    const r3 = try session.feedBytes(wire[pos..total]);
    expect_eq(r3.bytes_consumed, n3);
    expect_eq(r3.message.?.kind, WebSocketMessageKind.Text);
    expect_eq(std.mem.eql(u8, r3.message.?.data, "Hello, WebSocket!"), true);

    // Heartbeat timing
    session.recordActivity(1000);
    expect_eq(session.shouldSendHeartbeat(5999), false);
    expect_eq(session.shouldSendHeartbeat(6000), true);

    // Peer-initiated Close handshake: Open -> Closing -> Closed
    let close_in: [128]u8 = undefined;
    const close_n = try FrameBuilder.buildClose(&close_in, .NormalClosure, "bye", client_mask);
    const rc = try session.feedBytes(close_in[0..close_n]);
    expect_eq(rc.message.?.kind, WebSocketMessageKind.Close);
    expect_eq(rc.message.?.close_code, 1000 as u16);
    expect_eq(std.mem.eql(u8, rc.message.?.close_reason, "bye"), true);
    expect_eq(rc.state, WsSessionState.Closing);
    expect_eq(rc.needs_close_reply, true);

    let close_reply: [128]u8 = undefined;
    const cr_n = try session.takePendingCloseFrame(&close_reply, null);
    expect_eq(cr_n, 4);
    expect_eq(session.isClosed(), true);

    // Further data rejected when Closed
    let closed_err = false;
    * = session.feedBytes(f_buf[0..early_n]) catch e => {
        if (e == @as(anyerror, error.SessionClosed)) closed_err = true;
    };
    expect_eq(closed_err, true);
}

fn testWsSessionSecurityAndDeflate(): !void {
    const allocator: std.mem.Allocator = std.heap.page_allocator;

    // 4a. Server rejects unmasked client frame
    let srv = WsSession.initOpen(allocator, WsSessionConfig{
        is_server: true,
        max_message_bytes: 64,
        max_frame_bytes: 32,
    });
    defer srv.deinit();

    let buf: [256]u8 = undefined;
    const unmasked_n = try FrameBuilder.buildText(&buf, "hi", null);
    let unmasked_err = false;
    * = srv.feedBytes(buf[0..unmasked_n]) catch e => {
        if (e == @as(anyerror, error.UnmaskedClientFrame)) unmasked_err = true;
    };
    expect_eq(unmasked_err, true);

    // 4b. Client rejects masked server frame
    let cli = WsSession.initOpen(allocator, WsSessionConfig{
        is_server: false,
        max_message_bytes: 64,
        max_frame_bytes: 32,
    });
    defer cli.deinit();

    const srv_mask: [4]u8 = [0xAA as u8, 0xBB as u8, 0xCC as u8, 0xDD as u8];
    const masked_n = try FrameBuilder.buildText(&buf, "hi", srv_mask);
    let masked_err = false;
    * = cli.feedBytes(buf[0..masked_n]) catch e => {
        if (e == @as(anyerror, error.MaskedServerFrame)) masked_err = true;
    };
    expect_eq(masked_err, true);

    // 4c. Reject oversized frame header before full payload arrives
    const big_hdr = FrameHeader{
        .fin = true,
        .opcode = .Binary,
        .masked = true,
        .payload_len = 33, // > max_frame_bytes (32)
        .mask_key = srv_mask,
    };
    const big_hdr_n = try big_hdr.encode(&buf);
    let frame_too_large = false;
    * = srv.feedBytes(buf[0..big_hdr_n]) catch e => {
        if (e == @as(anyerror, error.FrameTooLarge)) frame_too_large = true;
    };
    expect_eq(frame_too_large, true);

    // 4d. Reject fragmented message exceeding max_message_bytes (64)
    const c1_n = try FrameBuilder.buildFragment(&buf, false, .Binary, "012345678901234567890123456789", srv_mask); // 30
    const fr1 = try srv.feedBytes(buf[0..c1_n]);
    expect_eq(fr1.message == null, true);
    const c2_n = try FrameBuilder.buildContinue(&buf, false, "012345678901234567890123456789", srv_mask); // +30 = 60
    const fr2 = try srv.feedBytes(buf[0..c2_n]);
    expect_eq(fr2.message == null, true);
    const c3_n = try FrameBuilder.buildContinue(&buf, true, "012345", srv_mask); // +6 = 66 > 64
    let msg_too_big = false;
    * = srv.feedBytes(buf[0..c3_n]) catch e => {
        if (e == @as(anyerror, error.MessageTooBig)) msg_too_big = true;
    };
    expect_eq(msg_too_big, true);

    // 4e. RSV1 compressed fragmented message in WsSession with permessage_deflate=true
    let comp_srv = WsSession.initOpen(allocator, WsSessionConfig{
        is_server: true,
        permessage_deflate: true,
        max_message_bytes: 4096,
        max_frame_bytes: 1024,
    });
    defer comp_srv.deinit();

    const plain_msg = "Compressed fragmented UTF-8 WebSocket message: 你好 CP!";
    const deflated = try PerMessageDeflate.compressPayload(plain_msg);
    const split_idx = deflated.len / 2;

    // First fragment: rsv1 = true, opcode = .Text, fin = false
    let fh1 = FrameHeader{
        .fin = false,
        .rsv1 = true,
        .opcode = .Text,
        .masked = true,
        .payload_len = split_idx,
        .mask_key = srv_mask,
    };
    let f1_wire: [256]u8 = undefined;
    const f1_hlen = try fh1.encode(&f1_wire);
    @memcpy(f1_wire[f1_hlen .. f1_hlen + split_idx], deflated[0..split_idx]);
    Mask.apply(f1_wire[f1_hlen .. f1_hlen + split_idx], srv_mask);

    const res1 = try comp_srv.feedBytes(f1_wire[0 .. f1_hlen + split_idx]);
    expect_eq(res1.message == null, true);

    // Second fragment: rsv1 = false, opcode = .Continue, fin = true
    let f2_wire: [256]u8 = undefined;
    const f2_n = try FrameBuilder.buildContinue(&f2_wire, true, deflated[split_idx..], srv_mask);
    const res2 = try comp_srv.feedBytes(f2_wire[0..f2_n]);
    expect_eq(res2.compressed, true);
    expect_eq(res2.message.?.kind, WebSocketMessageKind.Text);
    expect_eq(std.mem.eql(u8, res2.message.?.data, plain_msg), true);

    // Active Close handshake: Open -> Closing -> Closed
    let out_close: [64]u8 = undefined;
    const out_close_n = try comp_srv.initiateClose(&out_close, 1001, "going away", null);
    expect_eq(out_close_n, 2 + 12);
    expect_eq(comp_srv.getState(), WsSessionState.Closing);

    let peer_echo: [64]u8 = undefined;
    const peer_echo_n = try FrameBuilder.buildClose(&peer_echo, .GoingAway, "", srv_mask);
    const echo_res = try comp_srv.feedBytes(peer_echo[0..peer_echo_n]);
    expect_eq(echo_res.state, WsSessionState.Closed);
    expect_eq(echo_res.needs_close_reply, false);
}

fn testBackwardCompat(): !void {
    const allocator: std.mem.Allocator = std.heap.page_allocator;
    expect_eq(std.mem.eql(u8, WS_MAGIC_GUID, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"), true);

    // RFC 6455 §1.3 canonical Sec-WebSocket-Accept vector
    let acc_buf: [32]u8 = undefined;
    const accept = try ws.computeAcceptKey("dGhlIHNhbXBsZSBub25jZQ==", &acc_buf);
    expect_eq(std.mem.eql(u8, accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="), true);

    // Handshake request/response validation
    let req_buf: [512]u8 = undefined;
    const req = try WebSocketHandshake.buildClientUpgradeRequest(
        &req_buf,
        "example.com",
        "/ws",
        "dGhlIHNhbXBsZSBub25jZQ==",
        "chat, superchat",
    );
    const parsed_req = try WebSocketHandshake.validateClientUpgradeRequest(req);
    expect_eq(std.mem.eql(u8, parsed_req.path, "/ws"), true);

    const supported: [1][]const u8 = ["superchat"];
    const selected = try SubprotocolNegotiator.selectFromHeader(parsed_req.protocol, &supported);
    expect_eq(std.mem.eql(u8, selected.?, "superchat"), true);

    let resp_buf: [512]u8 = undefined;
    const resp = try WebSocketHandshake.buildServerUpgradeResponse(
        &resp_buf,
        parsed_req.key,
        selected,
    );
    const parsed_resp = try WebSocketHandshake.validateServerUpgradeResponse(resp, "dGhlIHNhbXBsZSBub25jZQ==");
    expect_eq(std.mem.eql(u8, parsed_resp.protocol, "superchat"), true);

    // Existing WebSocketMessageAssembler & ClosePayload compatibility
    let asm_inst = WebSocketMessageAssembler.init(allocator, 1024);
    defer asm_inst.deinit();
    let frame_buf: [128]u8 = undefined;
    const fn1 = try ws.buildText(&frame_buf, "legacy-ok", null);
    const ar1 = try asm_inst.feedBytes(frame_buf[0..fn1]);
    expect_eq(ar1.message.?.kind, WebSocketMessageKind.Text);
    expect_eq(std.mem.eql(u8, ar1.message.?.data, "legacy-ok"), true);

    const raw_cp: [4]u8 = [0x03 as u8, 0xE8 as u8, 'o' as u8, 'k' as u8];
    const cp = try ClosePayload.parse(&raw_cp);
    expect_eq(cp.code, @intFromEnum(CloseCode.NormalClosure));
    expect_eq(std.mem.eql(u8, cp.reason, "ok"), true);
}

fn main() {
    test("1. SubprotocolNegotiator, WsExtensionHeader & CloseCodeValidator (RFC 6455)", () => {
        try testSubprotocolExtClose();
    });
    test("2. RFC 7692 permessage-deflate negotiation, trailer strip/restore & lossless roundtrip", () => {
        try testPerMessageDeflate();
    });
    test("3. WsSession state machine: fragments, interleaved Ping/Auto-Pong & Close handshake", () => {
        try testWsSessionStateMachine();
    });
    test("4. WsSession security bounds: masking enforcement, max_frame_bytes, max_message_bytes & RSV1 deflate", () => {
        try testWsSessionSecurityAndDeflate();
    });
    test("5. cap:ws aggregate export compatibility & existing handshake/assembler zero regression", () => {
        try testBackwardCompat();
    });
}
