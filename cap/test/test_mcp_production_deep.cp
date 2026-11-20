// Source: cp/cap/test/test_mcp_production_deep.cp (Deep tests for cap:mcp & cap.mcp)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MCP_PROTOCOL_VERSION,
    JSONRPC_PARSE_ERROR,
    JSONRPC_INVALID_REQUEST,
    JSONRPC_METHOD_NOT_FOUND,
    JSONRPC_INVALID_PARAMS,
    McpToolDef,
    McpResourceDef,
    McpPromptDef,
    McpContent,
    McpToolResult,
    McpId,
    McpResponse,
    McpNotification,
    FrameMode,
    McpStreamFramer,
    McpServer,
    decodeRequest,
    encodeResponse,
    encodeNotification,
    extractJsonStringField,
} from "cap:mcp";

fn toolEcho(alloc: std.mem.Allocator, args_json: []const u8): anyerror!McpToolResult {
    const msg_opt = try extractJsonStringField(alloc, args_json, "msg");
    defer if (m := msg_opt) alloc.free(m);
    const msg = msg_opt orelse "empty";

    let items = try alloc.alloc(McpContent, 1);
    items[0] = McpContent.makeText(try alloc.dupe(u8, msg));
    return McpToolResult.ok(items);
}

fn toolFailing(_: std.mem.Allocator, _: []const u8): anyerror!McpToolResult {
    return error.BoomToolFailed;
}

fn readConfigResource(alloc: std.mem.Allocator, uri: []const u8): anyerror![]u8 {
    return try std.fmt.allocPrint(alloc, "resource:{s}", .{uri});
}

fn buildReviewPrompt(alloc: std.mem.Allocator, args_json: []const u8): anyerror![]u8 {
    const topic_opt = try extractJsonStringField(alloc, args_json, "topic");
    defer if (t := topic_opt) alloc.free(t);
    const topic = topic_opt orelse "general";
    return try std.fmt.allocPrint(alloc, "Review topic: {s}", .{topic});
}

test("mcp: initialize, ping, tools/list, tools/call and isError wrapping") {
    const alloc = std.testing.allocator;
    let srv = McpServer.init(alloc, "cp-mcp-srv", "0.1.0");
    defer srv.deinit();

    try srv.registerTool(McpToolDef{
        .name = "echo",
        .description = "Echo message",
    }, toolEcho);
    try srv.registerTool(McpToolDef{
        .name = "boom",
        .description = "Failing tool",
    }, toolFailing);

    const init_resp = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}")).?;
    defer alloc.free(init_resp);
    try expect_eq(std.mem.indexOf(u8, init_resp, MCP_PROTOCOL_VERSION) != null, true);
    try expect_eq(std.mem.indexOf(u8, init_resp, "\"tools\":{}") != null, true);

    const notif_resp = try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}");
    try expect_eq(notif_resp == null, true);

    const ping_resp = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":\"p-1\",\"method\":\"ping\"}")).?;
    defer alloc.free(ping_resp);
    try expect_eq(std.mem.indexOf(u8, ping_resp, "\"id\":\"p-1\"") != null, true);
    try expect_eq(std.mem.indexOf(u8, ping_resp, "\"result\":{}") != null, true);

    const list_resp = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}")).?;
    defer alloc.free(list_resp);
    try expect_eq(std.mem.indexOf(u8, list_resp, "\"name\":\"echo\"") != null, true);

    const call_resp = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\",\"arguments\":{\"msg\":\"hello\\nworld\"}}}")).?;
    defer alloc.free(call_resp);
    try expect_eq(std.mem.indexOf(u8, call_resp, "\"isError\":false") != null, true);
    try expect_eq(std.mem.indexOf(u8, call_resp, "hello\\nworld") != null, true);

    const fail_resp = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"boom\",\"arguments\":{}}}")).?;
    defer alloc.free(fail_resp);
    try expect_eq(std.mem.indexOf(u8, fail_resp, "\"isError\":true") != null, true);
    try expect_eq(std.mem.indexOf(u8, fail_resp, "BoomToolFailed") != null, true);
}

