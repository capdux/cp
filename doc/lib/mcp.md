# `cap:mcp` — Model Context Protocol (MCP 2025-03-26) 标准库

## 1. 概述

`cap:mcp`（及动态挂载 `cap.mcp`）是纯 CP 实现的工业级 Model Context Protocol (`2025-03-26`) 协议栈与 Stdio 增量分帧引擎，包含三层核心能力：
- **协议编解码 (`protocol.cp`)**：JSON-RPC 2.0 请求/响应/通知模型，支持字符串与整数 `id`、RFC 8259 字符串转义/反转义及标准错误码。
- **增量传输分帧 (`framer.cp`)**：`McpStreamFramer` 支持 `Ndjson` 与 LSP 风格 `Content-Length: N\r\n\r\n` 双模式，支持任意 `1..7` 字节半包/黏包跨 chunk `feed()` 拆帧与 `max_frame_bytes` 上限拦截。
- **能力路由器 (`server.cp`)**：`McpServer` 内置 `initialize`、`notifications/initialized`、`ping`、`tools/list`、`tools/call`、`resources/list`、`resources/read`、`prompts/list`、`prompts/get` 路由分发，并自动将工具执行错误封装为 `isError: true`。
- **零堆分配工具注册表与权限治理 (`registry.cp`)**：`McpToolRegistry` 基于调用方固定槽位缓冲实现工具注册、能力风险分级（`ReadOnly`/`WriteLocal`/`Network`/`Destructive`）、网络 ACL、高危人工审批门槛（`RequireApproval`）、固定 60 秒窗口限流与动态 `tools/list` 可见集过滤。

## 2. 核心类型

- **常量**：`MCP_PROTOCOL_VERSION` (`"2025-03-26"`)、`JSONRPC_PARSE_ERROR` (`-32700`)、`JSONRPC_INVALID_REQUEST` (`-32600`)、`JSONRPC_METHOD_NOT_FOUND` (`-32601`)、`JSONRPC_INVALID_PARAMS` (`-32602`)、`JSONRPC_INTERNAL_ERROR` (`-32603`)
- **协议模型**：`McpError`、`McpCapabilities`、`McpToolDef`、`McpResourceDef`、`McpPromptDef`、`McpContent`、`McpToolResult`、`McpIdKind`、`McpId`、`McpRequest`、`McpResponse`、`McpNotification`
- **分帧器**：`FrameMode` (`Ndjson` | `ContentLength`)、`McpStreamFramer`
- **服务端**：`ToolHandler`、`ResourceReader`、`PromptBuilder`、`McpServer`
- **工具治理注册表 (`registry.cp`)**：`ToolRiskLevel`、`ToolPolicy`、`PolicyDecision`、`RegisteredTool`、`McpToolRegistry`

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
| `McpToolRegistry.init(slots)` | 使用调用方固定 `RegisteredTool` 槽位缓冲初始化零堆分配注册表 |
| `reg.register(name, desc, risk, requires_network, rate_limit_per_min)` | 注册工具（拒绝空名、重名与池满），`rate_limit_per_min == 0` 表示不限流 |
| `reg.findIndex(name)` / `reg.get(index)` | 按名称查找槽位索引 / 读取只读副本 |
| `reg.setEnabled(name, enabled)` | 动态启用或禁用指定工具 |
| `reg.authorizeAndConsume(name, policy, now_ms)` | 综合校验存在性、启用状态、网络 ACL、风险分级、高危人工审批与 60s 速率限制，仅 `Allow` 时累加调用计数 |
| `reg.recordError(name)` | 累加指定工具的错误计数 |
| `reg.listAllowedTools(policy, out_indices)` | 按 `ToolPolicy` 过滤出可见工具索引，用于动态构造 `tools/list` |

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

### AI Agent 工具权限治理与限流示例 (`registry.cp`)

```cp
import {
    ToolRiskLevel,
    ToolPolicy,
    PolicyDecision,
    RegisteredTool,
    McpToolRegistry,
} from "cap:mcp";

pub fn demoAgentToolGovernance() {
    let slots_buf: [8]RegisteredTool = [RegisteredTool{}; 8];
    let slots: []RegisteredTool = slots_buf[0..];
    let reg = McpToolRegistry.init(slots);

    * = reg.register("read_file", "Read workspace file", ToolRiskLevel.ReadOnly, false, 60);
    * = reg.register("web_fetch", "Fetch external URL", ToolRiskLevel.Network, true, 10);
    * = reg.register("drop_table", "Drop SQL table", ToolRiskLevel.Destructive, false, 2);

    // 沙箱 Agent 策略：仅允许本地只读，禁止网络
    const sandbox_policy = ToolPolicy{
        max_risk: ToolRiskLevel.ReadOnly,
        require_approval_for_destructive: true,
        allow_network: false,
    };

    const d1 = reg.authorizeAndConsume("read_file", sandbox_policy, 1000); // PolicyDecision.Allow
    const d2 = reg.authorizeAndConsume("web_fetch", sandbox_policy, 1000); // PolicyDecision.DenyNetwork
    const d3 = reg.authorizeAndConsume("drop_table", sandbox_policy, 1000); // PolicyDecision.DenyRisk

    // 动态过滤当前 Agent 可见的 tools/list
    let visible_buf: [8]usize = [0; 8];
    let visible: []usize = visible_buf[0..];
    const visible_count = reg.listAllowedTools(sandbox_policy, visible);
    * = d1;
    * = d2;
    * = d3;
    * = visible_count;
}
```
