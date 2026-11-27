# @web/mcp — AI Agent Quick Ref

Official Model Context Protocol (MCP) Streamable HTTP JSON-RPC 2.0 server plugin for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class McpConfig { endpoint_path: string, server_name: string, server_version: string }
class McpToolEntry { name: string, description: string, handler: McpToolHandlerFn? }
```

## API Signatures

```cp
fn mcpBytesEql(a: string, b: string): bool;
fn mcpWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn mcpWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn mcpHexLower(n: u8): u8;
fn mcpWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
fn mcpSkipWs(s: string, start: usize): usize;
fn mcpSkipJsonValue(s: string, start: usize): usize;
fn mcpFindObjectFieldRaw(obj: string, target_key: string): string?;
fn mcpUnescapeJsonString(raw: string, out_buf: []u8): usize?;
fn mcpRegisterTool(name: string, description: string, handler: McpToolHandlerFn): bool;
fn mcpResetTools();
fn mcpFindTool(name: string): McpToolHandlerFn?;
fn mcpJsonRpcError(id_raw: string, code_str: string, message: string): Response;
fn mcpPostHandler(ctx: &mut Context): Response;
fn registerMcp(app: &mut App, cfg: McpConfig): &mut App;
```

## Minimal Usage

```cp
import { mcpBytesEql } from "@web/mcp";

pub fn main() {
    let result = mcpBytesEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/mcp"`; install via `cap add @web/mcp`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
