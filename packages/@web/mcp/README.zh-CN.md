[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/mcp

Official Model Context Protocol (MCP) Streamable HTTP JSON-RPC 2.0 server plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/mcp` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/mcp
```

```cp
import {
    McpConfig,
    McpToolEntry,
    mcpBytesEql,
    mcpWriteByte,
    mcpWriteRaw,
    mcpHexLower,
    mcpWriteJsonEscaped,
    mcpSkipWs,
} from "@web/mcp";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `McpConfig` | `class` | 字段: `endpoint_path: string, server_name: string, server_version: string` |
| `McpToolEntry` | `class` | 字段: `name: string, description: string, handler: McpToolHandlerFn?` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mcpBytesEql` | `(a: string, b: string): bool` | 调用 `mcpBytesEql` 并返回 `bool`。 |
| `mcpWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `mcpWriteByte` 并返回 `bool`。 |
| `mcpWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `mcpWriteRaw` 并返回 `bool`。 |
| `mcpHexLower` | `(n: u8): u8` | 调用 `mcpHexLower` 并返回 `u8`。 |
| `mcpWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `mcpWriteJsonEscaped` 并返回 `bool`。 |
| `mcpSkipWs` | `(s: string, start: usize): usize` | 调用 `mcpSkipWs` 并返回 `usize`。 |
| `mcpSkipJsonValue` | `(s: string, start: usize): usize` | 调用 `mcpSkipJsonValue` 并返回 `usize`。 |
| `mcpFindObjectFieldRaw` | `(obj: string, target_key: string): string?` | 调用 `mcpFindObjectFieldRaw` 并返回 `string?`。 |
| `mcpUnescapeJsonString` | `(raw: string, out_buf: []u8): usize?` | 调用 `mcpUnescapeJsonString` 并返回 `usize?`。 |
| `mcpRegisterTool` | `(name: string, description: string, handler: McpToolHandlerFn): bool` | 调用 `mcpRegisterTool` 并返回 `bool`。 |
| `mcpResetTools` | `()` | 就地执行 `mcpResetTools` 操作。 |
| `mcpFindTool` | `(name: string): McpToolHandlerFn?` | 调用 `mcpFindTool` 并返回 `McpToolHandlerFn?`。 |
| `mcpJsonRpcError` | `(id_raw: string, code_str: string, message: string): Response` | 调用 `mcpJsonRpcError` 并返回 `Response`。 |
| `mcpPostHandler` | `(ctx: &mut Context): Response` | 调用 `mcpPostHandler` 并返回 `Response`。 |
| `registerMcp` | `(app: &mut App, cfg: McpConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { mcpBytesEql } from "@web/mcp";

pub fn main() {
    let result = mcpBytesEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
