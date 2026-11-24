# CP 标准库：JSON-RPC 2.0 与 MCP 协议引擎手册 (`cap:rpc`)

## 一、 架构总览与设计准则

`cap:rpc` 是 CP 语言官方内置的纯 CP 协议引擎标准库（**零 `std.posix`、零 `@import("bun")`**），专为百万级 AI Agent 工具调用、LSP / DAP 语言服务器与分布式 RPC 打造。支持 `import { ... } from "cap:rpc"` 直接导入，以及 `import cap from "cap"; cap.rpc.*` 自动动态挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    AI Agent / LSP / MCP 协议栈 (cap:rpc)                 │
├───────────────────┬─────────────────────┬───────────────────────────────┤
│ 传输帧编解码层      │ JSON-RPC 2.0 线缆层 │ MCP 服务端分发层               │
│ ContentLengthFramer│ JsonRpcCodec        │ McpServer                     │
│ LineRpcFramer     │ JsonRpcRequest/Resp │ initialize / ping             │
│ DoS 上限防线       │ 标准错误码 -32700~  │ tools/list, tools/call        │
│                   │ -32603              │ resources/list                │
└───────────────────┴─────────────────────┴───────────────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/rpc/jsonrpc.cp` | `// Reference-hardened against paritytech/jsonrpsee (MIT) & JSON-RPC 2.0 / MCP` | JSON-RPC 2.0 请求/通知/响应/错误码编解码与严格校验（零 `zig {}`） |
| `lib/rpc/framing.cp` | `// Reference-hardened against paritytech/jsonrpsee (MIT) & JSON-RPC 2.0 / MCP` | `Content-Length` 与 NDJSON 增量流式分帧器、16 MiB 上限防护与 16-lane `RpcVec16u8` SIMD（`simdRpcFindCrlf` / `simdRpcAsciiLowerEql`） |
| `lib/rpc/mcp.cp` | `// Reference-hardened against paritytech/jsonrpsee (MIT) & JSON-RPC 2.0 / MCP` | MCP 核心服务端路由器、工具/资源注册上限（`MCP_MAX_TOOLS=256` / `MCP_MAX_RESOURCES=256`）与标准错误码映射 |
| `lib/rpc/index.cp` | `// Source: cp/lib/rpc/index.cp (Unified Exports for cap:rpc)` | `cap:rpc` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. JSON-RPC 2.0 编解码与标准错误码（`jsonrpc.cp`）

- **标准错误码常量**：
  - `ERR_PARSE = -32700`
  - `ERR_INVALID_REQUEST = -32600`
  - `ERR_METHOD_NOT_FOUND = -32601`
  - `ERR_INVALID_PARAMS = -32602`
  - `ERR_INTERNAL = -32603`
- **核心类型**：
  - `JsonRpcId`：标签联合体 `Null` / `Int: i64` / `Str: string`，提供 `isNull()`、`asInt()`、`asStr()`、`equals()`、`clone()`、`deinit()`。
  - `JsonRpcError`：`code: i32`、`message: string`、`data_json: string?`，提供 `init()` 及 `parseError()`、`invalidRequest()`、`methodNotFound()`、`invalidParams()`、`internalError()` 工厂方法。
  - `JsonRpcRequest`：`id: JsonRpcId`、`method: string`、`params_json: string`、`has_id: bool`，提供 `isNotification()` 与 `deinit()`。
  - `JsonRpcResponse`：`id: JsonRpcId`、`result_json: string?`、`rpc_error: JsonRpcError?`，提供 `isSuccess()`、`isError()`、`deinit()`。
- **`JsonRpcCodec`**：
  - `parseRequest(alloc, json_str): JsonRpcRequest!`
  - `encodeRequest(alloc, req): ([]u8)!`
  - `encodeSuccess(alloc, id, result_json): ([]u8)!`
  - `encodeError(alloc, id, rpc_err): ([]u8)!`
  - `parseResponse(alloc, json_str): JsonRpcResponse!`

