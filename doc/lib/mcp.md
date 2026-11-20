# `cap:mcp` — Model Context Protocol (MCP 2025-03-26) 标准库

## 1. 概述

`cap:mcp`（及动态挂载 `cap.mcp`）是纯 CP 实现的工业级 Model Context Protocol (`2025-03-26`) 协议栈与 Stdio 增量分帧引擎，包含三层核心能力：
- **协议编解码 (`protocol.cp`)**：JSON-RPC 2.0 请求/响应/通知模型，支持字符串与整数 `id`、RFC 8259 字符串转义/反转义及标准错误码。
- **增量传输分帧 (`framer.cp`)**：`McpStreamFramer` 支持 `Ndjson` 与 LSP 风格 `Content-Length: N\r\n\r\n` 双模式，支持任意 `1..7` 字节半包/黏包跨 chunk `feed()` 拆帧与 `max_frame_bytes` 上限拦截。
- **能力路由器 (`server.cp`)**：`McpServer` 内置 `initialize`、`notifications/initialized`、`ping`、`tools/list`、`tools/call`、`resources/list`、`resources/read`、`prompts/list`、`prompts/get` 路由分发，并自动将工具执行错误封装为 `isError: true`。

## 2. 核心类型

- **常量**：`MCP_PROTOCOL_VERSION` (`"2025-03-26"`)、`JSONRPC_PARSE_ERROR` (`-32700`)、`JSONRPC_INVALID_REQUEST` (`-32600`)、`JSONRPC_METHOD_NOT_FOUND` (`-32601`)、`JSONRPC_INVALID_PARAMS` (`-32602`)、`JSONRPC_INTERNAL_ERROR` (`-32603`)
- **协议模型**：`McpError`、`McpCapabilities`、`McpToolDef`、`McpResourceDef`、`McpPromptDef`、`McpContent`、`McpToolResult`、`McpIdKind`、`McpId`、`McpRequest`、`McpResponse`、`McpNotification`
- **分帧器**：`FrameMode` (`Ndjson` | `ContentLength`)、`McpStreamFramer`
- **服务端**：`ToolHandler`、`ResourceReader`、`PromptBuilder`、`McpServer`

## 3. API 速查

| 符号 | 签名/说明 |
| :--- | :--- |
| `decodeRequest(alloc, json_bytes)` | 解析并校验 JSON-RPC 2.0 请求/通知，返回 `!McpRequest` |
| `encodeResponse(alloc, resp)` | 编码 JSON-RPC 2.0 成功或错误响应，返回 `![]u8` |
| `encodeErrorResponse(alloc, id, code, message, data)` | 编码标准 JSON-RPC 错误响应，返回 `![]u8` |
| `encodeNotification(alloc, notif)` | 编码 JSON-RPC 2.0 通知报文，返回 `![]u8` |
| `McpStreamFramer.init(alloc, mode, max_frame_bytes)` | 初始化增量分帧器 |
| `framer.feed(chunk)` | 喂入任意长度字节切片并提取完整载荷帧，超限返回 `error.FrameTooLarge` |
| `McpStreamFramer.encodeFrame(alloc, mode, payload)` | 按 `Ndjson` 或 `ContentLength` 模式封装输出帧 |
| `McpServer.init(alloc, name, version)` | 创建 MCP 服务端实例 |
| `server.registerTool(def, handler)` | 注册工具定义与处理函数 |
| `server.registerResource(def, reader)` | 注册资源定义与读取函数 |
| `server.registerPrompt(def, builder)` | 注册提示词模板与构造函数 |
| `server.handleMessage(raw_json)` | 路由单条 JSON-RPC 报文，返回 `!([ ]u8)?`（通知返回 `null`） |

## 4. 示例

```cp
const std = @import("std");
import { McpServer, McpToolDef, McpContent, McpToolResult } from "cap:mcp";

fn pingTool(alloc: std.mem.Allocator, _: []const u8): anyerror!McpToolResult {
    const items = try alloc.alloc(McpContent, 1);
    items[0] = McpContent.makeText(try alloc.dupe(u8, "pong"));
    return McpToolResult.ok(items);
}

pub fn example(alloc: std.mem.Allocator): !void {
    let srv = McpServer.init(alloc, "demo-agent", "1.0.0");
    defer srv.deinit();
    try srv.registerTool(McpToolDef{ .name = "ping", .description = "Ping tool" }, pingTool);
    if (resp := try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}")) {
        alloc.free(resp);
    }
}
```
