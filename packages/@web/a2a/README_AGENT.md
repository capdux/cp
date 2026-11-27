# @web/a2a@1.0.0 (Agent Quickref)

> Official Google Agent-to-Agent (A2A) protocol Agent Card & JSON-RPC 2.0 task plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/a2a";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum A2aTaskState`

## Functions
- `export fn a2aBytesEql(a: string, b: string): bool;`
- `export fn a2aWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn a2aWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn a2aWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;`
- `export fn a2aHexLower(n: u8): u8;`
- `export fn a2aWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn a2aSkipWs(s: string, start: usize): usize;`
- `export fn a2aSkipJsonValue(s: string, start: usize): usize;`
- `export fn a2aFindObjectFieldRaw(obj: string, target_key: string): string?;`
- `export fn a2aUnescapeJsonString(raw: string, out_buf: []u8): usize?;`
- `export fn a2aSetTaskHandler(handler: A2aTaskHandlerFn);`
- `export fn a2aResetTasks();`
- `export fn a2aFindTaskIndex(task_id: string): usize?;`
- `export fn a2aAllocTaskSlot(task_id: string): usize?;`
- `export fn a2aJsonRpcError(id_raw: string, code_str: string, message: string): Response;`
- `export fn a2aTaskJsonResponse(id_raw: string, idx: usize): Response;`
- `export fn a2aCardHandler(ctx: &mut Context): Response;`
- `export fn a2aRpcHandler(ctx: &mut Context): Response;`
- `export fn registerA2a(app: &mut App, cfg: A2aConfig): &mut App;`
