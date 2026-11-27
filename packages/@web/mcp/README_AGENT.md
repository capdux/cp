# @web/mcp@1.0.0 (Agent Quickref)

> Official Model Context Protocol (MCP) Streamable HTTP JSON-RPC 2.0 server plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/mcp";` | Full types: `src/index.d.cp`

## Functions
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
