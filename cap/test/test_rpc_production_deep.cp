// Source: cp/cap/test/test_rpc_production_deep.cp
// Deep Production Test Suite for cap:rpc
// Covers:
//   1. JSON-RPC 2.0 request/notification/success/standard error codes encode & decode round-trip
//   2. ContentLengthFramer arbitrary chunk splits, multi-frame coalescing & max_frame_bytes DoS guard
//   3. LineRpcFramer stdio incremental NDJSON framing & invalid-line protection
//   4. McpServer full lifecycle: initialize -> notifications/initialized -> ping -> tools/list -> tools/call -> resources/list -> -32601
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.rpc`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { JsonParser } from "cap:json";
import {
    ERR_PARSE,
    ERR_INVALID_REQUEST,
    ERR_METHOD_NOT_FOUND,
    ERR_INVALID_PARAMS,
    ERR_INTERNAL,
    JsonRpcCodecError,
    JsonRpcId,
    JsonRpcError,
    JsonRpcRequest,
    JsonRpcResponse,
    JsonRpcCodec,
    FramingError,
    ContentLengthFramer,
    LineRpcFramer,
    McpToolSpec,
    McpResourceSpec,
    McpServerInfo,
    McpServer,
} from "cap:rpc";
import cap from "cap";

fn sumTool(alloc: std.mem.Allocator, args_json: []const u8): ([]const u8)! {
    const val = JsonParser.parse(alloc, args_json)?;
    const a = val.get("a")?.asNumber() ?? 0.0;
    const b = val.get("b")?.asNumber() ?? 0.0;
    const total = (a + b) as i64;
    let out: []u8 = [];
    zig {
        out = try std.fmt.allocPrint(alloc, "{d}", .{total});
    }
    return out;
}

fn failingTool(alloc: std.mem.Allocator, args_json: []const u8): ([]const u8)! {
    * = alloc;
    * = args_json;
    return err.ToolExecutionDenied;
}

fn testJsonRpcCodecRoundTrip()! {
    const alloc = std.heap.page_allocator;

    // 1. Standard error code constants
    expect_eq(ERR_PARSE, -32700 as i32);
    expect_eq(ERR_INVALID_REQUEST, -32600 as i32);
    expect_eq(ERR_METHOD_NOT_FOUND, -32601 as i32);
    expect_eq(ERR_INVALID_PARAMS, -32602 as i32);
    expect_eq(ERR_INTERNAL, -32603 as i32);

    // 2. Int-ID Request encode/decode round-trip
    const req1 = JsonRpcRequest{
        id: JsonRpcId.Int(42),
        method: "tools/call",
        params_json: "{\"name\":\"sum\",\"arguments\":{\"a\":10,\"b\":20}}",
        has_id: true,
    };
    expect_eq(req1.isNotification(), false);
    const wire1 = JsonRpcCodec.encodeRequest(alloc, req1) catch unreachable;
    defer alloc.free(wire1);

    let parsed1 = JsonRpcCodec.parseRequest(alloc, wire1) catch unreachable;
    defer parsed1.deinit(alloc);
    expect_eq(parsed1.isNotification(), false);
    expect_eq(parsed1.id.asInt().?, 42 as i64);
    expect_eq(std.mem.eql(u8, parsed1.method, "tools/call"), true);

    const p1_val = JsonParser.parse(alloc, parsed1.params_json) catch unreachable;
    expect_eq(std.mem.eql(u8, p1_val.get("name").?.asString().?, "sum"), true);

    // 3. Str-ID Request encode/decode round-trip
    const req2 = JsonRpcRequest{
        id: JsonRpcId.Str("req-uuid-1"),
        method: "ping",
        params_json: "",
        has_id: true,
    };
    const wire2 = JsonRpcCodec.encodeRequest(alloc, req2) catch unreachable;
    defer alloc.free(wire2);
    let parsed2 = JsonRpcCodec.parseRequest(alloc, wire2) catch unreachable;
    defer parsed2.deinit(alloc);
    expect_eq(std.mem.eql(u8, parsed2.id.asStr().?, "req-uuid-1"), true);
    expect_eq(std.mem.eql(u8, parsed2.method, "ping"), true);
    expect_eq(parsed2.params_json.len, 0 as usize);

    // 4. Notification (omitted id)
    const notif = JsonRpcRequest{
        id: JsonRpcId.Null,
        method: "notifications/initialized",
        params_json: "{\"ready\":true}",
    };
    expect_eq(notif.isNotification(), true);
    const wire_notif = JsonRpcCodec.encodeRequest(alloc, notif) catch unreachable;
    defer alloc.free(wire_notif);
    expect_eq(std.mem.indexOf(u8, wire_notif, "\"id\"") == null, true);

    let parsed_notif = JsonRpcCodec.parseRequest(alloc, wire_notif) catch unreachable;
    defer parsed_notif.deinit(alloc);
    expect_eq(parsed_notif.isNotification(), true);
    expect_eq(std.mem.eql(u8, parsed_notif.method, "notifications/initialized"), true);

    // 5. Success response round-trip
    const ok_wire = JsonRpcCodec.encodeSuccess(alloc, JsonRpcId.Int(7), "{\"status\":\"ok\",\"count\":3}") catch unreachable;
    defer alloc.free(ok_wire);
    let ok_resp = JsonRpcCodec.parseResponse(alloc, ok_wire) catch unreachable;
    defer ok_resp.deinit(alloc);
    expect_eq(ok_resp.isSuccess(), true);
    expect_eq(ok_resp.isError(), false);
    expect_eq(ok_resp.id.asInt().?, 7 as i64);
    const res_dom = JsonParser.parse(alloc, ok_resp.result_json.?) catch unreachable;
    expect_eq(std.mem.eql(u8, res_dom.get("status").?.asString().?, "ok"), true);
    expect_eq(res_dom.get("count").?.asNumber().?, 3.0);

    // 6. Error response round-trip with data_json
    const err_obj = JsonRpcError.init(ERR_INVALID_PARAMS, "Missing field", "{\"field\":\"name\"}");
    const err_wire = JsonRpcCodec.encodeError(alloc, JsonRpcId.Str("e1"), err_obj) catch unreachable;
    defer alloc.free(err_wire);
    let err_resp = JsonRpcCodec.parseResponse(alloc, err_wire) catch unreachable;
    defer err_resp.deinit(alloc);
    expect_eq(err_resp.isSuccess(), false);
    expect_eq(err_resp.isError(), true);
    expect_eq(std.mem.eql(u8, err_resp.id.asStr().?, "e1"), true);
    expect_eq(err_resp.rpc_error.?.code, ERR_INVALID_PARAMS);
    expect_eq(std.mem.eql(u8, err_resp.rpc_error.?.message, "Missing field"), true);
    const data_dom = JsonParser.parse(alloc, err_resp.rpc_error.?.data_json.?) catch unreachable;
    expect_eq(std.mem.eql(u8, data_dom.get("field").?.asString().?, "name"), true);
}

fn testContentLengthFramer()! {
    const alloc = std.heap.page_allocator;

    const p1 = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}";
    const p2 = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}";

    const f1 = ContentLengthFramer.encodeFrame(alloc, p1) catch unreachable;
    defer alloc.free(f1);
    const f2 = ContentLengthFramer.encodeFrame(alloc, p2) catch unreachable;
    defer alloc.free(f2);

    // Build coalesced sticky packet with case-insensitive header & extra header
    const custom_hdr_frame = "content-type: application/vscode-jsonrpc; charset=utf-8\r\nCoNtEnT-LeNgTh: 40\r\n\r\n{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}";
    expect_eq(p1.len, 40 as usize);

    let framer = ContentLengthFramer.init(alloc, 1024);
    defer framer.deinit();

    // Feed 1 byte at a time across header, \r\n\r\n, and payload
    let collected_count: usize = 0;
    let i: usize = 0;
    while (i < custom_hdr_frame.len) : (i += 1) {
        const out = framer.feed(custom_hdr_frame[i .. i + 1]) catch unreachable;
        defer ContentLengthFramer.freeFrames(alloc, out);
        if (i + 1 < custom_hdr_frame.len) {
            expect_eq(out.len, 0 as usize);
        } else {
            expect_eq(out.len, 1 as usize);
            expect_eq(std.mem.eql(u8, out[0], p1), true);
            collected_count += 1;
        }
    }
    expect_eq(collected_count, 1 as usize);
    expect_eq(framer.bufferedBytes(), 0 as usize);

    // Sticky packets: two frames concatenated and cut mid-way
    let combined = alloc.alloc(u8, f1.len + f2.len) catch unreachable;
    defer alloc.free(combined);
    std.mem.copyForwards(u8, combined[0..f1.len], f1);
    std.mem.copyForwards(u8, combined[f1.len .. f1.len + f2.len], f2);

    const split_at = f1.len + 10;
    const part1 = framer.feed(combined[0..split_at]) catch unreachable;
    defer ContentLengthFramer.freeFrames(alloc, part1);
    expect_eq(part1.len, 1 as usize);
    expect_eq(std.mem.eql(u8, part1[0], p1), true);
    expect_eq(framer.bufferedBytes(), 10 as usize);

    const part2 = framer.feed(combined[split_at..]) catch unreachable;
    defer ContentLengthFramer.freeFrames(alloc, part2);
    expect_eq(part2.len, 1 as usize);
    expect_eq(std.mem.eql(u8, part2[0], p2), true);
    expect_eq(framer.bufferedBytes(), 0 as usize);

    // All frames in a single chunk
    const both = framer.feed(combined) catch unreachable;
    defer ContentLengthFramer.freeFrames(alloc, both);
    expect_eq(both.len, 2 as usize);
    expect_eq(std.mem.eql(u8, both[0], p1), true);
    expect_eq(std.mem.eql(u8, both[1], p2), true);

    // DoS protection: reject over-limit frame BEFORE buffering full body
    let small_framer = ContentLengthFramer.init(alloc, 32);
    defer small_framer.deinit();
    let too_big_hit = false;
    * = small_framer.feed("Content-Length: 999999\r\n\r\n{}") catch e => {
        if (e == @as(anyerror, err.FrameTooLarge)) {
            too_big_hit = true;
        }
    };
    expect_eq(too_big_hit, true);
}

fn testLineRpcFramer()! {
    const alloc = std.heap.page_allocator;

    const m1 = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"}";
    const m2 = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";

    const l1 = LineRpcFramer.encodeLine(alloc, m1) catch unreachable;
    defer alloc.free(l1);
    const l2 = LineRpcFramer.encodeLine(alloc, m2) catch unreachable;
    defer alloc.free(l2);

    expect_eq(l1[l1.len - 1], '\n');

    // Reject payload containing embedded newline
    let invalid_line_hit = false;
    * = LineRpcFramer.encodeLine(alloc, "{\"a\":1}\n{\"b\":2}") catch e => {
        if (e == @as(anyerror, err.InvalidLine)) {
            invalid_line_hit = true;
        }
    };
    expect_eq(invalid_line_hit, true);

    let framer = LineRpcFramer.init(alloc, 512);
    defer framer.deinit();

    // Split across chunks, include CRLF and blank lines
    const c1 = framer.feed(l1[0..15]) catch unreachable;
    defer LineRpcFramer.freeLines(alloc, c1);
    expect_eq(c1.len, 0 as usize);

    let rest_buf: [256]u8 = undefined;
    let rest_len: usize = 0;
    const rem1 = l1[15..];
    std.mem.copyForwards(u8, rest_buf[0..rem1.len], rem1);
    rest_len += rem1.len;
    const blank_crlf = "\r\n   \n";
    std.mem.copyForwards(u8, rest_buf[rest_len .. rest_len + blank_crlf.len], blank_crlf);
    rest_len += blank_crlf.len;
    std.mem.copyForwards(u8, rest_buf[rest_len .. rest_len + l2.len], l2);
    rest_len += l2.len;

    const c2 = framer.feed(rest_buf[0..rest_len]) catch unreachable;
    defer LineRpcFramer.freeLines(alloc, c2);
    expect_eq(c2.len, 2 as usize);
    expect_eq(std.mem.eql(u8, c2[0], m1), true);
    expect_eq(std.mem.eql(u8, c2[1], m2), true);
    expect_eq(framer.bufferedBytes(), 0 as usize);

    // Oversized line rejection
    let tiny_framer = LineRpcFramer.init(alloc, 10);
    defer tiny_framer.deinit();
    let over_hit = false;
    * = tiny_framer.feed("{\"too\":\"long_payload\"}\n") catch e => {
        if (e == @as(anyerror, err.FrameTooLarge)) {
            over_hit = true;
        }
    };
    expect_eq(over_hit, true);
}

fn testMcpServerLifecycle()! {
    const alloc = std.heap.page_allocator;

    let server = McpServer.init(alloc, McpServerInfo{
        name: "cp-agent-mcp",
        version: "0.9.0",
        protocol_version: "2024-11-05",
    });
    defer server.deinit();

    server.registerTool(McpToolSpec{
        name: "sum",
        description: "Add two numbers a and b",
        input_schema_json: "{\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"number\"},\"b\":{\"type\":\"number\"}}}",
    }, sumTool) catch unreachable;

    server.registerTool(McpToolSpec{
        name: "fail_tool",
        description: "Always fails",
    }, failingTool) catch unreachable;

    server.registerResource(McpResourceSpec{
        uri: "file:///workspace/spec.md",
        name: "CP Spec",
        mime_type: "text/markdown",
    }) catch unreachable;

    expect_eq(server.toolCount(), 2 as usize);
    expect_eq(server.resourceCount(), 1 as usize);
    expect_eq(server.isInitialized(), false);

    // 1. initialize
    const init_req = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2024-11-05\",\"clientInfo\":{\"name\":\"test-client\",\"version\":\"1.0\"}}}";
    const init_raw = (server.handleRequestJson(alloc, init_req) catch unreachable).?;
    defer alloc.free(init_raw);
    let init_resp = JsonRpcCodec.parseResponse(alloc, init_raw) catch unreachable;
    defer init_resp.deinit(alloc);
    expect_eq(init_resp.isSuccess(), true);
    expect_eq(init_resp.id.asInt().?, 1 as i64);
    const init_res = JsonParser.parse(alloc, init_resp.result_json.?) catch unreachable;
    expect_eq(std.mem.eql(u8, init_res.get("protocolVersion").?.asString().?, "2024-11-05"), true);
    expect_eq(std.mem.eql(u8, init_res.get("serverInfo").?.get("name").?.asString().?, "cp-agent-mcp"), true);

    // 2. notifications/initialized -> must return null
    const notif_req = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
    const notif_resp = server.handleRequestJson(alloc, notif_req) catch unreachable;
    expect_eq(notif_resp == null, true);
    expect_eq(server.isInitialized(), true);

    // 3. ping -> {}
    const ping_raw = (server.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":\"p1\",\"method\":\"ping\"}") catch unreachable).?;
    defer alloc.free(ping_raw);
    let ping_resp = JsonRpcCodec.parseResponse(alloc, ping_raw) catch unreachable;
    defer ping_resp.deinit(alloc);
    expect_eq(ping_resp.isSuccess(), true);
    expect_eq(std.mem.eql(u8, ping_resp.id.asStr().?, "p1"), true);
    expect_eq(std.mem.eql(u8, ping_resp.result_json.?, "{}"), true);

    // 4. tools/list
    const tlist_raw = (server.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}") catch unreachable).?;
    defer alloc.free(tlist_raw);
    let tlist_resp = JsonRpcCodec.parseResponse(alloc, tlist_raw) catch unreachable;
    defer tlist_resp.deinit(alloc);
    const tlist_res = JsonParser.parse(alloc, tlist_resp.result_json.?) catch unreachable;
    const tools_arr = tlist_res.get("tools").?.asArray().?;
    expect_eq(tools_arr.len, 2 as usize);
    expect_eq(std.mem.eql(u8, tools_arr[0].get("name").?.asString().?, "sum"), true);
    expect_eq(std.mem.eql(u8, tools_arr[0].get("inputSchema").?.get("type").?.asString().?, "object"), true);

    // 5. tools/call (success)
    const call_req = "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"sum\",\"arguments\":{\"a\":17,\"b\":25}}}";
    const call_raw = (server.handleRequestJson(alloc, call_req) catch unreachable).?;
    defer alloc.free(call_raw);
    let call_resp = JsonRpcCodec.parseResponse(alloc, call_raw) catch unreachable;
    defer call_resp.deinit(alloc);
    expect_eq(call_resp.isSuccess(), true);
    const call_res = JsonParser.parse(alloc, call_resp.result_json.?) catch unreachable;
    expect_eq(call_res.get("isError").?.asBool().?, false);
    const content0 = call_res.get("content").?.getIndex(0).?;
    expect_eq(std.mem.eql(u8, content0.get("type").?.asString().?, "text"), true);
    expect_eq(std.mem.eql(u8, content0.get("text").?.asString().?, "42"), true);

    // 6. tools/call (tool handler error -> isError: true)
    const fail_raw = (server.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"fail_tool\"}}") catch unreachable).?;
    defer alloc.free(fail_raw);
    let fail_resp = JsonRpcCodec.parseResponse(alloc, fail_raw) catch unreachable;
    defer fail_resp.deinit(alloc);
    expect_eq(fail_resp.isSuccess(), true);
    const fail_res = JsonParser.parse(alloc, fail_resp.result_json.?) catch unreachable;
    expect_eq(fail_res.get("isError").?.asBool().?, true);

    // 7. resources/list
    const rlist_raw = (server.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"resources/list\"}") catch unreachable).?;
    defer alloc.free(rlist_raw);
    let rlist_resp = JsonRpcCodec.parseResponse(alloc, rlist_raw) catch unreachable;
    defer rlist_resp.deinit(alloc);
    const rlist_res = JsonParser.parse(alloc, rlist_resp.result_json.?) catch unreachable;
    const res_arr = rlist_res.get("resources").?.asArray().?;
    expect_eq(res_arr.len, 1 as usize);
    expect_eq(std.mem.eql(u8, res_arr[0].get("uri").?.asString().?, "file:///workspace/spec.md"), true);
    expect_eq(std.mem.eql(u8, res_arr[0].get("mimeType").?.asString().?, "text/markdown"), true);

    // 8. Unknown method -> -32601 Method not found
    const unk_raw = (server.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"prompts/missing\"}") catch unreachable).?;
    defer alloc.free(unk_raw);
    let unk_resp = JsonRpcCodec.parseResponse(alloc, unk_raw) catch unreachable;
    defer unk_resp.deinit(alloc);
    expect_eq(unk_resp.isError(), true);
    expect_eq(unk_resp.id.asInt().?, 6 as i64);
    expect_eq(unk_resp.rpc_error.?.code, ERR_METHOD_NOT_FOUND);
}

fn testCapRpcLazyMount()! {
    const alloc = std.heap.page_allocator;

    let srv = cap.rpc.McpServer.init(alloc, cap.rpc.McpServerInfo{
        name: "lazy-mount-mcp",
        version: "1.0.0",
    });
    defer srv.deinit();

    const req_line = cap.rpc.LineRpcFramer.encodeLine(
        alloc,
        "{\"jsonrpc\":\"2.0\",\"id\":99,\"method\":\"unknown/op\"}",
    ) catch unreachable;
    defer alloc.free(req_line);

    let line_framer = cap.rpc.LineRpcFramer.init(alloc, 4096);
    defer line_framer.deinit();
    const lines = line_framer.feed(req_line) catch unreachable;
    defer cap.rpc.LineRpcFramer.freeLines(alloc, lines);
    expect_eq(lines.len, 1 as usize);

    const resp_json = (srv.handleRequestJson(alloc, lines[0]) catch unreachable).?;
    defer alloc.free(resp_json);

    const framed = cap.rpc.ContentLengthFramer.encodeFrame(alloc, resp_json) catch unreachable;
    defer alloc.free(framed);

    let cl_framer = cap.rpc.ContentLengthFramer.init(alloc, 4096);
    defer cl_framer.deinit();
    const frames = cl_framer.feed(framed) catch unreachable;
    defer cap.rpc.ContentLengthFramer.freeFrames(alloc, frames);
    expect_eq(frames.len, 1 as usize);

    let parsed = cap.rpc.JsonRpcCodec.parseResponse(alloc, frames[0]) catch unreachable;
    defer parsed.deinit(alloc);
    expect_eq(parsed.isError(), true);
    expect_eq(parsed.id.asInt().?, 99 as i64);
    expect_eq(parsed.rpc_error.?.code, cap.rpc.ERR_METHOD_NOT_FOUND);
}

fn main() {
    test("1. JSON-RPC 2.0 Request/Notification/Success/Standard Error Codes Round-Trip", () => {
        testJsonRpcCodecRoundTrip()?;
    });

    test("2. ContentLengthFramer Cross-Chunk Splits, Sticky Frames & DoS Max-Frame Guard", () => {
        testContentLengthFramer()?;
    });

    test("3. LineRpcFramer Stdio NDJSON Incremental Framing & Invalid Line Rejection", () => {
        testLineRpcFramer()?;
    });

    test("4. McpServer Full Lifecycle (initialize, ping, tools/list, tools/call, resources/list, -32601)", () => {
        testMcpServerLifecycle()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.rpc`", () => {
        testCapRpcLazyMount()?;
    });
}
