[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jsonrpc-core

Zero-heap JSON-RPC 2.0 request/notification/result/error encoder, envelope validator, batch detector, and standard error codes.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:jsonrpc-core` ([https://github.com/paritytech/jsonrpc](https://github.com/paritytech/jsonrpc)) / `npm:json-rpc-2.0` ([https://github.com/shawninder/json-rpc-2.0](https://github.com/shawninder/json-rpc-2.0)) / `crates.io:jsonrpsee` ([https://github.com/paritytech/jsonrpsee](https://github.com/paritytech/jsonrpsee))（原库名 `jsonrpc-core` → CP 包名 `jsonrpc-core`）

| Metadata | Value |
| --- | --- |
| Package | `jsonrpc-core` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add jsonrpc-core
```

```cp
import {
    isServerError,
    errorMessage,
    isWs,
    appendByte,
    appendSlice,
    appendI32,
    appendJsonString,
    encodeRequest,
} from "jsonrpc-core";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isServerError` | `(code: i32): bool` | Checks predicate `isServerError` and returns `bool`. |
| `errorMessage` | `(code: i32): string` | Invokes `errorMessage` and returns `string`. |
| `isWs` | `(c: u8): bool` | Checks predicate `isWs` and returns `bool`. |
| `appendByte` | `(out: []u8, pos: usize, b: u8): i64` | Invokes `appendByte` and returns `i64`. |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | Invokes `appendSlice` and returns `i64`. |
| `appendI32` | `(out: []u8, pos: usize, v: i32): i64` | Invokes `appendI32` and returns `i64`. |
| `appendJsonString` | `(out: []u8, pos: usize, s: string): i64` | Invokes `appendJsonString` and returns `i64`. |
| `encodeRequest` | `(id_json: string, method: string, params_json: string, out: []u8): usize` | Serializes/formats data via `encodeRequest` returning `usize`. |
| `encodeResult` | `(id_json: string, result_json: string, out: []u8): usize` | Serializes/formats data via `encodeResult` returning `usize`. |
| `encodeError` | `(id_json: string, code: i32, message: string, data_json: string, out: []u8): usize` | Serializes/formats data via `encodeError` returning `usize`. |
| `isBatch` | `(json: string): bool` | Checks predicate `isBatch` and returns `bool`. |
| `isValidEnvelope` | `(json: string): bool` | Checks predicate `isValidEnvelope` and returns `bool`. |

## Minimal Example

```cp
import { encodeResult } from "jsonrpc-core";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeResult("example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
