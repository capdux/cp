[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/a2a

Official Google Agent-to-Agent (A2A) protocol Agent Card & JSON-RPC 2.0 task plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/a2a` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/a2a
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `A2aTaskState` | `Submitted, Working, Completed, Canceled, Failed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `A2aConfig` | `class` | Fields: `agent_name: string, agent_description: string, agent_url: string, card_path: string, rpc_path: string, version: string` |
| `A2aTaskEntry` | `class` | Fields: `used: bool, id_buf: [A2A_TASK_ID_CAP]u8, id_len: usize, state: A2aTaskState, result_buf: [A2A_TASK_RESULT_CAP]u8, result_len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `a2aBytesEql` | `(a: string, b: string): bool` | Invokes `a2aBytesEql` and returns `bool`. |
| `a2aWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `a2aWriteByte` and returns `bool`. |
| `a2aWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `a2aWriteRaw` and returns `bool`. |
| `a2aWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | Invokes `a2aWriteU64` and returns `bool`. |
| `a2aHexLower` | `(n: u8): u8` | Invokes `a2aHexLower` and returns `u8`. |
| `a2aWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `a2aWriteJsonEscaped` and returns `bool`. |
| `a2aSkipWs` | `(s: string, start: usize): usize` | Invokes `a2aSkipWs` and returns `usize`. |
| `a2aSkipJsonValue` | `(s: string, start: usize): usize` | Invokes `a2aSkipJsonValue` and returns `usize`. |
| `a2aFindObjectFieldRaw` | `(obj: string, target_key: string): string?` | Invokes `a2aFindObjectFieldRaw` and returns `string?`. |
| `a2aUnescapeJsonString` | `(raw: string, out_buf: []u8): usize?` | Invokes `a2aUnescapeJsonString` and returns `usize?`. |
| `a2aSetTaskHandler` | `(handler: A2aTaskHandlerFn)` | Executes `a2aSetTaskHandler` in-place. |
| `a2aResetTasks` | `()` | Executes `a2aResetTasks` in-place. |
| `a2aFindTaskIndex` | `(task_id: string): usize?` | Invokes `a2aFindTaskIndex` and returns `usize?`. |
| `a2aAllocTaskSlot` | `(task_id: string): usize?` | Invokes `a2aAllocTaskSlot` and returns `usize?`. |
| `a2aJsonRpcError` | `(id_raw: string, code_str: string, message: string): Response` | Invokes `a2aJsonRpcError` and returns `Response`. |
| `a2aTaskJsonResponse` | `(id_raw: string, idx: usize): Response` | Invokes `a2aTaskJsonResponse` and returns `Response`. |
| `a2aCardHandler` | `(ctx: &mut Context): Response` | Invokes `a2aCardHandler` and returns `Response`. |
| `a2aRpcHandler` | `(ctx: &mut Context): Response` | Invokes `a2aRpcHandler` and returns `Response`. |
| `registerA2a` | `(app: &mut App, cfg: A2aConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { a2aBytesEql } from "@web/a2a";

pub fn main() {
    let result = a2aBytesEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