test("mcp: resources/list, resources/read and prompts/list, prompts/get") {
    const alloc = std.testing.allocator;
    let srv = McpServer.init(alloc, "cp-mcp-srv", "0.1.0");
    defer srv.deinit();

    try srv.registerResource(McpResourceDef{
        .uri = "file:///app/config.json",
        .name = "config",
        .description = "App config",
        .mime_type = "application/json",
    }, readConfigResource);
    try srv.registerPrompt(McpPromptDef{
        .name = "review",
        .description = "Code review prompt",
    }, buildReviewPrompt);

    const init_resp = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"}")).?;
    defer alloc.free(init_resp);

    const rlist = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/list\"}")).?;
    defer alloc.free(rlist);
    try expect_eq(std.mem.indexOf(u8, rlist, "file:///app/config.json") != null, true);

    const rread = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"resources/read\",\"params\":{\"uri\":\"file:///app/config.json\"}}")).?;
    defer alloc.free(rread);
    try expect_eq(std.mem.indexOf(u8, rread, "resource:file:///app/config.json") != null, true);

    const plist = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"prompts/list\"}")).?;
    defer alloc.free(plist);
    try expect_eq(std.mem.indexOf(u8, plist, "\"name\":\"review\"") != null, true);

    const pget = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"prompts/get\",\"params\":{\"name\":\"review\",\"arguments\":{\"topic\":\"mcp\"}}}")).?;
    defer alloc.free(pget);
    try expect_eq(std.mem.indexOf(u8, pget, "Review topic: mcp") != null, true);
}

test("mcp: McpStreamFramer NDJSON and Content-Length 1..7 byte chunked framing and max_frame_bytes") {
    const alloc = std.testing.allocator;

    const p1 = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}";
    const p2 = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}";

    const f1_nd = try McpStreamFramer.encodeFrame(alloc, FrameMode.Ndjson, p1);
    defer alloc.free(f1_nd);
    const f2_nd = try McpStreamFramer.encodeFrame(alloc, FrameMode.Ndjson, p2);
    defer alloc.free(f2_nd);

    const combined_nd = try std.fmt.allocPrint(alloc, "{s}{s}", .{ f1_nd, f2_nd });
    defer alloc.free(combined_nd);

    let nd_framer = McpStreamFramer.init(alloc, FrameMode.Ndjson, 1024);
    defer nd_framer.deinit();

    let collected_nd = std.ArrayList([]u8).empty;
    defer {
        for (item of collected_nd.items) alloc.free(item);
        collected_nd.deinit(alloc);
    }

    let pos: usize = 0;
    let step: usize = 1;
    while (pos < combined_nd.len) {
        const end = @min(pos + step, combined_nd.len);
        const batch = try nd_framer.feed(combined_nd[pos..end]);
        for (fr of batch) {
            try collected_nd.append(alloc, fr);
        }
        alloc.free(batch);
        pos = end;
        step = (step % 7) + 1;
    }
    try expect_eq(collected_nd.items.len, 2);
    try expect_eq(std.mem.eql(u8, collected_nd.items[0], p1), true);
    try expect_eq(std.mem.eql(u8, collected_nd.items[1], p2), true);

    const f1_cl = try McpStreamFramer.encodeFrame(alloc, FrameMode.ContentLength, p1);
    defer alloc.free(f1_cl);
    const f2_cl = try McpStreamFramer.encodeFrame(alloc, FrameMode.ContentLength, p2);
    defer alloc.free(f2_cl);

    const combined_cl = try std.fmt.allocPrint(alloc, "{s}{s}", .{ f1_cl, f2_cl });
    defer alloc.free(combined_cl);

    let cl_framer = McpStreamFramer.init(alloc, FrameMode.ContentLength, 1024);
    defer cl_framer.deinit();

    let collected_cl = std.ArrayList([]u8).empty;
    defer {
        for (item of collected_cl.items) alloc.free(item);
        collected_cl.deinit(alloc);
    }

    pos = 0;
    step = 3;
    while (pos < combined_cl.len) {
        const end = @min(pos + step, combined_cl.len);
        const batch = try cl_framer.feed(combined_cl[pos..end]);
        for (fr of batch) {
            try collected_cl.append(alloc, fr);
        }
        alloc.free(batch);
        pos = end;
        step = (step % 7) + 1;
    }
    try expect_eq(collected_cl.items.len, 2);
    try expect_eq(std.mem.eql(u8, collected_cl.items[0], p1), true);
    try expect_eq(std.mem.eql(u8, collected_cl.items[1], p2), true);

    let guard_framer = McpStreamFramer.init(alloc, FrameMode.ContentLength, 16);
    defer guard_framer.deinit();
    const oversized = guard_framer.feed("Content-Length: 100\r\n\r\n{}");
    try expect_eq(std.meta.isError(oversized), true);
}

