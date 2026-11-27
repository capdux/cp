# @web/a2a — AI Agent Quick Ref

Official Google Agent-to-Agent (A2A) protocol Agent Card & JSON-RPC 2.0 task plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    A2aTaskState,
    A2aConfig,
    A2aTaskEntry,
    a2aBytesEql,
    a2aWriteByte,
    a2aWriteRaw,
    a2aWriteU64,
    a2aHexLower,
} from "@web/a2a";
```

## Core Types & Constants

```cp
enum A2aTaskState { Submitted, Working, Completed, Canceled, Failed }
class A2aConfig { agent_name: string, agent_description: string, agent_url: string, card_path: string, /* +2 fields */ }
class A2aTaskEntry { used: bool, id_buf: [A2A_TASK_ID_CAP]u8, id_len: usize, state: A2aTaskState, /* +2 fields */ }
```

## API Signatures

```cp
fn a2aBytesEql(a: string, b: string): bool;
fn a2aWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn a2aWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn a2aWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
fn a2aHexLower(n: u8): u8;
fn a2aWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
fn a2aSkipWs(s: string, start: usize): usize;
fn a2aSkipJsonValue(s: string, start: usize): usize;
fn a2aFindObjectFieldRaw(obj: string, target_key: string): string?;
fn a2aUnescapeJsonString(raw: string, out_buf: []u8): usize?;
fn a2aSetTaskHandler(handler: A2aTaskHandlerFn);
fn a2aResetTasks();
fn a2aFindTaskIndex(task_id: string): usize?;
fn a2aAllocTaskSlot(task_id: string): usize?;
fn a2aJsonRpcError(id_raw: string, code_str: string, message: string): Response;
fn a2aTaskJsonResponse(id_raw: string, idx: usize): Response;
fn a2aCardHandler(ctx: &mut Context): Response;
fn a2aRpcHandler(ctx: &mut Context): Response;
fn registerA2a(app: &mut App, cfg: A2aConfig): &mut App;
```

## Minimal Usage

```cp
import { a2aBytesEql } from "@web/a2a";

pub fn main() {
    let result = a2aBytesEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/a2a"`; install via `cap add @web/a2a`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