### 2. 增量流式分帧器（`framing.cp`）

- **`ContentLengthFramer`**（LSP / DAP / HTTP-style）：
  - `init(alloc, max_frame_bytes = 16 MiB)`、`deinit()`、`reset()`、`bufferedBytes()`
  - `feed(chunk): ([][]u8)!`：支持任意字节切分与多帧粘包，大小写不敏感解析 `Content-Length: N\r\n...\r\n\r\n<payload>`，超限提前返回 `FramingError.FrameTooLarge` 或 `FramingError.HeaderTooLarge`。
  - `ContentLengthFramer.encodeFrame(alloc, payload): ([]u8)!`
  - `ContentLengthFramer.freeFrames(alloc, frames)`
- **`LineRpcFramer`**（MCP stdio NDJSON）：
  - `init(alloc, max_frame_bytes = 16 MiB)`、`deinit()`、`reset()`、`bufferedBytes()`
  - `feed(chunk): ([][]u8)!`：兼容 `\n` 与 `\r\n` 行边界，忽略空行，校验单帧大小上限。
  - `LineRpcFramer.encodeLine(alloc, payload): ([]u8)!`（拒绝含 `\r`/`\n` 的非法帧）
  - `LineRpcFramer.freeLines(alloc, lines)`

### 3. MCP 核心服务端分发器（`mcp.cp`）

- **`McpToolSpec`**：`name`、`description`、`input_schema_json`
- **`McpResourceSpec`**：`uri`、`name`、`mime_type`、`description`
- **`McpServerInfo`**：`name`、`version`、`protocol_version`
- **`McpToolHandler`**：`fn (alloc: std.mem.Allocator, args_json: []const u8): ([]const u8)!`
- **`McpServer`**：
  - `init(alloc, info)`、`deinit()`
  - `registerTool(spec, handler_fn)`、`registerResource(spec)`
  - `toolCount()`、`resourceCount()`、`isInitialized()`
  - `handleRequestJson(alloc, req_json): ([]u8)?!`：
    - 通知（如 `notifications/initialized`）返回 `null`；
    - 自动路由 `initialize`、`ping`、`tools/list`、`tools/call`、`resources/list`；
    - 未知方法返回标准 `-32601 Method not found`；畸形 JSON 返回 `-32700`；未知工具或非法参数返回 `-32602`；工具内部错误封装为 MCP `isError: true`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { McpServer, McpServerInfo, McpToolSpec, McpResourceSpec } from "cap:rpc";
import { JsonParser } from "cap:json";
import cap from "cap";

fn addTool(alloc: std.mem.Allocator, args_json: []const u8): ([]const u8)! {
    const val = JsonParser.parse(alloc, args_json)?;
    const a = val.get("a")?.asNumber() ?? 0.0;
    const b = val.get("b")?.asNumber() ?? 0.0;
    const sum = (a + b) as i64;
    let out: []u8 = [];
    zig {
        out = try std.fmt.allocPrint(alloc, "{d}", .{sum});
    }
    return out;
}

pub fn demoMcpServer(): ! {
    const alloc = std.heap.page_allocator;
    let server = McpServer.init(alloc, McpServerInfo{ name: "agent-tools", version: "1.0.0" });
    defer server.deinit();

    server.registerTool(McpToolSpec{
        name: "add",
        description: "Add two numbers",
        input_schema_json: "{\"type\":\"object\",\"properties\":{\"a\":{\"type\":\"number\"},\"b\":{\"type\":\"number\"}}}",
    }, addTool)?;

    server.registerResource(McpResourceSpec{
        uri: "file:///workspace/README.md",
        name: "README",
        mime_type: "text/markdown",
    })?;

    if (resp := server.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"add\",\"arguments\":{\"a\":2,\"b\":3}}}")?) {
        defer alloc.free(resp);
        * = resp;
    }

    // 动态懒挂载调用
    const frame = cap.rpc.LineRpcFramer.encodeLine(alloc, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}")?;
    defer alloc.free(frame);
}
```