test("mcp: invalid JSON-RPC, unknown method -32601, unknown tool -32602, uninitialized guard") {
    const alloc = std.testing.allocator;
    let srv = McpServer.init(alloc, "cp-mcp-srv", "0.1.0");
    defer srv.deinit();

    const uninit_resp = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}")).?;
    defer alloc.free(uninit_resp);
    try expect_eq(std.mem.indexOf(u8, uninit_resp, "-32600") != null, true);

    const parse_err = (try srv.handleMessage("{broken json")).?;
    defer alloc.free(parse_err);
    const parse_code_s = try std.fmt.allocPrint(alloc, "{d}", .{JSONRPC_PARSE_ERROR});
    defer alloc.free(parse_code_s);
    try expect_eq(std.mem.indexOf(u8, parse_err, parse_code_s) != null, true);

    const bad_req = (try srv.handleMessage("{\"jsonrpc\":\"1.0\",\"id\":1,\"method\":\"ping\"}")).?;
    defer alloc.free(bad_req);
    const inv_code_s = try std.fmt.allocPrint(alloc, "{d}", .{JSONRPC_INVALID_REQUEST});
    defer alloc.free(inv_code_s);
    try expect_eq(std.mem.indexOf(u8, bad_req, inv_code_s) != null, true);

    const init_resp = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"initialize\"}")).?;
    defer alloc.free(init_resp);

    const unknown_method = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"does/not/exist\"}")).?;
    defer alloc.free(unknown_method);
    const mnf_code_s = try std.fmt.allocPrint(alloc, "{d}", .{JSONRPC_METHOD_NOT_FOUND});
    defer alloc.free(mnf_code_s);
    try expect_eq(std.mem.indexOf(u8, unknown_method, mnf_code_s) != null, true);

    const unknown_tool = (try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"missing\"}}")).?;
    defer alloc.free(unknown_tool);
    const ip_code_s = try std.fmt.allocPrint(alloc, "{d}", .{JSONRPC_INVALID_PARAMS});
    defer alloc.free(ip_code_s);
    try expect_eq(std.mem.indexOf(u8, unknown_tool, ip_code_s) != null, true);
}

test("mcp: cap.mcp dynamic mount and std.testing.allocator zero-leak") {
    const alloc = std.testing.allocator;
    try expect_eq(std.mem.eql(u8, cap.mcp.MCP_PROTOCOL_VERSION, "2025-03-26"), true);

    let req = try decodeRequest(alloc, "{\"jsonrpc\":\"2.0\",\"id\":\"req-99\",\"method\":\"tools/call\",\"params\":{\"name\":\"t\"}}");
    defer req.deinit(alloc);
    try expect_eq(std.mem.eql(u8, req.method, "tools/call"), true);
    try expect_eq(std.mem.eql(u8, req.id.str_val, "req-99"), true);

    const resp_bytes = try encodeResponse(alloc, McpResponse{
        .id = McpId.fromString("req-99"),
        .result_json = "{\"ok\":true}",
    });
    defer alloc.free(resp_bytes);
    try expect_eq(std.mem.indexOf(u8, resp_bytes, "\"result\":{\"ok\":true}") != null, true);

    const notif_bytes = try encodeNotification(alloc, McpNotification{
        .method = "notifications/tools/list_changed",
    });
    defer alloc.free(notif_bytes);
    try expect_eq(std.mem.indexOf(u8, notif_bytes, "notifications/tools/list_changed") != null, true);
}
