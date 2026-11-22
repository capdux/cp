// Source: cp/cap/test/test_msgpack_rpc_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RpcError,
    RpcValue,
    RpcMapEntry,
    RpcRequest,
    RpcResponse,
    RpcNotification,
    RpcMessage,
    encodeRpcMessage,
    decodeRpcMessage,
    RpcStreamDecoder,
} from "cap:web";

test("1. MessagePack-RPC Request, Response and Notification roundtrip") {
    const alloc = std.testing.allocator;
    let params: [2]RpcValue = [ RpcValue{ .str_val = "hello" }, RpcValue{ .int_val = -42 } ];
    const req = RpcMessage{ .request = RpcRequest{ .msgid = 1001, .method = "math.add", .params = &params } };
    const req_bytes = try encodeRpcMessage(alloc, req);
    defer alloc.free(req_bytes);

    const dec_req = try decodeRpcMessage(alloc, req_bytes);
    defer dec_req.message.deinit(alloc);
    expect_eq(dec_req.message.request.msgid, 1001);
    try std.testing.expectEqualStrings("math.add", dec_req.message.request.method);
    expect_eq(dec_req.message.request.params[1].int_val, -42);

    let map_entries: [1]RpcMapEntry = [ RpcMapEntry{ .key = RpcValue{ .str_val = "ok" }, .value = RpcValue{ .bool_val = true } } ];
    const resp = RpcMessage{ .response = RpcResponse{
        .msgid = 1001,
        .err = RpcValue{ .nil = {} },
        .result = RpcValue{ .map_val = &map_entries },
    } };
    const resp_bytes = try encodeRpcMessage(alloc, resp);
    defer alloc.free(resp_bytes);

    const dec_resp = try decodeRpcMessage(alloc, resp_bytes);
    defer dec_resp.message.deinit(alloc);
    expect_eq(dec_resp.message.response.msgid, 1001);
    expect_eq(dec_resp.message.response.result.map_val[0].value.bool_val, true);
}

test("2. RpcStreamDecoder 1..5 byte chunked half-packet and sticky-packet decoding") {
    const alloc = std.testing.allocator;
    let p1: [1]RpcValue = [ RpcValue{ .uint_val = 7 } ];
    const m1 = try encodeRpcMessage(alloc, RpcMessage{ .notification = RpcNotification{ .method = "ping", .params = &p1 } });
    defer alloc.free(m1);
    const m2 = try encodeRpcMessage(alloc, RpcMessage{ .notification = RpcNotification{ .method = "pong", .params = &p1 } });
    defer alloc.free(m2);

    let combined: std.ArrayList(u8) = .empty;
    defer combined.deinit(alloc);
    try combined.appendSlice(alloc, m1);
    try combined.appendSlice(alloc, m2);

    let framer = RpcStreamDecoder.init(alloc, 4096);
    defer framer.deinit();

    let count: usize = 0;
    let idx: usize = 0;
    let step: usize = 1;
    while (idx < combined.items.len) {
        const end = @min(idx + step, combined.items.len);
        try framer.feed(combined.items[idx..end]);
        idx = end;
        step = (step % 5) + 1;
        while (msg := try framer.next()) {
            defer msg.deinit(alloc);
            count += 1;
        }
    }
    expect_eq(count, 2);
}

test("3. Binary, Float64 and nested Array/Map RpcValue roundtrip") {
    const alloc = std.testing.allocator;
    let bin: [4]u8 = [ 0xDE, 0xAD, 0xBE, 0xEF ];
    let arr: [2]RpcValue = [ RpcValue{ .bin_val = &bin }, RpcValue{ .float_val = 3.125 } ];
    let params: [1]RpcValue = [ RpcValue{ .array_val = &arr } ];
    const raw = try encodeRpcMessage(alloc, RpcMessage{ .request = RpcRequest{ .msgid = 7, .method = "echo", .params = &params } });
    defer alloc.free(raw);

    const out = try decodeRpcMessage(alloc, raw);
    defer out.message.deinit(alloc);
    try std.testing.expectEqualSlices(u8, &bin, out.message.request.params[0].array_val[0].bin_val);
    expect_eq(out.message.request.params[0].array_val[1].float_val, 3.125);
}

test("4. Malformed frame, invalid msg_type and MessageTooLarge protection") {
    const alloc = std.testing.allocator;
    // Unknown msg_type 99
    let bad_type: [4]u8 = [ 0x93, 0x63, 0xA1, 'a' ];
    try std.testing.expectError(RpcError.InvalidMsgType, decodeRpcMessage(alloc, &bad_type));

    // Not an array
    let not_arr: [1]u8 = [ 0xC0 ];
    try std.testing.expectError(RpcError.InvalidFrame, decodeRpcMessage(alloc, &not_arr));

    // Max bytes limit in stream decoder
    let framer = RpcStreamDecoder.init(alloc, 4);
    defer framer.deinit();
    let too_big: [5]u8 = [ 1, 2, 3, 4, 5 ];
    try std.testing.expectError(RpcError.MessageTooLarge, framer.feed(&too_big));
}

test("5. Dynamic cap.msgpack_rpc namespace and zero-leak check") {
    const alloc = std.testing.allocator;
    let p: [1]cap.web.RpcValue = [ cap.web.RpcValue{ .bool_val = true } ];
    const raw = try cap.web.encodeRpcMessage(alloc, cap.web.RpcMessage{
        .notification = cap.web.RpcNotification{ .method = "ready", .params = &p },
    });
    defer alloc.free(raw);
    const decoded = try cap.web.decodeRpcMessage(alloc, raw);
    defer decoded.message.deinit(alloc);
    try std.testing.expectEqualStrings("ready", decoded.message.notification.method);
}
