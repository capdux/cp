[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/mcp

Official Model Context Protocol (MCP) Streamable HTTP JSON-RPC 2.0 server plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/mcp` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `McpConfig` | `class` | Fields: `endpoint_path: string, server_name: string, server_version: string` |
| `McpToolEntry` | `class` | Fields: `name: string, description: string, handler: McpToolHandlerFn?` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mcpBytesEql` | `(a: string, b: string): bool` | Invokes `mcpBytesEql` and returns `bool`. |
| `mcpWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `mcpWriteByte` and returns `bool`. |
| `mcpWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `mcpWriteRaw` and returns `bool`. |
| `mcpHexLower` | `(n: u8): u8` | Invokes `mcpHexLower` and returns `u8`. |
| `mcpWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `mcpWriteJsonEscaped` and returns `bool`. |
| `mcpSkipWs` | `(s: string, start: usize): usize` | Invokes `mcpSkipWs` and returns `usize`. |
| `mcpSkipJsonValue` | `(s: string, start: usize): usize` | Invokes `mcpSkipJsonValue` and returns `usize`. |
| `mcpFindObjectFieldRaw` | `(obj: string, target_key: string): string?` | Invokes `mcpFindObjectFieldRaw` and returns `string?`. |
| `mcpUnescapeJsonString` | `(raw: string, out_buf: []u8): usize?` | Invokes `mcpUnescapeJsonString` and returns `usize?`. |
| `mcpRegisterTool` | `(name: string, description: string, handler: McpToolHandlerFn): bool` | Invokes `mcpRegisterTool` and returns `bool`. |
| `mcpResetTools` | `()` | Executes `mcpResetTools` in-place. |
| `mcpFindTool` | `(name: string): McpToolHandlerFn?` | Invokes `mcpFindTool` and returns `McpToolHandlerFn?`. |
| `mcpJsonRpcError` | `(id_raw: string, code_str: string, message: string): Response` | Invokes `mcpJsonRpcError` and returns `Response`. |
| `mcpPostHandler` | `(ctx: &mut Context): Response` | Invokes `mcpPostHandler` and returns `Response`. |
| `registerMcp` | `(app: &mut App, cfg: McpConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { mcpBytesEql } from "@web/mcp";

pub fn main() {
    let result = mcpBytesEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
