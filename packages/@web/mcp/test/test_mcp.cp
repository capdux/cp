// Self-contained unit & integration tests for @web/mcp

const std = @import("std");
import { App, AppMethod, Context, Request } from "../../../../lib/web/app.cp";
import {
    McpConfig,
    mcpRegisterTool,
    mcpResetTools,
    registerMcp,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn contains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        let eq: bool = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                eq = false;
                break;
            }
        }
        if (eq) return true;
    }
    return false;
}

fn toolEcho(ctx: *Context, args_json: string): string {
    * = ctx;
    if (contains(args_json, "ping")) {
        return "pong";
    }
    return args_json;
}

fn toolTime(ctx: *Context, args_json: string): string {
    * = ctx;
    * = args_json;
    return "2026-11-25T00:00:00Z";
}

fn testMcpJsonRpcLifecycle() {
    mcpResetTools();
    assertTrue(mcpRegisterTool("echo", "Echoes input arguments", &toolEcho), "register echo tool");
    assertTrue(mcpRegisterTool("server_time", "Returns server timestamp", &toolTime), "register time tool");

    let app = App.init();
    const cfg = McpConfig{
        endpoint_path: "/mcp",
        server_name: "cp-agent-mcp",
        server_version: "2.0.0",
    };
    * = registerMcp(*app, cfg);

    // 1. initialize
    let req_init = Request.init(AppMethod.POST, "/mcp");
    * = req_init.withBody("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}");
    const resp_init = app.dispatch(req_init);
    assertTrue(resp_init.status == 200, "initialize status 200");
    assertTrue(contains(resp_init.body, "\"jsonrpc\":\"2.0\""), "init jsonrpc 2.0");
    assertTrue(contains(resp_init.body, "\"id\":1"), "init id 1");
    assertTrue(contains(resp_init.body, "\"protocolVersion\":\"2025-03-26\""), "init protocolVersion");
    assertTrue(contains(resp_init.body, "\"name\":\"cp-agent-mcp\""), "init server name");
    assertTrue(contains(resp_init.body, "\"version\":\"2.0.0\""), "init server version");
    assertTrue(contains(resp_init.body, "\"capabilities\":{\"tools\":{}}"), "init capabilities");

    // 2. tools/list
    let req_list = Request.init(AppMethod.POST, "/mcp");
    * = req_list.withBody("{\"jsonrpc\":\"2.0\",\"id\":\"req-list\",\"method\":\"tools/list\"}");
    const resp_list = app.dispatch(req_list);
    assertTrue(resp_list.status == 200, "tools/list status 200");
    assertTrue(contains(resp_list.body, "\"id\":\"req-list\""), "list string id preserved");
    assertTrue(contains(resp_list.body, "\"name\":\"echo\""), "list has echo tool");
    assertTrue(contains(resp_list.body, "\"description\":\"Echoes input arguments\""), "list has echo description");
    assertTrue(contains(resp_list.body, "\"name\":\"server_time\""), "list has server_time tool");

    // 3. tools/call (known tool)
    let req_call = Request.init(AppMethod.POST, "/mcp");
    * = req_call.withBody("{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\",\"arguments\":{\"msg\":\"ping\"}}}");
    const resp_call = app.dispatch(req_call);
    assertTrue(resp_call.status == 200, "tools/call status 200");
    assertTrue(contains(resp_call.body, "\"id\":3"), "call id 3");
    assertTrue(contains(resp_call.body, "\"content\":[{\"type\":\"text\",\"text\":\"pong\"}]"), "call returns MCP content text");

    // 4. tools/call (unknown tool -> JSON-RPC -32601)
    let req_unk = Request.init(AppMethod.POST, "/mcp");
    * = req_unk.withBody("{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"nonexistent\",\"arguments\":{}}}");
    const resp_unk = app.dispatch(req_unk);
    assertTrue(resp_unk.status == 200, "unknown tool returns JSON-RPC 200");
    assertTrue(contains(resp_unk.body, "\"code\":-32601"), "unknown tool returns -32601");

    // 5. unknown method -> JSON-RPC -32601
    let req_bad_method = Request.init(AppMethod.POST, "/mcp");
    * = req_bad_method.withBody("{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"resources/unknown\"}");
    const resp_bad_method = app.dispatch(req_bad_method);
    assertTrue(resp_bad_method.status == 200, "unknown method returns JSON-RPC 200");
    assertTrue(contains(resp_bad_method.body, "\"code\":-32601"), "unknown method returns -32601");
}

pub fn main() {
    testMcpJsonRpcLifecycle();
    std.debug.print("ALL @web/mcp TESTS PASSED!\n", .{});
}
