[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/mcp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official Model Context Protocol (MCP) Streamable HTTP JSON-RPC 2.0 server plugin for cap:web

## 安装与导入

```bash
cap add @web/mcp
```

```cp
import { mcpBytesEql } from "@web/mcp";
```

## 核心导出 API

- `export fn mcpBytesEql(a: string, b: string): bool;`
- `export fn mcpWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn mcpWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn mcpHexLower(n: u8): u8;`
- `export fn mcpWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn mcpSkipWs(s: string, start: usize): usize;`
- `export fn mcpSkipJsonValue(s: string, start: usize): usize;`
- `export fn mcpFindObjectFieldRaw(obj: string, target_key: string): string?;`
- `export fn mcpUnescapeJsonString(raw: string, out_buf: []u8): usize?;`
- `export fn mcpRegisterTool(name: string, description: string, handler: McpToolHandlerFn): bool;`
- `export fn mcpResetTools();`
- `export fn mcpFindTool(name: string): McpToolHandlerFn?;`
- `export fn mcpJsonRpcError(id_raw: string, code_str: string, message: string): Response;`
- `export fn mcpPostHandler(ctx: &mut Context): Response;`
- `export fn registerMcp(app: &mut App, cfg: McpConfig): &mut App;`
